#include "runfrml.h"
#include "pstring.h"
#include "legacy.h"
#include "rdrun.h"
#include <math.h>

#include "../pascal/random.h"
#include "oaccess.h"
#include "obaseww.h"
#include "olongstr.h"
#include "rdproc.h"
#include "runedi.h"
#include "runproc.h"
#include "wwmix.h"

FileDPtr TFD02;
TFilePtr TF02;
longint TF02Pos; // r33

double Owned(FrmlPtr Bool, FrmlPtr Sum, LinkDPtr LD)
{
	XScan* Scan; KeyDPtr K; XString x; LockMode md; longint n, nBeg;
	FileDPtr cf; void* cr; double r;
	x.PackKF(LD->ToKey->KFlds); cf = CFile; cr = CRecPtr;
	CFile = LD->FromFD; md = NewLMode(RdMode); TestXFExist(); K = GetFromKey(LD);
	if ((Bool == nullptr) && (Sum == nullptr) && !CFile->IsSQLFile) {
		K->FindNr(x, nBeg); x.S[0]; x.S[x.S.length()] = 0xFF;
		K->FindNr(x, n); r = n - nBeg;
	}
	else {
		r = 0; CRecPtr = GetRecSpace();
		//New(Scan, Init(CFile, K, nullptr, true));
		Scan = new XScan(CFile, K, nullptr, true);
		Scan->ResetOwner(&x, nullptr);
	label1:
		Scan->GetRec(); if (!Scan->eof) {
			if (RunBool(Bool)) {
				if (Sum == nullptr) r = r + 1;
				else r = r + RunReal(Sum);
			}
			goto label1;
		}
		Scan->Close(); ReleaseStore(CRecPtr);
	}
	OldLMode(md); CFile = cf; CRecPtr = cr;
	return r;
}

integer CompBool(bool B1, bool B2)
{
	if (B1 > B2) return _gt;
	if (B1 < B2) return _lt;
	return _equ;
}

integer CompReal(double R1, double R2, integer M)
{
	if (M > 0) { R1 = R1 * Power10[M]; R2 = R2 * Power10[M]; }
	if (M >= 0) {
		if (R1 >= 0) R1 = int(R1 + 0.5); else R1 = int(R1 - 0.5);
		if (R2 >= 0) R2 = int(R2 + 0.5); else R2 = int(R2 - 0.5);
	}
	if (R1 > R2) return _gt;
	if (R1 < R2) return _lt;
	return _equ;
}

LongStr* CopyToLongStr(pstring& SS)
{
	WORD l;
	l = SS.length();
	LongStr* s = new LongStr(l);
	s->LL = l;
	Move(&SS[1], s->A, l);
	return s;
}

pstring LeadChar(char C, pstring S)
{
	// TODO: øešit elegantnìji ...
	while (S.length() > 0 && (S[1] == C))
	{
		S = S.substr(1);
	}
	return S;
}

/// funkce má asi oøíznout všechny C na konci øetìzce?
pstring TrailChar(char C, pstring S)
{
	while ((S.length() > 0) && (S[S.length()] == C))
	{
		S[S.length()] = '\0';
		S[0] -= 1;
	}
	return S;
}

double RunRealStr(FrmlElem* X)
{
	WORD I, J, L, N;
	double R = 0.0;
	LongStr* S = nullptr;
	pstring Mask;
	BYTE b = 0;
	double result;
	switch (X->Op) {
	case _valdate: {
		auto iX = (FrmlElem6*)X;
		result = ValDate(RunShortStr(iX->PP1), iX->Mask);
		break;
	}
	case _val: {
		auto iX = (FrmlElem6*)X;
		val(LeadChar(' ', TrailChar(' ', RunShortStr(iX->PP1))), R, I);
		result = R;
		break;
	}
	case _length: {
		auto iX = (FrmlElem6*)X;
		S = RunLongStr(iX->PP1);
		result = S->LL;
		ReleaseStore(S);
		break;
	}
	case _linecnt: {
		auto iX = (FrmlElem6*)X;
		S = RunLongStr(iX->PP1);
		result = int(CountDLines(S->A, S->LL, 0x0D));
		ReleaseStore(S);
		break;
	}
	case _ord: {
		auto iX = (FrmlElem6*)X;
		S = RunLongStr(iX->PP1);
		N = 0;
		if (S->LL > 0) N = S->A[1];
		result = N;
		ReleaseStore(S);
		break;
	}
	case _prompt: {
		auto iX = (FrmlElem11*)X;
		result = PromptR(&RunShortStr(iX->PPP1), iX->PP2, iX->FldD);
		break; }
	case _pos: {
		auto iX = (FrmlElem12*)X;
		S = RunLongStr(iX->PPP2);
		Mask = RunShortStr(iX->PPPP1);
		N = 1;
		if (iX->PP3 != nullptr) N = RunInt(iX->PP3);
		J = 1;
	label1:
		L = S->LL + 1 - J; I = 0;
		if ((N > 0) && (L > 0)) {
			I = FindTextE(Mask, iX->Options, (char*)(&S->A[J]), L);
			if (I > 0) {
				J = J + I - Mask.length(); N--;
				if (N > 0) goto label1; I = J - 1;
			}
		}
		ReleaseStore(S); result = I;
		break;
	}
	case _diskfree: {
		auto iX = (FrmlElem0*)X;
		S = RunLongStr(iX->P1);
		result = DiskFree(toupper(S->A[1]) - '@');
		ReleaseStore(S);
		break;
	}
#ifdef FandSQL
	case _sqlfun: if (Strm1 = nullptr) RunRealStr = 0 else {
		S = RunLongStr(X->P1); RunRealStr = Strm1->SendTxt(S, false);
		ReleaseStore(S);
	}
#endif

	default: result = 0; break;
	}
	return result;
}

double RMod(FrmlElem0* X)
{
	double R1, R2;
	R1 = RunReal(X->P1);
	R2 = RunReal(X->P2);
	return int(R1 - int(R1 / R2) * R2);
}

double LastUpdate(FILE* Handle)
{
	//DateTime dt;
	//UnPackTime(GetDateTimeH(Handle), dt);
	//return RDate(dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec, 0);
	return 0.0;
}

WORD TypeDay(double R)
{
	WORD i, d;
	if ((R >= WDaysFirst) && (R <= WDaysLast))
	{
		d = trunc(R - WDaysFirst);
		for (i = 0; i < NWDaysTab; i++)
			if (WDaysTab[i].Nr == d) { return WDaysTab[i].Typ; }
	}
	d = longint(trunc(R)) % 7;
	switch (d) {
	case 0: d = 2/*Su*/; break;
	case 6: d = 1/*Sa*/; break;
	default: d = 0; break;
	}
	return d;
}

double AddWDays(double R, integer N, WORD d)
{
	if (N > 0) while ((N > 0) && (R <= 748383.0/*2050*/)) {
		R = R + 1; if (TypeDay(R) == d) N--;
	}
	else while ((N < 0) && (R >= 1)) { R = R - 1; if (TypeDay(R) == d) N++; }
	return R;
}

