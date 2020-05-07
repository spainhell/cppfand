#pragma once

#include "edevproc.h"
#include "common.h"
#include "edevinpt.h"
#include "edglobal.h"
#include "editor.h"
#include "edscreen.h"
#include "edtextf.h"
#include "kbdww.h"
#include "lexanal.h"
#include "memory.h"
#include "printtxt.h"
#include "runedi.h"
#include "wwmix.h"


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
		if (*T[LineI - 1] == _LF) SetDekCurrI(LineI - 2);
		else SetDekCurrI(LineI - 1); LineL--;
		if (LineL < ScrL)
		{
			GotoXY(1, 1); MyInsLine(); ScrL--; ChangeScr = true;
			if (Scroll)
			{ /*dec(RLineL);*/
				RScrL--;
				/*if (ModPage(RLineL))*/
				if (ModPage(RScrL))
				{
					GotoXY(1, 1); MyInsLine();/*dec(RLineL);*/RScrL--;
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
		GotoXY(1, 1); MyDelLine(); ScrL++; ChangeScr = true;
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
		GotoXY(1, 1); MyInsLine(); ScrL--; ChangeScr = true;
		if (LineL == ScrL + PageS)
		{
			TestKod(); LineL--;
			if (*T[LineI - 1] == _LF) SetDekCurrI(LineI - 2);
			else SetDekCurrI(LineI - 1);
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

	UpdStatLine(LineL, Posi); CrsBig(); odir = 0;
	ClrEvent();
	while (true) /* !!! with Event do!!! */
	{
		if (!MyGetEvent() ||
			((Event.What == evKeyDown) && (Event.KeyCode == _ESC_)) || (Event.What != evKeyDown))
		{
			ClrEvent(); CrsNorm(); Mode = TextM; return;
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

void direction1(BYTE x, BYTE& zn2)
{
	BYTE y;
	y = 0x10;
	if (x > 2) y = y << 1;
	if (x == 0) y = 0;
	if (Mode == DouFM) zn2 = zn2 || y;
	else zn2 = zn2 && !y;
}

void CleanFrameM()
{
	if (Mode == SinFM || Mode == DouFM || Mode == DelFM || Mode == NotFM) /* !!! with Event do!!! */
		if (!MyGetEvent() ||
			((Event.What == evKeyDown) && (Event.KeyCode == _ESC_)) || (Event.What != evKeyDown))
		{
			ClrEvent(); CrsNorm(); Mode = TextM;
			UpdStatLine(LineL, Posi); return;
		}
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

void direction2(BYTE x, BYTE& zn2)
{
	BYTE y;
	y = 0x10;
	if (x > 2) y = y << 1;
	if (x == 0) y = 0;
	if (Mode == DouFM) zn2 = zn2 || y;
	else zn2 = zn2 && !y;
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

void MoveB(WORD& B, WORD& F, WORD& T)
{
	if (F <= T) { if (B > F) B += T - F; }
	else if (B >= F) B -= F - T;
	else if (B > T) B = T; B = MinW(B, LastPosLine() + 1);
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
		FillChar(T[LineI + I], Posi - I - 1, 32); NextI += Posi - I - 1;
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
		if (*T[NextI - 1] == _LF) TestLenText(NextI, NextI - 2);
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
	*T[LP] = _CR; *T[succ(LP)] = _LF;
	if (Mode == 'm') { LineL++; LineI = LP + 2; }
	DekodLine();
}

WORD SetPredI()
{
	if ((LineL == 1) && (Part.PosP > 0)) PredPart();
	if (LineI <= 1) return LineI;
	else if (*T[LineI - 1] == _LF) return SetCurrI(LineI - 2);
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
		if ((i < 2) || (*T[i - 1] == _LF))
		{
			while (*T[ii1] == ' ') ii1++; Posit = MaxW(Posit, ii1 - i + 1);
		}
		ii1 = i; RelPos = 1;
		if (Posit > 1)
		{
			Move(T[i], A, Posit);
			for (ii = 1; ii < Posit - 1; i++)
			{
				if (CtrlKey.first(*T[i]) == 0) RelPos++;
				if (*T[i] == _CR) A[ii] = ' ';
				else i++;
			}
			if ((*T[i] == ' ') && (A[Posit - 1] != ' '))
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
					if (CtrlKey.first(*T[i]) == 0) RelPos++;
					if (*T[i] != _CR) i++;
					if (*T[i] == _CR) A[Posit] = ' ';
					else A[Posit] = *T[i];
				}
			while ((RelPos <= RightMarg) && (i < lst))
			{
				if ((*T[i] == _CR) || (*T[i] == ' '))
				{
					while (((*T[i] == _CR) || (*T[i] == ' ')) && (i < lst))
						if (*T[i + 1] == _LF) lst = i;
						else { *T[i] = ' '; i++; }
					if (!bBool) { nw++; if (i < lst) i--; };
				}
				if (i < lst)
				{
					bBool = false;
					A[Posit] = *T[i];
					if (CtrlKey.first(A[Posit]) == 0) RelPos++;
					i++; Posit++;
				}
			}
			if ((i < lst) && (*T[i] != ' ') && (*T[i] != _CR))
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
					while ((*T[i] != ' ') && (*T[i] != _CR) && (Posit < LineSize))
					{
						A[Posit] = *T[i]; i++; Posit++;
					}
					while (((*T[i] == _CR) || (*T[i] == ' ')) && (i < lst))
						if (*T[i + 1] == _LF) lst = i;
						else { *T[i] = ' '; i++; }
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
			if (Posit > 0) Move(A, T[ii1], Posit);
			ii = ii1 + Posit - i; i = ii1 + Posit; lst += ii; llst += ii;
			Posit = 1; RelPos = 1; ii1 = i;
		}
		if (Rep)
		{
			while ((*T[i] == _CR) || (*T[i] == _LF)) i++; fst = i; Rep = i < llst;
			if (llst > LenT) lst = LenT; else lst = llst;
		}
	} while (Rep);
	BegBLn = 1; BegBPos = 1; EndBLn = 1; EndBPos = 1; TypeB = TextBlock;
}

void Calculate_E()
{
	FrmlPtr Z = nullptr; pstring Txt; ExitRecord er; WORD I; pstring Msg;
	void* p = nullptr; char FTyp; double R; bool Del;
	MarkStore(p); //NewExit(Ovr(), er);
	goto label2; ResetCompilePars();
	RdFldNameFrml = RdFldNameFrmlT;
label0:
	Txt = CalcTxt; Del = true; I = 1;
label1:
	TxtEdCtrlUBrk = true; TxtEdCtrlF4Brk = true;
	PromptLL(114, &Txt, I, Del);
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

bool BlockHandle(longint& fs, FILE* W1, char Oper)
{
	WORD i, I1, I2, Ps;
	longint LL1, LL2, Ln;
	ArrLine a = nullptr;
	ColorOrd co; bool isPrintFile = false; CharArr* p = nullptr;
	bool tb; char c;

	TestKod(); Ln = LineAbs(LineL); Ps = Posi;
	if (Oper == 'p') { tb = TypeB; TypeB = TextBlock; }
	else
		if (!BlockExist()) { return false; }
	CrsHide();
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
				for (i = I1; i < I2 - 1; i++) *T[i] = UpcCharTab[*T[i]];
				LL1 += I2 - I1; break;
			}
			case 'L': { for (i = I1; i < I2 - 1; i++) LowCase(*T[i]); LL1 += I2 - I1; break; }
			case 'p':
			case 'P': {
				if (isPrintFile)
				{
					WriteH(W1, I2 - I1, T[I1]); HMsgExit(CPath);
				}
				else {
					Move(T[I1], p[fs + 1], I2 - I1);
					fs += I2 - I1; LL1 += I2 - I1;
				}
				break;
			}
			case 'W': {
				SeekH(W1, fs); WriteH(W1, I2 - I1, T[I1]); HMsgExit(CPath);
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
	if (!bScroll) CrsShow();
	return result;
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
	Move(T[I1], sp->A, L);
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
	Move(sp->A, T[I], I2); ReleaseStore2(P1);
	SetDekLnCurrI(I + I2);
	EndBLn = Part.LineP + LineL;
	EndBPos = succ(I + I2 - LineI);
	PosDekFindLine(BegBLn, BegBPos, true); /*ChangeScr = true;*/
}

bool BlockCGrasp(char Oper, void* P1, LongStr* sp)
{
	WORD i, I2;
	longint l1, L;
	ArrLine a = nullptr;

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
				*T[LenT - 2] = _CR; *T[LenT - 1] = _LF; NextI = LenT;
			}
			NextLine(false);
			;
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

void InsertLine(WORD& i, WORD& I1, WORD& I3, WORD& ww, LongStr* sp)
{
	i = MinW(I1 - I3, LineSize - LastPosLine());
	if (i > 0) { TestLastPos(ww, ww + i); Move(&sp->A[I3], &Arr[ww], i); }
	TestKod();
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

void NewBlock1(WORD& I1, longint& L2)
{
	if (I1 != Posi)
	{
		BegBLn = L2; EndBLn = L2;
		BegBPos = MinW(I1, Posi); EndBPos = MaxW(I1, Posi);
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

void NewBlock2(longint& L1, longint& L2)
{
	if (L1 != L2) {
		BegBPos = Posi; EndBPos = Posi;
		BegBLn = MinL(L1, L2); EndBLn = MaxL(L1, L2);
	}
}

bool MyPromptLL(WORD n, pstring* s)
{
	PromptLL(n, s, 1, true);
	return KbdChar == _ESC_;
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

void ReplaceString(WORD& J, WORD& fst, WORD& lst, longint& Last)
{
	integer r, f;
	r = ReplaceStr.length(); f = FindStr.length();
	TestLenText(J, longint(J) + r - f); ChangeP(fst);
	//if (TestLastPos(Posi, Posi + r - f));
	if (ReplaceStr != "") Move(&ReplaceStr[1], T[J - f], r);
	J += r - f;
	SetScreen(J, 0, 0);
	lst += r - f;
	Last += r - f;
}

char MyVerifyLL(WORD n, pstring s)
{
	longint w, t; WORD c1, c2, r1, r2, r; char cc;
	c2 = WhereX() + FirstC - 1; r2 = WhereY() + FirstR;
	w = PushW(1, 1, TxtCols, TxtRows); GotoXY(1, TxtRows);
	TextAttr = colors.pTxt; ClrEol(); SetMsgPar(s); WriteMsg(n);
	c1 = WhereX(); r1 = WhereY();
	TextAttr = colors.pNorm;
	printf(" "); CrsNorm(); t = Timer + 15; r = r1;
	do {
		while (!KbdPressed())
			if (Timer >= t) {
				t = Timer + 15;
				if (r == r1) { GotoXY(c2, r2); r = r2; }
				else { GotoXY(c1, r1); r = r1; }
			}
		cc = toupper(ReadKbd());
	} while (!(cc == AbbrYes || cc == AbbrNo || cc == _ESC));
	PopW(w);
	return cc;
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
