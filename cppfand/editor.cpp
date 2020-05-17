#pragma once

#include "editor.h"
#include "legacy.h"
#include "printtxt.h"
#include "genrprt.h"
#include "oaccess.h"
#include "obase.h"
#include "rdrun.h"
#include "runedi.h"
#include "runproc.h"
#include "wwmenu.h"
#include "wwmix.h"
#include <set>
#include "compile.h"
#include "obaseww.h"
#include "runfrml.h"

const int TXTCOLS = 80;
const int SuccLineSize = 256;
longint Timer = 0;
bool Insert, Indent, Wrap, Just;

// PROMENNE
bool InsPage;
typedef pstring ColorOrd;

struct Character {
	char ch = 0;
	BYTE color = 0;
};

char Arr[SuccLineSize];
WORD NextI;
integer LineL, ScrL;
longint RScrL;
bool UpdatedL, CtrlL, HardL;
WORD BCol, Colu, Row;
bool ChangeScr;
ColorOrd ColScr;
bool IsWrScreen;

WORD FirstR, FirstC, LastR, LastC, MinC, MinR, MaxC, MaxR;
WORD MargLL[4];
WORD PageS, LineS;
bool bScroll, FirstScroll, HelpScroll;
longint PredScLn;
WORD PredScPos;
BYTE FrameDir;
WORD WordL;
bool Konec;

const BYTE InterfL = 4; /*sizeof(Insert+Indent+Wrap+Just)*/

const BYTE LineSize = 255; const WORD TextStore = 0x1000;
const BYTE TStatL = 35; /*=10(Col Row)+length(InsMsg+IndMsg+WrapMsg+JustMsg+BlockMsg)*/

std::set<char> Oddel = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,58,59,60,61,62,63,64,
91,92,93,94,96,123,124,125,126,127 };

const WORD _QY_ = 0x1119; const WORD _QL_ = 0x110C; const WORD _QK_ = 0x110B;
const WORD _QB_ = 0x1102; const WORD _QI_ = 0x1109; const WORD _QF_ = 0x1106;
const WORD _QE_ = 0x1105; const WORD _QX_ = 0x1118; const WORD _QA_ = 0x1101;
const WORD _framesingle_ = 0x112D; const WORD _framedouble_ = 0x113D;
const WORD _delframe_ = 0x112F; const WORD _frmsin_ = 0x2D;
const WORD _frmdoub_ = 0x3D; const WORD _dfrm_ = 0x2F; const WORD _nfrm_ = 0x20;
const WORD _KB_ = 0x0B02; const WORD _KK_ = 0x0B0B; const WORD _KH_ = 0x0B08;
const WORD _KS_ = 0x0B13; const WORD _KY_ = 0x0B19; const WORD _KC_ = 0x0B03;
const WORD _KV_ = 0x0B16; const WORD _KW_ = 0x0B17; const WORD _KR_ = 0x0B12;
const WORD _KP_ = 0x0B10; const WORD _KN_ = 0x0B0E; const WORD _KU_ = 0x0B15;
const WORD _KL_ = 0x0B0C; const WORD _OW_ = 0x0F17; const WORD _OL_ = 0x0F0C;
const WORD _OR_ = 0x0F12; const WORD _OJ_ = 0x0F0A; const WORD _OC_ = 0x0F03;
const WORD _KF_ = 0x0B06;

const BYTE CountC = 7;
pstring CtrlKey = "\x13\x17\x11\x04\x02\x05\x01";
const bool ColBlock = true;
const bool TextBlock = false;

// {**********global param begin for SavePar}  // r85
char Mode;
char TypeT;
pstring NameT;
pstring ErrMsg;
WORD MaxLenT, LenT, IndT, ScrT;
pstring Breaks;
EdExitD* ExitD = nullptr;
bool SrchT, UpdatT;
WORD LastNr, CtrlLastNr;
integer LeftMarg, RightMarg;
bool TypeB;
pstring LastS, CtrlLastS, ShiftLastS, AltLastS, HeadS;
longint* LocalPPtr;
bool EditT;

// od r101
BYTE ColKey[CountC + 1];
BYTE TxtColor, BlockColor, SysLColor;
pstring InsMsg, nInsMsg, IndMsg, WrapMsg, JustMsg, BlockMsg;
pstring ViewMsg;
char CharPg;
bool InsPg;
longint BegBLn, EndBLn;
WORD BegBPos, EndBPos;
WORD ScrI, LineI, Posi, BPos; // {screen status}
pstring FindStr, ReplaceStr;
bool Replace;
pstring OptionStr;
bool FirstEvent;
WORD PHNum, PPageS; // {strankovani ve Scroll}
struct PartDescr
{
	longint PosP = 0; longint LineP = 0;
	WORD LenP = 0, MovI = 0, MovL = 0;
	bool UpdP = false;
	ColorOrd ColorP;
} Part;
FILE* TxtFH;
pstring TxtPath;
pstring TxtVol;
bool AllRd;
longint AbsLenT;
bool ChangePart, UpdPHead;
char T[50];
longint SavePar(); // r133
void RestorePar(longint l);

// ***********HELP**********  // r351
const BYTE maxStk = 15; WORD iStk = 0;
struct structStk { RdbDPtr Rdb; FileDPtr FD; WORD iR, iT; } Stk[maxStk];
void ViewHelpText(LongStr* S, WORD& TxtPos);



longint SavePar()
{
	LongStr* sp; WORD len = 0;
	// len = InterfL + ofs(T) - ofs(Mode) + 4;
	sp = (LongStr*)GetStore(len + 2);
	sp->LL = len;
	Move(&Insert, &sp->A, InterfL);
	Move(&Mode, &sp->A[InterfL + 1], len - InterfL);
	auto result = StoreInTWork(sp);
	ReleaseStore(sp);
	return result;
}

void RestorePar(longint l)
{
	LongStr* sp;
	sp = ReadDelInTWork(l);
	Move(sp->A, &Insert, InterfL);
	Move(&sp->A[InterfL + 1], &Mode, sp->LL - InterfL);
	ReleaseStore(sp);
}

FrmlPtr RdFldNameFrmlT(char& FTyp)
{
	Error(8);
	return nullptr;
}

void MyWrLLMsg(pstring s)
{
	if (HandleError == 4) s = ""; SetMsgPar(s); WrLLF10Msg(700 + HandleError);
}

void MyRunError(pstring s, WORD n)
{
	SetMsgPar(s); RunError(n);
}

void HMsgExit(pstring s)
{
	switch (HandleError) {
	case 0: return;
	case 1: { s = s[1]; SetMsgPar(s); RunError(700 + HandleError); break; }
	case 2:
	case 3: { SetMsgPar(s); RunError(700 + HandleError); break; }
	case 4: RunError(704); break;
	}
}

WORD FindChar(WORD& Num, char C, WORD Pos, WORD Len)
{
	for (WORD i = Pos; i < Len; i++) if (T[i] == C) { Num = i;  return i; }
	return 0;
}

bool TestOptStr(char c)
{
	return (OptionStr.first(c) != 0) || (OptionStr.first(toupper(c)) != 0);
}

WORD FindOrdChar(char C, WORD Pos, WORD Len)
{
	WORD I, K; char cc;
	I = Len; K = Pos - 1; cc = C;
	// TODO: ASM
	return Len - I;
}

WORD FindUpcChar(char C, WORD Pos, WORD Len)
{
	WORD I, K; char cc;
	I = Len; K = Pos - 1; cc = C;
	// TODO: ASM
	return Len - I;
}

bool SEquOrder(pstring S1, pstring S2)
{
	integer i;
	if (S1.length() != S2.length()) return false;
	for (i = 1; i < S1.length(); i++)
		if (CharOrdTab[S1[i]] != CharOrdTab[S2[i]]) return false;
	return true;
}

bool FindString(WORD& I, WORD Len)
{
	WORD j, i1;
	pstring s1, s2;
	char c;
	auto result = false;
	c = FindStr[1];
	if (FindStr != "")
	{
	label1:
		if (TestOptStr('~')) i1 = FindOrdChar(c, I, Len);
		else if (TestOptStr('u')) i1 = FindUpcChar(c, I, Len);
		else { j = 1; i1 = FindChar(j, c, I, Len); }
		I = i1;
		if (I + FindStr.length() > Len) return result; s2 = FindStr;
		Move(&T[I], &s1[1], FindStr.length()); s1[0] = FindStr.length();
		if (TestOptStr('~'))
		{
			if (!SEquOrder(s1, s2))
			{
				I++; goto label1;
			}
		}
		else if (TestOptStr('u'))
		{
			if (!SEquUpcase(s1, s2))
			{
				I++; goto label1;
			}
		}
		else if (s1 != s2) { I++; goto label1; }
		if (TestOptStr('w'))
			if (I > 1 && !Oddel.count(T[I - 1]) || !Oddel.count(T[I + FindStr.length()]))
			{
				I++;
				goto label1;
			}
		result = true; I += FindStr.length();
	}
	return result;
}

WORD FindCtrl(WORD F, WORD L)
{
	WORD I, K;
	I = L; K = F - 1;
	// ASM
	return L - I + 1;
}

void SetColorOrd(ColorOrd CO, WORD First, WORD Last)
{
	WORD I, pp;
	BYTE* len = (BYTE*)&CO;
	I = FindCtrl(First, Last);
	while (I < Last)
	{
		pp = CO.first(T[I]);
		if (pp > 0) CO = CO.substr(1, pp - 1) + CO.substr(pp + 1, *len - pp);
		else  CO = CO + T[I];
		I = FindCtrl(I + 1, Last);
	}
}

void SimplePrintHead()
{
	pstring ln;
	PHNum = 0; PPageS = 0x7FFF;
}

void LastLine(char* input, WORD from, WORD num, WORD& Ind, WORD& Count)
{
	WORD length = Count;
	WORD lastCR = 0;
	Count = 0;
	for (int i = from; i < length; i++)
	{
		if (input[i] == _CR) { lastCR = i; Count++; }
	}
	Count++;
	Ind = lastCR + 1;
}

bool RdNextPart()
{
	char* ppa = nullptr;
	WORD L11, L1;
	longint Max = MinL(MaxLenT, StoreAvail() + LenT);
	WORD Pass = Max - (Max >> 3);
	Part.MovL = 0;
	Part.MovI = 0;
	WORD MI = 0;
	auto result = false;
	if (AllRd) return result;
	if (Part.LenP == 0) { LenT = 0; /*T = (CharArr*)GetStore(0);*/ }
	longint Pos = Part.PosP;
	longint BL = Part.LineP;

	if (LenT > (Pass >> 1)) {
		LastLine(ppa, 0, LenT - (Pass >> 1), MI, Part.MovL);     /* 28kB+1 radek*/
		SetColorOrd(Part.ColorP, 1, MI + 1);
		Pos += MI;
		LenT -= MI;
		Move(&T[MI + 1], &T[1], LenT);
		ReleaseStore(&T[LenT + 1]);
		Part.LineP = BL + Part.MovL;
		Part.PosP = Pos;
		Part.MovI = MI;
	}
	Pos += LenT;
	longint FSize = FileSizeH(TxtFH);
	AllRd = false;
	WORD iL = LenT;

	do {
		longint Rest = FSize - Pos;
		if (Rest > 0x1000) L11 = 0x1000;
		else L11 = Rest;
		Max = StoreAvail();
		if (Max > 0x400) Max -= 0x400;
		if (L11 > Max) L11 = Max;
		ppa = new char[L11];
		L1 = L11;
		if (L1 > 0) { SeekH(TxtFH, Pos); ReadH(TxtFH, L1, ppa); }
		AllRd = Pos + L11 >= FSize;
		LenT += L1;
		Pos += L1;
	} while (!((LenT > Pass) || AllRd || (L11 == Max)));

	LastLine(ppa, iL, LenT - iL - 1, iL, L1);
	if (AllRd) iL = LenT;
	if ((iL < LenT)) { LenT = iL; AllRd = false; }
	Part.LenP = LenT;
	Part.UpdP = false;
	if ((T[LenT] == 0x1A) && AllRd) LenT--;
	if ((LenT <= 1)) return result;  /*????????*/
	if (LenT < 49) ReleaseStore(&T[LenT + 1]); // TODO: pùvodnì ReleaseStore(@T^[succ(LenT)]);
	result = true;
	return result;
}

void FirstLine(WORD from, WORD num, WORD& Ind, WORD& Count)
{
	char* C = nullptr;
	WORD* COfs = (WORD*)C;
	WORD i;
	Count = 0; Ind = from - 1; C = &T[from];
	for (i = 0; i < num - 1; i++)
	{
		COfs--; if (*C == _CR) { Count++; Ind = from - i; };
	}
	if ((Count > 0) && (T[Ind + 1] == _LF)) Ind++;
}

bool RdPredPart()
{
	CharArr* ppa;
	WORD L1, L11, MI;
	longint BL, FSize, Rest, Max, Pos;
	WORD Pass;
	Max = MinL(MaxLenT, StoreAvail() + LenT);
	Pass = Max - (Max >> 3);
	Part.MovL = 0; MI = 0;
	auto result = false;
	if (Part.PosP == 0) return result;
	Pos = Part.PosP; BL = Part.LineP;
	if (LenT <= (Pass >> 1)) goto label1;
	FirstLine(LenT + 1, LenT - (Pass >> 1), L1, L11);
	if (L1 < LenT)
	{
		AllRd = false; LenT = L1; ReleaseStore(&T[LenT + 1]);
	}

label1:
	L11 = LenT;
	do {
		if (Pos > 0x1000) L1 = 0x1000;
		else L1 = Pos;
		Max = StoreAvail();
		if (Max > 0x400) Max -= 0x400;
		if (L1 > Max) L1 = Max;
		ppa = (CharArr*)GetStore(L1);
		Move(&T[0], &T[L1 + 1], LenT);
		if (L1 > 0)
		{
			SeekH(TxtFH, Pos - L1); ReadH(TxtFH, L1, T);
		}
		LenT += L1; Pos -= L1;
	} while (!((LenT > Pass) || (Pos == 0) || (L1 == Max)));

	L11 = LenT - L11; FirstLine(L11 + 1, L11, MI, Part.MovL);
	if (Pos == 0) MI = L11;
	else if (Part.MovL > 0) { Part.MovL--; MI = L11 - MI; }
	L1 = L11 - MI; LenT -= L1; Pos += L1;
	if (L1 > 0)
	{
		Move(&T[L1 + 1], T, LenT);
		ReleaseStore(&T[LenT + 1]);
	}
	/* !!! with Part do!!! */
	Part.PosP = Pos; Part.LineP = BL - Part.MovL; Part.LenP = LenT;
	Part.MovI = MI; Part.UpdP = false;
	SetColorOrd(Part.ColorP, 1, MI + 1);
	if ((LenT == 0)) return result;  /*????????*/
	result = true;
	return result;
}

void UpdateFile()
{
	WORD n, HErr; longint d, pos1, pos;  void* p;
	n = 0xFFF0;
	if (n > StoreAvail()) n = StoreAvail();
	p = GetStore(n);
	d = longint(LenT) - Part.LenP;
	HErr = 0;
	pos1 = Part.PosP + Part.LenP;
	if (d > 0)
	{
		pos = AbsLenT;
		while (pos > pos1) {
			if (pos - pos1 < n) n = pos - pos1; pos -= n;
			SeekH(TxtFH, pos); ReadH(TxtFH, n, p);
			SeekH(TxtFH, pos + d); WriteH(TxtFH, n, p);
			if (HandleError != 0) HErr = HandleError;
		}
	}
	else if (d < 0)
	{
		pos = pos1;
		while (pos < AbsLenT)
		{
			if (pos + n > AbsLenT) n = AbsLenT - pos;
			SeekH(TxtFH, pos);
			ReadH(TxtFH, n, p);
			SeekH(TxtFH, pos + d);
			WriteH(TxtFH, n, p); pos += n;
		}
		TruncH(TxtFH, AbsLenT + d);
	}
	ReleaseStore(p);
	SeekH(TxtFH, Part.PosP);
	if (LenT > 0) WriteH(TxtFH, LenT, T);
	if (HandleError != 0) HErr = HandleError;
	FlushH(TxtFH); AbsLenT = FileSizeH(TxtFH);
	if (HErr != 0) { SetMsgPar(TxtPath); WrLLF10Msg(700 + HErr); }
	Part.UpdP = false; Part.LenP = LenT;
	if ((Part.PosP < 400) && (Part.LenP > 0x400)) UpdPHead = false;
}

