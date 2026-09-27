# Testy práce se soubory (fandio)

Testovací úloha PC-FANDu, která ověřuje zápis a čtení datových souborů v CppFandu.
Kapitoly jsou uložené jako textové soubory, spouštěč je zabalí do projektového
souboru, pustí úlohu v `cppfandlib.dll` a vypíše výsledky.

## Spuštění

```
x64\Debug\fandtest.exe --fand-dir C:\PCFAND\fis-cppfand tests\fandio
```

- `--fand-dir` (nebo proměnná `FANDDIR`) je adresář s `FAND.CFG` a `FAND.RES`,
  v repozitáři nejsou.
- `--timeout SEC` (výchozí 60): když úloha do té doby neskončí, spouštěč vypíše
  obrazovku (typicky chybu překladu nebo hlášku čekající na klávesu) a úlohu zastaví.
- `--keep`: nechá pracovní adresář s úlohou, daty a `fand.log`.

Návratový kód 0 = všechny testy prošly, 1 = některý selhal nebo úloha nedoběhla.

## Jak to funguje

1. `fandrdb pack` sestaví z kapitol `FANDTEST.RDB` a `.TTT` v novém dočasném adresáři.
2. `fandtest` spustí úlohu přes `FandStart` bez obsluhy.
3. Testy zapisují výsledky do souboru `VYSLEDKY` (`Test:A,20; Stav:A,5; Popis:A,60`)
   procedurou `Vysl`. `Stav` je `OK` nebo `CHYBA`, `Popis` první nalezený rozdíl.
4. `MAIN` na konci zapíše větu `*KONEC*`. Když chybí, úloha nedoběhla.
5. `fandtest` přečte `VYSLEDKY.000` přes knihovnu fandio a vypíše ho.

## Kapitoly

Soubor `NNNN_<název>_<typ>.txt`, kódování UTF-8 (při balení se převede do CP852).
`NNNN` určuje pořadí vět v projektovém souboru. Stejný formát vytváří
`fandrdb unpack`.

| Kapitoly | Obsah |
|---|---|
| 0001–0003 | `VYSLEDKY`, `Vysl`, `MAIN` |
| 0010–0099 | deklarace testovacích souborů (F) |
| 0100–     | testy (P), jeden test = jedna procedura |

Nový test: přidat proceduru `01x0_<Název>_P.txt`, na jejím konci zavolat
`proc(Vysl,('<Název>',cond(chyba='':'OK',else:'CHYBA'),chyba));`
a zařadit ji do `MAIN`. Každý test si na začátku vyprázdní své soubory
(`SOUBOR.nrecs:=0`), výsledky tak nezávisí na pořadí.

## Na co si dát pozor

- **Pořadí kapitol:** soubor (kapitola F) musí být v projektu před první kapitolou,
  která ho používá, jinak překlad hlásí „nedeklarovaný název souboru“.
- **`cond` s číselnou hodnotou:** `cond(i=1:0, …)` se přeloží špatně, `1:0` se čte
  jako číslo s formátem. Podmínky proto psát do závorek: `cond((i=1):0, …)`.
- **Logické hodnoty** nejdou porovnat operátorem `<>`; použít `&`, `|` a `^`.
- **`modulo`** je kontrola kontrolní číslice, ne zbytek po dělení
  (zbytek: `i-int(i/n)*n`).
- Procedura bez parametrů se volá `proc(Název)`.
