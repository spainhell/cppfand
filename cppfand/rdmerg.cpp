#include "rdmerg.h"

#include "compile.h"
#include "legacy.h"
#include "rdfildcl.h"
#include "rdrprt.h"

char WhatToRd = '\0'; /*i=Oi output FDs;O=O outp.FDs*/
bool ReadingOutpBool = false;
WORD Ii = 0, Oi = 0, SumIi = 0;
OutpRD* RD = nullptr;

FileD* InpFD_M(WORD I)
{
	return IDA[I]->Scan->FD;
}

bool RdIiPrefix_M()
{
	auto result = false;
	if ((ForwChar == '.') && (LexWord.length() == 2) && (LexWord[1] == 'I') &&
		(isdigit(LexWord[2]) && LexWord[2] != '0')) {
		Ii = LexWord[2] - '0';
		if ((Ii > MaxIi) || (WhatToRd == 'i') && (Ii > Oi)) Error(9);
		RdLex(); RdLex(); result = true;
	}
	else {
		Ii = 0; result = false;
	}
	return result;
}

FrmlPtr FindIiandFldFrml_M(FileD* FD, char& FTyp)
{
	integer i = 0; FrmlPtr z = nullptr;
	if (!Join && (WhatToRd == 'i')) {   /* for Oi search first in Ii*/
		FD = InpFD_M(Oi); z = TryRdFldFrml(FD, FTyp);
		if (z != nullptr) { Ii = Oi; goto label1; };
	}
	for (i = 1; i < MaxIi; i++) {     /* search in  I1 .. In, for Oi only I1 .. Ii*/
		FD = InpFD_M(i); z = TryRdFldFrml(FD, FTyp);
		if (z != nullptr) { Ii = i; goto label1; }
		if ((WhatToRd == 'i') && (i == Oi)) goto label1;
	}
label1:
	return z;
}

FrmlPtr RdFldNameFrmlM(char& FTyp)
{
	bool WasIiPrefix = false;
	FieldDescr* F = nullptr;                         /*RdFldNameFrml - body*/
	FrmlPtr Z = nullptr;
	LocVar* LV = nullptr; FileD* FD = nullptr;
	FrmlElem* result = nullptr;

	WasIiPrefix = RdIiPrefix_M();
	if ((FrmlSumEl != nullptr) && FrstSumVar) SumIi = 0;
	TestIdentif();
	if ((LexWord == 'O') && IsForwPoint() && !WasIiPrefix) {
		RdLex(); RdLex(); if ((FrmlSumEl != nullptr) || ReadingOutpBool) Error(99);
		RdOutpFldName(FTyp, result); return result;
	}
	if (IsForwPoint()) {
		RdDirFilVar_M(FTyp, result); TestSetSumIi_M(); return result;
	}
	if (!WasIiPrefix) if (FindLocVar(LVBD.Root, &LV)) {
		RdLex(); TestNotSum_M(); result = (FrmlPtr)(&LV->Op); FTyp = LV->FTyp; return result;
	}
	if (IsKeyWord("COUNT")) {
	label1:
		TestNotSum_M(); SetIi_M(); result = (FrmlPtr)(&IDA[Ii]->Op); FTyp = 'R'; return result;
	}
	if (IsKeyWord("GROUP")) {
	label2:
		TestNotSum_M(); if (WasIiPrefix) OldError(41);
		result = (FrmlPtr)(&MergOpGroup); FTyp = 'R'; return result;
	}
	if (IsKeyWord("ERROR")) {
		Err_M(); result = (FrmlPtr)(&IDA[Ii]->OpErr); FTyp = 'B'; return result;
	}
	if (IsKeyWord("WARNING")) {
		Err_M(); result = (FrmlPtr)(&IDA[Ii]->OpWarn); FTyp = 'B'; return result;
	}
	if (IsKeyWord("ERRORTEXT")) {
		Err_M(); result = IDA[Ii]->ErrTxtFrml; FTyp = 'S'; return result;
	}
	if (WasIiPrefix) { FD = InpFD_M(Ii); Z = TryRdFldFrml(FD, FTyp); }
	else Z = FindIiandFldFrml_M(FD, FTyp);
	if (Z == nullptr) {
		if (IsKeyWord('N')) goto label1; if (IsKeyWord('M')) goto label2; Error(8);
	}
	TestSetSumIi_M();
	result = FrmlContxt(Z, FD, FD->RecPtr);
	return result;
}

