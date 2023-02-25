#include "access.h"
#include "../pascal/real48.h"
#include "../pascal/asm.h"
#include "compile.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "legacy.h"
#include "oaccess.h"
#include "obaseww.h"
#include "olongstr.h"
#include "runfrml.h"
#include "../Indexes/sort.h"
#include "TFile.h"
#include "XFile.h"
#include "../Indexes/XKey.h"
#include "../Logging/Logging.h"
#include "../textfunc/textfunc.h"

integer CompLongStr(LongStr* S1, LongStr* S2)
{
	integer result = 0;
	if (S1->LL != S2->LL) {
		if (S1->LL < S2->LL) return _lt;
		else return _gt;
	}
	if (S2->LL == 0) return _equ;
	for (size_t i = 0; i < S2->LL; i++)
	{
		if (S1->A[i] == S2->A[i]) continue;
		if (S1->A[i] < S2->A[i]) return _lt;
		return _gt;
	}
	return _equ;
}

integer CompLongShortStr(LongStr* S1, pstring* S2)
{
	integer result = 0;
	if (S1->LL != (*S2)[0]) {
		if (S1->LL < (*S2)[0]) result = _lt;
		else result = _gt;
	}
	if ((*S2)[0] == 0) return result;
	for (size_t i = 0; i < (*S2)[0]; i++)
	{
		if ((BYTE)S1->A[i] == (*S2)[i + 1]) continue;
		if ((BYTE)S1->A[i] < (*S2)[i + 1]) return _lt;
		return _gt;
	}
	return _equ;
}

integer CompArea(void* A, void* B, integer L)
{
	int result = memcmp(A, B, L);

	if (result == 0) return _equ;
	if (result < 0) return _lt;
	return _gt;
}

void ResetCFileUpdH()
{
	ResetUpdHandle(CFile->Handle);
	if (CFile->file_type == FileType::INDEX) ResetUpdHandle(CFile->XF->Handle);
	if (CFile->TF != nullptr) ResetUpdHandle(CFile->TF->Handle);
}

void ClearCacheCFile()
{
	// cache nepouzivame
	return;
	/* !!! with CFile^ do!!! */
	/*ClearCacheH(CFile->Handle);
	if (CFile->file_type == INDEX) ClearCacheH(CFile->GetXFile->Handle);
	if (CFile->TF != nullptr) ClearCacheH(CFile->TF->Handle);*/
}

#ifdef FandNetV
// const longint TransLock = 0x0A000501;  /* locked while state transition */
const longint TransLock = 0x40000501; // MB160
// const longint ModeLock = 0x0A000000;  /* base for mode locking */
const longint ModeLock = 0x40000000;  // MB160
// const longint RecLock = 0x0B000000;  /* base for record locking */
const longint RecLock = 0x41000000;   // MB160


bool TryLockH(FILE* Handle, longint Pos, WORD Len)
{
	OVERLAPPED sOverlapped;
	sOverlapped.Offset = Pos;
	sOverlapped.OffsetHigh = 0;
	auto fSuccess = LockFileEx(Handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, Len, 0, &sOverlapped);

	if (!fSuccess) {
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError();

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

		return false;
	}
	return true;
}

bool UnLockH(FILE* Handle, longint Pos, WORD Len)
{
	OVERLAPPED sOverlapped;
	sOverlapped.Offset = Pos;
	sOverlapped.OffsetHigh = 0;
	auto fSuccess = UnlockFileEx(Handle, 0, Len, 0, &sOverlapped);

	if (!fSuccess) {
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError();

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

		return false;
	}
	return true;
}

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

bool ChangeLMode(FileD* fileD, LockMode Mode, WORD Kind, bool RdPref)
{
	longint oldpos; WORD oldlen, d;
	bool result = false;
	if (!fileD->IsShared()) {         /*neu!!*/
		result = true;
		fileD->LMode = Mode;
		return result;
	}
	result = false;
	LockMode oldmode = fileD->LMode;
	FILE* h = fileD->Handle;
	if (oldmode >= WrMode) {
		if (Mode < WrMode) WrPrefixes();
		if (oldmode == ExclMode) {
			SaveCache(0, fileD->Handle);
			ClearCacheCFile();
		}
		if (Mode < WrMode) ResetCFileUpdH();
	}
	longint w = 0;
	WORD count = 0;
label1:
	if (Mode != NullMode)
		if (!TryLockH(h, TransLock, 1)) {
		label2:
			if (Kind == 2) return result; /*0 Kind-wait, 1-wait until ESC, 2-no wait*/
			count++;
			if (count <= spec.LockRetries) d = spec.LockDelay;
			else {
				d = spec.NetDelay;
				SetCPathVol();
				SetMsgPar(CPath, LockModeTxt[Mode]);
				longint w1 = PushWrLLMsg(825, Kind == 1);
				if (w == 0) w = w1;
				else TWork.Delete(w1);
				LockBeep();
			}
			if (KbdTimer(spec.NetDelay, Kind)) goto label1;
			if (w != 0) PopW(w);
			return result;
		}
	if (oldmode != NullMode) {
		ModeLockBnds(oldmode, oldpos, oldlen);
		UnLockH(h, oldpos, oldlen);
	}
	if (Mode != NullMode) {
		WORD len;
		longint pos;
		ModeLockBnds(Mode, pos, len);
		if (!TryLockH(h, pos, len)) {
			if (oldmode != NullMode) TryLockH(h, oldpos, oldlen);
			UnLockH(h, TransLock, 1);
			goto label2;
		}
		UnLockH(h, TransLock, 1);
	}
	if (w != 0) PopW(w);
	fileD->LMode = Mode;
	if ((oldmode < RdMode) && (Mode >= RdMode) && RdPref) RdPrefixes();
	result = true;
	return result;
}

