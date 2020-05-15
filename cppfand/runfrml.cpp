#include "runfrml.h"
#include "pstring.h"
#include "legacy.h"
#include "rdrun.h"
#include <math.h>
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
	LongStr* s;
	l = SS.length();
	s = (LongStr*)GetStore(l + 2);
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

double RunRealStr(FrmlPtr X)
{
	WORD I, J, L, N; double R; LongStr* S; pstring Mask; BYTE b;
	double result;
	switch (X->Op) {
	case _valdate: result = ValDate(RunShortStr(X->P1), X->Mask); break;
	case _val: {
		val(LeadChar(' ', TrailChar(' ', RunShortStr(X->P1))), R, I);
		result = R;
		break;
	}
	case _length: {
		S = RunLongStr(X->P1); result = S->LL;
		ReleaseStore(S);
		break;
	}
	case _linecnt: {
		S = RunLongStr(X->P1);
		result = int(CountDLines(S->A, S->LL, 0x0D));
		ReleaseStore(S);
		break;
	}
	case _ord: {
		S = RunLongStr(X->P1); N = 0; if (S->LL > 0) N = S->A[1];
		result = N; ReleaseStore(S);
		break;
	}
	case _prompt: result = PromptR(&RunShortStr(X->P1), X->P2, X->FldD); break;
	case _pos: {
		S = RunLongStr(X->P2); Mask = RunShortStr(X->P1);
		N = 1;
		if (X->P3 != nullptr) N = RunInt(X->P3); J = 1;
	label1:
		L = S->LL + 1 - J; I = 0;
		if ((N > 0) && (L > 0)) {
			I = FindTextE(Mask, X->Options, CharArrPtr(S->A[J]), L);
			if (I > 0) {
				J = J + I - Mask.length(); N--;
				if (N > 0) goto label1; I = J - 1;
			}
		}
		ReleaseStore(S); result = I;
		break;
	}
	case _diskfree: {
		S = RunLongStr(X->P1);
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

double RMod(FrmlPtr X)
{
	double R1, R2;
	R1 = RunReal(X->P1); R2 = RunReal(X->P2);
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
		for (i = 1; i < NWDaysTab; i++)
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

longint RecNoFun(FrmlPtr Z)
{
	KeyDPtr k; FileDPtr cf; void* cr;
	bool b; longint n; LockMode md; XString x;
	GetRecNoXString(Z, x); cf = CFile; cr = CRecPtr; k = Z->Key;
	CFile = Z->FD; md = NewLMode(RdMode); CRecPtr = GetRecSpace;
	if (CFile->NRecs > 0) {
		if (CFile->Typ == 'X') {
			TestXFExist(); b = k->SearchIntvl(x, false, n);
		}
		else b = SearchKey(x, k, n);
		if (not b) n = -n;
	}
	else n = -1;
	OldLMode(md); ReleaseStore(CRecPtr); CFile = cf; CRecPtr = cr;
	return n;
}

longint AbsLogRecNoFun(FrmlPtr Z)
{
	longint result = 0;
	void* p = nullptr;
	FileDPtr cf = CFile;
	void* cr = CRecPtr;
	MarkStore(p);
	KeyDPtr k = Z->Key;
	longint N = RunInt(Z->Arg[1]);
	if (N <= 0) return result;
	CFile = Z->FD;
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

double LinkProc(FrmlPtr X)
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

WORD IntTSR(FrmlPtr X)
{
	BYTE IntNr; WORD FunNr; void* p; FrmlPtr z;
	pstring s; bool b; double r; LongStr* ss;
	IntNr = RunInt(X->P1); FunNr = RunInt(X->P2); z = X->P3;
	switch (X->N31) {
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
		p = (void*)(MyBP + z->BPOfs);
		switch (X->N31) {
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
	WORD i, j, l;
	i = 1; if (N > 1) { i = FindCtrlM(S, 1, N - 1); i = SkipCtrlMJ(S, i); }
	j = FindCtrlM(S, i, M); l = j - i; if ((i > 1) && (l > 0)) Move(&S->A[i], &S->A[1], l);
	S->LL = l;
	ReleaseAfterLongStr(S);
	return S;
}

bool RunBool(FrmlPtr X)
{
	longint RecNo; WORD* res = (WORD*)&RecNo;
	LongStr* S = nullptr; LongStr* S2 = nullptr; bool* b = (bool*)S;
	WORD* w1 = (WORD*)&RecNo; WORD* w2 = (WORD*)S;
	FileDPtr cf = nullptr; void* cr = nullptr; void* p = &RecNo;

	auto result = false;
	if (X == nullptr) { return true; }
	switch (X->Op) {
	case _and: if (RunBool(X->P1)) result = RunBool(X->P2); else result = false; break;
	case _or: if (RunBool(X->P1)) result = true; else result = RunBool(X->P2); break;
	case _lneg: result = !RunBool(X->P1); break;
	case _limpl: if (RunBool(X->P1)) result = RunBool(X->P2);
			   else result = true; break;
	case _lequ: if (RunBool(X->P1) == RunBool(X->P2)) result = true;
			  else result = false;
	case _instr: {
		S = RunLongStr(X->P1);
		if (X->N11 == 1) result = LexInStr(LongTrailChar(' ', 0, S), &X->N12);
		else result = InStr(S, &X->N12);
		ReleaseStore(S);
		break;
	}
	case _inreal: { result = InReal(RunReal(X->P1), &X->N12, X->N11); break; }
	case _compreal: {
		result = (CompReal(RunReal(X->P1), RunReal(X->P2), X->N22) && X->N21) != 0;
		break;
	}
	case _compstr: {
		S = RunLongStr(X->P1); S2 = RunLongStr(X->P2);
		if (X->N22 == 1)
			*res = CompLexLongStr(LongTrailChar(' ', 0, S), LongTrailChar(' ', 0, S2));
		else *res = CompLongStr(S, S2);
		result = res && X->N21 != 0; ReleaseStore(S); break; }
	case _const: result = X->B; break;
	case _mouseevent: {
	label2:
		Event.What = 0;
		GetMouseEvent();
		if (Event.What == 0) result = false;
		else { if ((Event.What && X->W01) == 0) goto label2; result = true; }
		break;
	}
	case _ismouse: {
		result = false;
		if (((Event.What && X->W01) != 0) && ((Event.Buttons && X->W02) == X->W02))
			result = true;
		break;
	}
	case _mousein: {
		*w1 = RunInt(X->P1); *w2 = RunInt(X->P2);
		result = MouseInRectProc(*w1, *w2, RunInt(X->P3) - *w1 + 1, RunInt(X->P4) - *w2 + 1);
		break;
	}
	case _getlocvar: result = (bool*)(&MyBP); break;
	case _modulo: result = RunModulo(X); break;
	case _field: result = _B(X->Field); break;
	case _access: { cf = CFile; cr = CRecPtr;
		if (X->LD != nullptr) *b = LinkUpw(X->LD, RecNo, false);
		else *b = LinkLastRec(X->File2, RecNo, false);
		if ((X->P1 = nullptr)) result = b;
		else result = RunBool(X->P1);
		ReleaseStore(CRecPtr); CFile = cf; CRecPtr = cr;
		break;
	}
	case _recvarfld: {
		cf = CFile; cr = CRecPtr;
		CFile = X->File2; CRecPtr = X->LD; result = RunBool(X->P1);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _eval: {
		MarkStore(p); result = RunBool(GetEvalFrml(X));
		ReleaseStore(p);
		break;
	}
	case _newfile: {
		cf = CFile; cr = CRecPtr;
		CFile = X->NewFile; CRecPtr = X->NewRP;
		result = RunBool(X->Frml);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _prompt: result = PromptB(&RunShortStr(X->P1), X->P2, X->FldD); break;
	case _promptyn: { SetMsgPar(RunShortStr(X->P1)); result = PromptYN(110); break; }
	case _accrecno: { cf = CFile; cr = CRecPtr; AccRecNoProc(X, 640);
		result = _B(X->RecFldD); ReleaseStore(CRecPtr);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _edupdated: result = EdUpdated; break;
	case _keypressed: result = KeyPressed(); break;/*Kbdpressed?*/
	case _escprompt: result = EscPrompt; break;
	case _isdeleted: {
		cr = CRecPtr; cf = CFile; AccRecNoProc(X, 642);
		result = DeletedFlag(); ReleaseStore(CRecPtr);
		CRecPtr = cr; CFile = cf;
		break;
	}
	case _lvdeleted: {
		cr = CRecPtr; cf = CFile; CRecPtr = X->LV->RecPtr; CFile = X->LV->FD;
		result = DeletedFlag(); CRecPtr = cr; CFile = cf;
		break;
	}
	case _trust: result = (UserCode == 0) || OverlapByteStr(&X->N01, &AccRight); break;
	case _isnewrec: result = TestIsNewRec(); break;
	case _testmode: result = IsTestRun; break;
	case _equmask: result = RunEquMask(X); break;
	case _userfunc: {
		result = *(bool*)(RunUserFunc(X));
		cr = MyBP; PopProcStk(); ReleaseStore(cr);
		break;
	}
	case _setmybp: {
		cr = MyBP; SetMyBP(ProcMyBP);
		result = RunBool(X->P1); SetMyBP((ProcStkD*)cr);
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

bool RunModulo(FrmlPtr X)
{
	pstring S; integer I, M, N; WORD* B1 = nullptr; WORD* B1Offs = B1;
	N = X->W11; S = RunShortStr(X->P1);
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

bool RunEquMask(FrmlPtr X)
{
	LongStr* s;
	s = RunLongStr(X->P1); auto result = EqualsMask(s->A, s->LL, RunShortStr(X->P2));
	ReleaseStore(s);
	return result;
}

double RunReal(FrmlPtr X)
{
	double R; FileD* cf = nullptr; LockMode md; bool b;
	longint RecNo; void* p = &RecNo; void* cr;
#ifdef FandGraph
	ViewPortType vp/*9 BYTE*/ absolute R;
#endif
	FILE* h = (FILE*)&R;
	WORD* n = (WORD*)cf;
	BYTE* AColors = (BYTE*)&colors;
	double result = 0;

	if (X == nullptr) return result;
label1:
	switch (X->Op) {
	case _field: result = _R(X->Field); break;
	case _getlocvar: result = *(double*)(uintptr_t(MyBP) + X->BPOfs); break;
	case _const: result = X->R; break;
	case _plus: { result = RunReal(X->P1) + RunReal(X->P2); break; }
	case _minus: { result = RunReal(X->P1) - RunReal(X->P2); break; }
	case _times: { result = RunReal(X->P1) * RunReal(X->P2); break; }
	case _access: {
		cf = CFile; cr = CRecPtr;
		if (X->LD != nullptr) LinkUpw(X->LD, RecNo, false);
		else LinkLastRec(X->File2, RecNo, false);
		result = RunReal(X->P1);
		ReleaseStore(CRecPtr);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _recvarfld: { cf = CFile; cr = CRecPtr; CFile = X->File2; CRecPtr = X->LD;
		result = RunReal(X->P1); CFile = cf; CRecPtr = cr; break; }
	case _eval: { MarkStore(p); result = RunReal(GetEvalFrml(X));	ReleaseStore(p); break; }
	case _divide: result = RunReal(X->P1) / RunReal(X->P2); break;
	case _cond: {
	label2:
		if (X->P1 != nullptr)
			if (!RunBool(X->P1))
			{
				if (X->P3 == nullptr) { result = 0; return result; }
				X = X->P3;
				goto label2;
			}
		X = X->P2;
		goto label1;
	}
	case _newfile: { cf = CFile; cr = CRecPtr;
		CFile = X->NewFile; CRecPtr = X->NewRP;
		result = RunReal(X->Frml);
		CFile = cf; CRecPtr = cr; break; }
	case _getWORDvar: result = int(WordVarArr[X->N01]); break;
	case _div: result = int(RunReal(X->P1) / RunReal(X->P2)); break;
	case _mod: result = RMod(X); break;
	case _unminus: result = -RunReal(X->P1); break;
	case _today: result = Today(); break;
	case _pi: result = std::atan(1.0) * 4; break;
	case _random: result = Random(); break;
	case _round: result = RoundReal(RunReal(X->P1), RunInt(X->P2)); break;
	case _abs: result = abs(RunReal(X->P1)); break;
	case _int: result = int(RunReal(X->P1)); break;
	case _frac: result = modf(RunReal(X->P1), nullptr); break;
	case _sqr: result = pow(RunReal(X->P1), 2); break;
	case _sqrt: result = sqrt(RunReal(X->P1)); break;
	case _sin: result = sin(RunReal(X->P1)); break;
	case _cos: result = cos(RunReal(X->P1)); break;
	case _arctan: result = atan(RunReal(X->P1)); break;
	case _ln: result = log(RunReal(X->P1));
	case _exp: { R = RunReal(X->P1);
		if ((R <= -50) || (R > 88)) result = 0; else result = exp(R); break; }
	case _nrecs:
	case _nrecsabs: {
		cf = CFile; CFile = X->FD; md = NewLMode(RdMode);
		if (X->Op == _nrecs) RecNo = XNRecs(CFile->Keys);
		else RecNo = CFile->NRecs;
		OldLMode(md); result = int(RecNo); CFile = cf;
		break; }
	case _generation: { cf = CFile; CFile = X->FD;
		result = int(Generation); CFile = cf; break; }
	case _lastupdate: { cf = CFile;
		CFile = X->FD; md = NewLMode(RdMode);
		result = LastUpdate(CFile->Handle); OldLMode(md); CFile = cf; break; }
	case _catfield: {
		RdCatPathVol(X->CatIRec);
		TestMountVol(CPath[1]);
		h = OpenH(_isoldfile, RdOnly);
		result = LastUpdate(h);
		CloseH(h);
		break;
	}
	case _currtime: result = CurrTime(); break;
	case _typeday: result = TypeDay(RunReal(X->P1)); break;
	case _addwdays: result = AddWDays(RunReal(X->P1), RunInt(X->P2), X->N21); break;
	case _difwdays: result = DifWDays(RunReal(X->P1), RunReal(X->P2), X->N21); break;
	case _addmonth: result = AddMonth(RunReal(X->P1), RunReal(X->P2)); break;
	case _difmonth: result = DifMonth(RunReal(X->P1), RunReal(X->P2)); break;
	case _recno: result = RecNoFun(X); break;
	case _recnoabs:
	case _recnolog: result = AbsLogRecNoFun(X); break;
	case _accrecno: {
		cf = CFile; cr = CRecPtr; AccRecNoProc(X, 640);
		result = _R(X->RecFldD);
		ReleaseStore(CRecPtr);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _link: result = LinkProc(X); break;
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
	case _mousex: if (IsGraphMode) result = Event.WhereG.X;
				else result = Event.Where.X + 1; break;
	case _mousey: if (IsGraphMode) result = Event.WhereG.Y;
				else result = Event.Where.Y + 1; break;
	case _filesize: {
		SetTxtPathVol(*X->TxtPath, X->TxtCatIRec);
		result = GetFileSize();
		break;
	}
	case _inttsr: result = IntTSR(X);
	case _userfunc: {
		result = *(double*)RunUserFunc(X);
		cr = MyBP; PopProcStk(); ReleaseStore(cr);
		break;
	}
	case _indexnrecs: result = X->WKey->NRecs(); break;
	case _owned: result = Owned(X->ownBool, X->ownSum, X->ownLD);
	case _color: result = AColors[MinW(WORD(RunInt(X->P1)), 53)]; break;
	case _portin: result = PortIn(RunBool(X->P1), WORD(RunInt(X->P2))); break;
	case _setmybp: {
		cr = MyBP; SetMyBP(ProcMyBP);
		result = RunReal(X->P1); SetMyBP((ProcStkD*)cr);
		break;
	}
	default: { result = RunRealStr(X); break; }
	}
	return result;
}

longint RunInt(FrmlPtr X)
{
	return trunc(RunReal(X));
}

void TestTFrml(FieldDPtr F, FrmlPtr Z)
{
	FileDPtr cf = nullptr; void* p = nullptr;
	longint n; LockMode md; FieldDPtr f1 = nullptr;
	switch (Z->Op) {
	case _newfile: {
		CFile = Z->NewFile; CRecPtr = Z->NewRP; TestTFrml(F, Z->Frml);
		break;
	}
	case _field: { f1 = Z->Field;
		if ((f1->Typ != 'T') || (f1->Flg && f_Stored == 0)) return;
		if (F = nullptr) { if (f1->Flg && f_Encryp != 0) return; }
		else if (F->Flg && f_Encryp != f1->Flg && f_Encryp) return;
		TFD02 = CFile; TF02 = CFile->TF;
		if (HasTWorkFlag()) TF02 = &TWork;
		TF02Pos = _T(f1);
		break;
	}
	case _getlocvar: {
		if ((F != nullptr) && (F->Flg && f_Encryp != 0)) return;
		TFD02 = CFile; TF02 = &TWork;
		TF02Pos = *(longint*)(MyBP + Z->BPOfs);
		break;
	}
	case _access: {
		cf = CFile; MarkStore(p); CFile = Z->File2;
		md = NewLMode(RdMode);
		if (Z->LD != nullptr) { CFile = cf; LinkUpw(Z->LD, n, true); }
		else LinkLastRec(Z->File2, n, true);
		TestTFrml(F, Z->P1);
		CFile = Z->File2; OldLMode(md); ReleaseStore(p);
		break;
	}
	case _recvarfld: {
		CFile = Z->File2; CRecPtr = Z->LD; TestTFrml(F, Z->P1);
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
	void* p; void* bp; LongStr* s; longint pos;
	p = LocVarAd(LV); bp = MyBP; SetMyBP((ProcStkD*)OldBP);
	switch (LV->FTyp) {
	case 'S': {
		if (not TryCopyT(nullptr, &TWork, pos, X)) {
			s = RunLongStr(X); pos = TWork.Store(s); ReleaseStore(s);
		}
		TWork.Delete(*(longint*)(p)); p = &pos;
		break;
	}
	case 'R': if (Add) *(double*)(p) = *(double*)(p) + RunReal(X);
			else *(double*)(p) = RunReal(X); break;
	case 'B': *(bool*)(p) = RunBool(X); break;
	}
	SetMyBP((ProcStkD*)bp);
}

void DecodeFieldRSB(FieldDPtr F, WORD LWw, double R, pstring T, bool B, pstring& Txt)
{
	WORD L, M; char C;
	L = F->L; M = F->M;
	switch (F->Typ) {
	case 'D':T = StrDate(R, *FieldDMask(F)); break;
	case 'N': { C = '0'; goto label1; break; }
	case 'A': { C = ' ';
	label1:
		if (M == LeftJust)
			while (T.length() < L) T = T + C;
		else while (T.length() < L) { pstring oldT = T; T = C; T += oldT; }
		break;
	}
	case 'B': if (B) T = AbbrYes; else T = AbbrNo; break;
	case 'R': str(R, L, T); break;
	default: /*"F"*/
		if (F->Flg && f_Comma != 0) R = R / Power10[M];
		str(RoundReal(R, M),L, M, T);
		break;
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
	double r; pstring s; bool b;
	switch (F->FrmlTyp) {
	case 'R': r = _R(F); break;
	case 'S': {
		if (F->Typ == 'T') {
			if ((F->Flg && f_Stored != 0) && (_R(F) == 0)) Txt = ".";
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

void RunWFrml(WRectFrml X, BYTE WFlags, WRect& W)
{
	WORD i;
	FrmlPtr XA = (FrmlPtr)&X;
	BYTE* WA = (BYTE*)&W;

	for (i = 0; i < 3; i++) WA[i] = RunInt(&XA[i]);
	CenterWw(W.C1, W.R1, W.C2, W.R2, WFlags);
}

WORD RunWordImpl(FrmlPtr Z, WORD Impl)
{
	WORD n;
	n = RunInt(Z); if (n == 0) n = Impl; return n;
}

bool FieldInList(FieldDPtr F, FieldListEl* FL)
{
	auto result = false;
	while (FL != nullptr) {
		if (FL->FldD == F) result = true;
		FL = FL->Chain;
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
	if ((Z != nullptr) && ((BYTE)Z->Op == _eval)) Z = GetEvalFrml(Z);
	return Z;
}

LongStr* RunLongStr(FrmlPtr X)
{
	LongStr* S; bool b;
	WORD I;
	LockMode* md = (LockMode*)&I;
	integer* J = (integer*)&I;
	longint RecNo;
	WORD* N = (WORD*)&RecNo; longint* L1 = (longint*)&RecNo;
	FileDPtr cf = nullptr; void* cr = nullptr; longint* L2 = (longint*)cr;
	void* p = &RecNo;

	LongStr* result = nullptr;

	if (X == nullptr) return (LongStr*)GetZStore(2);
label1:
	switch (X->Op) {
	case _field: result = _LongS(X->Field); break;
	case _getlocvar: result = TWork.Read(1, *(longint*)(MyBP + X->BPOfs));
	case _access: {
		cf = CFile; cr = CRecPtr;
		CFile = X->File2; *md = NewLMode(RdMode);
		if (X->LD != nullptr) { CFile = cf; LinkUpw(X->LD, RecNo, true); }
		else LinkLastRec(X->File2, RecNo, true);
		S = RunLongStr(X->P1);
		OldLMode(*md);  /*possibly reading .T*/
		ClearRecSpace(CRecPtr);
		MyMove(S, CRecPtr, S->LL + 2);
		ReleaseAfterLongStr(CRecPtr);
		result = (LongStr*)CRecPtr;
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _recvarfld: {
		cf = CFile; cr = CRecPtr;
		CFile = X->File2; CRecPtr = X->LD;
		result = RunLongStr(X->P1);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _eval: {
		MarkStore(p);
		S = RunLongStr(GetEvalFrml(X)); MyMove(S, p, S->LL + 2);
		ReleaseAfterLongStr(p); result = (LongStr*)p;
		break;
	}
	case _newfile: {
		cf = CFile; cr = CRecPtr;
		CFile = X->NewFile; CRecPtr = X->NewRP;
		result = RunLongStr(X->Frml);
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _cond: {
	label2:
		if (X->P1 != nullptr)
			if (!RunBool(X->P1))
			{
				if (X->P3 == nullptr)
				{
					S = (LongStr*)GetZStore(2);
					result = S; return result;
				}
				X = X->P3; goto label2;
			}
		X = X->P2; goto label1;
		break;
	}
	case _copy: {
		S = RunLongStr(X->P1);
		*L1 = RunInt(X->P2); *L2 = RunInt(X->P3);
		if ((L1 < 0) || (L2 < 0)) S->LL = 0; else CopyLongStr(S, (WORD)*L1, (WORD)*L2);
		ReleaseAfterLongStr(S);
		result = S;
		break;
	}
	case _concat: {
		S = RunLongStr(X->P1);
		ConcatLongStr(S, RunLongStr(X->P2));
		ReleaseAfterLongStr(S); result = S;
		break;
	}
	case _const: result = CopyToLongStr(X->S); break;
	case _leadchar: result = LongLeadChar(char(X->N11), char(X->N12), RunLongStr(X->P1)); break;
	case _trailchar: result = LongTrailChar(char(X->N11), char(X->N12), RunLongStr(X->P1)); break;
	case _upcase: {
		S = RunLongStr(X->P1);
		for (WORD i = 1; i < S->LL; i++) S->A[i] = UpcCharTab[S->A[i]];
		result = S;
		break;
	}
	case _lowcase: {
		S = RunLongStr(X->P1); LowCase(S); result = S;
		break;
	}
	case _copyline: {
		*J = 1; if (X->P3 != nullptr) *J = RunInt(X->P3);
		result = CopyLine(RunLongStr(X->P1), RunInt(X->P2), *J);
		break;
	}
	case _repeatstr: result = RepeatStr(RunLongStr(X->P1), RunInt(X->P2)); break;
	case _accrecno: {
		cf = CFile; cr = CRecPtr; AccRecNoProc(X, 640);
		S = _LongS(X->RecFldD);
		MyMove(S, CRecPtr, S->LL + 2);
		ReleaseAfterLongStr(CRecPtr);
		result = (LongStr*)CRecPtr;
		CFile = cf; CRecPtr = cr;
		break;
	}
	case _gettxt: result = GetTxt(X); break;
	case _nodiakr: {
		S = RunLongStr(X->P1); ConvToNoDiakr((WORD*)S->A[0], S->LL, fonts.VFont);
		result = S;
		break;
	}
	case _userfunc: {
		cr = RunUserFunc(X);
		*L1 = *(longint*)(cr);
		*(longint*)cr = 0;
		cr = MyBP; PopProcStk(); ReleaseStore(cr);
		result = TWork.Read(1, *L1); TWork.Delete(*L1);
		break;
	}
	case _setmybp: {
		cr = MyBP; SetMyBP(ProcMyBP);
		result = RunLongStr(X->P1);
		SetMyBP((ProcStkD*)cr);
		break;
	}
	case _selectstr: result = RunSelectStr(X); break;
	case _clipbd: result = TWork.Read(1, ClpBdPos); break;
	default: result = RunS(X);
	}
	return result;
}

pstring RunShortStr(FrmlPtr X)
{
	LongStr* s;
	s = RunLongStr(X);
	/* ASM */
	ReleaseStore(s);
	return "";
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

LongStr* RunS(FrmlPtr Z)
{
	wwmix ww;
	
	pstring s, snew; WORD w; FileDPtr cf; void* cr; XString* x = (XString*)&s;
	LongStr* t; LongStr* tnew; WORD l, i, j; double r; BYTE m;

	switch (Z->Op) {
	case _char: { s[0] = 1; s[1] = char(trunc(RunReal(Z->P1))); break; }
	case _strdate: s = StrDate(RunReal(Z->P1), Z->Mask);
	case _str: {
		if (Z->P3 != nullptr) {
			r = RunReal(Z->P1);
			l = RunInt(Z->P2);
			m = RunInt(Z->P3);
			if (m == 255) str(r, s); else str(m, s);
		}
		else { s = RunShortStr(Z->P2); StrMask(RunReal(Z->P1), s); }
		break;
	}
	case _replace: {
		t = RunLongStr(Z->P2); s = RunShortStr(Z->P1); j = 1;
		snew = RunShortStr(Z->P3); tnew = (LongStr*)GetZStore(2);
	label1:
		l = t->LL - (j - 1);
		if (l > 0) {
			i = FindTextE(s, Z->Options, (CharArr*)(t->A[j]), l);
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
	case _prompt:s = PromptS(&(RunShortStr(Z->P1)), Z->P2, Z->FldD);
	case _getpath: { s = ".*"; if (Z->P1 != nullptr) s = RunShortStr(Z->P1);
		s = ww.SelectDiskFile(s, 35, false); }
	case _catfield: {
		s = RdCatField(Z->CatIRec, Z->CatFld);
		if (Z->CatFld == CatPathName) s = FExpand(s);
		break;
	}
	case _passWORD: s = ww.PassWord(false); break;
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
		s = RunShortStr(Z->P1);
		if (s == "") s = paramstr[0];
		else s = GetEnv(s.c_str());
		break;
	}
	case _keyof: {
		cf = CFile;
		cr = CRecPtr;
		/* !!! with Z->LV^ do!!! */
		CFile = Z->LV->FD; CRecPtr = Z->LV->RecPtr;
		x->PackKF(Z->PackKey->KFlds); CFile = cf; CRecPtr = cr;
		break;
	}
	case _keybuf: { while (KeyPressed()) AddToKbdBuf(ReadKey()); s = KbdBuffer; break; }
	case _recno: GetRecNoXString(Z, *x); break;
	case _edbool: { s[0] = 0; if ((EditDRoot != nullptr) && EditDRoot->Select
		&& (EditDRoot->BoolTxt != nullptr)) s = *EditDRoot->BoolTxt;
		break;
	}
	}
	return CopyToLongStr(s);
}

LongStr* RunSelectStr(FrmlPtr Z)
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
	s2 = (LongStr*)GetStore2(s->LL + 2); n = 1; LastExitCode = 0;
	if (KbdChar = _ESC_) LastExitCode = 1;
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

void AccRecNoProc(FrmlPtr X, WORD Msg)
{
	longint N; LockMode md;
	CFile = X->RecFD;
	md = NewLMode(RdMode);
	CRecPtr = GetRecSpace();
	N = RunInt(X->P1);
	if ((N <= 0) || (N > CFile->NRecs))
	{
		Set2MsgPar(CFile->Name, X->RecFldD->Name); RunErrorM(md, Msg);
	}
	ReadRec(N);
	OldLMode(md);
}

void* RunUserFunc(FrmlPtr X)
{
	FrmlList fl; LocVar* lv; void* oldbp; void* oldprocbp;
	oldbp = MyBP; oldprocbp = ProcMyBP; LVBD = X->FC->LVB; PushProcStk();
	lv = LVBD.Root; fl = X->FrmlL;
	while (fl != nullptr) {
		LVAssignFrml(lv, oldbp, false, fl->Frml); lv = lv->Chain; fl = fl->Chain;
	}
	ProcMyBP = MyBP;
	RunProcedure(X->FC->Instr);
	auto result = LocVarAd(lv);
	ProcMyBP = (ProcStkD*)oldprocbp;
	return result;
}

void GetRecNoXString(FrmlPtr Z, XString& X)
{
	WORD i = 0;
	X.Clear();
	KeyFldDPtr kf = Z->Key->KFlds;
	while (kf != nullptr) {
		i++;
		FrmlPtr zz = Z->Arg[i];
		switch (kf->FldD->FrmlTyp) {
		case 'S': X.StoreStr(RunShortStr(zz), kf); break;
		case 'R': X.StoreReal(RunReal(zz), kf); break;
		case 'B': X.StoreBool(RunBool(zz), kf); break;
		}
		kf = kf->Chain;
	}
}




