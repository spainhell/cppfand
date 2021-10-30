#include "rdmerg.h"

#include "shared.h"
#include "../cppfand/compile.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/legacy.h"
#include "../cppfand/rdfildcl.h"
#include "../MergeReport/rdrprt.h"

char WhatToRd = '\0'; /*i=Oi output FDs;O=O outp.FDs*/
bool ReadingOutpBool = false;
WORD Ii = 0, Oi = 0, SumIi = 0;
OutpRD* RD = nullptr;

FileD* InpFD_M(WORD I)
{
	// tady si to sahalo na neexistujici polozky
	// proto je to cele prepsane
	// velikost IDA je 9, z toho [0] se nepoužívá, max. index je tedy 8
	// if (I > 8) return nullptr;
	if (IDA[I] != nullptr) return IDA[I]->Scan->FD;
	if (IDA[I + 1] != nullptr) {
		Ii++;
		return IDA[I + 1]->Scan->FD;
	}
	return nullptr;
}

FrmlElem* FindIiandFldFrml_M(FileD** FD, char& FTyp)
{
	integer i = 0; FrmlPtr z = nullptr;
	if (!Join && (WhatToRd == 'i')) {   /* for Oi search first in Ii*/
		*FD = InpFD_M(Oi); z = TryRdFldFrml(*FD, FTyp);
		if (z != nullptr) { Ii = Oi; goto label1; };
	}
	for (i = 1; i <= MaxIi; i++) {     /* search in  I1 .. In, for Oi only I1 .. Ii*/
		*FD = InpFD_M(i); z = TryRdFldFrml(*FD, FTyp);
		if (z != nullptr) { Ii = i; goto label1; }
		if ((WhatToRd == 'i') && (i == Oi)) goto label1;
	}
label1:
	return z;
}

FrmlPtr RdFldNameFrmlM(char& FTyp)
{
	FieldDescr* F = nullptr;                         /*RdFldNameFrml - body*/
	FrmlPtr Z = nullptr;
	LocVar* LV = nullptr; FileD* FD = nullptr;
	FrmlElem* result = nullptr;

	bool WasIiPrefix = RdIiPrefix();
	if ((FrmlSumEl != nullptr) && FrstSumVar) SumIi = 0;
	TestIdentif();
	if ((LexWord == "O") && IsForwPoint() && !WasIiPrefix) {
		RdLex(); RdLex();
		if ((FrmlSumEl != nullptr) || ReadingOutpBool) Error(99);
		RdOutpFldName(FTyp, &result);
		return result;
	}
	if (IsForwPoint()) {
		RdDirFilVar_M(FTyp, &result, WasIiPrefix);
		TestSetSumIi();
		return result;
	}
	if (!WasIiPrefix) if (FindLocVar(&LVBD, &LV)) {
		RdLex();
		TestNotSum();
		result = new FrmlElem18(_getlocvar, LV);
		FTyp = LV->FTyp;
		return result;
	}
	if (IsKeyWord("COUNT")) {
	label1:
		TestNotSum();
		SetIi_Merge(WasIiPrefix);
		//result = (FrmlElem*)(&IDA[Ii]->Op);
		result = new FrmlElemInp(_count, IDA[Ii]);
		FTyp = 'R';
		return result;
	}
	if (IsKeyWord("GROUP")) {
	label2:
		TestNotSum();
		if (WasIiPrefix) OldError(41);
		//result = (FrmlPtr)(&MergOpGroup);
		result = new FrmlElemMerge(_mergegroup, &MergOpGroup);
		FTyp = 'R';
		return result;
	}
	if (IsKeyWord("ERROR")) {
		Err('m', WasIiPrefix);
		result = (FrmlElem*)(&IDA[Ii]->OpErr);
		FTyp = 'B';
		return result;
	}
	if (IsKeyWord("WARNING")) {
		Err('m', WasIiPrefix);
		result = (FrmlElem*)(&IDA[Ii]->OpWarn);
		FTyp = 'B';
		return result;
	}
	if (IsKeyWord("ERRORTEXT")) {
		Err('m', WasIiPrefix);
		result = IDA[Ii]->ErrTxtFrml;
		FTyp = 'S';
		return result;
	}
	if (WasIiPrefix) {
		FD = InpFD_M(Ii);
		Z = TryRdFldFrml(FD, FTyp);
	}
	else Z = FindIiandFldFrml_M(&FD, FTyp);
	if (Z == nullptr) {
		if (IsKeyWord('N')) goto label1;
		if (IsKeyWord('M')) goto label2;
		Error(8);
	}
	TestSetSumIi();
	result = FrmlContxt(Z, FD, FD->RecPtr);
	return result;
}

