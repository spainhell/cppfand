#include "edscreen.h"

#include "editor.h"

/// POZOR na 'Len:byte absolute Blanks;'
//  http://home.pf.jcu.cz/~edpo/program/kap08.html
void WrStatusLine()
{
	/*string Blanks;
	int BlanksLen;
	string s;
	int i;

	if (Mode != HelpM) {
		fillchar(Blanks[1], TxtCols, 32);
		BlanksLen = TXTCOLS;
		if (HeadS != nullptr) {
			move(HeadS ^ [1], Blanks[1], length(HeadS^));
			i = pos('_', Blanks);
			if (i == 0) {
				move(Blanks[1], Blanks[TStatL + 3], 252 - TStatL);
				fillchar(Blanks[1], TStatL + 2, 32);
			}
			else {
				while ((i <= Len) && (Blanks[i] = ' ')) {
					Blanks[i] = ' ';
					i++;
				}
			}
		}
		else {
			s = ShortName(NameT);
			i = TStatL + 3;
			if (s.length + i >= TXTCOLS) i = TXTCOLS - s.length - 2;
			move(s[1], Blanks[i], s.length);
		}
		ScrWrStr(0, 0, Blanks, SysLColor);
	}*/
}

void WriteMargins()
{
	WORD LastL[201];

	/*if ((Mode != HelpM) && (Mode != ViewM) && Wrap) {
		ScrRdBuf(FirstC - 1, TxtRows - 1, LastL[1], LineS);
		LastL[MargLL[1]] = MargLL[2]; 
		LastL[MargLL[3]] = MargLL[4];
		MargLL[1] = MaxI(0, LeftMarg - BPos); 
		if (MargLL[1] > 0) {
			MargLL[2] = LastL[Margll[1]];
			LastL[MargLL[1]] = (LastL[LineS] & 0xFF00) + 0x10;
		}
		MargLL[3] = MaxI(0, RightMarg - BPos);
		if (MargLL[3] > 0) {
			MargLL[4] = LastL[MargLL[3]];
			LastL[MargLL[3]] = (LastL[LineS] & 0xFF00) + 0x11;
		}
		ScrWrBuf(FirstC - 1, TxtRows - 1, LastL[1], LineS);
	}*/
}

void WrLLMargMsg(string* s, WORD n)
{
	/*if (s != nullptr) {
		MsgLine = s^; 
		WrLLMsgTxt();
	}
	else {
		if (n != 0) WrLLMsg(n);
		else {
			if (LastS != nullptr) {
				MsgLine = LastS^; 
				WrLLMsgTxt();
			}
			else {
				WrLLMsg(LastNr);
			}
			if (Mode == TextM) WriteMargins();
		}
	}*/
}

void InitScr()
{
	/*FirstR = WindMin.Y + 1; 
	FirstC = WindMin.X + 1;
	LastR = WindMax.Y + 1; 
	LastC = WindMax.X + 1;
	if ((FirstR == 1) && (Mode != HelpM)) FirstR++;
	if (LastR == TxtRows) LastR--;
	MinC = FirstC; MinR = FirstR; MaxC = LastC; MaxR = LastR;
	window(FirstC, FirstR, LastC, LastR);
	FirstR--;
	if ((Mode != HelpM) and (Mode != ViewM) and Wrap) LastC--;
	PageS = LastR - FirstR; LineS = succ(LastC - FirstC);*/
}

void UpdStatLine(int Row, int Col)
{
	string StatLine;
	string st;
	int i;
	longint lRow;

	/*if (!HelpScroll) {
		lRow = Row + Part.LineP;
		StatLine = "     1:                             ";
		str(lRow:5, st);
		move(st[1], StatLine[2], 5);
		str(Col, st);
		while (st.length < 4) { st = st + ' '; }
		move(st[1], StatLine[8], 4);
		switch (Mode) {
			case TextM:	{
				if (Insert) move(InsMsg[1], StatLine[11], 5);
				else move(nInsMsg[1], StatLine[11], 5);
				if (Indent) move(IndMsg[1], StatLine[16], 5);
				if (Wrap) move(WrapMsg[1], StatLine[21], 5);
				if (Just) move(JustMsg[1], StatLine[26], 5);
				if (TypeB == ColBlock) move(BlockMsg[1], StatLine[31], 5);
				break;
			}
			case ViewM: { move(ViewMsg[1], StatLine[11], length(ViewMsg)); break; }
			case SinFM: { StatLine[13] = '-'; break; }
			case DouFM: { StatLine[13] = '='; break; }
			case DelFM: { StatLine[13] = '/'; break; }
			default: break;
		}
		i = 1;
		if (HeadS != nullptr) {
			i = MaxW(1, pos('_', HeadS^));
			if (i > TxtCols - TStatL) i = MaxI(integer(TxtCols) - TStatL, 1);
		}
		ScrWrStr(i - 1, 0, StatLine, SysLColor);
	}*/
}

