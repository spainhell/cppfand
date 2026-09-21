# spdlog

Verzovana kopie header-only knihovny [spdlog](https://github.com/gabime/spdlog).

- verze: **v1.17.0** (bundled fmt 12.1.0)
- zdroj: https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.tar.gz
- obsah: `include/spdlog` z tarballu, beze zmen, + `spdlog/LICENSE`

## Upgrade

Nahradit cely adresar `spdlog/` obsahem `include/spdlog` noveho tarballu
a aktualizovat verzi vyse. Soubory se nepatchuji -- kdyby bylo potreba,
patri to do `Logging/Logging.cpp`, ne do knihovny.

Pozor na MSVC: spdlog do 1.11 (fmt 9) pouziva `stdext::checked_array_iterator`,
ktery STL ve Visual Studiu 18 uz neobsahuje. Nepouzivat starsi verze.
