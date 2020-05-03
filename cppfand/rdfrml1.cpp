#include "rdfrml1.h"


#include "common.h"
#include "lexanal.h"
#include "memory.h"
#include "rdfrml.h"
#include "rdmix.h"
#include "rdrun.h"

FrmlPtr GetOp(BYTE Op, integer BytesAfter)
{
	FrmlPtr Z; WORD l;
	if (Op < 0x60) l = 1;
	else if (Op < 0xb0) l = 5;
	else if (Op < 0xf0) l = 9;
	else l = 13;
	Z = (FrmlPtr)GetZStore(l + BytesAfter); Z->Op = Op;
	return Z;
}

FieldDPtr FindFldName(FileDPtr FD)
{
	FieldDPtr F = FD->FldD;
	while (F != nullptr) {
		{
			if (EquUpcase(F->Name)) goto label1;
			F = F->Chain;
		}
	}
label1:
	return F;
}

FieldDPtr RdFldName(FileDPtr FD)
{
	FieldDPtr F;
	TestIdentif(); F = FindFldName(FD);
	if (F == nullptr) { Set2MsgPar(LexWord, FD->Name); Error(87); }
	RdLex(); return F;
}

FileDPtr FindFileD()
{
	FileDPtr FD; RdbDPtr R; LocVar* LV;
	if (FDLocVarAllowed && FindLocVar(LVBD.Root, LV) && (LV->FTyp == 'f'))
	{
		return LV->FD;
	}
	R = CRdb;
	while (R != nullptr) {
		FD = R->FD;
		while (FD != nullptr) {
			if (EquUpcase(FD->Name)) { return FD; }
			FD = FD->Chain;
		}
		R = R->ChainBack;
	}
	if (EquUpcase("CATALOG")) return CatFD; return nullptr;
}

FileD* RdFileName()
{
	FileDPtr FD;
	if (SpecFDNameAllowed && (Lexem == '@'))
	{
		LexWord = '@'; Lexem = _identifier;
	}
	TestIdentif(); FD = FindFileD();
	if ((FD == nullptr) || (FD == CRdb->FD) && !SpecFDNameAllowed) Error(9);
	RdLex(); return FD;
}

LinkDPtr FindLD(pstring RoleName)
{
	LinkDPtr L;
	L = LinkDRoot;
	while (L != nullptr) {
		if ((L->FromFD == CFile) && SEquUpcase(L->RoleName, RoleName)) {
			return L;
		}
		L = L->Chain;
	}
	return nullptr;
}

bool IsRoleName(bool Both, FileDPtr& FD, LinkDPtr& LD)
{
	TestIdentif();
	FD = FindFileD(); auto result = true;
	if ((FD != nullptr) && FD->IsParFile) { RdLex(); LD = nullptr; return result; }
	if (Both)
	{
		LD = FindLD(LexWord);
		if (LD != nullptr) { RdLex(); FD = LD->ToFD; return result; }
	}
	result = false;
	return result;
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

FrmlPtr FrmlContxt(FrmlPtr Z, FileDPtr FD, void* RP)
{
	FrmlPtr Z1;
	Z1 = GetOp(_newfile, 8); Z1->Frml = Z;
	Z1->NewFile = FD; Z1->NewRP = RP; return Z1;
}

FrmlPtr MakeFldFrml(FieldDPtr F, char& FTyp)
{
	FrmlPtr Z;
	Z = GetOp(_field, 4); Z->Field = F; FTyp = F->FrmlTyp; return Z;
}

FrmlPtr TryRdFldFrml(FileDPtr FD, char& FTyp)
{
	FileDPtr cf; FieldDPtr f; LinkDPtr ld; FrmlPtr z; pstring roleNm;
	FrmlElem* (*rff)(char&);
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

