#pragma once

#include "access.h"

#include "globconf.h"
#include "kbdww.h"
#include "legacy.h"
#include "oaccess.h"
#include "obaseww.h"
#include "olongstr.h"
#include "rdfrml1.h"
#include "sort.h"

globconf* gcfg1 = globconf::GetInstance();

integer CompLongStr(LongStrPtr S1, LongStrPtr S2)
{
	return 0;
}

integer CompLongShortStr(LongStrPtr S1, pstring S2)
{
	return 0;
}

integer CompArea(void* A, void* B, integer L)
{
	return 0;
}

#ifdef FandNetV
void ModeLockBnds(LockMode Mode, longint& Pos, WORD& Len)
{
	longint n = 0;
	switch (Mode) {       /* hi=how much BYTEs, low= first BYTE */
	case NoExclMode: n = 0x00010000 + LANNode; break;
	case NoDelMode: n = 0x00010100 + LANNode; break;
	case NoCrMode: n = 0x00010200 + LANNode; break;
	case RdMode: n = 0x00010300 + LANNode; break;
	case WrMode: n = 0x00FF0300; break;
	case CrMode: n = 0x01FF0200; break;
	case DelMode: n = 0x02FF0100; break;
	case ExclMode: n = 0x03FF0000; break;
}
	Pos = ModeLock + (n >> 16);
	Len = n & 0xFFFF;
}

bool ChangeLMode(LockMode Mode, WORD Kind, bool RdPref)
{
	FILE* h;
	longint pos, oldpos; WORD len, oldlen, count, d; longint w, w1; LockMode oldmode;
	bool result = false;
	if (!CFile->IsShared()) {         /*neu!!*/
		result = true; CFile->LMode = Mode; return result;
	}
	result = false; oldmode = CFile->LMode; h = CFile->Handle;
	if (oldmode >= WrMode) {
		if (Mode < WrMode) WrPrefixes();
		if (oldmode == ExclMode) { SaveCache(0); ClearCacheCFile(); }
		if (Mode < WrMode) ResetCFileUpdH();
	}
	w = 0; count = 0;
label1:
	if (Mode != NullMode)
		if (!TryLockH(h, TransLock, 1)) {
		label2:
			if (Kind == 2) return result; /*0 Kind-wait, 1-wait until ESC, 2-no wait*/
			count++;
			if (count <= spec.LockRetries) d = spec.LockDelay;
			else {
				d = spec.NetDelay; SetCPathVol();
				Set2MsgPar(CPath, LockModeTxt[Mode]);
				w1 = PushWrLLMsg(825, Kind = 1);
				if (w == 0) w = w1; else TWork.Delete(w1); LockBeep();
			}
			if (KbdTimer(spec.NetDelay, Kind)) goto label1;
			if (w != 0) PopW(w); return result;
		}
	if (oldmode != NullMode) {
		ModeLockBnds(oldmode, oldpos, oldlen); UnLockH(h, oldpos, oldlen);
	}
	if (Mode != NullMode) {
		ModeLockBnds(Mode, pos, len); if (not TryLockH(h, pos, len)) {
			if (oldmode != NullMode) TryLockH(h, oldpos, oldlen);
			UnLockH(h, TransLock, 1); goto label2;
		}
		UnLockH(h, TransLock, 1);
	}
	if (w != 0) PopW(w);
	CFile->LMode = Mode;
	if ((oldmode < RdMode) && (Mode >= RdMode) && RdPref) RdPrefixes();
	result = true;
	return result;
}
#else
bool ChangeLMode(LockMode Mode, WORD Kind, bool RdPref)
{
	CFile->LMode = Mode;
	return true;
}
#endif


void OldLMode(LockMode Mode)
{
	/* !!! with CFile^ do!!! */
#ifdef FandSQL
	if (CFile->IsSQLFile) { CFile->LMode = Mode; return; }
#endif
	if (CFile->Handle == nullptr) return;
	if (Mode != CFile->LMode) ChangeLMode(Mode, 0, true);
}

void RunErrorM(LockMode Md, WORD N)
{
	OldLMode(Md);
	RunError(N);
}

void CloseClearHCFile()
{
	/* !!! with CFile^ do!!! */
	CloseClearH(CFile->Handle);
	if (CFile->Typ == 'X') CloseClearH(CFile->XF->Handle);
	if (CFile->TF != nullptr) CloseClearH(CFile->TF->Handle);
}

void CloseGoExit()
{
	CloseClearHCFile(); GoExit();
}

void TFile::Err(WORD n, bool ex)
{
	if (IsWork) {
		SetMsgPar(gcfg1->FandWorkTName); WrLLF10Msg(n); if (ex) GoExit();
	}
	else { CFileMsg(n, 'T'); if (ex) CloseGoExit(); }
}

void TFile::TestErr()
{
	if (gcfg1->HandleError != 0) Err(700 + gcfg1->HandleError, true);
}

longint TFile::UsedFileSize()
{
	if (Format == FptFormat) return FreePart * BlockSize;
	else return longint(MaxPage + 1) << MPageShft;
}

bool TFile::NotCached()
{
	return !IsWork && CFile->NotCached();
}

BYTE ByteMask[_MAX_INT_DIG];

const BYTE DblS = 8;
const BYTE FixS = 8;
BYTE Fix[FixS];
BYTE RealMask[DblS + 1];
BYTE Dbl[DblS];

void UnPack(void* PackArr, void* NumArr, WORD NoDigits)
{
}

void Pack(void* NumArr, void* PackArr, WORD NoDigits)
{
	BYTE* source = (BYTE*)NumArr;
	BYTE* target = (BYTE*)PackArr;
	WORD i;
	for (i = 1; i < (NoDigits >> 1); i++)
		target[i] = ((source[(i << 1) - 1] & 0x0F) << 4) || (source[i << 1] & 0x0F);
	if (NoDigits % 2 == 1)
		target[(NoDigits >> 1) + 1] = (source[NoDigits] & 0x0F) << 4;
}

double RealFromFix(void* FixNo, WORD FLen)
{
	double r;
	BYTE* rr = (BYTE*)&r;
	BYTE ff[FixS];
	integer i;

	FillChar(rr, DblS, 0);
	Move(FixNo, ff, FLen);
	bool neg = (ff[1] & 0x80) != 0;
	if (neg) {
		if (ff[1] == 0x80) {
			for (i = 2; i < FLen; i++) if (ff[i] != 0x00) goto label1;   /*NULL value*/
			return 0.;
		}
	label1:
		for (i = 1; i < FLen; i++) ff[i] = !(ff[i]);
		ff[FLen]++;
		i = FLen;
		while (ff[i] == 0) { i--; if (i > 0) ff[i]++; }
	}
	integer first = 1;
	while (ff[first] == 0) first++;
	if (first > FLen) { return 0; }
	integer lef = 0;
	BYTE b = ff[first];
	while ((b & 0x80) == 0) { b = b << 1; lef++; }
	ff[first] = ff[first] && (0x7F >> lef);
	integer exp = ((FLen - first) << 3) - lef + 1030;
	if (lef == 7) first++;
	lef = (lef + 5) & 0x07;
	integer rig = 8 - lef;
	i = DblS - 1;
	if ((rig <= 4) && (first <= FLen)) { rr[i] = ff[first] >> rig; i--; }
	while ((i > 0) && (first < FLen))
	{
		rr[i] = (ff[first] << lef) + (ff[first + 1] >> rig);
		i--;
		first++;
	}
	if ((first == FLen) && (i > 0)) rr[i] = ff[first] << lef;
	rr[DblS - 1] = (rr[DblS - 1] & 0x0F) + ((exp & 0x0F) << 4);
	rr[DblS] = exp >> 4;
	if (neg) rr[DblS] = rr[DblS] | 0x80;
	return r;
}

void FixFromReal(double r, void* FixNo, WORD& flen)
{
	BYTE* rr = (BYTE*)&r;
	BYTE* ff = (BYTE*)FixNo;

	FillChar(ff, flen, 0);
	if (r > 0) r = r + 0.5;
	else r = r - 0.5;
	bool neg = bool(rr[DblS] & 0x80);
	integer exp = (rr[DblS - 1] >> 4) + (WORD(rr[DblS] & 0x7F) << 4);
	if (exp < 2047)
	{
		rr[DblS] = 0; rr[DblS - 1] = rr[DblS - 1] & 0x0F;
		if (exp > 0) { rr[DblS - 1] = rr[DblS - 1] | 0x10; }
		else { exp++; }
		exp -= 1023;
		if (exp > (flen << 3) - 1) /*overflow*/ return;
		integer lef = (exp + 4) & 0x0007;
		integer rig = 8 - lef;
		if ((exp & 0x0007) > 3) exp += 4;
		integer first = 7 - (exp >> 3);
		integer i = flen;
		while ((first < DblS) and (i > 0))
		{
			ff[i] = (rr[first] >> rig) + (rr[first + 1] << lef);
			i--; first++;
		}
		if (i > 0) ff[i] = rr[first] >> rig;
		if (neg)
		{
			for (i = 1; i < flen; i++) ff[i] = !ff[i]; ff[flen]++;
			i = flen;
			while (ff[i] == 0) {
				i--;
				if (i > 0) ff[i]++;
			}
		}
	}
}

#ifdef FandNetV
const longint TransLock = 0x0A000501;  /* locked while state transition */
const longint ModeLock = 0x0A000000;  /* base for mode locking */
const longint RecLock = 0x0B000000;  /* base for record locking */
#endif

struct TT1Page
{
	WORD Signum = 0, OldMaxPage = 0;
	longint FreePart = 0;
	bool Rsrvd1 = false, CompileProc = false, CompileAll = false;
	WORD IRec = 0;
	longint FreeRoot = 0, MaxPage = 0;   /*eldest version=>array Pw[1..40] of char;*/
	double TimeStmp = 0.0;
	bool HasCoproc = false;
	char Rsrvd2[25];
	char Version[4];
	BYTE LicText[105];
	BYTE Sum = 0;
	char X1[295];
	WORD LicNr = 0;
	char X2[11];
	char PwNew[40];
	BYTE Time = 0;
};

void ResetCFileUpdH()
{
	/* !!! with CFile^ do!!! */
	ResetUpdHandle(CFile->Handle);
	if (CFile->Typ == 'X') ResetUpdHandle(CFile->XF->Handle);
	if (CFile->TF != nullptr) ResetUpdHandle(CFile->TF->Handle);
}

