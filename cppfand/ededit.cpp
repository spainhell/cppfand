#include "ededit.h"

#include <algorithm>



#include "base.h"
#include "drivers.h"
#include "edevent.h"
#include "edevinpt.h"
#include "editor.h"
#include "kbdww.h"
#include "legacy.h"
#include "memory.h"

void Edit(WORD SuccLineSize)
{
	// {line descriptor LineI, Posi, BPos}
	ArrLine Arr;
	WORD NextI;
	int LineL, ScrL;
	longint RScrL;
	bool UpdatedL, CtrlL, HardL;
	// {screen descriptor  ScrI}
	WORD BCol, Colu, Row;
	bool ChangeScr;
	ColorOrd ColScr;
	bool IsWrScreen;

	WORD FirstR, FirstC, LastR, LastC, MinC, MinR, MaxC, MaxR;
	WORD MargLL[4];
	WORD PageS, LineS;

	bool Scroll, FirstScroll, HelpScroll;
	longint PredScLn;
	WORD PredScPos; // {pozice pred Scroll}
	BYTE FrameDir;

	WORD WordL; // {Mode=HelpM & ctrl-word is on screen}
	bool Konec;

	BYTE* LockPtr;
	// registers Regs;
	WORD i1, i2, i3;

	
	// *** zaèátek metody ***
	InitScr();
	IsWrScreen = false;
	WrEndT();
	IndT = std::min((WORD)std::max(1, (int)IndT), LenT);
	BegBLn = 1;
	EndBLn = 1;
	BegBPos = 1;
	EndBPos = 1;
	ScrL = 1;
	ScrI = 1;
	RScrL = 1;
	PredScLn = 1;
	PredScPos = 1;
	UpdPHead = false;
	if (TypeT != FileT) {
		AllRd = true; 
		AbsLenT = LenT - 1; 
		Part.LineP = 0; 
		Part.PosP = 0;
		Part.LenP = AbsLenT;
		Part.ColorP = "";
		Part.UpdP = false;
		NullChangePart(); 
		SimplePrintHead();
	}
	LockPtr = ptr(0, 0x417); 
	FirstScroll = ViewM;
	Mode = ViewM;
	Scroll = (((*LockPtr && 0x10) != 0) || FirstScroll) && (Mode != HelpM);
	if (Scroll) 
	{
		ScrL = NewL(RScrL); 
		ChangeScr = true;
	}
	HelpScroll = Scroll || (Mode == HelpM); 
	if (HelpScroll) { Drivers::CrsHide(); }
	else { Drivers::CrsNorm(); }
	BCol = 0; 
	BPos = 0;
	SetScreen(IndT, ScrT, Posi); 
	Konec = false;

	if (Mode == HelpM)
	{
		WordL = 0;
		ScrI = SetInd(LineI, Posi);
		if (WordFind(WordNo2() + 1, i1, i2, i3)) { SetWord(i1, i2); }
		if (!WordExist) { SetDekLnCurrI(IndT); ScrI = 1; }
	}
	FillChar((char*)MargLL, sizeof(MargLL), 0);
	ColScr = Part.ColorP;
	WrStatusLine;
	textattr = TxtColor; 
	clrscr; 
	Background; 
	FirstEvent = false;

	// {!!!!!!!!!!!!!!}
	if (ErrMsg != "")
	{
		SetMsgPar(ErrMsg);
		F10SpecKey = 0xffff;
		WrLLF10Msg(110);
		ClearKbdBuf();
		AddToKbdBuf(KbdChar);
	}
	FillChar((char*)MargLL, sizeof(MargLL), 0);
	WrLLMargMsg(LastS, LastNr);

	do {
		if (TypeT == FileT) { NullChangePart(); HandleEvent(); }
		if (!(Konec || IsWrScreen)) { Background; }
	} while (Konec);
	
	if (Scroll && (Mode != HelpM)) {
		Posi = BPos + 1; LineL = ScrL; LineI = ScrI;
	}

	IndT = SetInd(LineI, Posi); 
	ScrT = ((LineL - ScrL + 1) << 8) + Posi - BPos;
	if (Mode != HelpM) {
		TxtXY = ScrT + longint(Posi) << 16;
		CursorWord;
		if (Mode == HelpM) { ClrWord; }
	}
	Drivers::CrsHide(); 
	window(MinC, MinR, MaxC, MaxR);
	TestUpdFile();
}

