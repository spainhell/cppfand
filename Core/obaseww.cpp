#include "obaseww.h"

#include "../Common/textfunc.h"
#include "../fandio/files.h"
#include "base.h"
#include "OldDrivers.h"
#include "GlobalVariables.h"
#include "oaccess.h"
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

void WriteWFrame(BYTE WFlags, std::string top, std::string bottom)
{
	WORD i, cols, rows, n;
	if ((WFlags & WHasFrame) == 0) return;
	n = 0;
	if ((WFlags & WDoubleFrame) != 0) n = 9;
	cols = WindMax.X - WindMin.X + 1;
	rows = WindMax.Y - WindMin.Y + 1;
	screen.ScrWrFrameLn(WindMin.X, WindMin.Y, n, cols, TextAttr);
	for (i = 1; i <= rows - 2; i++) {
		if ((WFlags & WNoClrScr) == 0)
			screen.ScrWrFrameLn(WindMin.X, WindMin.Y + i, n + 6, cols, TextAttr);
		else {
			screen.ScrWrChar(WindMin.X, WindMin.Y + i, FrameChars[n + 6], TextAttr);
			screen.ScrWrChar(WindMin.X + cols - 1, WindMin.Y + i, FrameChars[n + 8], TextAttr);
		}
	}
	screen.ScrWrFrameLn(WindMin.X, WindMax.Y, n + 3, cols, TextAttr);
	WrHd(top, 1, cols - 2);
	WrHd(bottom, rows, cols - 2);
}

void WrHd(std::string header, WORD row, WORD maxCols)
{
	if (header.empty()) return;
	header = " " + header + " ";
	if (header.length() > maxCols) header = header.substr(0, maxCols);
	screen.ScrWrText((maxCols - header.length()) / 2 + 2, row, header.c_str());
}

void CenterWw(BYTE& C1, BYTE& R1, BYTE& C2, BYTE& R2, BYTE WFlags)
{
	short Cols, Rows, M;
	M = 0;
	if ((WFlags & WHasFrame) != 0) M = 2;
	Cols = C2 + M;
	if (C1 != 0) Cols = C2 - C1 + 1;
	Cols = MaxI(M + 1, MinI(Cols, TxtCols));
	if (C1 == 0) C1 = (TxtCols - Cols) / 2 + 1;
	else C1 = MinI(C1, TxtCols - Cols + 1);
	C2 = C1 + Cols - 1;
	Rows = R2 + M;
	if (R1 != 0) Rows = R2 - R1 + 1;
	Rows = MaxI(M + 1, MinI(Rows, TxtRows));
	if (R1 == 0) R1 = (TxtRows - Rows) / 2 + 1;
	else R1 = MinI(R1, TxtRows - Rows + 1);
	R2 = R1 + Rows - 1;
}

int PushWFramed(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, std::string top, std::string bottom, BYTE WFlags)
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
		WriteWFrame(WFlags, top, bottom);
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
	//printf("  ");
	screen.ScrWrText(1, 1, "  ");
	TextAttr = screen.colors.zNorm;
	if (WithESC) screen.ScrWrText(6, 1, "(ESC) ");  //printf("(ESC) ");
	ReadMessage(N);
	WORD l = TxtCols - screen.WhereX();
	if (MsgLine.length() > l) MsgLine[0] = (char)l;
	//printf("%s", MsgLine.c_str());
	screen.ScrWrText(WithESC ? 12 : 6, 1, MsgLine.c_str());
	return result;
}

// nacteni a zapis posledniho radku s klavesovymi zkratkami
void WrLLMsg(WORD N)
{
	ReadMessage(N);
	WrLLMsgTxt();
}

void WrLLMsgTxt()
{
	WordRec w; bool On;
	WORD Buf[MaxTxtCols + 1];
	WParam* p = PushWParam(1, TxtRows, TxtCols, TxtRows, true);
	w.Hi = screen.colors.lNorm;
	On = false;
	WORD i = 0;
	WORD j = 0;
	while ((i < MsgLine.length()) && (j < TxtCols)) {
		if (MsgLine[i] == 0x17)
		{
			if (On) { w.Hi = screen.colors.lNorm; On = false; }
			else { w.Hi = screen.colors.lFirst; On = true; }
		}
		else {
			w.Lo = MsgLine[i];
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

void WrLLF10MsgLine()
{
	WORD row = TxtRows - 1;
	CHAR_INFO* Buf = new CHAR_INFO[TxtCols];
	screen.ScrRdBuf(1, row + 1, Buf, TxtCols);
	Beep();
	screen.ScrClr(1, row + 1, TxtCols, 1, ' ', screen.colors.zNorm);
	if (F10SpecKey == 0xffff) {
		screen.ScrWrStr(1, row + 1, "...!", screen.colors.zNorm | 0x80);
	}
	else if (spec.F10Enter)	{
		screen.ScrWrStr(1, row + 1, "\x11\xD9 !", screen.colors.zNorm | 0x80);
	}
	else {
		screen.ScrWrStr(1, row + 1, "F10!", screen.colors.zNorm | 0x80);
	}
	WORD col = MsgLine.length() + 5;
	WORD len = 0;
	if ((F10SpecKey == 0xfffe) || (F10SpecKey == __F1)) {
		MsgLine = MsgLine + " " + "F1";
		len = 2;
	}
	if ((F10SpecKey == 0xfffe) || (F10SpecKey == __SHIFT_F7)) {
		MsgLine = MsgLine + " " + "ShiftF7";
		len += 7;
	}
	if (MsgLine.length() > TxtCols - 5) {
		MsgLine = MsgLine.substr(0, TxtCols - 5);
		len = 0;
	}
	screen.ScrWrStr(6, row + 1, MsgLine, screen.colors.zNorm);

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
	ReadMessage(msgNr);
	WrLLF10MsgLine();
}

void RunError(WORD N)
{
	RunErrNr = N;
	ClearKbdBuf();
	WrLLF10Msg(RunErrNr);
	GoExit();
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
	SetPathAndVolume(file_d);
	if (Typ == 'T') {
		CPath = CExtToT(CDir, CName, CExt);
	}
	else if (Typ == 'X') {
		CPath = CExtToX(CDir, CName, CExt);
	}
	std::string path = CPath;
	ReplaceChar(path, '/', '\\');
	SetMsgPar(path);
	WrLLF10Msg(n);
}