void RdDirFilVar_M(char& FTyp, FrmlElem** res, bool wasIiPrefix)
{
	LinkD* LD = nullptr; FileD* FD = nullptr;
	integer I = 0;
	FrmlPtr Z = nullptr;
	if (wasIiPrefix)
	{
		CFile = InpFD_M(Ii);
		if (!IsRoleName(true, &FD, &LD)) Error(9);
	}
	else {
		if (!Join && (WhatToRd == 'i'))
		{
			Ii = Oi;
			CFile = InpFD_M(Ii);
			if (IsRoleName(true, &FD, &LD)) goto label2;
		}
		for (I = 1; I <= MaxIi; I++)
		{
			CFile = InpFD_M(I);
			if (IsRoleName(true, &FD, &LD)) { Ii = I; goto label2; }
			if ((WhatToRd == 'i') && (I == Oi)) goto label1;
		}
	label1:
		Error(9);
	}
label2:
	Accept('.');
	Z = RdFAccess(FD, LD, FTyp);
	if (LD == nullptr) Ii = 0;
	else Z = FrmlContxt(Z, CFile, CFile->RecPtr);
	*res = Z;
}

void RdOutpFldName(char& FTyp, FrmlElem** res)
{
	if (RD->OD == nullptr /*dummy*/) {
		Error(85);
	}
	else {
		auto rdFldName = RdFldName(RD->OD->FD);
		auto makeFldFrml = MakeFldFrml(rdFldName, FTyp);
		*res = FrmlContxt(makeFldFrml, RD->OD->FD, RD->OD->RecPtr);
	}
}

void ChainSumElM()
{
	if (FrstSumVar || (SumIi == 0)) SumIi = 1;
	if (IDA[SumIi]->Sum == nullptr) {
		IDA[SumIi]->Sum = FrmlSumEl;
	}
	else {
		for (size_t i = 0; i < FrmlSumEl->size(); i++) {
			auto* el = FrmlSumEl->at(i);
			IDA[SumIi]->Sum->push_back(el);
			FrmlSumEl->clear();
		}
	}
	//FrmlSumEl->Chain = IDA[SumIi]->Sum;
	//IDA[SumIi]->Sum = FrmlSumEl;
}

