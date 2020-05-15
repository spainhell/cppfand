#pragma once
#include "constants.h"
#include <string>
#include <windows.h>

struct TCrs
{
	WORD X = 0; WORD Y = 0; bool Big = false; bool On = false; bool Enabled = false; WORD Ticks = 0;
};
struct Wind { BYTE X, Y; };
const char FrameChars[] = { 0xDA, 0xC4, 0xBF, 0xC0, 0xC4, 0xD9, 0xB3, 0x20, 0xB3, 0xC9, 0xCD, 0xBB, 0xC8, 0xCD, 0xBC, 0xBA, 0x20, 0xBA, 0xC3, 0xC4, 0xB4 };

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
	void ScrWrBuf(WORD X, WORD Y, void* Buf, WORD L);
	void ScrRdBuf(WORD X, WORD Y, void* Buf, WORD L);
	void* ScrPush(WORD X, WORD Y, WORD SizeX, WORD SizeY);
	void ScrPop(WORD X, WORD Y, void* P);
	void ScrPopToGraph(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P, WORD DOfs);
	void ScrMove(WORD X, WORD Y, WORD ToX, WORD ToY, WORD L);
	void ScrColor(WORD X, WORD Y, WORD L, BYTE Color);
	TCrs CrsGet();
	void CrsSet(TCrs S);
	void CrsShow();
	void CrsHide();
	void CrsBig();
	void CrsNorm();
	void GotoXY(WORD X, WORD Y);
	BYTE WhereX();
	BYTE WhereY();
	void Window(BYTE X1, BYTE Y1, BYTE X2, BYTE Y2);

	void ScrWr();
	void CrsDark();
	void CrsBlink();
	void CrsGotoXY(WORD aX, WORD aY);

	void ScrPush1(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P);
	void ScrGetPtr(WORD X, WORD Y, WORD& DX, WORD& DI);
	
private:
	WORD* TxtCols;
	WORD* TxtRows;
	Wind* WindMin;
	Wind* WindMax;
	TCrs* Crs;
	
	HANDLE _handle;
	CHAR_INFO* _scrBuf;
	size_t _actualIndex;
	DWORD _inBuffer;
};