#else
bool ChangeLMode(LockMode Mode, WORD Kind, bool RdPref)
{
	fileD->LMode = Mode;
	return true;
}
#endif

void OldLMode(FileD* fileD, LockMode Mode)
{
#ifdef FandSQL
	if (fileD->IsSQLFile) { fileD->LMode = Mode; return; }
#endif
	if (fileD->Handle == nullptr) return;
	if (Mode != fileD->LMode) ChangeLMode(fileD, Mode, 0, true);
}

void RunErrorM(LockMode Md, WORD N)
{
	OldLMode(CFile, Md);
	RunError(N);
}

void CloseClearHCFile()
{
	CloseClearH(&CFile->Handle);
	if (CFile->file_type == FileType::INDEX) CloseClearH(&CFile->XF->Handle);
	if (CFile->TF != nullptr) CloseClearH(&CFile->TF->Handle);
}

void CloseGoExit()
{
	CloseClearHCFile();
	GoExit();
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
	unsigned char ff[9]{ 0 };
	unsigned char rr[9]{ 0 };
	memcpy(ff + 1, FixNo, FLen); // zacneme na indexu 1 podle Pascal. kodu

	bool neg = ((ff[1] & 0x80) != 0); // zaporne cislo urcuje 1 bit
	if (neg) {
		if (ff[1] == 0x80) {
			for (size_t i = 2; i <= FLen; i++) {
				if (ff[i] != 0x00) break;
				return 0.0;
			}
		}
		for (size_t i = 1; i <= FLen; i++) ff[i] = ~ff[i];
		ff[FLen]++;
		WORD I = FLen;
		while (ff[I] == 0) {
			I--;
			if (I > 0) ff[I]++;
		}
	}
	integer first = 1;
	while (ff[first] == 0) first++;
	if (first > FLen) return 0;

	integer lef = 0;
	unsigned char b = ff[first];
	while ((b & 0x80) == 0) {
		b = b << 1;
		lef++;
	}
	ff[first] = ff[first] & (0x7F >> lef);
	integer exp = ((FLen - first) << 3) - lef + 1030;
	if (lef == 7) first++;
	lef = (lef + 5) & 0x07;
	integer rig = 8 - lef;
	integer i = 8 - 1; // velikost double - 1
	if ((rig <= 4) && (first <= FLen)) {
		rr[i] = ff[first] >> rig;
		i--;
	}
	while ((i > 0) && (first < FLen)) {
		rr[i] = (ff[first] << lef) + (ff[first + 1] >> rig);
		i--;
		first++;
	}
	if ((first == FLen) && (i > 0)) {
		unsigned char t = ff[first] << lef;
		rr[i] = t;
	}
	rr[DblS - 1] = (rr[DblS - 1] & 0x0F) + ((exp & 0x0F) << 4);
	rr[DblS] = exp >> 4;
	if (neg) rr[DblS] = rr[DblS] | 0x80;
	return *(double*)&rr[1];
}

void FixFromReal(double r, void* FixNo, WORD FLen)
{
	unsigned char ff[9]{ 0 };
	unsigned char rr[9]{ 0 };
	// memcpy(ff + 1, FixNo, FLen); // zacneme na indexu 1 podle Pascal. kodu
	if (r > 0) r += 0.5;
	else r -= 0.5;
	memcpy(rr + 1, &r, 8);
	bool neg = ((rr[8] & 0x80) != 0); // zaporne cislo urcuje 1 bit
	integer exp = (rr[8 - 1] >> 4) + (WORD)((rr[8] & 0x7F) << 4);
	if (exp < 2047)
	{
		rr[8] = 0;
		rr[8 - 1] = rr[8 - 1] & 0x0F;
		if (exp > 0) rr[8 - 1] = rr[8 - 1] | 0x10;
		else exp++;
		exp -= 1023;
		if (exp > (FLen << 3) - 1) return; // OVERFLOW
		longint lef = (exp + 4) & 0x0007;
		longint rig = 8 - lef;
		if ((exp & 0x0007) > 3) exp += 4;
		longint first = 7 - (exp >> 3);
		int i = FLen;
		while ((first < 8) && (i > 0)) {
			ff[i] = (rr[first] >> rig) + (rr[first + 1] << lef);
			i--;
			first++;
		}
		if (i > 0) ff[i] = rr[first] >> rig;
		if (neg) {
			for (i = 1; i <= FLen; i++) ff[i] = ~ff[i];
			ff[FLen]++;
			i = FLen;
			while (ff[i] == 0) {
				i--;
				if (i > 0) ff[i]++;
			}
		}
	}
	memcpy(FixNo, &ff[1], FLen);
}

bool TryLMode(FileD* fileD, LockMode Mode, LockMode& OldMode, WORD Kind)
{
	bool result = true;
#ifdef FandSQL
	if (fileD->IsSQLFile) {
		OldMode = fileD->LMode; if (Mode > fileD->LMode) fileD->LMode = Mode;
	}
	else
#endif
	{
		if (fileD->Handle == nullptr) OpenCreateF(Shared);
		OldMode = fileD->LMode;
		if (Mode > fileD->LMode) result = ChangeLMode(fileD, Mode, Kind, true);
	}
	return result;
}

