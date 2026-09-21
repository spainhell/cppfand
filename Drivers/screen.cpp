#include "screen.h"
#include "host.h"
#include <exception>
#include <stdarg.h>
#include <vector>
#include "../Common/textfunc.h"

const unsigned int BUFFSIZE = 128 * 1024;

Screen::Screen(short TxtCols, short TxtRows, Wind* WindMin, Wind* WindMax, TCrs* Crs)
{
	this->TxtCols = TxtCols;
	this->TxtRows = TxtRows;
	this->MaxColsIndex = (short)(TxtCols - 1);
	this->MaxRowsIndex = (short)(TxtRows - 1);
	this->WindMin = WindMin;
	this->WindMax = WindMax;
	this->Crs = Crs;

	WindMax->X = (uint8_t)MaxColsIndex;
	WindMax->Y = (uint8_t)MaxRowsIndex;

	resizeCells();
}

Screen::~Screen()
{
}

void Screen::resizeCells()
{
	CHAR_INFO blank;
	blank.Char.AsciiChar = ' ';
	blank.Attributes = 7;
	_cells.assign((size_t)TxtCols * TxtRows, blank);
	_crsX = 1;
	_crsY = 1;
	_version++;
}

void Screen::InitConsole()
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	if (FandHost::IsEnabled()) {
		_console = false;
		return;
	}
	_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (_handle == INVALID_HANDLE_VALUE || _handle == nullptr) {
		throw std::exception("Cannot open console output handle.");
	}
	_console = true;

	SMALL_RECT rect{ 0, 0, (short)(TxtCols - 1), (short)(TxtRows - 1) };
	SetConsoleScreenBufferSize(_handle, { TxtCols, TxtRows });
	SetConsoleWindowInfo(_handle, true, &rect);

	SetConsoleCP(852);
	SetConsoleOutputCP(852);
	SetConsoleTitle("C++ FAND");

	// avoid console window size changes
	HWND consoleWindow = GetConsoleWindow();
	if (consoleWindow != nullptr) {
		SetWindowLong(consoleWindow, GWL_STYLE, GetWindowLong(consoleWindow, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);
	}

	// pocatecni stav promitneme cely
	flush(0, 0, TxtCols, TxtRows);
	applyCursorPos();
	applyCursorInfo();
}

void Screen::ReInit(short TxtCols, short TxtRows)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	if (this->TxtCols != TxtCols || this->TxtRows != TxtRows) {
		// cfg changed -> reinitialize
		this->TxtCols = TxtCols;
		this->TxtRows = TxtRows;
		this->MaxColsIndex = (short)(TxtCols - 1);
		this->MaxRowsIndex = (short)(TxtRows - 1);

		this->WindMax->X = (uint8_t)this->MaxColsIndex;
		this->WindMax->Y = (uint8_t)this->MaxRowsIndex;

		resizeCells();

		if (_console) {
			SMALL_RECT rect{ 0, 0, (short)(TxtCols - 1), (short)(TxtRows - 1) };
			SetConsoleScreenBufferSize(_handle, { TxtCols, TxtRows });
			SetConsoleWindowInfo(_handle, true, &rect);
			flush(0, 0, TxtCols, TxtRows);
		}
	}
}

size_t Screen::BufSize()
{
	return BUFFSIZE;
}

// ---------------------------------------------------------------------------
// interni pomocne metody
// ---------------------------------------------------------------------------

void Screen::flush(int x0, int y0, int w, int h)
{
	_version++;
	if (!_console) return;
	// oriznuti na obrazovku
	if (x0 < 0) { w += x0; x0 = 0; }
	if (y0 < 0) { h += y0; y0 = 0; }
	if (x0 + w > TxtCols) w = TxtCols - x0;
	if (y0 + h > TxtRows) h = TxtRows - y0;
	if (w <= 0 || h <= 0) return;

	std::vector<CHAR_INFO> buf((size_t)w * h);
	for (int r = 0; r < h; r++) {
		memcpy(&buf[(size_t)r * w], &cell(x0, y0 + r), sizeof(CHAR_INFO) * w);
	}
	SMALL_RECT rect{ (short)x0, (short)y0, (short)(x0 + w - 1), (short)(y0 + h - 1) };
	WriteConsoleOutputA(_handle, buf.data(), { (short)w, (short)h }, { 0, 0 }, &rect);
}

