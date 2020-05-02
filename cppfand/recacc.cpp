#include "recacc.h"


#include "common.h"
#include "index.h"
#include "kbdww.h"
#include "legacy.h"
#include "memory.h"
#include "runfrml.h"
#include "type.h"

void IncNRecs(longint N)
{
#ifdef FandDemo
	if (NRecs > 100) RunError(884);
#endif
	CFile->NRecs += N;
	SetUpdHandle(CFile->Handle);
	if (CFile->Typ == 'X') SetUpdHandle(CFile->XF->Handle);
}

void DecNRecs(longint N)
{
	/* !!! with CFile^ do!!! */
	CFile->NRecs -= N;
	SetUpdHandle(CFile->Handle);
	if (CFile->Typ == 'X') SetUpdHandle(CFile->XF->Handle);
	CFile->WasWrRec = true;
}

void SeekRec(longint N)
{
	CFile->IRec = N;
	if (CFile->XF == nullptr) CFile->Eof = N >= CFile->NRecs;
	else CFile->Eof = N >= CFile->XF->NRecs;
}

void PutRec()
{
	/* !!! with CFile^ do!!! */
	CFile->NRecs++;
	RdWrCache(false, CFile->Handle, CFile->NotCached(),
		longint(CFile->IRec) * CFile->RecLen + CFile->FrstDispl, CFile->RecLen, CRecPtr);
	CFile->IRec++; CFile->Eof = true;
}


void WriteRec(longint N)
{
	RdWrCache(false, CFile->Handle, CFile->NotCached(),
		(N - 1) * CFile->RecLen + CFile->FrstDispl, CFile->RecLen, CRecPtr);
	CFile->WasWrRec = true;
}

void CreateRec(longint N)
{
	IncNRecs(1);
	void* cr = CRecPtr;
	CRecPtr = GetRecSpace();
	for (longint i = CFile->NRecs - 1; i > N; i--) {
		ReadRec(i);
		WriteRec(i + 1);
	}
	ReleaseStore(CRecPtr);
	CRecPtr = cr;
	WriteRec(N);
}

void DeleteRec(longint N)
{
	DelAllDifTFlds(CRecPtr, nullptr);
	for (longint i = N; i < CFile->NRecs - 1; i++) {
		ReadRec(i + 1); WriteRec(i);
	}
	DecNRecs(1);
}

bool LinkLastRec(FileDPtr FD, longint& N, bool WithT)
{
	CFile = FD;
	CRecPtr = GetRecSpace();
	LockMode md = NewLMode(RdMode);
	auto result = true;
#ifdef FandSQL
	if (FD->IsSQLFile)
	{
		if (Strm1->SelectXRec(nullptr, nullptr, _equ, WithT)) N = 1; else goto label1;
	}
	else
#endif
	{
		N = CFile->NRecs;
		if (N == 0) {
		label1:
			ZeroAllFlds();
			result = false;
			N = 1;
		}
		else ReadRec(N);
	}
	OldLMode(md);
	return result;
}

void AsgnParFldFrml(FileD* FD, FieldDPtr F, FrmlPtr Z, bool Ad)
{
	void* p; longint N; LockMode md; bool b;
	FileDPtr cf = CFile; void* cr = CRecPtr; CFile = FD;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		CRecPtr = GetRecSpace; ZeroAllFlds; AssgnFrml(F, Z, true, Ad);
		Strm1->UpdateXFld(nullptr, nullptr, F); ClearRecSpace(CRecPtr)
	}
	else
#endif
	{
		md = NewLMode(WrMode);
		if (!LinkLastRec(CFile, N, true)) { IncNRecs(1); WriteRec(N); }
		AssgnFrml(F, Z, true, Ad); WriteRec(N); OldLMode(md);
	}
	ReleaseStore(CRecPtr); CFile = cf; CRecPtr = cr;
}

