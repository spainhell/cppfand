#include "Fand0File.h"

#include "DbfFile.h"
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
// #include "../Core/RunMessage.h"
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

void Fand0File::CompileRecLen()
{
	WORD l = 0;
	WORD n = 0;
	if (file_type == FandFileType::INDEX) {
		l = 1;
	}

	for (FieldDescr* F : _parent->FldD) {
		if (file_type == FandFileType::FAND8 && F->field_type == FieldType::DATE) {
			F->NBytes = 2;
		}

		if (F->isStored()) {
			F->Displ = l;
			l += F->NBytes;
			n++;
		}
	}

	RecLen = l;

	if (file_type == FandFileType::FAND8) {
		FirstRecPos = 4;
	}
	else {
		FirstRecPos = 6;
	}
}

int Fand0File::UsedFileSize() const
{
	int n = NRecs * RecLen + FirstRecPos;
	return n;
}

bool Fand0File::IsShared()
{
	return (UMode == Shared) || (UMode == RdShared);
}

void Fand0File::Reset()
{
	RecLen = 0;
	RecPtr = nullptr;
	NRecs = 0;
	FirstRecPos = 0;
	WasWrRec = false; WasRdOnly = false; Eof = false;
	file_type = FandFileType::UNKNOWN;        // 8= Fand 8; 6= Fand 16; X= .X; 0= RDB; C= CAT 
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
	if (file_type == FandFileType::INDEX) {
		XF->SetUpdateFlag(); //SetUpdHandle(XF->Handle);
	}
}

