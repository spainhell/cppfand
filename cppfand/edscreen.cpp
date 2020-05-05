#include "edscreen.h"


#include "common.h"
#include "edglobal.h"
#include "editor.h"
#include "kbdww.h"
#include "keybd.h"

/// POZOR na 'Len:byte absolute Blanks;'
//  http://home.pf.jcu.cz/~edpo/program/kap08.html
void WrStatusLine()
{
	pstring Blanks;
	BYTE* Len = (BYTE*)&Blanks;
	string s;
	integer i;

	if (Mode != HelpM) {
		FillChar(&Blanks[1], TxtCols, 32);
		*Len = TXTCOLS;
		if (HeadS != nullptr) {
			Move(&HeadS[1], &Blanks[1], HeadS.length());
			i = Blanks.first('_');
			if (i == 0) {
				Move(&Blanks[1], &Blanks[TStatL + 3], 252 - TStatL);
				FillChar(&Blanks[1], TStatL + 2, 32);
			}
			else {
				while ((i <= *Len) && (Blanks[i] == ' ')) {
					Blanks[i] = ' ';
					i++;
				}
			}
		}
		else {
			s = ShortName(NameT);
			i = TStatL + 3;
			if (s.length() + i >= TXTCOLS) i = TXTCOLS - s.length() - 2;
			Move(&s[1], &Blanks[i], s.length());
		}
		ScrWrStr(0, 0, Blanks, SysLColor);
	}
}

void WriteMargins()
{
	WORD LastL[201];

	if ((Mode != HelpM) && (Mode != ViewM) && Wrap) {
		ScrRdBuf(FirstC - 1, TxtRows - 1, &LastL[1], LineS);
		LastL[MargLL[0]] = MargLL[1]; 
		LastL[MargLL[2]] = MargLL[3];
		MargLL[0] = MaxI(0, LeftMarg - BPos); 
		if (MargLL[0] > 0) {
			MargLL[1] = LastL[MargLL[0]];
			LastL[MargLL[0]] = (LastL[LineS] & 0xFF00) + 0x10;
		}
		MargLL[2] = MaxI(0, RightMarg - BPos);
		if (MargLL[2] > 0) {
			MargLL[3] = LastL[MargLL[2]];
			LastL[MargLL[2]] = (LastL[LineS] & 0xFF00) + 0x11;
		}
		ScrWrBuf(FirstC - 1, TxtRows - 1, &LastL[1], LineS);
	}
}

void WrLLMargMsg(string* s, WORD n)
{
	if (s != nullptr) {
		MsgLine = *s; 
		WrLLMsgTxt();
	}
	else {
		if (n != 0) WrLLMsg(n);
		else {
			if (LastS != nullptr) {
				MsgLine = LastS; 
				WrLLMsgTxt();
			}
			else {
				WrLLMsg(LastNr);
			}
			if (Mode == TextM) WriteMargins();
		}
	}
}

void InitScr()
{
	FirstR = WindMin.Y + 1; 
	FirstC = WindMin.X + 1;
	LastR = WindMax.Y + 1; 
	LastC = WindMax.X + 1;
	if ((FirstR == 1) && (Mode != HelpM)) FirstR++;
	if (LastR == TxtRows) LastR--;
	MinC = FirstC; MinR = FirstR; MaxC = LastC; MaxR = LastR;
	Window(FirstC, FirstR, LastC, LastR);
	FirstR--;
	if ((Mode != HelpM) and (Mode != ViewM) and Wrap) LastC--;
	PageS = LastR - FirstR; LineS = Succ(LastC - FirstC);
}

void UpdStatLine(int Row, int Col)
{
	pstring StatLine(35);
	pstring st(10);
	BYTE* len = (BYTE*)&st;
	integer i;
	longint lRow;

	if (!HelpScroll) {
		lRow = Row + Part.LineP;
		StatLine = "     1:                             ";
		str(lRow, 5, st);
		move(st[1], StatLine[2], 5);
		str(Col, st);
		while (st.length() < 4) { st = st + ' '; }
		move(st[1], StatLine[8], 4);
		switch (Mode) {
			case TextM:	{
				if (Insert) Move(&InsMsg[1], &StatLine[11], 5);
				else Move(&nInsMsg[1], &StatLine[11], 5);
				if (Indent) Move(&IndMsg[1], &StatLine[16], 5);
				if (Wrap) Move(&WrapMsg[1], &StatLine[21], 5);
				if (Just) Move(&JustMsg[1], &StatLine[26], 5);
				if (TypeB == ColBlock) Move(&BlockMsg[1], &StatLine[31], 5);
				break;
			}
			case ViewM: { Move(&ViewMsg[1], &StatLine[11], ViewMsg.length()); break; }
			case SinFM: { StatLine[13] = '-'; break; }
			case DouFM: { StatLine[13] = '='; break; }
			case DelFM: { StatLine[13] = '/'; break; }
			default: break;
		}
		i = 1;
		if (HeadS != nullptr) {
			i = MaxW(1, HeadS.first('_'));
			if (i > TxtCols - TStatL) i = MaxI(integer(TxtCols) - TStatL, 1);
		}
		ScrWrStr(i - 1, 0, StatLine, SysLColor);
	}
}

