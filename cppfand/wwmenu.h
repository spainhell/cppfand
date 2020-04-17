#pragma once
#include "drivers.h"

struct RdbD;
struct Instr;
typedef Instr* InstrPtr;

class TRect
{
public:
	TPoint A, Size;
	bool Contains(TPoint P);
};

class TWindow // TODO: : TObject
{
public:
	TWindow(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, string top, string bottom, bool SaveLL);
	virtual ~TWindow() = default;

	TPoint Orig, Size, Shadow;
	longint SavedW, SavedLLW;
	WORD State;
	bool WasCrsEnabled;
	//destructor Done; virtual;
	void Assign(BYTE C1, BYTE R1, BYTE C2, BYTE R2);
	WORD Col1();
	WORD Row1();
	WORD Col2();
	WORD Row2();
	bool Contains(TPoint T);
	bool GetState(WORD Flag);
	void SetState(WORD Flag, bool On);
};


class TMenu : TWindow
{
public:
	TMenu();

	TMenu* parent;
	WORD iTxt, nTxt, mx, my;
	RdbD* HlpRdb;
	BYTE Palette[4]; // norm, curr, char, disabled
	bool IsBoxS;
	// destructor Done; virtual;
	void ClearHlp();
	virtual bool Enabled(WORD I);
	virtual bool ExecItem(WORD I);
	bool FindChar();
	virtual string GetHlpName();
	virtual void GetItemRect(WORD I, TRect R);
	virtual string GetText(integer I);
	void HandleEvent();
	bool IsMenuBar();
	void LeadIn(TPoint T);
	void Next();
	bool ParentsContain(TPoint T);
	void Prev();
	bool UnderMenuBar();
	void WrText(WORD I);
	void SetPalette(InstrPtr aPD);
};

class TMenuBox : TMenu
{
public:
	TMenuBox(WORD C1, WORD R1);
	WORD Exec(WORD IStart);
	virtual void GetItemRect(WORD I, TRect R);
};

class TMenuBoxS : TMenuBox
{
public:
	TMenuBoxS(WORD C1, WORD R1, string* Msg);
	virtual string GetHlpName();
	virtual string GetText(integer I);
};
