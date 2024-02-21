#include "CodingRdb.h"

#include <memory>

#include "../Core/compile.h"
#include "../Core/Coding.h"
#include "../Core/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/oaccess.h"
#include "../Core/obaseww.h"
#include "../Core/RunMessage.h"
#include "../DataEditor/rdedit.h"
#include "../DataEditor/runedi.h"
#include "../MergeReport/Merge.h"


void CodingRdb::CodeRdb(bool Rotate)
{
	//WORD pos;
	std::string s;
	FileD* cf = CFile;
	void* cr = CRecPtr;
	CFile = Chpt;

	CRecPtr = CFile->GetRecSpace();
	RunMsgOn('C', CFile->FF->NRecs);
	WORD irec = ChptTF->IRec;
	bool compileAll = ChptTF->CompileAll;
	for (int i = 1; i <= CFile->FF->NRecs; i++) {
		CFile->ReadRec(i, CRecPtr);
		RunMsgN(i);
		s = CFile->loadS(ChptTyp, CRecPtr);
		SetMsgPar(CFile->loadS(ChptName, CRecPtr));
		if (Rotate && (s[0] == ' ' || s[0] == 'I')) {}
		else {
			CodeF(Rotate, i, ChptTxt, s[0]);
			CodeF(Rotate, i, ChptOldTxt, s[0]);
			CFile->WriteRec(i, CRecPtr);
		}
	}
	if (Rotate) {
		int i = 1;
		while (i <= CFile->FF->NRecs) {
			CFile->ReadRec(i, CRecPtr);
			s = CFile->loadS(ChptTyp, CRecPtr);
			if (s[0] == ' ' || s[0] == 'I') {
				CFile->DeleteRec(i, CRecPtr);
			}
			else {
				i++;
			}
		}
	}
	RunMsgOff();
	ReleaseStore(&CRecPtr);
	CFile = cf;
	CRecPtr = cr;
	CompressCRdb();
	ChptTF->IRec = irec;
	ChptTF->CompileAll = compileAll;
}

void CodingRdb::CompressTxt(WORD IRec, LongStr* s, char Typ)
{
	//WORD i;
	WORD n;
	bool b;
	//CompInpD* ci;
	//LongStr* s2;
	void* p2 = nullptr;

	InpArrLen = s->LL;
	InpArrPtr = (BYTE*)s->A;
	PrevCompInp.clear();
	if (InpArrLen == 0) {
		ForwChar = 0x1A;
	}
	else {
		ForwChar = InpArrPtr[1];
	}
	CurrPos = 1;
	SwitchLevel = 0;
	void* cr = CRecPtr;
	ss = new LongStr(MaxLStrLen + 2);
	MarkStore(p2);
	l = 0;
	if (Typ == 'E') {
	label0:
		while (!(ForwChar == '#' || ForwChar == 0x1A || ForwChar == '\r' || ForwChar == '{')) {
			// { read headlines }
			Wr(ForwChar);
			ReadChar();
		}
		switch (ForwChar) {
		case 0x1A:
		case '#': {
			goto label1;
			break;
		}
		case '{': {
			SkipBlank(true);
			Wr('{');
			Wr('}');
			break;
		}
		default: {
			ReadChar();
			if (ForwChar == '\n') ReadChar();
			break;
		}
		}
		Wr('\r'); Wr('\n');
		goto label0;
	}
label1:
	switch (ForwChar) {
	case '0x1A': {
		if (!PrevCompInp.empty()) {
			PrevCompInp.pop_back();
			if (CurrPos <= InpArrLen) ForwChar = InpArrPtr[CurrPos];
			goto label1;
		}
		else {
			s->LL = l;
			MyMove(ss->A, s->A, l);
			delete s; s = nullptr;
			ReleaseStore(&p2);
			CRecPtr = cr;
			return;
		}
		break;
	}
	case '{': {
		ReadChar();
		if (ForwChar == '$') {
			n = RdDirective(b);
			switch (n) {
			case 0:;
			case 1: {
				SwitchLevel++;
				if (!b) SkipLevel(true);
			}
			case 5: {
				PrevCompInp.emplace_back(CompInpD());
				SetInpTT(&ChptIPos, true);
			}

			default: {
				if (n == 3) SkipLevel(false);
				else SwitchLevel--;
			}
			}
			goto label1;
		}
		else {
			n = 1;
		label2:
			switch (ForwChar) {
			case '{': n++;
			case '}': {
				n--;
				if (n == 0) {
					Wr(' ');
					ReadChar();
					goto label1;
				}
			}
			}
			ReadChar();
			goto label2;
		}
		break;
	}
	case '\'': {
		do {
			Wr(ForwChar);
			ReadChar();
		} while (ForwChar == '\'' || ForwChar == 0x1A);
		if (ForwChar == 0x1A) goto label1;
		break;
	}
	default: {
		if (ForwChar <= ' ') { // ^@..' '
			if (!(Typ == 'R' || Typ == 'U' || Typ == 'E' || Typ == 'H')) {
				while ((ForwChar <= ' ') && (ForwChar != 0x1A)) ReadChar();
				Wr(' ');
				goto label1;
			}
		}
		break;
	}
	}
	Wr(ForwChar);
	ReadChar();
	goto label1;
}

