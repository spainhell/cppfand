#include "DbfFile.h"

#include "DBaseHeader.h"
#include "../Core/FieldDescr.h"
#include "../Core/GlobalVariables.h"
#include "../Common/textfunc.h"
#include "../Core/Coding.h"
#include "../Core/DateTime.h"
#include "../Common/compare.h"
#include "../Core/obaseww.h"


DbfFile::DbfFile(FileD* parent)
{
	_parent = parent;
}

DbfFile::~DbfFile()
{
}

size_t DbfFile::ReadRec(size_t rec_nr, void* record)
{
	return ReadData((rec_nr - 1) * RecLen + FirstRecPos, RecLen, record);
}

size_t DbfFile::WriteRec(size_t rec_nr, void* record)
{
	WasWrRec = true;
	return WriteData((rec_nr - 1) * RecLen + FirstRecPos, RecLen, record);
}

void DbfFile::CreateRec(int n, void* record)
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

void DbfFile::DeleteRec(int n, void* record)
{
	DelAllDifTFlds(record, nullptr);
	for (int i = n; i <= NRecs - 1; i++) {
		ReadRec(i + 1, record);
		WriteRec(i, record);
	}
	DecNRecs(1);
}

void DbfFile::DelAllDifTFlds(void* record, void* comp_record)
{
	for (auto& F : _parent->FldD) {
		if (F->field_type == FieldType::TEXT && ((F->Flg & f_Stored) != 0)) {
			DelDifTFld(F, record, comp_record);
		}
	}
}

void DbfFile::IncNRecs(int n)
{
	NRecs += n;
	SetUpdateFlag();
}

void DbfFile::DecNRecs(int n)
{
	NRecs -= n;
	SetUpdateFlag();
	WasWrRec = true;
}

void DbfFile::PutRec(void* record, int& i_rec)
{
	NRecs++;
	WriteData(i_rec * RecLen + FirstRecPos, RecLen, record);
	i_rec++;
	Eof = true;
}

bool DbfFile::loadB(FieldDescr* field_d, void* record)
{
	uint8_t* CP = (uint8_t*)record + field_d->Displ;
	return (*CP == 'Y' || *CP == 'y' || *CP == 'T' || *CP == 't');
}

double DbfFile::loadR(FieldDescr* field_d, void* record)
{
	uint8_t* source = static_cast<uint8_t*>(record) + field_d->Displ;
	return DBF_RforD(field_d, source);
}

std::string DbfFile::loadS(FieldDescr* field_d, void* record)
{
	char* source = (char*)record + field_d->Displ;
	std::string S;
	int pos = 0;
	LockMode md;
	WORD l = field_d->L;
	switch (field_d->field_type)
	{
	case FieldType::ALFANUM: {		// znakovy retezec max. 255 znaku
		S = std::string(source, l);
		if (field_d->isEncrypted()) {
			S = Coding::Code(S);
		}
		if (!S.empty() && S[0] == '\0') {
			S = RepeatString(' ', l);
		}
		break;
	}
	case FieldType::NUMERIC: {		// ciselny retezec max. 79 znaku
		// not supported in DBF
		break;
	}
	case FieldType::TEXT: { // volny text max. 65k
		if (HasTWorkFlag(record)) {
			S = TWork.Read(loadT(field_d, record));
		}
		else {
			S = TF->Read(loadT(field_d, record));
		}
		if (field_d->isEncrypted()) {
			S = Coding::Code(S);
		}
		break;
	}
	}
	return S;
}

int DbfFile::loadT(FieldDescr* F, void* record)
{
	// tvarime se, ze CRecPtr je pstring ...
	// TODO: toto je asi blbe, nutno opravit pred 1. pouzitim
	//pstring* s = (pstring*)CRecPtr;
	//auto result = std::stoi(LeadChar(' ', *s));
	return 0; // result;
}

