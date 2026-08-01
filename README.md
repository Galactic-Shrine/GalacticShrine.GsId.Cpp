# GalacticShrine.GsId C++

Bibliothèque C++20 pour générer, parser, valider et sérialiser des identifiants
Galactic-Shrine de **256 bits**.

Cette implémentation est compatible avec :

- `GalacticShrine.GsId` pour C# ;
- `galactic-shrine/gsid` pour PHP.

## Formats

| Format | Exemple | Longueur |
|---|---|---:|
| `N` | `0123456789abcdef...` | 64 |
| `D` | `0123456789abcdef-01234567-89abcdef-01234567-89abcdef-0123456789abcdef` | 69 |

Le format `D` suit le motif `16-8-8-8-8-16`.

## Fonctionnalités

- stockage binaire exact de 32 octets ;
- génération aléatoire cryptographiquement sûre ;
- formats `N` et `D` ;
- sorties `Upper` et `Lower` ;
- parsing avec ou sans espaces extérieurs ;
- validation générale ou par format ;
- options globales verrouillables ;
- comparaison, tri et `std::hash` ;
- formatage dans un `std::span<char>` sans allocation ;
- adaptateur optionnel pour `nlohmann::json` ;
- CMake, installation et export de cible.

## Construction

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DGSID_BUILD_TESTS=ON \
  -DGSID_BUILD_EXAMPLES=ON

cmake --build build
ctest --test-dir build --output-on-failure
```

Sous Visual Studio :

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Namespace C++

L’API publique canonique utilise la namespace `GalacticShrine` :

```cpp
GalacticShrine::GsId id = GalacticShrine::GsId::NewGsId();
```

## Utilisation

```cpp
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

    std::cout << id.ToString(GalacticShrine::GsIdFormat::N) << '\n';
    std::cout << id.ToString(GalacticShrine::GsIdFormat::D) << '\n';

    const GalacticShrine::GsId parsed =
        GalacticShrine::GsId::Parse(id.ToString(GalacticShrine::GsIdFormat::D));

    if (parsed == id)
    {
        std::cout << "GsId identique\n";
    }
}
```

## Formats rapides

Comme dans la version C# :

- `ToString('N')` : format N en majuscules ;
- `ToString('D')` : format D en majuscules ;
- `ToString('n')` : format N en minuscules ;
- `ToString('d')` : format D en minuscules.

## JSON optionnel

L'adaptateur n'impose aucune dépendance au cœur de la bibliothèque. Dans une
application qui utilise déjà `nlohmann/json`, inclure :

```cpp
#include <galactic_shrine/gsid/nlohmann_json.hpp>

nlohmann::json json = id;
GsId restored = json.get<GsId>();
```

## Installation CMake

```bash
cmake --install build --prefix ./install
```

Dans un autre projet :

```cmake
find_package(GalacticShrineGsId CONFIG REQUIRED)
target_link_libraries(mon_application PRIVATE GalacticShrine::GsId)
```

## Génération sécurisée

- Windows : `BCryptGenRandom` ;
- Linux : `getrandom` ;
- macOS : `SecRandomCopyBytes` ;
- autres systèmes Unix compatibles : `/dev/urandom`.

## Compatibilité interlangage

Les octets ne sont ni réordonnés ni transformés. Un GsId produit en C++, C# ou
PHP garde exactement la même chaîne `N`, la même chaîne `D` et les mêmes
32 octets bruts dans les trois implémentations.

## Licence

MPL-2.0.
