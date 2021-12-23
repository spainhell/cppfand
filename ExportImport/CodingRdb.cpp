#include "CodingRdb.h"
#include "../cppfand/compile.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"
#include "../Editor/rdedit.h"
#include "../Editor/runedi.h"
#include "../MergeReport/rdmerg.h"
#include "../MergeReport/runmerg.h"


void CodingRdb::CodeRdb(bool Rotate)
{
	WORD i, irec, pos; FileDPtr cf; void* cr; std::string s; bool compileAll;
	cf = CFile; cr = CRecPtr; CFile = Chpt;

	CRecPtr = GetRecSpace();
	RunMsgOn('C', CFile->NRecs);
	irec = ChptTF->IRec;
	compileAll = ChptTF->CompileAll;
	for (i = 1; i <= CFile->NRecs; i++) {
		ReadRec(CFile, i, CRecPtr);
		RunMsgN(i);
		s = _ShortS(ChptTyp);
		SetMsgPar(_ShortS(ChptName));
		if (Rotate && (s[0] == ' ' || s[0] == 'I')) {}
		else {
			CodeF(Rotate, i, ChptTxt, s[0]);
			CodeF(Rotate, i, ChptOldTxt, s[0]);
			WriteRec(CFile, i, CRecPtr);
		}
	}
	if (Rotate) {
		i = 1;
		while (i <= CFile->NRecs) {
			ReadRec(CFile, i, CRecPtr);
			s = _ShortS(ChptTyp);
			if (s[0] == ' ' || s[0] == 'I') DeleteRec(i);
			else i++;
		}
	}
	RunMsgOff();
	ReleaseStore(CRecPtr);
	CFile = cf;
	CRecPtr = cr;
	CompressCRdb();
	ChptTF->IRec = irec;
	ChptTF->CompileAll = compileAll;
}

void CodingRdb::CompressTxt(WORD IRec, LongStr* s, char Typ)
{
	WORD i, n; bool b; CompInpD* ci; void* cr; LongStr* s2;
	void* p2 = nullptr;

	InpArrLen = s->LL;
	InpArrPtr = (BYTE*)s->A;
	PrevCompInp.clear();
	if (InpArrLen == 0) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[1];
	CurrPos = 1;
	SwitchLevel = 0;
	cr = CRecPtr;
	ss = new LongStr(MaxLStrLen + 2); // GetStore(MaxLStrLen + 2);
	MarkStore2(p2);
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
			ReleaseAfterLongStr(s);
			ReleaseStore2(p2);
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
	void* p = nullptr, * p2 = nullptr;

	longint pos = _T(F);
	if (pos == 0) return;
	MarkBoth(p, p2);
	LongStr* s = ChptTF->Read(1, pos);
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
		LongStr* s2 = new LongStr(0xfffe); // GetStore(0xfffe);
		// TODO: XEncode(s, s2);
		s = s2;
	}
	else {
		Code(s->A, l);
	}
label2:
	LongS_(F, s);
	ReleaseBoth(p, p2);
}

void CodingRdb::CompressCRdb()
{
	void* p = nullptr;
	MarkStore(p);
	void* cr = Chpt->RecPtr;
	std::string s = "#I1_" + Chpt->Name + "#O1_" + Chpt->Name;
	SetInpStr(s);
	SpecFDNameAllowed = true;
	ReadMerge();
	SpecFDNameAllowed = false;
	RunMerge();
	SaveFiles();
	ReleaseStore(p);
	Chpt->RecPtr = cr;
	CFile = Chpt;
	CRecPtr = E->NewRecPtr;
	ReadRec(CFile, CRec(), CRecPtr);

	ChptTF->CompileAll = false;
	ChptTF->CompileProc = false;
	SetUpdHandle(ChptTF->Handle);
}