void RdDirFilVar_M(char& FTyp, FrmlElem* res)
{
	LinkD* LD = nullptr; FileD* FD = nullptr;
	integer I = 0;
	FrmlPtr Z = nullptr;
	if (WasIiPrefix)
	{
		CFile = InpFD_M(Ii);
		if (!IsRoleName(true, FD, LD)) Error(9);
	}
	else {
		if (!Join && (WhatToRd == 'i'))
		{
			Ii = Oi; CFile = InpFD_M(Ii);
			if (IsRoleName(true, FD, LD)) goto label2;
		}
		for (I = 1; I < MaxIi; I++)
		{
			CFile = InpFD_M(I);
			if (IsRoleName(true, FD, LD)) { Ii = I; goto label2; }
			if ((WhatToRd == 'i') && (I == Oi)) goto label1;
		}
	label1:
		Error(9);
	}
label2:
	Accept('.');
	Z = RdFAccess(FD, LD, FTyp);
	if (LD == nullptr) Ii = 0; else Z = FrmlContxt(Z, CFile, CFile->RecPtr);
	res = Z;
}

void TestSetSumIi_M()
{
	if ((FrmlSumEl != nullptr) && (Ii != 0)) {
		if (FrstSumVar || (SumIi == 0)) SumIi = Ii;
		else if (SumIi != Ii) OldError(27);
	}
}

void RdOutpFldName(char& FTyp, FrmlElem* res)
{
	if (RD->OD == nullptr /*dummy*/) Error(85);
	/* !!! with RD->OD^ do!!! */
	res = FrmlContxt(MakeFldFrml(RdFldName(RD->OD->FD), FTyp),
		RD->OD->FD, RD->OD->RecPtr);
}

void SetIi_M()
{
	if (!WasIiPrefix) {
		if (!Join && (WhatToRd == 'i')) Ii = Oi;
		else Ii = 1;
	}
}

void TestNotSum_M()
{
	if (FrmlSumEl != nullptr) OldError(41);
}

void Err_M()
{
	TestNotSum_M(); SetIi_M();
	if (IDA[Ii]->ErrTxtFrml == nullptr)
	{
		IDA[Ii]->ErrTxtFrml = GetOp(_const, 256);
	}
}

void ChainSumElM()
{
	if (FrstSumVar || (SumIi == 0)) SumIi = 1;
	FrmlSumEl->Chain = IDA[SumIi]->Sum; IDA[SumIi]->Sum = FrmlSumEl;
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
	if (IsKeyWord("VAR")) RdLocDcl(&LVBD, false, false, 'M');
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
		for (I = 1; I <= Ii - 1; I++) if (InpFD_M(I) == FD) OldError(26);
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
			ID->Bool = RdKeyInBool(KI, false, false, ID->SQLFilter);
			Accept(')');
		}
		//New(ID->Scan, Init(FD, CViewKey, KI, true));
		ID->Scan = new XScan(FD, CViewKey, KI, true);
		if (!(Lexem == ';' || Lexem == '#' || Lexem == 0x1A)) RdKFList(&ID->MFld, FD);
		if (Ii > 1) {
			if (IDA[Ii - 1]->MFld == nullptr) {
				if (ID->MFld != nullptr) OldError(22);
			}
			else if (ID->MFld == nullptr) CopyPrevMFlds_M();
			else CheckMFlds_M(IDA[Ii - 1]->MFld, ID->MFld);
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
			RdOutpRD(IDA[Oi]->RD);
		}
		else if (CurrChar == '_') {
			RdLex(); WhatToRd = 'O'; ChainSumEl = ChainSumElM;
			RdOutpRD(OutpRDs);
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
		if (ID->ErrTxtFrml != nullptr) RdChkDsFromPos(ID->Scan->FD, ID->Chk);
	}
}

void CopyPrevMFlds_M()
{
	KeyFldD* M; KeyFldD* MNew;
	FieldDescr* F;
	pstring S;
	M = IDA[Ii - 1]->MFld;
	S = LexWord;
	while (M != nullptr)
	{
		LexWord = M->FldD->Name; F = FindFldName(InpFD_M(Ii));
		if (F == nullptr) OldError(8);
		if (!FldTypIdentity(M->FldD, F)) OldError(12);
		MNew = (KeyFldD*)GetStore(sizeof(*MNew));
		Move(M, MNew, sizeof(*MNew));
		MNew->FldD = F; ChainLast(IDA[Ii]->MFld, MNew); M = (KeyFldD*)M->Chain;
	}
	LexWord = S;
}