void Screen::applyCursorPos()
{
	_version++;
	if (!_console) return;
	bool succ = SetConsoleCursorPosition(_handle, { (short)(_crsX - 1), (short)(_crsY - 1) });
	if (Crs->Enabled && !succ) {
		printf("GotoXY() fail");
	}
}

void Screen::applyCursorInfo()
{
	_version++;
	if (!_console) return;
	CONSOLE_CURSOR_INFO info{ Crs->Enabled ? Crs->Size : 1, Crs->Enabled };
	SetConsoleCursorInfo(_handle, &info);
}

uint64_t Screen::Version()
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	return _version;
}

uint64_t Screen::Snapshot(uint16_t* cells, size_t capacity, int& crsX, int& crsY, bool& crsVisible, int& crsSize)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	size_t n = min(capacity, _cells.size());
	for (size_t i = 0; i < n; i++) {
		cells[i] = (uint16_t)((uint8_t)_cells[i].Char.AsciiChar | ((_cells[i].Attributes & 0xFF) << 8));
	}
	crsX = _crsX - 1;
	crsY = _crsY - 1;
	crsVisible = Crs->Enabled;
	crsSize = (int)Crs->Size;
	return _version;
}

uint8_t Screen::AttrAt(WORD X, WORD Y)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	if (!inside(X - 1, Y - 1)) return 7;
	return (uint8_t)cell(X - 1, Y - 1).Attributes;
}

// ---------------------------------------------------------------------------
// vypis
// ---------------------------------------------------------------------------

void Screen::ScrClr(WORD X, WORD Y, WORD SizeX, WORD SizeY, char C, uint8_t Color)
{
	// cislovani radku a sloupcu prichazi od 1 .. X
	if (X < 1 || Y < 1) { throw std::exception("Bad ScrClr index."); }
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	CHAR_INFO ci; ci.Char.AsciiChar = C; ci.Attributes = Color;
	for (int r = 0; r < SizeY; r++) {
		for (int c = 0; c < SizeX; c++) {
			if (inside(X - 1 + c, Y - 1 + r)) cell(X - 1 + c, Y - 1 + r) = ci;
		}
	}
	flush(X - 1, Y - 1, SizeX, SizeY);
}

void Screen::ScrWrChar(WORD X, WORD Y, char C, uint8_t Color)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	if (!inside(X - 1, Y - 1)) return;
	CHAR_INFO ci; ci.Char.AsciiChar = C; ci.Attributes = Color;
	cell(X - 1, Y - 1) = ci;
	flush(X - 1, Y - 1, 1, 1);
}

void Screen::ScrWrStr(const std::string& s, uint8_t Color)
{
	ScrWrStr(WhereXabs(), WhereYabs(), s, Color);
}

void Screen::ScrWrStr(WORD X, WORD Y, const std::string& s, uint8_t Color)
{
	// TODO: doresit zobrazeni znaku jako '\r' nebo '\n'
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	short len = (short)s.length();
	int written = 0;
	for (int i = 0; i < len; i++) {
		if (!inside(X - 1 + i, Y - 1)) break;
		CHAR_INFO& ci = cell(X - 1 + i, Y - 1);
		ci.Char.AsciiChar = s[i];
		ci.Attributes = Color;
		written++;
	}
	flush(X - 1, Y - 1, written, 1);
}

