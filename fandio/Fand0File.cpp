#include "Fand0File.h"

#include "DbfFile.h"
#include "files.h"
#include "XScan.h"
#include "XWorkFile.h"
#include "XWKey.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"
#include "../Core/GlobalVariables.h"
#include "../Core/Coding.h"
#include "../Core/oaccess.h"
#include "../pascal/real48.h"
#include "../Core/obaseww.h"
#include "../Core/RunMessage.h"
#include "../Core/DateTime.h"

// ****************************** CONSTANTS **********************************
//double Power10[21] = { 1E0, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7, 1E8, 1E9, 1E10,
//	1E11, 1E12, 1E13, 1E14, 1E15, 1E16, 1E17, 1E18, 1E19, 1E20 };
//const double FirstDate = 6.97248E+5;
// ***************************************************************************

Fand0File::Fand0File(FileD* parent)
{
	_parent = parent;
}

Fand0File::Fand0File(const Fand0File& orig, FileD* parent)
{
	RecLen = orig.RecLen;
	file_type = orig.file_type;
	FirstRecPos = orig.FirstRecPos;
	Drive = orig.Drive;

	_parent = parent;

	if (orig.TF != nullptr) TF = new FandTFile(*orig.TF, this);
	if (orig.XF != nullptr) XF = new FandXFile(*orig.XF, this);
}

Fand0File::~Fand0File()
{
	if (Handle != nullptr) {
		CloseH(&Handle);
	}
	if (XF != nullptr) {
		delete XF;
	}
	if (TF != nullptr) {
		delete TF;
	}
}

/// <summary>
/// Vycte zaznam z datoveho souboru (.000)
/// </summary>
/// <param name="rec_nr">kolikaty zaznam (1 .. N)</param>
/// <param name="record">ukazatel na buffer</param>
size_t Fand0File::ReadRec(size_t rec_nr, void* record)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "ReadRec(), file 0x%p, RecNr %i", file, N);
	return ReadData((rec_nr - 1) * RecLen + FirstRecPos, RecLen, record);
}

size_t Fand0File::WriteRec(size_t rec_nr, void* record)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "WriteRec(%i), CFile 0x%p", N, file->Handle);
	WasWrRec = true;
	return WriteData((rec_nr - 1) * RecLen + FirstRecPos, RecLen, record);
}

void Fand0File::CreateRec(int n, void* record)
{
	IncNRecs(1);
	BYTE* tmp = _parent->GetRecSpace();
	for (int i = NRecs - 1; i >= n; i--) {
		ReadRec(i, tmp);
		WriteRec(i + 1, tmp);
	}
	delete[] tmp;
	tmp = nullptr;
	WriteRec(n, record);
}

void Fand0File::DeleteRec(int n, void* record)
{
	DelAllDifTFlds(record, nullptr);
	for (int i = n; i <= NRecs - 1; i++) {
		ReadRec(i + 1, record);
		WriteRec(i, record);
	}
	DecNRecs(1);
}

void Fand0File::DelAllDifTFlds(void* record, void* comp_record)
{
	for (auto& F : _parent->FldD) {
		if (F->field_type == FieldType::TEXT && ((F->Flg & f_Stored) != 0)) {
			DelDifTFld(F, record, comp_record);
		}
	}
}

int Fand0File::UsedFileSize()
{
	int n = NRecs * RecLen + FirstRecPos;
	if (file_type == FileType::DBF) n++;
	return n;
}

bool Fand0File::IsShared()
{
	return (UMode == Shared) || (UMode == RdShared);
}

bool Fand0File::NotCached()
{
	if (UMode == Shared) goto label1;
	if (UMode != RdShared) return false;
label1:
	if (LMode == ExclMode) return false;
	return true;
}

bool Fand0File::Cached()
{
	return !NotCached();
}

void Fand0File::Reset()
{
	RecLen = 0;
	RecPtr = nullptr;
	NRecs = 0;
	FirstRecPos = 0;
	WasWrRec = false; WasRdOnly = false; Eof = false;
	file_type = FileType::UNKNOWN;        // 8= Fand 8; 6= Fand 16; X= .X; 0= RDB; C= CAT 
	Handle = nullptr;
	ClearUpdateFlag();
	TF = nullptr;
	Drive = 0;         // 1=A, 2=B, else 0
	UMode = FileUseMode::Closed;
	LMode = NullMode; ExLMode = NullMode; TaLMode = NullMode;
	XF = nullptr;
}

void Fand0File::IncNRecs(int n)
{
#ifdef FandDemo
	if (NRecs > 100) RunError(884);
#endif
	NRecs += n;
	SetUpdateFlag(); //SetUpdHandle(Handle);
	if (file_type == FileType::INDEX) {
		XF->SetUpdateFlag(); //SetUpdHandle(XF->Handle);
	}
}

void Fand0File::DecNRecs(int n)
{
	NRecs -= n;
	SetUpdateFlag(); //SetUpdHandle(Handle);
	if (file_type == FileType::INDEX) {
		XF->SetUpdateFlag(); //SetUpdHandle(XF->Handle);
	}
	WasWrRec = true;
}

void Fand0File::PutRec(void* record, int& i_rec)
{
	NRecs++;
	WriteData(i_rec * RecLen + FirstRecPos, RecLen, record);
	i_rec++;
	Eof = true;
}

size_t Fand0File::RecordSize()
{
	return RecLen;
}

