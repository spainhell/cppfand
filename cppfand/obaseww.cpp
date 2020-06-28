#include "obaseww.h"

#include "base.h"
#include "drivers.h"
#include "oaccess.h"

WORD RunErrNr = 0;
RunMsgD* CM = nullptr;

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

longint PushW1(WORD C1, WORD R1, WORD C2, WORD R2, bool PushPixel, bool WW)
{
	longint pos = 0;
	WParam* wp = PushWParam(C1, R1, C2, R2, WW);
	wp->GrRoot = pos;
	screen.SaveScreen(wp, C1, R1, C2, R2);
	return 0;
}

longint PushW(WORD C1, WORD R1, WORD C2, WORD R2)
{
	return PushW1(C1, R1, C2, R2, false, true);
}

void PopScr(void* p, bool draw)
{
	WParam* wp = nullptr;
	screen.LoadScreen(draw, wp);
	//PopWParam(wp);
	//delete wp;
}

void PopW2(longint pos, bool draw)
{
	PopScr(nullptr, draw);
}

void PopW(longint pos)
{
	PopScr(nullptr, true);
}

void WriteWFrame(BYTE WFlags, pstring top, pstring bottom)
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

void WrHd(pstring Hd, WORD Row, WORD MaxCols)
{
	pstring s;
	if (Hd == "") return;
	s = " ";
	s += Hd + " ";
	if (s.length() > MaxCols) s[0] = char(MaxCols);
	//GotoXY((MaxCols - s.length()) / 2 + 2, Row);
	screen.ScrWrText((MaxCols - s.length()) / 2 + 2, Row, s.c_str());
	//printf("%s", s.c_str());
}

