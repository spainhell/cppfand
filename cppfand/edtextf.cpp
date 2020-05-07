#pragma once

#include "edtextf.h"
#include "edglobal.h"
#include "common.h"
#include "editor.h"
#include "kbdww.h"
#include "legacy.h"
#include "memory.h"
#include "oaccess.h"
#include "sort.h"

bool RdNextPart()
{
	WORD L11, L1;

	longint Max = MinL(MaxLenT, StoreAvail() + LenT);
	WORD Pass = Max - (Max >> 3);
	Part.MovL = 0;
	Part.MovI = 0;
	WORD MI = 0;
	auto result = false;
	if (AllRd) return result;
	if (Part.LenP == 0) { LenT = 0; T = (CharArr*)GetStore(0); }
	longint Pos = Part.PosP;
	longint BL = Part.LineP;

	if (LenT > (Pass >> 1)) {
		LastLine(0, LenT - (Pass >> 1), MI, Part.MovL);     /* 28kB+1 radek*/
		SetColorOrd(Part.ColorP, 1, MI + 1);
		Pos += MI;
		LenT -= MI;
		Move(T[MI + 1], T[1], LenT);
		ReleaseStore(T[LenT + 1]);
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
		CharArr* ppa = (CharArr*)GetStore(L11);
		L1 = L11;
		if (L1 > 0) { SeekH(TxtFH, Pos); ReadH(TxtFH, L1, ppa); }
		AllRd = Pos + L11 >= FSize;
		LenT += L1;
		Pos += L1;
	} while (!((LenT > Pass) || AllRd || (L11 == Max)));

	LastLine(iL, LenT - iL - 1, iL, L1);
	if (AllRd) iL = LenT;
	if ((iL < LenT)) { LenT = iL; AllRd = false; }
	Part.LenP = LenT;
	Part.UpdP = false;
	if ((*T[LenT] == 0x1A) && AllRd) LenT--;
	if ((LenT <= 1)) return result;  /*????????*/
	if (LenT < 49) ReleaseStore(T[LenT]); // TODO: pùvodnì ReleaseStore(@T^[succ(LenT)]);
	result = true;
	return result;
}

void LastLine(WORD from, WORD num, WORD& Ind, WORD& Count)
{
	char* C = nullptr;
	WORD* COfs = (WORD*)C;
	WORD i;
	Count = 0; Ind = from; C = &(*T[from]);
	for (i = 1; i < num; i++)
	{
		COfs++; if (*C == _CR) { Count++; Ind = from + i; };
	}
	if ((Count > 0) && (*T[Ind + 1] == _LF)) Ind++;
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
		AllRd = false; LenT = L1; ReleaseStore(T[LenT + 1]);
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
		Move(T[0], T[L1 + 1], LenT);
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
		Move(T[L1 + 1], T, LenT);
		ReleaseStore(T[LenT + 1]);
	}
	/* !!! with Part do!!! */
	Part.PosP = Pos; Part.LineP = BL - Part.MovL; Part.LenP = LenT;
	Part.MovI = MI; Part.UpdP = false;
	SetColorOrd(Part.ColorP, 1, MI + 1);
	if ((LenT = 0)) return result;  /*????????*/
	result = true;
	return result;
}

void FirstLine(WORD from, WORD num, WORD& Ind, WORD& Count)
{
	char* C = nullptr;
	WORD* COfs = (WORD*)C;
	WORD i;
	Count = 0; Ind = from - 1; C = &*T[from];
	for (i = 0; i < num - 1; i++)
	{
		COfs--; if (*C == _CR) { Count++; Ind = from - i; };
	}
	if ((Count > 0) and (*T[Ind + 1] == _LF)) Ind++;
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
	T = (CharArr*)GetStore(LenT);
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

