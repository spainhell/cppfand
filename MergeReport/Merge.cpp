#include "Merge.h"
#include "../Common/compare.h"
#include "../Core/Compiler.h"
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
	std::vector<KeyInD*> KI;
	bool WasOi = false, WasSqlFile = false, CompLex = false;
	ResetCompilePars();
	g_compiler->RdLex();
	ResetLVBD();
	if (g_compiler->IsKeyWord("VAR")) {
		g_compiler->RdLocDcl(&LVBD, false, false, 'M');
	}
	WhatToRd = 'I';
	ReadingOutpBool = false;
	WasSqlFile = false;
	Ii = 0;
	g_compiler->TestLex('#');

	do {
		g_compiler->ReadChar();
		bool err = true;
		WORD I = 0;
		if (CurrChar == 'I') {
			g_compiler->ReadChar();
			if (isdigit(CurrChar)) {
				I = CurrChar - '0';
				g_compiler->ReadChar();
				if (CurrChar == '_') {
					g_compiler->RdLex();
					err = false;
				}
			}
		}
		if (err) {
			g_compiler->Error(89);
		}
		else {
			Ii++;
			if (I != Ii) {
				g_compiler->OldError(61);
			}
			ID = new InpD();
			IDA[Ii] = ID;
			FD = g_compiler->RdFileName();

			std::unique_ptr<Compiler> report_compiler = std::make_unique<Compiler>(FD);
			report_compiler->rdFldNameType = FieldNameType::P;

#ifdef FandSQL
			if (report_compiler->processing_F->typSQLFile) WasSqlFile = true;
#endif 
			for (int16_t i = 1; i <= Ii - 1; i++) {
				if (InpFD_M(i) == FD) report_compiler->OldError(26);
			}
			CViewKey = report_compiler->RdViewKey(FD);
			if (Lexem == '!') {
				report_compiler->RdLex();
				ID->AutoSort = true;
			}
			ID->Op = _const;
			ID->OpErr = _const;
			ID->OpWarn = _const;
			KI.clear();
			ID->ForwRecPtr = report_compiler->processing_F->GetRecSpace();
			FD->FF->RecPtr = report_compiler->processing_F->GetRecSpace();
			if (Lexem == '(') {
				report_compiler->RdLex();
				g_compiler->processing_F = report_compiler->processing_F;
				ID->Bool = report_compiler->RdKeyInBool(KI, false, false, ID->SQLFilter, this);
				report_compiler->Accept(')');
			}
			ID->Scan = new XScan(FD, CViewKey, KI, true);
			if (!(Lexem == ';' || Lexem == '#' || Lexem == 0x1A)) {
				report_compiler->RdKFList(ID->MFld, FD);
			}
			if (Ii > 1) {
				if (IDA[Ii - 1]->MFld.empty()) {
					if (!ID->MFld.empty()) {
						report_compiler->OldError(22);
					}
				}
				else if (ID->MFld.empty()) {
					CopyPrevMFlds();
				}
				else {
					CheckMFlds(IDA[Ii - 1]->MFld, ID->MFld);
				}
			}
			RdAutoSortSK_M(ID, report_compiler);
			report_compiler->TestLex('#');
		}
	} while (ForwChar == 'I');

	MaxIi = Ii;
	MakeOldMFlds();
	OldMXStr.Clear();
	OutpFDRoot = nullptr; OutpRDs = nullptr; Join = false; WasOi = false;
	g_compiler->rdFldNameType = FieldNameType::none;

	while (true) {
		g_compiler->ReadChar();
		if (CurrChar == 'O') {
			g_compiler->ReadChar();
			if (isdigit(CurrChar)) {
				if (Join) g_compiler->Error(91);
				WasOi = true;
				Oi = CurrChar - '0';
				if ((Oi == 0) || (Oi > MaxIi)) g_compiler->Error(62);
				goto label4;
			}
			else if (CurrChar == '*') {
				if (WasOi) g_compiler->Error(91);
				if (WasSqlFile) g_compiler->Error(155);
				Join = true;
				Oi = MaxIi;
			label4:
				g_compiler->ReadChar();
				if (CurrChar != '_') g_compiler->Error(90);
				g_compiler->RdLex();
				WhatToRd = 'i';
				ChainSum = false;
				RdOutpRD(&(IDA[Oi]->RD));
			}
			else if (CurrChar == '_') {
				g_compiler->RdLex();
				WhatToRd = 'O';
				ChainSum = true;
				RdOutpRD(&OutpRDs);
			}
			else g_compiler->Error(90);
		}
		else g_compiler->Error(90);
		if (Lexem != 0x1A) {
			g_compiler->TestLex('#');
			continue;
		}
		break;
	}

	for (int16_t I = 1; I <= MaxIi; I++) {
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
		if (!b) {
			GoExit();
		}
		CloseInpOutp();
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
	g_compiler->TestIdentif();
	if ((LexWord == "O") && g_compiler->IsForwPoint() && !WasIiPrefix) {
		g_compiler->RdLex(); g_compiler->RdLex();
		if ((FrmlSumEl != nullptr) || ReadingOutpBool) {
			g_compiler->Error(99);
		}
		result = RdOutpFldName(FTyp);
		return result;
	}
	if (g_compiler->IsForwPoint()) {
		result = RdDirFilVar_M(FTyp, WasIiPrefix);
		TestSetSumIi();
		return result;
	}
	if (!WasIiPrefix) {
		if (g_compiler->FindLocVar(&LVBD, &LV)) {
			g_compiler->RdLex();
			TestNotSum();
			result = new FrmlElemLocVar(_getlocvar, LV);
			FTyp = LV->f_typ;
			return result;
		}
	}
	if (g_compiler->IsKeyWord("COUNT")) {
		TestNotSum();
		SetIi_Merge(WasIiPrefix);
		result = new FrmlElemInp(_count, IDA[Ii]);
		FTyp = 'R';
		return result;
	}
	if (g_compiler->IsKeyWord("GROUP")) {
		TestNotSum();
		if (WasIiPrefix) g_compiler->OldError(41);
		result = new FrmlElemMerge(_mergegroup, &MergOpGroup);
		FTyp = 'R';
		return result;
	}
	if (g_compiler->IsKeyWord("ERROR")) {
		Err('m', WasIiPrefix);
		result = (FrmlElem*)(&IDA[Ii]->OpErr);
		FTyp = 'B';
		return result;
	}
	if (g_compiler->IsKeyWord("WARNING")) {
		Err('m', WasIiPrefix);
		result = (FrmlElem*)(&IDA[Ii]->OpWarn);
		FTyp = 'B';
		return result;
	}
	if (g_compiler->IsKeyWord("ERRORTEXT")) {
		Err('m', WasIiPrefix);
		result = IDA[Ii]->ErrTxtFrml;
		FTyp = 'S';
		return result;
	}
	if (WasIiPrefix) {
		FD = InpFD_M(Ii);
		Z = g_compiler->TryRdFldFrml(FD, FTyp, this);
	}
	else Z = FindIiandFldFrml_M(&FD, FTyp);
	if (Z == nullptr) {
		if (g_compiler->IsKeyWord("N")) {
			// same as "COUNT"
			TestNotSum();
			SetIi_Merge(WasIiPrefix);
			result = new FrmlElemInp(_count, IDA[Ii]);
			FTyp = 'R';
			return result;
		}
		if (g_compiler->IsKeyWord("M")) {
			// same as "GROUP"
			TestNotSum();
			if (WasIiPrefix) g_compiler->OldError(41);
			result = new FrmlElemMerge(_mergegroup, &MergOpGroup);
			FTyp = 'R';
			return result;
		}
		g_compiler->Error(8);
	}
	TestSetSumIi();
	result = g_compiler->FrmlContxt(Z, FD, FD->FF->RecPtr);
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
		z = g_compiler->TryRdFldFrml(*FD, FTyp, this);
		if (z != nullptr) { Ii = Oi; goto label1; }
	}
	for (i = 1; i <= MaxIi; i++) {     /* search in  I1 .. In, for Oi only I1 .. Ii*/
		*FD = InpFD_M(i);
		z = g_compiler->TryRdFldFrml(*FD, FTyp, this);
		if (z != nullptr) { Ii = i; goto label1; }
		if ((WhatToRd == 'i') && (i == Oi)) goto label1;
	}