bool SearchKey(XString& XX, KeyDPtr Key, longint& NN)
{
	longint R = 0;
	XString x;

	auto bResult = false;
	longint L = 1;
	integer Result = _gt;
	NN = CFile->NRecs;
	longint N = NN;
	if (N == 0) return bResult;
	KeyFldDPtr KF = Key->KFlds;
	do {
		if (Result == _gt) R = N; else L = N + 1;
		N = (L + R) / 2; ReadRec(N); x.PackKF(KF);
		Result = CompStr(x.S, XX.S);
	} while (!((L >= R) || (Result == _equ)));
	if ((N == NN) && (Result == _lt)) NN++;
	else {
		if (Key->Duplic && (Result == _equ))
			while (N > 1) {
				N--; ReadRec(N); x.PackKF(KF);
				if (CompStr(x.S, XX.S) != _equ) {
					N++; ReadRec(N); goto label1;
				};
			}
	label1:  NN = N;
	}
	if ((Result == _equ) || Key->Intervaltest && (Result == _gt))
		bResult = true;
	return bResult;
}

bool LinkUpw(LinkDPtr LD, longint& N, bool WithT)
{
	KeyFldDPtr KF;
	FieldDPtr F, F2;
	bool LU; LockMode md;
	pstring s; double r; bool b;
	XString* x = (XString*)&s;

	FileDPtr ToFD = LD->ToFD; FileDPtr CF = CFile; void* CP = CRecPtr;
	KeyDPtr K = LD->ToKey; KeyFldDPtr Arg = LD->Args; x->PackKF(Arg);
	CFile = ToFD; void* RecPtr = GetRecSpace(); CRecPtr = RecPtr;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		LU = Strm1->SelectXRec(K, @X, _equ, WithT); N = 1; if (LU) goto label2; else goto label1;
	}
#endif
	md = NewLMode(RdMode);
	if (ToFD->Typ == 'X') { TestXFExist(); LU = K->SearchIntvl(*x, false, N); }
	else if (CFile->NRecs = 0) { LU = false; N = 1; }
	else LU = SearchKey(*x, K, N);
	if (LU) ReadRec(N);
	else {
	label1:
		ZeroAllFlds; KF = K->KFlds; while (Arg != nullptr) {
			F = Arg->FldD; F2 = KF->FldD; CFile = CF; CRecPtr = CP;
			if (F2->Flg && f_Stored != 0)
				switch (F->FrmlTyp) {
				case 'S': { s = _ShortS(F); CFile = ToFD; CRecPtr = RecPtr; S_(F2, s); break; }
				case 'R': { r = _R(F); CFile = ToFD; CRecPtr = RecPtr; R_(F2, r); break; }
				case 'B': { b = _B(F); CFile = ToFD; CRecPtr = RecPtr; B_(F2, b); break; }
				}
			Arg = Arg->Chain; KF = KF->Chain;
		}
		CFile = ToFD; CRecPtr = RecPtr;
	}
label2:
	auto result = LU;
#ifdef FandSQL
	if (!CFile->IsSQLFile)
#endif
		OldLMode(md);
	return result;
}

void AssignNRecs(bool Add, longint N)
{
	longint OldNRecs; LockMode md;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		if ((N = 0) && !Add) Strm1->DeleteXRec(nullptr, nullptr, false); return;
	}
#endif
	md = NewLMode(DelMode); OldNRecs = CFile->NRecs;
	if (Add) N = N + OldNRecs;
	if ((N < 0) || (N == OldNRecs)) goto label1;
	if ((N == 0) && (CFile->TF != nullptr)) CFile->TF->SetEmpty;
	if (CFile->Typ == 'X')
		if (N == 0) {
			CFile->NRecs = 0; SetUpdHandle(CFile->Handle); XFNotValid; goto label1;
		}
		else { SetMsgPar(CFile->Name); RunErrorM(md, 821); }
	if (N < OldNRecs) { DecNRecs(OldNRecs - N); goto label1; }
	CRecPtr = GetRecSpace(); ZeroAllFlds(); SetDeletedFlag();
	IncNRecs(N - OldNRecs); for (longint i = OldNRecs + 1; i < N; i++) WriteRec(i);
	ReleaseStore(CRecPtr);
label1:
	OldLMode(md);

}

