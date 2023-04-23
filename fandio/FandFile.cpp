#include "FandFile.h"

#include "DbfFile.h"
#include "../Common/textfunc.h"
#include "../cppfand/Coding.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/runfrml.h"
#include "../pascal/real48.h"

FandFile::FandFile()
{
}

FandFile::FandFile(const FandFile& orig)
{
	RecLen = orig.RecLen;
	file_type = orig.file_type;
	Drive = orig.Drive;

	if (orig.TF != nullptr) TF = new FandTFile(*orig.TF);
	if (orig.XF != nullptr) XF = new FandXFile(*orig.XF);
}


int FandFile::UsedFileSize()
{
	int n = int(NRecs) * RecLen + FrstDispl;
	if (file_type == FileType::DBF) n++;
	return n;
}

bool FandFile::IsShared()
{
	return (UMode == Shared) || (UMode == RdShared);
}

bool FandFile::NotCached()
{
	if (UMode == Shared) goto label1;
	if (UMode != RdShared) return false;
label1:
	if (LMode == ExclMode) return false;
	return true;
}

bool FandFile::Cached()
{
	return !NotCached();
}

void FandFile::Reset()
{
	RecLen = 0;
	RecPtr = nullptr;
	NRecs = 0;
	FrstDispl = 0;
	WasWrRec = false; WasRdOnly = false; Eof = false;
	file_type = FileType::UNKNOWN;        // 8= Fand 8; 6= Fand 16; X= .X; 0= RDB; C= CAT 
	Handle = nullptr;
	TF = nullptr;
	Drive = 0;         // 1=A, 2=B, else 0
	UMode = FileUseMode::Closed;
	LMode = NullMode; ExLMode = NullMode; TaLMode = NullMode;
	XF = nullptr;
}

void FandFile::IncNRecs(int n)
{
#ifdef FandDemo
	if (NRecs > 100) RunError(884);
#endif
	NRecs += n;
	SetUpdHandle(Handle);
	if (file_type == FileType::INDEX) {
		SetUpdHandle(XF->Handle);
	}
}

void FandFile::DecNRecs(int n)
{
	NRecs -= n;
	SetUpdHandle(Handle);
	if (file_type == FileType::INDEX) SetUpdHandle(XF->Handle);
	WasWrRec = true;
}

void FandFile::PutRec(void* record, int& i_rec)
{
	NRecs++;
	RdWrCache(WRITE, Handle, NotCached(),i_rec * RecLen + FrstDispl, RecLen, record);
	i_rec++;
	Eof = true;
}

size_t FandFile::RecordSize()
{
	return RecLen;
}

bool FandFile::loadB(FieldDescr* field_d, void* record)
{
	bool result = false;
	unsigned char* CP = (unsigned char*)record + field_d->Displ;
	if ((field_d->Flg & f_Stored) != 0) {
		if (CFile->FF->file_type == FileType::DBF) {
			result = *CP == 'Y' || *CP == 'y' || *CP == 'T' || *CP == 't';
		}
		else if ((*CP == '\0') || (*CP == 0xFF)) {
			result = false;
		}
		else result = true;
	}
	else {
		result = RunBool(field_d->Frml);
	}
	return result;
}

