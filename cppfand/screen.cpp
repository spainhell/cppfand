#include "screen.h"
#include <exception>
#include "pstring.h"
#include <stdarg.h>
#include <vector>
#include "../textfunc/textfunc.h"

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
	SetConsoleTitle("C++ FAND");
	//DWORD consoleMode = ENABLE_VIRTUAL_TERMINAL_PROCESSING; // | ENABLE_LVB_GRID_WORLDWIDE;
	//bool scm = SetConsoleMode(_handle, 0);
	_actualIndex = 0;
	_inBuffer = 0;
}

Screen::~Screen()
{
	//delete[] _scrBuf;
}

size_t Screen::BufSize()
{
	return BUFFSIZE;
}

void Screen::ScrClr(WORD X, WORD Y, WORD SizeX, WORD SizeY, char C, BYTE Color)
{
	// cislovani radku a sloupcu prichazi od 1 .. X
	if (X < 1 || Y < 1) { throw std::exception("Bad ScrClr index."); }

	DWORD written = 0;
	CHAR_INFO* _buf = new CHAR_INFO[SizeX * SizeY];
	COORD BufferSize = { (short)SizeX, (short)SizeY };
	SMALL_RECT rect = { X - 1, Y - 1, X + SizeX, Y + SizeY };

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
	SMALL_RECT rect = { (short)X - 1, (short)Y - 1, (short)X + len - 1, (short)Y - 1 };

	CHAR_INFO ci;
	ci.Attributes = Color;
	for (int i = 0; i < len; i++)
	{
		ci.Char.AsciiChar = S[i];
		_buf[i] = ci;
	}
	WriteConsoleOutputA(_handle, _buf, BufferSize, { 0, 0 }, &rect);
	delete[] _buf;
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
	WriteConsoleOutputCharacterA(_handle, S, len, { (short)X - 1, (short)Y - 1 }, &written);
}

void Screen::ScrFormatWrText(WORD X, WORD Y, char const* const _Format, ...)
{
	va_list args;
	va_start(args, _Format);
	char buffer[255];
	vsnprintf(buffer, sizeof(buffer), _Format, args);
	Screen::ScrWrText(X, Y, buffer);
	va_end(args);
}

void Screen::ScrFormatWrStyledText(WORD X, WORD Y, BYTE Color, char const* const _Format, ...)
{
	va_list args;
	va_start(args, _Format);
	char buffer[255];
	vsnprintf(buffer, sizeof(buffer), _Format, args);
	for (size_t i = 0; i < strlen(buffer); i++) {
		char c = buffer[i];
		WriteChar(X++, Y, c, Color, relative);
	}
	va_end(args);
}

// vypise pole WORDu (Attr + Znak)
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

