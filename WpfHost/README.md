# WpfHost – okenní hostitel cppfandu

Interpret FANDu (projekty Core, DataEditor, …) běží beze změny logiky uvnitř
`cppfandlib.dll` na vlastním vlákně. Tato WPF aplikace (`cppfand-wpf.exe`)
mu dělá obrazovku a klávesnici:

- **Obrazovka 80×25** se kreslí z bufferu, který si drží `Drivers/screen.cpp`
  (znak CP852 + atribut). Hostitel ji čte přes `FandGetScreen` ~30× za sekundu
  a překreslí jen při změně čísla verze, a to jen změněné řádky (každý řádek
  je samostatný `DrawingVisual`, kurzor také). Plné a stínované bloky (logo Účta)
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
  Insert (nebo F11) přepíná vkládání, filtr znaků podle typu pole, u data se vložený
  text převede na masku (`DD.MM.YY`). Enter, Esc, Tab, šipky nahoru/dolů,
  PgUp/PgDn, F-klávesy a Ctrl/Alt kombinace editaci ukončí a klávesu dostane
  FAND, který ji zpracuje stejně jako v konzoli.
- **Tisk a export sestav**: viz níže.

## Tisk, PDF, Word a e-mail

Výstup sestavy (`PRINTER.TXT`, kapitoly R i auto-sestavy) i jakýkoli text
v prohlížeči (`TextEditWindow`) má nahoře panel **Tisk… (F6) · PDF… · Word… ·
ODT… · E-mailem… · Nastavení e-mailu…**.

- Text se rozloží na stránky podle `0x0C` (FormFeed v `Report.cpp`) a přepínací
  znaky atributů se kreslí stylem písma Consolas: `^B`/`^D` tučně, `^W` kurzíva,
  `^S` podtržení, `^E` zhuštěně (17 cpi), `^A` elite (12 cpi), `^Q` dvojitá šířka.
  ESC sekvence tiskáren z `FAND.CFG` se na Windows nepoužívají.
- Písmo se přizpůsobí papíru: nejdelší řádek se vejde na šířku, stránka sestavy
  na výšku (7–12 pt); široké sestavy jdou na šířku papíru. Delší stránky se dělí.
  Rozvržení (`ReportLayout.cs`) sdílí tisk, PDF, DOCX i ODT.
- **Tisk** jde přes běžný tiskový dialog Windows (tiskárna, kopie, rozsah stran).
  Stejný dialog otevírají i cesty z jazyka FANDu, které dřív tiskly na LPT:
  `report(... ASSIGN=LPT1 ...)` (`TIMES=` = počet kopií), automatický tisk
  auto-sestavy (`AutoRprtPrint` ve `FAND.CFG`), `printtxt` a F6 v nápovědě.
  Interpret čeká v `FandHost::RunPrint` (`Drivers/host.h`), hostitel požadavek
  vyzvedne přes `FandPollPrint` / `FandGetPrintText` / `FandCompletePrint`.
- **PDF** vytváří PDFsharp (MIT) s vloženým písmem Consolas. **DOCX a ODT** se
  skládají ručně (`ReportOffice.cs`, `SimpleZip.cs`): každý řádek je odstavec
  s pevnou výškou řádku, stránky sestavy začínají novou stránkou.
- **E-mail** pošle sestavu jako PDF přílohu. Bez nastaveného SMTP serveru se
  otevře nový e-mail ve výchozím poštovním klientovi (Simple MAPI: klasický
  Outlook, Thunderbird, eM Client…); nový Outlook ani webová pošta MAPI neumí,
  program pak nabídne nastavení SMTP. S vyplněným serverem (**Nastavení e-mailu…**)
  pošle e-mail sám; nastavení je v `%AppData%\cppfand\mail.txt`, heslo
  zašifrované DPAPI pro daného uživatele Windows. Podporovaný je STARTTLS
  (obvykle port 587), implicitní TLS na portu 465 `SmtpClient` neumí.

## Spuštění

Obvyklé nasazení je zkopírovat obsah složky `net48` přímo do adresáře úlohy.

**Bez parametrů naskočí hlavní menu FANDu** (Ladit úlohu, Provést úlohu,
Instalace úlohy, Editace textu, Dos, Konec), stejně jako to dělal PC-FAND —
`runfand.cpp:389` přeskočí větvení a vykreslí plochu s menu. `FAND.CFG`
a `FAND.RES` se hledají vedle `cppfand-wpf.exe` a potom v aktuálním adresáři;
teprve když nejsou ani tam, zeptá se dialog na cesty.

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

Dialog se objeví jen tehdy, když se `FAND.CFG` a `FAND.RES` nenajdou — zeptá se
na obě cesty a hodnoty si zapamatuje v `%AppData%\cppfand\wpfhost.txt`. Pole pro
úlohu v něm smí zůstat prázdné, pak se jde taky do hlavního menu.

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
Mac nemá klávesu Insert, proto ji všude (terminál, editace pole, textový
editor, i se Shift/Ctrl) zastupuje F11; FAND sám F11 nepoužívá.

## Sestavení a nasazení

- Projekt cílí na **.NET Framework 4.8** (`net48`) i **.NET 10**
  (`net10.0-windows`), v platformách **x64** i **x86**. Výstup leží vedle
  nativních binárek: `x64\Release\wpf\<TFM>\` pro x64 a `Release\wpf\<TFM>\`
  pro 32bit, která používá 32bitovou `cppfandlib.dll` z konfigurace Win32.
  Pro klienty stačí složka `net48` (celá, včetně knihoven PDFsharp a jeho
  závislostí):
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
- Nový tiskový dialog Windows 11 u tisku z WPF nezobrazuje náhled.
- Tečkové příkazy textu pro tisk (`.pl`, `.he`, `.fo`, …) se při tisku přes
  Windows neinterpretují, stránkuje se jen podle `0x0C` a výšky papíru.
