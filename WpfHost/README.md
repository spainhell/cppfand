# WpfHost – okenní hostitel cppfandu

Interpret FANDu (projekty Core, DataEditor, …) běží beze změny logiky uvnitř
`cppfandlib.dll` na vlastním vlákně. Tato WPF aplikace (`cppfand-wpf.exe`)
mu dělá obrazovku a klávesnici:

- **Obrazovka 80×25** se kreslí z bufferu, který si drží `Drivers/screen.cpp`
  (znak CP852 + atribut). Hostitel ji čte přes `FandGetScreen` ~30× za sekundu
  a překreslí jen při změně čísla verze. Plné a stínované bloky (logo Účta)
  se kreslí jako obdélníky, ostatní znaky písmem Cascadia Mono / Consolas.
- **Klávesy** se posílají přes `FandPushKey` ve tvaru `KEY_EVENT_RECORD`
  konzole, takže `PressedKey` a všechny klávesové zkratky FANDu zůstávají.
- **Myš** se posílá přes `FandPushMouse` ve tvaru `MOUSE_EVENT_RECORD`
  konzole (souřadnice v buňkách mřížky). Interpret z ní dělá události
  `evMouseDown`/`Up`/`Move`/`Auto` stejně jako původní PC-FAND, takže
  funguje výběr položky v menu, klik na klávesu v posledním řádku,
  pravé tlačítko jako Esc i dvojklik v editoru dat.
- **Editace jednořádkového pole** (`DataEditor::EditTxt`) se předává
  hostiteli (`Drivers/host.h`, `FandPollFieldEdit` / `FandCompleteFieldEdit`).
  Nad polem se objeví běžný `TextBox`: schránka (Ctrl+C/V/X), označení myší,
  Insert přepíná vkládání, filtr znaků podle typu pole, u data se vložený
  text převede na masku (`DD.MM.YY`). Enter, Esc, Tab, šipky nahoru/dolů,
  PgUp/PgDn, F-klávesy a Ctrl/Alt kombinace editaci ukončí a klávesu dostane
  FAND, který ji zpracuje stejně jako v konzoli.

## Spuštění

Obvyklé nasazení je zkopírovat obsah složky `net48` přímo do adresáře úlohy.
Pak stačí `cppfand-wpf.exe` spustit dvojklikem: `FAND.CFG` a `FAND.RES` leží
vedle programu, takže se na cesty neptá, a je-li v adresáři jediná `.RDB`,
předvyplní i název úlohy — zbývá potvrdit.

```
cppfand-wpf.exe <úloha>                    ... obě cesty = složka s FAND.CFG
cppfand-wpf.exe <složka FANDu> <úloha>     ... složka je zároveň pracovní adresář
cppfand-wpf.exe <složka s FAND.CFG a FAND.RES> <pracovní adresář> <úloha>

cppfand-wpf.exe UCTO2024
cppfand-wpf.exe C:\ucto C:\ucto UCTO2024
```

Úloha je identifikátor bez cesty a přípony. Když ve složce z parametru
`FAND.CFG` a `FAND.RES` nejsou, program to ohlásí a skončí.

Za tím může stát ještě **režim**, stejně jako v PC-FANDu (`paramstr[2]`,
větví se podle něj `runfand.cpp:389` jako originál v `RUNFAND.PAS:364`):

```
cppfand-wpf.exe <úloha> D            ... ladicí běh
cppfand-wpf.exe <soubor> T           ... editace textového souboru
cppfand-wpf.exe *.TXT T              ... nejdřív nabídne výběr souboru
cppfand-wpf.exe C:\ucto UCTO2024 D
```

- **`D`** zapne `IsTestRun`. Úloha se spustí a po jejím konci program neskončí,
  ale zůstane ve vývojovém prostředí FANDu.
- **`T`** bere první parametr jako cestu k textovému souboru a rovnou ho otevře
  v editoru; po zavření program skončí. Začíná-li `*.`, nabídne se nejdřív výběr
  souboru s danou příponou.

Režim se pozná podle přesné shody posledního parametru s `D` nebo `T` (na
velikosti nezáleží). Úloha pojmenovaná `D` nebo `T` by se proto musela zadat
i s cestou — původní FAND měl stejné omezení, režim u něj byl prostě druhý
parametr.

Bez parametrů se ptá jen na to, co samo nezjistí:

- `FAND.CFG` a `FAND.RES` se hledají nejdřív vedle `cppfand-wpf.exe`, potom
  v aktuálním adresáři. Když se najdou, dialog ukáže jen pole pro úlohu.
- Když se nenajdou, dialog nabídne i obě cesty; hodnoty se pamatují
  v `%AppData%\cppfand\wpfhost.txt`.

Rozměr obrazovky jde nastavit v `cppfand-wpf.exe.config` (u .NET 10
`cppfand-wpf.dll.config`), např. pro 80×34 jako v PC-FANDu:

```xml
<appSettings>
  <add key="ScreenCols" value="80" />
  <add key="ScreenRows" value="34" />
</appSettings>
```

Prázdná hodnota znamená rozměr z `FAND.CFG` (obvykle 80×25). Šířka se
omezuje na 40..132, výška na 25..100.

Příkazy `EXEC` (a tisk přes externí program) se pouštějí přes `cmd.exe /c`
bez konzolového okna, se vstupem i výstupem do `NUL`.

Klávesy hostitele: Shift+Insert vloží text ze schránky jako psaní,
Ctrl+Shift+C zkopíruje celou obrazovku, Ctrl+kolečko mění velikost písma.

## Sestavení a nasazení

- Projekt cílí na **.NET Framework 4.8** (`net48`) i **.NET 10**
  (`net10.0-windows`), v platformách **x64** i **x86**. Výstup leží vedle
  nativních binárek: `x64\Release\wpf\<TFM>\` pro x64 a `Release\wpf\<TFM>\`
  pro 32bit, která používá 32bitovou `cppfandlib.dll` z konfigurace Win32.
  Pro klienty stačí složka `net48`:
  .NET Framework 4.8 je součástí Windows 10 (od 1903) a 11, nic dalšího
  se neinstaluje. Nativní `cppfandlib.dll` i `cppfand.exe` jsou linkované
  se statickou runtime knihovnou (`/MT`), takže nepotřebují Visual C++
  Redistributable; závisí jen na kernel32, user32 a comdlg32.
- V kódu proto nesmí být API novější než 4.8 (žádné `Math.Clamp`,
  rozsahy `[..]`, `char.IsAsciiDigit`, `System.Text.Json`,
  `OpenFolderDialog`); rozdíly řeší `#if NETFRAMEWORK` / `#if NETCOREAPP`.
- `dotnet build WpfHost -c Release -p:Platform=x64` přeloží obě verze,
  nativní `cppfandlib.dll` se do výstupu kopíruje ze stejné konfigurace
  solution (nejdřív přeložit `2_DynamicLibrary`).
- Ve Visual Studiu je projekt v `cppfand.sln` jako `3_WpfHost` se závislostí
  na `2_DynamicLibrary`. Aby ho VS umělo načíst a přeložit, musí být
  nainstalován workload **.NET desktop development** (samotný dotnet SDK
  nestačí, MSBuild z VS potřebuje resolver .NET SDK).
- Debug DLL čeká na debugger jen s proměnnou prostředí `FAND_WAIT_DEBUGGER=1`.

## Známá omezení

- Jedna relace FANDu na proces (globální stav interpretu).
- Kolečko myši původní PC-FAND neznal, interpretu se neposílá
  (Ctrl+kolečko mění velikost písma).
- Heslo (`star`) se v překryvném editoru jen skrývá barvou.