void DekodLine()
{	
	WORD LP, LL;
	LL = 1; 
	LP = FindChar(LL, _CR, LineI, LenT) - LineI; 
	HardL = true;
	FillChar(Arr, LineSize, 32);
	NextI = LineI + LP + 1;
	if ((NextI < LenT) && (T ^ [NextI] = _LF)) inc(NextI);
	else HardL = false;
	if (LP > LineSize) {
		LP = LineSize;
		if (Mode == TextM) {
			if (PromptYN(402))
			{
				LL = LineI + LineSize;
				NullChangePart();
				TestLenText(LL, longint(LL) + 1);
				dec(LL, Part.MovI);
				T^ [LL] = _CR;
				NextI = LineI + LP + 1;
			}
		}
		else Mode = ViewM;
	}
	if (LP > 0) move(*T[LineI], Arr, LP);
	UpdatedL = false;
}

pathstr ShortName(pathstr Name)
{
	WORD J;
	pathstr s;
	J = Name.length();
	/* TODO:
	while not(Name[J]in['\',':'])and(J>0) do dec(J);
	*/
    s = copy(Name,succ(J),length(Name)-J);
	if (Name[2] == ':') { s = copy(Name, 1, 2) + s; }
	return s;
}

WORD CountChar(char C, WORD First, WORD Last)
{
	WORD I, j, n;
	j = 1;
	i = FindChar(j, C, Fist, LenT);
	n = 0;
	while (I < Last) {
		n++;
		I = FindChar(j, C, I + 1, LenT);
	}
	return n;
}

WORD SetLine(WORD Ind)
{
	return CountChar(_CR, 1, Ind) + 1;
}

WORD SetCurrI(WORD Ind)
{
	_SetCurrI = 1; 
	dec(Ind);
	while (Ind > 0) {
		if (T ^ [Ind] == _CR)
		{
			inc(Ind);
			if (T ^ [Ind] == _LF) inc(Ind);
			_SetCurrI = Ind; exit;
		}
		dec(Ind);
	}
}

void SetDekCurrI(WORD Ind)
{
	LineI = SetCurrI(Ind); 
	DekodLine;
}

void SetDekLnCurrI(WORD Ind)
{
	SetDekCurrI(Ind); 
	LineL = SetLine(LineI);
}

WORD FindLine(int Num)
{
	WORD I, J;
	WORD result;

label1:
	if (Num <= 0) {
		if (Part.PosP == 0) Num = 1;
	}
	else {
		PredPart();
		goto label1;
	}
	if (Num = 1) { result = 1; }
	else {
		J = pred(Num); I = FindChar(J, _CR, 1, LenT) + 1;
		if (T ^ [I] == _LF) { inc(I); }
		if (I > LenT)
		{
			if (AllRd) {
				Num = SetLine(LenT);
				result = SetCurrI(LenT);
			}
			else {
				NextPart();
				if (Num != LineL) dec(Num, Part.MovL);
				goto label1;
			}
		}
		else { result = I; }
	}
	return result;
}

void DekFindLine(longint Num)
{
	SetPartLine(Num); 
	LineL = Num - Part.LineP; 
	LineI = FindLine(LineL);
	DekodLine();
}

void PosDekFindLine(longint Num, WORD Pos, bool ChScr)
{
	Posi = Pos; 
	DekFindLine(Num); 
	ChangeScr = ChangeScr || ChScr;
}

WORD SetInd(WORD Ind, WORD Pos) // { line, pozice --> index}
{
	WORD P;
	P = pred(Ind);
	if (Ind < LenT) 
	{
		while ((Ind - P < Pos) && (T ^ [Ind]<>_CR)) { inc(Ind); }
	}
	return Ind;
}

WORD Position(WORD c) // {PosToCol}
{
	WORD cc, p;
	cc = 1; p = 1;
	while (cc <= c)
	{
		if (Arr[p] >= ' ') inc(cc); 
		inc(p);
	}
	return p - 1;
}

WORD Column(WORD p)
{
	WORD c, pp;
	if (p == 0) { return 0; }
	pp = 1; c = 1;
	while (pp <= p) {
		if (Arr[pp] >= ' ') inc(c);
		inc(pp);
	}
	if (Arr[p] >= ' ') dec(c); 
	return c;
	
}

WORD LastPosLine()
{
	WORD LP;
	LP = LineSize; 
	while ((LP > 0) && (Arr[LP] = ' ')) { dec(LP); }
	return LP;
}

void KodLine()
{
	WORD LP;

	LP = LastPosLine + 1;
	if (HardL) inc(LP); 
	TestLenText(NextI, longint(LineI) + LP);
	move(Arr, T ^ [LineI], LP); 
	NextI = LineI + LP; 
	LP = NextI - 1;
	if (HardL) dec(LP); 
	T^ [LP] = _CR; 
	if (HardL) T^[LP + 1] = _LF;
	UpdatedL = false;
}

void TestKod()
{
	if (UpdatedL) KodLine();
}