void EditWrline(ArrPtr P, int Row)
{
	WORD BuffLine[255];
	WORD Line;
	short I, LP, B, E;
	short Newvalue;
	//Nv array[1..2] of byte absolute Newvalue;
	bool IsCtrl;

	/*Line = pred(ScrL + Row);
	if (LineInBlock(Line) && (TypeB == TextBlock)) Nv[2] = BlockColor;
	else Nv[2] = TxtColor;
	I = 1;
	while ((P ^ [I] != _CR) && (I <= LineSize)) {
		Nv[1] = ord(P ^ [I]); 
		BuffLine[I] = Newvalue;
		if (Nv[1] < 32) IsCtrl = true; 
		I++;
	}
	
	LP = I - 1; 
	Nv[1] = 32;

	for (int i = LP + 1; i < BPos + LineS; i++) { BuffLine[I] = Newvalue; }
	
	if (BegBLn <= EndBLn) {
		if (LineBndBlock(Line) || ((TypeB = ColBlock) && LineInBlock(Line))) {
			if ((BegBLn = LineAbs(Line)) || (TypeB = ColBlock)) {
				B = MinI(BegBPos, LineS + BPos + 1);
			}
			else { B = 1; }
			if ((EndBLn = LineAbs(Line)) || (TypeB = ColBlock)) {
				E = MinI(EndBPos, LineS + BPos + 1);
			}
			else { E = LineS + BPos + 1; }
			for (int i = B; i < pred(E); i++) {
				BuffLine[I] = (BuffLine[I] and 0x00FF) + BlockColor shl 8;
			}
		}
	}
	if (IsCtrl) {
		for (int i = succ(BPos); i < LP; i++) {
			if (ord(P ^ [I]) < 32) {
				BuffLine[I] = ((ord(P ^ [I]) + 64) and 0x00FF) +
					ColKey[pos(char(P ^ [I]), CtrlKey)] shl 8;
			}
		}
	}
	ScrWrBuf(WindMin.X, WindMin.Y + Row - 1, BuffLine[BPos + 1], LineS);*/
}

void ScrollWrline(ArrPtr P, int Row, ColorOrd CO)
{
	string GrafCtrl = { 3,6,9,11,15,16,18,21,22,24,25,26,29,30,31 };
	WORD BuffLine[255];
	short I, J, LP, pp;
	short Newvalue;
	// Nv : array [1..2] of byte absolute Newvalue;
	bool IsCtrl;
	char cc;
	BYTE Col;

	/*Col = Color(CO);
	Nv[2]=Col; I=1; J=1; cc=P^[I];
	while ((cc != _CR) && (I <= LineSize) || !InsPage) {
		if ((cc >= #32) or (pos(cc, GrafCtrl) > 0)) {
			Nv[1] = ord(cc); BuffLine[J] = Newvalue; J++;
		}
		else {
			if (pos(cc, CtrlKey) > 0) IsCtrl = true;
			else {
				if (Scroll && (cc = ^ l)) { InsPage = InsPg; I++; }
			}
		}
		I++; 
		cc = P ^ [I];
	}
    
	LP = I - 1; 
	Nv[1] = 32;

	while (J <= BCol + LineS) { BuffLine[J] = Newvalue; J++; }
	if (IsCtrl) {
		I = 1; J = 1;
		while (I <= LP) {
			cc = P ^ [I];
			if ((cc >= #32) || (pos(cc, GrafCtrl)) > 0)
			{
				BuffLine[J] = (BuffLine[J] and 0x00FF) + Col shl 8;
				J++;
			}
			else if (pos(cc, CtrlKey) > 0)
			{
				pp = pos(cc, CO);
				if (pp > 0) CO = copy(CO, 1, pp - 1) + copy(CO, pp + 1, len - pp);
				else CO = CO + cc;
				Col = Color(CO);
			}
			else if (cc = ^ l) BuffLine[J] = 219 + Col shl 8;
			I++;
		}
		while (J <= BCol + LineS) {
			BuffLine[J] = (BuffLine[J] and 0x00FF) + Col shl 8; J++;
		}
	}
    ScrWrBuf(WindMin.X,WindMin.Y+Row-1,BuffLine[BCol+1],LineS);*/
}