pstring _ShortS(FieldDPtr F)
{
	void* P = nullptr; WORD* POfs = (WORD*)P; /*absolute P;*/
	pstring S;
	if (F->Flg && f_Stored != 0) {
		WORD l = F->L;
		S[0] = char(l);
		P = CRecPtr;
		POfs += F->Displ;
		switch (F->Typ) {
		case 'A':
		case 'N': {
			if (F->Typ == 'A') {
				Move(P, &S[1], l);
				if (F->Flg && f_Encryp != 0) Code(&S[1], l);
				if (IsNullValue(&S[2], l)) FillChar(&S[0], l, ' ');
			}
			else if (IsNullValue(P, F->NBytes)) FillChar(&S[0], l, ' ');
			else
			{
				// nebudeme volat, zøejmìní není potøeba
				// UnPack(P, (WORD*)S[0], l);
			}
			break;
		}
		case 'T': {
			LongStrPtr ss = _LongS(F);
			if (ss->LL > 255) S = S.substr(0, 255);
			else S = S.substr(0, ss->LL);
			Move(&ss[0], &S[0], S.length());
			ReleaseStore(ss);
			break; };
		default:;
		}
		return S;
	}
	return RunShortStr(F->Frml);
}

LongStr* _LongS(FieldDPtr F)
{
	void* P = nullptr;
	WORD* POfs = (WORD*)P;
	//LP ^longint absolute P;
	LongStrPtr S = nullptr; longint Pos; integer err; LockMode md; WORD l;
	{
		if (F->Flg && f_Stored != 0) {
			P = CRecPtr; POfs += F->Displ; l = F->L;
			switch (F->Typ)
			{
			case 'A': case 'N': {
				S = (LongStr*)GetStore(l + 2);
				S->LL = l;
				if (F->Typ == 'A') {
					Move(P, &S[0], l);
					if (F->Flg && f_Encryp != 0) Code(S->A, l);
					if (IsNullValue(S, l)) { S->LL = 0; ReleaseAfterLongStr(S); }
				}
				else if (IsNullValue(P, F->NBytes)) {
					S->LL = 0;
					ReleaseAfterLongStr(S);
				}
				else
				{
					// nebudeme volat, zøejmìní není potøeba
					// UnPack(P, S->A, l);
				}
				break;
			}
			case 'T': {
				if (HasTWorkFlag()) S = TWork.Read(1, _T(F));
				else {
					md = NewLMode(RdMode);
					S = CFile->TF->Read(1, _T(F));
					OldLMode(md);
				}
				if (F->Flg && f_Encryp != 0) Code(S->A, S->LL);
				if (IsNullValue(&S->A, S->LL))
				{
					S->LL = 0;
					ReleaseAfterLongStr(S);
				}
				break; }
			}
			return S;
		}
		return RunLongStr(F->Frml);
	};
}

double _RforD(FieldDPtr F, void* P)
{
	pstring s; integer err;
	double r = 0; s[0] = F->NBytes;
	Move(P, &s[1], s.length());
	switch (F->Typ) {
	case 'F': { ReplaceChar(s, ',', '.');
		if (F->Flg && f_Comma != 0) {
			integer i = s.first('.');
			if (i > 0) s.Delete(i, 1);
		}
		val(LeadChar(' ', TrailChar(' ', s)), r, err);
		break;
	}
	case 'D': r = ValDate(s, "YYYYMMDD"); break;
	}
	return r;
}

double _R(FieldDPtr F)
{
	double result = 0.0;
	void* p; double r;
	WORD* O = (WORD*)p;
	integer* IP = (integer*)p;

	if (F->Flg && f_Stored != 0) {
		p = CRecPtr; *O += F->Displ;
		if (CFile->Typ == 'D') result = _RforD(F, p);
		else switch (F->Typ) {
		case 'F': {
			r = RealFromFix(p, F->NBytes);
			if (F->Flg && f_Comma == 0) result = r / Power10[F->M]; else result = r; break;
		}
		case 'D': {
			if (CFile->Typ == '8') {
				if (*IP == 0) result = 0.0;
				else result = *IP + FirstDate;
			}
			else goto label1; break;
		}
		case 'R': {
		label1:
			if (IsNullValue(p, F->NBytes)) result = 0;
			else result = *(double*)p;
		}
		}
	}
	else return RunReal(F->Frml);
	return result;
}

