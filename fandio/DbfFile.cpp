#include "DbfFile.h"
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

	DBaseHeader* P = new DBaseHeader();
	//unsigned short n = 0;

	for (FieldDescr* F : file_d->FldD) {
		if (F->isStored()) {
			//n++;
			DBaseField* actual = new DBaseField();
			P->flds.push_back(actual);
			switch (F->field_type) {
			case FieldType::FIXED: { actual->Typ = 'N'; actual->Dec = F->M; break; }
			case FieldType::NUMERIC: { actual->Typ = 'N'; break; }
			case FieldType::ALFANUM: { actual->Typ = 'C'; break; }
			case FieldType::DATE: { actual->Typ = 'D'; break; }
			case FieldType::BOOL: { actual->Typ = 'L'; break; }
			case FieldType::TEXT: { actual->Typ = 'M'; break; }
			default:;
			}
			actual->Len = F->NBytes;
			actual->Displ = F->Displ;
			actual->Name = upperCaseString(F->Name).substr(0, 11);
		}
	}

	if (file_d->FF->TF != nullptr) {
		if (file_d->FF->TF->Format == FandTFile::FptFormat) P->Ver = 0xf5;
		else P->Ver = 0x83;
	}
	else {
		P->Ver = 0x03;
	}

	P->RecLen = file_d->FF->RecLen;
	SplitDate(Today(), d, m, y);
	P->Date[0] = static_cast<uint8_t>(y - 1900);
	P->Date[1] = static_cast<uint8_t>(m);
	P->Date[2] = static_cast<uint8_t>(d);
	P->NRecs = file_d->FF->NRecs;
	P->HdLen = file_d->FF->FirstRecPos;
	//PA[(P->HdLen / 32) * 32 + 1] = m;

	bool cached = file_d->FF->NotCached();

	WriteCache(file_d->FF->Handle, cached, 0, 1, &P->Ver);		// 1 byte
	WriteCache(file_d->FF->Handle, cached, 1, 3, P->Date);		// 3 bytes
	WriteCache(file_d->FF->Handle, cached, 4, 4, &P->NRecs);	// 4 bytes
	WriteCache(file_d->FF->Handle, cached, 8, 2, &P->HdLen);	// 2 bytes
	WriteCache(file_d->FF->Handle, cached, 10, 2, &P->RecLen);	// 2 bytes

	size_t index = 12;
	uint8_t zeros[11]{ 0,0,0,0,0,0,0,0,0,0,0 };

	for (DBaseField* F : P->flds) {
		size_t name_len = F->Name.length();
		WriteCache(file_d->FF->Handle, cached, index, name_len, (void*)F->Name.c_str());
		index += name_len;
		WriteCache(file_d->FF->Handle, cached, index, 11 - name_len, zeros);	// 11 bytes
		index += 11 - name_len;
		WriteCache(file_d->FF->Handle, cached, index, 1, &F->Typ);		// 1 byte
		index++;
		WriteCache(file_d->FF->Handle, cached, index, 4, &F->Displ);	// 4 bytes
		index += 4;
		WriteCache(file_d->FF->Handle, cached, index, 1, &F->Len);		// 1 byte
		index++;
		WriteCache(file_d->FF->Handle, cached, index, 1, &F->Dec);		// 1 byte
		index++;
	}

	WriteCache(file_d->FF->Handle, cached, P->NRecs * P->RecLen + P->HdLen, 1, (void*)&CtrlZ);

	delete P; P = nullptr;
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
		s = Fd.Name;

		switch (Fd.Typ)
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
		case 'A': { str(Fd.Len, s1); s = s + ',' + s1; break; }
		case 'F': {
			Fd.Len -= Fd.Dec;
			if (Fd.Dec != 0) Fd.Len--;
			str(Fd.Len, s1); s = s + ',' + s1; str(Fd.Dec, s1); s = s + '.' + s1;
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