double DifWDays(double R1, double R2, WORD d)
{
	integer N; double x1, x2; bool neg; long double r;
	N = 0;
	x1 = R1; x2 = R2; neg = false;
	if (x1 > x2) { x1 = R2; x2 = R1; neg = true; }
	x1 = x1 + 1;
	if ((x1 >= 697248.0 /*1910*/) && (x2 <= 748383.0 /*2050*/))
		while (x1 <= x2)
		{
			if (TypeDay(x1) == d) N++;
			x1 = x1 + 1;
		}
	if (neg) N = -N;
	return int(N);
}

longint GetFileSize()
{
	FILE* h; FileUseMode um;
	TestMountVol(CPath[1]); um = RdOnly;
	if (IsNetCVol()) um = Shared;
	h = OpenH(_isoldfile, um);
	if (HandleError != 0) {
		return -1;
	}
	auto result = FileSizeH(h);
	CloseH(h);
	return result;
}

longint RecNoFun(FrmlElem13* Z)
{
	KeyDPtr k; FileDPtr cf; void* cr;
	bool b; longint n; LockMode md; XString x;
	GetRecNoXString(Z, x); cf = CFile; cr = CRecPtr; k = Z->Key;
	CFile = Z->FFD; md = NewLMode(RdMode); CRecPtr = GetRecSpace();
	if (CFile->NRecs > 0) {
		if (CFile->Typ == 'X') {
			TestXFExist(); b = k->SearchIntvl(x, false, n);
		}
		else b = SearchKey(x, k, n);
		if (!b) n = -n;
	}
	else n = -1;
	OldLMode(md); ReleaseStore(CRecPtr); CFile = cf; CRecPtr = cr;
	return n;
}

longint AbsLogRecNoFun(FrmlElem13* Z)
{
	longint result = 0;
	void* p = nullptr;
	FileDPtr cf = CFile;
	void* cr = CRecPtr;
	MarkStore(p);
	KeyDPtr k = Z->Key;
	longint N = RunInt(Z->Arg[1]);
	if (N <= 0) return result;
	CFile = Z->FFD;
	LockMode md = NewLMode(RdMode);
	if (N > CFile->NRecs) goto label1;
	if (CFile->Typ == 'X') {
		TestXFExist();
		if (Z->Op == _recnolog) {
			CRecPtr = GetRecSpace();
			ReadRec(N);
			if (DeletedFlag()) goto label1;
			result = k->RecNrToNr(N);
		}
		else /*_recnoabs*/ {
			if (N > k->NRecs()) goto label1;
			result = k->NrToRecNr(N);
		}
	}
	else result = N;
label1:
	OldLMode(md);
	ReleaseStore(p);
	CFile = cf;
	CRecPtr = cr;
	return result;
}

double LinkProc(FrmlElem15* X)
{
	void* p = nullptr;
	longint N;
	FileDPtr cf = CFile; void* cr = CRecPtr; MarkStore(p);
	LinkDPtr LD = X->LinkLD; CFile = LD->FromFD;
	if (X->LinkFromRec) CRecPtr = X->LinkLV->RecPtr;
	else {
		N = RunInt(X->LinkRecFrml);
		LockMode md = NewLMode(RdMode);
		if ((N <= 0) || (N > CFile->NRecs)) {
			Set2MsgPar(CFile->Name, LD->RoleName);
			RunErrorM(md, 609);
		}
		CRecPtr = GetRecSpace(); ReadRec(N); OldLMode(md);
	}
	if (!LinkUpw(LD, N, false)) N = -N;
	auto result = int(N);
	ReleaseStore(p); CFile = cf; CRecPtr = cr;
	return result;
}

WORD IntTSR(FrmlElem* X)
{
	BYTE IntNr; WORD FunNr; void* p; FrmlPtr z;
	pstring s; bool b; double r; LongStr* ss;
	auto iX0 = (FrmlElem0*)X;
	IntNr = RunInt(iX0->P1);
	FunNr = RunInt(iX0->P2);
	z = iX0->P3;
	switch (((FrmlElem1*)X)->N31) {
	case 'r': p = z; break;
	case 'S': { s = RunShortStr(z); p = &s; break; }
	case 'B': { b = RunBool(z); p = &b; break; }
	case 'R': { r = RunReal(z); p = &r; break; }
	}
	/*asm  push ds; mov cl, IntNr; mov ax, FunNr; lds dx, p;
	call @1; int 80H; pop ds; jmp @2;
	@pop 1 bx; push cs; push bx;
	mov cs : [bx + 1] , cl;  ret;  // modify int-instruction; far ret
	@mov 2 @result, ax;*/

	if (z->Op == _getlocvar) {
		p = (void*)(MyBP + ((FrmlElem18*)z)->BPOfs);
		switch (((FrmlElem1*)X)->N31) {
		case 'R': p = (void*)&r; break;
		case 'S': {
			ss = CopyToLongStr(s);
			TWork.Delete((longint)p);
			auto tmp = TWork.Store(ss);
			p = &tmp;
			ReleaseStore(ss);
			break;
		}
		case 'B': p = &b; break;
		}
	}
	return 0; // asi se vrací nìco z ASM kódu
}

WORD PortIn(bool IsWord, WORD Port)
{
	/*asm mov dx, Port;xor ax, ax; cmp IsWord, 0; je @1; in ax, dx; jmp @2;
	@1:   in al, dx;
	@2:;*/
	return 0;
}

LongStr* CopyLine(LongStr* S, WORD N, WORD M)
{
	WORD i = 1;
	if (N > 1) { i = FindCtrlM(S, 1, N - 1); i = SkipCtrlMJ(S, i); }
	WORD j = FindCtrlM(S, i, M);
	WORD l = j - i;
	if ((i > 1) && (l > 0)) Move(&S->A[i], &S->A[1], l);
	S->LL = l;
	ReleaseAfterLongStr(S);
	return S;
}