label1:
	return z;
}

FrmlElem* Merge::RdDirFilVar_M(char& FTyp, bool wasIiPrefix)
{
	LinkD* LD = nullptr;
	FileD* FD = nullptr;
	FrmlElem* result = nullptr;
	FileD* processed_file = nullptr;

	if (wasIiPrefix) {
		processed_file = InpFD_M(Ii);
		g_compiler->processing_F = processed_file;
		if (!g_compiler->IsRoleName(true, processed_file, &FD, &LD)) {
			g_compiler->Error(9);
		}
	}
	else {
		if (!Join && (WhatToRd == 'i')) {
			Ii = Oi;
			processed_file = InpFD_M(Ii);
			g_compiler->processing_F = processed_file;
			if (g_compiler->IsRoleName(true, processed_file, &FD, &LD)) {
				goto label2;
			}
		}
		for (int16_t i = 1; i <= MaxIi; i++) {
			processed_file = InpFD_M(i);
			g_compiler->processing_F = processed_file;
			if (g_compiler->IsRoleName(true, processed_file, &FD, &LD)) {
				Ii = i;
				goto label2;
			}
			if ((WhatToRd == 'i') && (i == Oi)) {
				goto label1;
			}
		}
	label1:
		g_compiler->Error(9);
	}
label2: 
	g_compiler->Accept('.'); // '.'
	result = g_compiler->RdFAccess(FD, LD, FTyp);
	if (LD == nullptr) {
		Ii = 0;
	}
	else {
		result = g_compiler->FrmlContxt(result, processed_file, processed_file->FF->RecPtr);
	}

	return result;
}

