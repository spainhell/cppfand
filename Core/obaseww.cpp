#include "obaseww.h"

#include <algorithm>
#include "base.h"
#include "GlobalVariables.h"
#include "oaccess.h"
#include "OldDrivers.h"
#include "../Common/textfunc.h"
#include "../Drivers/constants.h"

WORD RunErrNr = 0;

WParam* PushWParam(WORD C1, WORD R1, WORD C2, WORD R2, bool WW)
{
	WParam* wp = new WParam();
	wp->Min = WindMin;
	wp->Max = WindMax;
	wp->Attr = TextAttr;
	wp->Cursor = screen.CrsGet();
	if (WW) screen.Window(C1, R1, C2, R2);
	return wp;
}

void PopWParam(WParam* wp)
{
	if (wp == nullptr) return;
	WindMin = wp->Min;
	WindMax = wp->Max;
	TextAttr = wp->Attr;
	screen.CrsSet(wp->Cursor);
}

void* PushScr(WORD C1, WORD R1, WORD C2, WORD R2)
{
	screen.SaveScreen(nullptr, C1, R1, C2, R2);
	return nullptr;
}

int PushW(WORD C1, WORD R1, WORD C2, WORD R2, bool push_pixel, bool ww)
{
	int pos = 0;
	WParam* wp = PushWParam(C1, R1, C2, R2, ww);
	wp->GrRoot = pos;
	return screen.SaveScreen(wp, C1, R1, C2, R2);
}

void PopScr(bool draw)
{
	WParam* wp = screen.LoadScreen(draw);
	PopWParam(wp);
	delete wp;
}

void PopW(int pos, bool draw)
{
	int count = (int)screen.ScreenCount();
	if (pos > count) {
		return;
	}

	for (int i = 0; i < count - pos; i++) {
		PopScr(false);
	}

	PopScr(draw);
}

void WriteWFrame(uint8_t WFlags, const std::string& top, const std::string& bottom, const uint8_t color)
{
	if ((WFlags & WHasFrame) == 0) return;

	uint8_t n = 0;
	if ((WFlags & WDoubleFrame) != 0) n = 9;

	uint8_t cols = WindMax.X - WindMin.X + 1;
	uint8_t rows = WindMax.Y - WindMin.Y + 1;
	screen.ScrWrFrameLn(WindMin.X, WindMin.Y, n, cols, color);

	for (uint8_t i = 1; i <= rows - 2; i++) {
		if ((WFlags & WNoClrScr) == 0)
			screen.ScrWrFrameLn(WindMin.X, WindMin.Y + i, n + 6, cols, color);
		else {
			screen.ScrWrChar(WindMin.X, WindMin.Y + i, static_cast<char>(FrameChars[n + 6]), color);
			screen.ScrWrChar(WindMin.X + cols - 1, WindMin.Y + i, static_cast<char>(FrameChars[n + 8]), color);
		}
	}

	screen.ScrWrFrameLn(WindMin.X, WindMax.Y, n + 3, cols, color);
	WriteHeader(top, 1, cols - 2);
	WriteHeader(bottom, rows, cols - 2);
}

void WriteHeader(std::string header, WORD row, WORD maxCols)
{
	if (header.empty()) return;

	header = (" " + header + " ").substr(0, maxCols);
	const uint16_t col = static_cast<uint16_t>((maxCols - header.length()) / 2 + 2);
	screen.ScrWrText(col, row, header.c_str());
}

void CenterWw(uint8_t& C1, uint8_t& R1, uint8_t& C2, uint8_t& R2, uint8_t WFlags)
{
	int16_t m = 0;
	if ((WFlags & WHasFrame) != 0) {
		m = 2;
	}

	// calculate columns
	int16_t cols = static_cast<int16_t>(C2 + m);
	if (C1 != 0) {
		cols = static_cast<int16_t>(C2 - C1 + 1);
	}

	cols = max(m + 1, min(cols, TxtCols));
	if (C1 == 0) {
		C1 = static_cast<int8_t>((TxtCols - cols) / 2 + 1);
	}
	else {
		C1 = min(C1, static_cast<int8_t>(TxtCols - cols + 1));
	}
	C2 = static_cast<int8_t>(C1 + cols - 1);

	// calculate rows
	int16_t rows = static_cast<int16_t>(R2 + m);
	if (R1 != 0) {
		rows = static_cast<int16_t>(R2 - R1 + 1);
	}
	rows = max(m + 1, min(rows, TxtRows));
	if (R1 == 0) {
		R1 = static_cast<int8_t>((TxtRows - rows) / 2 + 1);
	}
	else {
		R1 = static_cast<int8_t>(min(R1, TxtRows - rows + 1));
	}
	R2 = static_cast<int8_t>(R1 + rows - 1);
}

