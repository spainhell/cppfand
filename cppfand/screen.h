#pragma once
#include <stack>

#include "constants.h"
#include <string>
#include <windows.h>

struct TCrs
{
	short X = 0; short Y = 0;
	bool Big = false; bool On = false; bool Enabled = false;
	WORD Ticks = 0;
};
struct Wind { BYTE X = 0, Y = 0; };
const BYTE FrameChars[] { 0xDA, 0xC4, 0xBF, 0xC0, 0xC4, 0xD9, 0xB3, 0x20, 0xB3, 0xC9, 0xCD, 0xBB, 0xC8, 0xCD, 0xBC, 0xBA, 0x20, 0xBA, 0xC3, 0xC4, 0xB4 };

enum Position { relative = 0, absolute = 1, actual = 2 };

struct WParam
{
	Wind Min{ 0, 0 };
	Wind Max{ 0,0 };
	WORD Attr = 0;
	TCrs Cursor; // { 0, 0, false, false, false, 0 };
	longint GrRoot = 0;
};

struct storeWindow
{
	WParam* wp = nullptr;
	COORD coord { 0, 0 };
	SMALL_RECT rect { 0, 0, 0, 0 };
	CHAR_INFO* content = nullptr;
};

struct Colors
{  // celkem 54x BYTE
	BYTE userColor[16]{ 0 };
	BYTE mNorm = 0, mHili = 0, mFirst = 0, mDisabled = 0; // menu
	BYTE sNorm = 0, sHili = 0, sMask = 0; // select
	BYTE pNorm = 0, pTxt = 0; // prompt, verify, password
	BYTE zNorm = 0; // message
	BYTE lNorm = 0, lFirst = 0, lSwitch = 0; // last line
	BYTE fNorm = 0; // first line
	BYTE tNorm = 0, tCtrl = 0, tBlock = 0; // text edit
	BYTE tUnderline = 0, tItalic = 0, tDWidth = 0, tDStrike = 0, tEmphasized = 0, tCompressed = 0, tElite = 0; // data edit
	BYTE dNorm = 0, dHili = 0, dSubset = 0, dTxt = 0, dDeleted = 0, dSelect = 0; // -"-
	BYTE uNorm = 0; // user screen
	BYTE hNorm = 0, hHili = 0, hMenu = 0, hSpec = 0;
	BYTE nNorm = 0;
	BYTE ShadowAttr = 0;
	BYTE DesktopColor = 0;
};

class Screen
{
public:
	Screen(WORD* TxtCols, WORD* TxtRows, Wind* WindMin, Wind* WindMax, TCrs* Crs);
	~Screen();
	size_t BufSize();

	void ScrClr(WORD X, WORD Y, WORD SizeX, WORD SizeY, char C, BYTE Color);
	void ScrWrChar(WORD X, WORD Y, char C, BYTE Color);
	void ScrWrStr(WORD X, WORD Y, std::string S, BYTE Color);
	void ScrWrFrameLn(WORD X, WORD Y, BYTE Typ, BYTE Width, BYTE Color);
	void ScrWrText(WORD X, WORD Y, const char* S);
	void ScrFormatWrText(WORD X, WORD Y, char const* const _Format, ...);
	void ScrFormatWrStyledText(WORD X, WORD Y, BYTE Color, char const* const _Format, ...);
	void ScrWrBuf(WORD X, WORD Y, void* Buf, WORD L);
	void ScrWrCharInfoBuf(short X, short Y, CHAR_INFO* Buf, short L);
	bool ScrRdBuf(WORD X, WORD Y, CHAR_INFO* Buf, WORD L);
	void ScrMove(short X, short Y, short ToX, short ToY, short L);
	void ScrColor(WORD X, WORD Y, WORD L, BYTE Color);
	void WriteChar(short X, short Y, char C, BYTE attr, Position pos = relative);
	size_t WriteStyledStringToWindow(std::string text, BYTE Attr);
	void LF();
	bool SetStyleAttr(char C, BYTE& a);
	TCrs CrsGet();
	void CrsSet(TCrs S);
	void CrsShow();
	void CrsHide();
	void CrsBig();
	void CrsNorm();
	void GotoXY(WORD X, WORD Y, Position pos = relative);
	short WhereX();
	short WhereY();
	short WhereXabs();
	short WhereYabs();
	void Window(BYTE X1, BYTE Y1, BYTE X2, BYTE Y2);

	void ScrWr();
	void CrsDark();
	void CrsBlink();
	void CrsGotoXY(WORD aX, WORD aY);

	int ScrPush1(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P);
	// void ScrGetPtr(WORD X, WORD Y, WORD& DX, WORD& DI);

	void pushScreen(storeWindow sw);
	storeWindow popScreen();
	int SaveScreen(WParam* wp, short c1, short r1, short c2, short r2);
	WParam* LoadScreen(bool draw);
	Colors colors;

private:
	short TxtCols;
	short TxtRows;
	short MaxColsIndex;
	short MaxRowsIndex;
	Wind* WindMin;
	Wind* WindMax;
	TCrs* Crs;
	short actualWindowRow = 1; // cislovane od 1
	short actualWindowCol = 1; // cislovane od 1

	std::stack<storeWindow> _windowStack;

	HANDLE _handle;
	//CHAR_INFO* _scrBuf;
	size_t _actualIndex;
	DWORD _inBuffer;
};

