#pragma once

#include "OldEditor.h"
#include <set>
#include <stdexcept>
#include "EditorEvents.h"
#include "runedi.h"
#include "../cppfand/compile.h"
#include "EditorHelp.h"
#include "../cppfand/GlobalVariables.h"
#include "../Drivers/keyboard.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obase.h"
#include "../cppfand/obaseww.h"
#include "../cppfand/printtxt.h"
#include "../cppfand/wwmenu.h"
#include "../cppfand/wwmix.h"
#include "../cppfand/models/FrmlElem.h"
#include "../textfunc/textfunc.h"

const int TXTCOLS = 80;
longint Timer = 0;
bool Insert, Indent, Wrap, Just;

// PROMENNE
bool InsPage;
PartDescr Part;

struct Character {
	char ch = 0;
	BYTE color = 0;
};

// *** Promenne metody EDIT
char Arr[SuccLineSize]{ '\0' }; // znaky pro 1 radek
char* T = nullptr; // ukazatel na vstupni retezec
WORD NextI = 0;
integer LineL = 0, ScrL = 0;
longint RScrL = 0;
bool UpdatedL = false, CtrlL = false, HardL = false;
WORD BCol = 0, Colu = 0, Row = 0;
bool ChangeScr = false;
ColorOrd ColScr;
bool IsWrScreen = false;
WORD FirstR = 0, FirstC = 0, LastR = 0, LastC = 0;
WORD MinC = 0, MinR = 0, MaxC = 0, MaxR = 0;
WORD MargLL[4]{ 0, 0, 0, 0 };
WORD PageS = 0, LineS = 0;
bool bScroll = false, FirstScroll = false, HelpScroll = false;
longint PredScLn = 0;
WORD PredScPos = 0; // {pozice pred Scroll}
BYTE FrameDir = 0;
WORD WordL = 0; // {Mode=HelpM & ctrl-word is on screen}
bool Konec = false;
WORD i1 = 0, i2 = 0, i3 = 0;
// *** konec promennych

const BYTE InterfL = 4; /*sizeof(Insert+Indent+Wrap+Just)*/
const WORD TextStore = 0x1000;
const BYTE TStatL = 35; /*=10(Col Row)+length(InsMsg+IndMsg+WrapMsg+JustMsg+BlockMsg)*/

const BYTE CountC = 7;

///  { ^s - underline, ^w - italic, ^q - expanded, ^d - double, ^b - bold, ^e - compressed, ^a - ELITE }
std::string CtrlKey = "\x13\x17\x11\x04\x02\x05\x01";

std::set<char> Separ = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,58,59,60,61,62,63,64,
91,92,93,94,96,123,124,125,126,127 };


// {**********global param begin for SavePar}  // r85
char Mode = '\0';
char TypeT = '\0';
std::string NameT;
std::string ErrMsg;
WORD MaxLenT = 0, IndT = 0, ScrT = 0;
size_t LenT = 0;
EdExitD* ExitD = nullptr;
bool SrchT, UpdatT;
WORD LastNr, CtrlLastNr;
integer LeftMarg, RightMarg;
bool TypeB;
std::string LastS, CtrlLastS, ShiftLastS, AltLastS, HeadS;
longint* LocalPPtr;
bool EditT;

// od r101
BYTE ColKey[CountC + 1]{ 0 };
BYTE TxtColor = 0, BlockColor = 0, SysLColor = 0;
pstring InsMsg, nInsMsg, IndMsg, WrapMsg, JustMsg, BlockMsg;
pstring ViewMsg;
char CharPg = '\0';
bool InsPg = false;
longint BegBLn = 0, EndBLn = 0;
WORD BegBPos = 0, EndBPos = 0;
WORD ScrI = 0, LineI = 0, Posi = 0, BPos = 0; // {screen status}
std::string FindStr, ReplaceStr;
bool Replace = false;
pstring OptionStr;
bool FirstEvent = false;
WORD PHNum = 0, PPageS = 0; // {strankovani ve Scroll}

FILE* TxtFH = nullptr;
pstring TxtPath;
pstring TxtVol;
bool AllRd = false;
longint AbsLenT = 0;
bool ChangePart, UpdPHead;
void RestorePar(longint l);


longint SavePar()
{
	WORD len = InterfL + 4;
	LongStr* sp = new LongStr(len); // (LongStr*)GetStore(len + 2);
	sp->LL = len;
	Move(&Insert, &sp->A[0], InterfL);
	Move(&Mode, &sp->A[InterfL + 1], len - InterfL);
	auto result = StoreInTWork(sp);
	ReleaseStore(sp);
	return result;
}

void RestorePar(longint l)
{
	LongStr* sp = ReadDelInTWork(l);
	Move(sp->A, &Insert, InterfL);
	Move(&sp->A[InterfL + 1], &Mode, sp->LL - InterfL);
	ReleaseStore(sp);
}

FrmlElem* RdFldNameFrmlT(char& FTyp)
{
	Error(8);
	return nullptr;
}

void MyWrLLMsg(pstring s)
{
	if (HandleError == 4) s = "";
	SetMsgPar(s);
	WrLLF10Msg(700 + HandleError);
}

void MyRunError(pstring s, WORD n)
{
	SetMsgPar(s);
	RunError(n);
}

void HMsgExit(pstring s)
{
	switch (HandleError) {
	case 0: return;
	case 1: {
		s = s[1];
		SetMsgPar(s);
		RunError(700 + HandleError);
		break;
	}
	case 2:
	case 3: {
		SetMsgPar(s);
		RunError(700 + HandleError);
		break;
	}
	case 4: {
		RunError(704);
		break;
	}
	}
}

/// Find index of nth character in C-string, if not found then index after LENGTH is returned!
/// Works with values 1 .. N (Pascal style)
size_t FindChar(char* text, size_t length, char c, size_t from, size_t n)
{
	size_t result = 0; // as not found
	from--;
	for (size_t j = 0; j < n; j++) {
		for (size_t i = from; i < length; i++) {
			if (text[i] == c) {
				result = i;
				break;
			}
		}
		from = result + 1;
	}
	return result == 0 ? length + 1 : result + 1;
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
	WORD i1 = 0;
	pstring s1, s2;
	char c = '\0';
	auto result = false;
	c = FindStr[1];
	if (!FindStr.empty())
	{
	label1:
		if (TestOptStr('~')) i1 = FindOrdChar(c, I, Len);
		else if (TestOptStr('u')) i1 = FindUpcChar(c, I, Len);
		else {
			i1 = FindChar(T, Len, c, I);
		}
		I = i1;
		if (I + FindStr.length() > Len) return result;
		s2 = FindStr;
		Move(&T[I], &s1[1], FindStr.length());
		s1[0] = FindStr.length();
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
			if (I > 1 && !Separ.count(T[I - 1]) || !Separ.count(T[I + FindStr.length()]))
			{
				I++;
				goto label1;
			}
		result = true;
		I += FindStr.length();
	}
	return result;
}

/// pracuje s Pascal indexem 1 .. N
size_t FindCtrl(char* t, size_t first, size_t last)
{
	first--; last--;
	// ^A ^B ^D ^E ^Q ^S ^W
	std::set<char> pc = { 0x01, 0x02, 0x04, 0x05, 0x11, 0x13, 0x17 };
	for (size_t i = first; i <= last; i++) {
		if (pc.count(t[i]) > 0) return i;
	}
	return std::string::npos; // nenalezeno
}

void SetColorOrd(ColorOrd& CO, WORD First, WORD Last)
{
	size_t index = FindCtrl(T, First, Last); 
	while (index < Last - 1) // if not found -> I = std::string::npos
	{
		size_t pp = CO.find(T[index]);
		if (pp != std::string::npos) {
			CO.erase(pp);
		}
		else {
			CO += T[index];
		}
		index = FindCtrl(T, index + 2, Last);
	}
}

void SimplePrintHead()
{
	//pstring ln;
	PHNum = 0;
	PPageS = 0x7FFF;
}

void LastLine(char* input, WORD from, WORD num, WORD& Ind, WORD& Count)
{
	WORD length = Count;
	Count = 0;
	Ind = from;
	for (int i = from; i < length; i++)
	{
		if (input[i] == _CR) { Ind = from + i; Count++; }
	}
	if (Count > 0 && input[Ind] == 0x0A) Ind++; // LF
}

