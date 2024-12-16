#include "DbfFile.h"

#include "DBaseHeader.h"
#include "../Common/FileEnums.h"
#include "../Core/FieldDescr.h"
#include "../Core/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/obaseww.h"
#include "../Common/textfunc.h"
#include "../Core/DateTime.h"

void DbfFile::WrDBaseHd(FileD* file_d)
{
	unsigned short m, d, y;
	const char CtrlZ = '\x1a';
	const char CtrlD = '\x0d';

	DBaseHeader* dbf_header = new DBaseHeader();
	//unsigned short n = 0;

	for (FieldDescr* F : file_d->FldD) {
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

	if (file_d->FF->TF != nullptr) {
		if (file_d->FF->TF->Format == FandTFile::FptFormat) dbf_header->Ver = 0xf5;
		else dbf_header->Ver = 0x83;
	}
	else {
		dbf_header->Ver = 0x03;
	}

	dbf_header->RecLen = file_d->FF->RecLen;
	SplitDate(Today(), d, m, y);
	dbf_header->Date[0] = static_cast<uint8_t>(y - 1900);
	dbf_header->Date[1] = static_cast<uint8_t>(m);
	dbf_header->Date[2] = static_cast<uint8_t>(d);
	dbf_header->NRecs = file_d->FF->NRecs;
	dbf_header->HdLen = file_d->FF->FirstRecPos;

	bool not_cached = file_d->FF->NotCached();

	WriteCache(file_d->FF, not_cached, 0, dbf_header->GetDataLength(), dbf_header->GetData());


	size_t index = dbf_header->GetDataLength();

	for (DBaseField* F : dbf_header->flds) {
		WriteCache(file_d->FF, not_cached, index, F->GetDataLength(), F->GetData());
		index += F->GetDataLength();
	}

	WriteCache(file_d->FF, not_cached, index, 1, (void*)&CtrlD);

	WriteCache(file_d->FF, not_cached, dbf_header->NRecs * dbf_header->RecLen + dbf_header->HdLen, 1, (void*)&CtrlZ);

	delete dbf_header;
	dbf_header = nullptr;
}

int DbfFile::MakeDbfDcl(pstring Nm)
{
	DBaseHeader Hd; DBaseField Fd;
	char c;
	pstring s(80);
	pstring s1(10);

	CPath = FExpand(Nm + ".DBF"); CVol = "";
	int i = catalog->GetCatalogIRec(Nm, true);

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