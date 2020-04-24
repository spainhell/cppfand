#include "runfrml.h"
#include "common.h"
#include "fileacc.h"
#include "index.h"
#include "pstring.h"
#include "legacy.h"
#include "memory.h"
#include "rdrun.h"
#include "recacc.h"
#include <math.h>

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
		New(Scan, Init(CFile, K, nullptr, true)); Scan->ResetOwner(&x, nullptr);
	label1:
		Scan->GetRec; if (!Scan->eof) {
			if (RunBool(Bool)) {
				if (Sum == nullptr) r = r + 1;
				else r = r + RunReal(Sum);
			}
			goto label1;
		}
		Scan->Close; ReleaseStore(CRecPtr);
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
	_RunRealStr: ValDate = ValDate(RunShortStr(X->P1), X->Mask);
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
		result = int(CountDLines(S->A, S->LL, m));
		ReleaseStore(S);
		break;
	}
	case _ord: {
		S = RunLongStr(X->P1); N = 0; if (S->LL > 0) N = S->A[1];
		result = N; ReleaseStore(S);
		break;
	}
			 _RunRealStr prompt = PromptR(RunShortStr(X->P1), X->P2, X->FldD);
	case _pos: {
		S = RunLongStr(X->P2); Mask = RunShortStr(X->P1);
		N = 1;
		if (X->P3 != nullptr) N = RunInt(X->P3); J = 1;
	label1:
		L = S->LL + 1 - J; I = 0;
		if ((N > 0) && (L > 0)) {
			I = FindText(Mask, X->Options, CharArrPtr(@S->A[J]), L);
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
		result = DiskFree(ord(upcase(S->A[1])) - ord("@"));
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
	DateTime dt;
	UnpackTime(GetDateTimeH(Handle), dt);
	return RDate(dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec, 0);
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
	void* p;
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
		p = (void*)uintptr_t(MyBP) + z->BPOfs;
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
	j = FindCtrlM(S, i, M); l = j - i; if ((i > 1) && (l > 0)) move(S->A[i], S->A[1], l);
	S->LL = l;
	ReleaseAfterLongStr(S);
	return S;
}

double RunReal(FrmlPtr X)
{
	double R; FileD* cf; LockMode md; bool b;
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
		SetTxtPathVol(X->TxtPath, X->TxtCatIRec);
		result = GetFileSize();
		break;
	}
	case _inttsr: result = IntTSR(X);
	case _userfunc: { result = FloatPtr(RunUserFunc(X));
		cr = MyBP; PopProcStk; ReleaseStore(cr); break; }
	case _indexnrecs: result = X->WKey->NRecs(); break;
	case _owned: result = Owned(X->ownBool, X->ownSum, X->ownLD);
	case _color: result = AColors[minw(WORD(RunInt(X->P1)), 53)]; break;
	case _portin: result = PortIn(RunBool(X->P1), WORD(RunInt(X->P2))); break;
	case _setmybp: {
		cr = MyBP; SetMyBP(ProcMyBP);
		result = RunReal(X->P1); SetMyBP((ProcStkD*)cr);
		break;
	}
	default: result = resultStr(X); break;
	}
				 return result;
}


	double RoundReal(double RR, integer M)
	{
		double R;
		M = maxi(0, mini(M, 10));
		R = RR * Power10[M]; if (R < 0) R = R - 0.50001; else R = R + 0.50001;
		return int(R) / Power10[M];
	}

	LongStr* LongLeadChar(char C, char CNew, LongStr * S)
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

	LongStr* LongTrailChar(char C, char CNew, LongStr * S)
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

	LongStr* RepeatStr(LongStr * S, integer N)
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

	void GetRecNoXString(FrmlPtr Z, XString & X)
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