BYTE Color(ColorOrd CO)
{
	if (CO=="") return TxtColor;
	return ColKey[pos(CO[len], CtrlKey)];
}

bool MyTestEvent()
{
	/*if (FirstEvent) return false;
	return TestEvent;*/
}

void UpdScreen()
{
	int r, rr;
	WORD w;
	WORD Ind;
	ColorOrd co1, co2;
	WORD oldSI;
	string PgStr;

//label1:
	/*oldSI = ScrI; InsPage = false;
	if (ChangeScr)
	{
		if (ChangePart) DekodLine(); 
		ChangeScr = false;

		if (Scroll) ScrI = LineI; 
		else ScrI = FindLine(ScrL);
		
		if (HelpScroll)
		{
			ColScr = Part.ColorP;
			SetColorOrd(ColScr, 1, ScrI);
		}
	}
	if (Scroll) // {tisk aktualniho radku}
	{
		fillchar(PgStr, 256, ord(CharPg));
		PgStr[0] = chr(255);
		co1 = ColScr; r = 1;
		while (Arr[r] == ^ l) { inc(r); }
		ScrollWrLine(addr(Arr[r]), 1, co1);
	}
	else if (Mode == HelpM)
	{
		co1 = Part.ColorP; SetColorOrd(co1, 1, LineI);
		ScrollWrLine(addr(Arr), LineL - ScrL + 1, co1);
	}
    else EditWrline(addr(Arr),LineL-ScrL+1);
	WrEndL(HardL, LineL - ScrL + 1);
    if (MyTestEvent) return;
	Ind = ScrI; r = 1; rr = 0; w = 1; InsPage = false; co2 = ColScr;
	if (Scroll) { while (T ^ [Ind] == ^ l) { inc(Ind); } }
	do {
		if (MyTestEvent) return;                   // {tisk celeho okna}
		if ((Ind >= LenT) && !AllRd)
		{
			NextPartDek; dec(Ind, Part.MovI);
		}

		if (Scroll && (Ind < LenT))
			if ((InsPg && (ModPage(r - rr + RScrL - 1))) || InsPage)
			{
				EditWrline(addr(PgStr[1]), r);
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
			if (HelpScroll) ScrollWrLine(addr(T ^ [Ind]), r, co2);
			else EditWrline(addr(T ^ [Ind]), r);
			if (InsPage) Ind = FindChar(w, ^ l, Ind, LenT) + 1;
			else Ind = FindChar(w, _CR, Ind, LenT) + 1;
			WrEndL((Ind < LenT) && (T ^ [Ind] = _LF), r);
			if (T ^ [Ind] == _LF) Ind++;
		}
		else {
			EditWrline(addr(T ^ [LenT]), r);
			WrEndL(false, r);
		}

	label1:
		r++;
		if (Scroll && (T ^ [Ind] == ^ l)) { InsPage = InsPg; inc(Ind); }
	} while (r > PageS);*/
}

void WrEndL(bool Hard, int Row)
{
	/*WORD w;
	if ((Mode != HelpM) && (Mode != ViewM) && Wrap) {
		if (Hard) w = $11 + TxtColor shl 8; 
		else w = 32 + TxtColor shl 8;
		ScrWrBuf(WindMin.X + LineS, WindMin.Y + Row - 1, w, 1);
	}*/
}

void Background()
{
	WORD p;

	/*UpdStatLine(LineL, Posi); 
	if (MyTestEvent) return;
    if (HelpScroll)
	{
		p = Posi;
		if (Mode == HelpM) if (WordL = LineL) while (Arr[p + 1]<>^ q) { p++; }
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
	gotoxy(Posi - BPos, succ(LineL - ScrL));
    IsWrScreen=true;*/
}