void CenterWw(BYTE& C1, BYTE& R1, BYTE& C2, BYTE& R2, BYTE WFlags)
{
	integer Cols, Rows, M;
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

longint PushWFramed(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, pstring top, pstring bottom, BYTE WFlags)
{
	WORD i, x, y;
	CenterWw(C1, R1, C2, R2, WFlags);
	x = 0; y = 0;
	if ((WFlags & WShadow) != 0) {
		x = MinW(2, TxtCols - C2);
		y = MinW(1, TxtRows - R2);
	}
	auto result = PushW1(C1, R1, C2 + x, R2 + y, (WFlags & WPushPixel) != 0, true);
	screen.CrsHide();
	if (y == 1) screen.ScrColor(C1 + 1, R2, C2 - C1 + x - 1, colors.ShadowAttr);
	if (x > 0) for (i = R1; i <= R2; i++) screen.ScrColor(C2, i, x, colors.ShadowAttr);
	screen.Window(C1, R1, C2, R2);
	TextAttr = Attr;
	if ((WFlags & WHasFrame) != 0) {
		WriteWFrame(WFlags, top, bottom);
		screen.Window(C1 + 1, R1 + 1, C2 - 1, R2 - 1);
	}
	return result;
}

longint PushWrLLMsg(WORD N, bool WithESC)
{
	WORD l;
	auto result = PushW(1, TxtRows, TxtCols, TxtRows);
	TextAttr = colors.zNorm; ClrEol();
	TextAttr = colors.zNorm | 0x80; printf("  ");
	TextAttr = colors.zNorm;
	if (WithESC) printf("(ESC) ");
	RdMsg(N);
	l = TxtCols - screen.WhereX();
	if (MsgLine.length() > l) MsgLine[0] = char(l);
	printf("%s", MsgLine.c_str());
	return result;
}

void WrLLMsg(WORD N)
{
	RdMsg(N); WrLLMsgTxt();
}

void WrLLMsgTxt()
{
	WordRec w; bool On;
	WORD Buf[MaxTxtCols + 1];
	WParam* p = PushWParam(1, TxtRows, TxtCols, TxtRows, true);
	w.Hi = colors.lNorm;
	On = false;
	WORD i = 1;
	WORD j = 0;
	while ((i <= MsgLine.length()) && (j < TxtCols)) {
		if (MsgLine[i] == 0x17)
		{
			if (On) { w.Hi = colors.lNorm; On = false; }
			else { w.Hi = colors.lFirst; On = true; }
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
	WORD col = 0, row = 0, len = 0;

	row = TxtRows - 1;
	CHAR_INFO* Buf = new CHAR_INFO[TxtCols];
	screen.ScrRdBuf(0, row, Buf, TxtCols);
	Beep();
	screen.ScrClr(1, row + 1, TxtCols, 1, ' ', colors.zNorm);
	if (F10SpecKey == 0xffff) screen.ScrWrStr(1, row + 1, "...!", colors.zNorm | 0x80);
	else screen.ScrWrStr(1, row + 1, "F10!", colors.zNorm | 0x80);
	col = MsgLine.length() + 5;
	len = 0;
	if ((F10SpecKey == 0xfffe) || (F10SpecKey == _F1_)) {
		MsgLine = MsgLine + " " + "F1";
		len = 2;
	}
	if ((F10SpecKey == 0xfffe) || (F10SpecKey == _ShiftF7_)) {
		MsgLine = MsgLine + " " + "ShiftF7"; len += 7;
	}
	if (MsgLine.length() > TxtCols - 5) {
		MsgLine[0] = char(TxtCols - 5);
		len = 0;
	}
	screen.ScrWrStr(6, row + 1, MsgLine, colors.zNorm);
label1:
	GetEvent();
	/*with Event*/
	switch (Event.What) {
	case evMouse:
		if (MouseInRect(0, row, 3, 1))
		{
			KbdChar = _F10_;
			goto label2;
		}
		if (len > 0 && MouseInRect(col, row, len, 1)) {
			KbdChar = F10SpecKey;
			goto label2;
		}
	case evKeyDown:
		if (Event.KeyCode == VK_F10 || Event.KeyCode == F10SpecKey || F10SpecKey == 0xffff
			|| F10SpecKey == 0xfffe && (Event.KeyCode == _ShiftF7_ || Event.KeyCode == _F1_))
		{
			KbdChar = Event.KeyCode;
		label2:
			ClrEvent();
			goto label3;
		}
	}
	ClrEvent();
	goto label1;
label3:
	F10SpecKey = 0;
	screen.ScrWrCharInfoBuf(0, row, Buf, TxtCols);
	delete[] Buf;
}

void WrLLF10Msg(WORD N)
{
	RdMsg(N);
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
	longint w; WORD col, row; char cc;
	w = PushW(1, TxtRows, TxtCols, TxtRows); TextAttr = colors.pTxt;
	ClrEol();
	RdMsg(NMsg);
	pstring tmp = MsgLine.substr(MaxI(MsgLine.length() - TxtCols + 3, 1), 255);
	printf("%s", tmp.c_str());
	col = screen.WhereX(); row = screen.WhereY(); TextAttr = colors.pNorm;
	printf(" "); screen.GotoXY(col, row); screen.CrsShow();
	label1:
	cc = toupper((char)ReadKbd);
	if ((KbdChar != F10SpecKey) && (cc != AbbrYes) && (cc != AbbrNo)) goto label1;
	F10SpecKey = 0; PopW(w);
	return cc == AbbrYes;
}

void CFileMsg(WORD n, char Typ)
{
	SetCPathVol();
	if (Typ == 'T') CExtToT();
	else if (Typ == 'X') CExtToX();
	SetMsgPar(CPath); WrLLF10Msg(n);
}

void CFileError(WORD N)
{
	CFileMsg(N, '0');
	CloseGoExit();
}

void RunMsgOn(char C, longint N)
{
	RunMsgD* CM1 = new RunMsgD(); // (RunMsgD*)GetStore(sizeof(RunMsgD));
#ifndef norunmsg
	CM1->Last = CM; 
	CM = CM1;
	CM->MsgStep = N / 100;
	if (CM->MsgStep == 0) CM->MsgStep = 1;
	CM->MsgKum = CM->MsgStep;
	CM->MsgNN = N;
	CM->W = PushW1(1, TxtRows, 8, TxtRows, true, true);
	TextAttr = colors.zNorm;

	screen.ScrFormatWrStyledText(1, 1, TextAttr, "%c%c", 0xAF, C);
	if (N == 0) screen.ScrFormatWrStyledText(3, 1, TextAttr, "    %c", 0xAE /*0x11*/);
	else screen.ScrFormatWrStyledText(3, 1, TextAttr, "  0%c%c", '%', 0xAE /*0x11*/);

	/*printf("%c%c", 0x10, C);
	if (N == 0) printf("    %c", 0x11);
	else printf("  0%c%c", '%', 0x11);*/
#endif
}

void RunMsgN(longint N)
{
	WORD Perc;
#ifndef norunmsg
	if (N < CM->MsgKum) return;
	while (N >= CM->MsgKum) CM->MsgKum += CM->MsgStep;
	Perc = (N * 100) / CM->MsgNN; screen.GotoXY(3, 1);
	printf("%*i", 3, Perc);
#endif
}

void RunMsgOff()
{
	void* p;
#ifndef norunmsg
	if (CM == nullptr) return;
	PopW(CM->W);
	CM = CM->Last;
#endif
}

void RunMsgClear()
{
	CM = nullptr;
}