void ReadMerge()
{
	InpD* ID = nullptr;
	FieldDescr* F = nullptr;
	FileD* FD = nullptr;
	KeyInD* KI = nullptr;
	WORD I = 0;
	bool WasOi = false, WasSqlFile = false, CompLex = false;
	ResetCompilePars();
	RdLex();
	ResetLVBD();
	if (IsKeyWord("VAR")) {
		RdLocDcl(&LVBD, false, false, 'M');
	}
	WhatToRd = 'I';
	ReadingOutpBool = false; WasSqlFile = false;
	Ii = 0;
	TestLex('#');
	do {
		ReadChar();
		if (CurrChar == 'I') {
			ReadChar();
			if (isdigit(CurrChar)) {
				I = CurrChar - '0';
				ReadChar();
				if (CurrChar == '_') {
					RdLex();
					goto label1;
				}
			}
		}
		Error(89);
	label1:
		Ii++;
		if (I != Ii) OldError(61);
		//ID = (InpD*)GetZStore(sizeof(*ID)); 
		ID = new InpD();
		IDA[Ii] = ID;
		FD = RdFileName();
		CFile = FD;
#ifdef FandSQL
		if (CFile->typSQLFile) WasSqlFile = true;
#endif 
		for (I = 1; I <= Ii - 1; I++) {
			if (InpFD_M(I) == FD) OldError(26);
		}
		CViewKey = RdViewKey();
		if (Lexem == '!') {
			RdLex();
			ID->AutoSort = true;
		}
		ID->Op = _const;
		ID->OpErr = _const;
		ID->OpWarn = _const;
		KI = nullptr;
		ID->ForwRecPtr = GetRecSpace();
		FD->RecPtr = GetRecSpace();
		if (Lexem == '(') {
			RdLex();
			ID->Bool = RdKeyInBool(&KI, false, false, ID->SQLFilter);
			Accept(')');
		}
		//New(ID->Scan, Init(FD, CViewKey, KI, true));
		ID->Scan = new XScan(FD, CViewKey, KI, true);
		if (!(Lexem == ';' || Lexem == '#' || Lexem == 0x1A)) {
			RdKFList(&ID->MFld, FD);
		}
		if (Ii > 1) {
			if (IDA[Ii - 1]->MFld == nullptr) {
				if (ID->MFld != nullptr) OldError(22);
			}
			else if (ID->MFld == nullptr) CopyPrevMFlds();
			else CheckMFlds(IDA[Ii - 1]->MFld, ID->MFld);
		}
		RdAutoSortSK_M(ID);
		TestLex('#');
	} while (ForwChar == 'I');

	MaxIi = Ii;
	MakeOldMFlds();
	OldMXStr.Clear();
	OutpFDRoot = nullptr; OutpRDs = nullptr; Join = false; WasOi = false;
	RdFldNameFrml = RdFldNameFrmlM;

label3:
	ReadChar();
	if (CurrChar == 'O') {
		ReadChar();
		if (isdigit(CurrChar)) {
			if (Join) Error(91);
			WasOi = true;
			Oi = CurrChar - '0';
			if ((Oi == 0) || (Oi > MaxIi)) Error(62);
			goto label4;
		}
		else if (CurrChar == '*') {
			if (WasOi) Error(91);
			if (WasSqlFile) Error(155);
			Join = true;
			Oi = MaxIi;
		label4:
			ReadChar();
			if (CurrChar != '_') Error(90);
			RdLex();
			WhatToRd = 'i';
			ChainSumEl = nullptr;
			RdOutpRD(&(IDA[Oi]->RD));
		}
		else if (CurrChar == '_') {
			RdLex();
			WhatToRd = 'O';
			ChainSumEl = ChainSumElM;
			RdOutpRD(&OutpRDs);
		}
		else Error(90);
	}
	else Error(90);
	if (Lexem != 0x1A) {
		TestLex('#');
		goto label3;
	}
	for (I = 1; I <= MaxIi; I++) {
		ID = IDA[I];
		if (ID->ErrTxtFrml != nullptr) {
			RdChkDsFromPos(ID->Scan->FD, ID->Chk);
		}
	}
}

void MakeOldMFlds()
{
	KeyFldD* M = IDA[1]->MFld;
	WORD n = 0;
	OldMFlds.clear();
	while (M != nullptr) {
		OldMFlds.push_back(ConstListEl());
		M = (KeyFldD*)M->Chain;
	}
}

void RdAutoSortSK_M(InpD* ID)
{
	KeyFldD* M = nullptr; KeyFldD* SK = nullptr;
	if (!ID->AutoSort) return;
	M = ID->MFld;
	while (M != nullptr) {
		SK = new KeyFldD(); // (KeyFldD*)GetStore(sizeof(*SK));
		//Move(M, SK, sizeof(SK));
		*SK = *M;
		if (ID->SK == nullptr) { ID->SK = SK; SK->Chain = nullptr; }
		else ChainLast(ID->SK, SK);
		M = (KeyFldD*)M->Chain;
	}
	if (Lexem == ';') {
		RdLex();
		RdKFList(&ID->SK, CFile);
	}
	if (ID->SK == nullptr) OldError(60);
}