// vypise pole CHAR_INFO
void Screen::ScrWrCharInfoBuf(short X, short Y, CHAR_INFO* Buf, short L)
{
	//X++; // v Pacalu to bylo od 1
	//Y++; // --""--
	SMALL_RECT XY = { X - 1, Y - 1, (short)(X + L), (short)(Y + 1) };
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

// vypise na zadanou pozici 1 znak v zadane barve
void Screen::WriteChar(short X, short Y, char C, BYTE attr, Position pos)
{
	DWORD written = 0;
	switch (pos) {
	case relative: {
		X += WindMin->X - 1;
		Y += WindMin->Y - 1;
		break;
	}
	case absolute: {
		break;
	}
	case actual: {
		X = WhereXabs();
		Y = WhereYabs();
		break;
	}
	default:;
	}
	WORD color = attr;
	auto resatr = WriteConsoleOutputAttribute(_handle, &color, 1, { X - 1, Y - 1 }, &written);
	auto result = WriteConsoleOutputCharacterA(_handle, &C, 1, { X - 1, Y - 1}, &written);
	GotoXY(WhereXabs() + 1, WhereYabs(), absolute); // po zapisu poseneme kurzor
}

// vypise stylizovany text do aktualniho okna
void Screen::WriteStyledStringToWindow(std::string text, BYTE Attr)
{
	if (text.length() == 0) return;

	std::string CStyle;
	std::string CColor;
	CColor = (char)Attr;

	// ziskame jendotlive radky textu
	auto vStr = GetAllRows(text);

	CHAR_INFO ci;

	short cols = 200;
	short rows = WindMax->Y - WindMin->Y + 1;

	// buffer bude mit delku jednoho radku okna
	CHAR_INFO* _buf = new CHAR_INFO[cols];

	// pocet radku je mensi hodnota z poctu textu nebo radku okna
	for (size_t i = 0; i < min(rows, vStr.size()); i++)
	{
		auto str = vStr[i];
		auto strLen = str.length();
		// okenko bude mit jen 1 radek
		SMALL_RECT rect = { 
			WindMin->X + actualWindowCol - 2,			// oba parametry jsou cislovane od 1
			WindMin->Y + actualWindowRow - 2,			// oba parametry jsou cislovane od 1
			WindMax->X - 1,								// prava strana zustava stejna
			WindMin->Y + actualWindowRow - 2,			// dolni strana stejna jako horni (jen 1 radek)
		};

		size_t ctrlCharsCount = 0;

		for (size_t i = 0; i < strLen; i++)
		{
			char c = str[i];
			BYTE a = 0;
			if (SetStyleAttr(c, a))
			{
				ctrlCharsCount++;
				size_t i = CStyle.find_first_of(c);
				if (i != std::string::npos)
				{
					CStyle.erase(i, 1);
					CColor.erase(i, 1);
				}
				else {
					CStyle = c + CStyle;
					CColor = (char)a + CColor;
				}
				Attr = CColor[0];
				continue;
			}
			if (c == '\n' || c == '\r') {
				ctrlCharsCount++;
				continue;
			}
			ci.Attributes = Attr;
			ci.Char.AsciiChar = c;
			size_t position = i - ctrlCharsCount;
			if (position > cols - 1) {
				// retezec se do radku nevleze, ale budeme pokracovat kvuli nastaveni barev
				continue;
			}
			_buf[position] = ci;
		}
		COORD BufferSize = { strLen - ctrlCharsCount, 1 }; // pocet tisknutelnych znaku, 1 radek
		WriteConsoleOutputA(_handle, _buf, BufferSize, { 0, 0 }, &rect);
		// nastavime zacatek dalsiho radku
		actualWindowRow++;
		actualWindowCol = 1;
	}
	delete[] _buf;
}

bool Screen::SetStyleAttr(char C, BYTE& a)
{
	auto result = true;
	if (C == 0x13) a = colors.tUnderline;
	else if (C == 0x17) a = colors.tItalic;
	else if (C == 0x11) a = colors.tDWidth;
	else if (C == 0x04) a = colors.tDStrike;
	else if (C == 0x02) a = colors.tEmphasized;
	else if (C == 0x05) a = colors.tCompressed;
	else if (C == 0x01) a = colors.tElite;
	else result = false;
	return result;
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

void Screen::GotoXY(WORD X, WORD Y, Position pos)
{
	// if (X > WindMax->X || Y > WindMax->Y) return;
	switch (pos)
	{
	case relative: {
		X = X + WindMin->X - 1;
		Y = Y + WindMin->Y - 1;
		break;
	}
	case absolute: break;
	case actual: return;
	default: return;
	}
	bool succ = SetConsoleCursorPosition(_handle, { (short)X - 1, (short)Y - 1 });
	if (!succ) {
		printf("GotoXY() fail");
	}
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

short Screen::WhereXabs()
{
	// vrací absolutní pozici èíslovanou od 1
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	GetConsoleScreenBufferInfo(_handle, &sbi);
	return sbi.dwCursorPosition.X + 1;
}

short Screen::WhereYabs()
{
	// vrací absolutní pozici èíslovanou od 1
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	GetConsoleScreenBufferInfo(_handle, &sbi);
	return sbi.dwCursorPosition.Y + 1;
}

void Screen::Window(BYTE X1, BYTE Y1, BYTE X2, BYTE Y2)
{
	// pùvodní kód z ASM
	if (X2 < X1) return;
	if (Y2 < Y1) return;
	if (X2 > * TxtCols) return;
	if (Y2 > * TxtRows) return;
	WindMin->X = X1;
	WindMin->Y = Y1;
	WindMax->X = X2;
	WindMax->Y = Y2;
	actualWindowRow = 1;
	actualWindowCol = 1;
	GotoXY(1, 1, relative);
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
	Crs->X = aX;
	Crs->Y = aY;
	bool succ = SetConsoleCursorPosition(_handle, { (short)Crs->X, (short)Crs->Y });
	if (!succ) {
		printf("GotoXY() fail");
	}
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

int Screen::SaveScreen(WParam* wp, short c1, short r1, short c2, short r2)
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
	return _windowStack.size();
}

void Screen::LoadScreen(bool draw, WParam* wp)
{
	if (_windowStack.size() == 0) {
			printf("Screen::LoadScreen() zasobnik je prazdny!!!\n");
		return;
	}
	auto scr = _windowStack.top();
	_windowStack.pop();
	wp = scr.wp;
	if (draw) {
		WriteConsoleOutput(_handle, scr.content, scr.coord, { 0, 0 }, &scr.rect);
	}
	delete[] scr.content;
}
