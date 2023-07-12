#include "Merge.h"
#include "../Common/compare.h"
#include "../Core/compile.h"
#include "../Core/FieldDescr.h"
#include "../Core/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/ChkD.h"
#include "../Core/KeyFldD.h"
#include "../Core/RunMessage.h"
#include "../Core/rdfildcl.h"
#include "../fandio/files.h"

Merge::Merge()
{
}

Merge::~Merge()
{
}

void Merge::Read()
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
		if (I != Ii) {
			OldError(61);
		}
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
		ID->ForwRecPtr = CFile->GetRecSpace();
		FD->FF->RecPtr = CFile->GetRecSpace();
		if (Lexem == '(') {
			RdLex();
			ID->Bool = RdKeyInBool(&KI, false, false, ID->SQLFilter, this);
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
	ptrRdFldNameFrml = nullptr; // RdFldNameFrml;

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
			ChainSum = false;
			RdOutpRD(&(IDA[Oi]->RD));
		}
		else if (CurrChar == '_') {
			RdLex();
			WhatToRd = 'O';
			ChainSum = true;
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

void Merge::Run()
{
	short MinIi = 0, res = 0, NEof = 0;      /*RunMerge - body*/
	bool EmptyGroup = false, b = false;
	//PushProcStk();
	OpenInpM();
	OpenOutp();
	MergOpGroup.Group = 1.0;
	RunMsgOn('M', NRecsAll);
	NRecsAll = 0;
	for (short i = 1; i <= MaxIi; i++) {
		ReadInpFileM(IDA[i]);
	}
label1:
	MinIi = 0; NEof = 0;
	for (short i = 1; i <= MaxIi; i++) /* !!! with IDA[I]^ do!!! */ {
		CFile = IDA[i]->Scan->FD;
		IDA[i]->IRec = IDA[i]->Scan->IRec;
		ZeroSumFlds(IDA[i]->Sum);
		if (IDA[i]->Scan->eof) NEof++;
		if (OldMFlds.empty()) {
			IDA[i]->Exist = !IDA[i]->Scan->eof;
			MinIi = 1;
		}
		else {
			CRecPtr = IDA[i]->ForwRecPtr;
			IDA[i]->Exist = false;
			IDA[i]->Count = 0.0;
			if (!IDA[i]->Scan->eof) {
				if (MinIi == 0) goto label2;
				res = CompMFlds(IDA[i]->MFld);
				if (res != _gt) {
					if (res == _lt)
					{
					label2:
						SetOldMFlds(IDA[i]->MFld);
						MinIi = i;
					}
					IDA[i]->Exist = true;
				}
			}
		}
	}
	for (short i = 1; i <= MinIi - 1; i++) {
		IDA[i]->Exist = false;
	}
	if (NEof == MaxIi) {
		b = SaveCache(0, CFile->FF->Handle);
		RunMsgOff();
		if (!b) GoExit();
		CloseInpOutp();
		//PopProcStk();
		return;
	}
	EmptyGroup = false;
	if (Join) {
		JoinProc(1, EmptyGroup);
	}
	else {
		MergeProcM();
	}
	if (!EmptyGroup) {
		WriteOutp(OutpRDs);
		MergOpGroup.Group = MergOpGroup.Group + 1.0;
	}
	goto label1;
}

FrmlElem* Merge::RdFldNameFrml(char& FTyp)
{
	FieldDescr* F = nullptr;                         /*RdFldNameFrml - body*/
	FrmlElem* Z = nullptr;
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
		Z = TryRdFldFrml(FD, FTyp, this);
	}
	else Z = FindIiandFldFrml_M(&FD, FTyp);
	if (Z == nullptr) {
		if (IsKeyWord("N")) goto label1;
		if (IsKeyWord("M")) goto label2;
		Error(8);
	}
	TestSetSumIi();
	result = FrmlContxt(Z, FD, FD->FF->RecPtr);
	return result;
}