void ClearCacheCFile()
{
	/* !!! with CFile^ do!!! */
	ClearCacheH(CFile->Handle);
	if (CFile->Typ == 'X') ClearCacheH(CFile->XF->Handle);
	if (CFile->TF != nullptr) ClearCacheH(CFile->TF->Handle);
}

bool TryLMode(LockMode Mode, LockMode& OldMode, WORD Kind)
{
	/* !!! with CFile^ do!!! */
	auto result = true;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		OldMode = CFile->LMode; if (Mode > CFile->LMode) CFile->LMode = Mode;
	}
	else
#endif
	{
		if (CFile->Handle == nullptr) OpenCreateF(Shared);
		OldMode = CFile->LMode;
		if (Mode > CFile->LMode) result = ChangeLMode(Mode, Kind, true);
	}
	return result;
}

LockMode NewLMode(LockMode Mode)
{
	LockMode md;
	TryLMode(Mode, md, 0);
	return md;
}

bool TryLockN(longint N, WORD Kind)
{
	longint w, w1; WORD m;
	pstring XTxt(3); XTxt = "CrX";
	auto result = true;
#ifdef FandSQL
	if (CFile->IsSQLFile) return result;
#endif
#ifdef FandNetV

	if (!CFile->IsShared) return result; w = 0;
label1:
	if (!TryLockH(CFile->Handle, RecLock + N, 1)) {
		if (Kind != 2) {   /*0 Kind-wait, 1-wait until ESC, 2-no wait*/
			m = 826;
			if (N == 0) { SetCPathVol(); Set2MsgPar(CPath, XTxt); m = 825; }
			w1 = PushWrLLMsg(m, Kind = 1);
			if (w == 0) w = w1;
			else TWork.Delete(w1);
			/*beep; don't disturb*/
			if (KbdTimer(spec.NetDelay, Kind)) goto label1;
		}
		result = false;
	}
	if (w != 0) PopW(w);
#endif
	return result;
}

void UnLockN(longint N)
{
	/* !!! with CFile^ do!!! */
#ifdef FandSQL
	if (CFile->IsSQLFile) return;
#endif
#ifdef FandNetV

	if ((CFile->Handle == nullptr) || !CFile->IsShared) return;
	UnLockH(CFile->Handle, RecLock + N, 1);
#endif
}


WORD RdPrefix()
{
	struct x6 { longint NRs = 0; WORD RLen = 0; } X6;
	struct x8 { WORD NRs = 0, RLen = 0; } X8;
	struct xD {
		BYTE Ver = 0; BYTE Date[3] = { 0,0,0 };
		longint NRecs = 0;
		WORD HdLen = 0; WORD RecLen = 0;
	} XD;
	auto result = 0xffff;
	/* !!! with CFile^ do!!! */
	switch (CFile->Typ) {
	case '8': {
		RdWrCache(true, CFile->Handle, CFile->NotCached(), 0, 4, &X8);
		CFile->NRecs = X8.NRs;
		if (CFile->RecLen != X8.RLen) { return X8.RLen; }
		break;
	}
	case 'D': {
		RdWrCache(true, CFile->Handle, CFile->NotCached(), 0, 12, &XD);
		CFile->NRecs = XD.NRecs;
		if ((CFile->RecLen != XD.RecLen)) { return XD.RecLen; }
		CFile->FrstDispl = XD.HdLen;
		break;
	}
	default: {
		RdWrCache(true, CFile->Handle, CFile->NotCached(), 0, 6, &X6);
		CFile->NRecs = abs(X6.NRs);
		if ((X6.NRs < 0) && (CFile->Typ != 'X') || (X6.NRs > 0) && (CFile->Typ == 'X')
			|| (CFile->RecLen != X6.RLen)) {
			return X6.RLen;
		}
		break;
	}
	}
	return result;
}

void RdPrefixes()
{
	if (RdPrefix() != 0xffff) CFileError(883);
	/* !!! with CFile^ do!!! */ {
		if ((CFile->XF != nullptr) && (CFile->XF->Handle != nullptr)) CFile->XF->RdPrefix();
		if ((CFile->TF != nullptr)) CFile->TF->RdPrefix(false); }
}

void WrDBaseHd()
{
	DBaseHd* P = nullptr;

	FieldDPtr F;
	WORD n, y, m, d, w;
	pstring s;

	const char CtrlZ = '\x1a';

	P = (DBaseHd*)GetZStore(CFile->FrstDispl);
	char* PA = (char*)&P; // PA:CharArrPtr absolute P;
	F = CFile->FldD;
	n = 0;
	while (F != nullptr) {
		if (F->Flg && f_Stored != 0) {
			n++;
			{ // with P^.Flds[n]
				auto actual = P->Flds[n];
				switch (F->Typ) {
				case 'F': { actual.Typ = 'N'; actual.Dec = F->M; break; }
				case 'N': {actual.Typ = 'N'; break; }
				case 'A': {actual.Typ = 'C'; break; }
				case 'D': {actual.Typ = 'D'; break; }
				case 'B': {actual.Typ = 'L'; break; }
				case 'T': {actual.Typ = 'M'; break; }
				default:;
				}
				actual.Len = F->NBytes;
				actual.Displ = F->Displ;
				s = F->Name;
				for (size_t i = 1; i < s.length(); i++) s[i] = toupper(s[i]);
				StrLPCopy((char*)&actual.Name[1], s, 11);
			}
		}
		F = F->Chain;
	}

	{ //with P^ do 
		if (CFile->TF != nullptr) {
			if (CFile->TF->Format == TFile::FptFormat) P->Ver = 0xf5;
			else P->Ver = 0x83;
		}
		else P->Ver = 0x03;

		P->RecLen = CFile->RecLen;
		SplitDate(Today(), d, m, y);
		P->Date[1] = BYTE(y - 1900);
		P->Date[2] = (BYTE)m;
		P->Date[3] = (BYTE)d;
		P->NRecs = CFile->NRecs;
		P->HdLen = CFile->FrstDispl;
		PA[(P->HdLen / 32) * 32 + 1] = m;
	}

	// with CFile^
	{
		RdWrCache(false, CFile->Handle, CFile->NotCached(), 0, CFile->FrstDispl, (void*)&P);
		RdWrCache(false, CFile->Handle, CFile->NotCached(),
			longint(CFile->NRecs) * CFile->RecLen + CFile->FrstDispl, 1, (void*)&CtrlZ);
	}

	ReleaseStore(P);
}

void WrPrefix()
{
	struct
	{
		longint NRs;
		WORD RLen;
	} Pfx6 = { 0, 0 };

	struct
	{
		WORD NRs;
		WORD RLen;
	} Pfx8 = { 0, 0 };

	if (IsUpdHandle(CFile->Handle))
	{
		switch (CFile->Typ)
		{
		case '8': {
			Pfx8.RLen = CFile->RecLen;
			Pfx8.NRs = CFile->NRecs;
			RdWrCache(false, CFile->Handle, CFile->NotCached(), 0, 4, (void*)&Pfx8);
			break;
		}
		case 'D': {
			WrDBaseHd(); break;
		}
		default: {
			Pfx6.RLen = CFile->RecLen;
			if (CFile->Typ == 'X') Pfx6.NRs = -CFile->NRecs;
			else Pfx6.NRs = CFile->NRecs;
			RdWrCache(false, CFile->Handle, CFile->NotCached(), 0, 6, (void*)&Pfx6);
		}
		}
	}
}

void WrPrefixes()
{
	WrPrefix(); /*with CFile^ do begin*/
	if (CFile->TF != nullptr && IsUpdHandle(CFile->TF->Handle))
		CFile->TF->WrPrefix();
	if (CFile->Typ == 'X' && (CFile->XF)->Handle != nullptr
		&& /*{ call from CopyDuplF }*/ (IsUpdHandle(CFile->XF->Handle) || IsUpdHandle(CFile->Handle)))
		CFile->XF->WrPrefix();
}

void CExtToX()
{
	gcfg1->CExt[2] = 'X'; gcfg1->CPath = gcfg1->CDir + gcfg1->CName + gcfg1->CExt;
}

void TestCFileError()
{
	if (gcfg1->HandleError != 0) CFileError(700 + gcfg1->HandleError);
}

void TestCPathError()
{
	WORD n;
	if (gcfg1->HandleError != 0) {
		n = 700 + gcfg1->HandleError;
		if ((n == 705) && (gcfg1->CPath[gcfg1->CPath.length()] == '\\')) n = 840;
		SetMsgPar(gcfg1->CPath); RunError(n);
	}
}

void CExtToT()
{
	if (SEquUpcase(gcfg1->CExt, ".RDB"))
		gcfg1->CExt = ".TTT";
	else
		if (SEquUpcase(gcfg1->CExt, ".DBF"))
			if (CFile->TF->Format == TFile::FptFormat) gcfg1->CExt = ".FPT";
			else gcfg1->CExt = ".DBT";
		else gcfg1->CExt[2] = 'T';
	gcfg1->CPath = gcfg1->CDir + gcfg1->CName + gcfg1->CExt;
}

void XFNotValid()
{
	XFile* XF;
	XF = CFile->XF;
	if (XF == nullptr) return;
	if (XF->Handle == nullptr) RunError(903);
	XF->SetNotValid();
}

void NegateESDI()
{
	// asm  jcxz @2; @1:not es:[di].byte; inc di; loop @1; @2:
}

void TestXFExist()
{
	XFile* xf = CFile->XF;
	if ((xf != nullptr) && xf->NotValid)
	{
		if (xf->NoCreate) CFileError(819);
		CreateIndexFile();
	}
}

longint XNRecs(KeyDPtr K)
{
	if ((CFile->Typ == 'X') && (K != nullptr))
	{
		TestXFExist();
		return CFile->XF->NRecs;
	}
	return CFile->NRecs;
}

void ReadRec(longint N) {}

void WriteRec(longint N)
{
	RdWrCache(false, CFile->Handle, CFile->NotCached(),
		(N - 1) * CFile->RecLen + CFile->FrstDispl, CFile->RecLen, CRecPtr);
	CFile->WasWrRec = true;
}

void RecallRec(longint RecNr)
{
	TestXFExist();
	CFile->XF->NRecs++;
	KeyDPtr K = CFile->Keys;
	while (K != nullptr) { K->Insert(RecNr, false); K = K->Chain; }
	ClearDeletedFlag();
	WriteRec(RecNr);
}

