#include <galactic_shrine/gsid/gsid.hpp>

#include <iostream>

int main()
{
    GalacticShrine::GsIdOptions::Configure(
        GalacticShrine::GsIdCase::Lower,
        GalacticShrine::GsIdFormat::N,
        GalacticShrine::GsIdFormat::D,
        GalacticShrine::GsIdFormat::N);

    const GalacticShrine::GsId id = GalacticShrine::GsId::NewGsId();

    std::cout << "N : " << id.ToString(GalacticShrine::GsIdFormat::N) << '\n';
    std::cout << "D : " << id.ToString(GalacticShrine::GsIdFormat::D) << '\n';

    const GalacticShrine::GsId parsed =
        GalacticShrine::GsId::Parse(id.ToString(GalacticShrine::GsIdFormat::D));
    std::cout << "Identique : " << std::boolalpha << (id == parsed) << '\n';
}