FrmlElem* Merge::RdOutpFldName(char& FTyp)
{
	FrmlElem* result = nullptr;
	if (RD->OD == nullptr /*dummy*/) {
		g_compiler->Error(85);
	}
	else {
		FieldDescr* rdFldName = g_compiler->RdFldName(RD->OD->FD);
		FrmlElem* makeFldFrml = g_compiler->MakeFldFrml(rdFldName, FTyp);
		result = g_compiler->FrmlContxt(makeFldFrml, RD->OD->FD, RD->OD->RecPtr);
	}
	return result;
}

void Merge::MakeOldMFlds()
{
	//KeyFldD* M = IDA[1]->MFld;
	WORD n = 0;
	OldMFlds.clear();
	//while (M != nullptr) {
	for (KeyFldD* M : IDA[1]->MFld) {
		OldMFlds.push_back(ConstListEl());
		//M = M->pChain;
	}
}

void Merge::RdAutoSortSK_M(InpD* ID, std::unique_ptr<Compiler>& compiler)
{
	//KeyFldD* M = nullptr;
	KeyFldD* SK = nullptr;
	if (!ID->AutoSort) return;
	//M = ID->MFld;

	//while (M != nullptr) {
	for (KeyFldD* M : ID->MFld) {
		SK = new KeyFldD();
		*SK = *M;
		ID->SK.push_back(SK);
		//M = M->pChain;
	}
	if (Lexem == ';') {
		compiler->RdLex();
		compiler->RdKFList(ID->SK, compiler->processing_F);
	}
	if (ID->SK.empty()) {
		compiler->OldError(60);
	}
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
		if ((inputFile->FF->file_type == outputFile->FF->file_type) && g_compiler->FldTypIdentity(inputField, outputField) && (inputField->field_type != FieldType::TEXT)
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
			FrmlElem* Z = g_compiler->MakeFldFrml(inputField, FTyp);
			Z = AdjustComma_M(Z, inputField, _divide);
			Z = AdjustComma_M(Z, outputField, _times);
			newAssign->Frml = g_compiler->FrmlContxt(Z, inputFile, inputFile->FF->RecPtr);
		}
	}
	//newAssign->pChain = RD_Ass;
	outputRD->Ass.push_back(newAssign);
	//label1:
}