void Merge::ChainSumEl()
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
	//FrmlSumEl->pChain = IDA[SumIi]->Sum;
	//IDA[SumIi]->Sum = FrmlSumEl;
}

FileD* Merge::InpFD_M(WORD I)
{
	// tady si to sahalo na neexistujici polozky
	// proto je to cele prepsane
	// velikost IDA je 9, z toho [0] se nepouziva, max. index je tedy 8
	// if (I > 8) return nullptr;
	if (IDA[I] != nullptr) return IDA[I]->Scan->FD;
	if (IDA[I + 1] != nullptr) {
		Ii++;
		return IDA[I + 1]->Scan->FD;
	}
	return nullptr;
}

FrmlElem* Merge::FindIiandFldFrml_M(FileD** FD, char& FTyp)
{
	short i = 0;
	FrmlElem* z = nullptr;
	if (!Join && (WhatToRd == 'i')) {   /* for Oi search first in Ii*/
		*FD = InpFD_M(Oi);
		z = TryRdFldFrml(*FD, FTyp, this);
		if (z != nullptr) { Ii = Oi; goto label1; }
	}
	for (i = 1; i <= MaxIi; i++) {     /* search in  I1 .. In, for Oi only I1 .. Ii*/
		*FD = InpFD_M(i);
		z = TryRdFldFrml(*FD, FTyp, this);
		if (z != nullptr) { Ii = i; goto label1; }
		if ((WhatToRd == 'i') && (i == Oi)) goto label1;
	}
label1:
	return z;
}

void Merge::RdDirFilVar_M(char& FTyp, FrmlElem** res, bool wasIiPrefix)
{
	LinkD* LD = nullptr; FileD* FD = nullptr;
	short I = 0;
	FrmlElem* Z = nullptr;
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
	else Z = FrmlContxt(Z, CFile, CFile->FF->RecPtr);
	*res = Z;
}

void Merge::RdOutpFldName(char& FTyp, FrmlElem** res)
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

void Merge::MakeOldMFlds()
{
	KeyFldD* M = IDA[1]->MFld;
	WORD n = 0;
	OldMFlds.clear();
	while (M != nullptr) {
		OldMFlds.push_back(ConstListEl());
		M = (KeyFldD*)M->pChain;
	}
}

void Merge::RdAutoSortSK_M(InpD* ID)
{
	KeyFldD* M = nullptr; KeyFldD* SK = nullptr;
	if (!ID->AutoSort) return;
	M = ID->MFld;
	while (M != nullptr) {
		SK = new KeyFldD(); // (KeyFldD*)GetStore(sizeof(*SK));
		//Move(M, SK, sizeof(SK));
		*SK = *M;
		if (ID->SK == nullptr) { ID->SK = SK; SK->pChain = nullptr; }
		else ChainLast(ID->SK, SK);
		M = (KeyFldD*)M->pChain;
	}
	if (Lexem == ';') {
		RdLex();
		RdKFList(&ID->SK, CFile);
	}
	if (ID->SK == nullptr) OldError(60);
}

void Merge::ImplAssign(OutpRD* outputRD, FieldDescr* outputField)
{
	char FTyp = 0;
	FileD* outputFile = outputRD->OD->FD;
	void* outputRecPointer = outputRD->OD->RecPtr;

	AssignD* newAssign = new AssignD();
	FieldDescr* inputField = FindIiandFldD(outputField->Name);
	newAssign->inputFldD = inputField;

	if ((inputField == nullptr) || (inputField->frml_type != outputField->frml_type) ||
		(inputField->frml_type == 'R') && (inputField->field_type != outputField->field_type))
	{
		newAssign->Kind = MInstrCode::_zero;
		newAssign->outputFldD = outputField;
	}
	else {
		FileD* inputFile = InpFD_M(Ii);
		void* inputRecPointer = inputFile->FF->RecPtr;
		if ((inputFile->FF->file_type == outputFile->FF->file_type) && FldTypIdentity(inputField, outputField) && (inputField->field_type != FieldType::TEXT)
			&& ((inputField->Flg & f_Stored) != 0) && (outputField->Flg == inputField->Flg))
		{
			newAssign->Kind = MInstrCode::_move;
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
			newAssign->Kind = MInstrCode::_output;
			newAssign->OFldD = outputField;
			FrmlElem* Z = MakeFldFrml(inputField, FTyp);
			Z = AdjustComma_M(Z, inputField, _divide);
			Z = AdjustComma_M(Z, outputField, _times);
			newAssign->Frml = FrmlContxt(Z, inputFile, inputFile->FF->RecPtr);
		}
	}
	//newAssign->pChain = RD_Ass;
	outputRD->Ass.push_back(newAssign);
	//label1:
}