bool RunBool(FrmlPtr X)
{
	longint RecNo;
	WORD* res = (WORD*)&RecNo;
	LongStr* S = nullptr;
	LongStr* S2 = nullptr;
	bool* b = (bool*)S;
	WORD* w1 = (WORD*)&RecNo;
	WORD* w2 = (WORD*)S;
	FileD* cf = nullptr;
	void* cr = nullptr;
	void* p = &RecNo;

	auto iX0 = (FrmlElem0*)X;
	auto iX1 = (FrmlElem1*)X;

	auto result = false;
	if (X == nullptr) { return true; }
	switch (X->Op) {
	case _and: if (RunBool(iX0->P1)) result = RunBool(iX0->P2); else result = false; break;
	case _or: if (RunBool(iX0->P1)) result = true; else result = RunBool(iX0->P2); break;
	case _lneg: result = !RunBool(iX0->P1); break;
	case _limpl: if (RunBool(iX0->P1)) result = RunBool(iX0->P2);
			   else result = true; break;
	case _lequ: if (RunBool(iX0->P1) == RunBool(iX0->P2)) result = true;
			  else result = false;
	case _instr: {
		S = RunLongStr(iX0->P1);
		if (iX1->N11 == 1) result = LexInStr(LongTrailChar(' ', 0, S), &iX1->N12);
		else result = InStr(S, &iX1->N12);
		ReleaseStore(S);
		break;
	}
	case _inreal: { result = InReal(RunReal(iX0->P1), &iX1->N12, iX1->N11); break; }
	case _compreal: {
		auto rrP1 = RunReal(iX0->P1);
		auto rrP2 = RunReal(iX0->P2);
		auto cmpR = CompReal(rrP1, rrP2, iX1->N22);
		result = (cmpR & iX1->N21) != 0;
		break;
	}
	case _compstr: {
		S = RunLongStr(iX0->P1); S2 = RunLongStr(iX0->P2);
		if (iX1->N22 == 1)
			*res = CompLexLongStr(LongTrailChar(' ', 0, S), LongTrailChar(' ', 0, S2));
		else *res = CompLongStr(S, S2);
		result = res && iX1->N21 != 0; ReleaseStore(S); break; }
	case _const: result = ((FrmlElem5*)X)->B; break;
	case _mouseevent: {
	label2:
		Event.What = 0;
		GetMouseEvent();
		if (Event.What == 0) result = false;
		else { if ((Event.What && iX1->W01) == 0) goto label2; result = true; }
		break;
	}
	case _ismouse: {
		result = false;
		if (((Event.What && iX1->W01) != 0) && ((Event.Buttons && iX1->W02) == iX1->W02))
			result = true;
		break;
	}
	case _mousein: {
		*w1 = RunInt(iX0->P1);
		*w2 = RunInt(iX0->P2);
		result = MouseInRectProc(*w1, *w2, RunInt(iX0->P3) - *w1 + 1, RunInt(iX0->P4) - *w2 + 1);
		break;
	}
	case _getlocvar: result = (bool*)(&MyBP); break;
	case _modulo: result = RunModulo((FrmlElem1*)X); break;
	case _field: result = _B(((FrmlElem7*)X)->Field); break;
	case _access: {
		auto iX = (FrmlElem7*)X;
		cf = CFile; cr = CRecPtr;
		if (iX->LD != nullptr) *b = LinkUpw(iX->LD, RecNo, false);
		else *b = LinkLastRec(iX->File2, RecNo, false);
		if ((iX->P011 == nullptr)) result = b;
		else result = RunBool(iX->P011);
		ReleaseStore(CRecPtr); CFile = cf; CRecPtr = cr;
		break;
	}
	case _recvarfld: {
		auto iX = (FrmlElem7*)X;
		cf = CFile; cr = CRecPtr;
		CFile = iX->File2; CRecPtr = iX->LD; result = RunBool(iX->P011);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _eval: {
		MarkStore(p); result = RunBool(GetEvalFrml((FrmlElem21*)X));
		ReleaseStore(p);
		break;
	}
	case _newfile: {
		auto iX = (FrmlElem8*)X;
		cf = CFile; cr = CRecPtr;
		CFile = iX->NewFile; CRecPtr = iX->NewRP;
		result = RunBool(iX->Frml);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _prompt: {
		auto iX = (FrmlElem11*)X;
		result = PromptB(&RunShortStr(iX->PPP1), iX->PP2, iX->FldD); break; }
	case _promptyn: { SetMsgPar(RunShortStr(iX0->P1)); result = PromptYN(110); break; }
	case _accrecno: {
		auto iX = (FrmlElem14*)X;
		cf = CFile; cr = CRecPtr; AccRecNoProc(iX, 640);
		result = _B(iX->RecFldD); ReleaseStore(CRecPtr);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _edupdated: result = EdUpdated; break;
	case _keypressed: result = KeyPressed(); break;/*Kbdpressed?*/
	case _escprompt: result = EscPrompt; break;
	case _isdeleted: {
		cr = CRecPtr; cf = CFile;
		AccRecNoProc((FrmlElem14*)X, 642);
		result = DeletedFlag(); ReleaseStore(CRecPtr);
		CRecPtr = cr; CFile = cf;
		break;
	}
	case _lvdeleted: {
		auto iX = (FrmlElem20*)X;
		cr = CRecPtr; cf = CFile;
		CRecPtr = iX->LV->RecPtr; CFile = iX->LV->FD;
		result = DeletedFlag(); CRecPtr = cr; CFile = cf;
		break;
	}
	case _trust: result = (UserCode == 0) || OverlapByteStr(&iX1->N01, &AccRight); break;
	case _isnewrec: result = TestIsNewRec(); break;
	case _testmode: result = IsTestRun; break;
	case _equmask: result = RunEquMask((FrmlElem0*)X); break;
	case _userfunc: {
		result = *(bool*)(RunUserFunc((FrmlElem19*)X));
		cr = MyBP; PopProcStk(); ReleaseStore(cr);
		break;
	}
	case _setmybp: {
		cr = MyBP; SetMyBP(ProcMyBP);
		result = RunBool(iX0->P1); SetMyBP((ProcStkD*)cr);
		break;
	}
	}
	return result;
}

bool InReal(double R, BYTE* L, integer M)
{
	WORD* LOffs = (WORD*)L; double* Cr = (double*)L; integer I, N;
	auto result = true;
label1:
	N = *L;
	LOffs++;
	if (N == 0) { result = false; exit; }
	if (N == 0xFF)
		if (CompReal(R, *Cr, M) == _lt) LOffs += 2 * sizeof(double);
		else {
			LOffs += sizeof(double);
			if (CompReal(R, *Cr, M) != _gt) return result;
			LOffs += sizeof(double);
		}
	else for (I = 1; I < N; I++)
		if (CompReal(R, *Cr, M) == _equ) return result;
		else LOffs += sizeof(double);
	goto label1;
}

bool LexInStr(LongStr* S, BYTE* L)
{
	WORD* LOffs = (WORD*)L; pstring* Cs = (pstring*)L; integer I, N;
	auto result = true;
label1:
	N = *L;
	*LOffs += 1;
	if (N == 0) { result = false; return result; }
	if (N == 0xFF) {
		if (CompLexLongShortStr(S, *Cs) == _lt) {
			*LOffs += *L + 1; *LOffs += *L + 1;
		}
		else {
			*LOffs += *L + 1;
			if (CompLexLongShortStr(S, *Cs) != _gt) return result;
			*LOffs += *L + 1;
		}
	}
	else {
		for (I = 1; I < N; I++) {
			if (CompLexLongShortStr(S, *Cs) == _equ) return result;
			else *LOffs += *L + 1;
		}
	}
	goto label1;
}

bool InStr(LongStr* S, BYTE* L)
{
	WORD* LOffs = (WORD*)L; pstring* Cs = (pstring*)L; integer I, N;

	auto result = true;
label1:
	N = *L;
	*LOffs++;
	if (N == 0) { result = false; return result; }
	if (N == 0xFF) {
		if (CompLongShortStr(S, *Cs) == _lt) {
			*LOffs += *L + 1; *LOffs += *L + 1;
		}
		else {
			*LOffs += *L + 1;
			if (CompLongShortStr(S, *Cs) != _gt) return result; *LOffs += *L + 1;
		}
	}
	else {
		for (I = 1; I < N; I++)
			if (CompLongShortStr(S, *Cs) == _equ) return result;
			else *LOffs += *L + 1;
	}
	goto label1;
}

bool RunModulo(FrmlElem1* X)
{
	pstring S; integer I, M, N; WORD* B1 = nullptr; WORD* B1Offs = B1;
	N = X->W11;
	S = RunShortStr(((FrmlElem0*)X)->P1);
	if (S.length() != N) { return false; }
	M = 0;
	*B1 = X->W21;
	for (I = 1; I < N - 1; I++) {
		M = M + *B1 * (S[I] & 0x0F);
		*B1Offs += 2;
	}
	I = (X->W12 - (M % X->W12)) % 10;
	if (I == (S[N] & 0x0F)) return true;
	return false;
}

bool RunEquMask(FrmlElem0* X)
{
	LongStr* s;
	s = RunLongStr(X->P1); auto result = EqualsMask(s->A, s->LL, RunShortStr(X->P2));
	ReleaseStore(s);
	return result;
}

double RunReal(FrmlElem* X)
{
	if (X == nullptr) return 0;

	double R = 0.0; FileD* cf = nullptr; LockMode md; bool b = false;
	longint RecNo = 0; void* p = &RecNo; void* cr = nullptr;
#ifdef FandGraph
	ViewPortType vp/*9 BYTE*/ absolute R;
#endif
	FILE* h = (FILE*)&R;
	WORD* n = (WORD*)cf;
	BYTE* AColors = (BYTE*)&colors;
	double result = 0;
	auto iX0 = (FrmlElem0*)X;
label1:
	switch (X->Op) {
	case _field: {
		auto iX = (FrmlElem7*)X;
		result = _R(iX->Field);
		break;
	}
	case _getlocvar: {
		auto iX = (FrmlElem18*)X;
		result = *(double*)(uintptr_t(MyBP) + iX->BPOfs);
		break;
	}
	case _const: result = ((FrmlElem2*)X)->R; break;
	case _plus: {
		result = RunReal(iX0->P1) + RunReal(iX0->P2); break;
	}
	case _minus: {
		result = RunReal(iX0->P1) - RunReal(iX0->P2); break;
	}
	case _times: {
		result = RunReal(iX0->P1) * RunReal(iX0->P2); break;
	}
	case _access: {
		auto iX = (FrmlElem7*)X;
		cf = CFile;
		cr = CRecPtr;
		if (iX->LD != nullptr) LinkUpw(iX->LD, RecNo, false);
		else LinkLastRec(iX->File2, RecNo, false);
		result = RunReal(iX->P011);
		ReleaseStore(CRecPtr);
		CFile = cf;
		CRecPtr = cr;
		break;
	}
	case _recvarfld: {
		auto iX = (FrmlElem7*)X;
		cf = CFile;
		cr = CRecPtr;
		CFile = iX->File2;
		CRecPtr = iX->LD;
		result = RunReal(iX->P011);
		CFile = cf;
		CRecPtr = cr;
		break;
	}
	case _eval: {
			MarkStore(p);
			result = RunReal(GetEvalFrml((FrmlElem21*)X));
			ReleaseStore(p);
			break;
	}
	case _divide: {
		result = RunReal(iX0->P1) / RunReal(iX0->P2);
		break;
	}
	case _cond: {
	label2:
		auto iX = (FrmlElem0*)X;
		if (iX->P1 != nullptr)
			if (!RunBool(iX->P1))
			{
				if (iX->P3 == nullptr) { result = 0; return result; }
				X = iX->P3;
				goto label2;
			}
		X = iX->P2;
		goto label1;
	}
	case _newfile: {
		auto iX = (FrmlElem8*)X;
		cf = CFile; cr = CRecPtr;
		CFile = iX->NewFile;
		CRecPtr = iX->NewRP;
		result = RunReal(iX->Frml);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _getWORDvar: result = (int)WordVarArr[((FrmlElem1*)X)->N01]; break;
	case _div: result = (int)(RunReal(iX0->P1) / RunReal(iX0->P2)); break;
	case _mod: result = RMod((FrmlElem0*)X); break;
	case _unminus: result = -RunReal(iX0->P1); break;
	case _today: result = Today(); break;
	case _pi: result = atan(1.0) * 4; break;
	case _random: result = Random(); break;
	case _round: result = RoundReal(RunReal(iX0->P1), RunInt(iX0->P2)); break;
	case _abs: result = abs(RunReal(iX0->P1)); break;
	case _int: result = (int)(RunReal(iX0->P1)); break;
	case _frac: result = modf(RunReal(iX0->P1), nullptr); break;
	case _sqr: result = pow(RunReal(iX0->P1), 2); break;
	case _sqrt: result = sqrt(RunReal(iX0->P1)); break;
	case _sin: result = sin(RunReal(iX0->P1)); break;
	case _cos: result = cos(RunReal(iX0->P1)); break;
	case _arctan: result = atan(RunReal(iX0->P1)); break;
	case _ln: result = log(RunReal(iX0->P1));
	case _exp: {
		R = RunReal(iX0->P1);
		if ((R <= -50) || (R > 88)) result = 0;
		else result = exp(R);
		break;
	}
	case _nrecs:
	case _nrecsabs: {
		auto iX = (FrmlElem9*)X;
		cf = CFile; CFile = iX->FD; md = NewLMode(RdMode);
		if (X->Op == _nrecs) RecNo = XNRecs(CFile->Keys);
		else RecNo = CFile->NRecs;
		OldLMode(md); result = int(RecNo); CFile = cf;
		break;
	}
	case _generation: {
		auto iX = (FrmlElem9*)X;
		cf = CFile; CFile = iX->FD;
		result = (int)Generation(); CFile = cf;
		break;
	}
	case _lastupdate: {
		cf = CFile;
		auto iX = (FrmlElem9*)X;
		CFile = iX->FD; md = NewLMode(RdMode);
		result = LastUpdate(CFile->Handle); OldLMode(md); CFile = cf;
		break;
	}
	case _catfield: {
		auto iX = (FrmlElem10*)X;
		RdCatPathVol(iX->CatIRec);
		TestMountVol(CPath[1]);
		h = OpenH(_isoldfile, RdOnly);
		result = LastUpdate(h);
		CloseH(h);
		break;
	}
	case _currtime: result = CurrTime(); break;
	case _typeday: {
		auto rr = RunReal(iX0->P1);
		result = TypeDay(rr);
		break;
	}
	case _addwdays: result = AddWDays(RunReal(iX0->P1), RunInt(iX0->P2), ((FrmlElem1*)X)->N21); break;
	case _difwdays: result = DifWDays(RunReal(iX0->P1), RunReal(iX0->P2), ((FrmlElem1*)X)->N21); break;
	case _addmonth: result = AddMonth(RunReal(iX0->P1), RunReal(iX0->P2)); break;
	case _difmonth: result = DifMonth(RunReal(iX0->P1), RunReal(iX0->P2)); break;
	case _recno: result = RecNoFun((FrmlElem13*)X); break;
	case _recnoabs:
	case _recnolog: result = AbsLogRecNoFun((FrmlElem13*)X); break;
	case _accrecno: {
		auto iX = (FrmlElem14*)X;
		cf = CFile; cr = CRecPtr; AccRecNoProc(iX, 640);
		result = _R(iX->RecFldD);
		ReleaseStore(CRecPtr);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _link: result = LinkProc((FrmlElem15*)X); break;
	case _memavail: result = int(StoreAvail); break;
	case _maxcol: result = int(TxtCols); break;
	case _maxrow: result = int(TxtRows); break;
#ifdef FandGraph
	case _getmaxx: if (IsGraphMode) {
		GetViewSettings(vp); result = vp.x2 - vp.x1;
				 else result = 0;
		break;
	}
	case _getmaxy: if (IsGraphMode) { GetViewSettings(vp); result = vp.y2 - vp.y1; }
				 else result = 0;
		break;
#endif
	case _exitcode: result = LastExitCode; break;
	case _edrecno: result = EdRecNo; break;
	case _txtpos: result = LastTxtPos; break;
	case _txtxy: result = TxtXY; break;
	case _cprinter: result = prCurr; break;
	case _mousex: {if (IsGraphMode) result = Event.WhereG.X;
				else result = Event.Where.X + 1; break; }
	case _mousey: {if (IsGraphMode) result = Event.WhereG.Y;
				else result = Event.Where.Y + 1; break; }
	case _filesize: {
		auto iX = (FrmlElem16*)X;
		SetTxtPathVol(*iX->TxtPath, iX->TxtCatIRec);
		result = GetFileSize();
		break;
	}
	case _inttsr: result = IntTSR(X);
	case _userfunc: {
		result = *(double*)RunUserFunc((FrmlElem19*)X);
		cr = MyBP; PopProcStk(); ReleaseStore(cr);
		break;
	}
	case _indexnrecs: {
		auto iX = (FrmlElem22*)X;
		result = iX->WKey->NRecs();
		break;
	}
	case _owned: {
		auto iX = (FrmlElem23*)X;
		result = Owned(iX->ownBool, iX->ownSum, iX->ownLD);
		break;
	}
	case _color: result = AColors[MinW(WORD(RunInt(iX0->P1)), 53)]; break;
	case _portin: result = PortIn(RunBool(iX0->P1), WORD(RunInt(iX0->P2))); break;
	case _setmybp: {
		cr = MyBP; SetMyBP(ProcMyBP);
		result = RunReal(iX0->P1); SetMyBP((ProcStkD*)cr);
		break;
	}
	default: { result = RunRealStr(X); break; }
	}
	return result;
}

longint RunInt(FrmlPtr X)
{
	auto rr = RunReal(X);
	return trunc(rr);
}

void TestTFrml(FieldDescr* F, FrmlElem* Z)
{
	FileDPtr cf = nullptr; void* p = nullptr;
	longint n; LockMode md; FieldDPtr f1 = nullptr;
	switch (Z->Op) {
	case _newfile: {
		auto iZ = (FrmlElem8*)Z;
		CFile = iZ->NewFile; CRecPtr = iZ->NewRP; TestTFrml(F, iZ->Frml);
		break;
	}
	case _field: {
		auto iZ = (FrmlElem7*)Z;
		f1 = iZ->Field;
		if ((f1->Typ != 'T') || ((f1->Flg & f_Stored) == 0)) return;
		if (F == nullptr) { if ((f1->Flg & f_Encryp) != 0) return; }
		else if ((F->Flg & f_Encryp) != (f1->Flg & f_Encryp)) return;
		TFD02 = CFile; TF02 = CFile->TF;
		if (HasTWorkFlag()) TF02 = &TWork;
		TF02Pos = _T(f1);
		break;
	}
	case _getlocvar: {
		if ((F != nullptr) && ((F->Flg & f_Encryp) != 0)) return;
		TFD02 = CFile; TF02 = &TWork;
		TF02Pos = *(longint*)(MyBP + ((FrmlElem18*)Z)->BPOfs);
		break;
	}
	case _access: {
		auto iZ = (FrmlElem7*)Z;
		cf = CFile; MarkStore(p); CFile = iZ->File2;
		md = NewLMode(RdMode);
		if (iZ->LD != nullptr) { CFile = cf; LinkUpw(iZ->LD, n, true); }
		else LinkLastRec(iZ->File2, n, true);
		TestTFrml(F, iZ->P011);
		CFile = iZ->File2; OldLMode(md); ReleaseStore(p);
		break;
	}
	case _recvarfld: {
		auto iZ = (FrmlElem7*)Z;
		CFile = iZ->File2; CRecPtr = iZ->LD; TestTFrml(F, iZ->P011);
		break;
	}
	}
}

bool CanCopyT(FieldDPtr F, FrmlPtr Z)
{
	FileDPtr cf; void* cr;
	auto result = false;
	cf = CFile; cr = CRecPtr; TF02 = nullptr; result = false; TestTFrml(F, Z);
	CFile = cf; CRecPtr = cr; result = TF02 != nullptr;
	return result;
}

bool TryCopyT(FieldDPtr F, TFilePtr TF, longint& pos, FrmlPtr Z)
{
	LockMode md, md2;
	bool result = false;
	if (TF->Format == TFile::DbtFormat || TF->Format == TFile::FptFormat) return result;
	if ((BYTE)Z->Op == _gettxt) { pos = CopyTFFromGetTxt(TF, Z); result = true; }
	else if (CanCopyT(F, Z) && (TF02->Format == TF->Format)) {
		result = true; pos = CopyTFString(TF, TFD02, TF02, TF02Pos);
	}
	return result;
}

void AssgnFrml(FieldDPtr F, FrmlPtr X, bool Delete, bool Add)
{
	LongStr* s; longint pos; TFilePtr tf;
	switch (F->FrmlTyp) {
	case 'S': {
		if (F->Typ == 'T') {
			if (HasTWorkFlag) tf = &TWork;
			else tf = CFile->TF;
			if (TryCopyT(F, tf, pos, X)) {
				if (Delete) DelTFld(F); T_(F, pos);
			}
			else {
				s = RunLongStr(X); if (Delete) DelTFld(F); LongS_(F, s);
				ReleaseStore(s);
			}
		}
		else S_(F, RunShortStr(X));
		break;
	}
	case 'R': { if (Add) R_(F, _R(F) + RunReal(X)); else R_(F, RunReal(X)); break; }
	case 'B': { B_(F, RunBool(X)); break;	}
	}
}

void LVAssignFrml(LocVar* LV, void* OldBP, bool Add, FrmlPtr X)
{
	longint pos = 0;
	void* p = LocVarAd(LV);
	void* bp = MyBP;
	SetMyBP((ProcStkD*)OldBP);
	switch (LV->FTyp) {
	case 'S': {
		if (!TryCopyT(nullptr, &TWork, pos, X)) {
			LongStr* s = RunLongStr(X);
			pos = TWork.Store(s);
			ReleaseStore(s);
		}
		TWork.Delete(*(longint*)(p));
		p = &pos;
		break;
	}
	case 'R': {
		if (Add) *(double*)(p) = *(double*)(p)+RunReal(X);
		else *(double*)(p) = RunReal(X);
		break;
	}
	case 'B': {
		*(bool*)(p) = RunBool(X);
		break;
	}
	}
	SetMyBP((ProcStkD*)bp);
}

void DecodeFieldRSB(FieldDPtr F, WORD LWw, double R, pstring T, bool B, pstring& Txt)
{
	WORD L = 0, M = 0; char C = 0;
	L = F->L; M = F->M;
	switch (F->Typ) {
	case 'D':T = StrDate(R, FieldDMask(F)); break;
	case 'N': { C = '0'; goto label1; break; }
	case 'A': { C = ' ';
	label1:
		if (M == LeftJust)
			while (T.length() < L) T.Append(C);
		else while (T.length() < L) { pstring oldT = T; T = C; T += oldT; }
		break;
	}
	case 'B': {
		if (B) T = AbbrYes;
		else T = AbbrNo;
		break;
	}
	case 'R': str(R, L, T); break;
	default: /*"F"*/ {
		if (F->Flg && f_Comma != 0) R = R / Power10[M];
		str(RoundReal(R, M), L, M, T);
		break;
	}
	}
	if (T.length() > L) { T[0] = char(L); T[L] = '>'; }
	if (T.length() > LWw) {
		if (M == LeftJust) { T[0] = (unsigned char)LWw; }
		else { T = copy(T, T.length() - LWw + 1, LWw); }
	}
	Txt = T;
}

void DecodeField(FieldDPtr F, WORD LWw, pstring& Txt)
{
	double r = 0;
	pstring s;
	bool b = false;;
	switch (F->FrmlTyp) {
	case 'R': r = _R(F); break;
	case 'S': {
		if (F->Typ == 'T') {
			if (((F->Flg & f_Stored) != 0) && (_R(F) == 0)) Txt = ".";
			else Txt = "*";
			return;
		}
		else s = _ShortS(F);
		break;
	}
	default: b = _B(F); break;
	}
	DecodeFieldRSB(F, LWw, r, s, b, Txt);
}

void RunWFrml(WRectFrml& X, BYTE WFlags, WRect& W)
{
	W.C1 = RunInt(X.C1);
	W.R1 = RunInt(X.R1);
	W.C2 = RunInt(X.C2);
	W.R2 = RunInt(X.R2);
	CenterWw(W.C1, W.R1, W.C2, W.R2, WFlags);
}

WORD RunWordImpl(FrmlElem* Z, WORD Impl)
{
	WORD n = RunInt(Z);
	if (n == 0) n = Impl;
	return n;
}

bool FieldInList(FieldDPtr F, FieldListEl* FL)
{
	auto result = false;
	while (FL != nullptr) {
		if (FL->FldD == F) result = true;
		FL = (FieldListEl*)FL->Chain;
	}
	return result;
}

KeyDPtr GetFromKey(LinkDPtr LD)
{
	KeyD* K;
	K = LD->FromFD->Keys;
	while (K->IndexRoot != LD->IndexRoot) K = K->Chain;
	return K;
}

FrmlPtr RunEvalFrml(FrmlPtr Z)
{
	if ((Z != nullptr) && ((BYTE)Z->Op == _eval)) Z = GetEvalFrml((FrmlElem21*)Z);
	return Z;
}

LongStr* RunLongStr(FrmlPtr X)
{
	LongStr* S = nullptr;
	bool b = false;
	WORD I = 0;
	//LockMode* md = (LockMode*)&I;
	//integer* J = (integer*)&I;
	longint RecNo = 0;
	WORD* N = (WORD*)&RecNo;
	longint* L1 = (longint*)&RecNo;
	FileDPtr cf = nullptr;
	void* cr = nullptr;
	longint* L2 = (longint*)cr;
	void* p = &RecNo;

	LongStr* result = nullptr;

	auto iX0 = (FrmlElem0*)X;
	auto iX7 = (FrmlElem7*)X;

	if (X == nullptr) return new LongStr(2);
label1:
	switch (X->Op) {
	case _field: result = _LongS(iX7->Field); break;
	case _getlocvar: result = TWork.Read(1, *(longint*)(MyBP + ((FrmlElem18*)X)->BPOfs)); break;
	case _access: {
		cf = CFile; cr = CRecPtr;
		CFile = iX7->File2;
		//*md = NewLMode(RdMode);
		I = (WORD)NewLMode(RdMode);
		if (iX7->LD != nullptr) {
			CFile = cf;
			LinkUpw(iX7->LD, RecNo, true);
		}
		else LinkLastRec(iX7->File2, RecNo, true);
		S = RunLongStr(iX7->P011);
		OldLMode((LockMode)I);  /*possibly reading .T*/
		ClearRecSpace(CRecPtr);
		//memcpy(CRecPtr, &S->LL, sizeof(S->LL));
		//memcpy(&((BYTE*)CRecPtr)[sizeof(S->LL)], S->A, S->LL);
		//MyMove(S, CRecPtr, S->LL + 2);
		ReleaseAfterLongStr(CRecPtr);
		//result = (LongStr*)CRecPtr;
		result = S;
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _recvarfld: {
		cf = CFile; cr = CRecPtr;
		CFile = iX7->File2; CRecPtr = iX7->LD;
		result = RunLongStr(iX7->P011);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _eval: {
		MarkStore(p);
		S = RunLongStr(GetEvalFrml((FrmlElem21*)X)); MyMove(S, p, S->LL + 2);
		ReleaseAfterLongStr(p); result = (LongStr*)p;
		break;
	}
	case _newfile: {
		auto iX = (FrmlElem8*)X;
		cf = CFile; cr = CRecPtr;
		CFile = iX->NewFile; CRecPtr = iX->NewRP;
		result = RunLongStr(iX->Frml);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _cond: {
	label2:
		if (iX0->P1 != nullptr)
			if (!RunBool(iX0->P1))
			{
				if (iX0->P3 == nullptr)
				{
					return new LongStr(2);
				}
				X = iX0->P3;
				goto label2;
			}
		X = iX0->P2;
		goto label1;
		break;
	}
	case _copy: {
		S = RunLongStr(iX0->P1);
		*L1 = RunInt(iX0->P2);
		*L2 = RunInt(iX0->P3);
		if ((*L1 < 0) || (*L2 < 0)) S->LL = 0;
		else CopyLongStr(S, (WORD)*L1, (WORD)*L2);
		ReleaseAfterLongStr(S);
		result = S;
		break;
	}
	case _concat: {
		S = RunLongStr(iX0->P1);
		ConcatLongStr(S, RunLongStr(iX0->P2));
		ReleaseAfterLongStr(S); result = S;
		break;
	}
	case _const: result = CopyToLongStr(((FrmlElem4*)X)->S); break;
	case _leadchar: {
		auto iX = (FrmlElem1*)X;
		result = LongLeadChar((char)iX->N11, (char)iX->N12, RunLongStr(iX0->P1));
		break;
	}
	case _trailchar: {
		auto iX = (FrmlElem1*)X;
		result = LongTrailChar((char)iX->N11, (char)iX->N12, RunLongStr(iX0->P1));
		break;
	}
	case _upcase: {
		S = RunLongStr(iX0->P1);
		for (WORD i = 1; i < S->LL; i++) S->A[i] = UpcCharTab[S->A[i]];
		result = S;
		break;
	}
	case _lowcase: {
		S = RunLongStr(iX0->P1); LowCase(S); result = S;
		break;
	}
	case _copyline: {
		I = 1;
		if (iX0->P3 != nullptr) I = (WORD)RunInt(iX0->P3);
		result = CopyLine(RunLongStr(iX0->P1), RunInt(iX0->P2), I);
		break;
	}
	case _repeatstr: result = RepeatStr(RunLongStr(iX0->P1), RunInt(iX0->P2)); break;
	case _accrecno: {
		auto iX = (FrmlElem14*)X;
		cf = CFile; cr = CRecPtr;
		AccRecNoProc(iX, 640);
		S = _LongS(iX->RecFldD);
		MyMove(S, CRecPtr, S->LL + 2);
		ReleaseAfterLongStr(CRecPtr);
		result = (LongStr*)CRecPtr;
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _gettxt: result = GetTxt(X); break;
	case _nodiakr: {
		S = RunLongStr(iX0->P1); ConvToNoDiakr((WORD*)S->A[0], S->LL, fonts.VFont);
		result = S;
		break;
	}
	case _userfunc: {
		cr = RunUserFunc((FrmlElem19*)X);
		*L1 = *(longint*)(cr);
		*(longint*)cr = 0;
		cr = MyBP; PopProcStk(); ReleaseStore(cr);
		result = TWork.Read(1, *L1); TWork.Delete(*L1);
		break;
	}
	case _setmybp: {
		cr = MyBP; SetMyBP(ProcMyBP);
		result = RunLongStr(iX0->P1);
		SetMyBP((ProcStkD*)cr);
		break;
	}
	case _selectstr: result = RunSelectStr((FrmlElem0*)X); break;
	case _clipbd: result = TWork.Read(1, ClpBdPos); break;
	default: result = RunS(X);
	}
	return result;
}

pstring RunShortStr(FrmlPtr X)
{
	LongStr* s = RunLongStr(X);
	pstring result;
	if (s->LL > 255) result[0] = 255;
	else result[0] = s->LL;
	memcpy(&result[1], s->A, result[0]);
	delete s;
	return result;
}

void ConcatLongStr(LongStr* S1, LongStr* S2)
{
}

void CopyLongStr(LongStr* S, WORD From, WORD Number)
{
}

void AddToLongStr(LongStr* S, void* P, WORD L)
{
	void* p2;
	L = MinW(L, MaxLStrLen - S->LL);
	p2 = GetStore(L);
	Move(P, p2, L);
	S->LL += L;
}

void StrMask(double R, pstring& Mask)
{
	pstring Num; WORD i, l, n, sw, pos, pos1; bool minus;

	sw = 2; l = Mask.length(); n = 0; pos = l + 1; pos1 = pos;
	for (i = l; i > 1; i--)
		switch (Mask[i]) {
		case ',': if (sw == 2) sw = 1; break;
		case '0':
		case '*': { pos = i; goto label1; break; }
		case '_': {
			if (sw == 1) pos1 = i;
		label1:
			if (sw == 1) sw = 0; else if (sw == 2) n++; break; }
		}
	if (sw == 2) n = 0; R = R * Power10[n];
	R = RoundReal(R, 0);
	minus = false;
	if (R == 0) Num[0] = 0;
	else {
		if (R < 0) { minus = true; R = -R; } str(R, Num);
		pos = MinW(pos, pos1);
	}
	i = Num.length();
	if ((Num == "INF") || (Num == "NAN")) {
		Mask = Num;
		while ((Mask.length() < l))
		{
			pstring tmp = " ";
			Mask = tmp + Mask;
		}
		return;
	}
	while (l > 0) {
		switch (Mask[l]) {
		case '0':
		case '*': if (i > 0) goto label3; break;
		case '.':
		case ',': if ((i == 0) && (l < pos)) goto label2; break;
		case '-': if (minus) minus = false; else Mask[l] = ' '; break;
		case '_': {
			if (i == 0) {
				if (l >= pos) Mask[l] = '0';
				else {
				label2:
					if (minus) { minus = false; Mask[l] = '-'; }
					else { Mask[l] = ' '; }
				}

			label3:
				Mask[l] = Num[i]; i--;
			}
			break;
		}
		}
		l--;
	}
	if (i > 0) Mask = copy(Num, 1, i) + Mask;
	pstring tmp = "-";
	if (minus) Mask = tmp + Mask;
}

LongStr* RunS(FrmlElem* Z)
{
	wwmix ww;

	pstring s, snew; WORD w; FileDPtr cf; void* cr; XString* x = (XString*)&s;
	LongStr* t; LongStr* tnew; WORD l, i, j; double r; BYTE m;

	auto iZ0 = (FrmlElem0*)Z;

	switch (Z->Op) {
	case _char: { s[0] = 1; s[1] = char(trunc(RunReal(iZ0->P1))); break; }
	case _strdate1: {
		auto iZ = (FrmlElem6*)Z;
		s = StrDate(RunReal(iZ->PP1), iZ->Mask);
		break;
	}
	case _str: {
		if (iZ0->P3 != nullptr) {
			r = RunReal(iZ0->P1);
			l = RunInt(iZ0->P2);
			m = RunInt(iZ0->P3);
			if (m == 255) str(r, s);
			else str(m, s);
		}
		else { s = RunShortStr(iZ0->P2); StrMask(RunReal(iZ0->P1), s); }
		break;
	}
	case _replace: {
		auto iZ = (FrmlElem12*)Z;
		t = RunLongStr(iZ->PPP2);
		s = RunShortStr(iZ->PPPP1); j = 1;
		snew = RunShortStr(iZ->PP3);
		tnew = new LongStr(2); // GetZStore(2);
	label1:
		l = t->LL - (j - 1);
		if (l > 0) {
			i = FindTextE(s, iZ->Options, (char*)(&t->A[j]), l);
			if (i > 0) {
				AddToLongStr(tnew, &t->A[j], i - s.length() - 1);
				AddToLongStr(tnew, &snew[1], snew.length());
				j += i - 1;
				goto label1;
			}
		}
		AddToLongStr(tnew, &t->A[j], l); MyMove(tnew, t, tnew->LL + 2);
		ReleaseAfterLongStr(t);
		return t;
		break;
	}
	case _prompt:
	{
		auto iZ = (FrmlElem11*)Z;
		s = PromptS(&(RunShortStr(iZ->PPP1)), iZ->PP2, iZ->FldD);
		break;
	}
	case _getpath: {
		s = ".*";
		if (iZ0->P1 != nullptr) s = RunShortStr(iZ0->P1);
		s = ww.SelectDiskFile(s, 35, false);
		break;
	}
	case _catfield: {
		auto iZ = (FrmlElem10*)Z;
		s = RdCatField(iZ->CatIRec, iZ->CatFld);
		if (iZ->CatFld == CatPathName) s = FExpand(s);
		break;
	}
	case _password: s = ww.PassWord(false); break;
	case _readkey: {
		ReadKbd();
		s[1] = char(Lo(KbdChar));
		s[0] = 1;
		if (s[1] == 0) {
			/*asm  mov al, KbdChar[1].BYTE; cmp al, 0; jne @1; mov al, 03H; jmp @2;
			@cmp 1 al, 03H; jne @2; mov al, 84H;
			@mov 2 s[2].BYTE, al; mov s.BYTE, 2*/
		}
		break;
	}
	case _username: s = UserName; break;
	case _accright: s = AccRight; break;
	case _version: s = Version; break;
	case _edfield: s = EdField; break;
	case _edfile: {
		s[0] = 0;
		if (EditDRoot != nullptr) s = EditDRoot->FD->Name;
		break;
	}
	case _edkey: s = EdKey; break;
	case _edreckey: s = EdRecKey;;
	case _getenv: {
		s = RunShortStr(iZ0->P1);
		if (s == "") s = paramstr[0];
		else s = GetEnv(s.c_str());
		break;
	}
	case _keyof: {
		auto iZ = (FrmlElem20*)Z;
		cf = CFile;
		cr = CRecPtr;
		/* !!! with Z->LV^ do!!! */
		CFile = iZ->LV->FD; CRecPtr = iZ->LV->RecPtr;
		x->PackKF(iZ->PackKey->KFlds); CFile = cf; CRecPtr = cr;
		break;
	}
	case _keybuf: { while (KeyPressed()) AddToKbdBuf(ReadKey()); s = KbdBuffer; break; }
	case _recno: GetRecNoXString((FrmlElem13*)Z, *x); break;
	case _edbool: { s[0] = 0; if ((EditDRoot != nullptr) && EditDRoot->Select
		&& (EditDRoot->BoolTxt != nullptr)) s = *EditDRoot->BoolTxt;
		break;
	}
	}
	return CopyToLongStr(s);
}

LongStr* RunSelectStr(FrmlElem0* Z)
{
	wwmix ww;

	LongStr* s = nullptr; LongStr* s2 = nullptr;
	pstring x(80); pstring mode(5);
	void* p2 = nullptr; void* pl = nullptr;
	WORD i, n;

	s = RunLongStr(Z->P3); n = CountDLines(s->A, s->LL, Z->Delim);
	for (i = 1; i < n; i++) {
		x = GetDLine(s->A, s->LL, Z->Delim, i);
		if (x != "") ww.PutSelect(x);
	}
	mode = RunShortStr(Z->P6);
	for (i = 1; i < mode.length(); i++)
		switch (toupper(mode[i])) {
		case 'A': ss.Abcd = true; break;
		case 'S': ss.Subset = true; break;
		case 'I': ss.ImplAll = true; break;
		}
	SetMsgPar(RunShortStr(Z->P4));
	ww.SelectStr(RunInt(Z->P1), RunInt(Z->P2), 110, RunShortStr(Z->P5));
	MarkStore2(p2);
	s2 = new LongStr(s->LL + 2); // GetStore2(s->LL + 2);
	n = 1; LastExitCode = 0;
	if (KbdChar == _ESC_) LastExitCode = 1;
	else
		do {
			x = ww.GetSelect();
			if (x != "") {
				if (n > 1) { s2->A[n] = 0x0D; n++; }
				Move(&x[1], &s2->A[n], x.length());
				n += x.length();
			}
		} while (!(!ss.Subset || (x == "")));
		ReleaseStore(s);
		s = (LongStr*)GetStore(n + 1); s->LL = n - 1; Move(s2->A, s->A, n - 1);
		ReleaseStore2(p2);
		return s;
}

void LowCase(LongStr* S)
{
}

double RoundReal(double RR, integer M)
{
	double R;
	M = MaxI(0, MinI(M, 10));
	R = RR * Power10[M];
	if (R < 0) R = R - 0.50001;
	else R = R + 0.50001;
	return int(R) / Power10[M];
}

LongStr* LongLeadChar(char C, char CNew, LongStr* S)
{
	WORD i, l;
	i = 1; l = S->LL;
	while (i <= l) {
		if (S->A[i] != C) goto label1;
		if (CNew != 0) S->A[i] = CNew; i++;
	}
label1: if (CNew == 0) {
	l -= i - 1;
	S->LL = l;
	if ((i > 1) && (l > 0)) MyMove(&S->A[i], &S->A[1], l);
	ReleaseAfterLongStr(S);
}
return S;
}

LongStr* LongTrailChar(char C, char CNew, LongStr* S)
{
	WORD l;
	l = S->LL; while (l > 0) {
		if (S->A[l] != C) goto label1;
		if (CNew != 0) S->A[l] = CNew; l--;
	}
label1:
	if (CNew == 0) { S->LL = l; ReleaseAfterLongStr(S); }
	return S;
}

LongStr* RepeatStr(LongStr* S, integer N)
{
	WORD l; void* p;
	l = S->LL;
	if (l == 0) return S;
	if (N <= 0) { S->LL = 0; ReleaseAfterLongStr(S); return S; }
	while ((N > 1) && (longint(S->LL) + l <= MaxLStrLen))
	{
		p = GetStore(l);
		MyMove(S->A, p, l);
		S->LL += l; N--;
	}
	return S;
}

void AccRecNoProc(FrmlElem14* X, WORD Msg)
{
	longint N; LockMode md;
	CFile = X->RecFD;
	md = NewLMode(RdMode);
	CRecPtr = GetRecSpace();
	N = RunInt(X->PPPPP1);
	if ((N <= 0) || (N > CFile->NRecs))
	{
		Set2MsgPar(CFile->Name, X->RecFldD->Name); RunErrorM(md, Msg);
	}
	ReadRec(N);
	OldLMode(md);
}

void* RunUserFunc(FrmlElem19* X)
{
	FrmlList fl; LocVar* lv; void* oldbp; void* oldprocbp;
	oldbp = MyBP; oldprocbp = ProcMyBP; LVBD = X->FC->LVB; PushProcStk();
	lv = LVBD.Root; fl = X->FrmlL;
	while (fl != nullptr) {
		LVAssignFrml(lv, oldbp, false, fl->Frml);
		lv = (LocVar*)lv->Chain; fl = (FrmlListEl*)fl->Chain;
	}
	ProcMyBP = MyBP;
	RunProcedure(X->FC->Instr);
	auto result = LocVarAd(lv);
	ProcMyBP = (ProcStkD*)oldprocbp;
	return result;
}

void GetRecNoXString(FrmlElem13* Z, XString& X)
{
	WORD i = 0;
	X.Clear();
	KeyFldDPtr kf = Z->Key->KFlds;
	while (kf != nullptr) {
		i++;
		FrmlElem* zz = Z->Arg[i];
		switch (kf->FldD->FrmlTyp) {
		case 'S': X.StoreStr(RunShortStr(zz), kf); break;
		case 'R': X.StoreReal(RunReal(zz), kf); break;
		case 'B': X.StoreBool(RunBool(zz), kf); break;
		}
		kf = (KeyFldD*)kf->Chain;
	}
}