FrmlElem* Merge::AdjustComma_M(FrmlElem* Z1, FieldDescr* F, instr_type Op)
{
	FrmlElemFunction* Z = nullptr;
	FrmlElemNumber* Z2 = nullptr;
	FrmlElem* result = Z1;
	if (F->field_type != FieldType::FIXED) return result;
	if ((F->Flg & f_Comma) == 0) return result;
	Z2 = new FrmlElemNumber(_const, 0, Power10[F->M]); // GetOp(_const, sizeof(double));
	//Z2->rdb = Power10[F->M];
	Z = new FrmlElemFunction(Op, 0); // GetOp(oper, 0);
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
		result = g_compiler->FindFldName(pFile, fieldName);
		if (result != nullptr) {
			Ii = Oi;
			return result;
		}
	}
	for (short i = 1; i <= MaxIi; i++) {    /* search in  I1 .. In, for Oi only I1 .. Ii*/
		FileD* pFile = InpFD_M(i);
		result = g_compiler->FindFldName(pFile, fieldName);
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
			g_compiler->OldError(173);
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
	if (g_compiler->IsKeyWord("BEGIN")) {
		result = RdAssSequ();
		g_compiler->AcceptKeyWord("END");
		return result;
	}

	AD = new AssignD();
	g_compiler->TestIdentif();
	if (g_compiler->IsKeyWord("IF")) {
		AD->Kind = MInstrCode::_ifThenElse;
		AD->Bool = g_compiler->RdBool(this);
		g_compiler->AcceptKeyWord("THEN");
		AD->Instr = RdAssign_M();
		if (g_compiler->IsKeyWord("ELSE")) AD->ElseInstr = RdAssign_M();
	}
	else if (ForwChar == '.') {
		AD->Kind = MInstrCode::_parfile;
		FD = g_compiler->RdFileName();
		if (!FD->IsParFile) g_compiler->OldError(9);
		TestIsOutpFile(FD);
		g_compiler->Accept('.');
		AD->FD = FD;
		F = g_compiler->RdFldName(FD);
		AD->PFldD = F;
		if ((F->Flg & f_Stored) == 0) g_compiler->OldError(14);
		g_compiler->RdAssignFrml(F->frml_type, AD->Add, &AD->Frml, this);
	}
	else if (g_compiler->FindLocVar(&LVBD, &LV)) {
		g_compiler->RdLex();
		AD->Kind = MInstrCode::_locvar;
		AD->LV = LV;
		g_compiler->RdAssignFrml(LV->f_typ, AD->Add, &AD->Frml, this);
	}
	else {
		if (RD->OD == nullptr) g_compiler->Error(72);  /*dummy*/
		else {
			AD->Kind = MInstrCode::_output;
			F = g_compiler->RdFldName(RD->OD->FD);
			AD->OFldD = F;
			if ((F->Flg & f_Stored) == 0) g_compiler->OldError(14);
			g_compiler->RdAssignFrml(F->frml_type, AD->Add, &AD->Frml, this);
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
		g_compiler->RdLex();
		if (!(Lexem == 0x1A || Lexem == '#') && !g_compiler->TestKeyWord("END")) goto label1;
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
	if (g_compiler->IsKeyWord("DUMMY")) RD->OD = nullptr;
	else {
		FD = g_compiler->RdFileName();
		OD = OutpFDRoot;
		while (OD != nullptr) {
			if (OD->FD == FD) {
				if (Lexem == '+') {
					if (OD->Append) {
						g_compiler->RdLex();
					}
					else {
						g_compiler->Error(31);
					}
				}
				else if (OD->Append) {
					g_compiler->Error(31);
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
				if (FD->typSQLFile) g_compiler->Error(172);
			}
		}
		if (Lexem == '+')
		{
			OD->Append = true;
			if (OD->InplFD != nullptr) g_compiler->Error(32);
			g_compiler->RdLex();
		}
		else OD->Append = false;
		if (OutpFDRoot == nullptr) OutpFDRoot = OD;
		else ChainLast(OutpFDRoot, OD);
	label1:
		RD->OD = OD;
	}
	if (Lexem == '(')
	{
		g_compiler->RdLex();
		ReadingOutpBool = true;
		RD->Bool = g_compiler->RdBool(this);
		ReadingOutpBool = false;
		g_compiler->Accept(')');
	}
	if (!(Lexem == '#' || Lexem == 0x1A)) {
		RD->Ass = RdAssSequ();
	}
	MakeImplAssign();
}

WORD Merge::CompMFlds(std::vector<KeyFldD*>& M)
{
	XString x;
	x.PackKF(CFile, M, CRecPtr);
	return CompStr(x.S, OldMXStr.S);
}

void Merge::SetOldMFlds(std::vector<KeyFldD*>& M)
{
	//ConstListEl* C = nullptr;
	FieldDescr* F = nullptr;
	OldMXStr.Clear();
	//C = OldMFlds;
	for (size_t i = 0; i < OldMFlds.size(); i++) {
		ConstListEl* C = &OldMFlds[i];
		F = M[i]->FldD;
		switch (F->frml_type) {
		case 'S': {
			C->S = CFile->loadS(F, CRecPtr);
			OldMXStr.StoreStr(C->S, M[i]);
			break;
		}
		case 'R': {
			C->R = CFile->loadR(F, CRecPtr);
			OldMXStr.StoreReal(C->R, M[i]);
			break;
		}
		default: {
			C->B = CFile->loadB(F, CRecPtr);
			OldMXStr.StoreBool(C->B, M[i]);
			break;
		}
		}
		//C = (ConstListEl*)C->pChain;
		//M = M->pChain;
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
			New(Strm, init);
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
	//ChkD* C = ID->Chk;
	//if (C != nullptr) {
	if (!ID->Chk.empty()) {
		ID->Error = false;
		ID->Warning = false;
		ID->ErrTxtFrml->S = "";  // ID->ErrTxtFrml->S[0] = 0;
		for (ChkD* C : ID->Chk) { // while (C != nullptr}
			if (!RunBool(CFile, C->Bool, CRecPtr)) {
				ID->Warning = true;
				ID->ErrTxtFrml->S = RunShortStr(CFile, C->TxtZ, CRecPtr);
				if (!C->Warning) {
					ID->Error = true;
					return;
				}
			}
			//C = C->pChain;
		}
	}
}

void Merge::SetMFlds(std::vector<KeyFldD*>& M)
{
	FieldDescr* F = nullptr;
	std::vector<ConstListEl>::iterator it0 = OldMFlds.begin();
	//while (M != nullptr) {
	for (KeyFldD* M : M) {
		F = M->FldD;
		switch (F->frml_type) {
		case 'S': { CFile->saveS(F, it0->S, CRecPtr); break; }
		case 'R': { CFile->saveR(F, it0->R, CRecPtr); break; }
		default: { CFile->saveB(F, it0->B, CRecPtr); break; }
		}
		//M = M->pChain;

		if (it0 != OldMFlds.end()) ++it0;
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