void TryInsertAllIndexes(longint RecNr)
{
	void* p = nullptr;
	TestXFExist();
	MarkStore(p);
	KeyDPtr K = CFile->Keys;
	while (K != nullptr) {
		if (not K->Insert(RecNr, true)) goto label1; K = K->Chain;
	}
	CFile->XF->NRecs++;
	return;
label1:
	ReleaseStore(p);
	KeyDPtr K1 = CFile->Keys;
	while ((K1 != nullptr) && (K1 != K)) {
		K1->Delete(RecNr); K1 = K1->Chain;
	}
	SetDeletedFlag();
	WriteRec(RecNr);
	/* !!! with CFile->XF^ do!!! */
	if (CFile->XF->FirstDupl) {
		SetMsgPar(CFile->Name);
		WrLLF10Msg(828);
		CFile->XF->FirstDupl = false;
	}
}

void DeleteAllIndexes(longint RecNr)
{
	KeyDPtr K;
	K = CFile->Keys;
	while (K != nullptr) {
		K->Delete(RecNr);
		K = K->Chain;
	}
}

bool IsNullValue(void* p, WORD l)
{
	return false;
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

void T_(FieldDPtr F, longint Pos)
{
	void* p = nullptr; pstring s;
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

void DelAllDifTFlds(void* Rec, void* CompRec)
{
	FieldDPtr F = CFile->FldD;
	while (F != nullptr)
	{
		if (F->Typ == 'T' && F->Flg && f_Stored != 0) DelDifTFld(Rec, CompRec, F);
		F = F->Chain;
	}
}

void DeleteXRec(longint RecNr, bool DelT)
{
	TestXFExist();
	DeleteAllIndexes(RecNr);
	if (DelT) DelAllDifTFlds(CRecPtr, nullptr);
	SetDeletedFlag();
	WriteRec(RecNr);
	CFile->XF->NRecs--;
}

void OverWrXRec(longint RecNr, void* P2, void* P)
{
}

void OverwrXRec(longint RecNr, void* P2, void* P)
{
	XString x, x2; KeyDPtr K;
	CRecPtr = P2;
	if (DeletedFlag()) { CRecPtr = P; RecallRec(RecNr); return; }
	TestXFExist();
	K = CFile->Keys;
	while (K != nullptr) {
		CRecPtr = P; x.PackKF(K->KFlds); CRecPtr = P2; x2.PackKF(K->KFlds);
		if (x.S != x2.S) {
			K->Delete(RecNr); CRecPtr = P; K->Insert(RecNr, false);
		}
		K = K->Chain;
	}
	CRecPtr = P;
	WriteRec(RecNr);
}

void AddFFs(KeyDPtr K, pstring& s)
{
	WORD l = MinW(K->IndexLen + 1, 255);
	for (WORD i = s.length() + 1; i < l; i++) s[i] = 0xff;
	s[0] = char(l);
}

void CompKIFrml(KeyDPtr K, KeyInD* KI, bool AddFF)
{
	XString x; bool b; integer i;
	while (KI != nullptr) {
		b = x.PackFrml(KI->FL1, K->KFlds);
		KI->X1 = &x.S;
		if (KI->FL2 != nullptr) x.PackFrml(KI->FL2, K->KFlds);
		if (AddFF) AddFFs(K, x.S);
		KI->X2 = &x.S;
		KI = KI->Chain;
	}
}

const WORD Alloc = 2048;
const double FirstDate = 6.97248E+5;

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
					md = NewLMode(WrMode);
					Pos = CFile->TF->Store(S);
					OldLMode(md);
				}
			if (F->Flg && f_Encryp != 0) Code(S->A, S->LL); T_(F, Pos);
		}
	}
}