void CheckMFlds_M(KeyFldD* M1, KeyFldD* M2)
{
	while (M1 != nullptr)
	{
		if (M2 == nullptr) OldError(30);
		if (!FldTypIdentity(M1->FldD, M2->FldD) 
			|| (M1->Descend != M2->Descend)
			|| (M1->CompLex != M2->CompLex))
			OldError(12);
		M1 = (KeyFldD*)M1->Chain; 
		M2 = (KeyFldD*)M2->Chain;
	}
	if (M2 != nullptr) OldError(30);
}

void MakeOldMFlds()
{
	KeyFldDPtr M; WORD n;
	M = IDA[1]->MFld; OldMFlds = nullptr;
	while (M != nullptr) {
		switch (M->FldD->FrmlTyp)
		{
		case 'B': n = 1; break;
		case 'R': n = sizeof(double); break;
		default: n = 256; break;
		}
		ChainLast(OldMFlds, (Chained*)GetStore(sizeof(void*) + n));
		M = (KeyFldD*)M->Chain;
	}
}

void RdAutoSortSK_M(InpD* ID)
{
	KeyFldDPtr M, SK;
	if (!ID->AutoSort) return; 
	M = ID->MFld;
	while (M != nullptr) {
		SK = (KeyFldD*)GetStore(sizeof(*SK));
		Move(M, SK, sizeof(SK));
		ChainLast(ID->SK, SK); 
		M = (KeyFldD*)M->Chain;
	}
	if (Lexem == ';') { RdLex(); RdKFList(&ID->SK, CFile); }
	if (ID->SK == nullptr) OldError(60);
}

void ImplAssign(OutpRD* RD, FieldDescr* FNew)
{
	FileD* FDNew = nullptr; FileD* FD = nullptr;
	FieldDescr* F = nullptr; AssignD* A = nullptr;
	AssignD* A1 = nullptr;
	void* RPNew = nullptr; void* RP = nullptr;
	FrmlPtr Z = nullptr; char FTyp = 0; pstring S;
	
	FDNew = RD->OD->FD; 
	RPNew = RD->OD->RecPtr; 
	S = LexWord;
	//A = (AssignD*)GetZStore(sizeof(*A)); 
	A = new AssignD();
	A1 = RD->Ass;
	LexWord = FNew->Name; 
	FindIiandFldD(F);
	if ((F == nullptr) || (F->FrmlTyp != FNew->FrmlTyp) ||
		(F->FrmlTyp == 'R') && (F->Typ != FNew->Typ)) {
		A->Kind = _zero; 
		A->FldD = FNew;
	}
	else {
		FD = InpFD_M(Ii); 
		RP = FD->RecPtr;
		if ((FD->Typ == FDNew->Typ) && FldTypIdentity(F, FNew) &&
			(F->Typ != 'T') && (F->Flg && f_Stored != 0) && (FNew->Flg == F->Flg)) {
			A->Kind = _move; 
			A->L = FNew->NBytes;
			// TODO: nutno doresit ToPtr a FromPtr
			//A->ToPtr = uintptr_t(RPNew) + FNew->Displ;
			//A->FromPtr = (uintptr_t(RP) + F->Displ);
			if ((A1 != nullptr) && (A1->Kind == _move) &&
				(uintptr_t(A1->FromPtr) + A1->L == uintptr_t(A->FromPtr)) &&
				(uintptr_t(A1->ToPtr) + A1->L == uintptr_t(A->ToPtr))) {
				A1->L = A1->L + A->L;
				ReleaseStore(A);
				goto label1;
			}
		}
		else {
			A->Kind = _output; 
			A->OFldD = FNew; 
			Z = MakeFldFrml(F, FTyp);
			Z = AdjustComma_M(Z, F, _divide); 
			Z = AdjustComma_M(Z, FNew, _times);
			A->Frml = FrmlContxt(Z, FD, FD->RecPtr);
		}
	}
	A->Chain = A1; 
	RD->Ass = A;
label1:
	LexWord = S;
}