void Screen::ScrWrFrameLn(WORD X, WORD Y, uint8_t Typ, uint8_t Width, uint8_t Color)
{
	std::string txt;
	txt.reserve(Width);
	txt += (char)FrameChars[Typ];
	for (int i = 0; i < Width - 2; i++)	{
		txt += (char)FrameChars[Typ + 1];
	}
	txt += (char)FrameChars[Typ + 2];
	ScrWrStr(X, Y, txt, Color);
}

void Screen::ScrWrText(WORD X, WORD Y, const char* S)
{
	// zapise jen znaky, barvy (atributy) na radku zustanou zachovane
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	X += WindMin->X - 1;
	Y += WindMin->Y - 1;

	// budeme predpokladat, ze se muze zobrazit jen 1 radek
	// jeho delka bude dle nastaveneho okna:
	size_t len = min((size_t)(WindMax->X - WindMin->X + 1), strlen(S));

	int written = 0;
	for (size_t i = 0; i < len; i++) {
		if (!inside((int)(X - 1 + i), Y - 1)) break;
		cell((int)(X - 1 + i), Y - 1).Char.AsciiChar = S[i];
		written++;
	}
	flush(X - 1, Y - 1, written, 1);

	if (X + len > (size_t)TxtCols) {
		GotoXY(1, Y + 1, absolute);
	}
	else {
		GotoXY((WORD)(X + len), Y, absolute);
	}
}

void Screen::ScrFormatWrText(WORD X, WORD Y, char const* const _Format, ...)
{
	va_list args;
	va_start(args, _Format);
	char buffer[255]{ 0 };
	vsnprintf(buffer, sizeof(buffer), _Format, args);
	Screen::ScrWrText(X, Y, buffer);
	va_end(args);
}

void Screen::ScrFormatWrStyledText(WORD X, WORD Y, uint8_t Color, char const* const _Format, ...)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	// souradnice jsou relativni, tiskneme do aktualniho okna
	X += WindMin->X - 1;
	Y += WindMin->Y - 1;

	va_list args;
	va_start(args, _Format);
	char buffer[255];
	vsnprintf(buffer, sizeof(buffer), _Format, args);
	va_end(args);
	size_t len = strlen(buffer);

	int written = 0;
	for (size_t i = 0; i < len; i++) {
		if (!inside((int)(X - 1 + i), Y - 1)) break;
		CHAR_INFO& ci = cell((int)(X - 1 + i), Y - 1);
		ci.Attributes = Color;
		ci.Char.AsciiChar = buffer[i];
		written++;
	}
	flush(X - 1, Y - 1, written, 1);
	// posuneme souradnici X o vytistene znaky
	GotoXY(WhereXabs() + (WORD)len, WhereYabs(), absolute);
}

// vypise pole WORDu (Attr + Znak); souradnice jsou 0-based
void Screen::ScrWrBuf(WORD X, WORD Y, void* Buf, WORD L)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	WORD* pBuf = (WORD*)Buf;
	int written = 0;
	for (int i = 0; i < L; i++) {
		if (!inside(X + i, Y)) break;
		CHAR_INFO& ci = cell(X + i, Y);
		ci.Attributes = pBuf[i] >> 8;
		ci.Char.AsciiChar = pBuf[i] & 0x00FF;
		written++;
	}
	flush(X, Y, written, 1);
}

// vypise pole CHAR_INFO; souradnice jsou 1-based
void Screen::ScrWrCharInfoBuf(short X, short Y, CHAR_INFO* Buf, short L)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	int written = 0;
	for (int i = 0; i < L; i++) {
		if (!inside(X - 1 + i, Y - 1)) break;
		cell(X - 1 + i, Y - 1) = Buf[i];
		written++;
	}
	flush(X - 1, Y - 1, written, 1);
}

// precte L bunek radku; souradnice jsou 1-based
bool Screen::ScrRdBuf(WORD X, WORD Y, CHAR_INFO* Buf, WORD L)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	if (!inside(X - 1, Y - 1)) return false;
	for (int i = 0; i < L; i++) {
		if (!inside(X - 1 + i, Y - 1)) return false;
		Buf[i] = cell(X - 1 + i, Y - 1);
	}
	return true;
}

