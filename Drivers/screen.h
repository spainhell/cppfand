#pragma once
#include <stack>
#include <string>
#include <Windows.h>

const uint8_t bigCrsSize = 50;

struct TCrs
{
	short X = 0;
	short Y = 0;
	DWORD Size = 1;
	bool On = false;
	bool Enabled = false;
	WORD Ticks = 0;
};
struct Wind { uint8_t X = 0, Y = 0; };
const uint8_t FrameChars[] { 0xDA, 0xC4, 0xBF, 0xC0, 0xC4, 0xD9, 0xB3, 0x20, 0xB3, 0xC9, 0xCD, 0xBB, 0xC8, 0xCD, 0xBC, 0xBA, 0x20, 0xBA, 0xC3, 0xC4, 0xB4 };

enum ScrPosition { relative = 0, absolute = 1, actual = 2 };

struct WParam
{
	Wind Min{ 0, 0 };
	Wind Max{ 0,0 };
	WORD Attr = 0;
	TCrs Cursor; // { 0, 0, false, false, false, 0 };
	__int32 GrRoot = 0;
};

struct storeWindow
{
	WParam* wp = nullptr;
	COORD coord { 0, 0 };
	SMALL_RECT rect { 0, 0, 0, 0 };
	CHAR_INFO* content = nullptr;
};

struct Colors
{  // celkem 54x uint8_t
	uint8_t userColor[16]{ 0 };
	uint8_t mNorm = 0, mHili = 0, mFirst = 0, mDisabled = 0; // menu
	uint8_t sNorm = 0, sHili = 0, sMask = 0; // select
	uint8_t pNorm = 0, pTxt = 0; // prompt, verify, password
	uint8_t zNorm = 0; // message
	uint8_t lNorm = 0, lFirst = 0, lSwitch = 0; // last line
	uint8_t fNorm = 0; // first line
	uint8_t tNorm = 0, tCtrl = 0, tBlock = 0; // text edit
	uint8_t tUnderline = 0, tItalic = 0, tDWidth = 0, tDStrike = 0, tEmphasized = 0, tCompressed = 0, tElite = 0; // data edit
	uint8_t dNorm = 0, dHili = 0, dSubset = 0, dTxt = 0, dDeleted = 0, dSelect = 0; // -"-
	uint8_t uNorm = 0; // user screen
	uint8_t hNorm = 0, hHili = 0, hMenu = 0, hSpec = 0;
	uint8_t nNorm = 0;
	uint8_t ShadowAttr = 0;
	uint8_t DesktopColor = 0;
};

class Screen
{
public:
	Screen(short TxtCols, short TxtRows, Wind* WindMin, Wind* WindMax, TCrs* Crs);
	~Screen();
	void ReInit(short TxtCols, short TxtRows);
	size_t BufSize();

	void ScrClr(WORD X, WORD Y, WORD SizeX, WORD SizeY, char C, uint8_t Color);
	void ScrWrChar(WORD X, WORD Y, char C, uint8_t Color);
	void ScrWrStr(const std::string& s, uint8_t Color);
	void ScrWrStr(WORD X, WORD Y, const std::string& s, uint8_t Color) const;
	void ScrWrFrameLn(WORD X, WORD Y, uint8_t Typ, uint8_t Width, uint8_t Color);
	void ScrWrText(WORD X, WORD Y, const char* S);
	void ScrFormatWrText(WORD X, WORD Y, char const* const _Format, ...);
	void ScrFormatWrStyledText(WORD X, WORD Y, uint8_t Color, char const* const _Format, ...);
	void ScrWrBuf(WORD X, WORD Y, void* Buf, WORD L);
	void ScrWrCharInfoBuf(short X, short Y, CHAR_INFO* Buf, short L);
	bool ScrRdBuf(WORD X, WORD Y, CHAR_INFO* Buf, WORD L);
	void ScrMove(short X, short Y, short ToX, short ToY, short L);
	void ScrColor(WORD X, WORD Y, WORD L, uint8_t Color);
	void WriteChar(short X, short Y, char C, uint8_t attr, ScrPosition pos = relative);
	size_t WriteStyledStringToWindow(const std::string& text, uint8_t Attr);
	void LF();
	bool SetStyleAttr(char C, uint8_t& a);
	TCrs CrsGet();
	void CrsSet(TCrs S);
	void CrsShow();
	void CrsHide();
	void CrsBig();
	void CrsNorm();
	void GotoXY(WORD X, WORD Y, ScrPosition pos = relative);
	short WhereX();
	short WhereY();
	short WhereXabs();
	short WhereYabs();
	void Window(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2);

	void ScrWr();
	void CrsDark();
	void CrsBlink();
	void CrsGotoXY(WORD aX, WORD aY);

	int ScrPush1(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P);

	size_t ScreenCount();
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
	size_t _actualIndex;
	DWORD _inBuffer;
};