void CodingRdb::Wr(BYTE c)
{
	if (l >= MaxLStrLen) RunError(661);
	l++;
	ss->A[l] = (char)c;
}

void CodingRdb::CodeF(bool rotate, WORD IRec, FieldDescr* F, char Typ)
{
	void* p = nullptr;
	void* p2 = nullptr;

	int pos = CFile->loadT(F, CRecPtr);
	if (pos == 0) return;
	MarkBoth(p, p2);
	LongStr* s = ChptTF->Read(pos);
	WORD l = s->LL;
	ChptTF->Delete(pos);
	if (l == 0) goto label2;
	if (rotate) {
		if (F == ChptOldTxt) {
			if (Typ == 'F') ((FileD*)s->A)->TxtPosUDLI = posUDLI;
		}
		else {
			CompressTxt(IRec, s, Typ);
			l = s->LL;
			if (l == 0) goto label2;
			if (l > MaxLStrLen) RunError(661);
			if (Typ == 'F') {
				posUDLI = 0;
				bool b = true;
				for (WORD i = 1; i <= l - 1; i++) {
					switch (s->A[i]) {
					case '#': {
						char c2 = s->A[i + 1];
						if (b && (c2 == 'U' || c2 == 'D' || c2 == 'L' || c2 == 'I')) {
							posUDLI = i;
							goto label1;
							break;
						}
					}
					case '\'': {
						b = !b;
						break;
					}
					}
				}
			label1: {}
			}
		}

		std::string plain = std::string(s->A, s->LL);
		std::string coded = Coding::XEncode(plain);

		delete s; s = nullptr;
		s = new LongStr(coded.length());
		memcpy(s->A, coded.c_str(), coded.length());
	}
	else {
		Coding::Code(s->A, l);
	}
label2:
	CFile->saveLongS(F, s, CRecPtr);
	ReleaseStore(&p);
	ReleaseStore(&p2);
}

void CodingRdb::CompressCRdb()
{
	void* p = nullptr;
	MarkStore(p);
	void* cr = Chpt->FF->RecPtr;
	std::string s = "#I1_" + Chpt->Name + "#O1_" + Chpt->Name;
	SetInpStr(s);
	SpecFDNameAllowed = true;

	const std::unique_ptr merge = std::make_unique<Merge>();
	merge->Read();
	SpecFDNameAllowed = false;
	merge->Run();

	SaveAndCloseAllFiles();
	ReleaseStore(&p);
	Chpt->FF->RecPtr = cr;
	CFile = Chpt;
	CRecPtr = E->NewRecPtr;
	CFile->ReadRec(CRec(), CRecPtr);

	ChptTF->CompileAll = false;
	ChptTF->CompileProc = false;
	SetUpdHandle(ChptTF->Handle);
}