void Screen::ScrMove(short X, short Y, short ToX, short ToY, short L)
{
	// souradnice chodi kupodivu od 0 ..
	if (L < 1) return;
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	// ulozime obsah obrazovky a "pretiskneme" na jine misto
	CrsHide();
	if ((X < 0) || (X > MaxColsIndex) || (Y < 0) || (Y > MaxRowsIndex))
		throw std::exception("Bad ScrMove index.");
	if ((ToX < 0) || (ToX > MaxColsIndex) || (ToY < 0) || (ToY > MaxRowsIndex))
		throw std::exception("Bad ScrMove index.");
	std::vector<CHAR_INFO> buf(L);
	int n = 0;
	for (int i = 0; i < L && inside(X + i, Y); i++) { buf[i] = cell(X + i, Y); n = i + 1; }
	int written = 0;
	for (int i = 0; i < n && inside(ToX + i, ToY); i++) { cell(ToX + i, ToY) = buf[i]; written++; }
	flush(ToX, ToY, written, 1);
	CrsShow();
}

// obarvi L bunek od pozice (0-based), linearne pres konce radku (jako FillConsoleOutputAttribute)
void Screen::ScrColor(WORD X, WORD Y, WORD L, uint8_t Color)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	size_t start = (size_t)Y * TxtCols + X;
	for (size_t i = 0; i < L && start + i < _cells.size(); i++) {
		_cells[start + i].Attributes = Color;
	}
	int rows = (int)((X + L + TxtCols - 1) / TxtCols);
	flush(0, Y, TxtCols, rows);
}

// vypise na zadanou pozici 1 znak v zadane barve
void Screen::WriteChar(short X, short Y, char C, uint8_t attr, ScrPosition pos)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
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
	if (inside(X - 1, Y - 1)) {
		CHAR_INFO& ci = cell(X - 1, Y - 1);
		ci.Attributes = attr;
		ci.Char.AsciiChar = C;
		flush(X - 1, Y - 1, 1, 1);
	}
	GotoXY(WhereXabs() + 1, WhereYabs(), absolute); // po zapisu poseneme kurzor
}

