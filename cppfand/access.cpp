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
#include "../fandio/FandFile.h"
#include "../Logging/Logging.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"

void ResetCFileUpdH()
{
	ResetUpdHandle(CFile->FF->Handle);
	if (CFile->FF->file_type == FileType::INDEX) ResetUpdHandle(CFile->FF->XF->Handle);
	if (CFile->FF->TF != nullptr) ResetUpdHandle(CFile->FF->TF->Handle);
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



void CloseClearHCFile(FandFile* fand_file)
{
	CloseClearH(&fand_file->Handle);
	if (fand_file->file_type == FileType::INDEX) {
		CloseClearH(&fand_file->XF->Handle);
	}
	if (fand_file->TF != nullptr) {
		CloseClearH(&fand_file->TF->Handle);
	}
}

void CloseGoExit(FandFile* fand_file)
{
	CloseClearHCFile(fand_file);
	GoExit();
}

BYTE ByteMask[_MAX_INT_DIG];

const BYTE DblS = 8;
const BYTE FixS = 8;
BYTE Fix[FixS];
BYTE RealMask[DblS + 1];
BYTE Dbl[DblS];

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
	short first = 1;
	while (ff[first] == 0) first++;
	if (first > FLen) return 0;

	short lef = 0;
	unsigned char b = ff[first];
	while ((b & 0x80) == 0) {
		b = b << 1;
		lef++;
	}
	ff[first] = ff[first] & (0x7F >> lef);
	short exp = ((FLen - first) << 3) - lef + 1030;
	if (lef == 7) first++;
	lef = (lef + 5) & 0x07;
	short rig = 8 - lef;
	short i = 8 - 1; // velikost double - 1
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
	short exp = (rr[8 - 1] >> 4) + (WORD)((rr[8] & 0x7F) << 4);
	if (exp < 2047)
	{
		rr[8] = 0;
		rr[8 - 1] = rr[8 - 1] & 0x0F;
		if (exp > 0) rr[8 - 1] = rr[8 - 1] | 0x10;
		else exp++;
		exp -= 1023;
		if (exp > (FLen << 3) - 1) return; // OVERFLOW
		int lef = (exp + 4) & 0x0007;
		int rig = 8 - lef;
		if ((exp & 0x0007) > 3) exp += 4;
		int first = 7 - (exp >> 3);
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

void NegateESDI()
{
	// asm  jcxz @2; @1:not es:[di].byte; inc di; loop @1; @2:
}

bool IsNullValue(void* p, WORD l)
{
	BYTE* pb = (BYTE*)p;
	for (size_t i = 0; i < l; i++) {
		if (pb[i] != 0xFF) return false;
	}
	return true;
}


void T_(FieldDescr* F, int Pos)
{
	// asi uklada data do CRecPtr
	pstring s;
	void* p = CRecPtr;
	char* source = (char*)p + F->Displ;
	int* LP = (int*)source;
	if ((F->field_type == FieldType::TEXT) && ((F->Flg & f_Stored) != 0)) {
		if (CFile->FF->file_type == FileType::DBF) {
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
	int n = CFile->_T(F, CRecPtr);
	if (HasTWorkFlag(CFile->FF, CRecPtr)) {
		TWork.Delete(n);
	}
	else {
		LockMode md = CFile->NewLockMode(WrMode);
		CFile->FF->TF->Delete(n);
		CFile->OldLockMode(md);
	}
	T_(F, 0);
}

void DelDifTFld(void* Rec, void* CompRec, FieldDescr* F)
{
	void* cr = CRecPtr;
	CRecPtr = CompRec;
	int n = CFile->_T(F, CRecPtr);
	CRecPtr = Rec;
	if (n != CFile->_T(F, CRecPtr)) DelTFld(F);
	CRecPtr = cr;
}

const WORD Alloc = 2048;
const double FirstDate = 6.97248E+5;


void ZeroAllFlds(FileD* file_d, void* record)
{
	FillChar(record, file_d->FF->RecLen, 0);
	for (auto& F : file_d->FldD) {
		if (((F->Flg & f_Stored) != 0) && (F->field_type == FieldType::ALFANUM)) {
			S_(CFile, F, "");
		}
	}
}

bool LinkLastRec(FileD* FD, int& N, bool WithT)
{
	CFile = FD;
	CRecPtr = FD->GetRecSpace();
	LockMode md = CFile->NewLockMode(RdMode);
	auto result = true;
#ifdef FandSQL
	if (FD->IsSQLFile)
	{
		if (Strm1->SelectXRec(nullptr, nullptr, _equ, WithT)) N = 1; else goto label1;
	}
	else
#endif
	{
		N = CFile->FF->NRecs;
		if (N == 0) {
		label1:
			ZeroAllFlds(CFile, CRecPtr);
			result = false;
			N = 1;
		}
		else CFile->ReadRec(N, CRecPtr);
	}
	CFile->OldLockMode(md);
	return result;
}

// ulozi hodnotu parametru do souboru
void AsgnParFldFrml(FileD* FD, FieldDescr* F, FrmlElem* Z, bool Ad)
{
	//#ifdef _DEBUG
	std::string FileName = FD->FullPath;
	std::string Varible = F->Name;
	//#endif
	void* p = nullptr; int N = 0; LockMode md; bool b = false;
	FileD* cf = CFile; void* cr = CRecPtr; CFile = FD;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		CRecPtr = GetRecSpace; ZeroAllFlds; AssgnFrml(F, Z, true, Ad);
		Strm1->UpdateXFld(nullptr, nullptr, F); ClearRecSpace(CRecPtr)
	}
	else
#endif
	{
		md = CFile->NewLockMode(WrMode);
		if (!LinkLastRec(CFile, N, true)) {
			CFile->IncNRecs(1);
			CFile->WriteRec(N, CRecPtr);
		}
		AssgnFrml(CFile, CRecPtr, F, Z, true, Ad);
		CFile->WriteRec(N, CRecPtr);
		CFile->OldLockMode(md);
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
	short err;
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
		if (CFile->FF->file_type == FileType::DBF) result = *CP == 'Y' || *CP == 'y' || *CP == 'T' || *CP == 't';
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
		if (CFile->FF->file_type == FileType::DBF)
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
		if (CFile->FF->file_type == FileType::DBF) result = _RforD(F, source);
		else switch (F->field_type) {
		case FieldType::FIXED: { // FIX CISLO (M,N)
			r = RealFromFix(source, F->NBytes);
			if ((F->Flg & f_Comma) == 0) result = r / Power10[F->M];
			else result = r;
			break;
		}
		case FieldType::DATE: { // DATUM DD.MM.YY
			if (CFile->FF->file_type == FileType::FAND8) {
				if (*(short*)source == 0) result = 0.0;
				else result = *(short*)source + FirstDate;
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
			short i = *(short*)source;
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
	pstring s; WORD m = 0; int l = 0;
	if ((F->Flg & f_Stored) != 0) {
		if (record == nullptr) { pRec = (BYTE*)CRecPtr + F->Displ; }
		else { pRec = (BYTE*)record + F->Displ; }

		m = F->M;
		switch (F->field_type) {
		case FieldType::FIXED: {
			if (CFile->FF->file_type == FileType::DBF) {
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
			switch (CFile->FF->file_type) {
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
	std::string s = _StdS(F, CRecPtr);

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
std::string _StdS(FieldDescr* F, void* record)
{
	char* source = (char*)record + F->Displ;
	std::string S;
	int Pos = 0; short err = 0;
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
			if (HasTWorkFlag(CFile->FF, CRecPtr)) {
				LongStr* ls = TWork.Read(CFile->_T(F, CRecPtr));
				S = std::string(ls->A, ls->LL);
				delete ls;
			}
			else {
				md = CFile->NewLockMode(RdMode);
				LongStr* ls = CFile->FF->TF->Read(CFile->_T(F, CRecPtr));
				S = std::string(ls->A, ls->LL);
				delete ls;
				CFile->OldLockMode(md);
			}
			if ((F->Flg & f_Encryp) != 0) {
				Code(S);
			}
			break;
		}
		}
		return S;
	}
	return RunStdStr(F->Frml);
}

/// Save LONG STRING to the record
void LongS_(FileD* file_d, FieldDescr* F, LongStr* S)
{
	// asi se vzdy uklada do souboru (nebo pracovniho souboru)
	// nakonec vola T_
	int Pos; LockMode md;

	if ((F->Flg & f_Stored) != 0) {
		if (S->LL == 0) T_(F, 0);
		else {
			if ((F->Flg & f_Encryp) != 0) Code(S->A, S->LL);
#ifdef FandSQL
			if (file_d->IsSQLFile) { SetTWorkFlag; goto label1; }
			else
#endif
				if (HasTWorkFlag(file_d->FF, CRecPtr))
					label1:
			Pos = TWork.Store(S->A, S->LL);
				else {
					md = CFile->NewLockMode(WrMode);
					Pos = file_d->FF->TF->Store(S->A, S->LL);
					CFile->OldLockMode(md);
				}
			if ((F->Flg & f_Encryp) != 0) Code(S->A, S->LL);
			T_(F, Pos);
		}
	}
}

/// Save STD::STRING to the record
void S_(FileD* file_d, FieldDescr* F, std::string S, void* record)
{
	const BYTE LeftJust = 1;
	BYTE* pRec = nullptr;

	if ((F->Flg & f_Stored) != 0) {
		if (record == nullptr) { pRec = (BYTE*)CRecPtr + F->Displ; }
		else { pRec = (BYTE*)record + F->Displ; }
		short L = F->L;
		short M = F->M;
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
			LongS_(file_d, F, ss);
			delete ss;
			break;
		}
		}
	}
}

// zrejme zajistuje pristup do jine tabulky (cizi klic)
bool LinkUpw(LinkD* LD, int& N, bool WithT)
{
	FileD* ToFD = LD->ToFD;
	FileD* CF = CFile;
	void* CP = CRecPtr;
	XKey* K = LD->ToKey;

	XString x;
	x.PackKF(LD->Args);

	CFile = ToFD;
	void* RecPtr = CFile->GetRecSpace();
	CRecPtr = RecPtr;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		LU = Strm1->SelectXRec(K, @X, _equ, WithT); N = 1;
		if (LU) goto label2; else goto label1;
	}
#endif
	const LockMode md = CFile->NewLockMode(RdMode);
	bool lu;
	if (ToFD->FF->file_type == FileType::INDEX) {
		TestXFExist();
		lu = K->SearchInterval(x, false, N);
	}
	else if (CFile->FF->NRecs == 0) {
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
		ZeroAllFlds(CFile, CRecPtr);
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
					S_(CFile, F2, x.S);
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
		CFile->OldLockMode(md);
	return result;
}

void AssignNRecs(bool Add, int N)
{
	int OldNRecs; LockMode md;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		if ((N = 0) && !Add) Strm1->DeleteXRec(nullptr, nullptr, false); return;
	}
#endif
	md = CFile->NewLockMode(DelMode);
	OldNRecs = CFile->FF->NRecs;
	if (Add) N = N + OldNRecs;
	if ((N < 0) || (N == OldNRecs)) goto label1;
	if ((N == 0) && (CFile->FF->TF != nullptr)) CFile->FF->TF->SetEmpty();
	if (CFile->FF->file_type == FileType::INDEX) {
		if (N == 0) {
			CFile->FF->NRecs = 0;
			SetUpdHandle(CFile->FF->Handle);
			XFNotValid();
			goto label1;
		}
		else {
			SetMsgPar(CFile->Name);
			CFile->RunErrorM(md);
			RunError(821);
		}
	}
	if (N < OldNRecs) {
		CFile->DecNRecs(OldNRecs - N);
		goto label1;
	}
	CRecPtr = CFile->GetRecSpace();
	ZeroAllFlds(CFile, CRecPtr);
	SetDeletedFlag(CFile->FF, CRecPtr);
	CFile->IncNRecs(N - OldNRecs);
	for (int i = OldNRecs + 1; i <= N; i++) {
		CFile->WriteRec(i, CRecPtr);
	}
	ReleaseStore(CRecPtr);
label1:
	CFile->OldLockMode(md);
}

void ClearRecSpace(void* p)
{
	void* cr = nullptr;
	if (CFile->FF->TF != nullptr) {
		cr = CRecPtr;
		CRecPtr = p;
		if (HasTWorkFlag(CFile->FF, CRecPtr)) {
			for (auto& f : CFile->FldD) {
				if (((f->Flg & f_Stored) != 0) && (f->field_type == FieldType::TEXT)) {
					TWork.Delete(CFile->_T(f, CRecPtr));
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
	memcpy(p2, p1, CFile->FF->RecLen);
	for (auto& F : CFile->FldD) {
		if ((F->field_type == FieldType::TEXT) && ((F->Flg & f_Stored) != 0)) {
			FandTFile* tf1 = CFile->FF->TF;
			FandTFile* tf2 = tf1;
			CRecPtr = p1;
			if ((tf1->Format != FandTFile::T00Format)) {
				LongStr* s = _LongS(F);
				CRecPtr = p2;
				LongS_(CFile, F, s);
				ReleaseStore(s);
			}
			else {
				if (HasTWorkFlag(CFile->FF, CRecPtr)) {
					tf1 = &TWork;
				}
				int pos = CFile->_T(F, CRecPtr);
				CRecPtr = p2;
				if (HasTWorkFlag(CFile->FF, CRecPtr)) {
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

bool DeletedFlag(FandFile* fand_file, void* record)
{
	if (fand_file->file_type == FileType::INDEX) {
		if (((BYTE*)record)[0] == 0) return false;
		else return true;
	}

	if (fand_file->file_type == FileType::DBF) {
		if (((BYTE*)record)[0] != '*') return false;
		else return true;
	}

	return false;
}

void ClearDeletedFlag(FandFile* fand_file, void* record)
{
	BYTE* ptr = (BYTE*)record;
	switch (fand_file->file_type) {
	case FileType::INDEX: { ptr[0] = 0; break; }
	case FileType::DBF: { ptr[0] = ' '; break; }
	}
}

void SetDeletedFlag(FandFile* fand_file, void* record)
{
	BYTE* ptr = (BYTE*)record;
	switch (fand_file->file_type) {
	case FileType::INDEX: { ptr[0] = 1; break; }
	case FileType::DBF: { ptr[0] = '*'; break; }
	}
}

void SetTWorkFlag(FandFile* fand_file, void* record)
{
	BYTE* p = (BYTE*)record;
	p[fand_file->RecLen] = 1;
}

bool HasTWorkFlag(FandFile* fand_file, void* record)
{
	BYTE* p = (BYTE*)record;
	const bool workFlag = p[fand_file->RecLen] == 1;
	return workFlag;
}

void SetUpdFlag(FandFile* fand_file, void* record)
{
	BYTE* p = (BYTE*)record;
	p[fand_file->RecLen + 1] = 1;
}

void ClearUpdFlag(FandFile* fand_file, void* record)
{
	BYTE* p = (BYTE*)record;
	p[fand_file->RecLen + 1] = 0;
}

bool HasUpdFlag(FandFile* fand_file, void* record)
{
	BYTE* p = (BYTE*)record;
	return p[fand_file->RecLen + 1] == 1;
}

void* LocVarAd(LocVar* LV)
{
	return nullptr;
}

void rotateByteLeft(BYTE& input, size_t count)
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

	BYTE* DS = S;     // ukazuje na delku pred retezcem
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
	rotateByteLeft(*AL, *CL);
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
	rotateByteLeft(RMask, 1);
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

	delete AX; delete BX; delete CX; delete DX;
	delete[] S;
}

void CodingLongStr(LongStr* S)
{
	if (CFile->FF->TF->LicenseNr == 0) Code(S->A, S->LL);
	else XDecode(S); // musi mit o 2B vic - saha si tam XDecode!!!
}

void DirMinusBackslash(pstring& D)
{
	if ((D.length() > 3) && (D[D.length() - 1] == '\\')) D[0]--;
}

int StoreInTWork(LongStr* S)
{
	return TWork.Store(S->A, S->LL);
}

LongStr* ReadDelInTWork(int Pos)
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
		if (c == 0x49 && trans.length() > 0) {       // znak 'H'
			if (trans[trans.length() - 1] == 0x43) { // posledni znak ve vystupnim retezci je 'C' ?
				trans[trans.length() - 1] = 0x4A;    // na vstupu bude 'J' jako 'CH'
				continue;
			}
		}
#endif
		trans += c;
	}
	return trans;
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
		if (CFile->FF->TF->Format == FandTFile::FptFormat) {
			ext = ".FPT";
		}
		else {
			ext = ".DBT";
		}
	}
	else ext[1] = 'T';
	return dir + name + ext;
}