#pragma once
#include "access.h"
#include "drivers.h"
#include "constants.h"
#include "rdrun.h"

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

class TWindow // TODO: : TObject
{
public:
	TWindow();
	TWindow(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, pstring top, pstring bottom, bool SaveLL);
	virtual ~TWindow();
	TPoint Orig = {0, 0};
	TPoint Size = { 0, 0 };
	TPoint Shadow = {0, 0};
	longint SavedW = 0, SavedLLW = 0;
	WORD State = 0;
	bool WasCrsEnabled = false;
	void Assign(BYTE C1, BYTE R1, BYTE C2, BYTE R2);
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
	virtual ~TMenu();
	TMenu* parent;
	WORD iTxt, nTxt, mx, my;
	RdbD* HlpRdb;
	BYTE Palette[4]; // norm, curr, char, disabled
	bool IsBoxS;
	void ClearHlp();
	virtual bool Enabled(WORD I);
	virtual bool ExecItem(WORD I);
	bool FindChar();
	virtual pstring GetHlpName();
	virtual void GetItemRect(WORD I, TRect R);
	virtual pstring GetText(integer I);
	void HandleEvent();
	bool IsMenuBar();
	void LeadIn(TPoint* T);
	void Next();
	bool ParentsContain(TPoint* T);
	void Prev();
	bool UnderMenuBar();
	void WrText(WORD I);
	void SetPalette(Instr* aPD);
};

class TMenuBox : public TMenu
{
public:
	TMenuBox(WORD C1, WORD R1);
	WORD Exec(WORD IStart);
	void GetItemRect(WORD I, TRect R) override;
};

typedef TMenuBox* PMenuBox;
typedef TMenu* PMenu;

class TMenuBoxS : public TMenuBox
{
public:
	TMenuBoxS(WORD C1, WORD R1, pstring* Msg);
	virtual pstring GetHlpName();
	virtual pstring GetText(integer I);
};

class TMenuBoxP : public TMenuBox
{
public:
	TMenuBoxP(WORD C1, WORD R1, PMenu aParent, Instr* aPD);
	Instr* PD;
	ChoiceD* CRoot;
	pstring* HdTxt;
	virtual bool Enabled(WORD I);
	virtual bool ExecItem(WORD& I);
	virtual pstring GetHlpName();
	virtual pstring GetText(integer I);
};

class TMenuBar : public TMenu
{
public:
	TMenuBar(WORD C1, WORD R1, WORD Cols);
	WORD nBlks;
	BYTE DownI[30];
	WORD Exec();
	virtual bool GetDownMenu(PMenuBox* W);
	virtual void GetItemRect(WORD I, TRect* R);
};

class TMenuBarS : public TMenuBar
{
public:
	TMenuBarS(WORD MsgNr);
	pstring* MsgTxt;
	virtual bool GetDownMenu(PMenuBox* W);
	virtual pstring GetHlpName();
	pstring GetText(integer I);
};

class TMenuBarP : public TMenuBar
{
	TMenuBarP Init(Instr* aPD);
	Instr* PD;
	ChoiceD* CRoot;
	virtual bool Enabled(WORD I);
	virtual bool ExecItem(WORD& I);
	virtual bool GetDownMenu(PMenuBox* W);
	virtual pstring GetHlpName();
	virtual pstring GetText(integer I);
};