void S_(FieldDPtr F, pstring S)
{
	void* p = nullptr;
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

void ZeroAllFlds()
{
	FillChar(CRecPtr, CFile->RecLen, 0);
	FieldDPtr F = CFile->FldD; while (F != nullptr) {
		if ((F->Flg && f_Stored != 0) && (F->Typ == 'A')) S_(F, "");
		F = F->Chain;
	}
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
	}
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
	void* p = nullptr; double r;
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
	void* p = nullptr;
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

void R_(FieldDPtr F, double R)
{
	void* p = nullptr; pstring s; WORD m; longint l;
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
	void* p = nullptr;
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
	if ((N == 0) && (CFile->TF != nullptr)) CFile->TF->SetEmpty();
	if (CFile->Typ == 'X')
		if (N == 0) {
			CFile->NRecs = 0; SetUpdHandle(CFile->Handle); XFNotValid(); goto label1;
		}
		else { SetMsgPar(CFile->Name); RunErrorM(md, 821); }
	if (N < OldNRecs) { DecNRecs(OldNRecs - N); goto label1; }
	CRecPtr = GetRecSpace(); ZeroAllFlds(); SetDeletedFlag();
	IncNRecs(N - OldNRecs); for (longint i = OldNRecs + 1; i < N; i++) WriteRec(i);
	ReleaseStore(CRecPtr);
label1:
	OldLMode(md);

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

/// nedìlá nic, pùvodnì dìlal XOR 0xAA;
void Code(void* A, WORD L)
{
	return;
}

void TFile::RdPrefix(bool Chk)
{
	TT1Page T;
	BYTE* TX = (BYTE*)&T;
	longint* TNxtAvailPage = (longint*)&T; /* .DBT */
	struct stFptHd { longint FreePart = 0; WORD X = 0, BlockSize = 0; }; /* .FPT */
	stFptHd* FptHd = (stFptHd*)&T;
	BYTE sum; longint FS, ML, RS = 0; WORD i, n;
	if (Chk) {
		FS = FileSizeH(Handle);
		if (FS <= 512) {
			FillChar(PwCode, 40, '@');
			Code(PwCode, 40);
			SetEmpty();
			return;
		}
	}
	RdWrCache(true, Handle, NotCached(), 0, 512, &T); srand(RS); LicenseNr = 0;
	if (Format == DbtFormat) {
		MaxPage = *TNxtAvailPage - 1; GetMLen(); return;
	}
	if (Format == FptFormat) {
		FreePart = SwapLong((*FptHd).FreePart);
		BlockSize = Swap((*FptHd).BlockSize); return;
	}
	Move(&T.FreePart, &FreePart, 23);
	if (!IsWork && (CFile == Chpt) && ((T.HasCoproc != HasCoproc) ||
		(CompArea((char*)Version[1], &T.Version, 4) != _equ))) CompileAll = true;
	if (T.OldMaxPage == 0xffff) goto label1;
	else {
		FreeRoot = 0;
		if (FreePart > 0) {
			if (!Chk) FS = FileSizeH(Handle); ML = FS;
			MaxPage = (FS - 1) >> MPageShft; GetMLen();
		}
		else {
			FreePart = -FreePart; MaxPage = T.OldMaxPage;
		label1:
			GetMLen(); ML = MLen; if (!Chk) FS = ML;
		}
	}
	if (IRec >= 0x6000) {
		IRec = IRec - 0x2000;
		if (!IsWork && (CFile->Typ == '0')) LicenseNr = T.LicNr;
	}
	if (IRec >= 0x4000) {
		IRec = IRec - 0x4000;
		srand(ML + T.Time);
		for (i = 14; i < 511; i++) TX[i] = TX[i] xor Random(255);
		Move(T.PwNew, PwCode, 40);
	}
	else {
		srand(ML); for (i = 14; i < 53; i++) TX[i] = TX[i] xor Random(255);
		Move(&T.FreeRoot/*Pw*/, PwCode, 40);
	}
	Code(PwCode, 40);
	if ((FreePart < MPageSize) || (FreePart > ML) || (FS < ML) ||
		(FreeRoot > MaxPage) || (MaxPage == 0)) {
		Err(893, false); MaxPage = (FS - 1) >> MPageShft; FreeRoot = 0; GetMLen();
		FreePart = NewPage(true); SetUpdHandle(Handle);
	}
	FillChar(&T, 512, 0); srand(RS);
}

void TFile::WrPrefix()
{
	TT1Page T;
	BYTE* TX = (BYTE*)&T;
	longint* TNxtAvailPage = (longint*)&T;                               /* .DBT */
	struct stFptHd { longint FreePart; WORD X, BlockSize; }; /* .FPT */
	stFptHd* FptHd = (stFptHd*)&T;
	char Pw[40];
	// BYTE absolute 0 Time:0x46C; TODO: TIMER
	WORD i, n; BYTE sum; longint RS = 0;
	const PwCodeArr EmptyPw = { '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@' };

	if (Format == DbtFormat) {
		FillChar(&T, 512, ' '); *TNxtAvailPage = MaxPage + 1; goto label1;
	}
	if (Format == FptFormat) {
		FillChar(&T, 512, 0); (*FptHd).FreePart = SwapLong(FreePart);
		(*FptHd).BlockSize = Swap(BlockSize); goto label1;
	}
	FillChar(&T, 512, '@');
	Move(PwCode, Pw, 40); Code(Pw, 40); srand(RS);
	if (LicenseNr != 0) for (i = 1; i < 20; i++) Pw[i] = char(Random(255));
	n = 0x4000;
	// TODO: T.Time = Time;
	Move(Pw, T.PwNew, 40);
	srand(MLen + T.Time);
	for (i = 14; i < 511; i++) TX[i] = TX[i] xor Random(255);
	T.LicNr = LicenseNr;
	if (LicenseNr != 0) {
		n = 0x6000; sum = T.LicNr;
		for (i = 1; i < 105; i++) sum = sum + T.LicText[i];
		T.Sum = sum;
	}
	Move(&FreePart, &T.FreePart, 23);
	T.OldMaxPage = 0xffff; T.Signum = 1; T.IRec += n;
	Move((char*)Version[1], T.Version, 4); T.HasCoproc = HasCoproc;
	srand(RS);
label1:
	RdWrCache(false, Handle, NotCached(), 0, 512, &T);
}

void TFile::SetEmpty()
{
	BYTE X[MPageSize];
	integer* XL = (integer*)&X;
	if (Format == DbtFormat) { MaxPage = 0; WrPrefix(); return; }
	if (Format == FptFormat) { FreePart = 8; BlockSize = 64; WrPrefix(); return; }
	FreeRoot = 0; MaxPage = 1; FreePart = MPageSize; MLen = 2 * MPageSize;
	WrPrefix();
	FillChar(X, MPageSize, 0); *XL = -510;
	RdWrCache(false, Handle, NotCached(), MPageSize, MPageSize, X);
}

void TFile::Create()
{
	Handle = OpenH(_isoverwritefile, Exclusive); TestErr();
	IRec = 1; LicenseNr = 0; FillChar(PwCode, 40, '@'); Code(PwCode, 40);
	SetEmpty();
}

longint TFile::NewPage(bool NegL)
{
	longint PosPg;
	BYTE X[MPageSize];
	longint* L = (longint*)&X;
	if (FreeRoot != 0) {
		PosPg = FreeRoot << MPageShft;
		RdWrCache(true, Handle, NotCached(), PosPg, 4, &FreeRoot);
		if (FreeRoot > MaxPage) {
			Err(888, false);
			FreeRoot = 0; goto label1;
		}
	}
	else {
	label1:
		MaxPage++; MLen += MPageSize; PosPg = MaxPage << MPageShft;
	}
	FillChar(X, MPageSize, 0); if (NegL) *L = -510;
	RdWrCache(false, Handle, NotCached(), PosPg, MPageSize, X);
	return PosPg;
}

void TFile::ReleasePage(longint PosPg)
{
	BYTE X[MPageSize - 1];
	longint* Next = (longint*)&X;
	FillChar(X, MPageSize, 0); *Next = FreeRoot;
	RdWrCache(false, Handle, NotCached(), PosPg, MPageSize, X);
	FreeRoot = PosPg >> MPageShft;
}

void TFile::Delete(longint Pos)
{
	longint PosPg, NxtPg; WORD PosI; integer N; WORD l;
	BYTE X[MPageSize]; integer* XL = (integer*)&X;
	WORD* wp = nullptr; WORD* wpofs = wp; bool IsLongTxt;
	if (Pos <= 0) return;
	if ((Format != T00Format) || NotCached()) return;
	if ((Pos < MPageSize) || (Pos >= MLen)) { Err(889, false); return; }
	PosPg = Pos & (0xFFFFFFFF << MPageShft); PosI = Pos & (MPageSize - 1);
	RdWrCache(true, Handle, NotCached(), PosPg, MPageSize, X);
	wp = (WORD*)(&X[PosI]); l = *wp;
	if (l <= MPageSize - 2) {       /* small text on 1 page*/
		*wp = -integer(l); N = 0; wp = (WORD*)(&X);
		while (N < MPageSize - 2) {
			if (*wp > 0) { FillChar(&X[PosI + 2], l, 0); goto label1; }
			N += -(*wp) + 2; *wpofs += -(*wp) + 2;
		}
		if ((FreePart >= PosPg) && (FreePart < PosPg + MPageSize)) {
			FillChar(X, MPageSize, 0); *XL = -510; FreePart = PosPg;
		label1:
			RdWrCache(false, Handle, NotCached(), PosPg, MPageSize, X);
		}
		else ReleasePage(PosPg);
	}
	else {                        /* long text on more than 1 page */
		if (PosI != 0) goto label3;
	label2:
		l = WORD(XL); if (l > MaxLStrLen + 1) {
		label3:
			Err(889, false); return;
		}
		IsLongTxt = (l = MaxLStrLen + 1); l += 2;
	label4:
		ReleasePage(PosPg);
		if ((l > MPageSize) || IsLongTxt) {
			PosPg = *(longint*)(&X[MPageSize - 4]);
			if ((PosPg < MPageSize) || (PosPg + MPageSize > MLen)) {
				Err(888, false); return;
			}
			RdWrCache(true, Handle, NotCached(), PosPg, MPageSize, X);
			if ((l <= MPageSize)) goto label2; l -= MPageSize - 4; goto label4;
		}
	}
}

LongStr* TFile::Read(WORD StackNr, longint Pos)
{
	LongStr* s = nullptr; WORD i = 0, l = 0; CharArr* p = nullptr;
	WORD* pofs = (WORD*)p;
	struct stFptD { longint Typ = 0, Len = 0; } FptD;
	Pos -= LicenseNr;
	if (Pos <= 0 /*OldTxt=-1 in RDB!*/) goto label11;
	else switch (Format) {
	case DbtFormat: {
		s = (LongStr*)GetStore(32770); Pos = Pos << MPageShft; p = &s->A; l = 0;
		while (l <= 32768 - MPageSize) {
			RdWrCache(true, Handle, NotCached(), Pos, MPageSize, p);
			for (i = 1; i < MPageSize; i++) { if ((*p)[i] == 0x1A) goto label0; l++; }
			pofs += MPageSize; Pos += MPageSize;
		}
		l--;
	label0:
		s->LL = l; ReleaseStore(&s->A[l + 1]);
		break;
	}
	case FptFormat: {
		Pos = Pos * BlockSize;
		RdWrCache(true, Handle, NotCached(), Pos, sizeof(FptD), &FptD);
		if (SwapLong(FptD.Typ) != 1/*text*/) goto label11;
		else {
			l = SwapLong(FptD.Len) & 0x7FFF; s = (LongStr*)GetStore(l + 2); s->LL = l;
			RdWrCache(true, Handle, NotCached(), Pos + sizeof(FptD), l, s->A);
		}
		break;
	}
	default:
		if ((Pos < MPageSize) || (Pos >= MLen)) goto label1;
		RdWrCache(true, Handle, NotCached(), Pos, 2, &l);
		if (l > MaxLStrLen + 1) {
		label1:
			Err(891, false);
		label11:
			if (StackNr == 1) s = (LongStr*)GetStore(2);
			else s = (LongStr*)GetStore2(2); s->LL = 0;
			goto label2;
		}
		if (l == MaxLStrLen + 1) l--;
		if (StackNr == 1) s = (LongStr*)GetStore(l + 2);
		else s = (LongStr*)GetStore2(l + 2); s->LL = l;
		RdWr(true, Pos + 2, l, s->A);
		break;
	}
label2:
	return s;
}

longint TFile::Store(LongStrPtr S)
{
	integer rest; WORD l, M; longint N; void* p; longint pos;
	char X[MPageSize + 1];
	struct stFptD { longint Typ = 0, Len = 0; } FptD;
	longint result = 0;
	l = S->LL; if (l == 0) { return result; }
	if (Format == DbtFormat) {
		pos = MaxPage + 1; N = pos << MPageShft; if (l > 0x7fff) l = 0x7fff;
		RdWrCache(false, Handle, NotCached(), N, l, S->A);
		FillChar(X, MPageSize, ' '); X[0] = 0x1A; X[1] = 0x1A;
		rest = MPageSize - (l + 2) % MPageSize;
		RdWrCache(false, Handle, NotCached(), N + l, rest + 2, X);
		MaxPage += (l + 2 + rest) / MPageSize;
		goto label1;
	}
	if (Format == FptFormat) {
		pos = FreePart; N = FreePart * BlockSize;
		if (l > 0x7fff) l = 0x7fff;
		FreePart = FreePart + (sizeof(FptD) + l - 1) / BlockSize + 1;
		FptD.Typ = SwapLong(1); FptD.Len = SwapLong(l);
		RdWrCache(false, Handle, NotCached(), N, sizeof(FptD), &FptD);
		N += sizeof(FptD);
		RdWrCache(false, Handle, NotCached(), N, l, S->A);
		N += l;
		l = FreePart * BlockSize - N;
		if (l > 0) {
			p = GetStore(l); FillChar(p, l, ' ');
			RdWrCache(false, Handle, NotCached(), N, l, p); ReleaseStore(p);
		}
		goto label1;
	}
	if (l > MaxLStrLen) l = MaxLStrLen;
	if (l > MPageSize - 2) pos = NewPage(false);  /* long text */
	else {                                  /* short text */
		rest = MPageSize - FreePart % MPageSize;
		if (l + 2 <= rest) pos = FreePart;
		else { pos = NewPage(false); FreePart = pos; rest = MPageSize; }
		if (l + 4 >= rest) FreePart = NewPage(false);
		else {
			FreePart += l + 2; rest = l + 4 - rest;
			RdWrCache(false, Handle, NotCached(), FreePart, 2, &rest);
		}
	}
	RdWrCache(false, Handle, NotCached(), pos, 2, &l);
	RdWr(false, pos + 2, l, S->A);
label1:
	return pos;
}

void TFile::RdWr(bool ReadOp, longint Pos, WORD N, void* X)
{
	WORD Rest, L; longint NxtPg;
	void* P = nullptr;
	WORD* POfs = (WORD*)P;
	Rest = MPageSize - (WORD(Pos) && (MPageSize - 1)); P = X;
	while (N > Rest) {
		L = Rest - 4;
		RdWrCache(ReadOp, Handle, NotCached(), Pos, L, P);
		*POfs += L; N -= L;
		if (!ReadOp) NxtPg = NewPage(false);
		RdWrCache(ReadOp, Handle, NotCached(), Pos + L, 4, &NxtPg);
		Pos = NxtPg;
		if (ReadOp && ((Pos < MPageSize) || (Pos + MPageSize > MLen))) {
			Err(890, false); FillChar(P, N, ' '); return;
		}
		Rest = MPageSize;
	}
	RdWrCache(ReadOp, Handle, NotCached(), Pos, N, P);
}

void TFile::GetMLen()
{
	MLen = (MaxPage + 1) << MPageShft;
}

FileD::FileD()
{
}

longint FileD::UsedFileSize()
{
	longint n;
	n = longint(NRecs) * RecLen + FrstDispl;
	if (Typ == 'D') n++;
	return n;
}

bool FileD::IsShared()
{
	return (UMode = Shared) || (UMode = RdShared);
}

bool FileD::NotCached()
{
	/*asm  les di,Self; xor ax,ax; mov dl,es:[di].FileD.UMode;
     cmp dl,Shared; je @1; cmp dl,RdShared; jne @2;
@1:  cmp es:[di].FileD.LMode,ExclMode; je @2;
     mov ax,1;
@2:  end;*/
	return false;
}

WORD FileD::GetNrKeys()
{
	KeyD* k; WORD n;
	n = 0; k = Keys;
	while (k != nullptr) { n++; k = k->Chain; }
	return n;
}


void XString::Clear()
{
	this->S.clean();
}

void XString::StoreReal(double R, KeyFldD* KF)
{
	BYTE A[20];
	const BYTE TabF[18] = { 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8 };
	auto X = KF->FldD;

	if (X->Typ == 'R' || X->Typ == 'D') {
		bool b = KF->Descend;
		if (R < 0) { b = !b; R = -R; }
		StoreD(&R, b);
		return;
	}
	if (X->Flg && f_Comma == 0) R = R * Power10[X->M];
	WORD n = X->L - 1;
	if (X->M > 0) n--;
	n = TabF[n];
	FixFromReal(R, A, n);
	StoreF(A, n, KF->Descend);
}

void XString::StoreStr(pstring V, KeyFldD* KF)
{
	WORD n;
	auto X = KF->FldD;
	while (V[0] < X->L) {
		if (X->M == LeftJust) V = V + " ";
		else {
			auto oldV = V;
			V = " ";
			V += oldV;
		}
	}
	if (X->Typ == 'N') {
		// Pack(&V[1], &V[0], X->L);
		n = (X->L + 1) / 2;
		StoreN(&V, n, KF->Descend);
	}
	else StoreA(&V[1], X->L, KF->CompLex, KF->Descend);
}

void XString::StoreBool(bool B, KeyFldD* KF)
{
	StoreN(&B, 1, KF->Descend);
}

void XString::StoreKF(KeyFldD* KF)
{
	FieldDPtr F;
	F = KF->FldD;
	switch (F->FrmlTyp) {
	case 'S': StoreStr(_ShortS(F), KF); break;
	case 'R': StoreReal(_R(F), KF); break;
	case 'B': StoreBool(_B(F), KF); break;
	}
}

void XString::PackKF(KeyFldD* KF)
{
	Clear();
	while (KF != nullptr) { StoreKF(KF); KF = KF->Chain; }
}

bool XString::PackFrml(FrmlList FL, KeyFldD* KF)
{
	FrmlPtr Z;
	Clear();
	while (FL != nullptr) {
		Z = FL->Frml;
		switch (KF->FldD->FrmlTyp) {
		case 'S':StoreStr(RunShortStr(Z), KF); break;
		case 'R':StoreReal(RunReal(Z), KF); break;
		case 'B':StoreBool(RunBool(Z), KF); break;
		}
		KF = KF->Chain; FL = FL->Chain;
	}
	return KF != nullptr;
}

void XString::StoreD(void* R, bool Descend)
{
}

void XString::StoreN(void* N, WORD Len, bool Descend)
{
}

void XString::StoreF(void* F, WORD Len, bool Descend)
{
}

void XString::StoreA(void* A, WORD Len, bool CompLex, bool Descend)
{
}

longint XItem::GetN()
{
	// asm les bx,Self; mov ax,es:[bx]; mov dl,es:[bx+2]; xor dh,dh end;
	return 0;
}

void XItem::PutN(longint N)
{
	// asm les bx,Self; mov ax,N.word; mov es:[bx],ax; mov al,N[2].byte;
	// mov es : [bx + 2] , al
}

WORD XItem::GetM(WORD O)
{
	// asm les bx,Self; add bx,O; xor ah,ah; mov al,es:[bx]
	return 0;
}

void XItem::PutM(WORD O, WORD M)
{
	// asm les bx,Self; add bx,O; mov ax,M; mov es:[bx],al
}

WORD XItem::GetL(WORD O)
{
	// asm les bx,Self; add bx,O; xor ah,ah; mov al,es:[bx+1]
	return 0;
}

void XItem::PutL(WORD O, WORD L)
{
	// asm les bx,Self; add bx,O; mov ax,L; mov es:[bx+1],al
}

XItem* XItem::Next(WORD O)
{
	// asm les bx,Self; add bx,O; xor ah,ah; mov al,es:[bx+1]; add ax,bx; add ax,2;
	// mov dx, es
	return nullptr;
}

WORD XItem::UpdStr(WORD O, pstring* S)
{
	/*asm  push ds; lds bx,Self; les di,S; cld; add bx,O;
     mov al,[bx];{M} add al,[bx+1];{L} stosb;
     mov al,[bx]; xor ah,ah; add di,ax; lea si,[bx+2];
     xor ch,ch; mov cl,[bx+1]; rep movsb; mov ax,si; pop ds;*/
	return 0;
}

WORD XPage::Off()
{
	if (IsLeaf) return oLeaf;
	else return oNotLeaf;
}

XItem* XPage::XI(WORD I)
{
	XItemPtr x; WORD o;
	x = XItemPtr(&A);
	o = Off();
	while (I > 1) { x = x->Next(o); I--; }
	return x;
}

uintptr_t XPage::EndOff()
{
	XItemPtr x = nullptr;
	WORD* xofs = (WORD*)x; // absolute x
	x = XI(NItems + 1); return uintptr_t(xofs);
}

bool XPage::Underflow()
{
	return (EndOff() - uintptr_t(A)) < (XPageSize - XPageOverHead) / 2;
}

bool XPage::Overflow()
{
	return EndOff() - uintptr_t(this) > XPageSize;
}

pstring XPage::StrI(WORD I)
{
	XItemPtr x = nullptr;
	WORD* xofs = (WORD*)x; // absolute x
	WORD o = 0;
	pstring* s = nullptr;

	x = XItemPtr(&A);
	o = Off();
	//TODO: asm les di, @result; mov s.WORD, di; mov s[2].WORD, es;

	if (I > NItems) s[0] = 0;
	else {
		for (WORD j = 1; j < I; j++) { *xofs = x->UpdStr(o, s); }
	}
	//TODO: co a jak to vrací?
	return "";
}

longint XPage::SumN()
{
	if (IsLeaf) { return NItems; }
	longint n = 0;
	XItemPtr x = XItemPtr(&A);
	WORD o = Off();
	for (WORD i = 1; i < NItems; i++) { n += x->GetN(); x = x->Next(o); }
	return n;
}

void XPage::Insert(WORD I, void* SS, XItem* XX)
{
	pstring* S = (pstring*)SS;
	XItemPtr x = nullptr, x2 = nullptr;
	WORD* xofs = (WORD*)x;
	WORD* x2ofs = (WORD*)x2;
	WORD m, m2, o, oE, l, l2, sz;
	integer d;

	o = Off(); oE = EndOff();
	NItems++; x = XI(I);
	m = 0;
	if (I > 1) m = SLeadEqu(StrI(I - 1), *S);
	l = S->length() - m;
	sz = o + 2 + l;
	if (I < NItems) {
		x2 = x;
		m2 = SLeadEqu(StrI(I), *S);
		d = m2 - x->GetM(o);
		if (d > 0) {
			l2 = x->GetL(o); x2ofs += d;
			Move(x, x2, o);
			x2->PutM(o, m2);
			x2->PutL(o, l2 - d);
			sz -= d;
		}
		Move(x2, uintptr_t(x2) + x2ofs + sz, oE - *x2ofs);
	}
	XX = x;
	x->PutM(o, m); x->PutL(o, l);
	xofs += (o + 2);
	Move(&S[m + 1], x, l);
}

void XPage::InsDownIndex(WORD I, longint Page, XPage* P)
{
	pstring s;
	XItemPtr x = nullptr;
	s = P->StrI(P->NItems);
	Insert(I, &s, x);
	x->PutN(P->SumN());
	x->DownPage = Page;
}

void XPage::Delete(WORD I)
{
	XItemPtr x = nullptr, x1 = nullptr, x2 = nullptr;
	WORD* xofs = (WORD*)x;
	WORD* x1ofs = (WORD*)x1;
	WORD* x2ofs = (WORD*)x2;
	WORD o = Off(); WORD oE = EndOff(); x = XI(I);
	if (I < NItems) {
		x2 = x->Next(o);
		integer d = x2->GetM(o) - x->GetM(o);
		if (d <= 0) Move(x2, x, oE - *x2ofs);
		else {
			Move(x2, x, o);
			x->PutL(o, x2->GetL(o) + d); x1 = x;
			*x1ofs = *x1ofs + o + 2 + d;
			*x2ofs = *x2ofs + o + 2;
			Move(x2, x1, oE - *x2ofs);
		}
		x = XI(NItems);
	}
	FillChar(x, oE - *xofs, 0);
	NItems--;
}

void XPage::AddPage(XPage* P)
{
	XItemPtr x = nullptr, x1 = nullptr;
	WORD* xofs = (WORD*)x;

	GreaterPage = P->GreaterPage;
	if (P->NItems == 0) return;
	XItemPtr xE = XI(NItems + 1);
	WORD oE = P->EndOff(); WORD o = Off(); x = XItemPtr(&P->A);
	if (NItems > 0) {
		WORD m = SLeadEqu(StrI(NItems), P->StrI(1));
		if (m > 0) {
			WORD l = x->GetL(o) - m;
			x1 = x;
			xofs += m;
			Move(x1, x, o);
			x->PutM(o, m); x->PutL(o, l);
		}
	}
	Move(x, xE, oE - *xofs);
	NItems += P->NItems;
}

void XPage::SplitPage(XPage* P, longint ThisPage)
{
	// figuruje tady pstring* s, ale výsledek se nikam neukládá, je to zakomentované

	XItemPtr x = nullptr, x1 = nullptr, x2 = nullptr;
	WORD* xofs = (WORD*)x;
	WORD* x1ofs = (WORD*)x1;
	WORD* x2ofs = (WORD*)x2;
	WORD o, oA, oE, n;
	pstring* s;

	x = XItemPtr(&A); x1 = x; o = Off(); oA = *xofs; oE = EndOff(); n = 0;
	while (*xofs - oA < oE - *xofs + x->GetM(o)) { x = x->Next(o); n++; }
	FillChar(P, XPageSize, 0);
	Move(x1, P->A, *xofs - oA);
	//s = (pstring*)(uintptr_t(x1) + oA + o + 1);;
	//s = &StrI(n + 1);
	Move(x, x1, o);
	x1->PutM(o, 0);
	x1 = x1->Next(o); x = x->Next(o); Move(x, x1, oE - *xofs);
	P->NItems = n; NItems -= n; *xofs = EndOff(); FillChar(x, oE - *xofs, 0);
	if (IsLeaf) P->GreaterPage = ThisPage; else P->GreaterPage = 0;
	P->IsLeaf = IsLeaf;
}

XWFile* XKey::XF()
{
	if (InWork) return &XWork;
	return CFile->XF;
}

longint XKey::NRecs()
{
	if (InWork) return NR;
	return CFile->XF->NRecs;
}

bool XKey::Search(XString& XX, bool AfterEqu, longint& RecNr)
{
	bool searchResult = false;
	XPagePtr p;
	WORD iItem = 0;
	char result;
	{
		p = (XPage*)GetStore(XPageSize);
	label1:
		XPathN = 1; longint page = IndexRoot; AfterEqu = AfterEqu && Duplic;
		XPath[XPathN].Page = page;
		XF()->RdPage(p, page);
		XItemPtr x = XItemPtr(p->A);
		WORD o = p->Off();
		WORD nItems = p->NItems;
		if (nItems == 0) {
			RecNr = CFile->NRecs + 1;
			XPath[1].I = 1;
			goto label2;
		}

		//__asm {
		//	push ds;
		//	cld;
		//	les dx, x;
		//	mov iItem, 1;
		//}

		//asm
		//	push ds; cld; les bx, x; mov iItem, 1; mov dx, 1;
		//@@add 1 bx, o;xor ax, ax; mov al, es: [bx] ; cmp dx, ax; jna @@5; /*first different <= prefix length?*/
		//mov dx, ax; lds si, XX;xor ax, ax; lodsb; sub ax, dx; add si, dx;
		//mov ah, es: [bx + 1] ; /*pstring length*/ lea di, [bx + 2]; /*pstring addr*/
		//xor cx, cx; mov cl, ah; cmp ah, al; jna @@2; mov cl, al;  /*min length*/
		//@@add 2 dx, cx;xor ch, ch; /*set zero flag*/
		//repe cmpsb; jb @@8; ja @@4; cmp al, ah; jb @@8; ja @@3;
		//cmp AfterEqu, 0; je @@7;
		//@@inc 3 dx;
		//@@sub 4 dx, cx;
		//@@mov 5 ax, iItem; cmp ax, nItems; je @@6; /*last item?*/
		//inc ax; mov iItem, ax;
		//xor ax, ax; mov al, es: [bx + 1] ; add ax, 2; add bx, ax;  /*next item*/
		//jmp @@1;
		//@@mov 6 al, _gt; inc iItem; jmp @@9;
		//@@mov 7 al, _equ; jmp @@9;
		//@@mov 8 al, _lt;
		//@@mov 9 result, al; sub bx, o; mov x.WORD, bx; pop ds; }
		XPath[XPathN].I = iItem;
		if (p->IsLeaf) {
			if (iItem > nItems) RecNr = CFile->NRecs + 1; else RecNr = x->GetN();
			if (searchResult == _equ)
				if
#ifdef FandSQL
					!CFile->IsSQLFile&&
#endif
					(((RecNr == 0) || (RecNr > CFile->NRecs))) XF()->Err(833);
				else searchResult = true;
			else
				label2:
			searchResult = false;
			ReleaseStore(p);
			return searchResult;
		}
		if (iItem > nItems) page = p->GreaterPage;
		else page = x->DownPage;
		XPathN++;
		goto label1;
	}
	return searchResult;
}

bool XKey::SearchIntvl(XString& XX, bool AfterEqu, longint& RecNr)
{
	return Search(XX, AfterEqu, RecNr) || Intervaltest && (RecNr <= CFile->NRecs);
}

longint XKey::PathToNr()
{
	WORD i, j; longint n; XPagePtr p; XItemPtr x;
	p = (XPage*)GetStore(XPageSize); n = 0;
	for (j = 1; j < XPathN - 1; j++)
	{
		XF()->RdPage(p, XPath[j].Page);
		x = XItemPtr(p->A);
		for (i = 1; i < XPath[j].I - 1; i++) { (n += x->GetN()); x = x->Next(oNotLeaf); };
	}
	n += XPath[XPathN].I;
	if (n > NRecs() + 1) XF()->Err(834);
	ReleaseStore(p);
	return n;
}

void XKey::NrToPath(longint I)
{
	XPagePtr p = (XPage*)GetStore(XPageSize);
	longint page = IndexRoot; XPathN = 0;
label1:
	XF()->RdPage(p, page); XPathN++; XPath[XPathN].Page = page;
	if (p->IsLeaf) {
		if (I > p->NItems + 1) XF()->Err(837);
		XPath[XPathN].I = I;
		ReleaseStore(p);
		return;
	}
	XItemPtr x = XItemPtr(p->A);
	for (WORD j = 1; j < p->NItems; j++) {
		if (I <= x->GetN()) { XPath[XPathN].I = j; page = x->DownPage; goto label1; }
		I -= x->GetN();
		x = x->Next(oNotLeaf);
	}
	XPath[XPathN].I = p->NItems + 1;
	page = p->GreaterPage;
	goto label1;
}

longint XKey::PathToRecNr()
{
	auto X = XPath[XPathN];
	XPagePtr p = (XPage*)GetStore(XPageSize); /* !!! with XPath[XPathN] do!!! */
	XF()->RdPage(p, X.Page);
	longint recnr = p->XI(X.I)->GetN();
	longint result = recnr;
	if ((recnr == 0) || (recnr > CFile->NRecs)) XF()->Err(835);
	ReleaseStore(p);
	return result;
}

bool XKey::RecNrToPath(XString& XX, longint RecNr)
{
	bool result;
	XPagePtr p; XItemPtr x; longint n;
	XX.PackKF(KFlds); Search(XX, false, n);
	p = (XPage*)GetStore(XPageSize);
	result = false; /* !!! with XPath[XPathN] do!!! */
	{
		auto X = XPath[XPathN];
	label1:
		XF()->RdPage(p, X.Page);
		x = p->XI(X.I);
		if (!(p->StrI(X.I) == XX.S)) goto label3;
	label2:
		if (x->GetN() == RecNr) { result = true; goto label3; }
		X.I++;
		if (X.I > p->NItems) {
			if (IncPath(XPathN - 1, X.Page)) { X.I = 1; goto label1; }
		}
		else {
			x = x->Next(oLeaf); if (x->GetL(oLeaf) != 0) goto label3; goto label2;
		}; }
label3:
	ReleaseStore(p);
	return result;
}

bool XKey::IncPath(WORD J, longint& Pg)
{
	bool result;
	XPagePtr p;
	p = (XPage*)GetStore(XPageSize);
	result = false;
	auto X = XPath[J];
	if (J == 0) { goto label2; } /* !!! with XPath[J] do!!! */
	{
	label1:
		XF()->RdPage(p, X.Page);
		if (X.I > p->NItems)
			if (IncPath(J - 1, X.Page)) { X.I = 0; goto label1; }
			else goto label2;
		X.I++;
		if (X.I > p->NItems)
			if (p->GreaterPage == 0) {
				X.I = 0; if (IncPath(J - 1, X.Page)) goto label1; goto label2;
			}
			else Pg = p->GreaterPage;
		else Pg = p->XI(X.I)->DownPage;
	}
	result = true;
label2:
	ReleaseStore(p);
	return result;
}

longint XKey::NrToRecNr(longint I)
{
	NrToPath(I);
	return PathToRecNr();
}

pstring XKey::NrToStr(longint I)
{
	pstring result;
	XPagePtr p = (XPage*)GetStore(XPageSize); NrToPath(I); /* !!! with XPath[XPathN] do!!! */
	XF()->RdPage(p, XPath[XPathN].Page);
	result = p->StrI(I);
	ReleaseStore(p);
	return result;
}

longint XKey::RecNrToNr(longint RecNr)
{
	XString x;
	if (RecNrToPath(x, RecNr)) return PathToNr(); else return 0;
}

bool XKey::FindNr(XString& X, longint& IndexNr)
{
	longint n;
	auto result = Search(X, false, n);
	IndexNr = PathToNr();
	return result;
}

void XKey::InsertOnPath(XString& XX, longint RecNr)
{
	WORD i, j;
	longint page, page1, uppage, downpage;
	XItemPtr x;
	longint n, upsum;
	XPagePtr p, p1, upp;

	p = (XPage*)GetStore(2 * XPageSize);
	p1 = (XPage*)GetStore(2 * XPageSize);
	upp = (XPage*)GetStore(2 * XPageSize);
	for (j = XPathN; j > 1; j--) {
		page = XPath[j].Page; XF()->RdPage(p, page); i = XPath[j].I;
		if (p->IsLeaf) {
			InsertItem(XX, p, upp, page, i, x, uppage); x->PutN(RecNr);
		}
		else {
			if (i <= p->NItems) {
				x = p->XI(i); n = x->GetN() + 1; if (uppage != 0) n -= upsum;
				x->PutN(n);
			}
			if (uppage != 0) {
				downpage = uppage; InsertItem(XX, p, upp, page, i, x, uppage);
				x->DownPage = downpage; x->PutN(upsum);
			};
		}
		XF()->WrPage(p, page); if (uppage != 0) {
			XF()->WrPage(upp, uppage); upsum = upp->SumN();
			if (upp->IsLeaf) ChainPrevLeaf(p1, uppage);
		}
	}
	if (uppage != 0) {
		page1 = XF()->NewPage(p1); p1->GreaterPage = page1;
		p1->InsDownIndex(1, uppage, upp); XF()->WrPage(p, page1); XF()->WrPage(p1, page);
		if (upp->IsLeaf) {
			upp->GreaterPage = page1; XF()->WrPage(upp, uppage);
		}
	}
	ReleaseStore(p);
}

void XKey::InsertItem(XString& XX, XPage* P, XPage* UpP, longint Page, WORD I, XItemPtr& X, longint& UpPage)
{
	P->Insert(I, &XX.S, X);
	UpPage = 0;
	if (P->Overflow()) {
		UpPage = XF()->NewPage(UpP);
		P->SplitPage(UpP, Page);
		if (I <= UpP->NItems) X = UpP->XI(I);
		else X = P->XI(I - UpP->NItems);
		XX.S = UpP->StrI(UpP->NItems);
	}
}

void XKey::ChainPrevLeaf(XPagePtr P, longint N)
{
	longint page; WORD i, j;
	for (j = XPathN - 1; j > 1; j--)
		if (XPath[j].I > 1) {
			XF()->RdPage(P, XPath[j].Page); i = XPath[j].I - 1;
		label1:
			page = P->XI(i)->DownPage; XF()->RdPage(P, page);
			if (P->IsLeaf) { P->GreaterPage = N; XF()->WrPage(P, page); return; }
			i = P->NItems; goto label1;
		}
}

bool XKey::Insert(longint RecNr, bool Try)
{
	longint N, XNr; XString x;
	x.PackKF(KFlds);
	if (Search(x, true, N)) {
		if (Try) { return false; }
	}
	else
	{
		XFNotValid();
		CFileError(822);
	}
	InsertOnPath(x, RecNr);
	return true;
}

void XKey::DeleteOnPath()
{
	longint page, page1, page2;
	longint uppage = 0;
	void* pp = nullptr;
	XItem* x = nullptr;
	bool released;
	longint n;

	MarkStore(pp);
	XPage* p = (XPage*)GetStore(2 * XPageSize);
	XPage* p1 = (XPage*)GetStore(2 * XPageSize);
	XPage* p2 = (XPage*)GetStore(2 * XPageSize);
	XPage* upp = p2;
	for (WORD j = XPathN; j > 1; j--) {
		page = XPath[j].Page; XF()->RdPage(p, page); WORD i = XPath[j].I;
		if (p->IsLeaf) p->Delete(i);
		else if (upp->Underflow()) {
			XF()->WrPage(upp, uppage);
			WORD i1 = i - 1;
			WORD i2 = i;
			if (i1 == 0) { i1 = 1; i2 = 2; }
			XIDown(p, p1, i1, page1); XIDown(p, p2, i2, page2);
			BalancePages(p1, p2, released);
			XF()->WrPage(p1, page1); p->Delete(i1);
			if (released) {
				XF()->ReleasePage(p2, page2);
				if (i1 > p->NItems) p->GreaterPage = page1;
				else { p->InsDownIndex(i1, page1, p1); p->Delete(i2); }
			}
			else {
				XF()->WrPage(p2, page2); p->InsDownIndex(i1, page1, p1);
				if (i2 <= p->NItems) {
					p->Delete(i2); p->InsDownIndex(i2, page2, p2);
				}
			}
		}
		else {
			if (upp->Overflow()) {
				page1 = XF()->NewPage(p1); upp->SplitPage(p1, uppage);
				XF()->WrPage(p1, page1); p->InsDownIndex(i, page1, p1); i++;
			}
			XF()->WrPage(upp, uppage);
			if (i <= p->NItems) {
				p->Delete(i); p->InsDownIndex(i, uppage, upp);
			}
		}
		uppage = page;
		XPage* px = upp;
		upp = p;
		p = px;
	}
	if (upp->Overflow()) {
		page1 = XF()->NewPage(p1); upp->SplitPage(p1, uppage); page = XF()->NewPage(p);
		p->GreaterPage = page; p->InsDownIndex(1, page1, p1);
		XF()->WrPage(p1, page1); XF()->WrPage(p, uppage); XF()->WrPage(upp, page);
	}
	else {
		page1 = upp->GreaterPage;
		if ((upp->NItems == 0) && (page1 > 0)) {
			XF()->RdPage(p1, page1); Move(p1, upp, XPageSize);
			XF()->ReleasePage(p1, page1);
		}
		XF()->WrPage(upp, uppage);
	}
	ReleaseStore(pp);
}

void XKey::BalancePages(XPage* P1, XPage* P2, bool& Released)
{
	longint n; WORD sz;
	n = P1->GreaterPage;
	P1->AddPage(P2);
	sz = P1->EndOff() - uintptr_t(P1);
	if (sz <= XPageSize) Released = true;
	else {
		Released = false; Move(P1, P2, sz);
		P2->SplitPage(P1, n);
	}
}

void XKey::XIDown(XPage* P, XPage* P1, WORD I, longint& Page1)
{
	if (I > P->NItems) Page1 = P->GreaterPage;
	else Page1 = P->XI(I)->DownPage;
	XF()->RdPage(P1, Page1);
}

bool XKey::Delete(longint RecNr)
{
	XString xx;
	bool b = RecNrToPath(xx, RecNr);
	if (b) DeleteOnPath();
	return b;
}

void XWKey::Open(KeyFldD* KF, bool Dupl, bool Intvl)
{
	KFlds = KF; Duplic = Dupl; InWork = true; Intervaltest = Intvl; NR = 0;
	XPagePtr p = (XPage*)GetStore(sizeof(p)); IndexRoot = XF()->NewPage(p);
	p->IsLeaf = true; XF()->WrPage(p, IndexRoot); ReleaseStore(p); IndexLen = 0;
	while (KF != nullptr) {
		IndexLen += KF->FldD->NBytes;
		KF = KF->Chain;
	}
}

void XWKey::Close()
{
	ReleaseTree(IndexRoot, true); IndexRoot = 0;
}

void XWKey::Release()
{
	ReleaseTree(IndexRoot, false); NR = 0;
}

void XWKey::ReleaseTree(longint Page, bool IsClose)
{
	if ((Page == 0) || (Page > XF()->MaxPage)) return;
	XPagePtr p = (XPage*)GetStore(XPageSize);
	XF()->RdPage(p, Page);
	if (not p->IsLeaf) {
		WORD n = p->NItems;
		for (WORD i = 1; i < n; i++) {
			ReleaseTree(p->XI(i)->DownPage, IsClose);
			XF()->RdPage(p, Page);
		}
		if (p->GreaterPage != 0) ReleaseTree(p->GreaterPage, IsClose);
	}
	if ((Page != IndexRoot) || IsClose)
		XF()->ReleasePage(p, Page);
	else {
		FillChar(p, XPageSize, 0); p->IsLeaf = true; XF()->WrPage(p, Page);
	}
	ReleaseStore(p);
}

void XWKey::OneRecIdx(KeyFldD* KF, longint N)
{
	Open(KF, true, false); Insert(N, true); NR++;
}

void XWKey::InsertAtNr(longint I, longint RecNr)
{
	XString x;
	x.PackKF(KFlds); NR++; NrToPath(I); InsertOnPath(x, RecNr);
}

longint XWKey::InsertGetNr(longint RecNr)
{
	XString x; longint n;
	NR++; x.PackKF(KFlds); Search(x, true, n);
	auto result = PathToNr();
	InsertOnPath(x, RecNr);
	return result;
}

void XWKey::DeleteAtNr(longint I)
{
	NrToPath(I); DeleteOnPath(); NR--;
}

void XWKey::AddToRecNr(longint RecNr, integer Dif)
{
	if (NRecs() == 0) return;
	NrToPath(1);
	XPagePtr p = (XPage*)GetStore(sizeof(*p)); /* !!! with XPath[XPathN] do!!! */
	longint pg = XPath[XPathN].Page;
	integer j = XPath[XPathN].I;
	do {
		XF()->RdPage(p, pg);
		integer n = p->NItems - j + 1;
		XItemPtr x = p->XI(j);
		while (n > 0) {
			longint nn = x->GetN();
			if (nn >= RecNr) x->PutN(nn + Dif);
			x = x->Next(oLeaf); n--;
		}
		XF()->WrPage(p, pg); pg = p->GreaterPage; j = 1;
	} while (pg != 0);
	ReleaseStore(p);
}

void XWFile::Err(WORD N)
{
	if (this == &XWork) { SetMsgPar(gcfg1->FandWorkXName); RunError(N); }
	else {
		CFile->XF->SetNotValid();
		CFileMsg(N, 'X');
		CloseGoExit();
	}
}

void XWFile::TestErr()
{
	if (gcfg1->HandleError != 0) Err(700 + gcfg1->HandleError);
}

longint XWFile::UsedFileSize()
{
	return longint(MaxPage + 1) << XPageShft;
}

bool XWFile::NotCached()
{
	return (this != &XWork) && CFile->NotCached();
}

void XWFile::RdPage(XPagePtr P, longint N)
{
	if ((N == 0) || (N > MaxPage)) Err(831);
	RdWrCache(true, Handle, NotCached(), N << XPageShft, XPageSize, P);
}

void XWFile::WrPage(XPagePtr P, longint N)
{
	if (UpdLockCnt > 0) Err(645);
	RdWrCache(false, Handle, NotCached(), N << XPageShft, XPageSize, P);
}

longint XWFile::NewPage(XPagePtr P)
{
	longint result = 0;
	if (FreeRoot != 0) {
		result = FreeRoot;
		RdPage(P, FreeRoot);
		FreeRoot = P->GreaterPage;
	}
	else {
		MaxPage++;
		if (MaxPage > 0x1fffff) Err(887);
		result = MaxPage;
	}
	FillChar(P, XPageSize, 0);
	return result;
}

void XWFile::ReleasePage(XPagePtr P, longint N)
{
	FillChar(P, XPageSize, 0);
	P->GreaterPage = FreeRoot;
	FreeRoot = N;
	WrPage(P, N);
}

void XFile::SetEmpty()
{
	auto p = (XPage*)GetZStore(XPageSize);
	WrPage(p, 0);
	p->IsLeaf = true; FreeRoot = 0; NRecs = 0;
	KeyDPtr k = CFile->Keys;
	while (k != nullptr) {
		longint n = k->IndexRoot;
		MaxPage = n;
		WrPage(p, n);
		k = k->Chain;
	}
	ReleaseStore(p);
	WrPrefix();
}

void XFile::RdPrefix()
{
	RdWrCache(true, Handle, NotCached(), 2, 18, &FreeRoot);
}

void XFile::WrPrefix()
{
	WORD Signum = 0x04FF;
	RdWrCache(false, Handle, NotCached(), 0, 2, &Signum);
	NRecsAbs = CFile->NRecs; NrKeys = CFile->GetNrKeys();
	RdWrCache(false, Handle, NotCached(), 2, 18, &FreeRoot);
}

void XFile::SetNotValid()
{
	NotValid = true; MaxPage = 0; WrPrefix(); SaveCache(0);
}

XScan::XScan(FileD* aFD, KeyD* aKey, KeyInD* aKIRoot, bool aWithT)
{
	FD = aFD; Key = aKey; KIRoot = aKIRoot; withT = aWithT;
#ifdef FandSQL
	if (aFD->IsSQLFile) {
		if ((aKey != nullptr) && aKey->InWork) { P = (XPage*)GetStore(XPageSize); Kind = 3; }
		else Kind = 4;
	}
	else
#endif
	{
		P = (XPage*)GetStore(XPageSize);
		Kind = 1;
		if (aKIRoot != nullptr) Kind = 2;
	}
	}

void XScan::Reset(FrmlPtr ABool, bool SQLFilter)
{
	KeyInD* k; longint n; XString xx; bool b;
	CFile = FD;
	Bool = ABool;
	if (SQLFilter) {
		if (CFile->IsSQLFile) hasSQLFilter = true;
		else Bool = nullptr;
	}
	switch (Kind) {
	case 0: NRecs = CFile->NRecs; break;
	case 1:
	case 3: { if (!Key->InWork) TestXFExist(); NRecs = Key->NRecs(); break; }
	case 2: {
		if (!Key->InWork) TestXFExist();
		CompKIFrml(Key, KIRoot, true); NRecs = 0; k = KIRoot;
		while (k != nullptr) {
			XString a1;
			XString b2;
			a1.S = *k->X1;
			b2.S = *k->X2;
			Key->FindNr(a1, k->XNrBeg);
			b = Key->FindNr(b2, n);
			k->N = 0;
			if (n >= k->XNrBeg) k->N = n - k->XNrBeg + b;
			NRecs += k->N;
			k = k->Chain;
		}
		break;
	}
#ifdef FandSQL
	case 4: { CompKIFrml(Key, KIRoot, false); New(SQLStreamPtr(Strm), Init); IRec = 1; break; }
#endif
	}
	SeekRec(0);
}

void XScan::ResetSort(KeyFldDPtr aSK, FrmlPtr& BoolZ, LockMode OldMd, bool SQLFilter)
{
	LockMode m;
	if (Kind == 4) {
		SK = aSK;
		if (SQLFilter) { Reset(BoolZ, true); BoolZ = nullptr; }
		else Reset(nullptr, false);
		return;
}
	if (aSK != nullptr) {
		Reset(BoolZ, false);
		ScanSubstWIndex(this, aSK, 'S');
		BoolZ = nullptr;
	}
	else Reset(nullptr, false);
	/* !!! with CFile^ do!!! */
	if (CFile->NotCached()) {
		switch (Kind) {
		case 0: { m = NoCrMode; if (CFile->XF != nullptr) m = NoExclMode; break; }
		case 1: { m = OldMd; if (Key->InWork) m = NoExclMode; break; }
		default: return;
		}
		m = LockMode(MaxW(m, OldMd));
		if (m != OldMd) ChangeLMode(m, 0, true);
	}
}

void XScan::SubstWIndex(WKeyDPtr WK)
{
	Key = WK; if (Kind != 3) Kind = 1;
	if (P == nullptr) P = (XPage*)GetStore(XPageSize);
	NRecs = Key->NRecs();
	Bool = nullptr; SeekRec(0); TempWX = true;
}

void XScan::ResetOwner(XString* XX, FrmlPtr aBool)
{
	longint n; bool b;
	CFile = FD; Bool = aBool;
#ifdef FandSQL
	if (Kind = 4) {           /* !on .SQL with Workindex */
		KIRoot = GetZStore(sizeof(KIRoot^));
		KIRoot->X1 = StoreStr(XX->S); KIRoot->X2 = StoreStr(XX->S);
		New(SQLStreamPtr(Strm), Init); IRec = 1
	}
	else
#endif
	{
		TestXFExist();
		KIRoot = (KeyInD*)GetZStore(sizeof(*KIRoot));
		Key->FindNr(*XX, KIRoot->XNrBeg);
		AddFFs(Key, XX->S);
		b = Key->FindNr(*XX, n); NRecs = n - KIRoot->XNrBeg + b;
		KIRoot->N = NRecs; Kind = 2;
	}
	SeekRec(0);
}

bool EquKFlds(KeyFldDPtr KF1, KeyFldDPtr KF2)
{
	bool result = false;
	while (KF1 != nullptr) {
		if ((KF2 == nullptr) || (KF1->CompLex != KF2->CompLex) || (KF1->Descend != KF2->Descend)
			|| (KF1->FldD->Name != KF2->FldD->Name)) return result;
		KF1 = KF1->Chain; KF2 = KF2->Chain;
	}
	if (KF2 != nullptr) return false;
	return true;
}

void XScan::ResetOwnerIndex(LinkDPtr LD, LocVar* LV, FrmlPtr aBool)
{
	WKeyDPtr k;
	CFile = FD; TestXFExist(); Bool = aBool; OwnerLV = LV; Kind = 2;
	if (!EquKFlds(WKeyDPtr(LV->RecPtr)->KFlds, LD->ToKey->KFlds)) RunError(1181);
	SeekRec(0);
}

#ifdef FandSQL
void XScan::ResetSQLTxt(FrmlPtr Z)
{
	LongStrPtr s;
	New(SQLStreamPtr(Strm), Init); s = RunLongStr(Z);
	SQLStreamPtr(Strm)->InpResetTxt(s); ReleaseStore(s);
	eof = false;
}
#endif

void XScan::ResetLV(void* aRP)
{
	Strm = aRP; Kind = 5; NRecs = 1;
}

void XScan::Close()
{
	CFile = FD;
#ifdef FandSQL
	if (Kind = 4) /* !!! with SQLStreamPtr(Strm)^ do!!! */ { InpClose; Done; }
#endif
	if (TempWX) WKeyDPtr(Key)->Close();
}

void XScan::SeekRec(longint I)
{
	KeyInD* k; FrmlPtr z;
	CFile = FD;

#ifdef FandSQL
	if (Kind == 4) {
		if (I != IRec) /* !!! with SQLStreamPtr(Strm)^ do!!! */
		{
			if (NotFrst) InpClose; NotFrst = true;
			if (hasSQLFilter) z = Bool else z = nullptr;
			InpReset(Key, SK, KIRoot, z, withT);
			EOF = AtEnd; IRec = 0; NRecs = 0x20000000;
	}
		return;
}
#endif
	if ((Kind == 2) && (OwnerLV != nullptr)) {
		IRec = 0;
		NRecs = 0x20000000;
		iOKey = 0;
		NextIntvl();
		eof = I >= NRecs;
		return;
	}
	IRec = I;
	eof = I >= NRecs;
	if (!eof)
		switch (Kind) {
		case 1:
		case 3: {
			Key->NrToPath(I + 1); /* !!! with XPath[XPathN] do!!! */
			SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
			break;
		}
		case 2: {
			k = KIRoot;
			while (I >= k->N) { I -= k->N; k = k->Chain; }
			KI = k; SeekOnKI(I);
			break;
		}
	}
	}

bool DeletedFlag()  // r771 ASM
{
	// TODO:
	return false;
}

void ClearDeletedFlag()
{
}

void SetDeletedFlag()
{
}

integer CompStr(pstring& S1, pstring& S2)
{
	return 0;
}

void CmpLxStr()
{
}

WORD CompLexLongStr(LongStrPtr S1, LongStrPtr S2)
{
	return 0;
}

WORD CompLexLongShortStr(LongStrPtr S1, pstring& S2)
{
	return 0;
}

WORD CompLexStr(const pstring& S1, const pstring& S2)
{
	return 0;
}


void XScan::GetRec()
{
	XString xx;
	CFile = FD;
#ifdef FandSQL
	if (Kind == 4) {
		repeat EOF = !SQLStreamPtr(Strm)->GetRec
			until EOF || hasSQLFilter || RunBool(Bool);
		inc(IRec); return;
	}
#endif
label1:
	eof = IRec >= NRecs;
	if (!eof) {
		IRec++;
		switch (Kind) {
		case 0: { RecNr = IRec; goto label2; break; }
		case 1:
		case 2: { RecNr = X->GetN(); NOnPg--;
			if (NOnPg > 0) X = X->Next(oLeaf);
			else if ((Kind == 2) && (NOfKI == 0)) NextIntvl();
			else if (P->GreaterPage > 0) SeekOnPage(P->GreaterPage, 1);
		label2:
			ReadRec(RecNr); if (DeletedFlag()) goto label1;
		label3:
			if (!RunBool(Bool)) goto label1;
		}
#ifdef FandSQL
		case 3: {
			NOnPg--;
			xx.S = P->StrI(P->NItems - NOnPg);
			if ((NOnPg == 0) && (P->GreaterPage > 0)) SeekOnPage(P->GreaterPage, 1);
			if (!Strm1->SelectXRec(Key, @xx, _equ, withT)) goto label1;
			goto label3;
			break;
		}
#endif
		case 5:
		{
			Move(Strm, CRecPtr, CFile->RecLen + 1);
			break;
		}
		}
	}
}

void XScan::SeekOnKI(longint I)
{
	NOfKI = KI->N - I; Key->NrToPath(KI->XNrBeg + I);
	/* !!! with XPath[XPathN] do!!! */
	SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
}

void XScan::SeekOnPage(longint Page, WORD I)
{
	Key->XF()->RdPage(P, Page); NOnPg = P->NItems - I + 1;
	if (Kind == 2)
	{
		if (NOnPg > NOfKI) NOnPg = NOfKI;
		NOfKI -= NOnPg;
	}
	X = P->XI(I);
}

void XScan::NextIntvl()
{
	XString xx; bool b; longint n, nBeg; WKeyDPtr k;
	if (OwnerLV != nullptr) {
		k = WKeyDPtr(OwnerLV->RecPtr);
		while (iOKey < k->NRecs()) {
			iOKey++;
			CFile = OwnerLV->FD; xx.S = k->NrToStr(iOKey);
			CFile = FD;
			Key->FindNr(xx, nBeg); AddFFs(Key, xx.S);
			b = Key->FindNr(xx, n);
			n = n - nBeg + b;
			if (n > 0) {
				NOfKI = n;
				Key->NrToPath(nBeg); /* !!! with XPath[XPathN] do!!! */
				SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
				return;
			};
		}
		NRecs = IRec; /*EOF*/
	}
	else {
		do { KI = KI->Chain; } while ((KI != nullptr) || (KI->N > 0));
		if (KI != nullptr) SeekOnKI(0);
	}
}


pstring* FieldDMask(FieldDPtr F)
{
	return nullptr;
}

void* GetRecSpace()
{
	return GetZStore(CFile->RecLen + 2);
}

void* GetRecSpace2()
{
	return GetZStore2(CFile->RecLen + 2);
}

WORD CFileRecSize()
{
	return 0;
}

void SetTWorkFlag()
{
}

bool HasTWorkFlag()
{
	return false;
}

void SetUpdFlag()
{
}

void ClearUpdFlag()
{
}

bool HasUpdFlag()
{
	return false;
}

void* LocVarAd(LocVar* LV)
{
	return nullptr;
}

/// nedìlá nic
void XDecode(LongStrPtr S)
{
	return;
}

void CodingLongStr(LongStrPtr S)
{
	if (CFile->TF->LicenseNr == 0) Code(S->A, S->LL);
	else XDecode(S);
}

void DirMinusBackslash(pstring& D)
{
	if ((D.length() > 3) && (D[D.length() - 1] == '\\')) D[0]--;
}

longint StoreInTWork(LongStrPtr S)
{
	return TWork.Store(S);
}

LongStrPtr ReadDelInTWork(longint Pos)
{
	auto result = TWork.Read(1, Pos);
	TWork.Delete(Pos);
	return result;
}

void ForAllFDs(void(*procedure)())
{
	RdbDPtr R; FileDPtr cf;
	cf = CFile; R = CRdb;
	while (R != nullptr) {
		CFile = R->FD;
		while (CFile != nullptr) { procedure(); CFile = CFile->Chain; };
		R = R->ChainBack;
	}
	CFile = cf;
}

bool IsActiveRdb(FileDPtr FD)
{
	RdbDPtr R;
	R = CRdb; while (R != nullptr) {
		if (FD == R->FD) return true;
		R = R->ChainBack;
	}
	return false;
}

void ResetCompilePars()
{
	char a;
	RdFldNameFrml = RdFldNameFrmlF;
	RdFunction = nullptr;
	ChainSumEl = nullptr;
	FileVarsAllowed = true;
	FDLocVarAllowed = false;
	IdxLocVarAllowed = false;
	PrevCompInp = nullptr;
}