// vypise stylizovany text do aktualniho okna a vrati pocet vypsanych znaku
size_t Screen::WriteStyledStringToWindow(const std::string& text, uint8_t Attr)
{
	if (text.length() == 0) return 0;
	std::lock_guard<std::recursive_mutex> lock(_mutex);

	std::string CStyle;
	std::string CColor;
	CColor = (char)Attr;

	// celkovy pocet vytistenych znaku
	size_t totalChars = 0;

	CHAR_INFO ci;

	short cols = WindMax->X - WindMin->X + 1;
	short rows = WindMax->Y - WindMin->Y + 1;

	// ziskame jednotlive radky textu
	auto vStr = GetAllLines(text, cols);

	// buffer bude mit delku jednoho radku okna
	std::vector<CHAR_INFO> _buf(cols);

	// prevezmeme aktualni pozici kurzoru:
	actualWindowCol = WhereX();
	actualWindowRow = WhereY();

	// pocet radku je mensi hodnota z poctu textu nebo radku okna
	short rowsToPrint = (short)min((size_t)rows, vStr.size());
	for (size_t i = 0; i < (size_t)rowsToPrint; i++)
	{
		auto str = vStr[i];
		auto strLen = str.length();
		// oblast: od aktualniho sloupce okna po pravy okraj okna, 1 radek (0-based)
		int col0 = WindMin->X + actualWindowCol - 2;
		int row0 = WindMin->Y + actualWindowRow - 2;
		int maxRight = WindMax->X - 1;

		size_t ctrlCharsCount = 0;

		for (size_t j = 0; j < strLen; j++)
		{
			char c = str[j];
			uint8_t a = 0;
			if (SetStyleAttr(c, a))
			{
				ctrlCharsCount++;
				size_t k = CStyle.find_first_of(c);
				if (k != std::string::npos)
				{
					CStyle.erase(k, 1);
					CColor.erase(k, 1);
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
			size_t position = j - ctrlCharsCount;
			if (position > (size_t)(cols - 1)) {
				// retezec se do radku nevleze, ale budeme pokracovat kvuli nastaveni barev
				continue;
			}
			_buf[position] = ci;
		}
		short printable = (short)(strLen - ctrlCharsCount); // pocet tisknutelnych znaku
		int written = 0;
		for (int k = 0; k < printable && col0 + k <= maxRight; k++) {
			if (!inside(col0 + k, row0)) break;
			cell(col0 + k, row0) = _buf[k];
			written++;
		}
		flush(col0, row0, written, 1);
		totalChars += printable;
		// nastavime zacatek dalsiho radku, pokud se nejedna o posledni radek
		if (i < (size_t)(rowsToPrint - 1)) {
			actualWindowRow++;
			actualWindowCol = 1;
		}
		// pokud se jedna o posledni radek, nastavime korektne RELATIVNI souradnice
		else {
			// pokud jsme na konci radku, prejdeme na zacatek
			if (printable + 1 > WindMax->X) {
				GotoXY(1, actualWindowRow);
			}
			else {
				GotoXY(printable + 1, actualWindowRow);
			}
		}
	}
	return totalChars;
}

void Screen::LF()
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	if (WindMax->Y - WindMin->Y + 1 == actualWindowRow) {
		// cursor is on a last row in the window -> move everything up
		short cols = WindMax->X - WindMin->X + 1;
		short rows = WindMax->Y - WindMin->Y + 1;
		int x0 = WindMin->X - 1;
		int y0 = WindMin->Y - 1;
		for (int r = 0; r < rows - 1; r++) {
			for (int c = 0; c < cols; c++) {
				if (inside(x0 + c, y0 + r) && inside(x0 + c, y0 + r + 1)) {
					cell(x0 + c, y0 + r) = cell(x0 + c, y0 + r + 1);
				}
			}
		}
		flush(x0, y0, cols, rows - 1);
		std::string spaces(cols, ' ');
		ScrWrText(1, actualWindowRow, spaces.c_str());
	}
	else {
		actualWindowRow++;
	}
	actualWindowCol = 1;
	GotoXY(actualWindowCol, actualWindowRow);
}

bool Screen::SetStyleAttr(char C, uint8_t& a)
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

// ---------------------------------------------------------------------------
// kurzor
// ---------------------------------------------------------------------------

TCrs Screen::CrsGet()
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	TCrs crs;
	crs.X = WhereXabs();
	crs.Y = WhereYabs();
	crs.Size = Crs->Size;
	crs.Enabled = Crs->Enabled;
	crs.Ticks = 0;
	return crs;
}

void Screen::CrsSet(TCrs S)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	CrsHide();
	Crs->X = S.X;
	Crs->Y = S.Y;
	Crs->Size = S.Size;
	Crs->Enabled = S.Enabled;
	GotoXY(Crs->X, Crs->Y, absolute);
	if (Crs->Enabled) CrsShow();
}

void Screen::CrsShow()
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	Crs->Enabled = true;
	applyCursorInfo();
}

void Screen::CrsHide()
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
#ifndef _DEBUG
	Crs->Enabled = false;
	applyCursorInfo();
#else
	if (FandHost::IsEnabled()) {
		Crs->Enabled = false;
		applyCursorInfo();
	}
	else {
		CrsShow();
	}
#endif
}

void Screen::CrsBig()
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	if (Crs->Size == 1) { CrsHide(); Crs->Size = bigCrsSize; } CrsShow();
}

void Screen::CrsNorm()
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	if (Crs->Size == bigCrsSize) { CrsHide(); Crs->Size = 1; } CrsShow();
}

/**
 * \brief Go to XY coords
 * \param X coord X
 * \param Y coord Y
 * \param pos positioning type
 */