bool _B(FieldDPtr F)
{
	bool result;
	void* p;
	WORD* O = (WORD*)p;
	unsigned char* CP = (unsigned char*)p;

	if (F->Flg && f_Stored != 0) {
		p = CRecPtr; O += F->Displ;
		if (CFile->Typ == 'D') result = *CP == 'Y' || *CP == 'y' || *CP == 'T' || *CP == 't';
		else if ((*CP == '\0') || (*CP == 0xff)) result = false;
		else result = true;
	}
	else result = RunBool(F->Frml);
	return result;
}

/// nechápu, co to dìlá - oøeže úvodní znaky, pøevádí na èíslo, ...
longint _T(FieldDescr* F)
{
	void* p; longint n; integer err;
	WORD* O = (WORD*)&p;

	p = CRecPtr;
	O += F->Displ;
	if (CFile->Typ == 'D')
	{
		n = 0;
		// tváøíme se, že CRecPtr je pstring ...
		pstring* s = (pstring*)CRecPtr;
		auto result = stoi(LeadChar(' ', *s));
		return result;
	}
	else
	{
		if (IsNullValue(p, 4)) return 0;
		longint* lip = (longint*)p;
		return *lip;
	}
}

void S_(FieldDPtr F, pstring S)
{
	void* p;
	WORD* O = (WORD*)p; double* RP = (double*)p;
	integer i, L, M; longint Pos; LongStrPtr ss;
	const BYTE LeftJust = 1;

	if (F->Flg && f_Stored != 0)
	{
		p = CRecPtr; O += F->Displ; L = F->L; M = F->M;
		switch (F->Typ) {
		case 'A': {
			while (S.length() < L)
				if (M == LeftJust) S = S + " ";
				else
				{
					pstring oldS = S;
					S = " ";
					S += oldS;
				}
			i = 1;
			if ((S.length() > L) && (M != LeftJust)) i = S.length() + 1 - L;
			Move(&S[i], p, L);
			if (F->Flg && f_Encryp != 0) Code(p, L);
			break;
		}
		case 'N': {
			while (S.length() < L)
				if (M == LeftJust) S = S + "0";
				else
				{
					pstring oldS = S;
					S = " ";
					S += oldS;
				}
			i = 1;
			if ((S.length() > L) && (M != LeftJust)) i = S.length() + 1 - L;
			//Pack(&S[i], p, L);
			break;
		}
		case 'T': {
			ss = CopyToLongStr(S);
			LongS_(F, ss);
			ReleaseStore(ss);
			break;
		}
		}
	}
}

void LongS_(FieldDPtr F, LongStr* S)
{
	longint Pos; LockMode md;

	if (F->Flg && f_Stored != 0) {
		if (S->LL == 0) T_(F, 0);
		else {
			if (F->Flg && f_Encryp != 0) Code(S->A, S->LL);
#ifdef FandSQL
			if (CFile->IsSQLFile) { SetTWorkFlag; goto label1; }
			else
#endif
				if (HasTWorkFlag())
					label1:
			Pos = TWork.Store(S);
				else {
					md = NewLMode(WrMode); Pos = CFile->TF->Store(S); OldLMode(md);
				}
			if (F->Flg && f_Encryp != 0) Code(S->A, S->LL); T_(F, Pos);
		}
	}
}

void R_(FieldDPtr F, double R)
{
	void* p; pstring s; WORD m; longint l;
	WORD* O = (WORD*)p;
	integer* IP = (integer*)p;

	if (F->Flg && f_Stored != 0) {
		p = CRecPtr; O += F->Displ; m = F->M;
		switch (F->Typ) {
		case 'F': {
			if (CFile->Typ == 'D') {
				if (F->Flg && f_Comma != 0) R = R / Power10[m];
				str(F->NBytes, s); Move(&s[1], p, F->NBytes);
			}
			else {
				if (F->Flg && f_Comma == 0) R = R * Power10[m];
				WORD tmp = F->NBytes;
				FixFromReal(R, p, tmp);
				F->NBytes = (BYTE)tmp;
			} }
		case 'D': {
			switch (CFile->Typ) {
			case '8': { if (trunc(R) == 0) *IP = 0; else *IP = trunc(R - FirstDate); break; }
			case 'D': { s = StrDate(R, "YYYYMMDD"); Move(&s[1], p, 8); break; }
			default: p = &R; break;
			}
		}
		case 'R': { p = &R; break; }
		}
	}
}