void Fand0File::DecNRecs(int n)
{
	NRecs -= n;
	SetUpdateFlag(); //SetUpdHandle(Handle);
	if (file_type == FandFileType::INDEX) {
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

bool Fand0File::loadB(FieldDescr* field_d, void* record)
{
	bool result = false;
	uint8_t* CP = (uint8_t*)record + field_d->Displ;

	if ((*CP == '\0') || (*CP == 0xFF)) { //x0FF is NULL value
		result = false;
	}
	else {
		result = true;
	}

	return result;
}

double Fand0File::loadR(FieldDescr* field_d, void* record)
{
	uint8_t* source = static_cast<uint8_t*>(record) + field_d->Displ;
	double result = 0.0;

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
		if (file_type == FandFileType::FAND8) {
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
	char* source = (char*)record + F->Displ;
	if (record == nullptr) return 0;
	return *reinterpret_cast<int*>(source);
}

void Fand0File::saveB(FieldDescr* field_d, bool b, void* record)
{
	char* pB = (char*)record + field_d->Displ;
	if ((field_d->field_type == FieldType::BOOL) && field_d->isStored()) {
		*pB = b ? 1 : 0;
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
			if ((field_d->Flg & f_Comma) == 0) r = r * Power10[m];
			FixFromReal(r, pRec, field_d->NBytes);
			break;
		}
		case FieldType::DATE: {
			switch (file_type) {
			case FandFileType::FAND8: {
				if (trunc(r) == 0) *(long*)&pRec = 0;
				else *(long*)pRec = trunc(r - FirstDate);
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
			if (int previous = loadT(field_d, record)) {
				// there already exists a text -> delete it
				if (HasTWorkFlag(record)) {
					TWork.Delete(previous);
				}
				else {
					LockMode md = parent->NewLockMode(WrMode);
					TF->Delete(previous);
					parent->OldLockMode(md);
				}
			}

			if (s.empty()) {
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
	if ((field_d->field_type == FieldType::TEXT) && field_d->isStored()) {
		*LP = pos;
		return 0;
	}
	else {
		//RunError(906);
		return 906;
	}
}

void Fand0File::DelTFld(FieldDescr* field_d, void* record)
{
	int pos = loadT(field_d, record);
	if (pos == 0) return;

	if (HasTWorkFlag(record)) {
		TWork.Delete(pos);
	}
	else {
		LockMode md = _parent->NewLockMode(WrMode);
		TF->Delete(pos);
		_parent->OldLockMode(md);
	}

	saveT(field_d, 0, record);
}

void Fand0File::DelTFlds(void* record)
{
	for (FieldDescr* field : _parent->FldD) {
		if (field->field_type == FieldType::TEXT && field->isStored()) {
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

	uint16_t result = 0xffff;

	switch (file_type) {
	case FandFileType::FAND8: {
		ReadData(0, 2, &X8.NRs);
		ReadData(2, 2, &X8.RLen);
		NRecs = X8.NRs;
		if (RecLen != X8.RLen) { return X8.RLen; }
		break;
	}
	default: {
		ReadData(0, 4, &X6.NRs);
		ReadData(4, 2, &X6.RLen);
		NRecs = abs(X6.NRs);
		if ((X6.NRs < 0) && (file_type != FandFileType::INDEX) || (X6.NRs > 0) && (file_type == FandFileType::INDEX)
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

	if (update_flag) {
		// const bool not_cached = NotCached();
		switch (file_type) {
		case FandFileType::FAND8: {
			Pfx8.RLen = RecLen;
			Pfx8.NRs = static_cast<unsigned short>(NRecs);
			WriteData(0, 2, &Pfx8.NRs);
			WriteData(2, 2, &Pfx8.RLen);
			break;
		}
		default: {
			Pfx6.RLen = RecLen;
			if (file_type == FandFileType::INDEX) Pfx6.NRs = -NRecs;
			else Pfx6.NRs = NRecs;
			WriteData(0, 4, &Pfx6.NRs);
			WriteData(4, 2, &Pfx6.RLen);
		}
		}
	}
}

void Fand0File::WrPrefixes()
{
	if (update_flag) {
		WrPrefix();
	}

	if (TF != nullptr && TF->HasUpdateFlag()) {
		TF->WrPrefix();
	}

	if (file_type == FandFileType::INDEX
		&& XF->Handle != nullptr
		/*{ call from CopyDuplF }*/
		&& (XF->HasUpdateFlag() || update_flag)) {
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
	if (file_type == FandFileType::INDEX) {
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

	if (file_type == FandFileType::INDEX) XF->NoCreate = true;
	if (TF != nullptr) TF->SetEmpty();
	return result;
}

void Fand0File::ClearUpdateFlag()
{
	update_flag = false;

	if (file_type == FandFileType::INDEX) {
		XF->ClearUpdateFlag();
	}

	if (TF != nullptr) {
		TF->ClearUpdateFlag();
	}
}

void Fand0File::SaveFile()
{
	WrPrefixes();
	if (file_type == FandFileType::INDEX) {
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
/// Close the file with saving changes (prefixes, ...)
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
	if (file_type == FandFileType::INDEX && XF != nullptr) {
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

	if (!IsShared() && (NRecs == 0)) {
		std::string path = _parent->SetPathAndVolume();
		MyDeleteFile(path);
	}

	if (WasRdOnly) {
		WasRdOnly = false;
		std::string path = _parent->SetPathAndVolume();
		SetFileAttr(path, HandleError, (GetFileAttr(CPath, HandleError) & 0x27) | 0x01); // {RdOnly; }
		if (TF != nullptr) {
			path = _parent->CExtToT(CDir, CName, CExt);
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
	if (file_type == FandFileType::INDEX) {
		CloseH(&XF->Handle);
	}
	if (TF != nullptr) {
		CloseH(&TF->Handle);
	}
}

void Fand0File::SetTWorkFlag(void* record) const
{
	BYTE* p = (BYTE*)record;
	p[RecLen] = 1;
}

bool Fand0File::HasTWorkFlag(void* record) const
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
	if (file_type == FandFileType::INDEX) {
		if (((BYTE*)record)[0] == 0) return false;
		else return true;
	}

	return false;
}

void Fand0File::ClearDeletedFlag(void* record)
{
	BYTE* ptr = (BYTE*)record;
	if (file_type == FandFileType::INDEX) {
		ptr[0] = 0;
	}
}

void Fand0File::SetDeletedFlag(void* record)
{
	BYTE* ptr = (BYTE*)record;
	if (file_type == FandFileType::INDEX) {
		ptr[0] = 1;
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
			std::unique_ptr<XScan> scan = std::make_unique<XScan>(_parent, nullptr, empty, false);
			std::unique_ptr<uint8_t[]> record = _parent->GetRecSpaceUnique();
			scan->Reset(nullptr, false, record.get());
			std::unique_ptr<XWorkFile> XW = std::make_unique<XWorkFile>(_parent, scan.get(), _parent->Keys);
			XW->Main('X', record.get());
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
			_parent->CFileError(819);
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
	if (file_type == FandFileType::INDEX && !K.empty()) {
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

void Fand0File::GenerateNew000File(XScan* x, void* record, void (*msgFuncUpdate)(int32_t))
{
	// vytvorime si novy buffer pro data,
	// ten pak zapiseme do souboru naprimo (bez cache)

	const unsigned short header000len = 6; // 4B pocet zaznamu, 2B delka jednoho zaznamu
	// z puvodniho .000 vycteme pocet zaznamu a jejich delku
	const size_t totalLen = x->FD->FF->NRecs * x->FD->FF->RecLen + header000len;
	unsigned char* buffer = new unsigned char[totalLen] { 0 };
	size_t offset = header000len; // zapisujeme nejdriv data; hlavicku az nakonec

	while (!x->eof) {
		if (msgFuncUpdate) {
			msgFuncUpdate(x->IRec);
		}
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
	std::vector<XKey*> xw_keys;
	xw_keys.push_back(K);
	std::unique_ptr<uint8_t[]> record = _parent->GetRecSpaceUnique();
	std::unique_ptr<XWorkFile> XW = std::make_unique<XWorkFile>(_parent, Scan, xw_keys);
	XW->Main(Typ, record.get());
}

void Fand0File::ScanSubstWIndex(XScan* Scan, std::vector<KeyFldD*>& SK, char Typ)
{
	unsigned short n = 0;
	XWKey* k2 = new XWKey(_parent);

	if (Scan->FD->IsSQLFile && (Scan->Kind == 3)) {
		/* F6-autoreport & sort */
		XKey* k = Scan->Key;
		n = k->IndexLen;

		for (KeyFldD* kf : SK) {
			n += kf->FldD->NBytes;
		}

		if (n > 255) {
			WrLLF10Msg(155);
			delete k2; k2 = nullptr;
			return;
		}

		std::vector<KeyFldD*> kfroot;

		for (KeyFldD* kf : k->KFlds) {
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

void Fand0File::SortAndSubst(std::vector<KeyFldD*>& SK, void (*msgFuncOn)(int8_t, int32_t), void (*msgFuncUpdate)(int32_t), void (*msgFuncOff)())
{
	std::unique_ptr<uint8_t[]> record = _parent->GetRecSpaceUnique();

	std::vector<KeyInD*> empty;
	XScan* Scan = new XScan(_parent, nullptr, empty, false);
	Scan->Reset(nullptr, false, record.get());
	ScanSubstWIndex(Scan, SK, 'S');
	FileD* FD2 = _parent->OpenDuplicateF(false);

	if (msgFuncOn) {
		msgFuncOn('S', Scan->NRecs);
	}

	Scan->GetRec(record.get());

	// write data to a file .100
	FD2->FF->GenerateNew000File(Scan, record.get(), msgFuncUpdate);

	SubstDuplF(FD2, false);
	Scan->Close();

	if (msgFuncOff) {
		msgFuncOff();
	}

	delete FD2; FD2 = nullptr;
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
	int result = XFNotValid();
	if (result != 0) {
		RunError(result);
	}

	std::string orig_path = _parent->SetPathAndVolume();
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

	// rename temp file to regular
	std::string temp_path = SetTempCExt('0', false);
	SaveCache(0, TempFD->FF->Handle);
	CloseClearH(&TempFD->FF->Handle);
	RenameFile56(temp_path, orig_path, true);
	Handle = OpenH(orig_path, _isOldFile, UMode);
	_parent->FullPath = orig_path;
	SetUpdateFlag(); //SetUpdHandle(Handle);

	if ((TempFD->FF->TF != nullptr) && DelTF) {
		CloseClearH(&TF->Handle);
		MyDeleteFile(orig_path_T);
		TestDelErr(orig_path_T);
		//*parent_tf = *ref_to_parent->FF->TF;
		//ref_to_parent->FF->TF = parent_tf;
		CloseClearH(&TempFD->FF->TF->Handle);
		std::string temp_path_t = SetTempCExt('T', false);
		RenameFile56(temp_path_t, orig_path_T, true);
		TF->Handle = OpenH(orig_path_T, _isOldFile, UMode);
		SetUpdateFlag(); //SetUpdHandle(TF->Handle);
		//if (orig.TF != nullptr) TF = new FandTFile(*orig.TF, this);
	}

	RdPrefixes();
}

void Fand0File::CopyDuplF(FileD* TempFD, bool DelTF)
{
	TempFD->FF->WrPrefixes();
	SaveCache(0, Handle);
	SetTempCExt('0', true);
	FileD::CopyH(TempFD->FF->Handle, Handle);

	// TempFD has been deleted in CopyH -> set Handle to nullptr
	TempFD->FF->Handle = nullptr;
	TempFD->FF->ClearUpdateFlag();

	if ((TF != nullptr) && DelTF) {
		HANDLE h1 = TempFD->FF->TF->Handle;
		HANDLE h2 = TF->Handle;
		SetTempCExt('T', true);
		*TF = *TempFD->FF->TF;
		TF->Handle = h2;
		FileD::CopyH(h1, h2);
	}
	int rp = RdPrefixes();
	if (rp != 0) {
		_parent->CFileError(rp);
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
		int xf_res = XFNotValid();
		if (xf_res != 0) {
			RunError(xf_res);
		}
		delete FD2; FD2 = nullptr;
	}
	XF->NoCreate = false;
	TestXFExist();
	_parent->OldLockMode(md);
	SaveFiles();
	delete[] record; record = nullptr;
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

std::string Fand0File::SetTempCExt(char typ, bool isNet) const
{
	char Nr;
	if (typ == 'T') {
		Nr = '2';
		switch (file_type) {
		case FandFileType::RDB: CExt = ".TTT"; break;
		default:;
		}
	}
	else {
		Nr = '1';
		switch (file_type) {
		case FandFileType::RDB: CExt = ".RDB"; break;
		default:;
		}
	}

	if (CExt.length() < 2) CExt = ".0";
	CExt[1] = Nr;

	if (isNet) {
		CPath = WrkDir + CName + CExt; /* work files are local */
	}
	else {
		CPath = CDir + CName + CExt;
	}

	return CPath;
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
	/*else if (EquUpCase(ext, ".DBF")) {
		if (TF->Format == FandTFile::FptFormat) {
			ext = ".FPT";
		}
		else {
			ext = ".DBT";
		}
	}*/
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
