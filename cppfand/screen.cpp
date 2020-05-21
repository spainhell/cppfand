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
	// cislovani radku a sloupcu prichazi od 1 .. X
	if (X < 0 || Y < 0) { throw std::exception("Bad ScrClr index."); }
	//X--; Y--;

	DWORD written = 0;
	CHAR_INFO* _buf = new CHAR_INFO[SizeX * SizeY];
	COORD BufferSize = { (short)SizeX, (short)SizeY };
	SMALL_RECT rect = { X, Y, X + SizeX, Y + SizeY };

	CHAR_INFO ci; ci.Char.AsciiChar = C; ci.Attributes = Color;
	for (int i = 0; i < SizeX * SizeY; i++) { _buf[i] = ci; }
	WriteConsoleOutput(_handle, _buf, BufferSize, { 0, 0 }, &rect);

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
	//X++; // v Pacalu to bylo od 1
	//Y++; // --""--
	SMALL_RECT XY = { (short)X, (short)Y, (short)X + L, (short)Y + 1 };
	COORD BufferSize = { (short)L, 1 };

	// zkonvertujeme WORD do CHAR_INFO
	WORD* pBuf = (WORD*)Buf;
	CHAR_INFO* ci = new CHAR_INFO[L];
	for (int i = 0; i < L; i++)
	{
		ci[i].Attributes = pBuf[i] >> 8;
		ci[i].Char.AsciiChar = pBuf[i] & 0x00FF;
	}
	
	WriteConsoleOutputA(_handle, ci, BufferSize, { 0, 0 }, &XY);
	delete[] ci;
}

void Screen::ScrWrCharInfoBuf(short X, short Y, CHAR_INFO* Buf, short L)
{
	//X++; // v Pacalu to bylo od 1
	//Y++; // --""--
	SMALL_RECT XY = { X, Y, (short)(X + L), (short)(Y + 1) };
	COORD BufferSize = { (short)L, 1 };
	WriteConsoleOutputA(_handle, Buf, BufferSize, { 0, 0 }, &XY);
}

bool Screen::ScrRdBuf(WORD X, WORD Y, CHAR_INFO* Buf, WORD L)
{
	SMALL_RECT rect{ (short)X, (short)Y, (short)X + L - 1, (short)Y };
	COORD bufSize{ (short)(L), 1 };
	//CHAR_INFO* buf = new CHAR_INFO[bufSize.X * bufSize.Y];
	bool result = ReadConsoleOutput(_handle, Buf, bufSize, { 0, 0 }, &rect);
	return result;
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
	TCrs crs;
	crs.X = Crs->X;
	crs.Y = Crs->Y;
	crs.Big = Crs->Big;
	crs.Enabled = Crs->Enabled;
	crs.Ticks = 0;
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
	X += WindMin->X - 1;
	Y += WindMin->Y - 1;
	SetConsoleCursorPosition(_handle, { (short)X, (short)Y });
}

BYTE Screen::WhereX()
{
	// vrací relativní pozici (k aktuálnímu oknu) èíslovanou od 1
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	GetConsoleScreenBufferInfo(_handle, &sbi);
	return (BYTE)sbi.dwCursorPosition.X - WindMin->X + 1;
}

BYTE Screen::WhereY()
{
	// vrací relativní pozici (k aktuálnímu oknu) èíslovanou od 1
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	GetConsoleScreenBufferInfo(_handle, &sbi);
	return (BYTE)sbi.dwCursorPosition.Y - WindMin->Y + 1;
}

void Screen::Window(BYTE X1, BYTE Y1, BYTE X2, BYTE Y2)
{
	// pùvodní kód z ASM
	if (X2 <= X1) return;
	if (Y2 <= Y1) return;
	if (X2 + 1 > *TxtCols) return;
	if (Y2 + 1 > *TxtRows) return;
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
	// cislovani radku a sloupcu prichazi od 1 .. X
	if (c1 < 1 || c2 > 80 || r1 < 1 || r2 > 25) { throw std::exception("Bad SaveScreen index."); }

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