LockMode NewLMode(FileD* fileD, LockMode Mode)
{
	LockMode md;
	TryLMode(fileD, Mode, md, 0);
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

	if (!CFile->IsShared()) return result; w = 0;
label1:
	if (!TryLockH(CFile->Handle, RecLock + N, 1)) {
		if (Kind != 2) {   /*0 Kind-wait, 1-wait until ESC, 2-no wait*/
			m = 826;
			if (N == 0) { SetCPathVol(); SetMsgPar(CPath, XTxt); m = 825; }
			w1 = PushWrLLMsg(m, Kind == 1);
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
	if ((CFile->Handle == nullptr) || !CFile->IsShared()) return;
	UnLockH(CFile->Handle, RecLock + N, 1);
#endif
}

void TestCPathError()
{
	WORD n;
	if (HandleError != 0) {
		n = 700 + HandleError;
		if ((n == 705) && (CPath[CPath.length()] == '\\')) n = 840;
		SetMsgPar(CPath);
		RunError(n);
	}
}

std::string CExtToX(const std::string dir, const std::string name, std::string ext)
{
	ext[1] = 'X';
	return dir + name + ext;
}

std::string CExtToT(const std::string dir, const std::string name, std::string ext)
{
	if (EquUpCase(ext, ".RDB")) ext = ".TTT";
	else if (EquUpCase(ext, ".DBF")) {
		if (CFile->TF->Format == TFile::FptFormat) {
			ext = ".FPT";
		}
		else {
			ext = ".DBT";
		}
	}
	else ext[1] = 'T';
	return dir + name + ext;
}

void NegateESDI()
{
	// asm  jcxz @2; @1:not es:[di].byte; inc di; loop @1; @2:
}

void RecallRec(longint RecNr)
{
	TestXFExist();
	CFile->XF->NRecs++;
	for (auto& K : CFile->Keys) {
		K->Insert(RecNr, false);
	}
	ClearDeletedFlag();
	CFile->WriteRec(RecNr, CRecPtr);
}

bool IsNullValue(void* p, WORD l)
{
	BYTE* pb = (BYTE*)p;
	for (size_t i = 0; i < l; i++) {
		if (pb[i] != 0xFF) return false;
	}
	return true;
}

// v CRecPtr vycte pozici zaznamu v .T00 souboru (ukazatel na zacatek textu)
longint _T(FieldDescr* F)
{
	return _T(F, (unsigned char*)CRecPtr, CFile->file_type);
}

longint _T(FieldDescr* F, unsigned char* data, FileType file_type)
{
	longint n = 0;
	integer err = 0;
	char* source = (char*)data + F->Displ;

	if (file_type == FileType::DBF) {
		// tváøíme se, že CRecPtr je pstring ...
		// TODO: toto je asi blbì, nutno opravit pøed 1. použitím
		pstring* s = (pstring*)CRecPtr;
		auto result = std::stoi(LeadChar(' ', *s));
		return result;
	}
	else {
		if (data == nullptr) return 0;
		return *(longint*)source;
	}
}

void T_(FieldDescr* F, longint Pos)
{
	// asi uklada data do CRecPtr
	pstring s;
	void* p = CRecPtr;
	char* source = (char*)p + F->Displ;
	longint* LP = (longint*)source;
	if ((F->field_type == FieldType::TEXT) && ((F->Flg & f_Stored) != 0)) {
		if (CFile->file_type == FileType::DBF) {
			if (Pos == 0) {
				FillChar(source, 10, ' ');
			}
			else {
				str(Pos, s);
				Move(&s[1], source, 10);
			}
		}
		else {
			*LP = Pos;
		}
	}
	else {
		RunError(906);
	}
}

void DelTFld(FieldDescr* F)
{
	longint n = _T(F);
	if (HasTWorkFlag()) {
		TWork.Delete(n);
	}
	else {
		LockMode md = NewLMode(CFile, WrMode);
		CFile->TF->Delete(n);
		OldLMode(CFile, md);
	}
	T_(F, 0);
}

void DelDifTFld(void* Rec, void* CompRec, FieldDescr* F)
{
	void* cr = CRecPtr;
	CRecPtr = CompRec;
	longint n = _T(F);
	CRecPtr = Rec;
	if (n != _T(F)) DelTFld(F);
	CRecPtr = cr;
}

void DelAllDifTFlds(void* Rec, void* CompRec)
{
	for (auto& F : CFile->FldD) {
		if (F->field_type == FieldType::TEXT && ((F->Flg & f_Stored) != 0)) DelDifTFld(Rec, CompRec, F);
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
	if (CFile->file_type == FileType::INDEX) SetUpdHandle(CFile->XF->Handle);
}

void DecNRecs(longint N)
{
	/* !!! with CFile^ do!!! */
	CFile->NRecs -= N;
	SetUpdHandle(CFile->Handle);
	if (CFile->file_type == FileType::INDEX) SetUpdHandle(CFile->XF->Handle);
	CFile->WasWrRec = true;
}

void SeekRec(FileD* fileD, longint N)
{
	fileD->IRec = N;
	if (fileD->XF == nullptr) fileD->Eof = N >= fileD->NRecs;
	else fileD->Eof = N >= fileD->XF->NRecs;
}

void PutRec(FileD* dataFile, void* recordData)
{
	/* !!! with CFile^ do!!! */
	dataFile->NRecs++;
	RdWrCache(WRITE, dataFile->Handle, dataFile->NotCached(),
		longint(dataFile->IRec) * dataFile->RecLen + dataFile->FrstDispl, dataFile->RecLen, recordData);
	dataFile->IRec++;
	dataFile->Eof = true;
}

void CreateRec(longint N)
{
	IncNRecs(1);
	void* cr = CRecPtr;
	CRecPtr = GetRecSpace();
	for (longint i = CFile->NRecs - 1; i >= N; i--) {
		CFile->ReadRec(i, CRecPtr);
		CFile->WriteRec(i + 1, CRecPtr);
	}
	ReleaseStore(CRecPtr);
	CRecPtr = cr;
	CFile->WriteRec(N, CRecPtr);
}

void DeleteRec(longint N)
{
	DelAllDifTFlds(CRecPtr, nullptr);
	for (longint i = N; i <= CFile->NRecs - 1; i++) {
		CFile->ReadRec(i + 1, CRecPtr);
		CFile->WriteRec(i, CRecPtr);
	}
	DecNRecs(1);
}

void ZeroAllFlds()
{
	FillChar(CRecPtr, CFile->RecLen, 0);
	for (auto& F : CFile->FldD) {
		if (((F->Flg & f_Stored) != 0) && (F->field_type == FieldType::ALFANUM)) S_(F, "");
	}
}

bool LinkLastRec(FileD* FD, longint& N, bool WithT)
{
	CFile = FD;
	CRecPtr = GetRecSpace();
	LockMode md = NewLMode(CFile, RdMode);
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
		else CFile->ReadRec(N, CRecPtr);
	}
	OldLMode(CFile, md);
	return result;
}

// ulozi hodnotu parametru do souboru
void AsgnParFldFrml(FileD* FD, FieldDescr* F, FrmlElem* Z, bool Ad)
{
	//#ifdef _DEBUG
	std::string FileName = FD->FullName;
	std::string Varible = F->Name;
	//#endif
	void* p = nullptr; longint N = 0; LockMode md; bool b = false;
	FileD* cf = CFile; void* cr = CRecPtr; CFile = FD;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		CRecPtr = GetRecSpace; ZeroAllFlds; AssgnFrml(F, Z, true, Ad);
		Strm1->UpdateXFld(nullptr, nullptr, F); ClearRecSpace(CRecPtr)
	}
	else
#endif
	{
		md = NewLMode(CFile, WrMode);
		if (!LinkLastRec(CFile, N, true)) {
			IncNRecs(1);
			CFile->WriteRec(N, CRecPtr);
		}
		AssgnFrml(F, Z, true, Ad);
		CFile->WriteRec(N, CRecPtr);
		OldLMode(CFile, md);
	}
	ReleaseStore(CRecPtr);
	CFile = cf; CRecPtr = cr;
}

//std::string UnPack(char* input, WORD length)
//{
//	std::string result;
//	size_t i = 0;
//
//	while (length > 0) {
//		char c = input[i++];
//		char c16 = c >> 4;
//		c16 += 0x30;
//		result += c16;
//		length--;
//		if (length == 0) break;
//		c16 = c;
//		c16 = c16 & 0x0F;
//		c16 += 0x30;
//		result += c16;
//		length--;
//	}
//
//	return result;
//}

double _RforD(FieldDescr* F, void* P)
{
	std::string s;
	integer err;
	double r = 0;
	s[0] = F->NBytes;
	Move(P, &s[1], s.length());
	switch (F->field_type) {
	case FieldType::FIXED: {
		ReplaceChar(s, ',', '.');
		if ((F->Flg & f_Comma) != 0) {
			size_t i = s.find('.');
			if (i != std::string::npos) s.erase(i, 1);
		}
		val(LeadChar(' ', TrailChar(s, ' ')), r, err);
		break;
	}
	case FieldType::DATE: {
		r = ValDate(s, "YYYYMMDD");
		break;
	}
	}
	return r;
}

/// Read BOOL from the record
bool _B(FieldDescr* F)
{
	bool result = false;
	void* p = CRecPtr;
	unsigned char* CP = (unsigned char*)p + F->Displ;
	if ((F->Flg & f_Stored) != 0) {
		if (CFile->file_type == FileType::DBF) result = *CP == 'Y' || *CP == 'y' || *CP == 'T' || *CP == 't';
		else if ((*CP == '\0') || (*CP == 0xFF)) result = false;
		else result = true;
	}
	else result = RunBool(F->Frml);
	return result;
}

/// Save BOOL into the record
void B_(FieldDescr* F, bool B)
{
	void* p = CRecPtr;
	char* pB = (char*)p + F->Displ;
	if ((F->field_type == FieldType::BOOL) && ((F->Flg & f_Stored) != 0)) {
		if (CFile->file_type == FileType::DBF)
		{
			if (B) *pB = 'T';
			else *pB = 'F';
		}
		else *pB = B;
	}
}

/// Read NUMBER from the record
double _R(FieldDescr* F)
{
	void* P = CRecPtr;
	char* source = (char*)P + F->Displ;
	double result = 0.0;
	double r;

	if ((F->Flg & f_Stored) != 0) {
		if (CFile->file_type == FileType::DBF) result = _RforD(F, source);
		else switch (F->field_type) {
		case FieldType::FIXED: { // FIX CISLO (M,N)
			r = RealFromFix(source, F->NBytes);
			if ((F->Flg & f_Comma) == 0) result = r / Power10[F->M];
			else result = r;
			break;
		}
		case FieldType::DATE: { // DATUM DD.MM.YY
			if (CFile->file_type == FileType::FAND8) {
				if (*(integer*)source == 0) result = 0.0;
				else result = *(integer*)source + FirstDate;
			}
			else goto label1;
			break;
		}
		case FieldType::REAL: {
		label1:
			if (P == nullptr) result = 0;
			else {
				result = Real48ToDouble(source);
			}
			break;
		}
		case FieldType::TEXT: {
			integer i = *(integer*)source;
			result = i;
			break;
		}
		}
	}
	else return RunReal(F->Frml);
	return result;
}

/// Save NUMBER to the record
void R_(FieldDescr* F, double R, void* record)
{
	BYTE* pRec = nullptr;
	pstring s; WORD m = 0; longint l = 0;
	if ((F->Flg & f_Stored) != 0) {
		if (record == nullptr) { pRec = (BYTE*)CRecPtr + F->Displ; }
		else { pRec = (BYTE*)record + F->Displ; }

		m = F->M;
		switch (F->field_type) {
		case FieldType::FIXED: {
			if (CFile->file_type == FileType::DBF) {
				if ((F->Flg & f_Comma) != 0) R = R / Power10[m];
				str(F->NBytes, s);
				Move(&s[1], pRec, F->NBytes);
			}
			else {
				if ((F->Flg & f_Comma) == 0) R = R * Power10[m];
				FixFromReal(R, pRec, F->NBytes);
			}
			break;
		}
		case FieldType::DATE: {
			switch (CFile->file_type) {
			case FileType::FAND8: {
				if (trunc(R) == 0) *(long*)&pRec = 0;
				else *(long*)pRec = trunc(R - FirstDate);
				break;
			}
			case FileType::DBF: {
				s = StrDate(R, "YYYYMMDD");
				Move(&s[1], pRec, 8);
				break;
			}
			default: {
				auto r48 = DoubleToReal48(R);
				for (size_t i = 0; i < 6; i++) {
					pRec[i] = r48[i];
				}
				break;
			}
			}
			break;
		}
		case FieldType::REAL: {
			auto r48 = DoubleToReal48(R);
			for (size_t i = 0; i < 6; i++) {
				pRec[i] = r48[i];
			}
			break;
		}
		}
	}
}

/// Read LONG STRING from the record
LongStr* _LongS(FieldDescr* F)
{
	std::string s = _StdS(F);

	LongStr* result = new LongStr(s.length());
	result->LL = s.length();
	memcpy(result->A, s.c_str(), s.length());

	return result;
}

/// Read PASCAL STRING from the record
pstring _ShortS(FieldDescr* F)
{
	void* P = CRecPtr;
	char* source = (char*)P + F->Displ;
	pstring S;
	if ((F->Flg & f_Stored) != 0) {
		WORD l = F->L;
		S[0] = l;
		switch (F->field_type) {
		case FieldType::ALFANUM:   	// znakovy retezec max. 255 znaku
		case FieldType::NUMERIC: {		// ciselny retezec max. 79 znaku
			if (F->field_type == FieldType::ALFANUM) {
				Move(source, &S[1], l);
				if ((F->Flg & f_Encryp) != 0) Code(&S[1], l);
				if (S[1] == '\0') memset(&S[1], ' ', l);
			}
			else if (IsNullValue(source, F->NBytes)) {
				FillChar(&S[0], l, ' ');
			}
			else {
				for (size_t i = 0; i < l; i++) {
					// kolikaty byte?
					size_t iB = i / 2;
					// leva nebo prava cislice?
					if (i % 2 == 0) {
						S[i + 1] = ((unsigned char)source[iB] >> 4) + 0x30;
					}
					else {
						S[i + 1] = (source[iB] & 0x0F) + 0x30;
					}
				}
			}
			break;
		}
		case FieldType::TEXT: {		// volny text max. 65k
			LongStr* ss = _LongS(F);
			if (ss->LL > 255) S = S.substr(0, 255);
			else S = S.substr(0, ss->LL);
			Move(&ss[0], &S[0], S.length());
			ReleaseStore(ss);
			break;
		}
		default:
			break;
		}
		return S;
	}
	return RunShortStr(F->Frml);
}

/// Read STD::STRING from the record
std::string _StdS(FieldDescr* F)
{
	void* P = CRecPtr;
	char* source = (char*)P + F->Displ;
	std::string S;
	longint Pos = 0; integer err = 0;
	LockMode md; WORD l = 0;
	if ((F->Flg & f_Stored) != 0) {
		l = F->L;
		switch (F->field_type)
		{
		case FieldType::ALFANUM:		// znakovy retezec max. 255 znaku
		case FieldType::NUMERIC: {		// ciselny retezec max. 79 znaku
			if (F->field_type == FieldType::ALFANUM) {
				S = std::string(source, l);
				if ((F->Flg & f_Encryp) != 0) Code(S);
				if (!S.empty() && S[0] == '\0') {
					S = RepeatString(' ', l);
				}
			}
			else if (IsNullValue(source, F->NBytes)) {
				S = RepeatString(' ', l);
			}
			else
			{
				//jedna je o typ N - prevedeme cislo na znaky
				//UnPack(P, S->A, l);
				for (BYTE i = 0; i < F->L; i++) {
					bool upper = (i % 2) == 0; // jde o "levou" cislici
					BYTE j = i / 2;
					if (upper) { S += ((BYTE)source[j] >> 4) + 0x30; }
					else { S += ((BYTE)source[j] & 0x0F) + 0x30; }
				}
				//S = UnPack(source, l);
			}
			break;
		}
		case FieldType::TEXT: { // volny text max. 65k
			if (HasTWorkFlag()) {
				LongStr* ls = TWork.Read(_T(F));
				S = std::string(ls->A, ls->LL);
				delete ls;
			}
			else {
				md = NewLMode(CFile, RdMode);
				LongStr* ls = CFile->TF->Read(_T(F));
				S = std::string(ls->A, ls->LL);
				delete ls;
				OldLMode(CFile, md);
			}
			if ((F->Flg & f_Encryp) != 0) Code(S);
			break;
		}
		}
		return S;
	}
	return RunStdStr(F->Frml);
}

/// Save LONG STRING to the record
void LongS_(FieldDescr* F, LongStr* S)
{
	// asi se vzdy uklada do souboru (nebo pracovniho souboru)
	// nakonec vola T_
	longint Pos; LockMode md;

	if ((F->Flg & f_Stored) != 0) {
		if (S->LL == 0) T_(F, 0);
		else {
			if ((F->Flg & f_Encryp) != 0) Code(S->A, S->LL);
#ifdef FandSQL
			if (CFile->IsSQLFile) { SetTWorkFlag; goto label1; }
			else
#endif
				if (HasTWorkFlag())
					label1:
			Pos = TWork.Store(S->A, S->LL);
				else {
					md = NewLMode(CFile, WrMode);
					Pos = CFile->TF->Store(S->A, S->LL);
					OldLMode(CFile, md);
				}
			if ((F->Flg & f_Encryp) != 0) Code(S->A, S->LL);
			T_(F, Pos);
		}
	}
}

/// Save STD::STRING to the record
void S_(FieldDescr* F, std::string S, void* record)
{
	const BYTE LeftJust = 1;
	BYTE* pRec = nullptr;

	if ((F->Flg & f_Stored) != 0) {
		if (record == nullptr) { pRec = (BYTE*)CRecPtr + F->Displ; }
		else { pRec = (BYTE*)record + F->Displ; }
		integer L = F->L;
		integer M = F->M;
		switch (F->field_type) {
		case FieldType::ALFANUM: {
			S = S.substr(0, F->L); // delka retezce je max. F->L
			if (M == LeftJust) {
				// doplnime mezery zprava
				memcpy(pRec, S.c_str(), S.length()); // probiha kontrola max. delky retezce
				memset(&pRec[S.length()], ' ', F->L - S.length());
			}
			else {
				// doplnime mezery zleva
				memset(pRec, ' ', F->L - S.length());
				memcpy(&pRec[F->L - S.length()], S.c_str(), S.length());
			}
			if ((F->Flg & f_Encryp) != 0) {
				Code(pRec, L);
			}
			break;
		}
		case FieldType::NUMERIC: {
			S = S.substr(0, F->L); // delka retezce je max. F->L
			BYTE tmpArr[80]{ 0 };
			if (M == LeftJust) {
				// doplnime nuly zprava
				memcpy(tmpArr, S.c_str(), S.length());
				memset(&tmpArr[S.length()], '0', F->L - S.length());
			}
			else {
				// doplnime nuly zleva
				memset(tmpArr, '0', F->L - S.length());
				memcpy(&tmpArr[F->L - S.length()], S.c_str(), S.length());
			}
			bool odd = F->L % 2 == 1; // lichy pocet znaku
			for (size_t i = 0; i < F->NBytes; i++) {
				if (odd && i == F->NBytes - 1) {
					pRec[i] = ((tmpArr[2 * i] - 0x30) << 4);
				}
				else {
					pRec[i] = ((tmpArr[2 * i] - 0x30) << 4) + (tmpArr[2 * i + 1] - 0x30);
				}
			}
			break;
		}
		case FieldType::TEXT: {
			LongStr* ss = CopyToLongStr(S);
			LongS_(F, ss);
			delete ss;
			break;
		}
		}
	}
}

// zrejme zajistuje pristup do jine tabulky (cizi klic)
bool LinkUpw(LinkD* LD, longint& N, bool WithT)
{
	FileD* ToFD = LD->ToFD;
	FileD* CF = CFile;
	void* CP = CRecPtr;
	XKey* K = LD->ToKey;

	XString x;
	x.PackKF(LD->Args);

	CFile = ToFD;
	void* RecPtr = GetRecSpace();
	CRecPtr = RecPtr;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		LU = Strm1->SelectXRec(K, @X, _equ, WithT); N = 1;
		if (LU) goto label2; else goto label1;
	}
#endif
	const LockMode md = NewLMode(CFile, RdMode);
	bool lu;
	if (ToFD->file_type == FileType::INDEX) {
		TestXFExist();
		lu = K->SearchInterval(x, false, N);
	}
	else if (CFile->NRecs == 0) {
		lu = false;
		N = 1;
	}
	else {
		lu = SearchKey(x, K, N);
	}

	if (lu) {
		CFile->ReadRec(N, CRecPtr);
	}
	else {
		bool b = false;
		double r = 0.0;
		ZeroAllFlds();
		const KeyFldD* KF = K->KFlds;
		for (auto& arg : LD->Args) {
			FieldDescr* F = arg->FldD;
			FieldDescr* F2 = KF->FldD;
			CFile = CF;
			CRecPtr = CP;
			if ((F2->Flg & f_Stored) != 0)
				switch (F->frml_type) {
				case 'S': {
					x.S = _ShortS(F);
					CFile = ToFD; CRecPtr = RecPtr;
					S_(F2, x.S);
					break;
				}
				case 'R': {
					r = _R(F);
					CFile = ToFD; CRecPtr = RecPtr;
					R_(F2, r);
					break;
				}
				case 'B': {
					b = _B(F);
					CFile = ToFD; CRecPtr = RecPtr;
					B_(F2, b);
					break;
				}
				}
			KF = KF->pChain;
		}
		CFile = ToFD;
		CRecPtr = RecPtr;
	}

	auto result = lu;
#ifdef FandSQL
	if (!CFile->IsSQLFile)
#endif
		OldLMode(CFile, md);
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
	md = NewLMode(CFile, DelMode);
	OldNRecs = CFile->NRecs;
	if (Add) N = N + OldNRecs;
	if ((N < 0) || (N == OldNRecs)) goto label1;
	if ((N == 0) && (CFile->TF != nullptr)) CFile->TF->SetEmpty();
	if (CFile->file_type == FileType::INDEX)
		if (N == 0) {
			CFile->NRecs = 0;
			SetUpdHandle(CFile->Handle);
			XFNotValid();
			goto label1;
		}
		else {
			SetMsgPar(CFile->Name);
			RunErrorM(md, 821);
		}
	if (N < OldNRecs) {
		DecNRecs(OldNRecs - N);
		goto label1;
	}
	CRecPtr = GetRecSpace();
	ZeroAllFlds();
	SetDeletedFlag();
	IncNRecs(N - OldNRecs);
	for (longint i = OldNRecs + 1; i < N; i++) {
		CFile->WriteRec(i, CRecPtr);
	}
	ReleaseStore(CRecPtr);
label1:
	OldLMode(CFile, md);
}

