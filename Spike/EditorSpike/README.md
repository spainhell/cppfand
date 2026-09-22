# Spike: textový editor PC-FANDu ve WPF

Odpovídá na otázku, jestli se dá `TextEditor/` nahradit běžnou WPF komponentou,
která umí barvy řízené řídicími znaky a zároveň myš, označování a schránku.

**Samostatná aplikace. Do interpretu nesahá a není v `cppfand.sln`.**
Až odpoví na své otázky, dá se celá složka `Spike/` smazat.

```
dotnet build Spike\EditorSpike -c Debug -f net48
bin\Debug\net48\editor-spike.exe [soubor]
```

Bez parametru otevře `vzorek-help.t00`.

## Závěr

`TextBox` **ne** — má formátování jednotné pro celý obsah, jednotlivé znaky
obarvit nejde. `RichTextBox` by barvy uměl, ale pracuje nad `FlowDocument`
a pozice adresuje `TextPointer`y, kdežto FAND pracuje s indexy znaků.

Spike stojí na **AvalonEditu** (NuGet `AvalonEdit` 6.3.1.120, MIT). Dokument
je prostý text, offsety jsou indexy znaků, takže mapování na pozice ve FANDu
je 1:1. Ověřeno, že balíček se obnoví a projekt přeloží pro `net48`
i `net10.0-windows`, tedy pro obě platformy, které cílí WpfHost.

## Co je ověřené

| co | jak dopadlo |
| --- | --- |
| barvy podle řídicích znaků | funguje, obojí vykreslení zvlášť (viz níže) |
| české znaky CP852 | správně, na reálném textu z Účta |
| myš, tažením označit text | funguje |
| schránka (Ctrl+C) | funguje **a zachová řídicí znaky** — zkopírovaný úsek si nese formátování |
| sloupcový blok (`ColBlock`) | funguje, `EnableRectangularSelection` + Alt+tažení, kopíruje se jako obdélník |
| F12 místo ScrollLocku | přepíná mezi editací a prohlížením |
| tvrdý vs. měkký konec řádku | rozlišitelné přes `DocumentLine.DelimiterLength` (2 = CR LF, 1 = CR) |
| 1 MB / 18 625 řádků | načtení včetně průchodu celým textem za **46 ms** |
| psaní do 1 MB textu | žádný měřitelný rozdíl proti 16 KB |

Měření psaní je celé v režii `SendKeys` (~64 ms na znak), takže neizoluje
náklad překreslení; říká jen, že mezi 16 KB a 1 MB není pozorovatelný rozdíl.

## Dvě různá vykreslení, ne jedno

To je hlavní věc, která z kódu není vidět na první pohled. `TextEditorScreen.cpp`
má dvě funkce a chovají se **odlišně**:

- `EditWrline` (editace) — řídicí znak **je vidět a zabírá sloupec**, zobrazí se
  jako písmeno `c + 64` (`^S` jako `S`) v barvě svého atributu. Okolní text má
  vždycky `TxtColor`, uvnitř bloku `BlockColor`. Atribut se na text **nepřenáší**.
- `ScrollWrline` (prohlížení) — řídicí znak **je skrytý, sloupec nezabírá**
  a přepíná `ColorOrd`; barvu podle posledního aktivního atributu dostává
  až navazující text. `0x0C` se kreslí jako plný blok (znak 219).

Spike umí obojí, přepíná se v liště nebo klávesou **F12**. V originálu na tom
sedí ScrollLock — `EDEVINPT.PAS:10` čte BIOS bajt 0040:0017 a testuje bit 0x10,
v portu je to `TextEditor.cpp:1274`. Dnešní klávesnice ScrollLock často nemají,
takže ho zastupuje F12, kterou PC-FAND nepoužívá.

V AvalonEditu obě vykreslení dělá jeden `VisualLineElementGenerator`: v editaci
vrací `FormattedTextElement` s písmenem, v prohlížení prvek nulové šířky.

## Vyřešeno: nekonzistentní odebrání atributu byla chyba portu

`ColorOrd` je řetězec aktivních přepínačů a řídicí znak ho přepíná — když v něm
znak je, odebere se, jinak přidá. V C++ portu bylo odebrání na dvou místech
různé: `ScrollWrline` (`TextEditorScreen.cpp`) odebíral `CO.substr(0,pp) +
CO.substr(pp+1,…)`, tedy jen ten jeden znak, kdežto `SetColorOrd`
(`TextEditor.cpp:1019`) volal `co.erase(pp)`, což v `std::string` zahodí všechno
od pozice `pp` do konce.

Originál je jednoznačný — obě místa odebírají **jen ten jeden znak**:

```pascal
{ EDGLOBAL.PAS, SetColorOrd }          { EDSCREEN.PAS, ScrollWrline }
pp := pos(T^[I], CO);                  pp := pos(cc, CO);
if pp > 0 then                         if pp > 0 then
  CO := copy(CO,1,pp-1)                  CO := copy(CO,1,pp-1)
      + copy(CO,pp+1,len-pp)                 + copy(CO,pp+1,len-pp)
else CO := CO + T^[I];                 else CO := CO + cc;
```

Šlo tedy o chybu při přepisu do C++, ne o zamýšlený rozdíl. Opraveno na
`co.erase(pp, 1)`.

## Nález: help texty využívají jen část atributů

Přepínačů je sedm (`TextEditor.h:249`), ale v `HELP.T00` z Účta 2024 jsou
prakticky jen `^S` a `^B`; `^W`, `^Q`, `^D`, `^E` a `^A` se objevují ojediněle.
Sedmice atributů je tiskařská, texty nápovědy z ní berou zlomek. Proto má spike
barvy v UI přepínatelné — jinak by se většina mapování nedala zrakem ověřit.

## Co spike záměrně neřeší

- **Napojení na interpret.** Vzor by byl stejný jako u jednořádkových polí
  (`Drivers/host.h`, `RunFieldEdit` / `PollFieldEdit` / `CompleteFieldEdit`),
  tedy přibylo by `RunTextEdit`. Neověřeno.
- **Barvy z `FAND.CFG`.** `ColKey[]` plní `TextEditor.cpp:3192` z `screen.colors`,
  které se čtou v `CfgFile::ReadVideoAndColors` až za celým blokem `Spec`.
  Spike je místo parsování nabízí v UI.
- **Limit 255 znaků na řádek se nezavádí.** V originálu plynul z pascalovského
  `string[255]`, ne z formátu textu, takže ho náhrada přebírat nemá. Stavový
  řádek délku ukazuje, ale nijak ji neomezuje.
- **Okraje, zalamování, zarovnání, hledání, náhrada, tisk.** Zůstává otázka,
  kolik z těch ~6 100 řádků `TextEditor/` by zůstalo FANDu a co by převzal editor.
- **Přírůstkový přepočet stavu.** `AttributeState` přepočítává celý dokument
  při každé změně. Na 1 MB to zatím nevadí, ale je to první věc na optimalizaci.

## Vzorek dat

`vzorek-help.t00` je 16 KB vyříznutých z `C:\PCFAND\orig_ulohy\UCTO2024\HELP.T00`
od offsetu 335 872 — souvislý čitelný úsek s pěti ze sedmi atributů. Celý
`HELP.T00` jím není proto, že začíná binární hlavičkou a čitelné úseky se v něm
střídají s binárními.