bool Fand0File::loadB(FieldDescr* field_d, void* record)
{
	bool result = false;
	unsigned char* CP = (unsigned char*)record + field_d->Displ;

	if (file_type == FileType::DBF) {
		result = *CP == 'Y' || *CP == 'y' || *CP == 'T' || *CP == 't';
	}
	else if ((*CP == '\0') || (*CP == 0xFF)) { //x0FF is NULL value
		result = false;
	}
	else result = true;

	return result;
}

double Fand0File::loadR(FieldDescr* field_d, void* record)
{
	uint8_t* source = static_cast<uint8_t*>(record) + field_d->Displ;
	double result = 0.0;

	if (file_type == FileType::DBF) {
		result = DBF_RforD(field_d, source);
	}
	else {
		switch (field_d->field_type) {
		case FieldType::FIXED: { // FIX CISLO (M,N)
			const double r = RealFromFix(source, field_d->NBytes);
			if ((field_d->Flg & f_Comma) == 0) {
				result = r / Power10[field_d->M];
			}
			else {
				result = r;
			}
			break;
		}
		case FieldType::DATE: { // DATUM DD.MM.YY
			if (file_type == FileType::FAND8) {
				if (*reinterpret_cast<int16_t*>(source) == 0) result = 0.0;
				else result = *reinterpret_cast<int16_t*>(source) + FirstDate;
			}
			else {
				if (is_null_value(field_d, source)) {
					result = 0;
				}
				else {
					result = Real48ToDouble(source);
				}
			}
			break;
		}
		case FieldType::REAL: {
			if (is_null_value(field_d, source)) {
				result = 0;
			}
			else {
				result = Real48ToDouble(source);
			}
			break;
		}
		case FieldType::TEXT: {
			// pointer to text file 4B
			const int32_t i = *reinterpret_cast<int32_t*>(source);
			result = i;
			break;
		}
		default:
			break;
		}
	}
	return result;
}

std::string Fand0File::loadS(FieldDescr* field_d, void* record)
{
	char* source = (char*)record + field_d->Displ;
	std::string S;
	int pos = 0;
	LockMode md;
	WORD l = field_d->L;
	switch (field_d->field_type)
	{
	case FieldType::ALFANUM:		// znakovy retezec max. 255 znaku
	case FieldType::NUMERIC: {		// ciselny retezec max. 79 znaku
		if (field_d->field_type == FieldType::ALFANUM) {
			S = std::string(source, l);
			if ((field_d->Flg & f_Encryp) != 0) {
				S = Coding::Code(S);
			}
			if (!S.empty() && S[0] == '\0') {
				S = RepeatString(' ', l);
			}
		}
		else if (is_null_value(field_d, reinterpret_cast<uint8_t*>(source))) {
			S = RepeatString(' ', l);
		}
		else {
			//jedna je o typ N - prevedeme cislo na znaky
			for (BYTE i = 0; i < field_d->L; i++) {
				bool upper = (i % 2) == 0; // jde o "levou" cislici
				BYTE j = i / 2;
				if (upper) {
					S += ((BYTE)source[j] >> 4) + 0x30;
				}
				else {
					S += ((BYTE)source[j] & 0x0F) + 0x30;
				}
			}
		}
		break;
	}
	case FieldType::TEXT: { // volny text max. 65k
		if (HasTWorkFlag(record)) {
			S = TWork.Read(loadT(field_d, record));
		}
		else {
			md = _parent->NewLockMode(RdMode);
			S = TF->Read(loadT(field_d, record));
			_parent->OldLockMode(md);
		}
		if ((field_d->Flg & f_Encryp) != 0) {
			S = Coding::Code(S);
		}
		break;
	}
	}
	return S;
}

int Fand0File::loadT(FieldDescr* F, void* record)
{
	//int n = 0;
	//short err = 0;
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
		return *reinterpret_cast<int*>(source);
	}
}

void Fand0File::saveB(FieldDescr* field_d, bool b, void* record)
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

