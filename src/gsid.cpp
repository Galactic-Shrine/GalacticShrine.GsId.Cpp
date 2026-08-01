#include <galactic_shrine/gsid/gsid.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstring>
#include <fstream>
#include <limits>
#include <mutex>
#include <ostream>
#include <string>
#include <system_error>

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <bcrypt.h>
#elif defined(__APPLE__)
    #include <Security/Security.h>
#elif defined(__linux__)
    #include <sys/random.h>
#endif

namespace GalacticShrine
{
    namespace
    {
        constexpr std::string_view UpperHexAlphabet = "0123456789ABCDEF";
        constexpr std::string_view LowerHexAlphabet = "0123456789abcdef";
        constexpr std::array<std::size_t, GsIdConstants::HyphenCount> HyphenPositions{
            16, 25, 34, 43, 52
        };

        struct OptionsState final
        {
            std::mutex Mutex;
            GsIdOptionsValues Values{};
            bool Locked = false;
        };

        OptionsState& GetOptionsState()
        {
            static OptionsState state;
            return state;
        }

        void EnsureUnlocked(const OptionsState& state)
        {
            if (state.Locked)
            {
                throw GsIdException(
                    "Les options GsId sont verrouillées et ne peuvent plus être modifiées.");
            }
        }

        void ValidateFormat(GsIdFormat format)
        {
            if (format != GsIdFormat::N && format != GsIdFormat::D)
            {
                throw GsIdException("Le format GsId demandé n'est pas supporté.");
            }
        }

        [[nodiscard]] bool IsAsciiSpace(unsigned char character) noexcept
        {
            return std::isspace(character) != 0;
        }

        [[nodiscard]] std::string_view Trim(std::string_view value) noexcept
        {
            while (!value.empty() && IsAsciiSpace(static_cast<unsigned char>(value.front())))
            {
                value.remove_prefix(1);
            }

            while (!value.empty() && IsAsciiSpace(static_cast<unsigned char>(value.back())))
            {
                value.remove_suffix(1);
            }

            return value;
        }

        [[nodiscard]] bool IsHexCharacter(char character) noexcept
        {
            return (character >= '0' && character <= '9')
                || (character >= 'a' && character <= 'f')
                || (character >= 'A' && character <= 'F');
        }

        [[nodiscard]] char ApplyCase(char character, GsIdCase letterCase) noexcept
        {
            if (character >= 'a' && character <= 'f' && letterCase == GsIdCase::Upper)
            {
                return static_cast<char>(character - ('a' - 'A'));
            }

            if (character >= 'A' && character <= 'F' && letterCase == GsIdCase::Lower)
            {
                return static_cast<char>(character + ('a' - 'A'));
            }

            return character;
        }

        [[nodiscard]] std::uint8_t ConvertHexCharacter(char character)
        {
            if (character >= '0' && character <= '9')
            {
                return static_cast<std::uint8_t>(character - '0');
            }

            if (character >= 'a' && character <= 'f')
            {
                return static_cast<std::uint8_t>(10 + character - 'a');
            }

            if (character >= 'A' && character <= 'F')
            {
                return static_cast<std::uint8_t>(10 + character - 'A');
            }

            throw GsIdException(
                std::string("Le caractère '") + character + "' n'est pas hexadécimal.");
        }

        [[nodiscard]] bool IsHyphenPosition(std::size_t index) noexcept
        {
            return std::find(HyphenPositions.begin(), HyphenPositions.end(), index)
                != HyphenPositions.end();
        }

        void FillSecureRandom(std::span<std::uint8_t> destination)
        {
#if defined(_WIN32)
            if (destination.size() > static_cast<std::size_t>(std::numeric_limits<ULONG>::max()))
            {
                throw GsIdException("Le tampon demandé est trop grand pour BCryptGenRandom.");
            }

            const NTSTATUS status = BCryptGenRandom(
                nullptr,
                reinterpret_cast<PUCHAR>(destination.data()),
                static_cast<ULONG>(destination.size()),
                BCRYPT_USE_SYSTEM_PREFERRED_RNG);

            if (status < 0)
            {
                throw GsIdException("BCryptGenRandom n'a pas pu générer un GsId sécurisé.");
            }
#elif defined(__APPLE__)
            const int status = SecRandomCopyBytes(
                kSecRandomDefault,
                destination.size(),
                destination.data());

            if (status != errSecSuccess)
            {
                throw GsIdException("SecRandomCopyBytes n'a pas pu générer un GsId sécurisé.");
            }
#elif defined(__linux__)
            std::size_t offset = 0;

            while (offset < destination.size())
            {
                const ssize_t received = ::getrandom(
                    destination.data() + offset,
                    destination.size() - offset,
                    0);

                if (received > 0)
                {
                    offset += static_cast<std::size_t>(received);
                    continue;
                }

                if (received < 0 && errno == EINTR)
                {
                    continue;
                }

                throw GsIdException(
                    std::string("getrandom n'a pas pu générer un GsId sécurisé : ")
                    + std::strerror(errno));
            }
#else
            std::ifstream source("/dev/urandom", std::ios::binary);

            if (!source)
            {
                throw GsIdException(
                    "Aucune source aléatoire cryptographiquement sûre n'est disponible.");
            }

            source.read(
                reinterpret_cast<char*>(destination.data()),
                static_cast<std::streamsize>(destination.size()));

            if (source.gcount() != static_cast<std::streamsize>(destination.size()))
            {
                throw GsIdException("/dev/urandom n'a pas retourné assez d'octets.");
            }
#endif
        }