int PushWFramed(uint8_t C1, uint8_t R1, uint8_t C2, uint8_t R2, WORD Attr, std::string top, std::string bottom, uint8_t WFlags)
{
	WORD i = 0;
	CenterWw(C1, R1, C2, R2, WFlags);
	WORD x = 0; WORD y = 0;
	if ((WFlags & WShadow) != 0) {
		x = MinW(2, TxtCols - C2);
		y = MinW(1, TxtRows - R2);
	}
	auto result = PushW(C1, R1, C2 + x, R2 + y, (WFlags & WPushPixel) != 0, true);
	screen.CrsHide();
	if (y == 1) screen.ScrColor(C1 + 1, R2, C2 - C1 + x - 1, screen.colors.ShadowAttr);
	if (x > 0) for (i = R1; i <= R2; i++) screen.ScrColor(C2, i, x, screen.colors.ShadowAttr);
	screen.Window(C1, R1, C2, R2);
	TextAttr = Attr;
	if ((WFlags & WHasFrame) != 0) {
		WriteWFrame(WFlags, top, bottom, TextAttr);
		screen.Window(C1 + 1, R1 + 1, C2 - 1, R2 - 1);
	}
	return result;
}

int PushWrLLMsg(WORD N, bool WithESC)
{
	int result = PushW(1, TxtRows, TxtCols, TxtRows);
	TextAttr = screen.colors.zNorm;
	ClrEol(TextAttr);
	TextAttr = screen.colors.zNorm | 0x80;
	screen.ScrWrText(1, 1, "  ");
	TextAttr = screen.colors.zNorm;
	if (WithESC) screen.ScrWrText(6, 1, "(ESC) ");
	ReadMessage(N);
	WORD max_len = TxtCols - screen.WhereX();
	//if (MsgLine.length() > max_len) MsgLine[0] = (char)max_len;
	MsgLine = MsgLine.substr(0, max_len);
	screen.ScrWrText(WithESC ? 12 : 6, 1, MsgLine.c_str());
	return result;
}

// nacteni a zapis posledniho radku s klavesovymi zkratkami
void WrLLMsg(WORD N)
{
	std::string message = ReadMessage(N);
	WrLLMsgTxt(message);
}

void WrLLMsgTxt(std::string& message)
{
	WordRec w; bool On;
	WORD Buf[MaxTxtCols + 1];
	WParam* p = PushWParam(1, TxtRows, TxtCols, TxtRows, true);
	w.Hi = screen.colors.lNorm;
	On = false;
	WORD i = 0;
	WORD j = 0;
	while ((i < message.length()) && (j < TxtCols)) {
		if (message[i] == 0x17)
		{
			if (On) { w.Hi = screen.colors.lNorm; On = false; }
			else { w.Hi = screen.colors.lFirst; On = true; }
		}
		else {
			w.Lo = message[i];
			Buf[j] = (w.Hi << 8) + w.Lo;
			j++;
		}
		i++;
	}
	w.Lo = ' ';
	while (j < TxtCols)
	{
		Buf[j] = (w.Hi << 8) + w.Lo;
		j++;
	}
	screen.ScrWrBuf(0, TxtRows - 1, Buf, TxtCols);
	PopWParam(p);
	delete p;
}

