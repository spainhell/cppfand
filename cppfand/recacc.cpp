#include "recacc.h"

#include "legacy.h"
#include "memory.h"
#include "runfrml.h"

void WriteRec(longint N)
{
	RdWrCache(false, CFile->Handle, CFile->NotCached(), (N - 1) * CFile->RecLen + CFile->FrstDispl, CFile->RecLen, CRecPtr);
	CFile->WasWrRec = true;
}

pstring _ShortS(FieldDPtr F)
{
	// TODO:
	void* P = nullptr; WORD* POfs = (WORD*)P; /*absolute P;*/
	pstring S; LongStrPtr ss; WORD l;
	if (F->Flg && f_Stored != 0) {
		l = F->L; S[0] = char(l);
		P = CRecPtr;
		POfs += F->Displ;
		switch (F->Typ) {
		case 'A':
		case 'N': {
			if (F->Typ == 'A') {
				Move(P, &S[1], l);
				if (F->Flg && f_Encryp != 0) Code(&S[1], l);
				if (IsNullValue(&S[2], l)) FillChar((char*)S[0], l, ' ');
			}
			else if (IsNullValue(P, F->NBytes)) FillChar((char*)S[0], l, ' ');
			else
			{
				// nebudeme volat, zøejmìní není potøeba
				// UnPack(P, (WORD*)S[0], l);
			}
			break;
		}
		case 'T': {
			ss = _LongS(F);
			if (ss->LL > 255) S = S.substr(0, 255);
			else S = S.substr(0, ss->LL);
			Move(&ss[0], &S[0], S.length());
			ReleaseStore(ss);
			break; };
		default:;
		}
		return S;
	}
	return runfrml::RunShortStr(F->Frml);
}

LongStr* _LongS(FieldDPtr F)
{
	void* P = nullptr;
	WORD* POfs = (WORD*)P;
	//LP ^longint absolute P;
	LongStrPtr S = nullptr; longint Pos; integer err; LockMode md; WORD l;
	{
		if (F->Flg && f_Stored != 0) {
			P = CRecPtr; POfs += F->Displ; l = F->L;
			switch (F->Typ)
			{
			case 'A': case 'N': {
				S = (LongStr*)GetStore(l + 2);
				S->LL = l;
				if (F->Typ == 'A') {
					Move(P, &S[0], l);
					if (F->Flg && f_Encryp != 0) Code(S->A, l);
					if (IsNullValue(S, l)) { S->LL = 0; ReleaseAfterLongStr(S); }
				}
				else if (IsNullValue(P, F->NBytes)) {
					S->LL = 0;
					ReleaseAfterLongStr(S);
				}
				else
				{
					// nebudeme volat, zøejmìní není potøeba
					// UnPack(P, S->A, l);
				}
				break;
			}
			case 'T': {
				if (HasTWorkFlag()) S = TWork.Read(1, _T(F));
				else {
					md = NewLMode(RdMode);
					S = CFile->TF->Read(1, _T(F));
					OldLMode(md);
				}
				if (F->Flg && f_Encryp != 0) Code(S->A, S->LL);
				if (IsNullValue(&S->A, S->LL))
				{
					S->LL = 0;
					ReleaseAfterLongStr(S);
				}
				break; }
			}
			return S;
		}
		return runfrml::RunLongStr(F->Frml);
	};

}

double _R(FieldDPtr F)
{
	double result;
	void* p; double r;
	WORD* O = (WORD*)p;
	integer* IP = (integer*)p;

	if (F->Flg && f_Stored != 0) {
		p = CRecPtr; *O += F->Displ;
		if (CFile->Typ == 'D') result = _RforD(F, p);
		else switch (F->Typ) {
		case 'F': {
			r = RealFromFix(p, F->NBytes);
			if (F->Flg && f_Comma == 0) result = r / Power10[F->M]; else result = r; break;
		}
		case 'D': {
			if (CFile->Typ == '8') {
				if (*IP == 0) result = 0.0;
				else result = *IP + FirstDate;
			}
			else goto label1; break;
		}
		case 'R': {
		label1:
			if (IsNullValue(p, F->NBytes)) result = 0;
			else result = *((double*)p);
		}
		}
	}
	else return RunReal(F->Frml);
	return result;
}

bool _B(FieldDPtr F)
{
	bool result;
	void* p;
	WORD* O = (WORD*)p;
	unsigned char* CP = (unsigned char*)p;

	if (F->Flg && f_Stored != 0) {
		p = CRecPtr; O += F->Displ;
		if (CFile->Typ == 'D') result = *CP == 'Y' || *CP == 'y' || *CP == 'T' || *CP == 't';
		else if ((*CP == '\0') || (*CP == 0xff)) result = false;
		else result = true;
	}
	else result = RunBool(F->Frml);
	return result;
}

/// nechápu, co to dìlá - oøee úvodní znaky, pøevádí na èíslo, ...
longint _T(FieldDescr* F)
{
	void* p; longint n; integer err;
	WORD* O = (WORD*)&p;

	p = CRecPtr;
	O += F->Displ;
	if (CFile->Typ == 'D')
	{
		n = 0;
		// tváøíme se, e CRecPtr je pstring ...
		pstring* s = (pstring*)CRecPtr;
		auto result = stoi(runfrml::LeadChar(' ', *s));
		return result;
	}
	else
	{
		if (IsNullValue(p, 4)) return 0;
		longint* lip = (longint*)p;
		return *lip;
	}
}

void DelAllDifTFlds(void* Rec, void* CompRec)
{
	FieldDPtr F = CFile->FldD;
	while (F != nullptr)
	{
		if (F->Typ == 'T' && F->Flg && f_Stored != 0) DelDifTFld(Rec, CompRec, F);
		F = F->Chain;
	}
}