void ImplAssign(OutpRD* outputRD, FieldDescr* outputField)
{
	char FTyp = 0;
	FileD* outputFile = outputRD->OD->FD;
	void* outputRecPointer = outputRD->OD->RecPtr;

	AssignD* newAssign = new AssignD();
	FieldDescr* inputField = FindIiandFldD(outputField->Name);
	newAssign->inputFldD = inputField;
	
	if ((inputField == nullptr) || (inputField->FrmlTyp != outputField->FrmlTyp) ||
		(inputField->FrmlTyp == 'R') && (inputField->Typ != outputField->Typ))
	{
		newAssign->Kind = MInstrCode::_zero;
		newAssign->outputFldD = outputField;
	}
	else {
		FileD* inputFile = InpFD_M(Ii);
		void* inputRecPointer = inputFile->RecPtr;
		if ((inputFile->Typ == outputFile->Typ) && FldTypIdentity(inputField, outputField) && (inputField->Typ != 'T')
			&& ((inputField->Flg & f_Stored) != 0) && (outputField->Flg == inputField->Flg))
		{
			newAssign->Kind = _move;
			newAssign->L = outputField->NBytes;
			newAssign->ToPtr = (BYTE*)outputRecPointer + outputField->Displ;
			newAssign->FromPtr = (BYTE*)inputRecPointer + inputField->Displ;
			//if (RD_Ass != nullptr
			//	&& RD_Ass->Kind == _move
			//	&& (uintptr_t)(RD_Ass->FromPtr + RD_Ass->L) == (uintptr_t)newAssign->FromPtr
			//	&& (uintptr_t)(RD_Ass->ToPtr + RD_Ass->L) == (uintptr_t)newAssign->ToPtr)
			//{
			//	RD_Ass->L += newAssign->L;
			//	ReleaseStore(newAssign);
			//	goto label1;
			//}
		}
		else {
			newAssign->Kind = _output;
			newAssign->OFldD = outputField;
			FrmlElem* Z = MakeFldFrml(inputField, FTyp);
			Z = AdjustComma_M(Z, inputField, _divide);
			Z = AdjustComma_M(Z, outputField, _times);
			newAssign->Frml = FrmlContxt(Z, inputFile, inputFile->RecPtr);
		}
	}
	//newAssign->Chain = RD_Ass;
	outputRD->Ass.push_back(newAssign);
//label1:
}

FrmlElem* AdjustComma_M(FrmlElem* Z1, FieldDescr* F, instr_type Op)
{
	FrmlElem0* Z = nullptr;
	FrmlElem2* Z2 = nullptr;
	FrmlElem* result = Z1;
	if (F->Typ != 'F') return result;
	if ((F->Flg & f_Comma) == 0) return result;
	Z2 = new FrmlElem2(_const, 0, Power10[F->M]); // GetOp(_const, sizeof(double));
	//Z2->R = Power10[F->M];
	Z = new FrmlElem0(Op, 0); // GetOp(Op, 0);
	Z->P1 = Z1;
	Z->P2 = Z2;
	result = Z;
	return result;
}

FieldDescr* FindIiandFldD(std::string fieldName)
{
	FieldDescr* result = nullptr;
	if (!Join && (WhatToRd == 'i')) {   /* for Oi search first in Ii*/
		FileD* pFile = InpFD_M(Oi);
		result = FindFldName(pFile, fieldName);
		if (result != nullptr) {
			Ii = Oi;
			return result;
		}
	}
	for (integer i = 1; i <= MaxIi; i++) {    /* search in  I1 .. In, for Oi only I1 .. Ii*/
		FileD* pFile = InpFD_M(i);
		result = FindFldName(pFile, fieldName);
		if (result != nullptr) {
			Ii = i;
			return result;
		}
		if ((WhatToRd == 'i') && (i == Oi)) return result;
	}
	return result;
}

bool FindAssignToF(std::vector<AssignD*> A, FieldDescr* F)
{
	for (auto* assign : A) {
		if ((assign->Kind == _output) && (assign->OFldD == F) && !assign->Add) {
			return true;
		}
		//A = (AssignD*)A->Chain;
	}
	return false;
}

void MakeImplAssign()
{
	AssignD* AD = nullptr;
	if (RD->OD == nullptr) return;
	for (auto& FNew : RD->OD->FD->FldD) { /*implic.assign   name = name*/
		if (((FNew->Flg & f_Stored) != 0) && !FindAssignToF(RD->Ass, FNew)) {
			ImplAssign(RD, FNew);
		}
	}
}

void TestIsOutpFile(FileDPtr FD)
{
	OutpFD* OFD = OutpFDRoot;
	while (OFD != nullptr) {
		if (OFD->FD == FD) OldError(173); OFD = (OutpFD*)OFD->Chain;
	}
}