void Fand0File::saveR(FieldDescr* field_d, double r, void* record)
{
	BYTE* pRec = (BYTE*)record + field_d->Displ;
	int l = 0;
	if ((field_d->Flg & f_Stored) != 0) {
		WORD m = field_d->M;
		switch (field_d->field_type) {
		case FieldType::FIXED: {
			if (file_type == FileType::DBF) {
				std::string s;
				if ((field_d->Flg & f_Comma) != 0) r = r / Power10[m];
				str(r, field_d->NBytes, field_d->M, s);
				memcpy(pRec, s.c_str(), s.length());
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
				std::string s = StrDate(r, "YYYYMMDD");
				memcpy(pRec, s.c_str(), s.length());
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

void Fand0File::saveS(FileD* parent, FieldDescr* field_d, std::string s, void* record)
{
	const BYTE LeftJust = 1;
	BYTE* pRec = (BYTE*)record + field_d->Displ;

	if (field_d->isStored()) {
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
				s = TrailChar(s, ' ');
				memcpy(tmpArr, s.c_str(), s.length());
				memset(&tmpArr[s.length()], '0', field_d->L - s.length());
			}
			else {
				// doplnime nuly zleva
				s = LeadChar(s, ' ');
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
			if (s.length() == 0) {
				saveT(field_d, 0, record);
			}
			else {
				if (field_d->isEncrypted() != 0) {
					s = Coding::Code(s);
				}
				if (HasTWorkFlag(record)) {
					int pos = TWork.Store(s);
					saveT(field_d, pos, record);
				}
				else {
					LockMode md = parent->NewLockMode(WrMode);
					int pos = TF->Store(s);
					saveT(field_d, pos, record);
					parent->OldLockMode(md);
				}
			}
			break;
		}
		}
	}
	else {
		// field is not stored
	}
}

int Fand0File::saveT(FieldDescr* field_d, int pos, void* record)
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

void Fand0File::DelTFld(FieldDescr* field_d, void* record)
{
	int n = loadT(field_d, record);
	if (HasTWorkFlag(record)) {
		TWork.Delete(n);
	}
	else {
		LockMode md = _parent->NewLockMode(WrMode);
		TF->Delete(n);
		_parent->OldLockMode(md);
	}
	saveT(field_d, 0, record);
}

void Fand0File::DelTFlds(void* record)
{
	for (auto& field : _parent->FldD) {
		if (((field->Flg & f_Stored) != 0) && (field->field_type == FieldType::TEXT)) {
			DelTFld(field, record);
		}
	}
}

/**
 * \brief Pokud zaznamy odkazuji na ruzne texty, je text z 'record' smazan
 * \param field_d popis pole
 * \param record 1. zaznam (ktery je pripadne smazan)
 * \param comp_record 2. zaznam
 */
void Fand0File::DelDifTFld(FieldDescr* field_d, void* record, void* comp_record)
{
	const int n1 = loadT(field_d, comp_record);
	const int n2 = loadT(field_d, record);
	if (n1 != n2) {
		DelTFld(field_d, record);
	}
}

uint16_t Fand0File::RdPrefix()
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

	uint16_t result = 0xffff;

	switch (file_type) {
	case FileType::FAND8: {
		ReadData(0, 2, &X8.NRs);
		ReadData(2, 2, &X8.RLen);
		NRecs = X8.NRs;
		if (RecLen != X8.RLen) { return X8.RLen; }
		break;
	}
	case FileType::DBF: {
		ReadData(0, 1, &XD.Ver);
		ReadData(1, 1, &XD.Date[0]);
		ReadData(2, 1, &XD.Date[1]);
		ReadData(3, 1, &XD.Date[2]);
		ReadData(4, 4, &XD.NRecs);
		ReadData(8, 2, &XD.HdLen);
		ReadData(10, 2, &XD.RecLen);
		NRecs = XD.NRecs;
		if ((RecLen != XD.RecLen)) { return XD.RecLen; }
		FirstRecPos = XD.HdLen;
		break;
	}
	default: {
		ReadData(0, 4, &X6.NRs);
		ReadData(4, 2, &X6.RLen);
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

int Fand0File::RdPrefixes()
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

void Fand0File::WrPrefix()
{
	struct { int NRs; unsigned short RLen; } Pfx6 = { 0, 0 };
	struct { unsigned short NRs; unsigned short RLen; } Pfx8 = { 0, 0 };

	if (_updateFlag) {
		const bool not_cached = NotCached();
		switch (file_type) {
		case FileType::FAND8: {
			Pfx8.RLen = RecLen;
			Pfx8.NRs = static_cast<unsigned short>(NRecs);
			WriteData(0, 2, &Pfx8.NRs);
			WriteData(2, 2, &Pfx8.RLen);
			break;
		}
		case FileType::DBF: {
			DbfFile::WrDBaseHd(this->_parent); // TODO: doplnit parametry
			break;
		}
		default: {
			Pfx6.RLen = RecLen;
			if (file_type == FileType::INDEX) Pfx6.NRs = -NRecs;
			else Pfx6.NRs = NRecs;
			WriteData(0, 4, &Pfx6.NRs);
			WriteData(4, 2, &Pfx6.RLen);
		}
		}
	}
}

void Fand0File::WrPrefixes()
{
	if (_updateFlag) {
		WrPrefix();
	}

	if (TF != nullptr && TF->HasUpdateFlag()) {
		TF->WrPrefix();
	}

	if (file_type == FileType::INDEX 
		&& XF->Handle != nullptr
		/*{ call from CopyDuplF }*/
		&& (XF->HasUpdateFlag() || _updateFlag)) {
		XF->WrPrefix(NRecs, _parent->GetNrKeys());
	}
}

void Fand0File::TruncFile()
{
	if (UMode == RdOnly) return;
	LockMode md = _parent->NewLockMode(RdMode);
	TruncF(Handle, HandleError, UsedFileSize());
	if (HandleError != 0) {
		FileMsg(_parent, 700 + HandleError, '0');
	}
	if (TF != nullptr) {
		TruncF(TF->Handle, HandleError, TF->UsedFileSize());
		TF->TestErr();
	}
	if (file_type == FileType::INDEX) {
		int sz = XF->UsedFileSize();
		if (XF->NotValid) sz = 0;
		TruncF(XF->Handle, HandleError, sz);
		XF->TestErr();
	}
	_parent->OldLockMode(md);
}

LockMode Fand0File::RewriteFile(bool append)
{
	LockMode result;
	if (append) {
		result = _parent->NewLockMode(CrMode);
		_parent->SeekRec(NRecs);
		if (XF != nullptr) {
			XF->FirstDupl = true;
			TestXFExist();
		}
		return result;
	}
	result = _parent->NewLockMode(ExclMode);
	NRecs = 0;
	_parent->SeekRec(0);
	SetUpdateFlag(); //SetUpdHandle(Handle);

	int notValid = XFNotValid();
	if (notValid != 0) {
		RunError(notValid);
	}

	if (file_type == FileType::INDEX) XF->NoCreate = true;
	if (TF != nullptr) TF->SetEmpty();
	return result;
}

void Fand0File::ClearUpdateFlag()
{
	_updateFlag = false;

	if (file_type == FileType::INDEX) {
		XF->ClearUpdateFlag();
	}

	if (TF != nullptr) {
		TF->ClearUpdateFlag();
	}
}

void Fand0File::SaveFile()
{
	WrPrefixes();
	if (file_type == FileType::INDEX) {
		XF->NoCreate = false;
	}
	//if (IsUpdHandle(Handle)) {
	//	SaveCache(0, Handle);
	//	ResetUpdHandle(Handle);
	//}
	//if (XF != nullptr && XF->Handle != nullptr && IsUpdHandle(XF->Handle)) {
	//	SaveCache(0, XF->Handle);
	//	ResetUpdHandle(XF->Handle);
	//}
	//if (TF != nullptr && TF->Handle != nullptr && IsUpdHandle(TF->Handle)) {
	//	SaveCache(0, TF->Handle);
	//	ResetUpdHandle(TF->Handle);
	//}
}

/// <summary>
/// Close file with saving changes (prefixes, ...)
/// </summary>
void Fand0File::CloseFile()
{
	if (IsShared()) {
		_parent->OldLockMode(NullMode);
	}
	else {
		WrPrefixes();
	}
	SaveCache(0, Handle);
	TruncFile();

	// close index file
	if (file_type == FileType::INDEX && XF != nullptr) {
		XF->CloseFile();
	}

	// close .T00 text file
	if (TF != nullptr) {
		TF->CloseFile();
	}

	CloseClearH(&Handle);
	if (HandleError == 0) {
		Handle = nullptr;
		ClearUpdateFlag();
	}
	LMode = NullMode;

	if (!IsShared() && (NRecs == 0) && (file_type != FileType::DBF)) {
		std::string path = SetPathAndVolume(_parent);
		MyDeleteFile(path);
	}

	if (WasRdOnly) {
		WasRdOnly = false;
		std::string path = SetPathAndVolume(_parent);
		SetFileAttr(path, HandleError, (GetFileAttr(CPath, HandleError) & 0x27) | 0x01); // {RdOnly; }
		if (TF != nullptr) {
			path = CExtToT(TF, CDir, CName, CExt);
			SetFileAttr(path, HandleError, (GetFileAttr(CPath, HandleError) & 0x27) | 0x01); //  {RdOnly; }
		}
	}
}

/// <summary>
/// Plain close file handle (main, index, text)
/// </summary>
void Fand0File::Close()
{
	CloseH(&Handle);
	if (file_type == FileType::INDEX) {
		CloseH(&XF->Handle);
	}
	if (TF != nullptr) {
		CloseH(&TF->Handle);
	}
}

void Fand0File::SetTWorkFlag(void* record)
{
	BYTE* p = (BYTE*)record;
	p[RecLen] = 1;
}

bool Fand0File::HasTWorkFlag(void* record)
{
	BYTE* p = (BYTE*)record;
	const bool workFlag = p[RecLen] == 1;
	return workFlag;
}

void Fand0File::SetRecordUpdateFlag(void* record)
{
	BYTE* p = (BYTE*)record;
	p[RecLen + 1] = 1;
}

void Fand0File::ClearRecordUpdateFlag(void* record)
{
	BYTE* p = (BYTE*)record;
	p[RecLen + 1] = 0;
}

bool Fand0File::HasRecordUpdateFlag(void* record)
{
	BYTE* p = (BYTE*)record;
	return p[RecLen + 1] == 1;
}

bool Fand0File::DeletedFlag(void* record)
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

void Fand0File::ClearDeletedFlag(void* record)
{
	BYTE* ptr = (BYTE*)record;
	switch (file_type) {
	case FileType::INDEX: { ptr[0] = 0; break; }
	case FileType::DBF: { ptr[0] = ' '; break; }
	}
}

void Fand0File::SetDeletedFlag(void* record)
{
	BYTE* ptr = (BYTE*)record;
	switch (file_type) {
	case FileType::INDEX: { ptr[0] = 1; break; }
	case FileType::DBF: { ptr[0] = '*'; break; }
	}
}

void Fand0File::ClearXFUpdLock()
{
	if (XF != nullptr) {
		XF->ClearUpdLock();
	}
}

int Fand0File::XFNotValid()
{
	if (XF == nullptr) {
		return 0;
	}
	else {
		return XF->XFNotValid(NRecs, _parent->GetNrKeys());
	}
}

int Fand0File::CreateIndexFile()
{
	Logging* log = Logging::getInstance();

	LockMode md = NullMode;
	bool fail = false;

	try {
		fail = true;
		//BYTE* record = _parent->GetRecSpace();
		md = _parent->NewLockMode(RdMode);
		_parent->Lock(0, 0);
		/*ClearCacheCFile;*/
		if (XF->Handle == nullptr) {
			//RunError(903);
			return 903;
		}
		log->log(loglevel::DEBUG, "CreateIndexFile() file 0x%p name '%s'", XF->Handle, _parent->Name.c_str());
		XF->RdPrefix();
		if (XF->NotValid) {
			XF->SetEmpty(NRecs, _parent->GetNrKeys());
			std::vector<KeyInD*> empty;
			XScan* scan = new XScan(_parent, nullptr, empty, false);
			BYTE* record = _parent->GetRecSpace();
			scan->Reset(nullptr, false, record);
			XWorkFile* XW = new XWorkFile(_parent, scan, _parent->Keys);
			XW->Main('X', record);
			delete[] record; record = nullptr;
			delete XW; XW = nullptr;
			XF->NotValid = false;
			XF->WrPrefix(NRecs, _parent->GetNrKeys());
			if (!SaveCache(0, Handle)) GoExit(MsgLine);
			/*FlushHandles; */
		}
		fail = false;
	}
	catch (std::exception& e) {
		// TODO: log error
	}

	if (fail) {
		XF->SetNotValid(NRecs, _parent->GetNrKeys());
		XF->NoCreate = true;
	}
	_parent->Unlock(0);
	_parent->OldLockMode(md);
	if (fail) GoExit(MsgLine);

	return 0;
}

int Fand0File::TestXFExist()
{
	if ((XF != nullptr) && XF->NotValid) {
		if (XF->NoCreate) {
			CFileError(_parent, 819);
			return 819;
		}
		int a = CreateIndexFile();
		if (a != 0) {
			RunError(a);
			return a;
		}
	}
	return 0;
}

FileD* Fand0File::GetFileD()
{
	return _parent;
}

bool Fand0File::SearchKey(XString& XX, XKey* Key, int& NN, void* record)
{
	int R = 0;
	XString x;

	bool bResult = false;
	int L = 1;
	short Result = _gt;
	NN = NRecs;
	int N = NN;
	if (N == 0) return bResult;

	do {
		if (Result == _gt) {
			R = N;
		}
		else {
			L = N + 1;
		}
		N = (L + R) / 2;
		_parent->ReadRec(N, record);
		x.PackKF(_parent, Key->KFlds, record);
		Result = CompStr(x.S, XX.S);
	} while (!((L >= R) || (Result == _equ)));

	if ((N == NN) && (Result == _lt)) {
		NN++;
	}
	else {
		if (Key->Duplic && (Result == _equ)) {
			while (N > 1) {
				N--;
				_parent->ReadRec(N, record);
				x.PackKF(_parent, Key->KFlds, record);
				if (CompStr(x.S, XX.S) != _equ) {
					N++;
					_parent->ReadRec(N, record);
					break;
				}
			}
		}
		NN = N;
	}

	if ((Result == _equ) || Key->IntervalTest && (Result == _gt)) {
		bResult = true;
	}

	return bResult;
}

int Fand0File::XNRecs(std::vector<XKey*>& K)
{
	if (file_type == FileType::INDEX && !K.empty()) {
		TestXFExist();
		return XF->NRecs;
	}
	else {
		return NRecs;
	}
}

void Fand0File::TryInsertAllIndexes(int RecNr, void* record)
{
	TestXFExist();
	XKey* lastK = nullptr;
	for (auto& K : _parent->Keys) {
		lastK = K;
		if (!K->Insert(_parent, RecNr, true, record)) {
			goto label1;
		}
	}
	XF->NRecs++;
	return;

label1:
	for (auto& K1 : _parent->Keys) {
		if (K1 == lastK) {
			break;
		}
		K1->Delete(_parent, RecNr, record);
	}
	_parent->SetDeletedFlag(record);
	_parent->WriteRec(RecNr, record);

	if (XF->FirstDupl) {
		SetMsgPar(_parent->Name);
		WrLLF10Msg(828);
		XF->FirstDupl = false;
	}
}

void Fand0File::DeleteAllIndexes(int RecNr, void* record)
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "DeleteAllIndexes(%i)", RecNr);

	for (auto& K : _parent->Keys) {
		K->Delete(_parent, RecNr, record);
	}
}

void Fand0File::DeleteXRec(int RecNr, bool DelT, void* record)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "DeleteXRec(%i, %s)", RecNr, DelT ? "true" : "false");
	TestXFExist();
	DeleteAllIndexes(RecNr, record);
	if (DelT) {
		_parent->DelAllDifTFlds(record, nullptr);
	}
	SetDeletedFlag(record);
	_parent->WriteRec(RecNr, record);
	XF->NRecs--;
}

void Fand0File::OverWrXRec(int RecNr, void* P2, void* P, void* record)
{
	XString x, x2;
	record = P2;
	if (_parent->DeletedFlag(record)) {
		record = P;
		_parent->RecallRec(RecNr, record);
		return;
	}
	TestXFExist();

	for (auto& K : _parent->Keys) {
		record = P;
		x.PackKF(_parent, K->KFlds, record);
		record = P2;
		x2.PackKF(_parent, K->KFlds, record);
		if (x.S != x2.S) {
			K->Delete(_parent, RecNr, record);
			record = P;
			K->Insert(_parent, RecNr, false, record);
		}
	}

	record = P;
	_parent->WriteRec(RecNr, record);
}

void Fand0File::GenerateNew000File(XScan* x, void* record)
{
	// vytvorime si novy buffer pro data,
	// ten pak zapiseme do souboru naprimo (bez cache)

	const unsigned short header000len = 6; // 4B pocet zaznamu, 2B delka jednoho zaznamu
	// z puvodniho .000 vycteme pocet zaznamu a jejich delku
	const size_t totalLen = x->FD->FF->NRecs * x->FD->FF->RecLen + header000len;
	unsigned char* buffer = new unsigned char[totalLen] { 0 };
	size_t offset = header000len; // zapisujeme nejdriv data; hlavicku az nakonec

	while (!x->eof) {
		RunMsgN(x->IRec);
		NRecs++;
		memcpy(&buffer[offset], record, RecLen);
		offset += RecLen;
		_parent->IRec++;
		Eof = true;
		x->GetRec(record);
	}

	// zapiseme hlavicku
	memcpy(&buffer[0], &NRecs, 4);
	memcpy(&buffer[4], &RecLen, 2);

	// provedeme primy zapis do souboru
	WriteH(Handle, totalLen, buffer);

	delete[] buffer; buffer = nullptr;
}

void Fand0File::CreateWIndex(XScan* Scan, XWKey* K, char Typ)
{
	BYTE* record = _parent->GetRecSpace();
	std::vector<XKey*> xw_keys;
	xw_keys.push_back(K);
	XWorkFile* XW = new XWorkFile(_parent, Scan, xw_keys);
	XW->Main(Typ, record);
	delete XW; XW = nullptr;
	delete[] record; record = nullptr;
}

void Fand0File::ScanSubstWIndex(XScan* Scan, std::vector<KeyFldD*>& SK, char Typ)
{
	unsigned short n = 0;
	XWKey* k2 = new XWKey(_parent);

	if (Scan->FD->IsSQLFile && (Scan->Kind == 3)) {
		/* F6-autoreport & sort */
		XKey* k = Scan->Key;
		n = k->IndexLen;

		//KeyFldD* kf = SK;
		//while (kf != nullptr) {
		for (KeyFldD* kf : SK) {
			n += kf->FldD->NBytes;
			//kf = kf->pChain;
		}

		if (n > 255) {
			WrLLF10Msg(155);
			delete k2; k2 = nullptr;
			return;
		}

		//kf = k->KFlds;
		std::vector<KeyFldD*> kfroot;
		//KeyFldD* kf2 = nullptr;
		//while (kf != nullptr) {
		for (KeyFldD* kf : k->KFlds) {
			//kf2 = new KeyFldD();
			//*kf2 = *kf;
			//ChainLast(kfroot, kf2);
			//kf = kf->pChain;
			kfroot.push_back(kf);
		}
		if (SK.size() > kfroot.size()) {
			for (size_t i = 0; i < SK.size(); i++) {
				kfroot.push_back(SK[i]);
			}
		}

		k2->Open(_parent, kfroot, true, false);
	}
	else {
		k2->Open(_parent, SK, true, false);
	}

	CreateWIndex(Scan, k2, Typ);
	Scan->SubstWIndex(k2);
}

void Fand0File::SortAndSubst(std::vector<KeyFldD*>& SK)
{
	BYTE* record = _parent->GetRecSpace();

	std::vector<KeyInD*> empty;
	XScan* Scan = new XScan(_parent, nullptr, empty, false);
	Scan->Reset(nullptr, false, record);
	ScanSubstWIndex(Scan, SK, 'S');
	FileD* FD2 = _parent->OpenDuplicateF(false);
	RunMsgOn('S', Scan->NRecs);
	Scan->GetRec(record);

	// write data to a file .100
	FD2->FF->GenerateNew000File(Scan, record);

	SubstDuplF(FD2, false);
	Scan->Close();
	RunMsgOff();

	delete FD2; FD2 = nullptr;
	delete[] record; record = nullptr;
}

void Fand0File::CopyIndex(XWKey* K, XKey* FromK)
{
	BYTE* record = _parent->GetRecSpace();

	K->Release(_parent);
	LockMode md = _parent->NewLockMode(RdMode);
	std::vector<KeyInD*> empty;
	XScan* Scan = new XScan(_parent, FromK, empty, false);
	Scan->Reset(nullptr, false, record);
	CreateWIndex(Scan, K, 'W');
	_parent->OldLockMode(md);

	delete[] record; record = nullptr;
}

void Fand0File::SubstDuplF(FileD* TempFD, bool DelTF)
{
	//bool net;
	int result = XFNotValid();
	if (result != 0) {
		RunError(result);
	}

	std::string orig_path = SetPathAndVolume(_parent);
	std::string orig_path_T = _extToT(orig_path);

	if (IsNetCVol()) {
		CopyDuplF(TempFD, DelTF);
		return;
	}

	// close and delete original file
	SaveCache(0, Handle);
	CloseClearH(&Handle);
	MyDeleteFile(orig_path);
	TestDelErr(orig_path);

	// copy data from TempFD
	RecLen = TempFD->FF->RecLen;
	file_type = TempFD->FF->file_type;
	FirstRecPos = TempFD->FF->FirstRecPos;
	Drive = TempFD->FF->Drive;

	// rename temp file to regular
	std::string temp_path = SetTempCExt(_parent, '0', false);
	SaveCache(0, TempFD->FF->Handle);
	CloseClearH(&TempFD->FF->Handle);
	RenameFile56(temp_path, orig_path, true);
	Handle = OpenH(orig_path, _isOldFile, UMode);
	_parent->FullPath = orig_path;
	SetUpdateFlag(); //SetUpdHandle(Handle);

	//// copy Index File
	//delete XF; XF = nullptr;
	//if (TempFD->FF->XF != nullptr) {
	//	XF = new FandXFile(*TempFD->FF->XF, this);
	//}

	//// copy Text File
	//delete TF; TF = nullptr;
	//if (TempFD->FF->TF != nullptr) {
	//	TF = new FandTFile(*TempFD->FF->TF, this);
	//}

	if ((TempFD->FF->TF != nullptr) && DelTF) {
		CloseClearH(&TF->Handle);
		MyDeleteFile(orig_path_T);
		TestDelErr(orig_path_T);
		//*parent_tf = *ref_to_parent->FF->TF;
		//ref_to_parent->FF->TF = parent_tf;
		CloseClearH(&TempFD->FF->TF->Handle);
		std::string temp_path_t = SetTempCExt(_parent, 'T', false);
		RenameFile56(temp_path_t, orig_path_T, true);
		TF->Handle = OpenH(orig_path_T, _isOldFile, UMode);
		SetUpdateFlag(); //SetUpdHandle(TF->Handle);
		//if (orig.TF != nullptr) TF = new FandTFile(*orig.TF, this);
	}
}

void Fand0File::CopyDuplF(FileD* TempFD, bool DelTF)
{
	TempFD->FF->WrPrefixes();
	SaveCache(0, Handle);
	SetTempCExt(_parent, '0', true);
	CopyH(TempFD->FF->Handle, Handle);

	// TempFD has been deleted in CopyH -> set Handle to nullptr
	TempFD->FF->Handle = nullptr;
	TempFD->FF->ClearUpdateFlag();

	if ((TF != nullptr) && DelTF) {
		HANDLE h1 = TempFD->FF->TF->Handle;
		HANDLE h2 = TF->Handle;
		SetTempCExt(_parent, 'T', true);
		*TF = *TempFD->FF->TF;
		TF->Handle = h2;
		CopyH(h1, h2);
	}
	int rp = RdPrefixes();
	if (rp != 0) {
		CFileError(_parent, rp);
	}
}

void Fand0File::IndexFileProc(bool Compress)
{
	LockMode md = _parent->NewLockMode(ExclMode);

	int result = XFNotValid();
	if (result != 0) {
		RunError(result);
	}

	BYTE* record = _parent->GetRecSpace();
	if (Compress) {
		FileD* FD2 = _parent->OpenDuplicateF(false);
		for (int rec_nr = 1; rec_nr <= NRecs; rec_nr++) {
			_parent->ReadRec(rec_nr, record);
			if (!_parent->DeletedFlag(record)) {
				FD2->PutRec(record);
			}
		}
		if (!SaveCache(0, Handle)) {
			GoExit(MsgLine);
		}
		SubstDuplF(FD2, false);
		NRecs = FD2->FF->NRecs;
		XFNotValid();
		delete FD2; FD2 = nullptr;
	}
	XF->NoCreate = false;
	TestXFExist();
	_parent->OldLockMode(md);
	SaveFiles();
	delete[] record; record = nullptr;
}

/// \brief Copies T from one .T00 file to another. Files should be locked before calling this function!
/// \param destT00File destination T file - should be locked in WrMode
/// \param srcT00File source T file - should be locked in RdMode
/// \param srcT00Pos position of T in source T file
/// \return position of T in destination T file
int Fand0File::CopyT(FandTFile* destT00File, FandTFile* srcT00File, int srcT00Pos)
{
	WORD l = 0;
	int pos = 0;

	BYTE page_buffer[MPageSize + 1]{ 0 };
	const uint16_t* ll = reinterpret_cast<uint16_t*>(page_buffer);

	int result = 0;

	if (srcT00Pos == 0) {
		return 0; // mark
	}

	// read length of text
	srcT00File->ReadData(srcT00Pos, 2, &l);
	if (l <= MPageSize - 2) { // short text
		if (l == 0) {
			return 0; // mark
		}

		// read text from source T file
		srcT00File->ReadData(srcT00Pos + 2, l, page_buffer);
		int16_t rest = static_cast<int16_t>(MPageSize - destT00File->FreePart % MPageSize);

		// find place for new short text
		if (rest >= l + 2) {
			// 2B length + text fits into current page
			pos = destT00File->FreePart;
		}
		else {
			// start on new page
			pos = destT00File->NewPage(false);
			destT00File->FreePart = pos;
			rest = MPageSize;
		}

		if (rest <= l + 4) {
			destT00File->FreePart = destT00File->NewPage(false);
		}
		else {
			destT00File->FreePart += l + 2;
			rest = l + 4 - rest;
			destT00File->WriteData(destT00File->FreePart, 2, & rest);
		}

		// write length and text to destination T file
		destT00File->WriteData(pos, 2, &l);
		destT00File->WriteData(pos + 2, l, page_buffer);
		// return position of new text in destination T file
		result = pos;
		return result;
	}

	if ((srcT00Pos % MPageSize) != 0) {
		// it's not short text, and it is not on the beginning of the page
		srcT00File->Err(889, false);
		result = 0;
		return result;
	}

	// process long text
	srcT00File->ReadData(srcT00Pos, MPageSize, page_buffer);
	bool first = true;
	bool end = false;

	while (!end) {
		end = true;
		if (l > MaxLStrLen + 1) {
			// l > 65001
			srcT00File->Err(889, false);
			result = 0;
			return result;
		}
		const bool isLongTxt = (l == MaxLStrLen + 1); // length is 65001
		if (isLongTxt) l--;
		l += 2;

		while (true) {
			if (first) {
				// always create new page for first part of long text
				pos = destT00File->NewPage(false);
				result = pos;
				first = false;
			}

			if ((l > MPageSize) || isLongTxt) {
				// the page buffer contains both source and destination data
				// - only position of next page is written to the end of the page

				// get position of next page in source T file
				srcT00Pos = *reinterpret_cast<int*>(&page_buffer[MPageSize - 4]);
				// get new page for next part of long text in destination T file
				const int next_pos = destT00File->NewPage(false);
				// write position of next page to the end of current page in destination T file
				*reinterpret_cast<int*>(&page_buffer[MPageSize - 4]) = next_pos;
				// write current page to destination T file
				destT00File->WriteData(pos, MPageSize, page_buffer);
				pos = next_pos;

				if ((srcT00Pos < MPageSize) || (srcT00Pos + MPageSize > srcT00File->MLen) || (srcT00Pos % MPageSize != 0)) {
					srcT00File->Err(888, false);
					result = 0;
					return result;
				}

				// read next part of long text from source T file
				srcT00File->ReadData(srcT00Pos, MPageSize, page_buffer);
				if ((l <= MPageSize)) {
					// last part of long text, page is not full
					l = *ll;
					end = false;
					break;
				}
				else {
					// MPageSize - 4 has been written to the destination T file
					l -= MPageSize - 4;
					continue;
				}
			}
			break;
		}
	}

	// write last part of long text
	destT00File->WriteData(pos, MPageSize, page_buffer);

	return result;
}

void Fand0File::CopyTFStringToH(FileD* file_d, HANDLE h, FandTFile* TF02, FileD* TFD02, int& TF02Pos)
{
	CFile = file_d;

	WORD i = 0;
	bool isLongTxt = false;
	int pos = 0;
	size_t n = 0;
	BYTE X[MPageSize + 1]{ 0 };
	WORD* ll = (WORD*)X;
	LockMode md2;

	pos = TF02Pos;
	if (pos == 0) return;
	FandTFile* tf = TF02;
	if (!tf->IsWork) md2 = TFD02->NewLockMode(RdMode);
	size_t l = 0;
	tf->ReadData(pos, 2, &l);
	if (l <= MPageSize - 2) { /* short text */
		tf->ReadData(pos + 2, l, X);
		WriteH(h, l, X);
		goto label4;
	}
	if ((pos % MPageSize) != 0) goto label2;
	tf->ReadData(pos, MPageSize, X);
label1:
	if (l > MaxLStrLen + 1) {
	label2:
		tf->Err(889, false);
		goto label4;
	}
	isLongTxt = (l == MaxLStrLen + 1);
	if (isLongTxt) l--;
	i = 2;
label3:
	if ((l > MPageSize - i) || isLongTxt) {
		n = MPageSize - 4 - i;
		if (n > l) n = l;
		WriteH(h, n, &X[i]);
		pos = *(int*)&X[MPageSize - 4];
		if ((pos < MPageSize) || (pos + MPageSize > tf->MLen) || (pos % MPageSize != 0)) {
			tf->Err(888, false);
			goto label4;
		}
		tf->ReadData(pos, MPageSize, X);
		if ((l <= MPageSize - i)) {
			l = *ll;
			goto label1;
		}
		l -= n;
		i = 0;
		goto label3;
	}
	WriteH(h, l, &X[i]);
label4:
	if (!tf->IsWork) TFD02->OldLockMode(md2);
	CFile = file_d;
}


double Fand0File::DBF_RforD(FieldDescr* field_d, uint8_t* source)
{
	char* ptr = (char*)source;
	short err;
	double r = 0;

	switch (field_d->field_type) {
	case FieldType::FIXED: {
		std::string s = std::string(ptr, field_d->NBytes);
		ReplaceChar(s, ',', '.');
		if ((field_d->Flg & f_Comma) != 0) {
			size_t i = s.find('.');
			if (i != std::string::npos) s.erase(i, 1);
		}
		s = TrailChar(s, ' ');
		s = LeadChar(s, ' ');
		val(s, r, err);
		break;
	}
	case FieldType::DATE: {
		std::string s = std::string(ptr, 8); // DBF Date is 8 bytes long 'YYYYMMDD' 
		r = ValDate(s, "YYYYMMDD");
		break;
	}
	}
	return r;
}

bool Fand0File::is_null_value(FieldDescr* field_d, uint8_t* record)
{
	if (field_d->field_type == FieldType::FIXED) {
		if (field_d->NBytes == 1 && record[0] == 0x80) {
			return true;
		}
		else {
			if (record[0] == 0x80) {
				for (size_t i = 1; i < field_d->NBytes; i++) {
					if (record[i] != 0x00) return false;
				}
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		// check if all bytes are 0xFF (null value in PCFAND)
		for (size_t i = 0; i < field_d->NBytes; i++) {
			if (record[i] != 0xFF) return false;
		}
	}
	return true;
}

std::string Fand0File::_extToT(const std::string& input_path)
{
	std::string dir, name, ext;
	FSplit(input_path, dir, name, ext);
	if (EquUpCase(ext, ".RDB")) ext = ".TTT";
	else if (EquUpCase(ext, ".DBF")) {
		if (TF->Format == FandTFile::FptFormat) {
			ext = ".FPT";
		}
		else {
			ext = ".DBT";
		}
	}
	else if (ext.length() > 1) {
		// at least '._'
		ext[1] = 'T';
	}
	else {
		ext = "___";
	}
	return dir + name + ext;
}

std::string Fand0File::_extToX(const std::string& dir, const std::string& name, std::string ext)
{
	ext[1] = 'X';
	return dir + name + ext;
}