FrmlElem* AdjustComma_M(FrmlElem* Z1, FieldDescr* F, char Op)
{
	FrmlPtr Z, Z2;
	FrmlElem* result = Z1;
	if (F->Typ != 'F') return result;
	if (F->Flg && f_Comma == 0) return result;
	Z2 = GetOp(_const, sizeof(double));
	Z2->R = Power10[F->M];
	Z = GetOp(Op, 0); Z->P1 = Z1; Z->P2 = Z2; result = Z;
	return result;
}

void FindIiandFldD(FieldDescr* F)
{
	integer i;
	if (!Join && (WhatToRd == 'i')) {   /* for Oi search first in Ii*/
		F = FindFldName(InpFD_M(Oi)); if (F != nullptr) { Ii = Oi; return; }
	}
	for (i = 1; i < MaxIi; i++) {    /* search in  I1 .. In, for Oi only I1 .. Ii*/
		F = FindFldName(InpFD_M(i)); if (F != nullptr) { Ii = i; return; }
		if ((WhatToRd == 'i') && (i == Oi)) return;
	}
}

bool FindAssignToF(AssignD* A, FieldDescr* F)
{
	while (A != nullptr) {
		if ((A->Kind == _output) && (A->OFldD == F) && !A->Add) return true;
		A = (AssignD*)A->Chain;
	}
	return false;
}

void MakeImplAssign()
{
	FieldDescr* FNew = nullptr; 
	AssignD* AD = nullptr;
	if (RD->OD == nullptr) return;
	FNew = RD->OD->FD->FldD;
	while (FNew != nullptr) {                 /*implic.assign   name = name*/
		if ((FNew->Flg && f_Stored != 0) && !FindAssignToF(RD->Ass, FNew)) ImplAssign(RD, FNew);
		FNew = (FieldDescr*)FNew->Chain;
	}
}

void TestIsOutpFile(FileDPtr FD)
{
	OutpFD* OFD = OutpFDRoot;
	while (OFD != nullptr) {
		if (OFD->FD == FD) OldError(173); OFD = (OutpFD*)OFD->Chain;
	}
}

AssignD* RdAssign_M()
{
	FieldDescr* F = nullptr; FileD* FD = nullptr; 
	LocVar* LV = nullptr; AssignD* AD = nullptr;
	AssignD* result = nullptr;
	if (IsKeyWord("BEGIN")) {
		result = RdAssSequ(); 
		AcceptKeyWord("END"); 
		return result;
	}
	//AD = (AssignD*)GetZStore(sizeof(*AD)); 
	AD = new AssignD();
	result = AD; 
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
		RdAssignFrml(F->FrmlTyp, AD->Add, AD->Frml);
	}
	else if (FindLocVar(LVBD.Root, &LV)) {
		RdLex(); 
		AD->Kind = _locvar; 
		AD->LV = LV;
		RdAssignFrml(LV->FTyp, AD->Add, AD->Frml);
	}
	else {
		if (RD->OD == nullptr) Error(72);  /*dummy*/
		else {
			AD->Kind = _output;
			F = RdFldName(RD->OD->FD);
			AD->OFldD = F;
			if ((F->Flg & f_Stored) == 0) OldError(14);
			RdAssignFrml(F->FrmlTyp, AD->Add, AD->Frml);
		}
	}
	return result;
}

AssignD* RdAssSequ()
{
	AssignD* A;
	AssignD* ARoot = nullptr;
label1:
	A = (AssignD*)(&ARoot);
	while (A->Chain != nullptr) A = (AssignD*)A->Chain;
	A->Chain = RdAssign_M();
	if (Lexem == ';')
	{
		RdLex();
		if (!(Lexem == 0x1A || Lexem == '#') && !TestKeyWord("END")) goto label1;
	}
	return ARoot;
}

void RdOutpRD(OutpRD* RDRoot)
{
	OutpRD* R = nullptr; FileD* FD = nullptr; 
	OutpFD* OD = nullptr; InpD* ID = nullptr;
	integer I = 0;
	//RD = (OutpRD*)GetStore(sizeof(*RD));
	RD = new OutpRD();
	if (RDRoot == nullptr) RDRoot = RD;
	else ChainLast(RDRoot, RD);
	RD->Ass = nullptr;
	RD->Bool = nullptr;
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
	if (!(Lexem == '#' || Lexem == 0x1A)) RD->Ass = RdAssSequ();
	MakeImplAssign();
}