void ClearRecSpace(void* p)
{
	void* cr = nullptr;
	if (CFile->TF != nullptr) {
		cr = CRecPtr;
		CRecPtr = p;
		if (HasTWorkFlag()) {
			for (auto& f : CFile->FldD) {
				if (((f->Flg & f_Stored) != 0) && (f->field_type == FieldType::TEXT)) {
					TWork.Delete(_T(f));
					T_(f, 0);
				}
			}
		}
		CRecPtr = cr;
	}
}

void DelTFlds()
{
	for (auto& F : CFile->FldD) {
		if (((F->Flg & f_Stored) != 0) && (F->field_type == FieldType::TEXT)) {
			DelTFld(F);
		}
	}
}

void CopyRecWithT(void* p1, void* p2)
{
	memcpy(p2, p1, CFile->RecLen);
	for (auto& F : CFile->FldD) {
		if ((F->field_type == FieldType::TEXT) && ((F->Flg & f_Stored) != 0)) {
			TFile* tf1 = CFile->TF;
			TFile* tf2 = tf1;
			CRecPtr = p1;
			if ((tf1->Format != TFile::T00Format)) {
				LongStr* s = _LongS(F);
				CRecPtr = p2;
				LongS_(F, s);
				ReleaseStore(s);
			}
			else {
				if (HasTWorkFlag()) {
					tf1 = &TWork;
				}
				longint pos = _T(F);
				CRecPtr = p2;
				if (HasTWorkFlag()) {
					tf2 = &TWork;
				}
				pos = CopyTFString(tf2, CFile, tf1, pos);
				T_(F, pos);
			}
		}
	}
}