void Screen::GotoXY(WORD X, WORD Y, ScrPosition pos)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
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
	_crsX = (short)X;
	_crsY = (short)Y;
	applyCursorPos();
}

short Screen::WhereX()
{
	// vraci relativni pozici (k aktualnimu oknu) cislovanou od 1
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	return _crsX - WindMin->X + 1;
}

short Screen::WhereY()
{
	// vraci relativni pozici (k aktualnimu oknu) cislovanou od 1
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	return _crsY - WindMin->Y + 1;
}

short Screen::WhereXabs()
{
	// vraci absolutni pozici cislovanou od 1
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	return _crsX;
}

short Screen::WhereYabs()
{
	// vraci absolutni pozici cislovanou od 1
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	return _crsY;
}

void Screen::Window(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	// puvodni kod z ASM
	if (X2 < X1) return;
	if (Y2 < Y1) return;
	if (X2 > TxtCols) return;
	if (Y2 > TxtRows) return;
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
	throw std::exception("Screen::ScrWr() not implemented");
}

void Screen::CrsDark()
{
	throw std::exception("Screen::CrsDark() not implemented");
}

void Screen::CrsBlink()
{
	throw std::exception("Screen::CrsBlink() not implemented");
}

// souradnice jsou 0-based
void Screen::CrsGotoXY(WORD aX, WORD aY)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	Crs->X = aX;
	Crs->Y = aY;
	_crsX = (short)(aX + 1);
	_crsY = (short)(aY + 1);
	applyCursorPos();
}

// ---------------------------------------------------------------------------
// ukladani a obnova casti obrazovky
// ---------------------------------------------------------------------------

// souradnice jsou 0-based; do P ulozi SizeX x SizeY bunek
int Screen::ScrPush1(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	CHAR_INFO* dst = (CHAR_INFO*)P;
	for (int r = 0; r < SizeY; r++) {
		for (int c = 0; c < SizeX; c++) {
			CHAR_INFO ci; ci.Char.AsciiChar = ' '; ci.Attributes = 7;
			if (inside(X + c, Y + r)) ci = cell(X + c, Y + r);
			dst[r * SizeX + c] = ci;
		}
	}
	return SizeX * SizeY;
}

size_t Screen::ScreenCount()
{
	return _windowStack.size();
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
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	// cislovani radku a sloupcu prichazi od 1 .. X
	c1--; c2--;
	r1--; r2--;

	SMALL_RECT rect{ c1, r1, c2, r2 };
	COORD bufSize{ (short)(c2 - c1 + 1), (short)(r2 - r1 + 1) };
	CHAR_INFO* buf = new CHAR_INFO[(size_t)bufSize.X * bufSize.Y];
	for (int r = 0; r < bufSize.Y; r++) {
		for (int c = 0; c < bufSize.X; c++) {
			CHAR_INFO ci; ci.Char.AsciiChar = ' '; ci.Attributes = 7;
			if (inside(c1 + c, r1 + r)) ci = cell(c1 + c, r1 + r);
			buf[r * bufSize.X + c] = ci;
		}
	}
	_windowStack.push({ wp, bufSize, rect, buf });
	return (int)_windowStack.size();
}

WParam* Screen::LoadScreen(bool draw)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	if (_windowStack.empty()) {
		printf("Screen::LoadScreen() zasobnik je prazdny!!!\n");
		return nullptr;
	}
	auto scr = _windowStack.top();
	_windowStack.pop();
	if (draw) {
		for (int r = 0; r < scr.coord.Y; r++) {
			for (int c = 0; c < scr.coord.X; c++) {
				if (inside(scr.rect.Left + c, scr.rect.Top + r)) {
					cell(scr.rect.Left + c, scr.rect.Top + r) = scr.content[r * scr.coord.X + c];
				}
			}
		}
		flush(scr.rect.Left, scr.rect.Top, scr.coord.X, scr.coord.Y);
	}
	delete[] scr.content;
	return scr.wp;
}
