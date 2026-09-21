#pragma once
// Hostitelsky rezim: interpret FANDu bezi uvnitr cizi aplikace (napr. WPF okna),
// ktera si obrazovku vykresluje sama a klavesy posila frontou.
// V konzolovem rezimu (cppfand.exe) je vse vypnute a chovani je puvodni.
#include <cstdint>
#include <string>
#include <stdexcept>

namespace FandHost
{
	// --- rezim ---------------------------------------------------------------
	bool IsEnabled();
	void Enable();

	// Pozadavek na zastaveni interpretu (napr. zavreni okna hostitele).
	void RequestStop();
	bool StopRequested();

	// Vyjimka, kterou v hostitelskem rezimu nahrazujeme exit()/Halt().
	class HaltException : public std::runtime_error
	{
	public:
		explicit HaltException(int code) : std::runtime_error("FAND halt"), Code(code) {}
		int Code;
	};

	// Fatalni chyba pri startu (FAND.CFG, FAND.RES): v konzoli vypise, pocka a ukonci program,
	// v hostitelskem rezimu vyhodi HaltException, aby nespadl hostitel.
	[[noreturn]] void Fatal(const std::string& message, int exitCode);

	// --- editace pole v hostiteli --------------------------------------------
	// Jednoradkovou editaci (DataEditor::EditTxt) muze prevzit hostitel a zobrazit
	// vlastni vstupni prvek. Interpret pozadavek zapise a ceka na vysledek.
	struct FieldEditRequest
	{
		int X = 0;            // sloupec, 0-based, absolutne na obrazovce
		int Y = 0;            // radek, 0-based
		int Width = 0;        // pocet zobrazenych bunek
		int MaxLen = 0;       // maximalni delka textu
		int FieldType = 0;    // hodnota FieldType (viz Common), pro filtrovani znaku
		int Pos = 1;          // pozice kurzoru, 1-based
		int InsertMode = 1;   // 1 = vkladani, 0 = prepis
		int Star = 0;         // 1 = heslo (hvezdicky)
		int DelOnFirstKey = 0;// 1 = prvni klavesa smaze obsah
		int TimeoutMs = 0;    // 0 = bez limitu, jinak po uplynuti ukoncit jako Esc
		char Text[256] = { 0 };   // CP852
		char Mask[64] = { 0 };    // maska pole (DD.MM.YY, ###-###, ...), muze byt prazdna
		char Attr = 0;        // barevny atribut pole
	};

	struct FieldEditResult
	{
		char Text[256] = { 0 };   // CP852
		int Pos = 1;              // 1-based
		int InsertMode = 1;
		uint16_t Key = 0;         // ukoncovaci klavesa v kodovani PressedKey::KeyCombination
	};

	// --- aktualni pole v prohlizecim rezimu ----------------------------------
	// DataEditor hlasi, ktere pole je prave zvyraznene (0-based souradnice, delka v bunkach),
	// aby hostitel umel napr. Ctrl+C zkopirovat jeho obsah, i kdyz se zrovna needituje.
	// text = cela hodnota pole (CP852), i kdyz se na obrazovku vejde jen cast
	void SetCurrentField(int x0, int y0, int len, const std::string& text);
	void ClearCurrentField();
	bool GetCurrentField(int& x0, int& y0, int& len);
	std::string GetCurrentFieldText();

	bool FieldEditEnabled();
	void SetFieldEditEnabled(bool enabled);

	// Vlakno interpretu: zapise pozadavek a blokuje do prijeti vysledku.
	// Vraci false, pokud hostitel editaci neprevzal (interpret pokracuje puvodni cestou).
	bool RunFieldEdit(const FieldEditRequest& request, FieldEditResult& result);

	// Vlakno hostitele: vyzvedne cekajici pozadavek (true = je co editovat).
	bool PollFieldEdit(FieldEditRequest& request);
	// Vlakno hostitele: preda vysledek a probudi interpret.
	void CompleteFieldEdit(const FieldEditResult& result);
}