void B_(FieldDPtr F, bool B)
{
	void* p;
	WORD* O = (WORD*)p; bool* BP = (bool*)p; char* CP = (char*)p;
	if ((F->Typ == 'B') && (F->Flg && f_Stored != 0)) {
		p = CRecPtr; *O += F->Displ;
		if (CFile->Typ == 'D')
		{
			if (B) *CP = 'T'; else *CP = 'F';
		}
		else *BP = B;
	}
}

void T_(FieldDPtr F, longint Pos)
{
	void* p; pstring s;
	WORD* O = (WORD*)p; longint* LP = (longint*)p;
	if ((F->Typ == 'T') && (F->Flg && f_Stored != 0)) {
		p = CRecPtr; *O += F->Displ;
		if (CFile->Typ == 'D')
			if (Pos == 0) FillChar(p, 10, ' ');
			else { str(Pos, s); Move(&s[1], p, 10); }
		else *LP = Pos;
	}
	else RunError(906);
}

void ZeroAllFlds()
{
	FillChar(CRecPtr, CFile->RecLen, 0);
	FieldDPtr F = CFile->FldD; while (F != nullptr) {
		if ((F->Flg && f_Stored != 0) && (F->Typ == 'A')) S_(F, "");
		F = F->Chain;
	}
}

void DelTFld(FieldDPtr F)
{
	longint n; LockMode md;
	n = _T(F);
	if (HasTWorkFlag()) TWork.Delete(n);
	else {
		md = NewLMode(WrMode); CFile->TF->Delete(n); OldLMode(md);
	}
	T_(F, 0);
}

void DelDifTFld(void* Rec, void* CompRec, FieldDPtr F)
{
	longint n; void* cr;
	cr = CRecPtr; CRecPtr = CompRec; n = _T(F); CRecPtr = Rec;
	if (n != _T(F)) DelTFld(F); CRecPtr = cr;
}

void ClearRecSpace(void* p)
{
	FieldDPtr f; void* cr;
	if (CFile->TF != nullptr) {
		cr = CRecPtr; CRecPtr = p;
		if (HasTWorkFlag) {
			f = CFile->FldD; while (f != nullptr) {
				if ((f->Flg && f_Stored != 0) && (f->Typ == 'T')) {
					TWork.Delete(_T(f)); T_(f, 0);
				}
				f = f->Chain;
			}
		}
		CRecPtr = cr;
	}
}

void DelAllDifTFlds(void* Rec, void* CompRec)
{
	FieldDPtr F = CFile->FldD;
	while (F != nullptr)
	{
		if (F->Typ == 'T' && F->Flg && f_Stored != 0) DelDifTFld(Rec, CompRec, F);
		F = F->Chain;
	}
}

void DelTFlds()
{
	FieldDPtr F = CFile->FldD;
	while (F != nullptr) {
		if ((F->Flg && f_Stored != 0) && (F->Typ == 'T')) DelTFld(F); F = F->Chain;
	}
}

void CopyRecWithT(void* p1, void* p2)
{
	Move(p1, p2, CFile->RecLen);
	FieldDPtr F = CFile->FldD;
	while (F != nullptr) {
		if ((F->Typ == 'T') && (F->Flg && f_Stored != 0)) {
			TFilePtr tf1 = CFile->TF; TFilePtr tf2 = tf1; CRecPtr = p1;
			if ((tf1->Format != TFile::T00Format)) {
				LongStrPtr s = _LongS(F);
				CRecPtr = p2; LongS_(F, s);
				ReleaseStore(s);
			}
			else {
				if (HasTWorkFlag()) tf1 = &TWork;
				longint pos = _T(F);
				CRecPtr = p2;
				if (HasTWorkFlag()) tf2 = &TWork;
				pos = CopyTFString(tf2, CFile, tf1, pos);
				T_(F, pos);
			}
		}
		F = F->Chain;
	}
}

