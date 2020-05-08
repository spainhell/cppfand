//#include "kbdww.h"
//
//#include "base.h"
//#include "drivers.h"
//#include <set>
//
//WORD LenStyleStr(pstring s)
//{
//	WORD l, i;
//	std::set<BYTE> ctrlChars = { 0x13, 0x17, 0x11, 0x04, 0x02, 0x05, 0x01 };
//	l = s.length(); for (i = 1; i < s.length(); i++)
//		if (ctrlChars.count(s[i]) > 0) l--;
//	return l;
//}
//
//WORD LogToAbsLenStyleStr(pstring s, WORD l)
//{
//	WORD i = l;
//	std::set<BYTE> ctrlChars = { 0x13, 0x17, 0x11, 0x04, 0x02, 0x05, 0x01 };
//	while ((i <= s.length()) && (l > 0))
//	{
//		if (ctrlChars.count(s[i]) == 0) l--;
//		i++;
//	}
//	return i - 1;
//}
//
//bool SetStyleAttr(char c, BYTE& a)
//{
//	auto result = true;
//	/* !!! with colors do!!! */
//	if (c == 0x13) a = colors.tUnderline;
//	else if (c == 0x17) a = colors.tItalic;
//	else if (c == 0x11) a = colors.tDWidth;
//	else if (c == 0x04) a = colors.tDStrike;
//	else if (c == 0x02) a = colors.tEmphasized;
//	else if (c == 0x05) a = colors.tCompressed;
//	else if (c == 0x01) a = colors.tElite;
//	else result = false;
//	return result;
//}
//
//void WrStyleChar(char c)
//{
//	BYTE a; bool b; WORD i;
//	if (SetStyleAttr(c, a))
//	{
//		i = CStyle.first(c);
//		if (i != 0)
//		{
//			CStyle.Delete(i, 1);
//			CColor.Delete(i, 1);
//		}
//		else
//		{
//			pstring oldCs = CStyle;
//			CStyle = c;
//			CStyle += oldCs;
//			pstring oldCo = CColor;
//			CColor = a;
//			CColor += oldCo;
//		}
//		TextAttr = CColor[1];
//	}
//	else if (c == 0x0D) printf("%c%c", 0x0D, 0x0A);
//	else if (c != 0x0A) printf("%c", c);
//}
//
//void WrStyleStr(pstring s, WORD Attr)
//{
//	TextAttr = Attr;
//	CStyle = "";
//	CColor = char(Attr);
//	for (WORD i = 1; i < s.length(); i++) WrStyleChar(s[i]);
//	TextAttr = Attr;
//}
//
//void WrLongStyleStr(LongStr* S, WORD Attr)
//{
//	WORD i;
//	TextAttr = Attr;
//	CStyle = "";
//	CColor = char(Attr);
//	for (i = 1; i < S->LL; i++) WrStyleChar(S->A[i]);
//	TextAttr = Attr;
//}
//
//void RectToPixel(WORD c1, WORD r1, WORD c2, WORD r2, WORD& x1, WORD& y1, WORD& x2, WORD& y2)
//{
//	WORD mx, my; BYTE dif;
//#ifdef FandGraph
//	if (IsGraphMode) {
//		if (GraphMode == HercMonoHi) dif = 2;
//		else dif = 0;
//		mx = GetMaxX;
//		if (GraphMode == VGAHi /*GraphDriver=VGA*/) my = GetMaxY - 5;
//		else my = GetMaxY;
//		x1 = (c1 * mx)/ TxtCols; y1 = (r1 * (my + dif))/ TxtRows;
//		x2 = ((c2 + 1) * mx)/ TxtCols; y2 = ((r2 + 1) * (my + dif))/ TxtRows;
//	}
//#endif
//}
//
//
//void wRllmSG(WORD N)
//{
//	RdMsg(N);
//	WrLLMsgTxt();
//}
//
//void WrLLF10MsgLine()
//{
//	WORD Buf[MaxTxtCols];
//	WORD col, row, len;
//
//	row = TxtRows - 1;
//	ScrRdBuf(0, row, &Buf[0], TxtCols);
//	Beep();
//	ScrClr(0, row, TxtCols, 1, ' ', colors.zNorm);
//	if (F10SpecKey == 0xffff) ScrWrStr(0, row, "...!", colors.zNorm | 0x80);
//	else ScrWrStr(0, row, "F10!", colors.zNorm | 0x80);
//	col = MsgLine.length() + 5;
//	len = 0;
//	if ((F10SpecKey == 0xfffe) || (F10SpecKey == _F1_)) {
//		MsgLine = MsgLine + ' ' + "F1";
//		len = 2;
//	}
//	if ((F10SpecKey == 0xfffe) || (F10SpecKey == _ShiftF7_)) {
//		MsgLine = MsgLine + ' ' + "ShiftF7"; len += 7;
//	}
//	if (MsgLine.length() > TxtCols - 5) {
//		MsgLine[0] = char(TxtCols - 5);
//		len = 0;
//	}
//	ScrWrStr(5, row, MsgLine, colors.zNorm);
//label1:
//	GetEvent();
//	/*with Event*/
//	switch (Event.What) {
//	case evMouse:
//		if (MouseInRect(0, row, 3, 1))
//		{
//			KbdChar = _F10_;
//			goto label2;
//		}
//		if (len > 0 && MouseInRect(col, row, len, 1)) {
//			KbdChar = F10SpecKey;
//			goto label2;
//		}
//	case evKeyDown:
//		if (Event.KeyCode == _F10_ || Event.KeyCode == F10SpecKey || F10SpecKey == 0xffff
//			|| F10SpecKey == 0xfffe && (Event.KeyCode == _ShiftF7_ || Event.KeyCode == _F1_))
//		{
//			KbdChar = Event.KeyCode;
//		label2:
//			ClrEvent();
//			goto label3;
//		}
//	}
//	ClrEvent();
//	goto label1;
//label3:
//	F10SpecKey = 0;
//	ScrWrBuf(0, row, &Buf[0], TxtCols);
//
//}
//
//void WrLLF10Msg(WORD N)
//{
//	RdMsg(N);
//	WrLLF10MsgLine();
//}
//
//void RunError(WORD N)
//{
//	RunErrNr = N;
//	ClearKbdBuf();
//	WrLLF10Msg(RunErrNr);
//	GoExit();
//}
