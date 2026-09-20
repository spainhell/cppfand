# WpfHost – okenní hostitel cppfandu

Interpret FANDu (projekty Core, DataEditor, …) běží beze změny logiky uvnitř
`cppfand_dll.dll` na vlastním vlákně. Tato WPF aplikace (`cppfand-wpf.exe`)
mu dělá obrazovku a klávesnici:

- **Obrazovka 80×25** se kreslí z bufferu, který si drží `Drivers/screen.cpp`
  (znak CP852 + atribut). Hostitel ji čte přes `FandGetScreen` ~30× za sekundu
  a překreslí jen při změně čísla verze. Plné a stínované bloky (logo Účta)
  se kreslí jako obdélníky, ostatní znaky písmem Cascadia Mono / Consolas.
- **Klávesy** se posílají přes `FandPushKey` ve tvaru `KEY_EVENT_RECORD`
  konzole, takže `PressedKey` a všechny klávesové zkratky FANDu zůstávají.
- **Editace jednořádkového pole** (`DataEditor::EditTxt`) se předává
  hostiteli (`Drivers/host.h`, `FandPollFieldEdit` / `FandCompleteFieldEdit`).
  Nad polem se objeví běžný `TextBox`: schránka (Ctrl+C/V/X), označení myší,
  Insert přepíná vkládání, filtr znaků podle typu pole, u data se vložený
  text převede na masku (`DD.MM.YY`). Enter, Esc, Tab, šipky nahoru/dolů,
  PgUp/PgDn, F-klávesy a Ctrl/Alt kombinace editaci ukončí a klávesu dostane
  FAND, který ji zpracuje stejně jako v konzoli.

## Spuštění

```
cppfand-wpf.exe <složka s FAND.CFG a FAND.RES> <pracovní adresář> <úloha>
cppfand-wpf.exe C:\ucto C:\ucto UCTO2024
```

Bez parametrů se zobrazí dialog; hodnoty se pamatují v
`%AppData%\cppfand\wpfhost.json`. Úloha je identifikátor bez cesty a přípony.

Klávesy hostitele: Shift+Insert vloží text ze schránky jako psaní,
Ctrl+Shift+C zkopíruje celou obrazovku, Ctrl+kolečko mění velikost písma.

## Sestavení a nasazení

- Projekt cílí na **.NET Framework 4.8** (`net48`) i **.NET 10**
  (`net10.0-windows`), v platformách **x64** i **x86**. Výstup leží vedle
  nativních binárek: `x64\Release\wpf\<TFM>\` pro x64 a `Release\wpf\<TFM>\`
  pro 32bit, která používá 32bitovou `cppfand_dll.dll` z konfigurace Win32.
  Pro klienty stačí složka `net48`:
  .NET Framework 4.8 je součástí Windows 10 (od 1903) a 11, nic dalšího
  se neinstaluje. Nativní `cppfand_dll.dll` i `cppfand.exe` jsou linkované
  se statickou runtime knihovnou (`/MT`), takže nepotřebují Visual C++
  Redistributable; závisí jen na kernel32, user32 a comdlg32.
- V kódu proto nesmí být API novější než 4.8 (žádné `Math.Clamp`,
  rozsahy `[..]`, `char.IsAsciiDigit`, `System.Text.Json`,
  `OpenFolderDialog`); rozdíly řeší `#if NETFRAMEWORK` / `#if NETCOREAPP`.
- `dotnet build WpfHost -c Release -p:Platform=x64` přeloží obě verze,
  nativní `cppfand_dll.dll` se do výstupu kopíruje ze stejné konfigurace
  solution (nejdřív přeložit `2_DynamicLibrary`).
- Ve Visual Studiu je projekt v `cppfand.sln` jako `3_WpfHost` se závislostí
  na `2_DynamicLibrary`. Aby ho VS umělo načíst a přeložit, musí být
  nainstalován workload **.NET desktop development** (samotný dotnet SDK
  nestačí, MSBuild z VS potřebuje resolver .NET SDK).
- Debug DLL čeká na debugger jen s proměnnou prostředí `FAND_WAIT_DEBUGGER=1`.

## Známá omezení

- Jedna relace FANDu na proces (globální stav interpretu).
- Myš se interpretu neposílá (konzolová verze ji také zahazuje).
- Heslo (`star`) se v překryvném editoru jen skrývá barvou.