void Code(std::string& data)
{
	for (char& i : data) {
		i = (char)(i ^ 0xAA);
	}
}

void Code(void* A, WORD L)
{
	BYTE* pb = (BYTE*)A;
	for (int i = 0; i < L; i++) {
		pb[i] = pb[i] ^ 0xAA;
	}
}

LocVar* LocVarBlkD::GetRoot()
{
	if (this->vLocVar.size() > 0) return this->vLocVar[0];
	return nullptr;
}

LocVar* LocVarBlkD::FindByName(std::string Name)
{
	for (auto& i : vLocVar) {
		if (EquUpCase(Name, i->Name)) return i;
	}
	return nullptr;
}

bool DeletedFlag()  // r771 ASM
{
	if (CFile->file_type == FileType::INDEX) {
		if (((BYTE*)CRecPtr)[0] == 0) return false;
		else return true;
	}

	if (CFile->file_type == FileType::DBF) {
		if (((BYTE*)CRecPtr)[0] != '*') return false;
		else return true;
	}

	return false;
}

void ClearDeletedFlag()
{
	BYTE* ptr = (BYTE*)CRecPtr;
	switch (CFile->file_type) {
	case FileType::INDEX: { ptr[0] = 0; break; }
	case FileType::DBF: { ptr[0] = ' '; break; }
	}
}