std::vector<AssignD*> RdAssign_M()
{
	FieldDescr* F = nullptr; FileD* FD = nullptr;
	LocVar* LV = nullptr; AssignD* AD = nullptr;

	std::vector<AssignD*> result;
	if (IsKeyWord("BEGIN")) {
		result = RdAssSequ();
		AcceptKeyWord("END");
		return result;
	}
	 
	AD = new AssignD();
	TestIdentif();
	if (IsKeyWord("IF")) {
		AD->Kind = _ifthenelseM;
		AD->Bool = RdBool();
		AcceptKeyWord("THEN");
		AD->Instr = RdAssign_M();
		if (IsKeyWord("ELSE")) AD->ElseInstr = RdAssign_M();
	}
	else if (ForwChar == '.') {
		AD->Kind = _parfile;
		FD = RdFileName();
		if (!FD->IsParFile) OldError(9);
		TestIsOutpFile(FD);
		Accept('.');
		AD->FD = FD;
		F = RdFldName(FD);
		AD->PFldD = F;
		if ((F->Flg & f_Stored) == 0) OldError(14);
		RdAssignFrml(F->FrmlTyp, AD->Add, &AD->Frml);
	}
	else if (FindLocVar(&LVBD, &LV)) {
		RdLex();
		AD->Kind = _locvar;
		AD->LV = LV;
		RdAssignFrml(LV->FTyp, AD->Add, &AD->Frml);
	}
	else {
		if (RD->OD == nullptr) Error(72);  /*dummy*/
		else {
			AD->Kind = _output;
			F = RdFldName(RD->OD->FD);
			AD->OFldD = F;
			if ((F->Flg & f_Stored) == 0) OldError(14);
			RdAssignFrml(F->FrmlTyp, AD->Add, &AD->Frml);
		}
	}
	result.push_back(AD);
	return result;
}

std::vector<AssignD*> RdAssSequ()
{
	std::vector<AssignD*> A;
label1:
	//if (ARoot == nullptr) {
	//	ARoot = RdAssign_M();
	//}
	//else {
	//	A = ARoot;
	//	while (A->Chain != nullptr) A = (AssignD*)A->Chain;
	//	A->Chain = RdAssign_M();
	//}
	auto rd_assign = RdAssign_M();
	for (AssignD* a : rd_assign) {
		A.push_back(a);
	}
	
	if (Lexem == ';')
	{
		RdLex();
		if (!(Lexem == 0x1A || Lexem == '#') && !TestKeyWord("END")) goto label1;
	}
	return A;
}

void RdOutpRD(OutpRD** RDRoot)
{
	OutpRD* R = nullptr; FileD* FD = nullptr;
	OutpFD* OD = nullptr; InpD* ID = nullptr;
	integer I = 0;
	//RD = (OutpRD*)GetStore(sizeof(*RD));
	RD = new OutpRD();
	if (*RDRoot == nullptr) *RDRoot = RD;
	else ChainLast(*RDRoot, RD);
	//RD->Ass = nullptr;
	//RD->Bool = nullptr;
	if (IsKeyWord("DUMMY")) RD->OD = nullptr;
	else {
		FD = RdFileName();
		OD = OutpFDRoot;
		while (OD != nullptr)
		{
			if (OD->FD == FD)
			{
				if (Lexem == '+') {
					if (OD->Append) RdLex();
					else Error(31);
				}
				else if (OD->Append) Error(31);
				goto label1;
			}
			OD = (OutpFD*)OD->Chain;
		}
		//OD = (OutpFD*)GetStore(sizeof(*OD));
		OD = new OutpFD();
		OD->FD = FD;
		CFile = FD;
		OD->RecPtr = GetRecSpace();
		OD->InplFD = nullptr;
		for (I = 1; I <= MaxIi; I++) {
			if (InpFD_M(I) == FD) {
				OD->InplFD = FD;
				IDA[I]->IsInplace = true;
				if (FD->typSQLFile) Error(172);
			}
		}
		if (Lexem == '+')
		{
			OD->Append = true;
			if (OD->InplFD != nullptr) Error(32);
			RdLex();
		}
		else OD->Append = false;
		if (OutpFDRoot == nullptr) OutpFDRoot = OD;
		else ChainLast(OutpFDRoot, OD);
	label1:
		RD->OD = OD;
	}
	if (Lexem == '(')
	{
		RdLex();
		ReadingOutpBool = true;
		RD->Bool = RdBool();
		ReadingOutpBool = false;
		Accept(')');
	}
	if (!(Lexem == '#' || Lexem == 0x1A)) {
		RD->Ass = RdAssSequ();
	}
	MakeImplAssign();
}


