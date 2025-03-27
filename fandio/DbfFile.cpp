#include "DbfFile.h"

#include "DBaseHeader.h"
#include "files.h"
#include "../Core/FieldDescr.h"
#include "../Core/GlobalVariables.h"
#include "../Common/textfunc.h"
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
	//unsigned short n = 0;

	for (FieldDescr* F : _parent->FldD) {
		if (F->isStored()) {
			//n++;
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

	//if (TF != nullptr) {
	//	if (TF->Format == DbfTFile::FptFormat) dbf_header->Ver = 0xF5;
	//	else dbf_header->Ver = 0x83;
	//}
	//else {
	//	dbf_header->Ver = 0x03;
	//}

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
	DBaseHeader Hd; DBaseField Fd;
	char c = '\0';
	pstring s(80);
	pstring s1(10);

	CPath = FExpand(name + ".DBF"); CVol = "";
	int i = catalog->GetCatalogIRec(name, true);

	if (i != 0) {
		CVol = catalog->GetVolume(i);
		CPath = FExpand(catalog->GetPathName(i));
		FSplit(CPath, CDir, CName, CExt);
	}

	HANDLE h = OpenH(CPath, _isOldFile, RdOnly);
	TestCPathError();
	ReadH(h, 32, &Hd);
	WORD n = (Hd.HdLen - 1) / 32 - 1;
	LongStr* t = new LongStr(2);
	t->LL = 0;

	for (i = 1; i <= n; i++) {
		ReadH(h, 32, &Fd);
		s = Fd.name;

		switch (Fd.typ)
		{
		case 'C': c = 'A'; break;
		case 'D': c = 'D'; break;
		case 'L': c = 'B'; break;
		case 'M': c = 'T'; break;
		case 'N':
		case 'F': c = 'F'; break;
		}
		s = s + ':' + c;

		switch (c) {
		case 'A': { str(Fd.len, s1); s = s + ',' + s1; break; }
		case 'F': {
			Fd.len -= Fd.dec;
			if (Fd.dec != 0) Fd.len--;
			str(Fd.len, s1); s = s + ',' + s1; str(Fd.dec, s1); s = s + '.' + s1;
			break;
		}
		}
		s = s + ';' + 0x0D + 0x0A; // ^M + ^J
		BYTE* p = new BYTE[s.length()];
		Move(&s[1], p, s.length());
		t->LL += s.length();
	}
	CFile->saveLongS(ChptTxt, t, CRecPtr);
	CloseH(&h);
	return 0;
}

int DbfFile::UsedFileSize() const
{
	int n = NRecs * RecLen + FirstRecPos + 1;
	return n;
}

void DbfFile::TruncFile()
{
	//if (UMode == RdOnly) return;

	TruncF(Handle, HandleError, UsedFileSize());
	if (HandleError != 0) {
		FileMsg(_parent, 700 + HandleError, '0');
	}
	if (TF != nullptr) {
		TruncF(TF->Handle, HandleError, TF->UsedFileSize());
		if (HandleError != 0) {
			FileMsg(GetFileD(), 700 + HandleError, 'T');
			CloseGoExit(GetFileD()->FF);
		}
	}
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
		std::string path = SetPathAndVolume(_parent);
		SetFileAttr(path, HandleError, (GetFileAttr(CPath, HandleError) & 0x27) | 0x01); // {RdOnly; }
		if (TF != nullptr) {
			path = _parent->CExtToT(CDir, CName, CExt);
			SetFileAttr(path, HandleError, (GetFileAttr(CPath, HandleError) & 0x27) | 0x01); //  {RdOnly; }
		}
	}
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
	FandFile::ClearUpdateFlag();
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