void SetDeletedFlag()
{
	BYTE* ptr = (BYTE*)CRecPtr;
	switch (CFile->file_type) {
	case FileType::INDEX: { ptr[0] = 1; break; }
	case FileType::DBF: { ptr[0] = '*'; break; }
	}
}

integer CompStr(pstring& S1, pstring& S2)
{
	std::string s1 = S1;
	std::string s2 = S2;
	return CompStr(s1, s2);
}

int CompStr(std::string& S1, std::string& S2)
{
	size_t cmpLen = min(S1.length(), S2.length());
	for (size_t i = 0; i < cmpLen; i++) {
		if (S1[i] == S2[i]) {
			continue;
		}
		else {
			if ((BYTE)S1[i] < (BYTE)S2[i]) return 2; // _lt
			else return 4; // _gt
		}
	}
	if (S1.length() < S2.length()) return _lt;
	if (S1.length() > S2.length()) return _gt;
	return _equ;
}

WORD CmpLxStr(char* p1, WORD len1, char* p2, WORD len2)
{
	if (len1 > 0) {
		for (size_t i = len1 - 1; i > 0; i--) {
			if (p1[i] == ' ') { len1--; continue; }
			break;
		}
	}
	if (len2 > 0) {
		for (size_t i = len2 - 1; i > 0; i--) {
			if (p2[i] == ' ') { len1--; continue; }
			break;
		}
	}

	WORD cmpLen = min(len1, len2);
	for (size_t i = 0; i < cmpLen; i++) {
		if ((BYTE)p1[i] == (BYTE)p2[i]) continue;
		if ((BYTE)p1[i] < (BYTE)p2[i]) return _lt;
		return _gt;
	}
	if (len1 < len2) return _lt;
	if (len1 > len2) return _gt;
	return _equ;
}