FrmlElem* Merge::AdjustComma_M(FrmlElem* Z1, FieldDescr* F, instr_type Op)
{
	FrmlElem0* Z = nullptr;
	FrmlElem2* Z2 = nullptr;
	FrmlElem* result = Z1;
	if (F->field_type != FieldType::FIXED) return result;
	if ((F->Flg & f_Comma) == 0) return result;
	Z2 = new FrmlElem2(_const, 0, Power10[F->M]); // GetOp(_const, sizeof(double));
	//Z2->R = Power10[F->M];
	Z = new FrmlElem0(Op, 0); // GetOp(Op, 0);
	Z->P1 = Z1;
	Z->P2 = Z2;
	result = Z;
	return result;
}

FieldDescr* Merge::FindIiandFldD(std::string fieldName)
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
	for (short i = 1; i <= MaxIi; i++) {    /* search in  I1 .. In, for Oi only I1 .. Ii*/
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

bool Merge::FindAssignToF(std::vector<AssignD*> A, FieldDescr* F)
{
	for (auto* assign : A) {
		if ((assign->Kind == MInstrCode::_output) && (assign->OFldD == F) && !assign->Add) {
			return true;
		}
		//A = (AssignD*)A->pChain;
	}
	return false;
}

void Merge::MakeImplAssign()
{
	AssignD* AD = nullptr;
	if (RD->OD == nullptr) return;
	for (auto& FNew : RD->OD->FD->FldD) { /*implic.assign   name = name*/
		if (((FNew->Flg & f_Stored) != 0) && !FindAssignToF(RD->Ass, FNew)) {
			ImplAssign(RD, FNew);
		}
	}
}

void Merge::TestIsOutpFile(FileD* FD)
{
	OutpFD* OFD = OutpFDRoot;
	while (OFD != nullptr) {
		if (OFD->FD == FD) {
			OldError(173);
		}
		OFD = OFD->pChain;
	}
}

std::vector<AssignD*> Merge::RdAssign_M()
{
	FieldDescr* F = nullptr;
	FileD* FD = nullptr;
	LocVar* LV = nullptr;
	AssignD* AD = nullptr;

	std::vector<AssignD*> result;
	if (IsKeyWord("BEGIN")) {
		result = RdAssSequ();
		AcceptKeyWord("END");
		return result;
	}

	AD = new AssignD();
	TestIdentif();
	if (IsKeyWord("IF")) {
		AD->Kind = MInstrCode::_ifThenElse;
		AD->Bool = RdBool(this);
		AcceptKeyWord("THEN");
		AD->Instr = RdAssign_M();
		if (IsKeyWord("ELSE")) AD->ElseInstr = RdAssign_M();
	}
	else if (ForwChar == '.') {
		AD->Kind = MInstrCode::_parfile;
		FD = RdFileName();
		if (!FD->IsParFile) OldError(9);
		TestIsOutpFile(FD);
		Accept('.');
		AD->FD = FD;
		F = RdFldName(FD);
		AD->PFldD = F;
		if ((F->Flg & f_Stored) == 0) OldError(14);
		RdAssignFrml(F->frml_type, AD->Add, &AD->Frml, this);
	}
	else if (FindLocVar(&LVBD, &LV)) {
		RdLex();
		AD->Kind = MInstrCode::_locvar;
		AD->LV = LV;
		RdAssignFrml(LV->FTyp, AD->Add, &AD->Frml, this);
	}
	else {
		if (RD->OD == nullptr) Error(72);  /*dummy*/
		else {
			AD->Kind = MInstrCode::_output;
			F = RdFldName(RD->OD->FD);
			AD->OFldD = F;
			if ((F->Flg & f_Stored) == 0) OldError(14);
			RdAssignFrml(F->frml_type, AD->Add, &AD->Frml, this);
		}
	}
	result.push_back(AD);
	return result;
}

std::vector<AssignD*> Merge::RdAssSequ()
{
	std::vector<AssignD*> A;
label1:
	//if (ARoot == nullptr) {
	//	ARoot = RdAssign_M();
	//}
	//else {
	//	A = ARoot;
	//	while (A->pChain != nullptr) A = (AssignD*)A->pChain;
	//	A->pChain = RdAssign_M();
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

void Merge::RdOutpRD(OutpRD** RDRoot)
{
	OutpRD* R = nullptr; FileD* FD = nullptr;
	OutpFD* OD = nullptr; InpD* ID = nullptr;
	short I = 0;

	RD = new OutpRD();
	if (*RDRoot == nullptr) *RDRoot = RD;
	else ChainLast(*RDRoot, RD);

	//RD->Ass = nullptr;
	//RD->Bool = nullptr;
	if (IsKeyWord("DUMMY")) RD->OD = nullptr;
	else {
		FD = RdFileName();
		OD = OutpFDRoot;
		while (OD != nullptr) {
			if (OD->FD == FD) {
				if (Lexem == '+') {
					if (OD->Append) {
						RdLex();
					}
					else {
						Error(31);
					}
				}
				else if (OD->Append) {
					Error(31);
				}
				goto label1;
			}
			OD = OD->pChain;
		}

		OD = new OutpFD();
		OD->FD = FD;
		CFile = FD;
		OD->RecPtr = FD->GetRecSpace();
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
		RD->Bool = RdBool(this);
		ReadingOutpBool = false;
		Accept(')');
	}
	if (!(Lexem == '#' || Lexem == 0x1A)) {
		RD->Ass = RdAssSequ();
	}
	MakeImplAssign();
}

WORD Merge::CompMFlds(KeyFldD* M)
{
	XString x;
	x.PackKF(CFile, M, CRecPtr);
	return CompStr(x.S, OldMXStr.S);
}

void Merge::SetOldMFlds(KeyFldD* M)
{
	//ConstListEl* C = nullptr;
	FieldDescr* F = nullptr;
	OldMXStr.Clear();
	//C = OldMFlds;
	for (auto& C : OldMFlds) { //while (C != nullptr) {
		F = M->FldD;
		switch (F->frml_type) {
		case 'S': {
			C.S = CFile->loadOldS(F, CRecPtr);
			OldMXStr.StoreStr(C.S, M);
			break;
		}
		case 'R': {
			C.R = CFile->loadR(F, CRecPtr);
			OldMXStr.StoreReal(C.R, M);
			break;
		}
		default: {
			C.B = CFile->loadB(F, CRecPtr);
			OldMXStr.StoreBool(C.B, M);
			break;
		}
		}
		//C = (ConstListEl*)C->pChain;
		M = M->pChain;
	}
}

void Merge::ReadInpFileM(InpD* ID)
{
	CRecPtr = ID->ForwRecPtr;
label1:
	ID->Scan->GetRec(CRecPtr);
	if (ID->Scan->eof) return;
	NRecsAll++;
	RunMsgN(NRecsAll);
	if (!RunBool(CFile, ID->Bool, CRecPtr)) goto label1;
}

void Merge::RunAssign(std::vector<AssignD*> Assigns)
{
	for (auto* A : Assigns) {
		/* !!! with A^ do!!! */
		switch (A->Kind) {
		case MInstrCode::_move: {
			if (A != nullptr && A->FromPtr != nullptr && A->ToPtr != nullptr) {
				memcpy(A->ToPtr, A->FromPtr, A->L);
			}
			break;
		}
		case MInstrCode::_zero: {
			switch (A->outputFldD->frml_type) {
			case 'S': { CFile->saveS(A->outputFldD, "", CRecPtr); break; }
			case 'R': { CFile->saveR(A->outputFldD, 0, CRecPtr); break; }
			default: { CFile->saveB(A->outputFldD, false, CRecPtr); break; }
			}
			break;
		}
		case MInstrCode::_output: {
			AssgnFrml(CFile, CRecPtr, A->OFldD, A->Frml, false, A->Add);
			break;
		}
		case MInstrCode::_locvar: {
			LVAssignFrml(CFile, A->LV, A->Add, A->Frml, CRecPtr);
			break;
		}
		case MInstrCode::_parfile: {
			AsgnParFldFrml(A->FD, A->PFldD, A->Frml, A->Add);
			break;
		}
		case MInstrCode::_ifThenElse: {
			if (RunBool(CFile, A->Bool, CRecPtr)) {
				RunAssign(A->Instr);
			}
			else {
				RunAssign(A->ElseInstr);
			}
			break;
		}
		}
	}
}

void Merge::WriteOutp(OutpRD* RD)
{
	OutpFD* OD;
	while (RD != nullptr) {
		if (RunBool(CFile, RD->Bool, CRecPtr)) {
			OD = RD->OD;
			if (OD == nullptr /*dummy */) RunAssign(RD->Ass);
			else {
				CFile = OD->FD;
				CRecPtr = OD->RecPtr;
				CFile->ClearDeletedFlag(CRecPtr);
				RunAssign(RD->Ass);
#ifdef FandSQL
				if (CFile->IsSQLFile) OD->Strm->PutRec;
				else
#endif
				{
					CFile->PutRec(CRecPtr);
					if (OD->Append && (CFile->FF->file_type == FileType::INDEX)) {
						CFile->FF->TryInsertAllIndexes(CFile->IRec, CRecPtr);
					}
				}
			}
		}
		RD = RD->pChain;
	}
}

void Merge::OpenInpM()
{
	NRecsAll = 0;
	for (short I = 1; I <= MaxIi; I++)
		/* !!! with IDA[I]^ do!!! */ {
		CFile = IDA[I]->Scan->FD;
		if (IDA[I]->IsInplace) IDA[I]->Md = CFile->NewLockMode(ExclMode);
		else IDA[I]->Md = CFile->NewLockMode(RdMode);
		IDA[I]->Scan->ResetSort(IDA[I]->SK, IDA[I]->Bool, IDA[I]->Md, IDA[I]->SQLFilter, CRecPtr);
		NRecsAll += IDA[I]->Scan->NRecs;
	}
}

void Merge::OpenOutp()
{
	OutpFD* OD = OutpFDRoot;
	while (OD != nullptr) {
		CFile = OD->FD;
#ifdef FandSQL
		if (CFile->IsSQLFile) {
			New(Strm, Init);
			Strm->OutpRewrite(OD->Append);
			CRecPtr = OD->RecPtr;
			SetTWorkFlag();
		}
		else
#endif
		{
			if (OD->InplFD != nullptr) {
				OD->FD = CFile->OpenDuplicateF(true);
			}
			else {
				OD->Md = CFile->FF->RewriteFile(OD->Append);
			}
			OD = OD->pChain;
		}
	}
}

void Merge::CloseInpOutp()
{
	OutpFD* OD = OutpFDRoot;
	while (OD != nullptr) {
		CFile = OD->FD;
		OD->FD->ClearRecSpace(OD->RecPtr);
#ifdef FandSQL
		if (CFile->IsSQLFile) /* !!! with Strm^ do!!! */ {
			OutpClose(); Done();
		}
		else
#endif
		{
			if (OD->InplFD != nullptr) {
				CFile = OD->InplFD;
				CFile->FF->SubstDuplF(OD->FD, true);
			}
			else CFile->OldLockMode(OD->Md);
		}
		OD = OD->pChain;
	}
	for (short i = 1; i <= MaxIi; i++) {
		IDA[i]->Scan->Close();
		CFile->ClearRecSpace(IDA[i]->ForwRecPtr);
		CFile->OldLockMode(IDA[i]->Md);
	}
}

void Merge::MoveForwToRecM(InpD* ID)
{
	CFile = ID->Scan->FD;
	CRecPtr = CFile->FF->RecPtr;
	memcpy(CRecPtr, ID->ForwRecPtr, CFile->FF->RecLen + 1);
	ID->Count = ID->Count + 1;
	ChkD* C = ID->Chk;
	if (C != nullptr) {
		ID->Error = false;
		ID->Warning = false;
		ID->ErrTxtFrml->S = "";  // ID->ErrTxtFrml->S[0] = 0;
		while (C != nullptr) {
			if (!RunBool(CFile, C->Bool, CRecPtr)) {
				ID->Warning = true;
				ID->ErrTxtFrml->S = RunShortStr(CFile, C->TxtZ, CRecPtr);
				if (!C->Warning) {
					ID->Error = true;
					return;
				}
			}
			C = C->pChain;
		}
	}
}

void Merge::SetMFlds(KeyFldD* M)
{
	FieldDescr* F = nullptr;
	std::vector<ConstListEl>::iterator it0 = OldMFlds.begin();
	while (M != nullptr) {
		F = M->FldD;
		switch (F->frml_type) {
		case 'S': { CFile->saveS(F, it0->S, CRecPtr); break; }
		case 'R': { CFile->saveR(F, it0->R, CRecPtr); break; }
		default: { CFile->saveB(F, it0->B, CRecPtr); break; }
		}
		M = M->pChain;

		if (it0 != OldMFlds.end()) it0++;
	}
}

void Merge::MergeProcM()
{
	WORD res = 0;
	for (WORD i = 1; i <= MaxIi; i++) {
		InpD* ID = IDA[i];
		if (ID->Exist)
			do {
				MoveForwToRecM(ID);
				SumUp(CFile, ID->Sum, CRecPtr);
				WriteOutp(ID->RD);
				ReadInpFileM(ID);
				if (ID->Scan->eof) res = _gt;
				else {
					res = CompMFlds(ID->MFld);
					if (res == _lt) CFileError(CFile, 607);
				}
			} while (res != _gt);
		else {
			CFile = ID->Scan->FD;
			CRecPtr = CFile->FF->RecPtr;
			CFile->ZeroAllFlds(CRecPtr, false);
			SetMFlds(ID->MFld);
		}
	}
}

void Merge::JoinProc(WORD Ii, bool& EmptyGroup)
{
	if (Ii > MaxIi) {
		if (!EmptyGroup) {
			for (WORD I = 1; I <= MaxIi; I++) {
				SumUp(CFile, IDA[I]->Sum, CRecPtr);
			}
			WriteOutp(IDA[MaxIi]->RD);
		}
	}
	else {
		InpD* ID = IDA[Ii];
		if (ID->Exist) {
			ID->Scan->SeekRec(ID->IRec - 1);
			ID->Count = 0.0;
			CRecPtr = ID->ForwRecPtr;
			ID->Scan->GetRec(CRecPtr);
			WORD res;
			do {
				MoveForwToRecM(ID);
				JoinProc(Ii + 1, EmptyGroup);
				ReadInpFileM(ID);
				if (ID->Scan->eof) {
					res = _gt;
				}
				else {
					res = CompMFlds(ID->MFld);
					if (res == _lt) CFileError(CFile, 607);
				}
			} while (res == _gt);
		}
		else {
			CFile = ID->Scan->FD;
			CRecPtr = CFile->FF->RecPtr;
			EmptyGroup = true;
			CFile->ZeroAllFlds(CRecPtr, false);
			SetMFlds(ID->MFld);
			JoinProc(Ii + 1, EmptyGroup);
		}
	}
}