void EditWrline(ArrPtr P, int Row)
{
	WORD BuffLine[255];
	WORD Line;
	integer I, LP, B, E;
	integer Newvalue;
	BYTE* Nv = (BYTE*)&Newvalue;
	bool IsCtrl;

	Line = pred(ScrL + Row);
	if (LineInBlock(Line) && (TypeB == TextBlock)) Nv[2] = BlockColor;
	else Nv[2] = TxtColor;
	I = 1;
	while ((P[I] != _CR) && (I <= LineSize)) {
		Nv[1] = P[I]; 
		BuffLine[I] = Newvalue;
		if (Nv[1] < 32) IsCtrl = true; 
		I++;
	}
	
	LP = I - 1; 
	Nv[1] = 32;

	for (int i = LP + 1; i < BPos + LineS; i++) { BuffLine[I] = Newvalue; }
	
	if (BegBLn <= EndBLn) {
		if (LineBndBlock(Line) || ((TypeB == ColBlock) && LineInBlock(Line))) {
			if ((BegBLn == LineAbs(Line)) || (TypeB == ColBlock)) {
				B = MinI(BegBPos, LineS + BPos + 1);
			}
			else { B = 1; }
			if ((EndBLn == LineAbs(Line)) || (TypeB = ColBlock)) {
				E = MinI(EndBPos, LineS + BPos + 1);
			}
			else { E = LineS + BPos + 1; }
			for (int i = B; i < pred(E); i++) {
				BuffLine[I] = (BuffLine[I] & 0x00FF) + (BlockColor << 8);
			}
		}
	}
	if (IsCtrl) {
		for (int i = succ(BPos); i < LP; i++) {
			if (P[I] < 32) {
				BuffLine[I] = ((P[I] + 64) & 0x00FF) + (ColKey[CtrlKey.first(P[I])] << 8);
			}
		}
	}
	ScrWrBuf(WindMin.X, WindMin.Y + Row - 1, &BuffLine[BPos + 1], LineS);
}

void ScrollWrline(ArrPtr P, int Row, ColorOrd CO)
{
	pstring GrafCtrl(15);
	BYTE* len = (BYTE*)&CO;
	GrafCtrl = "\x03\x06\x09\x11\x15\x16\x18\x21\x22\x24\x25\x26\x29\x30\x31";
	WORD BuffLine[255];
	integer I, J, LP, pp;
	integer Newvalue;
	// Nv : array [1..2] of byte absolute Newvalue;
	BYTE* Nv = (BYTE*)&Newvalue;
	bool IsCtrl;
	char cc;
	BYTE Col;

	Col = Color(CO);
	Nv[2]=Col; I=1; J=1; cc=P[I];
	while ((cc != _CR) && (I <= LineSize) || !InsPage) {
		if ((cc >= 32) || (GrafCtrl.first(cc) > 0)) {
			Nv[1] = cc; BuffLine[J] = Newvalue; J++;
		}
		else {
			if (CtrlKey.first(cc) > 0) IsCtrl = true;
			else {
				if (bScroll && (cc == 0x0C)) { InsPage = InsPg; I++; }
			}
		}
		I++; 
		cc = P[I];
	}
    
	LP = I - 1; 
	Nv[1] = 32;

	while (J <= BCol + LineS) { BuffLine[J] = Newvalue; J++; }
	if (IsCtrl) {
		I = 1; J = 1;
		while (I <= LP) {
			cc = P[I];
			if ((cc >= 32) || (GrafCtrl.first(cc)) > 0)
			{
				BuffLine[J] = (BuffLine[J] & 0x00FF) + (Col << 8);
				J++;
			}
			else if (CtrlKey.first(cc) > 0)
			{
				pp = CO.first(cc);
				if (pp > 0) CO = CO.substr(1, pp - 1) + CO.substr(pp + 1, *len - pp);
				else CO = CO + cc;
				Col = Color(CO);
			}
			else if (cc == 0x0C) BuffLine[J] = 219 + (Col << 8);
			I++;
		}
		while (J <= BCol + LineS) {
			BuffLine[J] = (BuffLine[J] & 0x00FF) + (Col << 8); J++;
		}
	}
    ScrWrBuf(WindMin.X, WindMin.Y+Row-1, &BuffLine[BCol+1], LineS);
}