WORD CompLexLongStr(LongStr* S1, LongStr* S2)
{
	WORD l1 = min(S1->LL, 256);
	char* b1 = new char[l1];
	WORD l2 = min(S2->LL, 256);
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[S1->A[i]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[S2->A[i]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

WORD CompLexLongShortStr(LongStr* S1, pstring& S2)
{
	WORD l1 = min(S1->LL, 256);
	char* b1 = new char[l1];
	WORD l2 = S2[0];
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[S1->A[i]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[S2[i + 1]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

WORD CompLexStr(const pstring& S1, const pstring& S2)
{
	WORD l1 = static_cast<pstring>(S1)[0];
	char* b1 = new char[l1];
	WORD l2 = static_cast<pstring>(S2)[0];
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[static_cast<pstring>(S1)[i + 1]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[static_cast<pstring>(S2)[i + 1]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

WORD CompLexStr(pstring& S1, pstring& S2)
{
	WORD l1 = S1[0];
	char* b1 = new char[l1];
	WORD l2 = S2[0];
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[S1[i + 1]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[S2[i + 1]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

WORD CompLexStrings(const std::string& S1, const std::string& S2)
{
	WORD l1 = S1.length();
	char* b1 = new char[l1];
	WORD l2 = S2.length();
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[(BYTE)S1[i]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[(BYTE)S2[i]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

void* GetRecSpace()
{
	size_t length = CFile->RecLen + 2;
	void* result = new BYTE[length];
	memset(result, '\0', length);
	return result;
}

//void* GetRecSpace2()
//{
//	return GetZStore2(CFile->RecLen + 2);
//}

WORD CFileRecSize()
{
	return CFile->RecLen;
}

void SetTWorkFlag()
{
	BYTE* p = (BYTE*)CRecPtr;
	p[CFile->RecLen] = 1;
}

bool HasTWorkFlag()
{
	BYTE* p = (BYTE*)CRecPtr;
	const bool workFlag = p[CFile->RecLen] == 1;
	return workFlag;
}

void SetUpdFlag()
{
	BYTE* p = (BYTE*)CRecPtr;
	p[CFile->RecLen + 1] = 1;
}

void ClearUpdFlag()
{
	BYTE* p = (BYTE*)CRecPtr;
	p[CFile->RecLen + 1] = 0;
}

bool HasUpdFlag()
{
	BYTE* p = (BYTE*)CRecPtr;
	return p[CFile->RecLen + 1] == 1;
}

void* LocVarAd(LocVar* LV)
{
	return nullptr;
}

void rol(BYTE& input, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		input = input << 1 | input >> 7;
	}
}

void XDecode(LongStr* origS)
{
	BYTE RMask = 0;
	WORD* AX = new WORD();
	BYTE* AL = (BYTE*)AX;
	BYTE* AH = AL + 1;

	WORD* BX = new WORD();
	BYTE* BL = (BYTE*)BX;
	BYTE* BH = BL + 1;

	WORD* CX = new WORD();
	BYTE* CL = (BYTE*)CX;
	BYTE* CH = CL + 1;

	WORD* DX = new WORD();
	BYTE* DL = (BYTE*)DX;
	BYTE* DH = DL + 1;

	if (origS->LL == 0) return;
	BYTE* S = new BYTE[origS->LL + 2];
	WORD* len = (WORD*)&S[0];
	*len = origS->LL;
	memcpy(&S[2], &origS->A[0], origS->LL);

	WORD offset = 0;

	BYTE* DS = S; // ukazuje na delku pred retezcem
	BYTE* ES = &S[2]; // ukazuje na zacatek retezce
	WORD SI = 0;
	*BX = SI;
	*AX = *(WORD*)&DS[SI];
	if (*AX == 0) return;
	SI = 2;
	WORD DI = 0;
	*BX += *AX;
	*AX = *(WORD*)&S[*BX];
	*AX = *AX ^ 0xCCCC;
	SI += *AX;
	(*BX)--;
	*CL = S[*BX];
	*CL = *CL & 3;
	*AL = 0x9C;
	rol(*AL, *CL);
	RMask = *AL;
	*CH = 0;

label1:
	*AL = DS[SI]; SI++; // lodsb
	*DH = 0xFF;
	*DL = *AL;

label2:
	if (SI >= *BX) goto label4;
	if ((*DH & 1) == 0) goto label1;
	if ((*DL & 1) != 0) goto label3;
	*AL = DS[SI]; SI++; // lodsb
	rol(RMask, 1);
	*AL = *AL ^ RMask;
	ES[DI] = *AL; DI++; // stosb
	*DX = *DX >> 1;
	goto label2;

label3:
	*AL = DS[SI]; SI++; // lodsb
	*CL = *AL;
	*AX = *(WORD*)&DS[SI]; SI++; SI++; // lodsw
	offset = *AX;
	for (size_t i = 0; i < *CX; i++) { // rep
		ES[DI] = DS[offset]; offset++; DI++; // movsb
	}
	*DX = *DX >> 1;
	goto label2;

label4:
	origS->LL = DI;
	memcpy(origS->A, &S[2], DI);

	/**AX = DI;
	DI = *len;
	*AX -= DI;
	*AX -= 2;
	ES[DI] = *AL; DI++;
	ES[DI] = *AH; DI++;*/

	delete AX; delete BX; delete CX; delete DX;
	delete[] S;
}

void CodingLongStr(LongStr* S)
{
	if (CFile->TF->LicenseNr == 0) Code(S->A, S->LL);
	else XDecode(S); // musí mít o 2B víc - saha si tam XDecode!!!
}

void DirMinusBackslash(pstring& D)
{
	if ((D.length() > 3) && (D[D.length() - 1] == '\\')) D[0]--;
}

longint StoreInTWork(LongStr* S)
{
	return TWork.Store(S->A, S->LL);
}

LongStr* ReadDelInTWork(longint Pos)
{
	auto result = TWork.Read(Pos);
	TWork.Delete(Pos);
	return result;
}

void ForAllFDs(void(*procedure)())
{
	FileD* cf = CFile;
	RdbD* R = CRdb;
	while (R != nullptr) {
		CFile = R->FD;
		while (CFile != nullptr) {
			procedure();
			CFile = CFile->pChain;
		}
		R = R->ChainBack;
	}
	CFile = cf;
}

bool IsActiveRdb(FileD* FD)
{
	RdbD* R = CRdb;
	while (R != nullptr) {
		if (FD == R->FD) return true;
		R = R->ChainBack;
	}
	return false;
}

void ResetCompilePars()
{
	RdFldNameFrml = RdFldNameFrmlF;
	RdFunction = nullptr;
	ChainSumEl = nullptr;
	FileVarsAllowed = true;
	FDLocVarAllowed = false;
	IdxLocVarAllowed = false;
	PrevCompInp.clear();
}

std::string TranslateOrd(std::string text)
{
	std::string trans;
	for (size_t i = 0; i < text.length(); i++) {
		char c = CharOrdTab[(BYTE)text[i]];
#ifndef FandAng
		if (c == 0x49 && trans.length() > 0) { // znak 'H'
			if (trans[trans.length() - 1] == 0x43) { // posledni znak ve vystupnim retezci je 'C' ?
				trans[trans.length() - 1] = 0x4A; // na vstupu bude 'J' jako 'CH'
				continue;
			}
		}
#endif
		trans += c;
	}
	return trans;
}