void WrLLF10MsgLine(std::string& message)
{
	WORD row = TxtRows - 1;
	CHAR_INFO* Buf = new CHAR_INFO[TxtCols];
	screen.ScrRdBuf(1, row + 1, Buf, TxtCols);
	Beep();
	screen.ScrClr(1, row + 1, TxtCols, 1, ' ', screen.colors.zNorm);
	if (F10SpecKey == 0xffff) {
		screen.ScrWrStr(1, row + 1, "...!", screen.colors.zNorm | 0x80);
	}
	else if (spec.F10Enter) {
		screen.ScrWrStr(1, row + 1, "\x11\xD9 !", screen.colors.zNorm | 0x80);
	}
	else {
		screen.ScrWrStr(1, row + 1, "F10!", screen.colors.zNorm | 0x80);
	}
	WORD col = message.length() + 5;
	WORD len = 0;
	if ((F10SpecKey == 0xfffe) || (F10SpecKey == __F1)) {
		message = message + " " + "F1";
		len = 2;
	}
	if ((F10SpecKey == 0xfffe) || (F10SpecKey == __SHIFT_F7)) {
		message = message + " " + "ShiftF7";
		len += 7;
	}
	if (MsgLine.length() > TxtCols - 5) {
		message = message.substr(0, TxtCols - 5);
		len = 0;
	}
	screen.ScrWrStr(6, row + 1, message, screen.colors.zNorm);

	bool end = false;
	while (!end) {
		GetEvent();
		unsigned short key = Event.Pressed.KeyCombination();
		switch (Event.What) {
		case evMouse:
			if (MouseInRect(0, row, 3, 1)) {
				Event.Pressed.UpdateKey(__F10);
				end = true;
			}
			if (len > 0 && MouseInRect(col, row, len, 1)) {
				Event.Pressed.UpdateKey(F10SpecKey);
				end = true;
			}
			break;
		case evKeyDown:
			if (spec.F10Enter && key == __ENTER) {
				key = __F10;
			}
			if (key == __F10 || key == F10SpecKey || F10SpecKey == 0xffff
				|| F10SpecKey == 0xfffe && (key == __SHIFT_F7 || key == __F1)) {
				ClrEvent();
				end = true;
			}
			break;
		}

		if (end) {
			break;
		}
		else {
			ClrEvent();
		}
	}

	F10SpecKey = 0;
	screen.ScrWrCharInfoBuf(1, row + 1, Buf, TxtCols);
	delete[] Buf;
}

/**
 * \brief Write F10 message in last line
 * \param msgNr Message number
 */
void WrLLF10Msg(int msgNr)
{
	std::string message = ReadMessage(msgNr);
	WrLLF10MsgLine(message);
}

void RunError(WORD N)
{
	RunErrNr = N;
	ClearKbdBuf();
	WrLLF10Msg(RunErrNr);
	GoExit(MsgLine);
}

bool PromptYN(WORD NMsg)
{
	int w = PushW(1, TxtRows, TxtCols, TxtRows);
	TextAttr = screen.colors.pTxt;
	ClrEol(TextAttr);
	std::string message = ReadMessage(NMsg);
	if (message.length() > TxtCols - 3) {
		message = message.substr(0, TxtCols - 3);
	}
	screen.ScrFormatWrText(screen.WhereX(), screen.WhereY(), "%s", message.c_str());
	WORD col = screen.WhereX();
	WORD row = screen.WhereY();
	TextAttr = screen.colors.pNorm;
	screen.ScrFormatWrStyledText(col, row, screen.colors.pNorm, " ");
	screen.GotoXY(col, row);
	screen.CrsShow();

	char cc;
	while (true) {
		cc = (char)toupper(ReadKbd());
		if ((Event.Pressed.KeyCombination() != F10SpecKey) && (cc != AbbrYes) && (cc != AbbrNo)) {
			continue;
		}
		break;
	}

	F10SpecKey = 0;
	PopW(w);
	return cc == AbbrYes;
}

void FileMsg(FileD* file_d, int n, char Typ)
{
	file_d->SetPathAndVolume();
	if (Typ == 'T') {
		CPath = file_d->CExtToT(CDir, CName, CExt);
	}
	else if (Typ == 'X') {
		CPath = CExtToX(CDir, CName, CExt);
	}
	std::string path = CPath;
	ReplaceChar(path, '/', '\\');
	SetMsgPar(path);
	WrLLF10Msg(n);
}
