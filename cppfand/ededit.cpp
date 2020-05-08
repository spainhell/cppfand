#pragma once

#include "ededit.h"
#include <algorithm>
#include "base.h"
#include "common.h"
#include "drivers.h"
#include "edevent.h"
#include "edevinpt.h"
#include "edevproc.h"
#include "editor.h"
#include "edscreen.h"
#include "edtextf.h"
#include "kbdww.h"
#include "keybd.h"
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

	WORD FirstR, FirstC, LastR, LastC, MinC = 0, MinR =0, MaxC=0, MaxR=0;
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
	WORD i1=0, i2=0, i3=0;

	
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
	LockPtr = nullptr; // ptr(0, 0x417);
	FirstScroll = Mode == ViewM;
	Mode = ViewM;
	bScroll = (((*LockPtr & 0x10) != 0) || FirstScroll) && (Mode != HelpM);
	if (bScroll) 
	{
		ScrL = NewL(RScrL); 
		ChangeScr = true;
	}
	HelpScroll = bScroll || (Mode == HelpM); 
	if (HelpScroll) { CrsHide(); }
	else { CrsNorm(); }
	BCol = 0; 
	BPos = 0;
	SetScreen(IndT, ScrT, Posi); 
	Konec = false;

	if (Mode == HelpM)
	{
		WordL = 0;
		ScrI = SetInd(LineI, Posi);
		if (WordFind(WordNo2() + 1, i1, i2, i3)) { SetWord(i1, i2); }
		if (!WordExist()) { SetDekLnCurrI(IndT); ScrI = 1; }
	}
	FillChar((char*)MargLL, sizeof(MargLL), 0);
	ColScr = Part.ColorP;
	WrStatusLine();
	TextAttr = TxtColor; 
	ClrScr(); 
	Background(); 
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
	WrLLMargMsg(&LastS, LastNr);

	do {
		if (TypeT == FileT) { NullChangePart(); HandleEvent(); }
		if (!(Konec || IsWrScreen)) { Background(); }
	} while (Konec);
	
	if (bScroll && (Mode != HelpM)) {
		Posi = BPos + 1; LineL = ScrL; LineI = ScrI;
	}

	IndT = SetInd(LineI, Posi); 
	ScrT = ((LineL - ScrL + 1) << 8) + Posi - BPos;
	if (Mode != HelpM) {
		TxtXY = ScrT + longint(Posi) << 16;
		CursorWord();
		if (Mode == HelpM) { ClrWord(); }
	}
	CrsHide(); 
	Window(MinC, MinR, MaxC, MaxR);
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
	if ((NextI < LenT) && (*T[NextI] == _LF)) NextI++;
	else HardL = false;
	if (LP > LineSize) {
		LP = LineSize;
		if (Mode == TextM) {
			if (PromptYN(402))
			{
				LL = LineI + LineSize;
				NullChangePart();
				TestLenText(LL, longint(LL) + 1);
				LL -= Part.MovI;
				*T[LL] = _CR;
				NextI = LineI + LP + 1;
			}
		}
		else Mode = ViewM;
	}
	if (LP > 0) Move(T[LineI], Arr, LP);
	UpdatedL = false;
}

pstring ShortName(pstring Name)
{
	WORD J;
	pstring s;
	J = Name.length();
	while (!(Name[J] == '\\' || Name[J] == ':') && (J > 0)) J--;
    s = Name.substr(succ(J),Name.length()-J);
	if (Name[2] == ':') { s = Name.substr(1, 2) + s; }
	return s;
}

WORD CountChar(char C, WORD First, WORD Last)
{
	WORD I, j, n;
	j = 1;
	I = FindChar(j, C, First, LenT);
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
	Ind--;
	while (Ind > 0) {
		if (*T[Ind] == _CR)
		{
			Ind++;
			if (*T[Ind] == _LF) Ind++;
			return Ind;
		}
		Ind--;
	}
	return 1;
}

