#include "screen.h"
#include <exception>
#include "pstring.h"

const unsigned int BUFFSIZE = 128 * 1024;

Screen::Screen(WORD* TxtCols, WORD* TxtRows, Wind* WindMin, Wind* WindMax, TCrs* Crs)
{
	this->TxtCols = TxtCols;
	this->TxtRows = TxtRows;
	this->WindMin = WindMin;
	this->WindMax = WindMax;
	this->Crs = Crs;
	
	_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (_handle == INVALID_HANDLE_VALUE) { throw std::exception("Cannot open console output handle."); }
	SetConsoleScreenBufferSize(_handle, { (short)*TxtCols, (short)*TxtRows });
	_scrBuf = new CHAR_INFO[BUFFSIZE];
	_actualIndex = 0;
	_inBuffer = 0;
}

Screen::~Screen()
{
	delete[] _scrBuf;
}

size_t Screen::BufSize()
{
	return BUFFSIZE;
}

void Screen::ScrClr(WORD X, WORD Y, WORD SizeX, WORD SizeY, char C, BYTE Color)
{
	//WORD DX = 0;
	//WORD DI = 0;
	//ScrGetPtr(X, Y, DX, DI);
	//
	//CHAR_INFO* videobufferESDI = new CHAR_INFO[128*1024];
	//CHAR_INFO AhAl;
	//AhAl.Char.AsciiChar = C;
	//AhAl.Attributes = Color;

	//for (WORD row = 0; row <= SizeY; row++)
	//{
	//	for (WORD col = 0; col < SizeX; col++)
	//	{
	//		videobufferESDI[DI] = AhAl; // do videopamìti na indexu z DI uložíme znak
	//	}
	//	DI += TxtCols;
	//	DI += TxtCols;
	//}

	//AhAl.Attributes = 9;

	//do { AhAl.Char.AsciiChar = videobufferESDI[DX].Char.AsciiChar; }
	//while ((videobufferESDI[DX].Char.AsciiChar & 0x0001) == 1);

	DWORD written = 0;
	CHAR_INFO* _buf = new CHAR_INFO[SizeX * SizeY];
	COORD BufferSize = { (short)SizeX, (short)SizeY };
	SMALL_RECT rect = { X, Y, X + SizeX, Y + SizeY };

	CHAR_INFO ci; ci.Char.AsciiChar = C; ci.Attributes = Color;
	for (int i = 0; i < SizeX * SizeY; i++) { _buf[i] = ci; }
	WriteConsoleOutput(_handle, _buf, BufferSize, { 0, 0 }, &rect);

	//SetConsoleCursorPosition(hConsOutput, leftTop);
	//WriteConsoleOutput(hConsOutput, _buf, rightBottom, leftTop, &hWin);
	//FillConsoleOutputAttribute(hConsOutput, Color, SizeX * SizeY, coordScreen, &written);
	//FillConsoleOutputCharacter(hConsOutput, C, SizeX * SizeY, coordScreen, &written);

	delete[] _buf;
}

void Screen::ScrWrChar(WORD X, WORD Y, char C, BYTE Color)
{
	DWORD written = 0;
	WORD attr = Color;
	WriteConsoleOutputCharacterA(_handle, &C, 1, { (short)X, (short)Y }, &written);
	WriteConsoleOutputAttribute(_handle, &attr, 1, { (short)X, (short)Y }, &written);
}


void Screen::ScrWrStr(WORD X, WORD Y, std::string S, BYTE Color)
{
	short len = S.length();
	CHAR_INFO* _buf = new CHAR_INFO[len];
	COORD BufferSize = { len, 1 };
	SMALL_RECT rect = { (short)X, (short)Y, (short)X + len, (short)Y };

	CHAR_INFO ci;
	ci.Attributes = Color;
	for (int i = 0; i < len; i++)
	{
		ci.Char.AsciiChar = S[i];
		_buf[i] = ci;
	}
	WriteConsoleOutputA(_handle, _buf, BufferSize, { 0, 0 }, &rect);
}

void Screen::ScrWrFrameLn(WORD X, WORD Y, BYTE Typ, BYTE Width, BYTE Color)
{
	pstring txt;
	txt[0] = Width;
	txt[1] = FrameChars[Typ];
	txt[Width] = FrameChars[Typ + 2];
	for (int i = 2; i <= Width - 1; i++) { txt[i] = FrameChars[Typ + 1]; }
	ScrWrStr(X, Y, txt, Color);
}

void Screen::ScrWrText(WORD X, WORD Y, const char* S)
{
	X += WindMin->X - 1;
	Y += WindMin->Y - 1;
	DWORD written = 0;
	size_t len = strlen(S);
	WriteConsoleOutputCharacterA(_handle, S, len, { (short)X, (short)Y }, &written);
}

void Screen::ScrWrBuf(WORD X, WORD Y, void* Buf, WORD L)
{
	X++; // v Pacalu to bylo od 1
	Y++; // --""--
	SMALL_RECT XY = { (short)X, (short)Y, (short)X + L, (short)Y + 1 };
	COORD BufferSize = { (short)L, 1 };
	WriteConsoleOutputA(_handle, (CHAR_INFO*)Buf, BufferSize, { 0, 0 }, &XY);
}

void Screen::ScrRdBuf(WORD X, WORD Y, void* Buf, WORD L)
{
}

