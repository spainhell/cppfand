#include "rdfrml1.h"


#include "common.h"
#include "lexanal.h"
#include "memory.h"
#include "rdfrml.h"

FrmlPtr GetOp(char Op, integer BytesAfter)
{
	FrmlPtr Z; WORD l;
	if (Op < 0x60) l = 1;
	else if (Op < 0xb0) l = 5;
	else if (Op < 0xf0) l = 9;
	else l = 13;
	Z = (FrmlPtr)GetZStore(l + BytesAfter); Z->Op = Op;
	return Z;
}

FrmlPtr RdFAccess(FileDPtr FD, LinkD* LD, char& FTyp)
{
	FrmlPtr Z; FileDPtr cf; bool fa;
	TestIdentif();
	Z = GetOp(_access, 12);
	Z->File2 = FD;
	Z->LD = LD;
	if ((LD != nullptr) && EquUpcase("EXIST")) { RdLex(); FTyp = 'B'; }
	else {
		cf = CFile;
		CFile = FD;
		fa = FileVarsAllowed;
		FileVarsAllowed = true;
		Z->P1 = RdFldNameFrmlF(FTyp);
		CFile = cf;
		FileVarsAllowed = fa;
	}
	return Z;
}

FrmlPtr TryRdFldFrml(FileDPtr FD, char& FTyp)
{
	FileDPtr cf; FieldDPtr f; LinkDPtr ld; FrmlPtr z; pstring roleNm;
	FrmlElem*(*rff)(char&);
	char typ = '\0';

	if (IsKeyWord("OWNED")) {
		rff = RdFldNameFrml;
		RdFldNameFrml = RdFldNameFrmlF;
		Accept('(');
		z = GetOp(_owned, 12);
		TestIdentif();
		SkipBlank(false);
		if (ForwChar == '(') {
			roleNm = LexWord;
			RdLex();
			RdLex();
			ld = FindOwnLD(FD, roleNm);
			Accept(')');
		}
		else ld = FindOwnLD(FD, FD->Name);
		if (ld == nullptr) OldError(182);
		z->ownLD = ld;
		cf = CFile;
		CFile = ld->FromFD;
		if (Lexem == '.') {
			RdLex();
			z->ownSum = RdFldNameFrmlF(FTyp);
			if (FTyp != 'R') OldError(20);
		}
		if (Lexem == ':') {
			RdLex();
			z->ownBool = RdFormula(typ);
			TestBool(typ);
		}
		Accept(')');
		CFile = cf;
		FTyp = 'R';
		RdFldNameFrml = rff;
	}
	else {
		f = FindFldName(FD);
		if (f == nullptr) z = nullptr;
		else { RdLex(); z = MakeFldFrml(f, FTyp); };
	}
	return z;
}

LinkDPtr FindOwnLD(FileDPtr FD, const pstring& RoleName)
{
	LinkDPtr ld;
	LinkDPtr result = nullptr;
	ld = LinkDRoot;
	while (ld != nullptr) {
		if ((ld->ToFD == FD) && EquUpcase(ld->FromFD->Name) &&
			(ld->IndexRoot != 0) && SEquUpcase(ld->RoleName, RoleName)) goto label1;
		ld = ld->Chain;
	}
label1:
	RdLex();
	return ld;
}


FrmlElem* RdFldNameFrmlF(char& FTyp)
{
	LinkDPtr ld;
	FileDPtr fd;
	FrmlPtr z;

	if (IsForwPoint())
	{
		if (!IsRoleName(FileVarsAllowed, fd, ld)) Error(9);
		RdLex();
		return RdFAccess(fd, ld, FTyp);
	}
	if (!FileVarsAllowed) Error(110);
	z = TryRdFldFrml(CFile, FTyp);
	if (z == nullptr) Error(8);
	return z;
}