void DbfFile::saveB(FieldDescr* field_d, bool b, void* record)
{
	char* pB = (char*)record + field_d->Displ;
	if ((field_d->field_type == FieldType::BOOL) && ((field_d->Flg & f_Stored) != 0)) {
		if (b) *pB = 'T';
		else *pB = 'F';
	}
}

void DbfFile::saveR(FieldDescr* field_d, double r, void* record)
{
	BYTE* pRec = (BYTE*)record + field_d->Displ;

	if ((field_d->Flg & f_Stored) != 0) {
		WORD m = field_d->M;
		switch (field_d->field_type) {
		case FieldType::FIXED: {
			std::string s;
			if ((field_d->Flg & f_Comma) != 0) r = r / Power10[m];
			str(r, field_d->NBytes, field_d->M, s);
			memcpy(pRec, s.c_str(), s.length());
			break;
		}
		case FieldType::DATE: {
			std::string s = StrDate(r, "YYYYMMDD");
			memcpy(pRec, s.c_str(), s.length());
			break;
		}
		default:
			// other types are not allowed
			break;
		}
	}
}

void DbfFile::saveS(FileD* parent, FieldDescr* field_d, std::string s, void* record)
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
			// not suported in DBF
			break;
		}
		case FieldType::TEXT: {
			if (int previous = loadT(field_d, record)) {
				// there already exists a text -> delete it
				if (HasTWorkFlag(record)) {
					TWork.Delete(previous);
				}
				else {
					TF->Delete(previous);
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
					int pos = TF->Store(s);
					saveT(field_d, pos, record);
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


int DbfFile::saveT(FieldDescr* field_d, int pos, void* record)
{
	char* source = (char*)record + field_d->Displ;
	int* LP = (int*)source;
	if ((field_d->field_type == FieldType::TEXT) && field_d->isStored()) {
		if (pos == 0) {
			FillChar(source, 10, ' ');
		}
		else {
			pstring s;
			str(pos, s);
			memcpy(source, &s[1], 10);
		}
		return 0;
	}
	else {
		//RunError(906);
		return 906;
	}
}

void DbfFile::DelTFld(FieldDescr* field_d, void* record)
{
	int pos = loadT(field_d, record);
	if (pos == 0) return;

	if (HasTWorkFlag(record)) {
		TWork.Delete(pos);
	}
	else {
		TF->Delete(pos);
	}

	saveT(field_d, 0, record);
}

void DbfFile::DelTFlds(void* record)
{
	for (FieldDescr* field : _parent->FldD) {
		if (field->field_type == FieldType::TEXT && field->isStored()) {
			DelTFld(field, record);
		}
	}
}

void DbfFile::DelDifTFld(FieldDescr* field_d, void* record, void* comp_record)
{
	const int n1 = loadT(field_d, comp_record);
	const int n2 = loadT(field_d, record);
	if (n1 != n2) {
		DelTFld(field_d, record);
	}
}

uint16_t DbfFile::RdPrefix()
{
	// NRs - celkovy pocet zaznamu v souboru;
	// RLen - delka 1 zaznamu
	struct xD {
		uint8_t Ver = 0; uint8_t Date[3] = { 0,0,0 };
		int32_t NRecs = 0;
		uint16_t HdLen = 0; uint16_t RecLen = 0;
	} XD;

	uint16_t result = 0xffff;

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

	return result;
}

void DbfFile::WrPrefix()
{
	if (update_flag) {
		WriteHeader();
	}
}

void DbfFile::WrPrefixes()
{
	if (update_flag) {
		WrPrefix();
	}

	if (TF != nullptr && TF->HasUpdateFlag()) {
		TF->WrPrefix();
	}
}

void DbfFile::WriteHeader()
{
	unsigned short m, d, y;
	const char CtrlZ = '\x1a';
	const char CtrlD = '\x0d';

	DBaseHeader* dbf_header = new DBaseHeader();

	for (FieldDescr* F : _parent->FldD) {
		if (F->isStored()) {
			DBaseField* actual = new DBaseField();
			dbf_header->flds.push_back(actual);
			switch (F->field_type) {
			case FieldType::FIXED: { actual->typ = 'N'; actual->dec = F->M; break; }
			case FieldType::NUMERIC: { actual->typ = 'N'; break; }
			case FieldType::ALFANUM: { actual->typ = 'C'; break; }
			case FieldType::DATE: { actual->typ = 'D'; break; }
			case FieldType::BOOL: { actual->typ = 'L'; break; }
			case FieldType::TEXT: { actual->typ = 'M'; break; }
			default:;
			}
			actual->len = F->NBytes;
			actual->displ = F->Displ;
			actual->name = upperCaseString(F->Name).substr(0, 11);
		}
	}

	if (TF != nullptr) {
		if (TF->Format == DbfTFile::FptFormat) {
			dbf_header->Ver = 0xF5;
		}
		else {
			dbf_header->Ver = 0x83;
		}
	}
	else {
		dbf_header->Ver = 0x03;
	}

	dbf_header->RecLen = RecLen;
	SplitDate(Today(), d, m, y);
	dbf_header->Date[0] = static_cast<uint8_t>(y - 1900);
	dbf_header->Date[1] = static_cast<uint8_t>(m);
	dbf_header->Date[2] = static_cast<uint8_t>(d);
	dbf_header->NRecs = NRecs;
	dbf_header->HdLen = FirstRecPos;

	WriteData(0, dbf_header->GetDataLength(), dbf_header->GetData());

	size_t index = dbf_header->GetDataLength();

	for (DBaseField* F : dbf_header->flds) {
		WriteData(index, F->GetDataLength(), F->GetData());
		index += F->GetDataLength();
	}

	WriteData(index, 1, (void*)&CtrlD);
	WriteData(dbf_header->NRecs * dbf_header->RecLen + dbf_header->HdLen, 1, (void*)&CtrlZ);

	delete dbf_header;
	dbf_header = nullptr;
}

int DbfFile::MakeDbfDcl(std::string& name)
{
	DBaseHeader dbf_header;
	DBaseField dbf_field;
	char c = '\0';

	CPath = FExpand(name + ".DBF"); CVol = "";
	int i = catalog->GetCatalogIRec(name, true);

	if (i != 0) {
		CVol = catalog->GetVolume(i);
		CPath = FExpand(catalog->GetPathName(i));
		FSplit(CPath, CDir, CName, CExt);
	}

	HANDLE h = OpenH(CPath, _isOldFile, RdOnly);
	TestCPathError();
	ReadH(h, 32, &dbf_header);
	WORD n = (dbf_header.HdLen - 1) / 32 - 1;

	std::string result;

	for (i = 1; i <= n; i++) {
		ReadH(h, 32, &dbf_field);
		result += dbf_field.name;

		switch (dbf_field.typ) {
		case 'C': c = 'A'; break;
		case 'D': c = 'D'; break;
		case 'L': c = 'B'; break;
		case 'M': c = 'T'; break;
		case 'N':
		case 'F': c = 'F'; break;
		}
		result += ':';
		result += c;

		std::string s1;

		switch (c) {
		case 'A': {
			str(dbf_field.len, s1);
			result += ',' + s1;
			break;
		}
		case 'F': {
			dbf_field.len -= dbf_field.dec;
			if (dbf_field.dec != 0) {
				dbf_field.len--;
			}
			str(dbf_field.len, s1);
			result += ',' + s1;
			str(dbf_field.dec, s1);
			result += '.' + s1;
			break;
		}
		}
		result += ";\x0D\x0A"; // ^M + ^J
	}

	_parent->saveS(ChptTxt, result, CRecPtr);
	CloseH(&h);
	return 0;
}

void DbfFile::CompileRecLen()
{
	WORD l = 1;
	WORD n = 0;

	for (FieldDescr* F : _parent->FldD) {
		switch (F->field_type) {
		case FieldType::FIXED: {
			F->NBytes = F->L - 1;
			break;
		}
		case FieldType::DATE: {
			F->NBytes = 8;
			break;
		}
		case FieldType::TEXT: {
			F->NBytes = 10;
			break;
		}
		default: break;
		}

		if (F->isStored()) {
			F->Displ = l;
			l += F->NBytes;
			n++;
		}
	}

	RecLen = l;
	FirstRecPos = (n + 1) * 32 + 1;
}

int DbfFile::UsedFileSize() const
{
	int n = NRecs * RecLen + FirstRecPos + 1;
	return n;
}

void DbfFile::TruncFile()
{
	if (UMode == RdOnly) return;

	TruncF(Handle, HandleError, UsedFileSize());
	if (HandleError != 0) {
		FileMsg(_parent, 700 + HandleError, '0');
	}
	if (TF != nullptr) {
		TruncF(TF->Handle, HandleError, TF->UsedFileSize());
		if (HandleError != 0) {
			FileMsg(GetFileD(), 700 + HandleError, 'T');
			GetFileD()->Close();
			GoExit(MsgLine);
		}
	}
}

void DbfFile::SaveFile()
{
	WrPrefixes();
}

/// <summary>
/// Close the file with saving changes (prefixes, ...)
/// </summary>
void DbfFile::CloseFile()
{
	WrPrefixes();
	SaveCache(0, Handle);
	TruncFile();

	// close .DBT or .FTP text file
	if (TF != nullptr) {
		TF->CloseFile();
	}

	CloseClearH(&Handle);
	if (HandleError == 0) {
		Handle = nullptr;
		ClearUpdateFlag();
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

void DbfFile::Close()
{
	CloseH(&Handle);
	if (TF != nullptr) {
		CloseH(&TF->Handle);
	}
}

void DbfFile::SetTWorkFlag(void* record) const
{
	BYTE* p = (BYTE*)record;
	p[RecLen] = 1;
}

bool DbfFile::HasTWorkFlag(void* record) const
{
	BYTE* p = (BYTE*)record;
	const bool workFlag = p[RecLen] == 1;
	return workFlag;
}

void DbfFile::SetRecordUpdateFlag(void* record) const
{
	BYTE* p = (BYTE*)record;
	p[RecLen + 1] = 1;
}

void DbfFile::ClearRecordUpdateFlag(void* record) const
{
	BYTE* p = (BYTE*)record;
	p[RecLen + 1] = 0;
}

bool DbfFile::HasRecordUpdateFlag(void* record) const
{
	BYTE* p = (BYTE*)record;
	return p[RecLen + 1] == 1;
}

bool DbfFile::DeletedFlag(void* record)
{
	if (((BYTE*)record)[0] != '*') return false;
	else return true;
}

void DbfFile::ClearDeletedFlag(void* record)
{
	BYTE* ptr = (BYTE*)record;
	ptr[0] = ' ';
}

void DbfFile::SetDeletedFlag(void* record)
{
	BYTE* ptr = (BYTE*)record;
	ptr[0] = '*';
}

FileD* DbfFile::GetFileD()
{
	return _parent;
}

void DbfFile::ClearUpdateFlag()
{
	DataFileBase::ClearUpdateFlag();
}

std::string DbfFile::SetTempCExt(char typ, bool isNet) const
{
	char Nr;
	if (typ == 'T') {
		Nr = '2';
		CExt = ".DBT";
	}
	else {
		Nr = '1';
		CExt = ".DBF";
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

double DbfFile::DBF_RforD(FieldDescr* field_d, uint8_t* source)
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

std::string DbfFile::_extToT(const std::string& input_path)
{
	std::string dir, name, ext;
	FSplit(input_path, dir, name, ext);

	if (TF->Format == DbfTFile::FptFormat) {
		ext = ".FPT";
	}
	else {
		ext = ".DBT";
	}

	return dir + name + ext;
}