        [[nodiscard]] std::array<char, GsIdConstants::HexLength> EncodeNormalized(
            const GsId::ByteArray& bytes,
            GsIdCase letterCase) noexcept
        {
            const std::string_view alphabet =
                letterCase == GsIdCase::Lower ? LowerHexAlphabet : UpperHexAlphabet;

            std::array<char, GsIdConstants::HexLength> result{};

            for (std::size_t index = 0; index < bytes.size(); ++index)
            {
                const std::uint8_t value = bytes[index];
                result[index * 2] = alphabet[value >> 4];
                result[(index * 2) + 1] = alphabet[value & 0x0F];
            }

            return result;
        }
    }

    GsIdCase GsIdOptions::GetDefaultCase()
    {
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        return state.Values.DefaultCase;
    }

    GsIdFormat GsIdOptions::GetDefaultTextFormat()
    {
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        return state.Values.DefaultTextFormat;
    }

    GsIdFormat GsIdOptions::GetDefaultJsonFormat()
    {
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        return state.Values.DefaultJsonFormat;
    }

    GsIdFormat GsIdOptions::GetDefaultDatabaseFormat()
    {
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        return state.Values.DefaultDatabaseFormat;
    }

    GsIdOptionsValues GsIdOptions::GetValues()
    {
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        return state.Values;
    }

    bool GsIdOptions::IsLocked()
    {
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        return state.Locked;
    }

    void GsIdOptions::SetDefaultCase(GsIdCase value)
    {
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        EnsureUnlocked(state);
        state.Values.DefaultCase = value;
    }

    void GsIdOptions::SetDefaultTextFormat(GsIdFormat value)
    {
        ValidateFormat(value);
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        EnsureUnlocked(state);
        state.Values.DefaultTextFormat = value;
    }

    void GsIdOptions::SetDefaultJsonFormat(GsIdFormat value)
    {
        ValidateFormat(value);
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        EnsureUnlocked(state);
        state.Values.DefaultJsonFormat = value;
    }

    void GsIdOptions::SetDefaultDatabaseFormat(GsIdFormat value)
    {
        ValidateFormat(value);
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        EnsureUnlocked(state);
        state.Values.DefaultDatabaseFormat = value;
    }

    void GsIdOptions::Configure(
        std::optional<GsIdCase> defaultCase,
        std::optional<GsIdFormat> defaultTextFormat,
        std::optional<GsIdFormat> defaultJsonFormat,
        std::optional<GsIdFormat> defaultDatabaseFormat)
    {
        if (defaultTextFormat)
        {
            ValidateFormat(*defaultTextFormat);
        }

        if (defaultJsonFormat)
        {
            ValidateFormat(*defaultJsonFormat);
        }

        if (defaultDatabaseFormat)
        {
            ValidateFormat(*defaultDatabaseFormat);
        }

        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        EnsureUnlocked(state);

        if (defaultCase)
        {
            state.Values.DefaultCase = *defaultCase;
        }

        if (defaultTextFormat)
        {
            state.Values.DefaultTextFormat = *defaultTextFormat;
        }

        if (defaultJsonFormat)
        {
            state.Values.DefaultJsonFormat = *defaultJsonFormat;
        }

        if (defaultDatabaseFormat)
        {
            state.Values.DefaultDatabaseFormat = *defaultDatabaseFormat;
        }
    }

    void GsIdOptions::Lock()
    {
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        state.Locked = true;
    }

    void GsIdOptions::Reset()
    {
        OptionsState& state = GetOptionsState();
        std::scoped_lock lock(state.Mutex);
        EnsureUnlocked(state);
        state.Values = GsIdOptionsValues{};
    }

