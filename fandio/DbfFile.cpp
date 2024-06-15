#include "DbfFile.h"
#include "../Common/FileEnums.h"
#include "../Core/FieldDescr.h"
#include "../Core/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/obaseww.h"
#include "../pascal/real48.h"
#include "../Common/textfunc.h"
#include "../Core/DateTime.h"

void DbfFile::WrDBaseHd()
{
	unsigned short m, d, w;
	const char CtrlZ = '\x1a';

	DBaseHd* P = new DBaseHd();
	char* PA = (char*)&P; // PA:CharArrPtr absolute P;
	unsigned short n = 0;
	//FieldDescr* F = CFile->FldD.front();
	//while (F != nullptr) {
	for (FieldDescr* F : CFile->FldD) {
		if ((F->Flg & f_Stored) != 0) {
			n++;
			{ // with P^.Flds[n]
				auto actual = P->Flds[n];
				switch (F->field_type) {
				case FieldType::FIXED: { actual.Typ = 'N'; actual.Dec = F->M; break; }
				case FieldType::NUMERIC: { actual.Typ = 'N'; break; }
				case FieldType::ALFANUM: { actual.Typ = 'C'; break; }
				case FieldType::DATE: { actual.Typ = 'D'; break; }
				case FieldType::BOOL: { actual.Typ = 'L'; break; }
				case FieldType::TEXT: { actual.Typ = 'M'; break; }
				default:;
				}
				actual.Len = F->NBytes;
				actual.Displ = F->Displ;
				pstring s = F->Name;
				for (size_t i = 1; i <= s.length(); i++) {
					s[i] = toupper(s[i]);
				}
				StrLPCopy((char*)&actual.Name[1], s, 11);
			}
		}
		//F = F->pChain;
	}

	unsigned short y;
	if (CFile->FF->TF != nullptr) {
		if (CFile->FF->TF->Format == FandTFile::FptFormat) P->Ver = 0xf5;
		else P->Ver = 0x83;
	}
	else {
		P->Ver = 0x03;
	}

	P->RecLen = CFile->FF->RecLen;
	SplitDate(Today(), d, m, y);
	P->Date[1] = unsigned char(y - 1900);
	P->Date[2] = (unsigned char)m;
	P->Date[3] = (unsigned char)d;
	P->NRecs = CFile->FF->NRecs;
	P->HdLen = CFile->FF->FirstRecPos;
	PA[(P->HdLen / 32) * 32 + 1] = m;

	bool cached = CFile->FF->NotCached();
	RdWrCache(WRITE, CFile->FF->Handle, cached, 0, CFile->FF->FirstRecPos, (void*)&P);
	RdWrCache(WRITE, CFile->FF->Handle, cached,
		CFile->FF->NRecs * CFile->FF->RecLen + CFile->FF->FirstRecPos, 1, (void*)&CtrlZ);

	delete P; P = nullptr;
}
