#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <compare>
#include <functional>
#include <iosfwd>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace GalacticShrine
{
    enum class GsIdCase : std::uint8_t
    {
        Upper = 0,
        Lower = 1
    };

    enum class GsIdFormat : std::uint8_t
    {
        N = 0,
        D = 1
    };

    struct GsIdConstants final
    {
        static constexpr std::size_t ByteLength = 32;
        static constexpr std::size_t HexLength = 64;
        static constexpr std::size_t FormattedLength = 69;
        static constexpr std::size_t HyphenCount = 5;
        static constexpr std::string_view DGroupPattern = "16-8-8-8-8-16";
    };

    class GsIdException final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    struct GsIdOptionsValues final
    {
        GsIdCase DefaultCase = GsIdCase::Upper;
        GsIdFormat DefaultTextFormat = GsIdFormat::D;
        GsIdFormat DefaultJsonFormat = GsIdFormat::D;
        GsIdFormat DefaultDatabaseFormat = GsIdFormat::N;
    };

    class GsIdOptions final
    {
    public:
        GsIdOptions() = delete;

        [[nodiscard]] static GsIdCase GetDefaultCase();
        [[nodiscard]] static GsIdFormat GetDefaultTextFormat();
        [[nodiscard]] static GsIdFormat GetDefaultJsonFormat();
        [[nodiscard]] static GsIdFormat GetDefaultDatabaseFormat();
        [[nodiscard]] static GsIdOptionsValues GetValues();
        [[nodiscard]] static bool IsLocked();

        static void SetDefaultCase(GsIdCase value);
        static void SetDefaultTextFormat(GsIdFormat value);
        static void SetDefaultJsonFormat(GsIdFormat value);
        static void SetDefaultDatabaseFormat(GsIdFormat value);

        static void Configure(
            std::optional<GsIdCase> defaultCase = std::nullopt,
            std::optional<GsIdFormat> defaultTextFormat = std::nullopt,
            std::optional<GsIdFormat> defaultJsonFormat = std::nullopt,
            std::optional<GsIdFormat> defaultDatabaseFormat = std::nullopt);

        static void Lock();
        static void Reset();
    };

    class GsId final
    {
    public:
        using ByteArray = std::array<std::uint8_t, GsIdConstants::ByteLength>;

        constexpr GsId() noexcept = default;
        explicit constexpr GsId(ByteArray bytes) noexcept : bytes_(bytes) {}

        [[nodiscard]] static constexpr GsId Empty() noexcept { return GsId{}; }
        [[nodiscard]] static GsId NewGsId();
        [[nodiscard]] static GsId FromBytes(std::span<const std::uint8_t> bytes);
        [[nodiscard]] static GsId FromNormalized(std::string_view normalizedValue);
        [[nodiscard]] static GsId FromString(std::string_view value);
        [[nodiscard]] static GsId Parse(std::string_view value);
        [[nodiscard]] static bool TryParse(std::string_view value, GsId& result) noexcept;
        [[nodiscard]] static std::optional<GsId> TryParse(std::string_view value) noexcept;

        [[nodiscard]] bool IsEmpty() const noexcept;
        [[nodiscard]] constexpr const ByteArray& Bytes() const noexcept { return bytes_; }
        [[nodiscard]] constexpr ByteArray ToByteArray() const noexcept { return bytes_; }

        [[nodiscard]] std::string ToString() const;
        [[nodiscard]] std::string ToString(GsIdFormat format) const;
        [[nodiscard]] std::string ToString(GsIdFormat format, GsIdCase letterCase) const;
        [[nodiscard]] std::string ToString(char format) const;
        [[nodiscard]] std::string ToNormalizedString() const;
        [[nodiscard]] std::string ToNormalizedString(GsIdCase letterCase) const;

        [[nodiscard]] bool TryFormat(
            std::span<char> destination,
            std::size_t& charsWritten,
            GsIdFormat format,
            GsIdCase letterCase) const noexcept;

        [[nodiscard]] bool TryFormat(
            std::span<char> destination,
            std::size_t& charsWritten,
            GsIdFormat format) const noexcept;

        [[nodiscard]] bool TryFormat(
            std::span<char> destination,
            std::size_t& charsWritten) const noexcept;

        [[nodiscard]] constexpr bool operator==(const GsId&) const noexcept = default;
        [[nodiscard]] constexpr bool operator!=(const GsId&) const noexcept = default;
        [[nodiscard]] constexpr auto operator<=>(const GsId&) const noexcept = default;

    private:
        ByteArray bytes_{};
    };

    class GsIdGenerator final
    {
    public:
        GsIdGenerator() = delete;
        [[nodiscard]] static GsId NewGsId();
    };

    class GsIdParser final
    {
    public:
        GsIdParser() = delete;

        [[nodiscard]] static GsId Parse(std::string_view value);
        [[nodiscard]] static bool TryParse(std::string_view value, GsId& result) noexcept;
        [[nodiscard]] static std::optional<GsId> TryParse(std::string_view value) noexcept;
        [[nodiscard]] static std::string Normalize(std::string_view value);
        [[nodiscard]] static std::string Normalize(std::string_view value, GsIdCase letterCase);
    };

    class GsIdValidator final
    {
    public:
        GsIdValidator() = delete;

        [[nodiscard]] static bool IsValid(std::string_view value) noexcept;
        [[nodiscard]] static bool IsValid(std::string_view value, GsIdFormat format) noexcept;
    };

    std::ostream& operator<<(std::ostream& stream, const GsId& value);
}

template<>
struct std::hash<GalacticShrine::GsId>
{
    [[nodiscard]] std::size_t operator()(const GalacticShrine::GsId& value) const noexcept;
};
