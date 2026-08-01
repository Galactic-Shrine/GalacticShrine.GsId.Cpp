#include <galactic_shrine/gsid/gsid.hpp>

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>

using namespace GalacticShrine;

namespace
{
    constexpr std::string_view NormalizedUpper =
        "9F2A6C1E8D4B7A90A13F9C2DE88B421091AF77CB4D6E39A2FC018AD92E7B5C64";

    constexpr std::string_view FormattedUpper =
        "9F2A6C1E8D4B7A90-A13F9C2D-E88B4210-91AF77CB-4D6E39A2-FC018AD92E7B5C64";

    int failures = 0;

    void Check(bool condition, const char* expression, const char* file, int line)
    {
        if (!condition)
        {
            std::cerr << file << ':' << line << " : échec : " << expression << '\n';
            ++failures;
        }
    }
}

#define GSID_CHECK(expression) Check((expression), #expression, __FILE__, __LINE__)

int main()
{
    GsIdOptions::Reset();

    const GsId generated = GsId::NewGsId();
    GSID_CHECK(!generated.IsEmpty());
    GSID_CHECK(generated.ToByteArray().size() == GsIdConstants::ByteLength);
    GSID_CHECK(generated.ToString(GsIdFormat::N).size() == GsIdConstants::HexLength);
    GSID_CHECK(generated.ToString(GsIdFormat::D).size() == GsIdConstants::FormattedLength);

    const GsId fromN = GsId::Parse(NormalizedUpper);
    const GsId fromD = GsId::Parse(FormattedUpper);
    GSID_CHECK(fromN == fromD);
    GSID_CHECK(fromN.ToString('N') == NormalizedUpper);
    GSID_CHECK(fromD.ToString('D') == FormattedUpper);
    GSID_CHECK(fromD.ToString('n') ==
        "9f2a6c1e8d4b7a90a13f9c2de88b421091af77cb4d6e39a2fc018ad92e7b5c64");
    GSID_CHECK(fromD.ToString('d') ==
        "9f2a6c1e8d4b7a90-a13f9c2d-e88b4210-91af77cb-4d6e39a2-fc018ad92e7b5c64");

    GsId invalidResult = generated;
    GSID_CHECK(!GsId::TryParse("invalid", invalidResult));
    GSID_CHECK(invalidResult.IsEmpty());
    GSID_CHECK(!GsId::TryParse("invalid").has_value());

    GSID_CHECK(GsIdValidator::IsValid(NormalizedUpper));
    GSID_CHECK(GsIdValidator::IsValid(FormattedUpper));
    GSID_CHECK(GsIdValidator::IsValid(NormalizedUpper, GsIdFormat::N));
    GSID_CHECK(GsIdValidator::IsValid(FormattedUpper, GsIdFormat::D));
    GSID_CHECK(!GsIdValidator::IsValid(FormattedUpper, GsIdFormat::N));
    GSID_CHECK(!GsIdValidator::IsValid(
        "9F2A6C1E8D4B7A90_A13F9C2D-E88B4210-91AF77CB-4D6E39A2-FC018AD92E7B5C64"));

    GSID_CHECK(GsIdParser::Normalize(
        "  9f2a6c1e8d4b7a90-a13f9c2d-e88b4210-91af77cb-4d6e39a2-fc018ad92e7b5c64  ",
        GsIdCase::Upper) == NormalizedUpper);

    std::array<char, GsIdConstants::FormattedLength> buffer{};
    std::size_t charsWritten = 0;
    GSID_CHECK(fromD.TryFormat(buffer, charsWritten, GsIdFormat::D, GsIdCase::Upper));
    GSID_CHECK(charsWritten == GsIdConstants::FormattedLength);
    GSID_CHECK(std::string_view(buffer.data(), charsWritten) == FormattedUpper);

    std::array<char, 10> smallBuffer{};
    GSID_CHECK(!fromD.TryFormat(smallBuffer, charsWritten, GsIdFormat::D, GsIdCase::Upper));
    GSID_CHECK(charsWritten == 0);

    const auto roundTrip = GsId::FromBytes(fromD.Bytes());
    GSID_CHECK(roundTrip == fromD);

    std::unordered_set<GsId> set;
    set.insert(fromN);
    set.insert(fromD);
    GSID_CHECK(set.size() == 1);

    GsIdOptions::Configure(
        GsIdCase::Lower,
        GsIdFormat::N,
        GsIdFormat::D,
        GsIdFormat::N);

    GSID_CHECK(fromD.ToString() ==
        "9f2a6c1e8d4b7a90a13f9c2de88b421091af77cb4d6e39a2fc018ad92e7b5c64");

    GsIdOptions::Lock();
    GSID_CHECK(GsIdOptions::IsLocked());

    bool lockRejectedMutation = false;

    try
    {
        GsIdOptions::SetDefaultCase(GsIdCase::Upper);
    }
    catch (const GsIdException&)
    {
        lockRejectedMutation = true;
    }

    GSID_CHECK(lockRejectedMutation);

    if (failures != 0)
    {
        std::cerr << failures << " test(s) en échec.\n";
        return EXIT_FAILURE;
    }

    std::cout << "Tous les tests GsId C++ sont réussis.\n";
    return EXIT_SUCCESS;
}