void SetDekCurrI(WORD Ind)
{
	LineI = SetCurrI(Ind); 
	DekodLine();
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
	if (Num == 1) { result = 1; }
	else {
		J = pred(Num); I = FindChar(J, _CR, 1, LenT) + 1;
		if (*T[I] == _LF) { I++; }
		if (I > LenT)
		{
			if (AllRd) {
				Num = SetLine(LenT);
				result = SetCurrI(LenT);
			}
			else {
				NextPart();
				if (Num != LineL) Num -= Part.MovL;
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
		while ((Ind - P < Pos) && (*T[Ind] != _CR)) { Ind++; }
	}
	return Ind;
}

WORD Position(WORD c) // {PosToCol}
{
	WORD cc, p;
	cc = 1; p = 1;
	while (cc <= c)
	{
		if (Arr[p] >= ' ') cc++;
		p++;
	}
	return p - 1;
}

WORD Column(WORD p)
{
	WORD c, pp;
	if (p == 0) { return 0; }
	pp = 1; c = 1;
	while (pp <= p) {
		if (Arr[pp] >= ' ') c++;
		pp++;
	}
	if (Arr[p] >= ' ') c--; 
	return c;
	
}

WORD LastPosLine()
{
	WORD LP = 0;
	LP = LineSize; 
	while ((LP > 0) && (Arr[LP] == ' ')) { LP--; }
	return LP;
}

void KodLine()
{
	WORD LP = 0;

	LP = LastPosLine() + 1;
	if (HardL) LP++; 
	TestLenText(NextI, longint(LineI) + LP);
	Move(Arr, T[LineI], LP); 
	NextI = LineI + LP; 
	LP = NextI - 1;
	if (HardL) LP--; 
	*T[LP] = _CR; 
	if (HardL) *T[LP + 1] = _LF;
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
	if ((NextI >= LenT) && !AllRd) NextPartDek();
	if (NextI <= LenT) {
		LineI = NextI; DekodLine(); LineL++;
		if (bScroll) {
			if (PageS > 1) MyWriteln();
			ScrL++;
			ChangeScr = true;
			RScrL++;
			if (ModPage(RScrL)) {
				if (PageS > 1) MyWriteln();
				RScrL++;
			}
		}
		else
			if (WrScr && (LineL == ScrL + PageS)) {
				if (PageS > 1) MyWriteln();
				ScrL++;
				ChangeScr = true;
			}
	}
}

void MyWriteln() 
{
	TextAttr = TxtColor;
	printf("\n");
}

bool WordExist()
{
	return (WordL >= ScrL) && (WordL < ScrL + PageS);
}

WORD WordNo(WORD I)
{
	return (CountChar(0x13, 1, MinW(LenT, I - 1)) + 1) / 2;
}

WORD WordNo2()
{
	if (WordExist()) return WordNo(SetInd(LineI, Posi));
	return WordNo(ScrI);
}

void ClrWord()
{
	WORD k, m;
	m = 1;
	k = 1;
	k = FindChar(m, 0x11, k, LenT);
	while (k < LenT) {
		*T[k] = 0x13; 
		m = 1; 
		k = FindChar(m, 0x11, k, LenT);
	}
}

bool WordFind(WORD i, WORD WB, WORD WE, WORD LI)
{
	WORD k;
	bool result = false;
	if (i == 0) return result;
	i = i * 2 - 1;
	k = FindChar(i, 0x13, 1, LenT);
	if (k >= LenT) return result;
	WB = k;
	k++;
	while (*T[k] != 0x13) { k++; } 
	if (k >= LenT) return result;
	WE = k; LI = SetLine(WB); 
	result = true;
	return result;
}

void SetWord(WORD WB, WORD WE)
{
	*T[WB] = 0x11; 
	*T[WE] = 0x11; 
	SetDekLnCurrI(WB);
	WordL = LineL; 
	Posi = WB - LineI + 1; 
	Colu = Column(Posi);
}

void CursorWord()
{
	set<char> O;
	WORD pp;

	LexWord = ""; 
	pp = Posi;
	if (Mode == HelpM) O.insert(0x11);
	else {
		O = Oddel; 
		if (O.count(Arr[pp]) > 0) pp--;
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
		ScrL = LineL - (ScrXY >> 8) + 1;
		Posi = MaxW(Posi, ScrXY & 0x00FF); 
		BPos = Posi - (ScrXY & 0x00FF);
		ChangeScr = true;
	}
	Colu = Column(Posi); BCol = Column(BPos);
	if (bScroll) {
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
			if (LineL > 10) ScrL = LineL - 10;
			else ScrL = 1;
			ChangeScr = true;
		}
	}
}

void DelEndT()
{
	if (LenT > 0) {
		ReleaseStore(T[LenT]);
		LenT--;
	}
}

void WrEndT()
{
	void* p; // var p:pointer;
	p = GetStore(1);
	LenT++;
	*T[LenT] = _CR;
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
	TestUpdFile(); 
	ChangePart = RdPredPart(); 
	MoveIdx(-1); 
	WrEndT();
}

void NextPart()
{
	TestUpdFile(); 
	ChangePart = RdNextPart(); 
	MoveIdx(1); 
	WrEndT();
}

void NextPartDek()
{
	NextPart();
	DekodLine();
}

void SetPart(longint Idx)
{
	if ((Idx > Part.PosP) && (Idx < Part.PosP + LenT) || (TypeT != FileT)) {
		return;
	}
	TestUpdFile(); 
	ReleaseStore(T); 
	RdFirstPart();
	while ((Idx > Part.PosP + Part.LenP) && !AllRd) 
	{
		ChangePart = RdNextPart();
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
	WORD* L = (WORD*)&LL; longint _size;
	_size = LL - F;
	if (F < LL) {
		if (TypeT == FileT)
		{
			SmallerPart(F, _size);
			F -= Part.MovI;
			L -= Part.MovI;
		}
		if ((StoreAvail() <= _size) || (MaxLenT <= LenT + _size)) { RunError(404); } // text prilis dlouhy, nestaci pamet
		else { GetStore(_size); }
	}
	if (LenT >= F) { Move(T[F], T[*L], succ(LenT - F)); }
	if (F >= LL) { ReleaseStore(T[LenT + _size + 1]); };
	LenT += _size; 
	SetUpdat();
}

void SmallerPart(WORD Ind, WORD FreeSize)
{
	WORD i, il, l; 
	longint lon;
	NullChangePart();
	if ((StoreAvail() > FreeSize) && (MaxLenT - LenT > FreeSize)) { exit; }
	TestUpdFile(); 
	WrEndT();
	lon = MinL(LenT + StoreAvail(), MaxLenT);
	lon -= FreeSize; 
	if (lon <= 0) { return; }
	lon -= lon >> 3;
	i = 1; il = 0; l = 0;
	
	while (i < Ind) {
		if (*T[i] == _CR) 
		{ 
			l++; il = i; 
			if (*T[il + 1] == _LF) { il++; }
		}
		if (LenT - il < lon) { i = Ind; i++; }
	}
	
	if (il > 0) 
	{
		// with Part do:
		Part.PosP += il; Part.LineP += l;
		Part.MovI = il; Part.MovL = l;
		SetColorOrd(Part.ColorP, 1, Part.MovI + 1);
		// end

		LenT -= il; 
		Move(T[il + 1], T, LenT); 
		*T[LenT] = _CR;
		ReleaseStore(T[LenT + 1]); 
		ChangePart = true;
		MoveIdx(1);
	}
		 
	Ind -= il; 
	if (LenT < lon) { return; }
	i = LenT; il = LenT;
	while (i > Ind) {
		if (*T[i] == _CR) {
			il = i;
			if (*T[il + 1] == _LF) { il++; }
		}
		i--;
		if (il < lon) { i = Ind; }
	}
	if (il < LenT) 
	{
		if (il < LenT - 1) { AllRd = false; }
		Part.LenP = il; 
		LenT = il + 1; 
		*T[LenT] = _CR; 
		ReleaseStore(T[LenT + 1]);
	}
}