BYTE Color(ColorOrd CO)
{
	BYTE* len = (BYTE*)&CO;
	if (CO == "") return TxtColor;
	return ColKey[CtrlKey.first(CO[*len])];
}

WORD PColumn(WORD w, ArrPtr P)
{
	WORD c, ww;
	if (w == 0) { return 0; }
	ww = 1; c = 1;
	while (ww <= w) { if (P[ww] >= ' ') c++; ww++; }
	if (P[w] >= ' ') c--;
	return c;
}

bool MyTestEvent()
{
	if (FirstEvent) return false;
	return TestEvent();
}

void UpdScreen()
{
	integer r, rr;
	WORD w;
	WORD Ind;
	ColorOrd co1, co2;
	WORD oldSI;
	pstring PgStr;

label1:
	oldSI = ScrI; InsPage = false;
	if (ChangeScr)
	{
		if (ChangePart) DekodLine(); 
		ChangeScr = false;

		if (bScroll) ScrI = LineI; 
		else ScrI = FindLine(ScrL);
		
		if (HelpScroll)
		{
			ColScr = Part.ColorP;
			SetColorOrd(ColScr, 1, ScrI);
		}
	}
	if (bScroll) // {tisk aktualniho radku}
	{
		FillChar(&PgStr, 256, CharPg);
		PgStr[0] = 255;
		co1 = ColScr; r = 1;
		while (Arr[r] == 0x0C) { r++; }
		ScrollWrline(&Arr[r], 1, co1);
	}
	else if (Mode == HelpM)
	{
		co1 = Part.ColorP; SetColorOrd(co1, 1, LineI);
		ScrollWrline(Arr, LineL - ScrL + 1, co1);
	}
    else EditWrline(Arr,LineL-ScrL+1);
	WrEndL(HardL, LineL - ScrL + 1);
    if (MyTestEvent()) return;
	Ind = ScrI; r = 1; rr = 0; w = 1; InsPage = false; co2 = ColScr;
	if (bScroll) { while (*T[Ind] == 0x0C) { Ind++; } }
	do {
		if (MyTestEvent()) return;                   // {tisk celeho okna}
		if ((Ind >= LenT) && !AllRd)
		{
			NextPartDek(); Ind -= Part.MovI;
		}

		if (bScroll && (Ind < LenT))
			if ((InsPg && (ModPage(r - rr + RScrL - 1))) || InsPage)
			{
				EditWrline((ArrPtr)PgStr[1], r);
				WrEndL(false, r);
				if (InsPage) rr++;
				InsPage = false;
				goto label1;
			}
		if (!Scroll && (Ind == LineI)) {
			Ind = NextI;
			co2 = co1;
			goto label1;
		}
		if (Ind < LenT) {
			if (HelpScroll) ScrollWrline(T[Ind], r, co2);
			else EditWrline(T[Ind], r);
			if (InsPage) Ind = FindChar(w, 0x0C, Ind, LenT) + 1;
			else Ind = FindChar(w, _CR, Ind, LenT) + 1;
			WrEndL((Ind < LenT) && (*T[Ind] == _LF), r);
			if (*T[Ind] == _LF) Ind++;
		}
		else {
			EditWrline(T[LenT], r);
			WrEndL(false, r);
		}

	label1:
		r++;
		if (Scroll && (*T[Ind] == 0x0C)) { InsPage = InsPg; Ind++; }
	} while (r > PageS);
}

void WrEndL(bool Hard, int Row)
{
	WORD w;
	if ((Mode != HelpM) && (Mode != ViewM) && Wrap) {
		if (Hard) w = 0x11 + (TxtColor << 8); 
		else w = 32 + (TxtColor << 8);
		ScrWrBuf(WindMin.X + LineS, WindMin.Y + Row - 1, &w, 1);
	}
}

void Background()
{
	WORD p;

	UpdStatLine(LineL, Posi); 
	if (MyTestEvent()) return;
    if (HelpScroll)
	{
		p = Posi;
		if (Mode == HelpM) if (WordL == LineL) while (Arr[p + 1] != 0x11) { p++; }
        if (Column(p)-BCol>LineS)
		{
			BCol = Column(p) - LineS; BPos = Position(BCol);
		}
        if (Column(Posi)<=BCol)
		{
			BCol = Column(Posi) - 1; BPos = Position(BCol);
		}
	}
	else {
		if (Posi > LineS) if (Posi > BPos + LineS) BPos = Posi - LineS;
		if (Posi <= BPos) BPos = pred(Posi);
	}
	if (LineL < ScrL) { ScrL = LineL; ChangeScr = true; }
	if (LineL >= ScrL + PageS) { ScrL = succ(LineL - PageS); ChangeScr = true; }
    UpdScreen(); // {tisk obrazovky}
    WriteMargins();
	GotoXY(Posi - BPos, succ(LineL - ScrL));
    IsWrScreen=true;
}