void RdPart()
{
	LenT = Part.LenP;
	//T = (CharArr*)GetStore(LenT);
	if (LenT == 0) return;
	SeekH(TxtFH, Part.PosP);
	ReadH(TxtFH, LenT, T);
}

void NullChangePart()
{
	ChangePart = false; Part.MovI = 0; Part.MovL = 0;
}

void RdFirstPart()
{
	NullChangePart();
	Part.PosP = 0; Part.LineP = 0; Part.LenP = 0; Part.ColorP = "";
	AllRd = false; ChangePart = RdNextPart();
}

void OpenTxtFh(char Mode)
{
	FileUseMode UM;
	CPath = TxtPath; CVol = TxtVol;
	TestMountVol(CPath[1]);
	if (Mode == ViewM) UM = RdOnly;
	else UM = Exclusive;
	TxtFH = OpenH(_isoldnewfile, UM);
	if (HandleError != 0) {
		SetMsgPar(CPath);
		RunError(700 + HandleError);
	}
	AbsLenT = FileSizeH(TxtFH);
}

pstring ShortName(pstring Name)
{
	WORD J;
	pstring s;
	J = Name.length();
	while (!(Name[J] == '\\' || Name[J] == ':') && (J > 0)) J--;
	s = Name.substr(J, Name.length() - J);
	if (Name[2] == ':') { s = Name.substr(0, 2) + s; }
	return s;
}