void NextLine(bool WrScr)
{
	bool b;
	TestKod();
	if ((NextI >= LenT) && !AllRd) NextPartDek;
	if (NextI <= LenT) {
		LineI = NextI; DekodLine; inc(LineL);
		if (Scroll) {
			if (PageS > 1) MyWriteln();
			inc(ScrL);
			ChangeScr = true;
			inc(RScrL);
			if (ModPage(RScrL)) {
				if (PageS > 1) MyWriteln();
				inc(RScrL);
			}
		}
		else
			if (WrScr && (LineL = ScrL + PageS)) {
				if (PageS > 1) MyWriteln();
				inc(ScrL);
				ChangeScr = true;
			}
	}
}

void MyWriteln() 
{
	textattr = TxtColor;
	cout << "\n";
}

bool WordExist()
{
	return (WordL >= ScrL) && (WordL < Scrl + PageS);
}

WORD WordNo(WORD I)
{
	return (CountChar(^s, 1, MinW(LenT, I - 1)) + 1) / 2;
}

WORD WordNo2()
{
	if (WordExist) return WordNo(SetInd(LineI, Posi));
	return WordNo(ScrI);
}

void ClrWord()
{
	WORD k, m;
	m = 1;
	k = 1;
	k = FindChar(m, ^ q, k, LenT);
	while (k < LenT) {
		T^ [k] = ^ s; 
		m = 1; 
		k = FindChar(m, ^ q, k, LenT);
	}
}

bool WordFind(WORD i, WORD WB, WORD WE, WORD LI)
{
	WORD k;
	bool result = false;
	if (i == 0) return result;
	i = i * 2 - 1;
	k = FindChar(i, ^ s, 1, LenT);
	if (k >= LenT) return result;
	WB = k;
	inc(k);
	while (T ^ [k]<>^ s) { inc(k); } 
	if (k >= LenT) return result;
	WE = k; LI = SetLine(WB); 
	result = true;
	return result;
}

void SetWord(WORD WB, WORD WE)
{
	T^ [WB] = ^ q; 
	T^ [WE] = ^ q; 
	SetDekLnCurrI(WB);
	WordL = LineL; 
	Posi = WB - LineI + 1; 
	Colu = Column(Posi);
}

void CursorWord()
{
	// var O:set of char;
	set<char> O;
	WORD pp;

	LexWord = ''; 
	pp = Posi;
	if (Mode == HelpM) O = [^ q];
	else {
		O = Oddel; 
		if (Arr[pp] in O) pp--;
	}
	while ((pp > 0) && !O.count(Arr[pp])) { pp--; }
	pp++;
	while ((pp <= LastPosLine()) && !O.count(Arr[pp])) {
		LexWord = LexWord + Arr[pp];
		pp++;
	}
}

void SetScreen(WORD Ind, WORD ScrXY, WORD Pos)
{
	WORD X;
	longint rl;

	SetDekLnCurrI(Ind); 
	Posi = MinI(LineSize, MaxI(MaxW(1, Pos), Ind - LineI + 1));
	if (ScrXY > 0) {
		ScrL = LineL - (ScrXY shr 8) + 1;
		Posi = MaxW(Posi, ScrXY and 0x00FF); 
		BPos = Posi - (ScrXY and 0x00FF);
		ChangeScr = true;
	}
	Colu = Column(Posi); BCol = Column(BPos);
	if (Scroll) {
		RScrL = NewRL(ScrL);
		LineL = MaxI(PHNum + 1, LineAbs(LineL)) - Part.LineP;
		rl = NewRL(LineL);
		if ((rl >= RScrL + PageS) || (rl < RScrL)) {
			if (rl > 10) RScrL = rl - 10;
			else RScrL = 1;
			ChangeScr = true; ScrL = NewL(RScrL);
		}
		LineL = ScrL; DekFindLine(LineAbs(LineL));
	}
	else {
		if ((LineL >= ScrL + PageS) || (LineL < ScrL)) {
			if (LineL > 10) then ScrL = LineL - 10;
			else ScrL = 1;
			ChangeScr = true;
		}
	}
}

void DelEndT()
{
	if (LenT > 0) {
		ReleaseStore(@T^[LenT]);
		dec(LenT);
	}
}

void WrEndT()
{
	void* p; // var p:pointer;
	p = GetStore(1);
	LenT++;
	T^[LenT] = _CR;
}

void MoveIdx(int dir)
{
	WORD mi, ml;
	mi = -dir * Part.MovI; 
	ml = -dir * Part.MovL;
	ScrI += mi; LineI += mi; // {****GLOBAL***}
	NextI += mi; LineL += ml; ScrL += ml; // {****Edit***}
}

