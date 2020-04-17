#include "recacc.h"

#include "legacy.h"
#include "memory.h"

string _ShortS(FieldDPtr F)
{
	// TODO:
	void* P; WORD* POfs = (WORD*)P; /*absolute P;*/
	string S; LongStrPtr ss; WORD l;
	if (F->Flg && f_Stored != 0) {
		l = F->L; S[0] = char(l);
		P = CRecPtr;
		POfs += F->Displ;
		switch (F->Typ) {
		case 'A':
		case 'N': {
			if (F->Typ == 'A') {
				Move(P, &S[1], l);
				if (F->Flg && f_Encryp != 0) Code(S[1], l);
				if (IsNullValue(&S[2], l)) FillChar(&S[0], l, ' ');
			}
			else if (IsNullValue(P, F->NBytes)) FillChar(&S[0], l, ' ');
			else UnPack(P, (WORD*)S[0], l);
			break;
		}
		case 'T': {
			ss = &_LongS(F);
			if (ss->length() > 255) S = S.substr(0, 255);
			else S = S.substr(0, ss->length());
			Move(&ss[0], &S[0], S.length());
			ReleaseStore(ss);
			break; };
		default:;
		}
		return S;
	}
	return RunShortStr(F->Frml);
}

string _LongS(FieldDPtr F)
{

	void* P;
	WORD* POfs = (WORD*)P;
	//LP ^longint absolute P;
	LongStrPtr S; longint Pos; integer err; LockMode md; WORD l;
	{
		if (F->Flg && f_Stored != 0) {
			P = CRecPtr; POfs += F->Displ; l = F->L;
			switch (F->Typ)
			{
			case 'A': case 'N': { S = GetStore(l + 2);
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
				else Unpack(P^, S->A, l);
				break;
			}
			case 'T': {
				if (HasTWorkFlag) S = TWork.Read(1, _t(F));
				else {
					md = NewLMode(RdMode);
					S = CFile->TF->Read(1, _t(F));
					OldLMode(md);
				}
				if (F->Flg && f_Encryp != 0) Code(S->A, S->LL);
				if (IsNullValue(@S->A, S->LL))
				{
					S->LL = 0;
					ReleaseAfterLongStr(S);
				}
				break; }
			}
			return *S;
		}
		return RunLongStr(F->Frml);
	};

}