/// POZOR na 'Len:byte absolute Blanks;'
//  http://home.pf.jcu.cz/~edpo/program/kap08.html
void WrStatusLine()
{
	pstring Blanks;
	pstring s;
	integer i;

	if (Mode != HelpM) {
		FillChar(&Blanks[1], TxtCols, 32);
		Blanks[0] = TXTCOLS;
		if (HeadS.length() > 0) {
			Move(&HeadS[1], &Blanks[1], HeadS.length());
			i = Blanks.first('_');
			if (i == 0) {
				Move(&Blanks[1], &Blanks[TStatL + 3], 252 - TStatL);
				FillChar(&Blanks[1], TStatL + 2, 32);
			}
			else {
				while ((i <= Blanks.length()) && (Blanks[i] == ' ')) {
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
		screen.ScrWrStr(0, 0, Blanks, SysLColor);
	}
}

void WriteMargins()
{
	WORD LastL[201];

	if ((Mode != HelpM) && (Mode != ViewM) && Wrap) {
		screen.ScrRdBuf(FirstC - 1, TxtRows - 1, &LastL[1], LineS);
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
		screen.ScrWrBuf(FirstC - 1, TxtRows - 1, &LastL[1], LineS);
	}
}

void WrLLMargMsg(pstring* s, WORD n)
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
	FirstR = WindMin.Y; // +1;
	FirstC = WindMin.X; // +1;
	LastR = WindMax.Y; // +1;
	LastC = WindMax.X; // +1;
	if ((FirstR == 0) && (Mode != HelpM)) FirstR++;
	if (LastR + 1 == TxtRows) LastR--;
	MinC = FirstC; MinR = FirstR; MaxC = LastC; MaxR = LastR;
	screen.Window(FirstC, FirstR, LastC, LastR);
	FirstR--;
	if ((Mode != HelpM) and (Mode != ViewM) and Wrap) LastC--;
	PageS = LastR - FirstR; LineS = succ(LastC - FirstC);
}

void UpdStatLine(int Row, int Col)
{
	pstring StatLine;
	StatLine[0] = 35;
	pstring st;
	st[0] = 10;
	integer i;
	longint lRow;

	if (!HelpScroll) {
		lRow = Row + Part.LineP;
		StatLine = "     1:                             ";
		str(lRow, 5, st);
		Move(&st[1], &StatLine[2], 5);
		str(Col, st);
		while (st.length() < 4) { st.Append(' '); }
		Move(&st[1], &StatLine[8], 4);
		switch (Mode) {
		case TextM: {
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
		if (HeadS.length() > 0) {
			i = MaxW(1, HeadS.first('_'));
			if (i > TxtCols - TStatL) i = MaxI(integer(TxtCols) - TStatL, 1);
		}
		screen.ScrWrStr(i - 1, 0, StatLine, SysLColor);
	}
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

void EditWrline(char* P, int Row)
{
	WORD BuffLine[255] { 0 };
	WORD Line = 0;
	integer I = 0, LP = 0, B = 0, E = 0;

	BYTE nv1 = 0;
	BYTE nv2 = 0;
	
	//integer Newvalue = 0;
	//BYTE* Nv = (BYTE*)&Newvalue;
	bool IsCtrl = false;

	Line = pred(ScrL + Row);
	if (LineInBlock(Line) && (TypeB == TextBlock)) nv2 = BlockColor;
	else nv2 = TxtColor;
	I = 1;
	while ((P[I] != _CR) && (I < LineSize)) {
		nv1 = P[I];
		if (I < 0 || I > 254) throw std::exception("Index");
		BuffLine[I] = (nv2 << 8) + nv1;
		if (nv1 < 32) IsCtrl = true;
		I++;
	}

	LP = I - 1;
	nv1 = 32;

	for (I = LP + 1; I < BPos + LineS; I++)
	{
		if (I < 0 || I > 254) throw std::exception("Index");
		BuffLine[I] = (nv2 << 8) + nv1;
	}

	if (BegBLn <= EndBLn) {
		if (LineBndBlock(Line) || ((TypeB == ColBlock) && LineInBlock(Line))) {
			if ((BegBLn == LineAbs(Line)) || (TypeB == ColBlock)) {
				B = MinI(BegBPos, LineS + BPos + 1);
			}
			else { B = 1; }
			if ((EndBLn == LineAbs(Line)) || (TypeB == ColBlock)) {
				E = MinI(EndBPos, LineS + BPos + 1);
			}
			else { E = LineS + BPos + 1; }
			for (I = B; I < pred(E); I++) {
				if (I < 0 || I > 254) throw std::exception("Index");
				BuffLine[I] = (BuffLine[I] & 0x00FF) + (BlockColor << 8);
			}
		}
	}
	if (IsCtrl) {
		for (I = succ(BPos); I < LP; I++) {
			if (P[I] < 32) {
				if (I < 0 || I > 254) throw std::exception("Index");
				BuffLine[I] = ((P[I] + 64) & 0x00FF) + (ColKey[CtrlKey.first(P[I])] << 8);
			}
		}
	}
	screen.ScrWrBuf(WindMin.X, WindMin.Y + Row - 1, &BuffLine[BPos + 1], LineS);
}

BYTE Color(ColorOrd CO)
{
	BYTE* len = (BYTE*)&CO;
	if (CO == "") return TxtColor;
	return ColKey[CtrlKey.first(CO[*len])];
}

void ScrollWrline(char* P, int Row, ColorOrd CO)
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
	Nv[2] = Col; I = 1; J = 1; cc = P[I];
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
	screen.ScrWrBuf(WindMin.X, WindMin.Y + Row - 1, &BuffLine[BCol + 1], LineS);
}

WORD PColumn(WORD w, char* P)
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

void DelEndT()
{
	if (LenT > 0) {
		ReleaseStore(&T[LenT]);
		LenT--;
	}
}

void TestUpdFile()
{
	DelEndT();
	if (Part.UpdP) { UpdateFile(); }
}

void WrEndT()
{
	void* p; // var p:pointer;
	p = GetStore(1);
	LenT++;
	T[LenT] = _CR;
}

void MoveIdx(int dir)
{
	WORD mi, ml;
	mi = -dir * Part.MovI;
	ml = -dir * Part.MovL;
	ScrI += mi; LineI += mi; // {****GLOBAL***}
	NextI += mi; LineL += ml; ScrL += ml; // {****Edit***}
}

void PredPart()
{
	TestUpdFile();
	ChangePart = RdPredPart();
	MoveIdx(-1);
	WrEndT();
}

WORD CountChar(char C, WORD First, WORD Last)
{
	return 1;
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
		if (T[Ind] == _CR)
		{
			Ind++;
			if (T[Ind] == _LF) Ind++;
			return Ind;
		}
		Ind--;
	}
	return 1;
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
		if (T[i] == _CR)
		{
			l++; il = i;
			if (T[il + 1] == _LF) { il++; }
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
		Move(&T[il + 1], T, LenT);
		T[LenT] = _CR;
		ReleaseStore(&T[LenT + 1]);
		ChangePart = true;
		MoveIdx(1);
	}

	Ind -= il;
	if (LenT < lon) { return; }
	i = LenT; il = LenT;
	while (i > Ind) {
		if (T[i] == _CR) {
			il = i;
			if (T[il + 1] == _LF) { il++; }
		}
		i--;
		if (il < lon) { i = Ind; }
	}
	if (il < LenT)
	{
		if (il < LenT - 1) { AllRd = false; }
		Part.LenP = il;
		LenT = il + 1;
		T[LenT] = _CR;
		ReleaseStore(&T[LenT + 1]);
	}
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
	if (LenT >= F) { Move(&T[F], &T[*L], succ(LenT - F)); }
	if (F >= LL) { ReleaseStore(&T[LenT + _size + 1]); };
	LenT += _size;
	SetUpdat();
}

void DekodLine()
{
	WORD LP, LL;
	LL = 1;
	LP = FindChar(LL, _CR, LineI, LenT) - LineI;
	HardL = true;
	FillChar(Arr, LineSize, 32);
	NextI = LineI + LP + 1;
	if ((NextI < LenT) && (T[NextI] == _LF)) NextI++;
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
				T[LL] = _CR;
				NextI = LineI + LP + 1;
			}
		}
		else Mode = ViewM;
	}
	if (LP > 0) Move(&T[LineI], Arr, LP);
	UpdatedL = false;
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

WORD SetInd(WORD Ind, WORD Pos) // { line, pozice --> index}
{
	WORD P;
	P = pred(Ind);
	if (Ind < LenT)
	{
		while ((Ind - P < Pos) && (T[Ind] != _CR)) { Ind++; }
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

void NextPart()
{
	TestUpdFile();
	ChangePart = RdNextPart();
	MoveIdx(1);
	WrEndT();
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
		if (T[I] == _LF) { I++; }
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

void WrEndL(bool Hard, int Row)
{
	WORD w;
	if ((Mode != HelpM) && (Mode != ViewM) && Wrap) {
		if (Hard) w = 0x11 + (TxtColor << 8);
		else w = 32 + (TxtColor << 8);
		screen.ScrWrBuf(WindMin.X + LineS, WindMin.Y + Row - 1, &w, 1);
	}
}

void NextPartDek()
{
	NextPart();
	DekodLine();
}

bool ModPage(longint RLine)
{
	return false;
}

void UpdScreen()
{
	integer r, rr;
	WORD w;
	WORD Ind;
	ColorOrd co1, co2;
	WORD oldSI;
	pstring PgStr;

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
		FillChar(&PgStr[0], 255, CharPg);
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
	else EditWrline(Arr, LineL - ScrL + 1);
	WrEndL(HardL, LineL - ScrL + 1);
	if (MyTestEvent()) return;
	Ind = ScrI; r = 1; rr = 0; w = 1; InsPage = false; co2 = ColScr;
	if (bScroll) { while (T[Ind] == 0x0C) { Ind++; } }
	do {
		if (MyTestEvent()) return;                   // {tisk celeho okna}
		if ((Ind >= LenT) && !AllRd)
		{
			NextPartDek(); Ind -= Part.MovI;
		}

		if (bScroll && (Ind < LenT))
			if ((InsPg && (ModPage(r - rr + RScrL - 1))) || InsPage)
			{
				EditWrline((char*)PgStr[1], r);
				WrEndL(false, r);
				if (InsPage) rr++;
				InsPage = false;
				goto label1;
			}
		if (!bScroll && (Ind == LineI)) {
			Ind = NextI;
			co2 = co1;
			goto label1;
		}
		if (Ind < LenT) {
			if (HelpScroll) ScrollWrline((char*)&T[Ind], r, co2);
			else EditWrline((char*)&T[Ind], r);
			if (InsPage) Ind = FindChar(w, 0x0C, Ind, LenT) + 1;
			else Ind = FindChar(w, _CR, Ind, LenT) + 1;
			WrEndL((Ind < LenT) && (T[Ind] == _LF), r);
			if (T[Ind] == _LF) Ind++;
		}
		else {
			EditWrline((char*)&T[LenT], r);
			WrEndL(false, r);
		}

	label1:
		r++;
		if (bScroll && (T[Ind] == 0x0C)) { InsPage = InsPg; Ind++; }
	} while (r > PageS);
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
		if (Column(p) - BCol > LineS)
		{
			BCol = Column(p) - LineS; BPos = Position(BCol);
		}
		if (Column(Posi) <= BCol)
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
	screen.GotoXY(Posi - BPos, succ(LineL - ScrL));
	IsWrScreen = true;
}

void KodLine()
{
	WORD LP = 0;

	LP = LastPosLine() + 1;
	if (HardL) LP++;
	TestLenText(NextI, longint(LineI) + LP);
	Move(Arr, &T[LineI], LP);
	NextI = LineI + LP;
	LP = NextI - 1;
	if (HardL) LP--;
	T[LP] = _CR;
	if (HardL) T[LP + 1] = _LF;
	UpdatedL = false;
}

void TestKod()
{
	if (UpdatedL) KodLine();
}

longint NewRL(int Line)
{
	return LineAbs(Line);
}

int NewL(longint RLine)
{
	return RLine - Part.LineP;
}

void ScrollPress()
{
	BYTE* BP = nullptr;
	bool old = false, fyz = false;
	longint L1 = 0;
	void* ptr = nullptr;

	old = bScroll;
	//fyz = *(BP(ptr(0, 0x417)) && 0x10) != 0;
	if (fyz == old) FirstScroll = false;
	bScroll = (fyz || FirstScroll) && (Mode != HelpM);
	HelpScroll = bScroll || (Mode == HelpM);
	L1 = LineAbs(ScrL);
	if (old != bScroll)
	{
		if (bScroll)
		{
			WrStatusLine();
			TestKod();
			screen.CrsHide();
			PredScLn = LineAbs(LineL);
			PredScPos = Posi;
			if (UpdPHead)
			{
				SetPart(1);
				SimplePrintHead();
				DekFindLine(MaxL(L1, PHNum + 1));
			}
			else
			{
				DekFindLine(MaxL(L1, PHNum + 1));
			}
			ScrL = LineL;
			RScrL = NewRL(ScrL);
			if (L1 != LineAbs(ScrL)) ChangeScr = true; // { DekodLine; }
			BCol = Column(BPos);
			Colu = Column(Posi);
			ColScr = Part.ColorP;
			SetColorOrd(ColScr, 1, ScrI);
		}
		else
		{
			if ((PredScLn < L1) || (PredScLn >= L1 + PageS)) PredScLn = L1;
			if (!(PredScPos >= BPos + 1 && PredScPos <= BPos + LineS)) PredScPos = BPos + 1;
			PosDekFindLine(PredScLn, PredScPos, false);
			if (Mode == ViewM || Mode == SinFM || Mode == DouFM
				|| Mode == DelFM || Mode == NotFM) screen.CrsBig();
			else screen.CrsNorm();
		}
		Background();
	}
}

void DisplLL(WORD Flags)
{
	if ((Flags & 0x04) != 0) // { Ctrl }
		WrLLMargMsg(&CtrlLastS, CtrlLastNr);
	else if ((Flags & 0x03) != 0) // { Shift }
		WrLLMargMsg(&ShiftLastS, 0);
	else if ((Flags & 0x08) != 0) // { Alt }
		WrLLMargMsg(&AltLastS, 0);
}

void CtrlShiftAlt()
{
	bool Ctrl; WORD Delta, flgs;
	Ctrl = false;  Delta = 0; flgs = 0;
	//(*MyTestEvent 1; *)
label1:
	WaitEvent(Delta);
	if (Mode != HelpM) ScrollPress();
	if (LLKeyFlags != 0)      /* mouse */
	{
		flgs = LLKeyFlags; DisplLL(LLKeyFlags); Ctrl = true;
	}
	else if ((KbdFlgs & 0x0F) != 0) /* Ctrl Shift Alt pressed */
	{
		if (!Ctrl)
			if (Delta > 0) { flgs = KbdFlgs; DisplLL(KbdFlgs); Ctrl = true; }
			else Delta = spec.CtrlDelay;
	}
	else if (Ctrl) {
		flgs = 0; WrLLMargMsg(&LastS, LastNr);
		Ctrl = false; Delta = 0;
	}
	/*      WaitEvent(Delta);*/
	if (!(Event.What == evKeyDown || Event.What == evMouseDown))
	{
		ClrEvent(); if (!IsWrScreen) Background(); goto label1;
	}
	if (flgs != 0) {
		LLKeyFlags = 0; WrLLMargMsg(&LastS, LastNr);
		AddCtrlAltShift(flgs);
	}
}

void Wr(pstring s, pstring OrigS)
{
	if (Mode != HelpM)
	{
		if (s.empty()) s = OrigS;
		else {
			screen.ScrRdBuf(0, 0, &OrigS[1], 2);
			Move(&OrigS[3], &OrigS[2], 1);
			OrigS[0] = 2;
		}
		screen.ScrWrStr(0, 0, s, SysLColor);
	}
}

bool My2GetEvent()
{
	ClrEvent();
	GetEvent();
	if (Event.What != evKeyDown)
	{
		ClrEvent();
		return false;
	}
	auto ek = &Event.KeyCode;
	if (*ek >= 'A' && *ek <= 'Z') // with Event do if upcase(chr(KeyCode)) in ['A'..'Z'] then
	{
		*ek = toupper(*ek) - '@';
		if ((*ek == _Y_ || *ek == _Z_) && (spec.KbdTyp == CsKbd || spec.KbdTyp == SlKbd))
		{
			switch (*ek)
			{
			case _Z_: *ek = _Y_; break;
			case _Y_: *ek = _Z_; break;
			default: break;
			}
		}
	}
	return true;
}

bool ScrollEvent() {
	EdExitD* X;
	bool result = false;

	if (Event.What != evKeyDown) return result;
	// with Event do case KeyCode of
	switch (Event.KeyCode) {
	case _ESC_:
	case _left_: case _right_:
	case _up_: case _down_: case _PgUp_: case _PgDn_:
	case _CtrlPgUp_: case _CtrlPgDn_: case _CtrlF5_: case _AltF8_:
		result = true;
		break;
	default: {
		if ((Lo(Event.KeyCode) == 0x00) && (Breaks.first(Hi(Event.KeyCode)) != 0)) result = true;
		else
		{
			X = ExitD;
			while (X != nullptr) {
				if (TestExitKey(Event.KeyCode, X))
				{
					result = true;
					return result;
				}
				else X = X->Chain;
			}
		}
	}
	}
	return result;
}

bool ViewEvent()
{
	bool result = ScrollEvent();
	if (Event.What != evKeyDown) return result;
	switch (Event.KeyCode)
	{
	case _QF_:
	case _L_: case _F7_: case _F8_: case _KP_: case _QB_:
	case _QK_: case _CtrlF5_: case _AltF8_: case _CtrlF3_:
	case _Home_: case _End_:
	case _CtrlLeft_: case _CtrlRight_:
	case _QX_: case _QE_: case _Z_: case _W_:
	case _CtrlF6_:
	case _KW_: case _KB_:
	case _KK_: { result = true; break; }
	default:;
	}
	return result;
}

bool HelpEvent()
{
	bool result = false;
	if (Event.What == evKeyDown)
		switch (Event.KeyCode) {
		case _ESC_:
		case _left_: case _right_: case _up_: case _down_:
		case _PgDn_: case _PgUp_: case _M_:
		{ result = true; break; }
		default:;
		}

	else if ((Lo(Event.KeyCode) == 0x00) && (Breaks.first(Hi(Event.KeyCode))) != 0)
		result = true;
	if (Event.What == evMouseDown) result = true;
	return result;
}

bool MyGetEvent() {
	pstring OrigS(4);
	OrigS = "    ";
	WORD ww;

	auto result = false;

	CtrlShiftAlt();
	// *** Prekodovani klaves ***
	GetEvent();
	if (Event.What == evKeyDown)
		switch (Event.KeyCode) {
		case _S_: Event.KeyCode = _left_; break;
		case _D_: Event.KeyCode = _right_; break;
		case _E_: Event.KeyCode = _up_; break;
		case _X_: Event.KeyCode = _down_; break;
		case _R_: Event.KeyCode = _PgUp_; break;
		case _C_: Event.KeyCode = _PgDn_; break;
		case _A_: Event.KeyCode = _CtrlLeft_; break;
		case _F_: Event.KeyCode = _CtrlRight_; break;
		case _V_: Event.KeyCode = _Ins_; break;
		case _P_:
		{
			Wr("\x10", OrigS);
			ww = Event.KeyCode;
			if (My2GetEvent())
			{
				Wr("", OrigS);
				if (Event.KeyCode <= 0x31) Event.KeyCode = (ww << 8) || Event.KeyCode;
			}
			break;
		}
		case _Q_:
		{
			Wr("\x11", OrigS);
			ww = Event.KeyCode;
			if (My2GetEvent())
			{
				Wr("", OrigS);
				switch (Event.KeyCode) {
				case _S_: Event.KeyCode = _Home_; break;
				case _D_: Event.KeyCode = _End_; break;
				case _R_: Event.KeyCode = _CtrlPgUp_; break;
				case _C_: Event.KeyCode = _CtrlPgDn_; break;
				case _E_: case _X_: case _Y_: case _L_: case _B_: case _K_: case _I_: case _F_: case _A_:
				case 0x2D: // -
				case 0x2F: // /
				case 0x3D: // =
					Event.KeyCode = (ww << 8) | Event.KeyCode;
					break;
				default: Event.KeyCode = 0;
				}
			}
		}
		case _K_:
		{
			Wr("\x0B", OrigS);
			ww = Event.KeyCode;
			if (My2GetEvent())
			{
				Wr("", OrigS);
				std::set<char> setKc = { _B_, _K_, _H_, _S_, _Y_, _C_, _V_, _W_, _R_, _P_, _F_, _U_, _L_, _N_ };
				if (setKc.count(Event.KeyCode) > 0)
				{
					Event.KeyCode = (ww << 8) | Event.KeyCode;
				}
				else { Event.KeyCode = 0; }
			}
		}
		case _O_:
		{
			Wr("\x0F", OrigS);
			ww = Event.KeyCode;
			if (My2GetEvent())
			{
				Wr("", OrigS);
				switch (Event.KeyCode) {
				case _W_: case  _R_: case  _L_: case  _J_: case  _C_:
				{
					Event.KeyCode = (ww << 8) | Event.KeyCode;
					break;
				}
				default: Event.KeyCode = 0;
				}
			}
		}
		}
	// *** Rezim - test ***
	switch (Mode)
	{
	case HelpM:
	{
		result = HelpEvent();
		break;
	}
	case ViewM: {
		if (bScroll) result = ScrollEvent();
		else result = ViewEvent();
		break;
	}
	case TextM: {
		if (bScroll) result = ScrollEvent();
		else result = true;
		break;
	}
	default:;
	}
	return result;
}

void MyInsLine()
{
	TextAttr = TxtColor;
	InsLine();
}

void MyDelLine()
{
	TextAttr = TxtColor;
	DelLine();
}

void PredLine()
{
	WORD mi, ml;
	TestKod;
	if ((LineL == 1) && (Part.PosP > 0)) PredPart();
	if (LineL > 1)
	{
		if (T[LineI - 1] == _LF) SetDekCurrI(LineI - 2);
		else SetDekCurrI(LineI - 1); LineL--;
		if (LineL < ScrL)
		{
			screen.GotoXY(1, 1); MyInsLine(); ScrL--; ChangeScr = true;
			if (Scroll)
			{ /*dec(RLineL);*/
				RScrL--;
				/*if (ModPage(RLineL))*/
				if (ModPage(RScrL))
				{
					screen.GotoXY(1, 1); MyInsLine();/*dec(RLineL);*/RScrL--;
				}
			}
		}
	}
}

void RollNext()
{
	if ((NextI >= LenT) && !AllRd) NextPartDek();
	if (NextI <= LenT)
	{
		screen.GotoXY(1, 1); MyDelLine(); ScrL++; ChangeScr = true;
		if (LineL < ScrL)
		{
			TestKod(); LineL++; LineI = NextI; DekodLine();
		}
	}
}

void RollPred()
{
	if ((ScrL == 1) && (Part.PosP > 0)) PredPart();
	if (ScrL > 1)
	{
		screen.GotoXY(1, 1); MyInsLine(); ScrL--; ChangeScr = true;
		if (LineL == ScrL + PageS)
		{
			TestKod(); LineL--;
			if (T[LineI - 1] == _LF) SetDekCurrI(LineI - 2);
			else SetDekCurrI(LineI - 1);
		}
	}
}

void direction1(BYTE x, BYTE& zn2)
{
	BYTE y;
	y = 0x10;
	if (x > 2) y = y << 1;
	if (x == 0) y = 0;
	if (Mode == DouFM) zn2 = zn2 || y;
	else zn2 = zn2 && !y;
}

void MyWriteln()
{
	TextAttr = TxtColor;
	printf("\n");
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

void Frame()
{
	pstring FrameString(15);
	FrameString = "\x50\x48\xB3\x4D\xDA\xC0\xC3\x4B\xBF\xD9\xB4\xC4\xC2\xC1\xC5";
	pstring FS1(15);
	FS1 = "\x50\x48\xBA\x4D\xD6\xD3\xC7\x4B\xB7\xBD\xB6\xC4\xD2\xD0\xD7";
	pstring FS2(15);
	FS2 = "\x50\x48\xB3\x4D\xD5\xD4\xC6\x4B\xB8\xBE\xB5\xCD\xD1\xCF\xD8";
	pstring FS3(15);
	FS3 = "\x50\x48\xBA\x4D\xC9\xC8\xCC\x4B\xBB\xBC\xB9\xCD\xCB\xCA\xCE";
	char oldzn; BYTE dir, odir, zn1, zn2, b;

	UpdStatLine(LineL, Posi); screen.CrsBig(); odir = 0;
	ClrEvent();
	while (true) /* !!! with Event do!!! */
	{
		if (!MyGetEvent() ||
			((Event.What == evKeyDown) && (Event.KeyCode == _ESC_)) || (Event.What != evKeyDown))
		{
			ClrEvent(); screen.CrsNorm(); Mode = TextM; return;
		}
		switch (Event.KeyCode) {
		case _frmsin_: Mode = SinFM; break;
		case _frmdoub_: Mode = DouFM; break;
		case _dfrm_: Mode = DelFM; break;
		case _nfrm_: Mode = NotFM; break;
		case _left_:
		case _right_:
		case _up_:
		case _down_:
			if (!bScroll) {
				FrameString[0] = 63;
				zn1 = FrameString.first(Arr[Posi]); zn2 = zn1 & 0x30; zn1 = zn1 & 0x0F;
				dir = FrameString.first(Hi(Event.KeyCode));
				auto dirodir = dir + odir;
				if (dirodir == 2 || dirodir == 4 || dirodir == 8 || dirodir == 16) odir = 0;
				if (zn1 == 1 || zn1 == 2 || zn1 == 4 || zn1 == 8) zn1 = 0;
				oldzn = Arr[Posi]; Arr[Posi] = ' ';
				if (Mode == DelFM) b = zn1 && !(odir || dir);
				else b = zn1 | (odir ^ dir);
				if (b == 1 || b == 2 || b == 4 || b == 8) b = 0;
				if ((Mode == DelFM) && (zn1 != 0) && (b == 0)) oldzn = ' ';
				direction1(dir, zn2); direction1(odir, zn2);
				if (Mode == NotFM) b = 0;
				if ((b != 0) && ((Event.KeyCode == _left_) || (Event.KeyCode == _right_) ||
					(Event.KeyCode == _up_) || (Event.KeyCode == _down_)))
					Arr[Posi] = FrameString[zn2 + b];
				else Arr[Posi] = oldzn;
				if ((dir == 1) || (dir == 4)) odir = dir * 2;
				else odir = dir / 2;
				if (Mode == NotFM) odir = 0;
				else UpdatedL = true;
				switch (Event.KeyCode) {
				case _left_: if (Posi > 1) Posi--; break;
				case _right_: if (Posi < LineSize) Posi++; break;
				case _up_: PredLine(); break;
				case _down_: NextLine(true); break;
				}
			}
			break;
		}
		ClrEvent();
		UpdStatLine(LineL, Posi);/*if (not MyTestEvent) */
		Background();
	}
}

void CleanFrameM()
{
	if (Mode == SinFM || Mode == DouFM || Mode == DelFM || Mode == NotFM) /* !!! with Event do!!! */
		if (!MyGetEvent() ||
			((Event.What == evKeyDown) && (Event.KeyCode == _ESC_)) || (Event.What != evKeyDown))
		{
			ClrEvent(); screen.CrsNorm(); Mode = TextM;
			UpdStatLine(LineL, Posi); return;
		}
}

void direction2(BYTE x, BYTE& zn2)
{
	BYTE y;
	y = 0x10;
	if (x > 2) y = y << 1;
	if (x == 0) y = 0;
	if (Mode == DouFM) zn2 = zn2 || y;
	else zn2 = zn2 && !y;
}

void FrameStep(BYTE& odir, WORD EvKeyC)
{
	pstring FrameString(15);
	FrameString = "\x50\x48\xB3\x4D\xDA\xC0\xC3\x4B\xBF\xD9\xB4\xC4\xC2\xC1\xC5";
	pstring FS1(15);
	FS1 = "\x50\x48\xBA\x4D\xD6\xD3\xC7\x4B\xB7\xBD\xB6\xC4\xD2\xD0\xD7";
	pstring FS2(15);
	FS2 = "\x50\x48\xB3\x4D\xD5\xD4\xC6\x4B\xB8\xBE\xB5\xCD\xD1\xCF\xD8";
	pstring FS3(15);
	FS3 = "\x50\x48\xBA\x4D\xC9\xC8\xCC\x4B\xBB\xBC\xB9\xCD\xCB\xCA\xCE";
	char oldzn; BYTE dir, zn1, zn2, b;

	switch (EvKeyC) {
	case _frmsin_: Mode = SinFM; break;
	case _frmdoub_: Mode = DouFM; break;
	case _dfrm_: Mode = DelFM; break;
	case _nfrm_: Mode = NotFM; break;
	case _left_:
	case _right_:
	case _up_:
	case _down_:
	{
		FrameString[0] = 63;
		zn1 = FrameString.first(Arr[Posi]);
		zn2 = zn1 & 0x30; zn1 = zn1 & 0x0F;
		dir = FrameString.first(Hi(EvKeyC));
		auto dirodir = dir + odir;
		if (dirodir == 2 || dirodir == 4 || dirodir == 8 || dirodir == 16) odir = 0;
		if (zn1 == 1 || zn1 == 2 || zn1 == 4 || zn1 == 8) zn1 = 0;
		oldzn = Arr[Posi]; Arr[Posi] = ' ';
		if (Mode == DelFM) b = zn1 && !(odir || dir);
		else b = zn1 || (odir ^ dir);
		if (b == 1 || b == 2 || b == 4 || b == 8) b = 0;
		if ((Mode == DelFM) && (zn1 != 0) && (b == 0)) oldzn = ' ';
		direction2(dir, zn2); direction2(odir, zn2);
		if (Mode == NotFM) b = 0;
		if ((b != 0) && ((Event.KeyCode == _left_) || (Event.KeyCode == _right_) ||
			(Event.KeyCode == _up_) || (Event.KeyCode == _down_)))
			Arr[Posi] = FrameString[zn2 + b];
		else Arr[Posi] = oldzn;
		if ((dir == 1) || (dir == 4)) odir = dir * 2;
		else odir = dir / 2;
		if (Mode == NotFM) odir = 0;
		else UpdatedL = true;
		switch (Event.KeyCode) {
		case _left_: if (Posi > 1) Posi--; break;
		case _right_: if (Posi < LineSize) Posi++; break;
		case _up_: PredLine(); break;
		case _down_: NextLine(true); break;
		}
	}
	break;
	}
	UpdStatLine(LineL, Posi);
}

void MoveB(WORD& B, WORD& F, WORD& T)
{
	if (F <= T) { if (B > F) B += T - F; }
	else if (B >= F) B -= F - T;
	else if (B > T) B = T; B = MinW(B, LastPosLine() + 1);
}

bool TestLastPos(WORD F, WORD T)
{
	WORD LP;
	LP = LastPosLine();
	if (F > LP) F = LP + 1;
	if (LP + T - F <= LineSize)
	{
		if (LP >= F) Move(&Arr[F], &Arr[T], succ(LP - F));
		if (TypeB == TextBlock)
		{
			if (LineAbs(LineL) == BegBLn) MoveB(BegBPos, F, T);
			if (LineAbs(LineL) == EndBLn) MoveB(EndBPos, F, T);
		}
		if (F > T)
		{
			if (T <= LP) FillChar(&Arr[LP + 1 + T - F], F - T, 32);
		}
		UpdatedL = true; return true;
	}
	else return false;
}

void DelChar()
{
	WORD LP;
	if (TestLastPos(succ(Posi), Posi));
}

void FillBlank()
{
	WORD I;
	KodLine();
	I = LastPosLine();
	if (Posi > I + 1) {
		TestLenText(LineI + I, longint(LineI) + Posi - 1);
		FillChar(&T[LineI + I], Posi - I - 1, 32); NextI += Posi - I - 1;
	}
}

void DeleteL()
{
	bool B;
	FillBlank();
	if (LineAbs(LineL) + 1 <= BegBLn)
	{
		BegBLn--;
		if ((LineAbs(LineL) == BegBLn) && (TypeB == TextBlock))
			BegBPos += LastPosLine();
	}
	if (LineAbs(LineL) + 1 <= EndBLn)
	{
		EndBLn--;
		if ((LineAbs(LineL) == EndBLn) && (TypeB == TextBlock))
			EndBPos += LastPosLine();
	}
	if ((NextI >= LenT) && !AllRd) NextPartDek();
	if (NextI <= LenT)
		if (T[NextI - 1] == _LF) TestLenText(NextI, NextI - 2);
		else TestLenText(NextI, NextI - 1);
	DekodLine();
}

void NewLine(char Mode)
{
	WORD LP;
	KodLine(); LP = LineI + MinI(LastPosLine(), Posi - 1);
	NullChangePart();
	TestLenText(LP, longint(LP) + 2);
	LP -= Part.MovI;
	if (LineAbs(LineL) <= BegBLn)
		if (LineAbs(LineL) < BegBLn) BegBLn++;
		else if ((BegBPos > Posi) && (TypeB == TextBlock))
		{
			BegBLn++; BegBPos -= Posi - 1;
		}
	if (LineAbs(LineL) <= EndBLn)
		if (LineAbs(LineL) < EndBLn) EndBLn++;
		else if ((EndBPos > Posi) && (TypeB == TextBlock))
		{
			EndBLn++; EndBPos -= Posi - 1;
		}
	T[LP] = _CR; T[succ(LP)] = _LF;
	if (Mode == 'm') { LineL++; LineI = LP + 2; }
	DekodLine();
}

WORD SetPredI()
{
	if ((LineL == 1) && (Part.PosP > 0)) PredPart();
	if (LineI <= 1) return LineI;
	else if (T[LineI - 1] == _LF) return SetCurrI(LineI - 2);
	else return SetCurrI(LineI - 1);
}

void WrChar_E(char Ch)
{
	if (Insert)
	{
		if (TestLastPos(Posi, succ(Posi)))
		{
			Arr[Posi] = Ch;
			if (Posi < LineSize) Posi++;
		}
	}
	else {
		Arr[Posi] = Ch;
		UpdatedL = true;
		if (Posi < LineSize) Posi++;
	}
}

void Format(WORD& i, longint First, longint Last, WORD Posit, bool Rep)
{
	WORD fst, lst, ii1;
	integer ii;
	longint llst;
	char A[260];
	bool bBool;
	WORD rp, nb, nw, n;
	WORD RelPos;

	SetPart(First); fst = First - Part.PosP; llst = Last - Part.PosP;
	if (llst > LenT) lst = LenT; else lst = llst;
	do {
		if (LenT > 0x400) ii1 = LenT - 0x400; else ii1 = 0;
		if ((fst >= ii1) && !AllRd)
		{
			NextPartDek();
			fst -= Part.MovI; lst -= Part.MovI; llst -= Part.MovI;
			if (llst > LenT) lst = LenT; else lst = llst;
		}
		i = fst; ii1 = i;
		if ((i < 2) || (T[i - 1] == _LF))
		{
			while (T[ii1] == ' ') ii1++; Posit = MaxW(Posit, ii1 - i + 1);
		}
		ii1 = i; RelPos = 1;
		if (Posit > 1)
		{
			Move(&T[i], A, Posit);
			for (ii = 1; ii < Posit - 1; i++)
			{
				if (CtrlKey.first(T[i]) == 0) RelPos++;
				if (T[i] == _CR) A[ii] = ' ';
				else i++;
			}
			if ((T[i] == ' ') && (A[Posit - 1] != ' '))
			{
				Posit++; RelPos++;
			}
		}
		while (i < lst)
		{
			bBool = true; nw = 0; nb = 0;
			if (RelPos < LeftMarg)
				if ((Posit == 1) || (A[Posit - 1] == ' '))
				{
					ii = LeftMarg - RelPos; FillChar(&A[Posit], ii, 32);
					Posit += ii; RelPos = LeftMarg;
				}
				else while (RelPos < LeftMarg)
				{
					Posit++;
					if (CtrlKey.first(T[i]) == 0) RelPos++;
					if (T[i] != _CR) i++;
					if (T[i] == _CR) A[Posit] = ' ';
					else A[Posit] = T[i];
				}
			while ((RelPos <= RightMarg) && (i < lst))
			{
				if ((T[i] == _CR) || (T[i] == ' '))
				{
					while (((T[i] == _CR) || (T[i] == ' ')) && (i < lst))
						if (T[i + 1] == _LF) lst = i;
						else { T[i] = ' '; i++; }
					if (!bBool) { nw++; if (i < lst) i--; };
				}
				if (i < lst)
				{
					bBool = false;
					A[Posit] = T[i];
					if (CtrlKey.first(A[Posit]) == 0) RelPos++;
					i++; Posit++;
				}
			}
			if ((i < lst) && (T[i] != ' ') && (T[i] != _CR))
			{
				ii = Posit - 1;
				if (CtrlKey.first(A[ii]) != 0) ii--;
				rp = RelPos; RelPos--;
				while ((A[ii] != ' ') && (ii > LeftMarg))
				{
					if (CtrlKey.first(A[ii]) == 0) RelPos--; ii--;
				}
				if (RelPos > LeftMarg)
				{
					nb = rp - RelPos; i -= (Posit - ii); Posit = ii;
				}
				else
				{
					while ((T[i] != ' ') && (T[i] != _CR) && (Posit < LineSize))
					{
						A[Posit] = T[i]; i++; Posit++;
					}
					while (((T[i] == _CR) || (T[i] == ' ')) && (i < lst))
						if (T[i + 1] == _LF) lst = i;
						else { T[i] = ' '; i++; }
				}
			}
			if (Just)
			{
				ii = LeftMarg;
				while ((nb > 0) and (nw > 1))
				{
					while (A[ii] == ' ') ii++;
					while (A[ii] != ' ') ii++;
					nw--; n = nb / nw;
					if ((nw % nb != 0) && (nw % 2) && (nb > n)) n++;
					if (Posit - ii > 0) Move(&A[ii], &A[ii + n], Posit - ii + 1);
					FillChar(&A[ii], n, 32); Posit += n;
					nb -= n;
				}
			}
			ii = 1;
			while (A[ii] == ' ') ii++;
			if (ii >= Posit) Posit = 1;
			if (i < lst) A[Posit] = _CR; else Posit--;
			TestLenText(i, longint(ii1) + Posit);
			if (Posit > 0) Move(A, &T[ii1], Posit);
			ii = ii1 + Posit - i; i = ii1 + Posit; lst += ii; llst += ii;
			Posit = 1; RelPos = 1; ii1 = i;
		}
		if (Rep)
		{
			while ((T[i] == _CR) || (T[i] == _LF)) i++; fst = i; Rep = i < llst;
			if (llst > LenT) lst = LenT; else lst = llst;
		}
	} while (Rep);
	BegBLn = 1; BegBPos = 1; EndBLn = 1; EndBPos = 1; TypeB = TextBlock;
}

void Calculate()
{
	wwmix ww;
	FrmlPtr Z = nullptr; pstring Txt; ExitRecord er; WORD I; pstring Msg;
	void* p = nullptr; char FTyp; double R; bool Del;
	MarkStore(p); //NewExit(Ovr(), er);
	goto label2; ResetCompilePars();
	RdFldNameFrml = RdFldNameFrmlT;
label0:
	Txt = CalcTxt; Del = true; I = 1;
label1:
	TxtEdCtrlUBrk = true; TxtEdCtrlF4Brk = true;
	ww.PromptLL(114, &Txt, I, Del);
	if (KbdChar == _U_) goto label0;
	if ((KbdChar == _ESC_) || (Txt.length() == 0)) goto label3;
	CalcTxt = Txt;
	if ((KbdChar == _CtrlF4_) && (Mode == TextM) && !bScroll)
	{
		if (Txt.length() > LineSize - LastPosLine())
		{
			I = LineSize - LastPosLine(); WrLLF10Msg(419); goto label1;
		}
		if (Posi <= LastPosLine()) TestLastPos(Posi, Posi + Txt.length());
		Move(&Txt[1], &Arr[Posi], Txt.length()); UpdatedL = true; goto label3;
	}
	SetInpStr(Txt);
	RdLex();
	Z = RdFrml(FTyp);
	if (Lexem != 0x1A) Error(21);
	switch (FTyp) {
	case 'R': {
		R = RunReal(Z); str(R, 30, 10, Txt);
		Txt = LeadChar(' ', TrailChar('0', Txt));
		if (Txt[Txt.length()] == '.') Txt[0]--;
		break;
	}
	case 'S': { Txt = RunShortStr(Z);   /* wie RdMode fuer T ??*/ break; }
	case 'B': {
		if (RunBool(Z)) Txt = AbbrYes;
		else Txt = AbbrNo;
		break;
	}
	}
	I = 1; goto label1;
label2:
	Msg = MsgLine; I = CurrPos; SetMsgPar(Msg); WrLLF10Msg(110);
	IsCompileErr = false; ReleaseStore(p); Del = false; goto label1;
label3:
	ReleaseStore(p); RestoreExit(er);
}

bool BlockExist()
{
	if (TypeB == TextBlock)
		return (BegBLn < EndBLn) || (BegBLn == EndBLn) && (BegBPos < EndBPos);
	return (BegBLn <= EndBLn) && (BegBPos < EndBPos);
}

void SetBlockBound(longint& BBPos, longint& EBPos)
{
	integer i;
	SetPartLine(EndBLn);
	i = EndBLn - Part.LineP;
	EBPos = SetInd(FindLine(i), EndBPos) + Part.PosP;
	SetPartLine(BegBLn);
	i = BegBLn - Part.LineP;
	BBPos = SetInd(FindLine(i), BegBPos) + Part.PosP;
}

void ResetPrint(char Oper, longint& fs, FILE* W1, longint LenPrint, ColorOrd* co, WORD& I1, bool isPrintFile, CharArr* p)
{
	*co = Part.ColorP; SetColorOrd(*co, 1, I1); isPrintFile = false;
	fs = co->length(); LenPrint += fs;
	if (Oper == 'p') LenPrint++;
	if ((StoreAvail() > LenPrint) && (LenPrint < 0xFFF0))
	{
		p = (CharArr*)GetStore2(LenPrint); Move(&co[1], p, co->length());
	}
	else
	{
		isPrintFile = true; W1 = WorkHandle; SeekH(W1, 0);
		WriteH(W1, co->length(), &co[1]); HMsgExit(CPath);
	}
}

void LowCase(unsigned char& c)
{
	BYTE i;
	if ((c >= 'A') && (c <= 'Z')) { c = c + 0x20; return; }
	for (i = 128; i <= 255; i++)
		if ((UpcCharTab[i] == c) && (i != c)) { c = i; return; }
}

void LowCase(char& c)
{
	BYTE i;
	if ((c >= 'A') && (c <= 'Z')) { c = c + 0x20; return; }
	for (i = 128; i <= 255; i++)
		if ((UpcCharTab[i] == c) && (i != c)) { c = i; return; }
}

bool BlockHandle(longint& fs, FILE* W1, char Oper)
{
	WORD i, I1, I2, Ps;
	longint LL1, LL2, Ln;
	char* a = nullptr;
	ColorOrd co; bool isPrintFile = false; CharArr* p = nullptr;
	bool tb; char c;

	TestKod(); Ln = LineAbs(LineL); Ps = Posi;
	if (Oper == 'p') { tb = TypeB; TypeB = TextBlock; }
	else
		if (!BlockExist()) { return false; }
	screen.CrsHide();
	auto result = true;
	if (TypeB == TextBlock)
	{
		if (Oper == 'p')
		{
			LL2 = AbsLenT - Part.LenP + LenT;
			LL1 = Part.PosP + SetInd(LineI, Posi);
		}
		else SetBlockBound(LL1, LL2); I1 = LL1 - Part.PosP;
		if (toupper(Oper) == 'P') ResetPrint(Oper, fs, W1, LL2 - LL1, &co, I1, isPrintFile, p);
		do {
			if (LL2 > Part.PosP + LenT) I2 = LenT;
			else I2 = LL2 - Part.PosP;
			switch (Oper) {
			case 'Y': { TestLenText(I2, I1); LL2 -= I2 - I1; break; }
			case 'U': {
				for (i = I1; i < I2 - 1; i++) T[i] = UpcCharTab[T[i]];
				LL1 += I2 - I1; break;
			}
			case 'L': { for (i = I1; i < I2 - 1; i++) LowCase(T[i]); LL1 += I2 - I1; break; }
			case 'p':
			case 'P': {
				if (isPrintFile)
				{
					WriteH(W1, I2 - I1, &T[I1]); HMsgExit(CPath);
				}
				else {
					Move(&T[I1], p[fs + 1], I2 - I1);
					fs += I2 - I1; LL1 += I2 - I1;
				}
				break;
			}
			case 'W': {
				SeekH(W1, fs); WriteH(W1, I2 - I1, &T[I1]); HMsgExit(CPath);
				fs += I2 - I1; LL1 += I2 - I1;
				break;
			}
			}
			if (Oper == 'U' || Oper == 'L' || Oper == 'Y') SetUpdat();
			if ((Oper == 'p') && AllRd) LL1 = LL2;
			if (!AllRd && (LL1 < LL2))
			{
				I1 = LenT; NextPart(); I1 -= Part.MovI;
			}
		} while (LL1 != LL2);
	}
	else              /*ColBlock*/
	{
		PosDekFindLine(BegBLn, BegBPos, false);
		I1 = EndBPos - BegBPos; LL1 = (EndBLn - BegBLn + 1) * (I1 + 2); LL2 = 0;
		if (Oper == 'P') ResetPrint(Oper, fs, W1, LL1, &co, I1, isPrintFile, p);
		do {
			switch (Oper) {
			case 'Y': { TestLastPos(EndBPos, BegBPos); break; }
			case 'U': {
				for (i = BegBPos; i < EndBPos - 1; i++)
					Arr[i] = UpcCharTab[Arr[i]]; UpdatedL = true;
				break;
			}
			case 'L': {
				for (i = BegBPos; i < EndBPos - 1; i++) LowCase(Arr[i]);
				UpdatedL = true;
				break;
			}
			case 'W':
			case 'P': {
				Move(&Arr[BegBPos], a, I1); a[I1 + 1] = _CR; a[I1 + 2] = _LF;
				if ((Oper == 'P') && !isPrintFile) Move(a, p[fs + 1], I1 + 2);
				else { WriteH(W1, I1 + 2, a); HMsgExit(CPath); }
				fs += I1 + 2;
				break;
			}
			}
			LL2 += I1 + 2;
			NextLine(false);
		} while (LL2 != LL1);

	}
	if (toupper(Oper) == 'P') {
		if (isPrintFile)
		{
			WriteH(W1, 0, T);/*truncH*/PrintFandWork();
		}
		else
		{
			PrintArray(p, fs, false); ReleaseStore2(p);
		}
	}
	if (Oper == 'p') TypeB = tb;
	if (Oper == 'Y') PosDekFindLine(BegBLn, BegBPos, true);
	else { if (Oper == 'p') SetPart(1); PosDekFindLine(Ln, Ps, true); }
	if (!bScroll) screen.CrsShow();
	return result;
}

void DelStorClpBd(void* P1, LongStr* sp)
{
	TWork.Delete(ClpBdPos);
	ClpBdPos = TWork.Store(sp);
	ReleaseStore2(P1);
}

void MarkRdClpBd(void* P1, LongStr* sp)
{
	MarkStore2(P1); sp = TWork.Read(2, ClpBdPos);
}

void MovePart(WORD Ind)
{
	if (TypeT != FileT) return; TestUpdFile(); WrEndT();
	/* !!! with Part do!!! */
	{ Part.MovI = SetCurrI(Ind) - 1; Part.MovL = SetLine(Part.MovI) - 1;
	Part.LineP += Part.MovL; Part.PosP += Part.MovI; Part.LenP -= Part.MovI;
	SetColorOrd(Part.ColorP, 1, Part.MovI + 1);
	TestLenText(Part.MovI + 1, 1);
	ChangePart = true;
	}
}

bool BlockGrasp(char Oper, void* P1, LongStr* sp)
{
	longint L, L1, L2, ln;
	WORD I1;
	auto result = false;
	if (!BlockExist()) return result; L = Part.PosP + LineI + Posi - 1;
	ln = LineAbs(LineL); if (Oper == 'G') TestKod();
	SetBlockBound(L1, L2);
	if ((L > L1) and (L < L2) and (Oper != 'G')) return result;
	L = L2 - L1; if (L > 0x7FFF) { WrLLF10Msg(418); return result; }
	if (L2 > Part.PosP + LenT) MovePart(L1 - Part.PosP);
	I1 = L1 - Part.PosP;
	MarkStore2(P1); sp = (LongStr*)GetStore2(L + 2); sp->LL = L;
	Move(&T[I1], sp->A, L);
	if (Oper == 'M')
	{
		TestLenText(I1 + L, I1);
		/*   if (L1>Part.PosP+I1) dec(L1,L);*/
		if (EndBLn <= ln)
		{
			if ((EndBLn == ln) && (Posi >= EndBPos))
				Posi = BegBPos + Posi - EndBPos;
			ln -= EndBLn - BegBLn;
		}
	}
	if (Oper == 'G') DelStorClpBd(P1, sp);
	PosDekFindLine(ln, Posi, false);
	result = true;
	return result;
}

void BlockDrop(char Oper, void* P1, LongStr* sp)
{
	WORD I, I2;
	if (Oper == 'D') MarkRdClpBd(P1, sp); if (sp->LL == 0) return;
	/* hlidani sp->LL a StoreAvail, MaxLenT, dela TestLenText, prip.SmallerPart */
	if (Oper == 'D') FillBlank();
	I = LineI + Posi - 1; I2 = sp->LL;
	BegBLn = LineAbs(LineL); BegBPos = Posi;
	NullChangePart(); TestLenText(I, longint(I) + I2);
	if (ChangePart) I -= Part.MovI;
	Move(sp->A, &T[I], I2); ReleaseStore2(P1);
	SetDekLnCurrI(I + I2);
	EndBLn = Part.LineP + LineL;
	EndBPos = succ(I + I2 - LineI);
	PosDekFindLine(BegBLn, BegBPos, true); /*ChangeScr = true;*/
}

bool BlockCGrasp(char Oper, void* P1, LongStr* sp)
{
	WORD i, I2;
	longint l1, L;
	char* a = nullptr;

	auto result = false;
	if (!BlockExist()) return result;
	TestKod();
	L = LineAbs(LineL);
	if ((L >= BegBLn && L <= EndBLn)
		&& (Posi >= BegBPos + 1 && Posi <= EndBPos - 1)
		&& (Oper != 'G')) return result;
	l1 = (EndBLn - BegBLn + 1) * (EndBPos - BegBPos + 2);
	if (l1 > 0x7FFF) { WrLLF10Msg(418); return result; }
	MarkStore2(P1); sp = (LongStr*)GetStore2(l1 + 2); sp->LL = l1;
	PosDekFindLine(BegBLn, Posi, false); I2 = 0; i = EndBPos - BegBPos;
	do {
		Move(&Arr[BegBPos], a, i); a[i + 1] = _CR; a[i + 2] = _LF;
		if (Oper == 'M') TestLastPos(EndBPos, BegBPos);
		Move(a, &sp->A[I2 + 1], i + 2); I2 += i + 2;
		TestKod();
		NextLine(false);
	} while (I2 != sp->LL);
	if ((Oper == 'M') && (L >= BegBLn && L <= EndBLn) && (Posi > EndBPos))
		Posi -= EndBPos - BegBPos;
	if (Oper == 'G') DelStorClpBd(P1, sp); PosDekFindLine(L, Posi, false);
	result = true;
	return result;
}

void InsertLine(WORD& i, WORD& I1, WORD& I3, WORD& ww, LongStr* sp)
{
	i = MinW(I1 - I3, LineSize - LastPosLine());
	if (i > 0) { TestLastPos(ww, ww + i); Move(&sp->A[I3], &Arr[ww], i); }
	TestKod();
}

void BlockCDrop(char Oper, void* P1, LongStr* sp)
{
	WORD i, I1, I3, ww;
	if (Oper == 'D') MarkRdClpBd(P1, sp);
	if (sp->LL == 0) return;
	/* hlidani sp->LL a StoreAvail, MaxLenT
		dela NextLine - prechazi mezi segmenty */
	if (Oper != 'R')
	{
		EndBPos = Posi; BegBPos = Posi; BegBLn = LineL + Part.LineP;
	}
	ww = BegBPos; I1 = 1; I3 = 1;
	do {
		if (sp->A[I1] == _CR)
		{
			InsertLine(i, I1, I3, ww, sp);
			ww = BegBPos; EndBPos = MaxW(ww + i, EndBPos);
			if ((NextI > LenT) && ((TypeT != FileT) || AllRd))
			{
				TestLenText(LenT, longint(LenT) + 2);
				T[LenT - 2] = _CR; T[LenT - 1] = _LF; NextI = LenT;
			}
			NextLine(false);
		}
		if (sp->A[I1] == _CR || sp->A[I1] == _LF || sp->A[I1] == 0x1A) I3 = I1 + 1;
		I1++;
	} while (I1 <= sp->LL);
	if (I3 < I1) InsertLine(i, I1, I3, ww, sp);
	if (Oper != 'R')
	{
		EndBLn = Part.LineP + LineL - 1; ReleaseStore2(P1);
		PosDekFindLine(BegBLn, BegBPos, true);
	}
}

void BlockCopyMove(char Oper, void* P1, LongStr* sp)
{
	bool b;
	if (!BlockExist()) return; FillBlank();
	if (TypeB == TextBlock)
	{
		if (BlockGrasp(Oper, P1, sp)) BlockDrop(Oper, P1, sp);
	}
	else
		if (BlockCGrasp(Oper, P1, sp)) BlockCDrop(Oper, P1, sp);
}

bool ColBlockExist()
{
	bool b;
	if ((TypeB == ColBlock) && (BegBPos == EndBPos) && (BegBLn < EndBLn)) return true;
	else return BlockExist();
}

void NewBlock1(WORD& I1, longint& L2)
{
	if (I1 != Posi)
	{
		BegBLn = L2; EndBLn = L2;
		BegBPos = MinW(I1, Posi); EndBPos = MaxW(I1, Posi);
	}
}

void BlockLRShift(WORD I1)
{
	longint L2;
	if (!bScroll && (Mode != HelpM) && ((KbdFlgs & 0x03) != 0))   /*Shift*/
	{
		L2 = LineAbs(LineL);
		if (!ColBlockExist()) NewBlock1(I1, L2);
		else
			switch (TypeB) {
			case TextBlock: {
				if ((BegBLn == EndBLn) && (L2 == BegBLn) && (EndBPos == BegBPos)
					&& (I1 == BegBPos)) if (I1 > Posi) BegBPos = Posi;
					else EndBPos = Posi;
				else if ((L2 == BegBLn) && (I1 == BegBPos)) BegBPos = Posi;
				else if ((L2 == EndBLn) && (I1 == EndBPos)) EndBPos = Posi;
				else NewBlock1(I1, L2);
				break;
			}
			case ColBlock: {
				if ((L2 >= BegBLn) && (L2 <= EndBLn))
					if ((EndBPos == BegBPos) && (I1 == BegBPos))
						if (I1 > Posi) BegBPos = Posi;
						else EndBPos = Posi;
					else if (I1 == BegBPos) BegBPos = Posi;
					else if (I1 == EndBPos) EndBPos = Posi;
					else NewBlock1(I1, L2);
				else NewBlock1(I1, L2);
				break;
			}
			}
	}
}

void NewBlock2(longint& L1, longint& L2)
{
	if (L1 != L2) {
		BegBPos = Posi; EndBPos = Posi;
		BegBLn = MinL(L1, L2); EndBLn = MaxL(L1, L2);
	}
}

void BlockUDShift(longint L1)
{
	longint L2;
	if (!bScroll && (Mode != HelpM) && ((KbdFlgs & 0x03) != 0))   /*Shift*/
	{
		L2 = LineAbs(LineL);
		if (!ColBlockExist()) NewBlock2(L1, L2);
		else
			switch (TypeB) {
			case TextBlock: {
				if ((BegBLn == EndBLn) && (L1 == BegBLn))
					if ((Posi >= BegBPos) && (Posi <= EndBPos))
						if (L1 < L2) { EndBLn = L2; EndBPos = Posi; }
						else { BegBLn = L2; BegBPos = Posi; }
					else NewBlock2(L1, L2);
				else if ((L1 == BegBLn) && (BegBPos == Posi)) BegBLn = L2;
				else if ((L1 == EndBLn) && (EndBPos == Posi)) EndBLn = L2;
				else NewBlock2(L1, L2);
				break;
			}
			case ColBlock:
				if ((Posi >= BegBPos) && (Posi <= EndBPos))
					if ((BegBLn == EndBLn) && (L1 == BegBLn))
						if (L1 < L2) EndBLn = L2;
						else BegBLn = L2;
					else if (L1 == BegBLn) BegBLn = L2;
					else if (L1 == EndBLn) EndBLn = L2;
					else NewBlock2(L1, L2);
				else NewBlock2(L1, L2);
			}
	}
}

bool MyPromptLL(WORD n, pstring* s)
{
	wwmix ww;
	ww.PromptLL(n, s, 1, true);
	return KbdChar == _ESC_;
}

void ChangeP(WORD& fst)
{
	if (ChangePart)
	{
		if (fst <= Part.MovI) fst = 1;
		else fst -= Part.MovI;
		/* if (Last>Part.PosP+LenT) lst = LenT-1 else lst = Last-Part.PosP; */
		NullChangePart();
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

void ReplaceString(WORD& J, WORD& fst, WORD& lst, longint& Last)
{
	integer r, f;
	r = ReplaceStr.length(); f = FindStr.length();
	TestLenText(J, longint(J) + r - f); ChangeP(fst);
	//if (TestLastPos(Posi, Posi + r - f));
	if (ReplaceStr != "") Move(&ReplaceStr[1], &T[J - f], r);
	J += r - f;
	SetScreen(J, 0, 0);
	lst += r - f;
	Last += r - f;
}

char MyVerifyLL(WORD n, pstring s)
{
	longint w, t; WORD c1, c2, r1, r2, r; char cc;
	c2 = screen.WhereX() + FirstC - 1; r2 = screen.WhereY() + FirstR;
	w = PushW(1, 1, TxtCols, TxtRows); screen.GotoXY(1, TxtRows);
	TextAttr = colors.pTxt; ClrEol(); SetMsgPar(s); WriteMsg(n);
	c1 = screen.WhereX(); r1 = screen.WhereY();
	TextAttr = colors.pNorm;
	printf(" "); screen.CrsNorm(); t = Timer + 15; r = r1;
	do {
		while (!KbdPressed())
			if (Timer >= t) {
				t = Timer + 15;
				if (r == r1) { screen.GotoXY(c2, r2); r = r2; }
				else { screen.GotoXY(c1, r1); r = r1; }
			}
		cc = toupper(ReadKbd());
	} while (!(cc == AbbrYes || cc == AbbrNo || cc == _ESC));
	PopW(w);
	return cc;
}

void FindReplaceString(longint First, longint Last)
{
	WORD fst, lst;
	char c;
	if (First >= Last)
	{
		if ((TypeT == MemoT) && TestOptStr('e'))
		{
			SrchT = true; Konec = true;
		}
		return;
	}
	FirstEvent = false;
	SetPart(First); fst = First - Part.PosP; NullChangePart();
label1:
	if (Last > Part.PosP + LenT) lst = LenT - 1; else lst = Last - Part.PosP;
	ChangeP(fst);            /* Background muze volat NextPart */
	if (FindString(fst, lst))
	{
		SetScreen(fst, 0, 0);
		if (Replace)
		{
			if (TestOptStr('n'))
			{
				ReplaceString(fst, fst, lst, Last); UpdStatLine(LineL, Posi);/*BackGround*/
			}
			else {
				FirstEvent = true; Background(); FirstEvent = false;
				c = MyVerifyLL(408, "");
				if (c == AbbrYes) ReplaceString(fst, fst, lst, Last);
				else if (c == _ESC) return;
				;
			}
			if (TestOptStr('g') || TestOptStr('e') || TestOptStr('l')) goto label1;
		}
	}
	else                        /* !FindString */
		if (!AllRd && (Last > Part.PosP + LenT))
		{
			NextPart(); goto label1;
		}
		else
			if (TestOptStr('e') && (TypeT == MemoT))
			{
				SrchT = true; Konec = true;
			}
			else SetScreen(lst, 0, 0);
	/* BackGround; */
}

WORD WordNo(WORD I)
{
	return (CountChar(0x13, 1, MinW(LenT, I - 1)) + 1) / 2;
}

bool WordExist()
{
	return (WordL >= ScrL) && (WordL < ScrL + PageS);
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
		T[k] = 0x13;
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
	while (T[k] != 0x13) { k++; }
	if (k >= LenT) return result;
	WE = k; LI = SetLine(WB);
	result = true;
	return result;
}

void SetWord(WORD WB, WORD WE)
{
	T[WB] = 0x11;
	T[WE] = 0x11;
	SetDekLnCurrI(WB);
	WordL = LineL;
	Posi = WB - LineI + 1;
	Colu = Column(Posi);
}

void HelpLU(char dir)
{
	WORD I = 0, I1 = 0, I2 = 0, h1 = 0, h2 = 0;
	ClrWord();
	h1 = WordNo2();
	if (dir == 'U')
	{
		DekFindLine(LineL - 1); Posi = Position(Colu);
		h2 = MinW(h1, WordNo2() + 1);
	}
	else h2 = h1;
	if (WordFind(h2, I1, I2, I) and (I >= ScrL - 1)) SetWord(I1, I2);
	else
	{
		if (WordFind(h1 + 1, I1, I2, I) && (I >= ScrL)) SetWord(I1, I2);
		else { I1 = SetInd(LineI, Posi); WordL = 0; }
		I = ScrL - 1;
	}
	if (I <= ScrL - 1)
	{
		DekFindLine(ScrL); RollPred();
	}
	if (WordExist()) SetDekLnCurrI(I1);
}

void HelpRD(char dir)
{
	WORD I = 0, I1 = 0, I2 = 0, h1 = 0, h2 = 0;
	ClrWord(); h1 = WordNo2(); if (WordExist()) h1++;
	if (dir == 'D')
	{
		NextLine(false); Posi = Position(Colu);
		while ((Posi > 0) && (Arr[Posi] != 0x13)) Posi--;
		Posi++;
		h2 = MaxW(h1 + 1, WordNo2() + 1);
	}
	else h2 = h1 + 1;
	if (WordFind(h2, I1, I2, I) and (I <= ScrL + PageS)) SetWord(I1, I2);
	else
	{
		if (WordNo2() > h1) h1++;
		if (WordFind(h1, I1, I2, I) and (I <= ScrL + PageS)) SetWord(I1, I2);
		else { I1 = SetInd(LineI, Posi); WordL = 0; }
		I = ScrL + PageS;
	}
	if (I >= ScrL + PageS)
	{
		DekFindLine(ScrL + PageS - 1); RollNext();
	}
	if (WordExist()) SetDekLnCurrI(I1);
}

void HandleEvent() {
	wwmix wwmix1;

	WORD I, I1, I2, I3;
	FILE* F1 = nullptr;
	WORD W1 = 0, W2 = 0, ww = 0;
	longint L1, L2, fs;
	pstring ss;
	int j;
	WORD LastL[161];
	LongStr* sp = nullptr;
	void* P1 = nullptr;
	bool bb;

	ExitRecord er;
	EdExitD* X = nullptr;

	IsWrScreen = false;

	if (!MyGetEvent()) { ClrEvent(); IsWrScreen = false; return; }
	if (!bScroll) CleanFrameM();
	//NewExit(Ovr(), er);
	goto Opet;
	// !!! with Event do:
	if (Event.What == evKeyDown) {
		EdOk = false; ww = Event.KeyCode; ClrEvent();
		X = ExitD;                         // Exit-procedure
		while (X != nullptr) {
			if (TestExitKey(ww, X)) {  // nastavuje i EdBreak
				TestKod(); IndT = SetInd(LineI, Posi);
				ScrT = ((LineL - ScrL + 1) << 8) + Posi - BPos;
				LastTxtPos = IndT + Part.PosP;
				TxtXY = ScrT + (longint(Posi) << 16);
				if (X->Typ == 'Q') {
					KbdChar = ww; Konec = true; EditT = false;
					goto Nic;
				}
				switch (TypeT) {
				case FileT: { TestUpdFile(); ReleaseStore(T); CloseH(TxtFH); break; }
				case LocalT:
				case MemoT: {
					DelEndT(); GetStore(2); Move(T, &T[3], LenT);
					sp = (LongStr*)(T); sp->LL = LenT;
					if (TypeT == LocalT) {
						TWork.Delete(*LocalPPtr);
						LocalPPtr = (longint*)StoreInTWork(sp);
					}
					else if (UpdatT)
					{
						UpdateEdTFld(sp); UpdatT = false;
					}
					ReleaseStore(sp);
					break;
				}
				}
				L2 = SavePar(); screen.CrsHide();
				RestoreExit(er);
				if (TypeT == MemoT) StartExit(X, false);
				else CallProcedure(X->Proc);
				//NewExit(Ovr(), er);
				//goto Opet;
				if (!bScroll) screen.CrsShow(); RestorePar(L2);
				switch (TypeT) {
				case FileT: {
					fs = Part.PosP + IndT;
					OpenTxtFh(Mode); RdFirstPart(); SimplePrintHead();
					while ((fs > Part.PosP + Part.LenP) && !AllRd) { RdNextPart(); }
					IndT = fs - Part.PosP;
					break;
				}
				case LocalT:
				case MemoT:
				{
					if (TypeT == LocalT) sp = TWork.Read(1, *LocalPPtr);
					else {
						CRecPtr = EditDRoot->NewRecPtr;
						sp = _LongS(CFld->FldD);
					}
					LenT = sp->LL; /*T = (CharArr*)(sp)*/; Move(&T[3], &T[1], LenT);
					break;
				}
				}

				WrEndT();
				IndT = MinW(IndT, LenT);
				if (TypeT != FileT)  // !!! with Part do
				{
					AbsLenT = LenT - 1; Part.LenP = AbsLenT; SimplePrintHead();
				}
				SetScreen(IndT, ScrT, Posi);
				if (!bScroll) { screen.CrsShow(); }
				if (!EdOk) { goto Nic; }
			}
			X = X->Chain;

			if (((Mode == SinFM) || (Mode == DouFM) || (Mode == DelFM) || (Mode == NotFM)) && !bScroll)
				FrameStep(FrameDir, ww);
			else
				switch (ww) {
				case _left_: {
					if (Mode == HelpM) HelpLU('L');
					else
						if (bScroll)
						{
							if (BCol > 0) { Colu = BCol; Posi = Position(Colu); }
						}
						else {
							I1 = Posi;
							if (Posi > 1) Posi--;
							BlockLRShift(I1);
						}
					break;
				}
				case _right_: {
					if (Mode == HelpM) HelpRD('R');
					else {
						if (bScroll) {
							Posi = MinI(LineSize, Position(BCol + LineS + 1));
							Colu = Column(Posi);
						}
						else {
							I1 = Posi; if (Posi < LineSize) Posi++;
							BlockLRShift(I1);
						}
					}
					break;
				}
				case _up_: {
					if (Mode == HelpM) HelpLU('U');
					else {
						if (bScroll) if (RScrL == 1) goto Nic;
						L1 = LineAbs(LineL);
						PredLine();
						BlockUDShift(L1);
						if (bScroll) Posi = Position(Colu);
					}
					break;
				}
				case _down_: {
					if (Mode == HelpM) HelpRD('D');
					else {
						L1 = LineAbs(LineL); NextLine(true); BlockUDShift(L1);
						if (bScroll) Posi = Position(Colu);
					}
					break;
				}
				case _PgUp_: {
					if (Mode == HelpM) TestKod();
					else { ClrWord(); LineL = ScrL; }
					L1 = LineAbs(LineL);
					if (bScroll)
					{
						RScrL = MaxL(1, RScrL - PageS);
						if (ModPage(RScrL)) RScrL++;
						ScrL = NewL(RScrL); LineL = ScrL;
						DekFindLine(LineAbs(LineL)); Posi = Position(Colu);
						j = CountChar(0x0C, LineI, ScrI);
						if ((j > 0) && InsPg) {
							DekFindLine(LineAbs(LineL + j));
							ScrL = LineL; RScrL = NewRL(ScrL);
						}
					}
					else
					{
						ScrL -= PageS; DekFindLine(LineAbs(LineL - PageS));
					}
					ChangeScr = true;
					if (Mode == HelpM)
					{
						ScrI = FindLine(ScrL);
						Posi = Position(Colu);
						if (WordFind(WordNo2() + 1, I1, I2, WordL) && WordExist())
							SetWord(I1, I2);
						else WordL = 0;
					}
					else BlockUDShift(L1);
					break;
				}
				case _PgDn_: {
					if (Mode != HelpM) TestKod();
					else {
						ClrWord(); LineL = ScrL;
					}
					L1 = LineAbs(LineL);
					if (bScroll)
					{
						RScrL += PageS; if (ModPage(RScrL)) RScrL--;
						DekFindLine(LineAbs(NewL(RScrL))); Posi = Position(Colu);
						j = CountChar(0x0C, ScrI, LineI);
						if ((j > 0) && InsPg) DekFindLine(LineAbs(LineL - j));
						ScrL = LineL; RScrL = NewRL(ScrL);
					}
					else
					{
						DekFindLine(LineAbs(LineL) + PageS);
						if (LineL >= ScrL + PageS)  ScrL += PageS;
					}
					ChangeScr = true;
					if (Mode == HelpM) {
						ScrI = FindLine(ScrL);
						Posi = Position(Colu); W1 = WordNo2(); I3 = WordL;
						if (WordFind(W1 + 1, I1, I2, WordL) && WordExist()) SetWord(I1, I2);
						else if (WordFind(W1, I1, I2, WordL) && WordExist()) SetWord(I1, I2);
						else WordL = 0;
					}
					else BlockUDShift(L1);
					break;
				}
				case _CtrlLeft_: {
					do {
						Posi--;
						if (Posi == 0)
						{
							I = LineI; PredLine();
							if ((I > 1) || ChangePart) Posi = LastPosLine();
							goto label1;
						}
					} while (Oddel.count(Arr[Posi]) > 0);

					while (!(Oddel.count(Arr[Posi]) > 0))
					{
						Posi--;
						if (Posi == 0) goto label1;
					}
				label1:
					Posi++;
					break;
				}
				case _CtrlRight_:
				{
					while (!(Oddel.count(Arr[Posi]) > 0))
					{
						Posi++; if (Posi > LastPosLine()) goto label2;
					}
					while (Oddel.count(Arr[Posi]) > 0)
					{
						Posi++;
						I = LastPosLine();
						if (Posi > I)
							if ((NextI <= LenT) && ((I == 0) || (Posi > I + 1)))
							{
								NextLine(true); Posi = 1;
							}
							else { Posi = I + 1; goto label2; }
					}
				label2:
					break; }
				case _Z_: { RollNext(); break; }
				case _W_: { RollPred(); break; }
				case _Home_: {
					I1 = Posi; Posi = 1; if (Wrap) Posi = MaxI(LeftMarg, 1);
					BlockLRShift(I1);
					break;
				}
				case _End_: {
					I1 = Posi; Posi = LastPosLine(); if (Posi < LineSize) Posi++;
					BlockLRShift(I1);
					break;
				}
				case _QE_: { TestKod(); LineL = ScrL; LineI = ScrI; DekodLine(); break; }
				case _QX_: { TestKod(); DekFindLine(LineAbs(ScrL + PageS - 1)); break; }
				case _CtrlPgUp_: { TestKod(); SetPart(1); SetScreen(1, 0, 0); break; }
				case _CtrlPgDn_: {
					TestKod(); SetPart(AbsLenT - Part.LenP + LenT);
					SetScreen(LenT, 0, 0); break;
				}
				case _CtrlF3_: {
					ss = ""; TestKod();
					do {
						if (MyPromptLL(420, &ss)) goto Nic;
						val(ss, L1, I);
					} while (!(L1 > 0));
					DekFindLine(L1);
					break;
				}
							 // *****************************************
							 // tady byly pùvodnì *********CHAR**********
							 // *****************************************
				case _M_: {
					if (Mode == HelpM) { Konec = WordExist(); KbdChar = ww; }
					else
					{
						if ((NextI >= LenT) && !AllRd) NextPartDek();
						if ((NextI > LenT) || Insert)
						{
							NewLine('m'); Posi = 1; ClrEol();
							if (LineL - ScrL == PageS)
							{
								screen.GotoXY(1, 1); MyDelLine(); ScrL++; ChangeScr = true;
							}
							else { screen.GotoXY(1, succ(LineL - ScrL)); MyInsLine(); }
							if (Indent)
							{
								I1 = SetPredI(); I = I1;
								while ((T[I] == ' ') && (T[I] != _CR)) { I++; }
								if (T[I] != _CR) Posi = I - I1 + 1;
							}
							else if (Wrap) Posi = LeftMarg;
							if (TestLastPos(1, Posi)) FillChar(&Arr[1], Posi - 1, 32);
						}
						else if (NextI <= LenT) { NextLine(true); Posi = 1; }
					}
					break;
				}
				case _N_: { NewLine('n'); ClrEol(); screen.GotoXY(1, LineL - ScrL + 2); MyInsLine(); break; }

				case _Ins_: { Insert = !Insert; break; }
				case _Del_:
				case _G_: {
					if (Posi <= LastPosLine()) DelChar();
					else DeleteL(); break;
				}
				case _H_: {
					if (Posi > 1) { Posi--; DelChar(); }
					else {
						if ((LineL == 1) && (Part.PosP > 0)) PredPart();
						if (LineI > 1)
						{
							TestKod(); LineL--;
							if (T[LineI - 1] == _LF) SetDekCurrI(LineI - 2);
							else SetDekCurrI(LineI - 1);
							Posi = MinW(255, succ(LastPosLine()));
							DeleteL();
							if (LineL < ScrL) { ScrL--; ChangeScr = true; }
						}
					}
					break;
				}
				case _Y_: {
					if ((NextI >= LenT) && !AllRd) NextPartDek();
					NextI = MinW(NextI, LenT); TestLenText(NextI, LineI);
					if (BegBLn > LineAbs(LineL)) BegBLn--;
					else if (BegBLn == LineAbs(LineL)) if (TypeB == TextBlock) BegBPos = 1;
					if (EndBLn >= LineAbs(LineL))
						if ((EndBLn == LineAbs(LineL)) && (TypeB == TextBlock)) BPos = 1;
						else EndBLn--;
					MyDelLine(); DekodLine(); Posi = 1;
					break;
				}
				case _T_: {
					if (Posi > LastPosLine()) DeleteL();
					else
					{
						I = Posi;
						if (Oddel.count(Arr[Posi]) > 0) DelChar();
						else while ((I <= LastPosLine()) && !(Oddel.count(Arr[Posi]) > 0)) { I++; }
						while ((I <= LastPosLine()) && (Arr[I] == ' ')) { I++; }
						// TODO: k èemu to tady je? if ((I>Posi) and TestLastPos(I,Posi))
					}
					break;
				}
				case _QI_: { Indent = !Indent; break; }
				case _QL_: { if (UpdatedL) DekodLine(); break; }
				case _QY_: {if (TestLastPos(LastPosLine() + 1, Posi)) ClrEol(); break; }
				case _QF_:
				case _QA_: {
					Replace = false;
					if (MyPromptLL(405, &FindStr)) goto Nic;
					if (ww == _QA_)
					{
						if (MyPromptLL(407, &ReplaceStr)) goto Nic;
						Replace = true;
					}
					ss = OptionStr; if (MyPromptLL(406, &ss)) goto Nic; OptionStr = ss;
					TestKod();
					if (TestOptStr('l') && (!BlockExist() || (TypeB = ColBlock))) goto Nic;
					if (TestOptStr('l')) SetBlockBound(L1, L2);
					else {
						L2 = AbsLenT - Part.LenP + LenT;
						if (TestOptStr('g') || TestOptStr('e'))  L1 = 1;
						else L1 = Part.PosP + SetInd(LineI, Posi);
					}
					FindReplaceString(L1, L2); if (ww == _QA_) DekodLine();
					if (!Konec) { FirstEvent = false; Background(); }
					break;
				}
				case _L_: {
					if (FindStr != "")
					{
						TestKod();
						if (TestOptStr('l') && (!BlockExist() || (TypeB == ColBlock))) goto Nic;
						fs = 1; L1 = Part.PosP + SetInd(LineI, Posi);
						if (TestOptStr('l'))  SetBlockBound(fs, L2);
						else L2 = AbsLenT - Part.LenP + LenT;
						if (L1 < fs)  L1 = fs;  // { if L1>=L2  goto Nic;}
						FindReplaceString(L1, L2);
						if (!Konec) { FirstEvent = false; Background(); };
					}
					break;
				}
				case _I_: {
					I1 = SetPredI() + Posi;
					if (I1 >= LineI - 1) goto Nic;
					I = I1;
					while ((T[I] != ' ') && (T[I] != _CR)) { I++; }
					while (T[I] == ' ') { I++; }
					I2 = I - I1 + 1;
					if (TestLastPos(Posi, Posi + I2)) FillChar(&Arr[Posi], I2, 32);
					Posi += I2;
					break;
				}
				case _J_: {
					I1 = SetPredI() + Posi - 2;
					if ((I1 >= LineI - 1) || (I1 == 0)) goto Nic;
					I = I1; while (T[I] == ' ') { I++; }
					while ((T[I] != ' ') && (T[I] != _CR)) { I++; }
					if (I == I1) goto Nic;
					I2 = I - I1 - 1;
					I = Posi;
					Posi--;
					while ((Posi > 0) and (Arr[Posi] != ' ')) { Posi--; }
					Posi++;
					if (TestLastPos(Posi, Posi + I2)) FillChar(&Arr[Posi], I2, 32);
					Posi = I + I2 + 1;
					break;
				}
				case _QB_: {
					TestKod();
					PosDekFindLine(BegBLn, MinW(LastPosLine() + 1, BegBPos), false); break; }
				case _QK_: {
					TestKod();
					PosDekFindLine(EndBLn, MinW(LastPosLine() + 1, EndBPos), false); break; }
				case _KB_:
				case _F7_:
				case _KH_:
				{
					BegBLn = LineAbs(LineL);
					if (TypeB == TextBlock) BegBPos = MinI(LastPosLine() + 1, Posi);
					else BegBPos = Posi;
					if (ww == _KH_) goto OznB;
					break;
				}
				case _KK_:
				case _F8_: {
				OznB:
					EndBLn = LineAbs(LineL);
					if (TypeB == TextBlock) EndBPos = MinI(LastPosLine() + 1, Posi);
					else EndBPos = Posi;
					break;
				}
				case _KN_: {
					if (TypeB == TextBlock) TypeB = ColBlock;
					else TypeB = TextBlock;
					break;
				}
				case _KY_: {
					if (BlockHandle(fs, F1, 'Y')) { EndBLn = BegBLn; EndBPos = BegBPos; };
					break;
				}
				case _KC_: BlockCopyMove('C', P1, sp); break;
				case _KV_: BlockCopyMove('M', P1, sp); break;
				case _KU_: BlockHandle(fs, F1, 'U'); break;
				case _KL_: BlockHandle(fs, F1, 'L'); break;
				case _CtrlF7_: {
					if (TypeB == TextBlock) BlockGrasp('G', P1, sp);
					else BlockCGrasp('G', P1, sp);
					break;
				}
				case _KW_: {
					I1 = BegBLn; I2 = BegBPos; I3 = EndBLn; I = EndBPos; bb = TypeB;
					if (!BlockExist())
					{
						BegBLn = 1; EndBLn = 0x7FFF; BegBPos = 1; EndBPos = 0xFF; TypeB = TextBlock;
					}
					CPath = wwmix1.SelectDiskFile(".TXT", 401, false);
					if (CPath == "")  goto Nic;
					CVol = "";
					F1 = OpenH(_isnewfile, Exclusive);
					if (HandleError == 80)
					{
						SetMsgPar(CPath);
						if (PromptYN(780)) F1 = OpenH(_isoverwritefile, Exclusive);
						else goto Nic;
					}
					if (HandleError != 0) { MyWrLLMsg(CPath); goto Nic; }
					fs = 0; // {L1 =LineAbs(LineL);I =Posi;}
					if (BlockHandle(fs, F1, 'W'))
					{
						WriteH(F1, 0, T); /*truncH*/ CloseH(F1); HMsgExit(CPath);
					}
					// { PosDekFindLine(L1,I,true); }
					BegBLn = I1; BegBPos = I2; EndBLn = I3; EndBPos = I; TypeB = bb;
					break;
				}
				case _ShiftF7_: {
					if (TypeB == TextBlock) BlockDrop('D', P1, sp);
					else BlockCDrop('D', P1, sp);
					break;
				}
				case _KR_: {
					CPath = wwmix1.SelectDiskFile(".TXT", 400, false);
					if (CPath == "") goto Nic;
					CVol = ""; F1 = OpenH(_isoldfile, RdOnly);
					if (HandleError != 0) { MyWrLLMsg(CPath); goto Nic; }
					BegBLn = Part.LineP + LineL; BegBPos = Posi;
					L1 = Part.PosP + LineI + Posi - 1;
					FillBlank();
					fs = FileSizeH(F1); L2 = 0;
					NullChangePart();
					switch (TypeB) {
					case TextBlock: {
						do {
							I2 = 0x1000; if (fs - L2 < longint(I2))  I2 = fs - L2;
							if ((TypeT != FileT) && ((I2 >= MaxLenT - LenT) || (I2 >= StoreAvail())))
							{
								if (I2 >= StoreAvail())  I2 = StoreAvail();
								I2 = MinW(I2, MaxLenT - LenT) - 2; fs = L2 + I2;
								WrLLF10Msg(404);
							}
							I1 = L1 + L2 - Part.PosP;
							TestLenText(I1, longint(I1) + I2);
							if (ChangePart) I1 -= Part.MovI;
							SeekH(F1, L2); ReadH(F1, I2, &T[I1]); HMsgExit("");
							L2 += I2;
						} while (L2 != fs);
						I = L1 + L2 - Part.PosP;
						if (T[I - 1] == 0x1A) { TestLenText(I, I - 1); I--; }
						SetDekLnCurrI(I); EndBLn = Part.LineP + LineL; EndBPos = succ(I - LineI);
					}
					case ColBlock: {
						EndBPos = Posi; I2 = 0x1000;
						MarkStore2(P1); sp = (LongStr*)GetStore2(I2 + 2); //ww =BegBPos;}
						do {
							if (fs - L2 < longint(I2)) I2 = fs - L2;
							SeekH(F1, L2); ReadH(F1, I2, sp->A); HMsgExit("");
							L2 += I2; sp->LL = I2; BlockCDrop('R', P1, sp);
						} while (L2 != fs);
						EndBLn = Part.LineP + LineL - 1; ReleaseStore2(P1);
					}
					}
					CloseH(F1); HMsgExit("");
					SetPartLine(BegBLn); SetDekLnCurrI(L1 - Part.PosP); UpdatedL = true;
					break;
				} // end case _KR_
				case _KP_: { if (!BlockHandle(fs, F1, 'P'))
				{
					I1 = BegBLn; I2 = BegBPos; I3 = EndBLn; I = EndBPos; bb = TypeB;
					BegBLn = 1; EndBLn = 0x7FFF; BegBPos = 1; EndBPos = 0xFF;
					TypeB = TextBlock;
					BegBLn = I1; BegBPos = I2; EndBLn = I3; EndBPos = I; TypeB = bb;
					break;
				}
				case _KF_: {
					if (BlockExist() && (TypeB == TextBlock))
					{
						TestKod(); screen.CrsHide();
						SetPartLine(EndBLn); I2 = EndBLn - Part.LineP;
						L1 = SetInd(FindLine(integer(I2)), EndBPos) + Part.PosP;
						L2 = BegBLn; Posi = BegBPos; SetPartLine(L2); I2 = BegBLn - Part.LineP;
						Format(I, FindLine(integer(I2)) + Part.PosP, L1, BegBPos, true);
						DekFindLine(L2);
						if (!bScroll) screen.CrsShow();
					}
					break;
				}
				case _OJ_: { Just = !Just; break; }
				case _OW_: {
					Wrap = !Wrap;
					if (Wrap) { LineS--; LastC--; }
					else {
						LastC++; LineS++;
						screen.ScrRdBuf(FirstC - 1, TxtRows - 1, &LastL[1], LineS);
						LastL[MargLL[0]] = MargLL[1];
						LastL[MargLL[2]] = MargLL[3];
						screen.ScrWrBuf(FirstC - 1, TxtRows - 1, &LastL[1], LineS);
					}
					break;
				}
				case _OL_: {       // LeftMarg
					do {
						str(Posi, ss);
						if (MyPromptLL(410, &ss)) goto Nic;
						val(ss, I1, I);
					} while (!((I1 < RightMarg) && (I1 > 0)));
					LeftMarg = I1;
					break;
				}
				case _OR_: {       //RightMarg
					do {
						str(Posi, ss);
						if (MyPromptLL(409, &ss)) goto Nic;
						val(ss, I1, I); // inc(I1);
					} while (!((I1 <= 255) && (LeftMarg < I1)));
					RightMarg = I1;
					break;
				}
				case _OC_: {
					I1 = 1; while ((I1 < LastPosLine()) && (Arr[I1] == ' ')) { I1++; }
					I2 = LastPosLine(); while ((I2 > 1) && (Arr[I2] == ' ')) { I2--; }
					j = (LeftMarg + (RightMarg - LeftMarg) / 2) - int(I1 + (I2 - I1) / 2);
					if ((I2 < I1) || (j == 0)) goto Nic;
					if (j > 0)
					{
						if (TestLastPos(1, j + 1)) FillChar(&Arr[1], j, 32);
					}
					else {
						j = MinI(-j, I1 - 1);
						TestLastPos(j + 1, 1);
					}
					Posi = MinW(LineSize, LastPosLine() + 1);
					break;
				}
				case _B_: {
					TestKod(); L1 = Part.PosP + LineI;
					Format(I, L1, AbsLenT + LenT - Part.LenP, MinI(LeftMarg, Posi), false);
					SetPart(L1); I2 = L1 - Part.PosP; SetDekLnCurrI(I2); Posi = 1;
					break;
				}
				case _framesingle_: { Mode = SinFM; screen.CrsBig(); FrameDir = 0; break; }
				case _framedouble_: { Mode = DouFM; screen.CrsBig(); FrameDir = 0; break; }
				case _delframe_: { Mode = DelFM; screen.CrsBig(); FrameDir = 0; break; }
				case _F4_: {
					W1 = ToggleCS(Arr[Posi]);
					UpdatedL = W1 != Arr[Posi];
					Arr[Posi] = W1;
					break;
				}
				case _CtrlF5_:
					Calculate(); break;
				case _AltF8_: {
					L2 = SavePar();
					W1 = Menu(45, spec.KbdTyp + 1);
					if (W1 != 0) spec.KbdTyp = TKbdConv(W1 - 1);
					RestorePar(L2);
					break;
				}
				case _CtrlF6_: {
					if ((TypeT == FileT) || (TypeT == LocalT)) BlockHandle(fs, F1, 'p');
					break;
				}

				case 0x1000: {
				Opet:
					if ((Mode != HelpM) && (Mode != ViewM) && Wrap)
						screen.Window(FirstC, FirstR + 1, LastC + 1, LastR);
					else screen.Window(FirstC, FirstR + 1, LastC, LastR);

					if (!Scroll) screen.CrsShow();
					SetScreen(LineI, 0, 0); }

				case _U_: {
					if (TypeT != FileT)
						if (PromptYN(108))
						{
							IndT = 1; KbdChar = _U_; Konec = true; EdBreak = 0xFFFF;

						}
					break;
				}
				default:
				{
					if (ww >= 0x1000 && ww <= 0x101F) {
						WrChar(Lo(ww)); // ***CTRL-klavesy***
						if (ww == 0x100D) { TestKod(); DekodLine(); Posi--; }
					}
					if (ww >= 0x0020 && ww <= 0x00FF) { // *********CHAR********** }
						WrChar(Lo(ww));
						if (Wrap) if (Posi > RightMarg + 1)
						{
							W1 = Arr[Posi]; Arr[Posi] = 0xFF; KodLine();
							I1 = LeftMarg; while (Arr[I1] == ' ') I1++;
							if (I1 > RightMarg) I1 = RightMarg;
							L1 = Part.PosP + LineI;
							Format(I, L1, AbsLenT + LenT - Part.LenP, I1, false);
							SetPart(L1); I = 1;
							I = FindChar(I, 0xFF, 1, LenT); T[I] = W1;
							SetDekLnCurrI(I); Posi = I - LineI + 1;
						}
					}
					if (((Lo(ww) == 0x00) && (Breaks.first(Hi(ww)) != 0))
						|| ((ww == _AltEqual_) && (TypeT != FileT)))
					{
						TestKod(); KbdChar = ww; Konec = true; EdBreak = 0xFFFF;
					}
					else if (ww == _ESC_)
					{
						TestKod(); KbdChar = ww; Konec = true; EdBreak = 0;
					}
					break;
					// ***ERROR TESTLENTEXT***
				}
				}
				}
		}
	}
	else if ((Event.What == evMouseDown) && ((Mode == HelpM) || (Mode == TextM)))
	{
		if (Mode == TextM) TestKod();
		if (!((Event.Where.Y >= FirstR && Event.Where.Y <= LastR - 1)
			&& (Event.Where.X >= FirstC - 1 && Event.Where.X <= LastC - 1)))
		{
			ClrEvent();
			goto Nic;
		}
		I3 = LineI; j = Posi;
		W1 = Event.Where.Y - WindMin.Y + ScrL;
		if (Mode == HelpM) W2 = WordNo2() + 1;
		DekFindLine(LineAbs(W1));
		Posi = Event.Where.X - WindMin.X + 1;
		if (Mode != TextM) Posi = Position(Posi);
		Posi += BPos;
		I = SetInd(LineI, Posi);
		if (I < LenT)
		{
			if (Mode == HelpM)
			{
				ClrWord();
				WordFind(WordNo(I + 1), I1, I2, W1);
				if ((I1 <= I) && (I2 >= I)) {
					SetWord(I1, I2); KbdChar = _M_;
					Konec = true;
				}
				else if (WordExist())
				{
					WordFind(W2, I1, I2, W1);
					SetWord(I1, I2);
				}
				else SetDekLnCurrI(I3);
			}
		}
		else { SetDekLnCurrI(I3); Posi = j; }
		ClrEvent();
	}
	else ClrEvent();

Nic:
	//ClrEvent;
	RestoreExit(er); IsWrScreen = false;

}

void CursorWord()
{
	std::set<char> O;
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

void Edit(WORD SuccLineSize)
{
	// {line descriptor LineI, Posi, BPos}
	char* Arr = nullptr;
	WORD NextI = 0;
	int LineL = 0, ScrL = 0;
	longint RScrL = 0;
	bool UpdatedL = false, CtrlL = false, HardL = false;
	// {screen descriptor  ScrI}
	WORD BCol = 0, Colu = 0, Row = 0;
	bool ChangeScr = false;
	ColorOrd ColScr;
	bool IsWrScreen = false;

	WORD FirstR = 0, FirstC = 0, LastR = 0, LastC = 0, MinC = 0, MinR = 0, MaxC = 0, MaxR = 0;
	WORD MargLL[4] = { 0,0,0,0 };
	WORD PageS = 0, LineS = 0;

	bool Scroll = false, FirstScroll = false, HelpScroll = false;
	longint PredScLn = 0;
	WORD PredScPos = 0; // {pozice pred Scroll}
	BYTE FrameDir = 0;

	WORD WordL = 0; // {Mode=HelpM & ctrl-word is on screen}
	bool Konec = false;

	// registers Regs;
	WORD i1 = 0, i2 = 0, i3 = 0;

	// *** zaèátek metody ***
	InitScr();
	IsWrScreen = false;
	WrEndT();
	IndT = min((WORD)max(1, (int)IndT), LenT);
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
	
	bool keybScroll = GetKeyState(VK_SCROLL) & 0x0001;
	FirstScroll = Mode == ViewM;
	Mode = ViewM;
	bScroll = (keybScroll || FirstScroll) && (Mode != HelpM);
	if (bScroll)
	{
		ScrL = NewL(RScrL);
		ChangeScr = true;
	}
	HelpScroll = bScroll || (Mode == HelpM);
	if (HelpScroll) { screen.CrsHide(); }
	else { screen.CrsNorm(); }
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
		TxtXY = ScrT + (longint(Posi) << 16);
		CursorWord();
		if (Mode == HelpM) { ClrWord(); }
	}
	screen.CrsHide();
	screen.Window(MinC, MinR, MaxC, MaxR);
	TestUpdFile();
}

void SetEditTxt(Instr* PD)
{
	if (PD->Insert != nullptr) Insert = !RunBool(PD->Insert);
	if (PD->Indent != nullptr) Indent = RunBool(PD->Indent);
	if (PD->Wrap != nullptr) Wrap = RunBool(PD->Wrap);
	if (PD->Just != nullptr) Just = RunBool(PD->Just);
	if (PD->ColBlk != nullptr) TypeB = RunBool(PD->ColBlk);
	if (PD->Left != nullptr) LeftMarg = MaxI(1, RunInt(PD->Left));
	if (PD->Right != nullptr) RightMarg = MaxI(LeftMarg, MinI(255, RunInt(PD->Right)));
}

void GetEditTxt(bool& pInsert, bool& pIndent, bool& pWrap, bool& pJust, bool& pColBlk, integer& pLeftMarg,
	integer& pRightMarg)
{
	pInsert = Insert; pIndent = Indent; pWrap = Wrap; pJust = Just; pColBlk = TypeB;
	pLeftMarg = LeftMarg; pRightMarg = RightMarg;
}

bool EditText(char pMode, char pTxtType, pstring pName, pstring pErrMsg, char* pTxtPtr, WORD pMaxLen, WORD& pLen,
	WORD& pInd, longint pScr, pstring pBreaks, EdExitD* pExD, bool& pSrch, bool& pUpdat, WORD pLastNr,
	WORD pCtrlLastNr, MsgStrPtr pMsgS)
{
	bool oldEdOK;
	oldEdOK = EdOk; EditT = true;
	Mode = pMode; TypeT = pTxtType; NameT = pName; ErrMsg = pErrMsg;
	/*T = pTxtPtr*/; MaxLenT = pMaxLen;
	LenT = pLen; IndT = pInd;
	ScrT = pScr & 0xFFFF;
	Posi = pScr >> 16;
	Breaks = pBreaks;
	ExitD = pExD;
	SrchT = pSrch; UpdatT = pUpdat;
	LastNr = pLastNr; CtrlLastNr = pCtrlLastNr;
	if (pMsgS != nullptr)
	{
		LastS = pMsgS->Last; CtrlLastS = pMsgS->CtrlLast;
		ShiftLastS = pMsgS->ShiftLast; AltLastS = pMsgS->AltLast;
		HeadS = *pMsgS->Head;
	}
	else
	{
		/*LastS = nullptr; CtrlLastS = nullptr; ShiftLastS = nullptr; AltLastS = nullptr;
		HeadS = nullptr;*/
		LastS = ""; CtrlLastS = ""; ShiftLastS = ""; AltLastS = ""; HeadS = "";
	}
	if (Mode != HelpM) TxtColor = TextAttr;
	FirstEvent = !SrchT;
	if (SrchT)
	{
		SrchT = false;
		pstring OldKbdBuffer = KbdBuffer;
		KbdBuffer = 0x0C;
		KbdBuffer += OldKbdBuffer;
		KbdChar = _L_;
		IndT = 0;
	}

	Edit(0);
	if (Mode != HelpM) TextAttr = TxtColor;
	pUpdat = UpdatT; pSrch = SrchT; pLen = LenT;
	pInd = IndT; pScr = ScrT + (longint(Posi) << 16);
	EdOk = oldEdOK;
	return EditT;
}

void SimpleEditText(char pMode, pstring pErrMsg, pstring pName, char* TxtPtr, WORD MaxLen, WORD& Len, WORD& Ind,
	bool& Updat)
{
	bool Srch; longint Scr;
	Srch = false; Scr = 0;
	EditText(pMode, LocalT, pName, pErrMsg, TxtPtr, MaxLen, Len, Ind, Scr, "", nullptr, Srch, Updat, 0, 0, nullptr);
}

WORD FindTextE(const pstring& Pstr, pstring Popt, CharArr* PTxtPtr, WORD PLen)
{
	CharArrPtr tt = (CharArr*)&T; /*T = (char*)PTxtPtr;*/
	pstring f = FindStr; pstring o = OptionStr;
	bool r = Replace;
	FindStr = Pstr; OptionStr = Popt; Replace = false;
	WORD I = 1;
	WORD result;
	if (FindString(I, PLen + 1)) result = I;
	else result = 0;
	FindStr = f; OptionStr = o; Replace = r; /*T = tt;*/
	return result;
}

void EditTxtFile(longint* LP, char Mode, pstring& ErrMsg, EdExitD* ExD,
	longint TxtPos, longint Txtxy, WRect* V,
	WORD Atr, const pstring Hd, BYTE WFlags, MsgStrPtr MsgS)
{
	bool Srch = false, Upd = false;
	longint Size = 0, L = 0;
	longint w1 = 0;
	ExitRecord er;
	bool Loc = false;
	WORD Ind = 0, oldInd = 0;
	longint oldTxtxy = 0;
	LongStr* LS = nullptr; pstring compErrTxt;

	if (Atr == 0) Atr = colors.tNorm;
	longint w2 = 0; longint w3 = 0;
	if (V != nullptr) {
		w1 = PushW1(1, 1, TxtCols, 1, (WFlags & WPushPixel) != 0, false);
		w2 = PushW1(1, TxtRows, TxtCols, TxtRows, (WFlags & WPushPixel) != 0, false);
		w3 = PushWFramed(V->C1, V->R1, V->C2, V->R2, Atr, Hd, "", WFlags);
	}
	else
	{
		w1 = PushW(1, 1, TxtCols, TxtRows);
		TextAttr = Atr;
	}
	//NewExit(Ovr(), er);
	//goto label4;
	Loc = (LP != nullptr);
	LocalPPtr = LP;
	if (!Loc)
	{
		MaxLenT = 0xFFF0; LenT = 0; Part.UpdP = false;
		TxtPath = CPath; TxtVol = CVol;
		// zaèátek práce se souborem
		OpenTxtFh(Mode);
		RdFirstPart();
		SimplePrintHead();
		while ((TxtPos > Part.PosP + Part.LenP) && !AllRd) RdNextPart();
		Ind = TxtPos - Part.PosP;
	}
	else {
		LS = TWork.Read(1, *LP);
		Ind = TxtPos; L = StoreInTWork(LS);
	}
	oldInd = Ind; oldTxtxy = Txtxy;

label1:
	Srch = false; Upd = false;
	if (!Loc)
		EditText(Mode, FileT, TxtPath, ErrMsg, ppa, 0xFFF0, LenT, Ind, Txtxy,
			_F1 + _F6 + _F9 + _AltF10, ExD,
			Srch, Upd, 126, 143, MsgS);
	else EditText(Mode, LocalT, "", ErrMsg, (char*)&LS->A, MaxLStrLen, LS->LL, Ind, Txtxy,
		_F1 + _F6, ExD, Srch, Upd, 126, 143, MsgS);
	TxtPos = Ind + Part.PosP;
	if (Upd) EdUpdated = true;
	if ((KbdChar == _AltEqual_) || (KbdChar == _U_))
	{
		ReleaseStore(LS); LS = TWork.Read(1, L);
		if (KbdChar == _AltEqual_) { KbdChar = _ESC_; goto label4; }
		else { Ind = oldInd; Txtxy = oldTxtxy; goto label1; }
	}
	if (!Loc) ReleaseStore(T);
	if (EdBreak == 0xFFFF)
		switch (KbdChar) {
		case _F9_: {
			if (Loc) { TWork.Delete(*LP); *LP = StoreInTWork(LS); }
			else RdPart();
			goto label1;
		}
		case _AltF10_: { Help(nullptr, "", false); goto label2; }
		case _F1_: {
			RdMsg(6); Help((RdbDPtr)&HelpFD, MsgLine, false);
		label2:
			if (!Loc) RdPart(); goto label1; }
		}
	if (!Loc) { Size = FileSizeH(TxtFH); CloseH(TxtFH); }
	if ((EdBreak == 0xFFFF) && (KbdChar == _F6_))
		if (Loc) { PrintArray(T, LenT, false); goto label1; }
		else {
			CPath = TxtPath; CVol = TxtVol; PrintTxtFile(0);
			OpenTxtFh(Mode); RdPart(); goto label1;
		}
	if (!Loc && (Size < 1)) MyDeleteFile(TxtPath);
	if (Loc and (KbdChar == _ESC_)) LS->LL = LenT;
label4:
	if (IsCompileErr) {
		IsCompileErr = false;
		compErrTxt = MsgLine;
		SetMsgPar(compErrTxt); WrLLF10Msg(110);
	}
	if (Loc)
	{
		TWork.Delete(L);
		TWork.Delete(*LP); *LP = StoreInTWork(LS);
		ReleaseStore(LS);
	}
	if (w3 != 0) PopW2(w3, (WFlags & WNoPop) == 0); if (w2 != 0) PopW(w2);
	PopW(w1);
	LastTxtPos = Ind + Part.PosP;
	RestoreExit(er);
}

void ViewPrinterTxt()
{
	WRect V = { 1, 2, 80, 24 };
	if (!PrintView) return;
	SetPrintTxtPath();
	V.C2 = TxtCols; V.R2 = TxtRows - 1;
	pstring temp(1);
	EditTxtFile(nullptr, 'T', temp, nullptr, 1, 0, &V, 0, "", WPushPixel, nullptr);
}

void Help(RdbDPtr R, pstring Name, bool InCWw)
{
	void* p = nullptr; ExitRecord er; FileDPtr fd = nullptr;
	WORD c1, c2, r1, r2; longint w, w2; WORD i, l, l2; WORD iRec, oldIRec;
	LongStr* s = nullptr; LongStr* s2 = nullptr;
	WORD* os = (WORD*)s; WORD* os2 = (WORD*)s2;
	integer delta; bool frst, byName, backw;
	FileDPtr cf, cf2;

	if (R == nullptr) {
		if (iStk == 0) return; R = Stk[iStk].Rdb; backw = true;
	}
	else { if (Name == "") return; backw = false; }
	if (R == (RdbD*)&HelpFD) {
		fd = &HelpFD; if (HelpFD.Handle == nullptr) { WrLLF10Msg(57); return; }
	}
	else { fd = R->HelpFD; if (fd == nullptr) return; }
	MarkStore(p); cf = CFile; w = 0; w2 = 0;
	//NewExit(Ovr(), er);
	goto label4;
	if (InCWw) {
		c1 = WindMin.X + 1; c2 = WindMax.X + 1; r1 = WindMin.Y + 1; r2 = WindMax.Y + 1;
		if ((c1 == 1) && (c2 == TxtCols) && (r1 == 2) && (r2 == TxtRows)) r1 = 1;
		w = PushW(1, TxtRows, TxtCols, TxtRows);
		w2 = PushW1(c1, r1, c2, r2, true, true);
	}
	else w = PushW1(1, 1, TxtCols, TxtRows, true, true);
	i = 1; frst = true; delta = 0;
	if (backw) { byName = false; goto label3; }
label1:
	byName = true;
label2:
	s = GetHlpText(R, Name, byName, iRec); cf2 = CFile;
	if (s == nullptr)
		if (frst && (R == (RdbD*)(&HelpFD)) && (KbdChar == _CtrlF1_)) {
			KbdChar = 0; Name = "Ctrl-F1 error"; goto label1;
		}
		else { Set2MsgPar(Name, fd->Name); WrLLF10Msg(146); }
	else {
		frst = false; byName = false;
		s2 = s;
		if ((s->LL > 0) && (s->A[1] == '{')) {
			{ //view after 1. line
				l = FindCtrlM(s, 1, 1);
				l = SkipCtrlMJ(s, l) - 1;
				os2 += l; s2->LL = s->LL - l;
			}
			if (s2->LL == 0) { if (delta == 0) goto label4; }
			else if (iRec != oldIRec) {
				oldIRec = iRec; iRec += delta; goto label2;
			}
			ViewHelpText(s2, i);
			if (iStk < maxStk) iStk++;
			else Move(&Stk[2], Stk, sizeof(Stk) - 4);
			/* !!! with Stk[iStk] do!!! */
			Stk[iStk].Rdb = R; Stk[iStk].FD = cf2; Stk[iStk].iR = iRec; Stk[iStk].iT = i;
			oldIRec = iRec; i = 1; delta = 0; ReleaseStore(s); CFile = cf2;
			switch (KbdChar) {
			case _ESC_: break;
			case _F10_: {
				iStk--;
			label3:
				if (iStk > 0) {
					/* !!! with Stk[iStk] do!!! */
					R = Stk[iStk].Rdb; CFile = Stk[iStk].FD; iRec = Stk[iStk].iR; i = Stk[iStk].iT;
					iStk--;
					goto label2;
				}
			}
			case _CtrlHome_: { iRec--; delta = -1; goto label2; }
			case _CtrlEnd_: { iRec++; delta = 1; goto label2; }
			default:
				if (KbdChar == _F1_) Name = "root"; else Name = LexWord; goto label1;
			}
		}
	label4:
		RestoreExit(er);
		if (w2 != 0) PopW(w2); if (w != 0) PopW(w);
		ReleaseStore(p); CFile = cf;
	}
}

void ViewHelpText(LongStr* S, WORD& TxtPos)
{
	longint L = SavePar();
	TxtColor = colors.hNorm; FillChar(ColKey, 8, colors.tCtrl);
	ColKey[5] = colors.hSpec; ColKey[3] = colors.hHili; ColKey[1] = colors.hMenu;
	bool Srch = false; bool Upd = false; longint Scr = 0;
label1:
	EditText(HelpM, MemoT, "", "", &S->A, 0xFFF0, S->LL, TxtPos, Scr,
		_F1 + _F10 + _F6 + _CtrlHome + _CtrlEnd, nullptr, Srch, Upd, 142, 145, nullptr);
	if (KbdChar == _F6_) { PrintArray(&S->A, S->LL, true); goto label1; }
	RestorePar(L);
}

void ClearHelpStkForCRdb()
{
	WORD i = 1;
	while (i <= iStk)
		if (Stk[i].Rdb == CRdb) { Move(&Stk[i + 1], &Stk[i], (iStk - i) * 12); iStk--; }
		else i++;
}

void InitTxtEditor()
{
	FindStr[0] = 0; ReplaceStr[0] = 0; OptionStr[0] = 0; Replace = false;
	TxtColor = colors.tNorm; BlockColor = colors.tBlock; SysLColor = colors.fNorm;
	ColKey[0] = colors.tCtrl;
	Move(&colors.tUnderline, &ColKey[1], 7);
	RdMsg(411); InsMsg = MsgLine;
	RdMsg(412); nInsMsg = MsgLine;
	RdMsg(413); IndMsg = MsgLine;
	RdMsg(414); WrapMsg = MsgLine;
	RdMsg(415); JustMsg = MsgLine;
	RdMsg(417); BlockMsg = MsgLine;
	RdMsg(416); ViewMsg = MsgLine;
	Insert = true; Indent = true; Wrap = false; Just = false; TypeB = false;
	LeftMarg = 1; RightMarg = 78;
	CharPg = /*char(250)*/ spec.TxtCharPg; InsPg = /*true*/ spec.TxtInsPg;
}