void TestUpdFile()
{
	DelEndT();
	if (Part.UpdP) { UpdateFile(); }
}

void SetUpdat()
{
	UpdatT = true;
	if (TypeT == FileT) {
		if (Part.PosP < 0x400) {
			UpdPHead = true;
			Part.UpdP = true;
		}
	}
}

void PredPart()
{
	TestUpdFile; 
	ChangePart = RdPredPart; 
	MoveIdx(-1); 
	WrEndT();
}

void NextPart()
{
	TestUpdFile(); 
	ChangePart = RdNextPart; 
	MoveIdx(1); 
	WrEndT();
}

void NextPartDek()
{
	NextPart();
	DekodLine();
}

void SetPart(longint Idex)
{
	if ((Idx > Part.PosP) && (Idx < Part.PosP + LenT) || (TypeT != FileT)) {
		exit;
	}
	TestUpdFile(); 
	ReleaseStore(@T^); 
	RdFirstPart();
	while ((Idx > Part.PosP + Part.LenP) && !AllRd) 
	{
		ChangePart = RdNextPart;
	}
	WrEndT();
}

void SetPartLine(longint Ln)
{
	while ((Ln <= Part.LineP) && (Part.PosP > 0)) { PredPart(); }
	while ((Ln - Part.LineP > 0x7FFF) && !AllRd) { NextPart(); }
}

longint LineAbs(int Ln)
{
	return Part.LineP + Ln;
}

bool LineInBlock(int Ln)
{
	if ((LineAbs(Ln) > BegBLn) && (LineAbs(Ln) < EndBLn)) 
	{
		return true;
	}
	else { return false; }
}

bool LineBndBlock(int Ln)
{
	if ((LineAbs(Ln) == BegBLn) || (LineAbs(Ln) == EndBLn)) {
		return true;
	}
	else { return false; }
}

longint NewRL(int Line)
{
	return LineAbs(Line);
}

int NewL(longint RLine)
{
	return RLine - Part.LineP;
}

bool ModPage(longint RLine)
{
	return false;
}

void TestLenText(WORD F, longint LL)
{
	WORD L, /*absolute*/ LL; longint _size;
	_size = LL - F;
	if (F < LL) {
		if (TypeT == FileT)
		{
			SmallerPart(F, _size);
			dec(F, Part.MovI);
			dec(L, Part.MovI);
		}
		if ((StoreAvail <= _size) || (MaxLenT <= LenT + size)) { RunError(404); } // text prilis dlouhy, nestaci pamet
		else { GetStore(_Size); }
	}
	if (LenT >= F) { move(T ^ [F], T ^ [L], succ(LenT - F) };
	if (F >= LL) { ReleaseStore(@T ^ [LenT + size + 1]) };
	inc(LenT, _size); 
	SetUpdat();
}

void SmallerPart(WORD Ind, WORD FreeSize)
{
	WORD i, il, l; 
	longint lon;
	NullChangePart();
	if ((StoreAvail > FreeSize) && (MaxLenT - LenT > FreeSize)) { exit; }
	TestUpdFile(); 
	WrEndT();
	lon = MinL(LenT + StoreAvail, MaxLenT);
	dec(lon, FreeSize); 
	if (lon <= 0) { exit; }
	dec(lon, lon shr 3);
	i = 1; il = 0; l = 0;
	
	while (i < Ind) {
		if (T^ [i] == _CR) 
		{ 
			inc(l); il = i; 
			if (T ^ [il + 1] == _LF) { inc(il); }
		}
		if (LenT - il < lon) { i = Ind; inc(i); }
	}
	
	if (il > 0) 
	{
		// with Part do:
		inc(PosP, il); inc(LineP, l); 
		MovI = il; MovL = l;
		SetColorOrd(ColorP, 1, MovI + 1);
		// end

		dec(LenT, il); 
		move(T ^ [il + 1], T^, LenT); 
		T^ [LenT]: = _CR;
		ReleaseStore(@T ^ [LenT + 1]); 
		ChangePart = true;
		MoveIdx(1);
	}
		 
	dec(Ind, il); 
	if (LenT < lon) { exit; }
	i = LenT; il = LenT;
	while (i > Ind) {
		if (T^ [i] == _CR) {
			il = i;
			if (T ^ [il + 1] == _LF) { inc(il); }
		}
		dec(i);
		if (il < lon) { i = Ind; }
	}
	if (il < LenT) 
	{
		if (il < LenT - 1) { AllRd = false; }
		Part.LenP = il; 
		LenT = il + 1; 
		T^ [LenT] = _CR; 
		ReleaseStore(@T ^ [LenT + 1]);
	}
}