bool RdNextPart()
{
	// kompletne prepsano -> vycte cely soubor do promenne T
	auto fileSize = FileSizeH(TxtFH);
	T = new char[fileSize];
	SeekH(TxtFH, 0);
	ReadH(TxtFH, fileSize, T);
	LenT = fileSize;
	AllRd = true;
	return false; // return ChangePart
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
	if (L1 < LenT) {
		AllRd = false;
		LenT = L1;
		ReleaseStore(&T[LenT + 1]);
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
	SeekH(TxtFH, 0);
	WriteH(TxtFH, LenT, T);
	if (HandleError != 0) {
		SetMsgPar(TxtPath);
		WrLLF10Msg(700 + HandleError);
	}
	FlushH(TxtFH);
	TruncH(TxtFH, LenT);
	AbsLenT = FileSizeH(TxtFH);
	if (HandleError != 0) {
		SetMsgPar(TxtPath);
		WrLLF10Msg(700 + HandleError);
	}
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
	AllRd = false;
	ChangePart = RdNextPart();
}

void OpenTxtFh(char Mode)
{
	FileUseMode UM;
	CPath = TxtPath; CVol = TxtVol;
	TestMountVol(CPath[0]);
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
	WORD J = Name.length();
	while (!(Name[J] == '\\' || Name[J] == ':') && (J > 0)) {
		J--;
	}
	pstring s = Name.substr(J, Name.length() - J);
	if (Name[2] == ':') {
		s = Name.substr(0, 2) + s;
	}
	return s;
}

void WrStatusLine()
{
	std::string Blanks;
	if (Mode != HelpM) {
		if (HeadS.length() > 0) {
			Blanks = AddTrailChars(HeadS, ' ', TXTCOLS);
			size_t i = Blanks.find('_');
			if (i == std::string::npos) {
				Blanks = Blanks.substr(0, TStatL + 3) + Blanks.substr(TStatL + 3 - 1, 252 - TStatL);
				for (size_t j = 0; j < TStatL + 2; j++) {
					Blanks[j] = ' ';
				}
			}
			else {
				while ((i < Blanks.length()) && (Blanks[i] == '_')) {
					Blanks[i] = ' ';
					i++;
				}
			}
		}
		else {
			Blanks = RepeatString(' ', TxtCols);
			std::string s = ShortName(NameT);
			size_t i = TStatL + 3 - 1;
			if (s.length() + i >= TXTCOLS) i = TXTCOLS - s.length() - 2;
			for (size_t j = 0; j < s.length(); j++) {
				Blanks[i + j] = s[j];
			}
		}
		screen.ScrWrStr(1, 1, Blanks, SysLColor);
	}
}

void WriteMargins()
{
	CHAR_INFO LastL[201];

	if ((Mode != HelpM) && (Mode != ViewM) && Wrap) {
		screen.ScrRdBuf(FirstC - 1, TxtRows - 1, LastL, LineS);
		LastL[MargLL[0]].Attributes = MargLL[1] >> 8;
		LastL[MargLL[0]].Char.AsciiChar = MargLL[1] & 0x00FF;
		LastL[MargLL[2]].Attributes = MargLL[3] >> 8;
		LastL[MargLL[2]].Char.AsciiChar = MargLL[3] & 0x00FF;

		MargLL[0] = MaxI(0, LeftMarg - BPos);
		if (MargLL[0] > 0) {
			MargLL[1] = (LastL[MargLL[0]].Attributes << 8) + LastL[MargLL[0]].Char.AsciiChar;
			LastL[MargLL[0]].Attributes = LastL[LineS].Attributes;
			LastL[MargLL[0]].Char.AsciiChar = 0x10;
		}
		MargLL[2] = MaxI(0, RightMarg - BPos);
		if (MargLL[2] > 0) {
			MargLL[3] = (LastL[MargLL[2]].Attributes << 8) + LastL[MargLL[2]].Char.AsciiChar;
			LastL[MargLL[2]].Attributes = LastL[LineS].Attributes;
			LastL[MargLL[2]].Char.AsciiChar = 0x11;
		}
		screen.ScrWrCharInfoBuf(short(FirstC - 1), short(TxtRows - 1), LastL, LineS);
	}
}

void WrLLMargMsg(std::string& s, WORD n)
{
	if (!s.empty()) {
		MsgLine = s;
		WrLLMsgTxt();
	}
	else {
		if (n != 0) WrLLMsg(n);
		else {
			if (!LastS.empty()) {
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

/// Inicializuje obrazovku - sirku, vysku editoru
void InitScr()
{
	FirstR = WindMin.Y;
	FirstC = WindMin.X;
	LastR = WindMax.Y;
	LastC = WindMax.X;

	if ((FirstR == 1) && (Mode != HelpM)) FirstR++;
	if (LastR == TxtRows) LastR--;
	MinC = FirstC; MinR = FirstR; MaxC = LastC; MaxR = LastR;
	screen.Window(FirstC, FirstR, LastC, LastR);
	FirstR--;
	if ((Mode != HelpM) && (Mode != ViewM) && Wrap) LastC--;
	PageS = LastR - FirstR; LineS = succ(LastC - FirstC);
}

void UpdStatLine(int Row, int Col, char mode)
{
	char RowCol[] = "RRRRR:CCCCC";
	char StatLine[] = "                                   ";

	if (!HelpScroll) {
		longint lRow = Row + Part.LineP;
		snprintf(RowCol, sizeof(RowCol), "%5i:%-5i", lRow, Col);
		memcpy(&StatLine[1], RowCol, 11); // 11 znaku ve format 'RRRRR:CCCCC'
		switch (mode) { // uses parameter 'mode', not global variable 'Mode'
		case TextM: {
			if (Insert) Move(&InsMsg[1], &StatLine[10], 5);
			else Move(&nInsMsg[1], &StatLine[10], 5);
			if (Indent) Move(&IndMsg[1], &StatLine[15], 5);
			if (Wrap) Move(&WrapMsg[1], &StatLine[20], 5);
			if (Just) Move(&JustMsg[1], &StatLine[25], 5);
			if (TypeB == ColBlock) Move(&BlockMsg[1], &StatLine[30], 5);
			break;
		}
		case ViewM: { Move(&ViewMsg[1], &StatLine[10], ViewMsg.length()); break; }
		case SinFM: { StatLine[12] = '-'; break; }
		case DouFM: { StatLine[12] = '='; break; }
		case DelFM: { StatLine[12] = '/'; break; }
		default: break;
		}
		integer i = 1;
		if (HeadS.length() > 0) {
			size_t find = HeadS.find('_');
			if (find == std::string::npos) {
				find = 0;
			}
			i = MaxW(1, find);
			if (i > TxtCols - TStatL) {
				i = MaxI((integer)(TxtCols)-TStatL, 1);
			}
		}
		screen.ScrWrStr(i, 1, StatLine, SysLColor);
	}
}

longint LineAbs(int Ln)
{
	return Part.LineP + Ln;
}

bool LineInBlock(int Ln)
{
	if ((LineAbs(Ln) > BegBLn) && (LineAbs(Ln) < EndBLn)) {
		return true;
	}
	else {
		return false;
	}
}

bool LineBndBlock(int Ln)
{
	if ((LineAbs(Ln) == BegBLn) || (LineAbs(Ln) == EndBLn)) {
		return true;
	}
	else {
		return false;
	}
}

BYTE Color(char c)
{
	const size_t indexOfKey = CtrlKey.find(c);
	return ColKey[indexOfKey + 1];
}

BYTE Color(ColorOrd CO)
{
	if (CO.length() == 0) return TxtColor;
	const char lastColor = CO[CO.length() - 1];
	return Color(lastColor);
}

void EditWrline(char* P, int Row)
{
	WORD BuffLine[256]{ 0 };
	BYTE nv1;
	BYTE nv2;
	bool IsCtrl = false;

	WORD Line = pred(ScrL + Row);
	if (LineInBlock(Line) && (TypeB == TextBlock)) {
		nv2 = BlockColor;
	}
	else {
		nv2 = TxtColor;
	}
	integer I = 0;
	while (P[I] != _CR && I < LineSize - 1) {
		nv1 = P[I];
		if (I < 0 || I > 255) throw std::exception("Index");
		BuffLine[I] = (nv2 << 8) + nv1;
		if (nv1 < 32) IsCtrl = true;
		I++;
	}

	integer LP = I - 1;  // index of last character (before CR)
	nv1 = ' ';

	for (I = LP + 1; I < BPos + LineS; I++) {
		// all characters after last char will be spaces (to the end of screen)
		if (I < 0 || I > 255) throw std::exception("Index");
		BuffLine[I] = (nv2 << 8) + nv1;
	}

	if (BegBLn <= EndBLn) {
		if (LineBndBlock(Line) || ((TypeB == ColBlock) && LineInBlock(Line))) {
			integer B, E;
			if ((BegBLn == LineAbs(Line)) || (TypeB == ColBlock)) {
				B = MinI(BegBPos, LineS + BPos + 1);
			}
			else { B = 1; }
			if ((EndBLn == LineAbs(Line)) || (TypeB == ColBlock)) {
				E = MinI(EndBPos, LineS + BPos + 1);
			}
			else { E = LineS + BPos + 1; }
			for (I = B; I < pred(E); I++) {
				if (I < 0 || I > 255) throw std::exception("Index");
				BuffLine[I] = (BuffLine[I] & 0x00FF) + (BlockColor << 8);
			}
		}
	}
	if (IsCtrl) {
		// retezec obsahuje kontrolni znaky
		// -> budou zmeneny na pismena a prebarveny
		for (I = BPos; I <= LP; I++) {
			if ((unsigned char)P[I] < 32) {
				if (I < 0 || I > 254) throw std::exception("Index");
				BuffLine[I] = ((P[I] + 64) & 0x00FF) + (Color(P[I]) << 8);
			}
		}
	}
	// both 'WindMin.Y' and 'Row' are counted from 1 -> that's why -2 
	screen.ScrWrBuf(WindMin.X - 1, WindMin.Y + Row - 2, &BuffLine[BPos], LineS);
}

void ScrollWrline(char* P, int Row, ColorOrd& CO)
{
	std::set<char> GrafCtrl = { 3,6,9,11,15,16,18,21,22,24,25,26,29,30,31 };
	BYTE len = 15; // GrafCtrl has 15 members

	WORD BuffLine[256]{ 0 };
	BYTE nv1;
	BYTE nv2;

	bool IsCtrl = false;
	BYTE Col = Color(CO);
	nv2 = Col;

	integer I = 0; integer J = 0;
	char cc = P[I];
	while (cc != _CR && I < LineSize && !InsPage) {
		if (((unsigned char)cc >= 32) || (GrafCtrl.count(cc) > 0)) {
			nv1 = cc;
			BuffLine[J] = (nv2 << 8) + nv1;
			J++;
		}
		else {
			if (CtrlKey.find(cc) != std::string::npos) IsCtrl = true;
			else {
				if (bScroll && (cc == 0x0C)) { InsPage = InsPg; I++; }
			}
		}
		I++;
		cc = P[I];
	}

	integer LP = I - 1;   // index of last character (before CR)
	nv1 = ' ';

	while (J < BCol + LineS) {
		BuffLine[J] = (nv2 << 8) + nv1;
		J++;
	}
	if (IsCtrl) {
		I = 0; J = 0;
		while (I <= LP) {
			cc = P[I];
			if (((unsigned char)cc >= 32) || (GrafCtrl.count(cc) > 0)) {
				BuffLine[J] = (BuffLine[J] & 0x00FF) + (Col << 8);
				J++;
			}
			else if (CtrlKey.find(cc) != std::string::npos) {
				size_t pp = CO.find(cc);
				if (pp != std::string::npos) {
					// TODO: nevim, jak to ma presne fungovat
					// original: if pp>0 then CO:=copy(CO,1,pp-1)+copy(CO,pp+1,len-pp)
					CO = CO.substr(0, pp) + CO.substr(pp + 1, len - pp + 1);
				}
				else {
					CO += cc;
				}
				Col = Color(CO);
			}
			else if (cc == 0x0C) {
				BuffLine[J] = 219 + (Col << 8);
			}
			I++;
		}
		while (J <= BCol + LineS) {
			BuffLine[J] = (BuffLine[J] & 0x00FF) + (Col << 8);
			J++;
		}
	}
	// both 'WindMin.Y' and 'Row' are counted from 1 -> that's why -2 
	screen.ScrWrBuf(WindMin.X - 1, WindMin.Y + Row - 2, &BuffLine[BCol], LineS);
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
	//return TestEvent();
}

void DelEndT()
{
	T[LenT - 1] = '\0';
	LenT--;
}

void TestUpdFile()
{
	DelEndT();
	if (Part.UpdP) { UpdateFile(); }
}

void WrEndT()
{
	// vytvori nove pole o delce puvodniho + 1,
	// puvodni pole se do nej prekopiruje a na konec se vlozi CR
	LenT++;
	char* T2 = new char[LenT];
	memcpy(T2, T, LenT - 1);
	T2[LenT - 1] = _CR;
	delete[] T;
	T = T2;
}

void MoveIdx(int dir)
{
	WORD mi = -dir * Part.MovI;
	WORD ml = -dir * Part.MovL;
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

/// Counts the number of occurrences of a character. 'first' & 'last' are (1 .. N)
size_t CountChar(char* text, size_t text_len, char C, size_t first, size_t last)
{
	size_t count = 0;
	first--; last--; // to be C indexes
	if (first < text_len) {
		if (last >= text_len) last = text_len - 1;
		for (size_t i = first; i <= last; i++) {
			if (text[i] == C) count++;
		}
	}
	else {
		// out of index
	}
	return count;

	//size_t I = FindChar(T, LenT, C, First);
	//WORD n = 0;
	//while (I < Last) {
	//	n++;
	//	I = FindChar(T, LenT, C, I + 1);
	//}
	//return n;
}

WORD SetLine(WORD Ind)
{
	return CountChar(T, LenT, _CR, 1, Ind) + 1;
}

WORD SetCurrI(WORD Ind)
{
	WORD result = 1;
	Ind--;
	while (Ind > 0) {
		if (T[Ind - 1] == _CR) {
			Ind++;
			if (T[Ind - 1] == _LF) Ind++;
			result = Ind;
			break;
		}
		Ind--;
	}
	return result;
}

void SmallerPart(WORD Ind, WORD FreeSize)
{
	WORD i, il, l;
	longint lon;
	NullChangePart();
	if ((StoreAvail() > FreeSize) && (MaxLenT - LenT > FreeSize)) {
		return;
	}
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

void TestLenText(char** text, size_t& textLength, WORD i, int j)
{
	int lenDiff = j - i;
	if (lenDiff != 0) {
		// TEXT will be shorter or longer
		char* newT = new char[textLength + lenDiff];
		memcpy(newT, *text, i + lenDiff - 1);
		memcpy(&newT[i + lenDiff - 1], &(*text)[i - 1], textLength - i + 1);
		delete[] * text;
		*text = newT;
	}
	textLength += lenDiff;
	SetUpdat();
}

void DekodLine()
{
	WORD LL = 1;
	WORD LP = FindChar(T, LenT, _CR, LineI) - LineI;
	HardL = true;
	FillChar(Arr, LineSize, 32);
	NextI = LineI + LP + 1;
	if ((NextI < LenT) && (T[NextI - 1] == _LF)) NextI++;
	else HardL = false;
	if (LP > LineSize) {
		LP = LineSize;
		if (Mode == TextM) {
			if (PromptYN(402)) {
				LL = LineI + LineSize;
				NullChangePart();
				TestLenText(&T, LenT, LL, longint(LL) + 1);
				LL -= Part.MovI;
				T[LL] = _CR;
				NextI = LineI + LP + 1;
			}
		}
		else {
			Mode = ViewM;
		}
	}
	if (LP > 0) Move(&T[LineI - 1], Arr, LP);
	UpdatedL = false;
}

/// nastavuje LineI, vola DekodLine()
void SetDekCurrI(WORD Ind)
{
	LineI = SetCurrI(Ind);
	DekodLine();
}

/// nastavuje LineL, vola SetDekCurrI(i)
void SetDekLnCurrI(WORD Ind)
{
	SetDekCurrI(Ind);
	LineL = SetLine(LineI);
}

WORD SetInd(char* text, size_t len_text, WORD Ind, WORD Pos) // { line, pozice --> index}
{
	WORD P = Ind == 0 ? 0 : Ind - 1;
	if (Ind < len_text)	{
		while ((Ind - P < Pos) && (text[Ind - 1] != _CR)) { Ind++; }
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
	int LP = LineSize;
	while ((LP >= 0) && (Arr[LP] == ' ' || Arr[LP] == '\0')) {
		LP--;
	}
	return LP + 1; // vraci Pascal index 1 .. N;
}

void NextPart()
{
	TestUpdFile();
	ChangePart = RdNextPart();
	MoveIdx(1);
	WrEndT();
}

WORD FindLine(integer& Num)
{
	WORD result;

	while (true) {
		if (Num <= 0) {
			if (Part.PosP == 0) {
				Num = 1;
				//if (LineI < 1) LineI = 1;
				//if (LineL < 1) LineL = 1;
			}
			else {
				PredPart();
				continue;
			}
		}
		if (Num == 1) {
			result = 1;
		}
		else {
			// WORD J = Num - 1;
			// TODO: tady se vyuziva jinak puvodni kod -> k cemu je to 'J'?
			// I = FindChar(J, _CR, 1, LenT) + 1;
			WORD I = FindChar(T, LenT, _CR, 1, Num - 1) + 1;
			if (T[I - 1] == _LF) {
				I++;
			}
			if (I > LenT) {
				if (AllRd) {
					Num = SetLine(LenT);
					result = SetCurrI(LenT);
				}
				else {
					NextPart();
					if (Num != LineL) Num -= Part.MovL;
					continue;
				}
			}
			else {
				result = I;
			}
		}
		return result; // returns C string index (0..n)
	}
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
	if ((Mode != HelpM) && (Mode != ViewM) && Wrap) {
		WORD w;
		if (Hard) {
			w = 0x11 + static_cast<WORD>(TxtColor << 8);
		}
		else {
			w = ' ' + static_cast<WORD>(TxtColor << 8);
		}
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
	integer r; // row number, starts from 1
	ColorOrd co1;
	WORD oldSI = ScrI;
	pstring PgStr;

	InsPage = false;
	if (ChangeScr) {
		if (ChangePart) DekodLine();
		ChangeScr = false;

		if (bScroll) ScrI = LineI;
		else ScrI = FindLine(ScrL);

		if (HelpScroll) {
			ColScr = Part.ColorP;
			SetColorOrd(ColScr, 1, ScrI);
		}
	}
	if (bScroll) {
		// {tisk aktualniho radku}
		FillChar(&PgStr[0], 255, CharPg);
		PgStr[0] = 255;
		co1 = ColScr;
		r = 0;
		while (Arr[r] == 0x0C) {
			r++;
		}
		ScrollWrline(&Arr[r], 1, co1);
	}
	else if (Mode == HelpM) {
		co1 = Part.ColorP;
		SetColorOrd(co1, 1, LineI);
		ScrollWrline(Arr, LineL - ScrL + 1, co1);
	}
	else {
		EditWrline(Arr, LineL - ScrL + 1);
	}
	WrEndL(HardL, LineL - ScrL + 1);
	if (MyTestEvent()) return;
	if (ScrI < 1) {
		throw std::exception("UpdScreen(): variable ScrI < 1");
	}
	WORD index = ScrI - 1;
	r = 1;
	integer rr = 0;
	WORD w = 1;
	InsPage = false;
	ColorOrd co2 = ColScr;
	if (bScroll) {
		while (T[index] == 0x0C) {
			index++;
		}
	}
	do {
		if (MyTestEvent()) return;                   // {tisk celeho okna}
		if ((index >= LenT) && !AllRd) {
			NextPartDek();
			index -= Part.MovI;
		}

		if (bScroll && (index < LenT)) {
			if ((InsPg && (ModPage(r - rr + RScrL - 1))) || InsPage) {
				EditWrline((char*)PgStr[1], r);
				WrEndL(false, r);
				if (InsPage) rr++;
				InsPage = false;
				goto label1;
			}
		}
		if (!bScroll && (index == LineI - 1)) {
			index = NextI - 1;
			co2 = co1;
			goto label1;
		}
		if (index < LenT) {
			// index je mensi nez delka textu -> porad je co tisknout
			if (HelpScroll) {
				ScrollWrline((char*)&T[index], r, co2);
			}
			else {
				EditWrline((char*)&T[index], r);
			}
			if (InsPage) {
				index = FindChar(T, LenT, 0x0C, index + 1);
			}
			else {
				index = FindChar(T, LenT, _CR, index + 1);
			}
			WrEndL((index < LenT) && (T[index] == _LF), r);
			if (T[index] == _LF) {
				index++;
			}
		}
		else {
			EditWrline((char*)&T[LenT - 1], r);
			WrEndL(false, r);
		}

	label1:
		r++;
		if (bScroll && (T[index] == 0x0C)) {
			InsPage = InsPg;
			index++;
		}
	} while (r <= PageS);
}

void Background()
{
	UpdStatLine(LineL, Posi, Mode);
	// TODO: musi to tady byt?
	// if (MyTestEvent()) return;
	if (HelpScroll) {
		WORD p = Posi;
		if (Mode == HelpM) {
			if (WordL == LineL) {
				while (Arr[p + 1] != 0x11) {
					p++;
				}
			}
		}
		if (Column(p) - BCol > LineS) {
			BCol = Column(p) - LineS;
			BPos = Position(BCol);
		}
		if (Column(Posi) <= BCol) {
			BCol = Column(Posi) - 1;
			BPos = Position(BCol);
		}
	}
	else {
		if (Posi > LineS) {
			if (Posi > BPos + LineS) {
				BPos = Posi - LineS;
			}
		}
		if (Posi <= BPos) {
			BPos = pred(Posi);
		}
	}
	if (LineL < ScrL) {
		ScrL = LineL;
		ChangeScr = true;
	}
	if (LineL >= ScrL + PageS) {
		ScrL = succ(LineL - PageS);
		ChangeScr = true;
	}
	UpdScreen(); // {tisk obrazovky}
	WriteMargins();
	screen.GotoXY(Posi - BPos, LineL - ScrL + 1);
	IsWrScreen = true;
}

void KodLine()
{
	WORD LP = LastPosLine() + 1; // position behind last char on the line (counted from 1)
	if (HardL) LP++;
	TestLenText(&T, LenT, NextI, LineI + LP);
	Move(Arr, &T[LineI - 1], LP);
	NextI = LineI + LP;
	LP = NextI - 1;
	if (HardL) {
		T[LP - 2] = _CR;
		T[LP - 1] = _LF;
	}
	else {
		T[LP - 1] = _CR;
	}
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
	//void* ptr = nullptr;

	bool old = bScroll;
	//fyz = *(BP(ptr(0, 0x417)) && 0x10) != 0;
	const bool fyz = GetKeyState(VK_SCROLL) & 0x0001;
	if (fyz == old) FirstScroll = false;
	bScroll = (fyz || FirstScroll) && (Mode != HelpM);
	HelpScroll = bScroll || (Mode == HelpM);
	longint L1 = LineAbs(ScrL);
	if (old != bScroll) {
		if (bScroll) {
			WrStatusLine();
			TestKod();
			screen.CrsHide();
			PredScLn = LineAbs(LineL);
			PredScPos = Posi;
			if (UpdPHead) {
				SetPart(1);
				SimplePrintHead();
				DekFindLine(MaxL(L1, PHNum + 1));
			}
			else {
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
		else {
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
		WrLLMargMsg(CtrlLastS, CtrlLastNr);
	else if ((Flags & 0x03) != 0) // { Shift }
		WrLLMargMsg(ShiftLastS, 0);
	else if ((Flags & 0x08) != 0) // { Alt }
		WrLLMargMsg(AltLastS, 0);
}

//void MyInsLine()
//{
//	TextAttr = TxtColor;
//	InsLine();
//}

//void MyDelLine()
//{
//	TextAttr = TxtColor;
//	DelLine();
//}

void PredLine()
{
	WORD mi, ml;
	TestKod();
	if ((LineL == 1) && (Part.PosP > 0)) PredPart();
	if (LineL > 1) {
		if (T[LineI - 1 - 1] == _LF) {
			SetDekCurrI(LineI - 2);
		}
		else {
			SetDekCurrI(LineI - 1);
		}
		LineL--;
		if (LineL < ScrL) {
			screen.GotoXY(1, 1);
			//MyInsLine();
			ScrL--;
			ChangeScr = true;
			if (bScroll) {
				/*dec(RLineL);*/
				RScrL--;
				/*if (ModPage(RLineL))*/
				if (ModPage(RScrL))	{
					screen.GotoXY(1, 1);
					//MyInsLine();/*dec(RLineL);*/
					RScrL--;
				}
			}
		}
	}
}

void RollNext()
{
	if ((NextI >= LenT) && !AllRd) NextPartDek();
	if (NextI <= LenT) {
		screen.GotoXY(1, 1);
		//MyDelLine();
		ScrL++;
		ChangeScr = true;
		if (LineL < ScrL) {
			TestKod();
			LineL++;
			LineI = NextI;
			DekodLine();
		}
	}
}

void RollPred()
{
	if ((ScrL == 1) && (Part.PosP > 0)) PredPart();
	if (ScrL > 1) {
		screen.GotoXY(1, 1);
		//MyInsLine();
		ScrL--;
		ChangeScr = true;
		if (LineL == ScrL + PageS) {
			TestKod();
			LineL--;
			if (T[LineI - 1] == _LF) { SetDekCurrI(LineI - 2); }
			else { SetDekCurrI(LineI - 1); }
		}
	}
}

void direction1(BYTE x, BYTE& zn2)
{
	BYTE y = 0x10;
	if (x > 2) { y = y << 1; }
	if (x == 0) { y = 0; }
	if (Mode == DouFM) { zn2 = zn2 || y; }
	else { zn2 = zn2 && !y; }
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
		LineI = NextI;
		DekodLine();
		LineL++;
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
		else if (WrScr && (LineL == ScrL + PageS)) {
			//if (PageS > 1) MyWriteln();
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
	BYTE dir, zn1, zn2, b;

	UpdStatLine(LineL, Posi, Mode);
	screen.CrsBig();
	BYTE odir = 0;
	ClrEvent();

	while (true) /* !!! with Event do!!! */
	{
		if (!MyGetEvent(Mode, SysLColor, LastS, LastNr, IsWrScreen, bScroll, ExitD) ||
			((Event.What == evKeyDown) && (Event.Pressed.KeyCombination() == __ESC)) || (Event.What != evKeyDown)) {
			ClrEvent();
			screen.CrsNorm();
			Mode = TextM;
			return;
		}
		switch (Event.Pressed.KeyCombination()) {
		case _frmsin_: Mode = SinFM; break;
		case _frmdoub_: Mode = DouFM; break;
		case _dfrm_: Mode = DelFM; break;
		case _nfrm_: Mode = NotFM; break;
		case __LEFT:
		case __RIGHT:
		case __UP:
		case __DOWN:
			if (!bScroll) {
				FrameString[0] = 63;
				zn1 = FrameString.first(Arr[Posi]);
				zn2 = zn1 & 0x30;
				zn1 = zn1 & 0x0F;
				dir = FrameString.first(Hi(Event.Pressed.KeyCombination()));
				auto dirodir = dir + odir;
				if (dirodir == 2 || dirodir == 4 || dirodir == 8 || dirodir == 16) odir = 0;
				if (zn1 == 1 || zn1 == 2 || zn1 == 4 || zn1 == 8) zn1 = 0;
				char oldzn = Arr[Posi];
				Arr[Posi] = ' ';
				if (Mode == DelFM) b = zn1 && !(odir || dir);
				else b = zn1 | (odir ^ dir);
				if (b == 1 || b == 2 || b == 4 || b == 8) b = 0;
				if ((Mode == DelFM) && (zn1 != 0) && (b == 0)) oldzn = ' ';
				direction1(dir, zn2); direction1(odir, zn2);
				if (Mode == NotFM) b = 0;

				if ((b != 0) && ((Event.Pressed.KeyCombination() == __LEFT) || (Event.Pressed.KeyCombination() == __RIGHT) ||
					(Event.Pressed.KeyCombination() == __UP) || (Event.Pressed.KeyCombination() == __DOWN)))
					Arr[Posi] = FrameString[zn2 + b];
				else Arr[Posi] = oldzn;

				if ((dir == 1) || (dir == 4)) odir = dir * 2;
				else odir = dir / 2;

				if (Mode == NotFM) odir = 0;
				else UpdatedL = true;

				switch (Event.Pressed.KeyCombination()) {
				case __LEFT: if (Posi > 1) Posi--; break;
				case __RIGHT: if (Posi < LineSize) Posi++; break;
				case __UP: PredLine(); break;
				case __DOWN: NextLine(true); break;
				default:;
				}
			}
			break;
		}
		ClrEvent();
		UpdStatLine(LineL, Posi, Mode);/*if (not MyTestEvent) */
		Background();
	}
}

void CleanFrameM()
{
	if (Mode == SinFM || Mode == DouFM || Mode == DelFM || Mode == NotFM) /* !!! with Event do!!! */
		if (!MyGetEvent(Mode, SysLColor, LastS, LastNr, IsWrScreen, bScroll, ExitD) ||
			((Event.What == evKeyDown) && (Event.Pressed.KeyCombination() == __ESC)) || (Event.What != evKeyDown))
		{
			ClrEvent();
			screen.CrsNorm();
			Mode = TextM;
			UpdStatLine(LineL, Posi, Mode);
			return;
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

		if ((b != 0) && ((Event.Pressed.KeyCombination() == __LEFT) || (Event.Pressed.KeyCombination() == __RIGHT) ||
			(Event.Pressed.KeyCombination() == __UP) || (Event.Pressed.KeyCombination() == __DOWN)))
			Arr[Posi] = FrameString[zn2 + b];
		else Arr[Posi] = oldzn;

		if ((dir == 1) || (dir == 4)) odir = dir * 2;
		else odir = dir / 2;

		if (Mode == NotFM) odir = 0;
		else UpdatedL = true;

		switch (Event.Pressed.KeyCombination()) {
		case __LEFT: {
			if (Posi > 1) Posi--;
			break;
		}
		case __RIGHT: {
			if (Posi < LineSize) Posi++;
			break;
		}
		case __UP: {
			PredLine();
			break;
		}
		case __DOWN: {
			NextLine(true);
			break;
		}
		default:;
		}
	}
	break;
	}
	UpdStatLine(LineL, Posi, Mode);
}

void MoveB(WORD& B, WORD& F, WORD& T)
{
	if (F <= T) { if (B > F) B += T - F; }
	else if (B >= F) B -= F - T;
	else if (B > T) B = T; B = MinW(B, LastPosLine() + 1);
}

bool TestLastPos(WORD F, WORD T)
{
	WORD LP = LastPosLine();
	if (F > LP) F = LP + 1;
	if (LP + T - F <= LineSize) {
		if (LP >= F) {
			memcpy(&Arr[T - 1], &Arr[F - 1], LP - F + 1);
		}
		if (TypeB == TextBlock) {
			if (LineAbs(LineL) == BegBLn) {
				MoveB(BegBPos, F, T);
			}
			if (LineAbs(LineL) == EndBLn) {
				MoveB(EndBPos, F, T);
			}
		}
		if (F > T) {
			if (T <= LP) {
				memset(&Arr[LP + T - F], ' ', F - T);
			}
		}
		UpdatedL = true;
		return true;
	}
	else return false;
}

void DelChar()
{
	WORD LP;
	TestLastPos(Posi + 1, Posi);
}

void FillBlank()
{
	WORD I;
	KodLine();
	I = LastPosLine();
	if (Posi > I + 1) {
		TestLenText(&T, LenT, LineI + I, longint(LineI) + Posi - 1);
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
	if (NextI <= LenT) {
		if (T[NextI - 1] == _LF) { TestLenText(&T, LenT, NextI, NextI - 2); }
		else { TestLenText(&T, LenT, NextI, NextI - 1); }
	}
	DekodLine();
}

void NewLine(char Mode)
{
	KodLine();
	WORD LP = LineI + MinI(LastPosLine(), Posi - 1);
	NullChangePart();
	TestLenText(&T, LenT, LP, LP + 2);
	LP -= Part.MovI;
	if (LineAbs(LineL) <= BegBLn) {
		if (LineAbs(LineL) < BegBLn) BegBLn++;
		else if ((BegBPos > Posi) && (TypeB == TextBlock)) {
			BegBLn++; BegBPos -= Posi - 1;
		}
	}
	if (LineAbs(LineL) <= EndBLn) {
		if (LineAbs(LineL) < EndBLn) EndBLn++;
		else if ((EndBPos > Posi) && (TypeB == TextBlock))
		{
			EndBLn++;
			EndBPos -= Posi - 1;
		}
	}
	T[LP - 1] = _CR;
	T[LP] = _LF;
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

void WrCharE(char Ch)
{
	if (Insert) {
		if (TestLastPos(Posi, Posi + 1)) {
			Arr[Posi - 1] = Ch;
			if (Posi < LineSize) {
				Posi++;
			}
		}
	}
	else {
		Arr[Posi - 1] = Ch;
		UpdatedL = true;
		if (Posi < LineSize) {
			Posi++;
		}
	}
}

void Format(WORD& i, longint First, longint Last, WORD Posit, bool Rep)
{
	WORD lst, ii1;
	integer ii;
	char A[260];
	bool bBool;
	WORD rp, nb, nw, n;
	WORD RelPos;

	SetPart(First);
	WORD fst = First - Part.PosP;
	longint llst = Last - Part.PosP;
	if (llst > LenT) lst = LenT;
	else lst = llst;
	do {
		if (LenT > 0x400) ii1 = LenT - 0x400;
		else ii1 = 0;
		if ((fst >= ii1) && !AllRd) {
			NextPartDek();
			fst -= Part.MovI; lst -= Part.MovI; llst -= Part.MovI;
			if (llst > LenT) lst = LenT; else lst = llst;
		}
		i = fst; ii1 = i;
		if ((i < 2) || (T[i - 1] == _LF)) {
			while (T[ii1] == ' ') ii1++; Posit = MaxW(Posit, ii1 - i + 1);
		}
		ii1 = i; RelPos = 1;
		if (Posit > 1) {
			Move(&T[i], A, Posit);
			for (ii = 1; ii < Posit - 1; i++) {
				if (CtrlKey.find(T[i]) == std::string::npos) RelPos++;
				if (T[i] == _CR) A[ii] = ' ';
				else i++;
			}
			if ((T[i] == ' ') && (A[Posit - 1] != ' ')) {
				Posit++; RelPos++;
			}
		}
		while (i < lst) {
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
					if (CtrlKey.find(T[i]) == std::string::npos) RelPos++;
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
					if (CtrlKey.find(A[Posit]) == std::string::npos) RelPos++;
					i++; Posit++;
				}
			}
			if ((i < lst) && (T[i] != ' ') && (T[i] != _CR))
			{
				ii = Posit - 1;
				if (CtrlKey.find(A[ii]) != std::string::npos) ii--;
				rp = RelPos; RelPos--;
				while ((A[ii] != ' ') && (ii > LeftMarg))
				{
					if (CtrlKey.find(A[ii]) == std::string::npos) RelPos--; ii--;
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
			TestLenText(&T, LenT, i, longint(ii1) + Posit);
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
	FrmlPtr Z = nullptr;
	std::string txt;
	ExitRecord er; WORD I; pstring Msg;
	void* p = nullptr; char FTyp; double R; bool Del;
	MarkStore(p);
	//NewExit(Ovr(), er);
	goto label2;
	ResetCompilePars();
	RdFldNameFrml = RdFldNameFrmlT;
label0:
	txt = CalcTxt; Del = true; I = 1;
label1:
	TxtEdCtrlUBrk = true; TxtEdCtrlF4Brk = true;
	ww.PromptLL(114, &txt, I, Del);
	if (Event.Pressed.KeyCombination() == _U_) goto label0;
	if ((Event.Pressed.KeyCombination() == _ESC_) || (txt.length() == 0)) goto label3;
	CalcTxt = txt;
	if ((Event.Pressed.KeyCombination() == _CtrlF4_) && (Mode == TextM) && !bScroll)
	{
		if (txt.length() > LineSize - LastPosLine())
		{
			I = LineSize - LastPosLine(); WrLLF10Msg(419); goto label1;
		}
		if (Posi <= LastPosLine()) TestLastPos(Posi, Posi + txt.length());
		Move(&txt[1], &Arr[Posi], txt.length()); UpdatedL = true; goto label3;
	}
	SetInpStr(txt);
	RdLex();
	Z = RdFrml(FTyp);
	if (Lexem != 0x1A) Error(21);
	switch (FTyp) {
	case 'R': {
		R = RunReal(Z); str(R, 30, 10, txt);
		txt = LeadChar(' ', TrailChar(txt, '0'));
		if (txt[txt.length()] == '.') txt[0]--;
		break;
	}
	case 'S': { txt = RunShortStr(Z);   /* wie RdMode fuer T ??*/ break; }
	case 'B': {
		if (RunBool(Z)) txt = AbbrYes;
		else txt = AbbrNo;
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
	EBPos = SetInd(T, LenT, FindLine(i), EndBPos) + Part.PosP;
	SetPartLine(BegBLn);
	i = BegBLn - Part.LineP;
	BBPos = SetInd(T, LenT, FindLine(i), BegBPos) + Part.PosP;
}

void ResetPrint(char Oper, longint& fs, FILE* W1, longint LenPrint, ColorOrd* co, WORD& I1, bool isPrintFile, CharArr* p)
{
	*co = Part.ColorP;
	SetColorOrd(*co, 1, I1);
	isPrintFile = false;
	fs = co->length(); LenPrint += fs;
	if (Oper == 'p') LenPrint++;
	if ((StoreAvail() > LenPrint) && (LenPrint < 0xFFF0)) {
		p = (CharArr*)GetStore2(LenPrint);
		Move(&co[1], p, co->length());
	}
	else {
		isPrintFile = true;
		W1 = WorkHandle;
		SeekH(W1, 0);
		WriteH(W1, co->length(), &co[1]); HMsgExit(CPath);
	}
}

void LowCase(unsigned char& c)
{
	if ((c >= 'A') && (c <= 'Z')) { c = c + 0x20; return; }
	for (size_t i = 128; i <= 255; i++)
		if ((UpcCharTab[i] == c) && (i != c)) { c = i; return; }
}

void LowCase(char& c)
{
	if ((c >= 'A') && (c <= 'Z')) { c = c + 0x20; return; }
	for (size_t i = 128; i <= 255; i++)
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
			LL1 = Part.PosP + SetInd(T, LenT, LineI, Posi);
		}
		else SetBlockBound(LL1, LL2); I1 = LL1 - Part.PosP;
		if (toupper(Oper) == 'P') ResetPrint(Oper, fs, W1, LL2 - LL1, &co, I1, isPrintFile, p);
		do {
			if (LL2 > Part.PosP + LenT) I2 = LenT;
			else I2 = LL2 - Part.PosP;
			switch (Oper) {
			case 'Y': { TestLenText(&T, LenT, I2, I1); LL2 -= I2 - I1; break; }
			case 'U': {
				for (i = I1; i < I2 - 1; i++) T[i] = UpcCharTab[T[i]];
				LL1 += I2 - I1; break;
			}
			case 'L': { for (i = I1; i < I2 - 1; i++) LowCase(T[i]); LL1 += I2 - I1; break; }
			case 'p':
			case 'P': {
				if (isPrintFile) {
					WriteH(W1, I2 - I1, &T[I1]);
					HMsgExit(CPath);
				}
				else {
					Move(&T[I1], p[fs + 1], I2 - I1);
					fs += I2 - I1; LL1 += I2 - I1;
				}
				break;
			}
			case 'W': {
				SeekH(W1, fs);
				WriteH(W1, I2 - I1, &T[I1]);
				HMsgExit(CPath);
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
				for (i = BegBPos; i < EndBPos - 1; i++) {
					Arr[i] = UpcCharTab[Arr[i]];
				}
				UpdatedL = true;
				break;
			}
			case 'L': {
				for (i = BegBPos; i < EndBPos - 1; i++) {
					LowCase(Arr[i]);
				}
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
	TestLenText(&T, LenT, Part.MovI + 1, 1);
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
		TestLenText(&T, LenT, I1 + L, I1);
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
	NullChangePart();
	TestLenText(&T, LenT, I, longint(I) + I2);
	if (ChangePart) I -= Part.MovI;
	Move(sp->A, &T[I], I2);
	ReleaseStore2(P1);
	SetDekLnCurrI(I + I2);
	EndBLn = Part.LineP + LineL;
	EndBPos = succ(I + I2 - LineI);
	PosDekFindLine(BegBLn, BegBPos, true); /*ChangeScr = true;*/
}

bool BlockCGrasp(char Oper, void* P1, LongStr* sp)
{
	WORD i, I2;
	longint L;
	char* a = nullptr;

	auto result = false;
	if (!BlockExist()) return result;
	TestKod();
	L = LineAbs(LineL);
	if ((L >= BegBLn && L <= EndBLn) && (Posi >= BegBPos + 1 && Posi <= EndBPos - 1) && (Oper != 'G')) return result;
	longint l1 = (EndBLn - BegBLn + 1) * (EndBPos - BegBPos + 2);
	if (l1 > 0x7FFF) { WrLLF10Msg(418); return result; }
	MarkStore2(P1);
	sp = (LongStr*)GetStore2(l1 + 2); sp->LL = l1;
	PosDekFindLine(BegBLn, Posi, false); I2 = 0; i = EndBPos - BegBPos;
	do {
		Move(&Arr[BegBPos], a, i); a[i + 1] = _CR; a[i + 2] = _LF;
		if (Oper == 'M') TestLastPos(EndBPos, BegBPos);
		Move(a, &sp->A[I2 + 1], i + 2); I2 += i + 2;
		TestKod();
		NextLine(false);
	} while (I2 != sp->LL);
	if ((Oper == 'M') && (L >= BegBLn && L <= EndBLn) && (Posi > EndBPos)) {
		Posi -= EndBPos - BegBPos;
	}
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
	if (Oper != 'R') {
		EndBPos = Posi; BegBPos = Posi; BegBLn = LineL + Part.LineP;
	}
	ww = BegBPos; I1 = 1; I3 = 1;
	do {
		if (sp->A[I1] == _CR) {
			InsertLine(i, I1, I3, ww, sp);
			ww = BegBPos; EndBPos = MaxW(ww + i, EndBPos);
			if ((NextI > LenT) && ((TypeT != FileT) || AllRd))
			{
				TestLenText(&T, LenT, LenT, longint(LenT) + 2);
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

bool MyPromptLL(WORD n, std::string* s)
{
	wwmix ww;
	ww.PromptLL(n, s, 1, true);
	return Event.Pressed.KeyCombination() == __ESC;
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
		longint rl = NewRL(LineL);
		if ((rl >= RScrL + PageS) || (rl < RScrL)) {
			if (rl > 10) RScrL = rl - 10;
			else RScrL = 1;
			ChangeScr = true; ScrL = NewL(RScrL);
		}
		LineL = ScrL;
		DekFindLine(LineAbs(LineL));
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
	integer r = ReplaceStr.length();
	integer f = FindStr.length();
	TestLenText(&T, LenT, J, longint(J) + r - f);
	ChangeP(fst);
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
	c2 = screen.WhereX() + FirstC - 1;
	r2 = screen.WhereY() + FirstR;
	w = PushW(1, 1, TxtCols, TxtRows);
	screen.GotoXY(1, TxtRows);
	TextAttr = screen.colors.pTxt;
	ClrEol();
	SetMsgPar(s);
	WriteMsg(n);
	c1 = screen.WhereX(); r1 = screen.WhereY();
	TextAttr = screen.colors.pNorm;
	printf(" ");
	screen.CrsNorm();
	t = Timer + 15;
	r = r1;
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
				ReplaceString(fst, fst, lst, Last);
				UpdStatLine(LineL, Posi, Mode);/*BackGround*/
			}
			else {
				FirstEvent = true;
				Background();
				FirstEvent = false;
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
	return (CountChar(T, LenT, 0x13, 1, MinW(LenT, I - 1)) + 1) / 2;
}

bool WordExist()
{
	return (WordL >= ScrL) && (WordL < ScrL + PageS);
}

WORD WordNo2()
{
	if (WordExist()) return WordNo(SetInd(T, LenT, LineI, Posi));
	return WordNo(ScrI);
}

void ClrWord()
{
	WORD k, m;
	m = 1;
	k = 1;
	k = FindChar(T, LenT, 0x11, k);
	while (k < LenT) {
		T[k] = 0x13;
		m = 1;
		k = FindChar(T, LenT, 0x11, k);
	}
}

bool WordFind(WORD i, WORD WB, WORD WE, WORD LI)
{
	WORD k;
	bool result = false;
	if (i == 0) return result;
	i = i * 2 - 1;
	// TODO: tady puvodne pouzite 'i'
	k = FindChar(T, LenT, 0x13, 1);
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
	if (dir == 'U') {
		DekFindLine(LineL - 1);
		Posi = Position(Colu);
		h2 = MinW(h1, WordNo2() + 1);
	}
	else {
		h2 = h1;
	}
	if (WordFind(h2, I1, I2, I) && (I >= ScrL - 1)) {
		SetWord(I1, I2);
	}
	else {
		if (WordFind(h1 + 1, I1, I2, I) && (I >= ScrL)) SetWord(I1, I2);
		else { I1 = SetInd(T, LenT, LineI, Posi); WordL = 0; }
		I = ScrL - 1;
	}
	if (I <= ScrL - 1) {
		DekFindLine(ScrL);
		RollPred();
	}
	if (WordExist()) SetDekLnCurrI(I1);
}

void HelpRD(char dir)
{
	WORD I = 0, I1 = 0, I2 = 0, h1 = 0, h2 = 0;
	ClrWord();
	h1 = WordNo2();
	if (WordExist()) {
		h1++;
	}
	if (dir == 'D') {
		NextLine(false);
		Posi = Position(Colu);
		while ((Posi > 0) && (Arr[Posi] != 0x13)) Posi--;
		Posi++;
		h2 = MaxW(h1 + 1, WordNo2() + 1);
	}
	else {
		h2 = h1 + 1;
	}
	if (WordFind(h2, I1, I2, I) && (I <= ScrL + PageS)) {
		SetWord(I1, I2);
	}
	else {
		if (WordNo2() > h1) {
			h1++;
		}
		if (WordFind(h1, I1, I2, I) && (I <= ScrL + PageS)) {
			SetWord(I1, I2);
		}
		else {
			I1 = SetInd(T, LenT, LineI, Posi); WordL = 0;
		}
		I = ScrL + PageS;
	}
	if (I >= ScrL + PageS) {
		DekFindLine(ScrL + PageS - 1);
		RollNext();
	}
	if (WordExist()) {
		SetDekLnCurrI(I1);
	}
}

void CursorWord()
{
	std::set<char> O;
	WORD pp;

	LexWord = "";
	pp = Posi;
	if (Mode == HelpM) O.insert(0x11);
	else {
		O = Separ;
		if (O.count(Arr[pp]) > 0) { pp--; }
	}
	while ((pp > 0) && !O.count(Arr[pp])) { pp--; }
	pp++;
	while ((pp <= LastPosLine()) && !O.count(Arr[pp])) {
		LexWord = LexWord + Arr[pp];
		pp++;
	}
}

void Edit()
{
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
		Part.LenP = (WORD)AbsLenT;
		Part.ColorP = "";
		Part.UpdP = false;
		NullChangePart();
		SimplePrintHead();
	}

	FirstScroll = Mode == ViewM;
	const bool keybScroll = GetKeyState(VK_SCROLL) & 0x0001;
	bScroll = (keybScroll || FirstScroll) && (Mode != HelpM);
	if (bScroll)
	{
		ScrL = NewL(RScrL);
		ChangeScr = true;
	}
	HelpScroll = bScroll || (Mode == HelpM);
	if (HelpScroll) {
		screen.CrsHide();
	}
	else {
		screen.CrsNorm();
	}
	BCol = 0;
	BPos = 0;
	SetScreen(IndT, ScrT, Posi);
	Konec = false;

	if (Mode == HelpM) {
		WordL = 0;
		ScrI = SetInd(T, LenT, LineI, Posi);
		if (WordFind(WordNo2() + 1, i1, i2, i3)) {
			SetWord(i1, i2);
		}
		if (!WordExist()) {
			SetDekLnCurrI(IndT);
			ScrI = 1;
		}
	}
	FillChar((char*)MargLL, sizeof(MargLL), 0);
	ColScr = Part.ColorP;
	WrStatusLine();
	TextAttr = TxtColor;
	ClrScr();
	Background();
	FirstEvent = false;

	// {!!!!!!!!!!!!!!}
	if (ErrMsg != "") {
		SetMsgPar(ErrMsg);
		F10SpecKey = 0xffff;
		WrLLF10Msg(110);
		ClearKbdBuf();
		AddToKbdBuf(Event.Pressed.KeyCombination());
	}
	FillChar((char*)MargLL, sizeof(MargLL), 0);
	WrLLMargMsg(LastS, LastNr);

	do {
		if (TypeT == FileT) {
			NullChangePart();
		}
		HandleEvent(Mode, IsWrScreen, SysLColor, LastS, LastNr);
		if (!(Konec || IsWrScreen)) {
			Background();
		}
	} while (!Konec);

	if (bScroll && (Mode != HelpM)) {
		Posi = BPos + 1; LineL = ScrL; LineI = ScrI;
	}

	IndT = SetInd(T, LenT, LineI, Posi);
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

void SetEditTxt(Instr_setedittxt* PD)
{
	if (PD->Insert != nullptr) Insert = !RunBool(PD->Insert);
	if (PD->Indent != nullptr) Indent = RunBool(PD->Indent);
	if (PD->Wrap != nullptr) Wrap = RunBool(PD->Wrap);
	if (PD->Just != nullptr) Just = RunBool(PD->Just);
	if (PD->ColBlk != nullptr) TypeB = RunBool(PD->ColBlk);
	if (PD->Left != nullptr) LeftMarg = MaxI(1, RunInt(PD->Left));
	if (PD->Right != nullptr) RightMarg = MaxI(LeftMarg, MinI(255, RunInt(PD->Right)));
}

void GetEditTxt(bool& pInsert, bool& pIndent, bool& pWrap, bool& pJust, bool& pColBlk, integer& pLeftMarg, integer& pRightMarg)
{
	pInsert = Insert; pIndent = Indent; pWrap = Wrap; pJust = Just; pColBlk = TypeB;
	pLeftMarg = LeftMarg; pRightMarg = RightMarg;
}

bool EditText(char pMode, char pTxtType, std::string pName, std::string pErrMsg, LongStr* pLS, WORD pMaxLen,
	WORD& pInd, longint& pScr, std::vector<WORD> break_keys, EdExitD* pExD, bool& pSrch, bool& pUpdat, WORD pLastNr,
	WORD pCtrlLastNr, MsgStr* pMsgS)
{
	bool oldEdOK = EdOk; EditT = true;
	Mode = pMode;
	TypeT = pTxtType;
	NameT = pName;
	ErrMsg = pErrMsg;
	T = pLS->A;
	MaxLenT = pMaxLen;
	LenT = pLS->LL; IndT = pInd;
	ScrT = pScr & 0xFFFF;
	Posi = pScr >> 16;
	//Breaks = break_keys;
	ExitD = pExD;
	SrchT = pSrch; UpdatT = pUpdat;
	LastNr = pLastNr; CtrlLastNr = pCtrlLastNr;
	if (pMsgS != nullptr) {
		LastS = pMsgS->Last;
		CtrlLastS = pMsgS->CtrlLast;
		ShiftLastS = pMsgS->ShiftLast;
		AltLastS = pMsgS->AltLast;
		HeadS = pMsgS->Head;
	}
	else {
		/*LastS = nullptr; CtrlLastS = nullptr; ShiftLastS = nullptr; AltLastS = nullptr; HeadS = nullptr;*/
		LastS = ""; CtrlLastS = ""; ShiftLastS = ""; AltLastS = ""; HeadS = "";
	}
	if (Mode != HelpM) TxtColor = TextAttr;
	FirstEvent = !SrchT;
	if (SrchT) {
		SrchT = false;
		keyboard.AddToFrontKeyBuf(0x0C); // '^L' .. '\f' .. #12
		/*pstring OldKbdBuffer = KbdBuffer;
		KbdBuffer = 0x0C;
		KbdBuffer += OldKbdBuffer;*/
		Event.Pressed.UpdateKey('L');
		IndT = 0;
	}

	Edit();
	if (Mode != HelpM) { TextAttr = TxtColor; }
	pUpdat = UpdatT;
	pSrch = SrchT;
	pLS->LL = LenT;
	pLS->A = T;
	pInd = IndT;
	pScr = ScrT + ((longint)Posi << 16);
	EdOk = oldEdOK;
	return EditT;
}

void SimpleEditText(char pMode, std::string pErrMsg, std::string pName, LongStr* pLS, WORD MaxLen, WORD& Ind, bool& Updat)
{
	bool Srch; longint Scr;
	Srch = false; Scr = 0;
	EditText(pMode, LocalT, std::move(pName), std::move(pErrMsg), pLS, MaxLen, Ind, Scr, 
		std::vector<WORD>(), nullptr, Srch, Updat, 0, 0, nullptr);
}

WORD FindTextE(const pstring& Pstr, pstring Popt, char* PTxtPtr, WORD PLen)
{
	auto* origT = T;
	T = (char*)PTxtPtr;
	pstring f = FindStr; pstring o = OptionStr;	bool r = Replace;
	FindStr = Pstr; OptionStr = Popt; Replace = false;
	WORD I = 1;
	WORD result;
	if (FindString(I, PLen + 1)) result = I;
	else result = 0;
	FindStr = f; OptionStr = o; Replace = r;
	T = origT;
	return result;
}

void EditTxtFile(longint* LP, char Mode, std::string& ErrMsg, EdExitD* ExD, longint TxtPos, longint Txtxy, WRect* V, WORD Atr, const std::string Hd, BYTE WFlags, MsgStr* MsgS)
{
	bool Srch = false, Upd = false;
	longint Size = 0, L = 0;
	longint w1 = 0;
	ExitRecord er;
	bool Loc = false;
	WORD Ind = 0, oldInd = 0;
	longint oldTxtxy = 0;
	LongStr* LS = nullptr; pstring compErrTxt;
	//T = new char[65536];

	if (Atr == 0) Atr = screen.colors.tNorm;
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
		// zacatek prace se souborem
		OpenTxtFh(Mode);
		RdFirstPart();
		SimplePrintHead();
		while ((TxtPos > Part.PosP + Part.LenP) && !AllRd) RdNextPart();
		Ind = TxtPos - Part.PosP;
	}
	else {
		LS = TWork.Read(1, *LP);
		Ind = TxtPos;
		L = StoreInTWork(LS);
	}
	oldInd = Ind; oldTxtxy = Txtxy;

	//label1:
	while (true) {
		Srch = false; Upd = false;
		if (!Loc) {
			LongStr* LS2 = new LongStr(T, LenT);
			std::vector<WORD> brkKeys = { __F1, __F6, __F9, __ALT_F10 };
			EditText(Mode, FileT, TxtPath, ErrMsg, LS2, 0xFFF0, Ind, Txtxy,
				std::move(brkKeys), ExD, Srch, Upd, 126, 143, MsgS);
			T = LS2->A; LenT = LS2->LL;
		}
		else {
			std::vector<WORD> brkKeys = { __F1, __F6 };
			EditText(Mode, LocalT, "", ErrMsg, LS, MaxLStrLen, Ind, Txtxy,
				std::move(brkKeys), ExD, Srch, Upd, 126, 143, MsgS);
		}
		TxtPos = Ind + Part.PosP;
		if (Upd) EdUpdated = true;
		WORD KbdChar = Event.Pressed.KeyCombination();
		if ((KbdChar == __ALT_EQUAL) || (KbdChar == 'U')) {
			ReleaseStore(LS);
			LS = TWork.Read(1, L);
			if (KbdChar == __ALT_EQUAL) {
				Event.Pressed.UpdateKey(__ESC);
				goto label4;
			}
			else {
				Ind = oldInd; Txtxy = oldTxtxy;
				continue;
			}
		}
		if (!Loc) {
			delete[] T;
			T = nullptr;
		}
		if (EdBreak == 0xFFFF)
			switch (KbdChar) {
			case __F9: {
				if (Loc) { TWork.Delete(*LP); *LP = StoreInTWork(LS); }
				else RdPart();
				continue;
			}
			case __F10: {
				if (Event.Pressed.Alt()) { Help(nullptr, "", false); goto label2; }
				break;
			}
			case __F1: {
				RdMsg(6);
				Help((RdbD*)HelpFD, MsgLine, false);
			label2:
				if (!Loc) RdPart();
				continue;
			}
			}
		if (!Loc) { Size = FileSizeH(TxtFH); CloseH(&TxtFH); }
		if ((EdBreak == 0xFFFF) && (KbdChar == _F6_)) {
			if (Loc) {
				PrintArray(T, LenT, false);
				continue;
			}
			else {
				CPath = TxtPath;
				CVol = TxtVol;
				PrintTxtFile(0);
				OpenTxtFh(Mode);
				RdPart();
				continue;
			}
		}
		if (!Loc && (Size < 1)) MyDeleteFile(TxtPath);
		if (Loc && (KbdChar == __ESC)) LS->LL = LenT;
	label4:
		if (IsCompileErr) {
			IsCompileErr = false;
			compErrTxt = MsgLine;
			SetMsgPar(compErrTxt);
			WrLLF10Msg(110);
		}
		if (Loc) {
			TWork.Delete(L);
			TWork.Delete(*LP); *LP = StoreInTWork(LS);
			ReleaseStore(LS);
		}
		if (w3 != 0) {
			PopW2(w3, (WFlags & WNoPop) == 0);
		}
		if (w2 != 0) {
			PopW(w2);
		}
		PopW(w1);
		LastTxtPos = Ind + Part.PosP;
		RestoreExit(er);

		break;
	}
}

void ViewHelpText(LongStr* S, WORD& TxtPos)
{
	longint L = SavePar();
	TxtColor = screen.colors.hNorm;
	FillChar(ColKey, 8, screen.colors.tCtrl);
	ColKey[5] = screen.colors.hSpec;
	ColKey[3] = screen.colors.hHili;
	ColKey[1] = screen.colors.hMenu;
	bool Srch = false;
	bool Upd = false;
	longint Scr = 0;
	while (true) {
		std::vector<WORD> brkKeys;
		brkKeys.push_back(__F1);
		brkKeys.push_back(__F6);
		brkKeys.push_back(__F10);
		brkKeys.push_back(__CTRL_HOME);
		brkKeys.push_back(__CTRL_END);
		EditText(HelpM, MemoT, "", "", S, 0xFFF0, TxtPos, Scr,
			std::move(brkKeys), nullptr, Srch, Upd, 142, 145, nullptr);
		if (Event.Pressed.KeyCombination() == __F6) {
			PrintArray(&S->A, S->LL, true);
			continue;
		}
		break;
	}
	RestorePar(L);
}

void InitTxtEditor()
{
	FindStr = ""; ReplaceStr = "";
	OptionStr[0] = 0; Replace = false;
	TxtColor = screen.colors.tNorm;
	BlockColor = screen.colors.tBlock;
	SysLColor = screen.colors.fNorm;
	ColKey[0] = screen.colors.tCtrl;
	ColKey[1] = screen.colors.tUnderline;
	ColKey[2] = screen.colors.tItalic;
	ColKey[3] = screen.colors.tDWidth;
	ColKey[4] = screen.colors.tDStrike;
	ColKey[5] = screen.colors.tEmphasized;
	ColKey[6] = screen.colors.tCompressed;
	ColKey[7] = screen.colors.tElite;

	RdMsg(411); InsMsg = MsgLine;
	RdMsg(412); nInsMsg = MsgLine;
	RdMsg(413); IndMsg = MsgLine;
	RdMsg(414); WrapMsg = MsgLine;
	RdMsg(415); JustMsg = MsgLine;
	RdMsg(417); BlockMsg = MsgLine;
	RdMsg(416); ViewMsg = MsgLine;
	Insert = true; Indent = true; Wrap = false; Just = false; TypeB = false;
	LeftMarg = 1; RightMarg = 78;
	CharPg = /*char(250)*/ spec.TxtCharPg;
	InsPg = /*true*/ spec.TxtInsPg;
}
