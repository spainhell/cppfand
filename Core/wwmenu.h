#pragma once
#include "access.h"
#include "OldDrivers.h"

#include "rdrun.h"
#include "models/Instr.h"

const WORD sfCursorVis = 0x0002;
const WORD sfCursorBig = 0x0004;
const WORD sfShadow = 0x0008;
const WORD sfFramed = 0x0010;
const WORD sfFrDouble = 0x0020;
const WORD sfFocused = 0x0040;
const WORD sfModal = 0x2000;

class TRect
{
public:
	TPoint A, Size;
	bool Contains(TPoint* P);
};

class TWindow
{
public:
	TWindow();
	//TWindow(uint8_t C1, uint8_t R1, uint8_t C2, uint8_t R2, WORD Attr, pstring top, pstring bottom, bool SaveLL);
	void InitTWindow(uint8_t C1, uint8_t R1, uint8_t C2, uint8_t R2, WORD Attr, const std::string& top, const std::string& bottom, bool SaveLL);
	virtual ~TWindow();
	TPoint Orig;
	TPoint Size;
	TPoint Shadow;
	int SavedW = 0, SavedLLW = 0;
	WORD State = 0;
	bool WasCrsEnabled = false;
	void Assign(uint8_t C1, uint8_t R1, uint8_t C2, uint8_t R2);
	WORD Col1();
	WORD Row1();
	WORD Col2();
	WORD Row2();
	bool Contains(TPoint* T);
	bool GetState(WORD Flag);
	void SetState(WORD Flag, bool On);
};


class TMenu : public TWindow
{
public:
	TMenu();
	void InitTMenu();
	virtual ~TMenu();
	TMenu* parent = nullptr;
	WORD iTxt = 0, nTxt = 0, mx = 0, my = 0;
	RdbD* HlpRdb = nullptr;
	uint8_t Palette[4]; // norm, curr, char, disabled
	bool IsBoxS = false;
	void ClearHlp();
	virtual bool Enabled(WORD I) = 0;
	virtual bool ExecItem(WORD& I) = 0;
	bool FindChar(char c1);
	virtual std::string GetHlpName() = 0;
	virtual void GetItemRect(WORD I, TRect* R) = 0;
	virtual std::string GetText(short I) = 0;
	void HandleEvent();
	bool IsMenuBar();
	void LeadIn(TPoint* T);
	void Next();
	bool ParentsContain(TPoint* T);
	void Prev();
	bool UnderMenuBar();
	void WrText(WORD I);
	void SetPalette(Instr_menu* aPD);
protected:
	ChoiceD* getChoice(size_t order);
	void insertChoices(std::vector<ChoiceD*>& choices, bool isMenuBar);
	void countChoices(bool isMenuBar);
	std::vector<ChoiceD*> choices;
	std::vector<ChoiceD*> filteredChoices;
	
};

class TMenuBox : public TMenu
{
public:
	TMenuBox();
	void InitTMenuBox(WORD C1, WORD R1);
	WORD Exec(WORD IStart);
	void GetItemRect(WORD I, TRect* R) override;
};

class TMenuBoxS : public TMenuBox
{
public:
	TMenuBoxS(WORD C1, WORD R1, pstring Msg);
	pstring MsgTxt;
	bool Enabled(WORD I) override;
	bool ExecItem(WORD& I) override;
	std::string GetHlpName() override;
	std::string GetText(short I) override;
};

class TMenuBoxP : public TMenuBox
{
public:
	TMenuBoxP(WORD C1, WORD R1, TMenu* aParent, Instr_menu* aPD);
	Instr_menu* PD;
	std::string HdTxt;
	bool Enabled(WORD I) override;
	bool ExecItem(WORD& I) override;
	std::string GetHlpName() override;
	std::string GetText(short I) override;
	void call();
};

class TMenuBar : public TMenu
{
public:
	TMenuBar() = default;
	TMenuBar(WORD C1, WORD R1, WORD Cols);
	void InitTMenuBar(WORD C1, WORD R1, WORD Cols);
	WORD nBlks = 0;
	uint8_t DownI[30]{ '\0' };
	WORD Exec();
	virtual TMenuBox* GetDownMenu();
	void GetItemRect(WORD I, TRect* R) override;
};

//class TMenuBarS : public TMenuBar
//{
//public:
//	TMenuBarS();
//	TMenuBarS(WORD MsgNr);
//	std::string MsgTxt;
//	TMenuBox* GetDownMenu() override;
//	std::string GetHlpName() override;
//	std::string GetText(short I) override;
//};

class TMenuBarP : public TMenuBar
{
public:
	TMenuBarP();
	TMenuBarP(Instr_menu* aPD);
	Instr_menu* PD;
	bool Enabled(WORD I) override;
	bool ExecItem(WORD& I) override;
	TMenuBox* GetDownMenu() override;
	std::string GetHlpName() override;
	std::string GetText(short I) override;
};

WORD Menu(WORD MsgNr, WORD IStart);
bool PrinterMenu(WORD Msg);
void MenuBarProc(Instr_menu* PD);
std::string GetHlpText(RdbD* R, std::string S, bool ByName, WORD& IRec);
void DisplayLastLineHelp(RdbD* R, std::string Name, bool R24);