    GsId GsId::NewGsId()
    {
        return GsIdGenerator::NewGsId();
    }

    GsId GsId::FromBytes(std::span<const std::uint8_t> bytes)
    {
        if (bytes.size() != GsIdConstants::ByteLength)
        {
            throw GsIdException(
                "Un GsId brut doit contenir exactement 32 octets.");
        }

        ByteArray result{};
        std::copy(bytes.begin(), bytes.end(), result.begin());
        return GsId(result);
    }

    GsId GsId::FromNormalized(std::string_view normalizedValue)
    {
        if (normalizedValue.size() != GsIdConstants::HexLength)
        {
            throw GsIdException(
                "La valeur normalisée GsId doit contenir exactement 64 caractères hexadécimaux.");
        }

        return GsIdParser::Parse(normalizedValue);
    }

    GsId GsId::FromString(std::string_view value)
    {
        return GsIdParser::Parse(value);
    }

    GsId GsId::Parse(std::string_view value)
    {
        return GsIdParser::Parse(value);
    }

    bool GsId::TryParse(std::string_view value, GsId& result) noexcept
    {
        return GsIdParser::TryParse(value, result);
    }

    std::optional<GsId> GsId::TryParse(std::string_view value) noexcept
    {
        return GsIdParser::TryParse(value);
    }

    bool GsId::IsEmpty() const noexcept
    {
        return std::all_of(bytes_.begin(), bytes_.end(), [](std::uint8_t value)
        {
            return value == 0;
        });
    }

    std::string GsId::ToString() const
    {
        const GsIdOptionsValues options = GsIdOptions::GetValues();
        return ToString(options.DefaultTextFormat, options.DefaultCase);
    }

    std::string GsId::ToString(GsIdFormat format) const
    {
        return ToString(format, GsIdOptions::GetDefaultCase());
    }

    std::string GsId::ToString(GsIdFormat format, GsIdCase letterCase) const
    {
        ValidateFormat(format);

        const std::size_t length =
            format == GsIdFormat::N
                ? GsIdConstants::HexLength
                : GsIdConstants::FormattedLength;

        std::string result(length, '\0');
        std::size_t charsWritten = 0;

        if (!TryFormat(std::span<char>(result.data(), result.size()), charsWritten, format, letterCase))
        {
            throw GsIdException("Impossible de formater le GsId.");
        }

        result.resize(charsWritten);
        return result;
    }

    std::string GsId::ToString(char format) const
    {
        switch (format)
        {
            case 'N':
                return ToString(GsIdFormat::N, GsIdCase::Upper);
            case 'D':
                return ToString(GsIdFormat::D, GsIdCase::Upper);
            case 'n':
                return ToString(GsIdFormat::N, GsIdCase::Lower);
            case 'd':
                return ToString(GsIdFormat::D, GsIdCase::Lower);
            default:
                throw GsIdException(
                    std::string("Le format GsId '") + format + "' n'est pas supporté.");
        }
    }

    std::string GsId::ToNormalizedString() const
    {
        return ToString(GsIdFormat::N, GsIdOptions::GetDefaultCase());
    }

    std::string GsId::ToNormalizedString(GsIdCase letterCase) const
    {
        return ToString(GsIdFormat::N, letterCase);
    }

    bool GsId::TryFormat(
        std::span<char> destination,
        std::size_t& charsWritten,
        GsIdFormat format,
        GsIdCase letterCase) const noexcept
    {
        charsWritten = 0;

        if (format != GsIdFormat::N && format != GsIdFormat::D)
        {
            return false;
        }

        const std::size_t required =
            format == GsIdFormat::N
                ? GsIdConstants::HexLength
                : GsIdConstants::FormattedLength;

        if (destination.size() < required)
        {
            return false;
        }

        const auto normalized = EncodeNormalized(bytes_, letterCase);

        if (format == GsIdFormat::N)
        {
            std::copy(normalized.begin(), normalized.end(), destination.begin());
            charsWritten = GsIdConstants::HexLength;
            return true;
        }

        std::size_t sourceIndex = 0;
        std::size_t destinationIndex = 0;

        for (; destinationIndex < GsIdConstants::FormattedLength; ++destinationIndex)
        {
            if (IsHyphenPosition(destinationIndex))
            {
                destination[destinationIndex] = '-';
            }
            else
            {
                destination[destinationIndex] = normalized[sourceIndex++];
            }
        }

        charsWritten = GsIdConstants::FormattedLength;
        return true;
    }

