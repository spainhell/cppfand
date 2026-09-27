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
- `--only Test1,Test2`: pustí jen vybrané testy. Ostatní testovací kapitoly (0100 a výš)
  do úlohy nedá, takže chyba překladu v jednom testu nebrání ladění ostatních.
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

| Test | Co ověřuje |
|---|---|
| ZapisCteni | všechny typy uložených údajů, přepis vět, čtení po zavření souboru |
| Klice | vlastní a sestupný alternativní klíč, `keyof`, rušení, `indexfile` s `compress` |
| Texty | volné texty 0 B až 65 000 B, přepis, rušení věty, `appendrec` |
| Trideni | `sort` vzestupně i sestupně |
| DuplKlice | duplicitní klíč, `key in`, `getindex`, `recno`, `forall` s podmínkou, `recallrec` |
| VypocKlic | klíč nad vypočítaným údajem, změna věty, reindexace |
| Lexikalni | české řazení (`~`) v klíči, v `sort` a v porovnání; závisí na tabulce abecedy ve FAND.CFG |
| Transformace | `merge` s filtrem do indexovaného souboru, připojení `+`, přepis s výpočtem |
| Dbf | soubor .DBF s memo souborem .DBT: všechny typy údajů, přepis, rušení věty, `appendrec`, `forall`, `merge` do .DBF (i s `+`, do sebe sama a s tříděním vstupu `!`), `sort`, vazba do .DBF číselníku a `.exist` |
| Aditivni | aditivní změny `#A` s `!!` (založení nadřízené věty) při `writerec` a `deleterec` s `+`; nadřízený indexovaný soubor i .DBF |
| Editor | datový editor nad .DBF ovládaný přes `setkeybuf`: oprava údaje, zrušení (Ctrl-Y) a vložení (Ctrl-N) věty |

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
- Cyklus `for` zná jen `to`, ne `downto`.
- **Podmínka ve `forall`** se píše nad údaji souboru: `forall r (Por>90)`, ne `(r.Por>90)`.
- **`readrec` podle alternativního klíče** potřebuje klíč i u proměnné:
  `readrec(r/Klic,keyof(SOUBOR/Klic,…))`. Bez něj se hledá podle vlastního klíče `@`.
- **České řazení:** samostatná písmena jsou jen Č, Ř, Š, Ž a CH. Ď, Ť, Ň se řadí jako D, T, N.
- **Editor v testu:** klávesy se zadají předem příkazem `setkeybuf` (Enter `char(13)`, Esc `char(27)`,
  šipky `char(0)+char(72)`/`char(80)`, Ctrl-písmeno `char(1)`..`char(26)`). Nová věta se uloží
  až po průchodu všemi údaji (Enter na posledním). Znaky s diakritikou `setkeybuf` zatím nepředá.