double FandFile::loadR(FieldDescr* field_d, void* record)
{
	char* source = (char*)record + field_d->Displ;
	double result = 0.0;
	double r;

	if ((field_d->Flg & f_Stored) != 0) {
		if (CFile->FF->file_type == FileType::DBF) result = _RforD(field_d, source);
		else switch (field_d->field_type) {
		case FieldType::FIXED: { // FIX CISLO (M,N)
			r = RealFromFix(source, field_d->NBytes);
			if ((field_d->Flg & f_Comma) == 0) result = r / Power10[field_d->M];
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
			if (record == nullptr) result = 0;
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
	else return RunReal(field_d->Frml);
	return result;
}

int FandFile::loadT(FieldDescr* F, void* record)
{
	int n = 0;
	short err = 0;
	char* source = (char*)record + F->Displ;

	if (file_type == FileType::DBF) {
		// tvarime se, ze CRecPtr je pstring ...
		// TODO: toto je asi blbe, nutno opravit pred 1. pouzitim
		//pstring* s = (pstring*)CRecPtr;
		//auto result = std::stoi(LeadChar(' ', *s));
		return 0; // result;
	}
	else {
		if (record == nullptr) return 0;
		return *(int*)source;
	}
}

void FandFile::saveB(FieldDescr* field_d, bool b, void* record)
{
	char* pB = (char*)record + field_d->Displ;
	if ((field_d->field_type == FieldType::BOOL) && ((field_d->Flg & f_Stored) != 0)) {
		if (file_type == FileType::DBF) {
			if (b) *pB = 'T';
			else *pB = 'F';
		}
		else *pB = b ? 1 : 0;
	}
}

void FandFile::saveR(FieldDescr* field_d, double r, void* record)
{
	BYTE* pRec = (BYTE*)record + field_d->Displ;
	int l = 0;
	if ((field_d->Flg & f_Stored) != 0) {
		WORD m = field_d->M;
		switch (field_d->field_type) {
		case FieldType::FIXED: {
			if (file_type == FileType::DBF) {
				pstring s;
				if ((field_d->Flg & f_Comma) != 0) r = r / Power10[m];
				str(field_d->NBytes, s);
				Move(&s[1], pRec, field_d->NBytes);
			}
			else {
				if ((field_d->Flg & f_Comma) == 0) r = r * Power10[m];
				FixFromReal(r, pRec, field_d->NBytes);
			}
			break;
		}
		case FieldType::DATE: {
			switch (file_type) {
			case FileType::FAND8: {
				if (trunc(r) == 0) *(long*)&pRec = 0;
				else *(long*)pRec = trunc(r - FirstDate);
				break;
			}
			case FileType::DBF: {
				pstring s = StrDate(r, "YYYYMMDD");
				Move(&s[1], pRec, 8);
				break;
			}
			default: {
				auto r48 = DoubleToReal48(r);
				for (size_t i = 0; i < 6; i++) {
					pRec[i] = r48[i];
				}
				break;
			}
			}
			break;
		}
		case FieldType::REAL: {
			auto r48 = DoubleToReal48(r);
			for (size_t i = 0; i < 6; i++) {
				pRec[i] = r48[i];
			}
			break;
		}
		}
	}
}

void FandFile::saveS(FileD* parent, FieldDescr* field_d, std::string s, void* record)
{
	const BYTE LeftJust = 1;
	BYTE* pRec = (BYTE*)record + field_d->Displ;

	if ((field_d->Flg & f_Stored) != 0) {
		short L = field_d->L;
		short M = field_d->M;
		switch (field_d->field_type) {
		case FieldType::ALFANUM: {
			s = s.substr(0, field_d->L); // delka retezce je max. field_d->L
			if (M == LeftJust) {
				// doplnime mezery zprava
				memcpy(pRec, s.c_str(), s.length()); // probiha kontrola max. delky retezce
				memset(&pRec[s.length()], ' ', field_d->L - s.length());
			}
			else {
				// doplnime mezery zleva
				memset(pRec, ' ', field_d->L - s.length());
				memcpy(&pRec[field_d->L - s.length()], s.c_str(), s.length());
			}
			if ((field_d->Flg & f_Encryp) != 0) {
				Coding::Code(pRec, L);
			}
			break;
		}
		case FieldType::NUMERIC: {
			s = s.substr(0, field_d->L); // delka retezce je max. field_d->L
			BYTE tmpArr[80]{ 0 };
			if (M == LeftJust) {
				// doplnime nuly zprava
				memcpy(tmpArr, s.c_str(), s.length());
				memset(&tmpArr[s.length()], '0', field_d->L - s.length());
			}
			else {
				// doplnime nuly zleva
				memset(tmpArr, '0', field_d->L - s.length());
				memcpy(&tmpArr[field_d->L - s.length()], s.c_str(), s.length());
			}
			bool odd = field_d->L % 2 == 1; // lichy pocet znaku
			for (size_t i = 0; i < field_d->NBytes; i++) {
				if (odd && i == field_d->NBytes - 1) {
					pRec[i] = ((tmpArr[2 * i] - 0x30) << 4);
				}
				else {
					pRec[i] = ((tmpArr[2 * i] - 0x30) << 4) + (tmpArr[2 * i + 1] - 0x30);
				}
			}
			break;
		}
		case FieldType::TEXT: {
			LongStr* ss = CopyToLongStr(s);
			saveLongS(parent, field_d, ss, record);
			delete ss;
			break;
		}
		}
	}
}

void FandFile::saveLongS(FileD* parent, FieldDescr* field_d, LongStr* ls, void* record)
{
	// asi se vzdy uklada do souboru (nebo pracovniho souboru)
	// nakonec vola saveT
	int Pos; LockMode md;

	if ((field_d->Flg & f_Stored) != 0) {
		if (ls->LL == 0) saveT(field_d, 0, record);
		else {
			if ((field_d->Flg & f_Encryp) != 0) Coding::Code(ls->A, ls->LL);
#ifdef FandSQL
			if (file_d->IsSQLFile) { SetTWorkFlag; goto label1; }
			else
#endif
				if (HasTWorkFlag(record))
					label1:
			Pos = TWork.Store(ls->A, ls->LL);
				else {
					md = parent->NewLockMode(WrMode);
					Pos = TF->Store(ls->A, ls->LL);
					parent->OldLockMode(md);
				}
			if ((field_d->Flg & f_Encryp) != 0) {
				Coding::Code(ls->A, ls->LL);
			}
			saveT(field_d, Pos, record);
		}
	}
}

int FandFile::saveT(FieldDescr* field_d, int pos, void* record)
{
	char* source = (char*)record + field_d->Displ;
	int* LP = (int*)source;
	if ((field_d->field_type == FieldType::TEXT) && ((field_d->Flg & f_Stored) != 0)) {
		if (file_type == FileType::DBF) {
			if (pos == 0) {
				FillChar(source, 10, ' ');
			}
			else {
				pstring s;
				str(pos, s);
				memcpy(source, &s[1], 10);
			}
		}
		else {
			*LP = pos;
		}
		return 0;
	}
	else {
		//RunError(906);
		return 906;
	}
}

unsigned short FandFile::RdPrefix()
{
	// NRs - celkovy pocet zaznamu v souboru;
	// RLen - delka 1 zaznamu
	struct x6 { int NRs = 0; unsigned short RLen = 0; } X6;
	struct x8 { unsigned short NRs = 0, RLen = 0; } X8;
	struct xD {
		unsigned char Ver = 0; unsigned char Date[3] = { 0,0,0 };
		int NRecs = 0;
		unsigned short HdLen = 0; unsigned short RecLen = 0;
	} XD;
	auto result = 0xffff;
	const bool not_cached = NotCached();
	switch (file_type) {
	case FileType::FAND8: {
		RdWrCache(READ, Handle, not_cached, 0, 2, &X8.NRs);
		RdWrCache(READ, Handle, not_cached, 2, 2, &X8.RLen);
		NRecs = X8.NRs;
		if (RecLen != X8.RLen) { return X8.RLen; }
		break;
	}
	case FileType::DBF: {
		RdWrCache(READ, Handle, not_cached, 0, 1, &XD.Ver);
		RdWrCache(READ, Handle, not_cached, 1, 1, &XD.Date[0]);
		RdWrCache(READ, Handle, not_cached, 2, 1, &XD.Date[1]);
		RdWrCache(READ, Handle, not_cached, 3, 1, &XD.Date[2]);
		RdWrCache(READ, Handle, not_cached, 4, 4, &XD.NRecs);
		RdWrCache(READ, Handle, not_cached, 8, 2, &XD.HdLen);
		RdWrCache(READ, Handle, not_cached, 10, 2, &XD.RecLen);
		NRecs = XD.NRecs;
		if ((RecLen != XD.RecLen)) { return XD.RecLen; }
		FrstDispl = XD.HdLen;
		break;
	}
	default: {
		RdWrCache(READ, Handle, not_cached, 0, 4, &X6.NRs);
		RdWrCache(READ, Handle, not_cached, 4, 2, &X6.RLen);
		NRecs = abs(X6.NRs);
		if ((X6.NRs < 0) && (file_type != FileType::INDEX) || (X6.NRs > 0) && (file_type == FileType::INDEX)
			|| (RecLen != X6.RLen)) {
			return X6.RLen;
		}
		break;
	}
	}
	return result;
}

int FandFile::RdPrefixes()
{
	if (RdPrefix() != 0xffff) {
		//CFileError(CFile, 883);
		return 883;
	}
	if ((XF != nullptr) && (XF->Handle != nullptr)) {
		XF->RdPrefix();
	}
	if ((TF != nullptr)) {
		TF->RdPrefix(false);
	}
	return 0;
}

void FandFile::WrPrefix()
{
	struct { int NRs; unsigned short RLen; } Pfx6 = { 0, 0 };
	struct { unsigned short NRs; unsigned short RLen; } Pfx8 = { 0, 0 };

	if (IsUpdHandle(Handle)) {
		const bool not_cached = NotCached();
		switch (file_type) {
		case FileType::FAND8: {
			Pfx8.RLen = RecLen;
			Pfx8.NRs = static_cast<unsigned short>(NRecs);
			RdWrCache(WRITE, Handle, not_cached, 0, 2, &Pfx8.NRs);
			RdWrCache(WRITE, Handle, not_cached, 2, 2, &Pfx8.RLen);
			break;
		}
		case FileType::DBF: {
			DbfFile::WrDBaseHd(); // TODO: doplnit parametry
			break;
		}
		default: {
			Pfx6.RLen = RecLen;
			if (file_type == FileType::INDEX) Pfx6.NRs = -NRecs;
			else Pfx6.NRs = NRecs;
			RdWrCache(WRITE, Handle, not_cached, 0, 4, &Pfx6.NRs);
			RdWrCache(WRITE, Handle, not_cached, 4, 2, &Pfx6.RLen);
		}
		}
	}
}

void FandFile::WrPrefixes()
{
	WrPrefix();
	if (TF != nullptr && IsUpdHandle(TF->Handle)) {
		TF->WrPrefix();
	}
	if (file_type == FileType::INDEX && XF->Handle != nullptr
		&& /*{ call from CopyDuplF }*/ (IsUpdHandle(XF->Handle) || IsUpdHandle(Handle))) {
		XF->WrPrefix();
	}
}

void FandFile::SaveFile()
{
	WrPrefixes();
	if (file_type == FileType::INDEX) {
		XF->NoCreate = false;
	}
}

void FandFile::CloseFile()
{

}

void FandFile::SetTWorkFlag(void* record)
{
	BYTE* p = (BYTE*)record;
	p[RecLen] = 1;
}

bool FandFile::HasTWorkFlag(void* record)
{
	BYTE* p = (BYTE*)record;
	const bool workFlag = p[RecLen] == 1;
	return workFlag;
}

void FandFile::SetUpdFlag(void* record)
{
	BYTE* p = (BYTE*)record;
	p[RecLen + 1] = 1;
}

void FandFile::ClearUpdFlag(void* record)
{
	BYTE* p = (BYTE*)record;
	p[RecLen + 1] = 0;
}

bool FandFile::HasUpdFlag(void* record)
{
	BYTE* p = (BYTE*)record;
	return p[RecLen + 1] == 1;
}

bool FandFile::DeletedFlag(void* record)
{
	if (file_type == FileType::INDEX) {
		if (((BYTE*)record)[0] == 0) return false;
		else return true;
	}

	if (file_type == FileType::DBF) {
		if (((BYTE*)record)[0] != '*') return false;
		else return true;
	}

	return false;
}

void FandFile::ClearDeletedFlag(void* record)
{
	BYTE* ptr = (BYTE*)record;
	switch (file_type) {
	case FileType::INDEX: { ptr[0] = 0; break; }
	case FileType::DBF: { ptr[0] = ' '; break; }
	}
}

void FandFile::SetDeletedFlag(void* record)
{
	BYTE* ptr = (BYTE*)record;
	switch (file_type) {
	case FileType::INDEX: { ptr[0] = 1; break; }
	case FileType::DBF: { ptr[0] = '*'; break; }
	}
}

double FandFile::_RforD(FieldDescr* F, void* P)
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