    bool GsId::TryFormat(
        std::span<char> destination,
        std::size_t& charsWritten,
        GsIdFormat format) const noexcept
    {
        return TryFormat(destination, charsWritten, format, GsIdOptions::GetDefaultCase());
    }

    bool GsId::TryFormat(
        std::span<char> destination,
        std::size_t& charsWritten) const noexcept
    {
        const GsIdOptionsValues options = GsIdOptions::GetValues();
        return TryFormat(destination, charsWritten, options.DefaultTextFormat, options.DefaultCase);
    }

    GsId GsIdGenerator::NewGsId()
    {
        GsId::ByteArray bytes{};
        FillSecureRandom(bytes);
        return GsId(bytes);
    }

    GsId GsIdParser::Parse(std::string_view value)
    {
        const std::string normalized = Normalize(value, GsIdCase::Upper);
        GsId::ByteArray bytes{};

        for (std::size_t index = 0; index < GsIdConstants::ByteLength; ++index)
        {
            const std::uint8_t left = ConvertHexCharacter(normalized[index * 2]);
            const std::uint8_t right = ConvertHexCharacter(normalized[(index * 2) + 1]);
            bytes[index] = static_cast<std::uint8_t>((left << 4) | right);
        }

        return GsId(bytes);
    }

    bool GsIdParser::TryParse(std::string_view value, GsId& result) noexcept
    {
        result = GsId::Empty();

        try
        {
            result = Parse(value);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    std::optional<GsId> GsIdParser::TryParse(std::string_view value) noexcept
    {
        GsId result;

        if (!TryParse(value, result))
        {
            return std::nullopt;
        }

        return result;
    }

    std::string GsIdParser::Normalize(std::string_view value)
    {
        return Normalize(value, GsIdOptions::GetDefaultCase());
    }

    std::string GsIdParser::Normalize(std::string_view value, GsIdCase letterCase)
    {
        value = Trim(value);

        if (value.empty())
        {
            throw GsIdException("La valeur GsId ne peut pas être vide ou blanche.");
        }

        if (value.size() != GsIdConstants::HexLength
            && value.size() != GsIdConstants::FormattedLength)
        {
            throw GsIdException(
                "La valeur GsId doit contenir 64 caractères sans tirets "
                "ou 69 caractères avec tirets.");
        }

        std::string result;
        result.reserve(GsIdConstants::HexLength);

        if (value.size() == GsIdConstants::HexLength)
        {
            for (const char character : value)
            {
                if (!IsHexCharacter(character))
                {
                    throw GsIdException(
                        std::string("Le caractère '") + character
                        + "' n'est pas hexadécimal.");
                }

                result.push_back(ApplyCase(character, letterCase));
            }

            return result;
        }

        for (std::size_t index = 0; index < value.size(); ++index)
        {
            const char character = value[index];

            if (IsHyphenPosition(index))
            {
                if (character != '-')
                {
                    throw GsIdException(
                        "La valeur GsId n'utilise pas les positions de tirets officielles.");
                }

                continue;
            }

            if (!IsHexCharacter(character))
            {
                throw GsIdException(
                    std::string("Le caractère '") + character
                    + "' n'est pas hexadécimal.");
            }

            result.push_back(ApplyCase(character, letterCase));
        }

        return result;
    }

    bool GsIdValidator::IsValid(std::string_view value) noexcept
    {
        GsId ignored;
        return GsIdParser::TryParse(value, ignored);
    }

    bool GsIdValidator::IsValid(std::string_view value, GsIdFormat format) noexcept
    {
        const std::string_view trimmed = Trim(value);

        if (format == GsIdFormat::N && trimmed.size() != GsIdConstants::HexLength)
        {
            return false;
        }

        if (format == GsIdFormat::D && trimmed.size() != GsIdConstants::FormattedLength)
        {
            return false;
        }

        if (format != GsIdFormat::N && format != GsIdFormat::D)
        {
            return false;
        }

        return IsValid(trimmed);
    }

    std::ostream& operator<<(std::ostream& stream, const GsId& value)
    {
        return stream << value.ToString();
    }
}

std::size_t std::hash<GalacticShrine::GsId>::operator()(
    const GalacticShrine::GsId& value) const noexcept
{
    // FNV-1a 64 bits, replié automatiquement vers size_t sur les plateformes 32 bits.
    std::uint64_t hash = 14695981039346656037ULL;

    for (const std::uint8_t byte : value.Bytes())
    {
        hash ^= byte;
        hash *= 1099511628211ULL;
    }

    return static_cast<std::size_t>(hash);
}