void Screen::ScrMove(WORD X, WORD Y, WORD ToX, WORD ToY, WORD L)
{
}

void Screen::ScrColor(WORD X, WORD Y, WORD L, BYTE Color)
{
	DWORD written = 0;
	FillConsoleOutputAttribute(_handle, Color, L, { (short)X, (short)Y }, &written);
}

TCrs Screen::CrsGet()
{
	TCrs crs{ Crs->X, Crs->Y, Crs->Big, Crs->Enabled, 0 };
	return crs;
}

void Screen::CrsSet(TCrs S)
{
	CrsHide();
	Crs->X = S.X;
	Crs->Y = S.Y;
	Crs->Big = S.Big;
	Crs->Enabled = S.Enabled;
	CrsGotoXY(Crs->X, Crs->Y);
	if (Crs->Enabled) CrsShow();
}

void Screen::CrsShow()
{
	CONSOLE_CURSOR_INFO visible{ 1, true };
	SetConsoleCursorInfo(_handle, &visible);
	Crs->Enabled = true;
}

void Screen::CrsHide()
{
	CONSOLE_CURSOR_INFO invisible{ 1, false };
	SetConsoleCursorInfo(_handle, &invisible);
	Crs->Enabled = false;
}

void Screen::CrsBig()
{
	if (!Crs->Big) { CrsHide(); Crs->Big = true; } CrsShow();
}

void Screen::CrsNorm()
{
	if (Crs->Big) { CrsHide(); Crs->Big = false; } CrsShow();
}

void Screen::GotoXY(WORD X, WORD Y)
{
	if (X > WindMax->X || Y > WindMax->Y) return;
	X += WindMin->X;
	Y += WindMin->Y;
	SetConsoleCursorPosition(_handle, { (short)X, (short)Y });
}

BYTE Screen::WhereX()
{
	// vrací relativní pozici (k aktuálnímu oknu)
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	GetConsoleScreenBufferInfo(_handle, &sbi);
	return (BYTE)sbi.dwCursorPosition.X - WindMin->X;
}

BYTE Screen::WhereY()
{
	// vrací relativní pozici (k aktuálnímu oknu)
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	GetConsoleScreenBufferInfo(_handle, &sbi);
	return (BYTE)sbi.dwCursorPosition.Y - WindMin->Y;
}

void Screen::Window(BYTE X1, BYTE Y1, BYTE X2, BYTE Y2)
{
	// pùvodní kód z ASM
	if (X2 <= X1) return;
	if (Y2 <= Y1) return;
	if (X1 == 0) return;
	X1--;
	if (Y1 == 0) return;
	Y1--;
	if (X2 > *TxtCols) return;
	X2--;
	if (Y2 > *TxtRows) return;
	Y2--;
	WindMin->X = X1;
	WindMin->Y = Y1;
	WindMax->X = X2;
	WindMax->Y = Y2;
	GotoXY(X1, Y1);
}

void Screen::ScrWr()
{
}

void Screen::CrsDark()
{
}

void Screen::CrsBlink()
{
}

void Screen::CrsGotoXY(WORD aX, WORD aY)
{
	bool b;
	Crs->X = aX;
	Crs->Y = aY;
	SetConsoleCursorPosition(_handle, { (short)Crs->X, (short)Crs->Y });
}

int Screen::ScrPush1(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P)
{
	SMALL_RECT rect{ (short)X, (short)Y, (short)X + (short)SizeX, (short)Y + (short)SizeY };
	// do ukazatele zøejmì uloží obsah videopamìti ...
	ReadConsoleOutput(_handle, (CHAR_INFO*)P, { (short)SizeX, (short)SizeY }, { 0, 0 }, &rect);
	return SizeX * SizeY;
}

void Screen::ScrGetPtr(WORD X, WORD Y, WORD& DX, WORD& DI)
{
	// v AX je Y, v DI je X
	DI = X;
	int DXAX = Y * *TxtCols;
	WORD AX = DXAX & 0x0000FFFF; // dolní WORD z int. DXAX
	DX = DXAX >> 16; // horní WORD z int. DXAX
	AX = AX < 1;
	DI = DI < 2; // (v reg. DI je X)
	DI += AX;
	// v ES i AX se vrací video adresa B800H - ignorujeme
	// dále se vrací hodnoty DX a DI(tady X)
}

void Screen::pushScreen(storeWindow sw)
{
	_windowStack.push(sw);
}

storeWindow Screen::popScreen()
{
	auto result = _windowStack.top();
	_windowStack.pop();
	return result;
}

void Screen::SaveScreen(WParam* wp, short c1, short r1, short c2, short r2)
{
	c1--; c2--;
	r1--; r2--;
	
	SMALL_RECT rect{ c1, r1, c2, r2 };
	COORD bufSize{ (short)(c2 - c1 + 1), (short)r2 - r1 + 1 };
	CHAR_INFO* buf = new CHAR_INFO[bufSize.X * bufSize.Y];
	ReadConsoleOutput(_handle, buf, bufSize, { 0, 0 }, &rect);
	_windowStack.push({ wp, bufSize, rect, buf });
}

void Screen::LoadScreen(bool draw, WParam* wp)
{
	auto scr = _windowStack.top();
	_windowStack.pop();
	wp = scr.wp;
	if (draw) {
		WriteConsoleOutput(_handle, scr.content, scr.coord, { 0, 0 }, &scr.rect);
	}
	delete[] scr.content;
}
