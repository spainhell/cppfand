#include "rdrprt.h"

#include "compile.h"
#include "legacy.h"
#include "obase.h"
#include "rdfildcl.h"
#include "rdmerg.h"
#include "runfrml.h"

BlkD* CBlk;
FloatPtrList CZeroLst;
LvDescr* LvToRd;           /*all used while translating frml*/
bool WasIiPrefix;
BlkD* CBlkSave;


FileD* InpFD(WORD I)
{
	return IDA[I]->Scan->FD;
}

bool FindInLvBlk(LvDescr* L, BlkD* B, RFldD* RF)
{
	RFldD* RF1; BlkD* B1; LvDescr* L1; bool first;
	first = true;
	auto result = false;
label1:
	while (L != nullptr) {
		B1 = L->Ft; 
		while (B1 != nullptr) {
			RF1 = B1->RFD; 
			while (RF1 != nullptr) {
				if ((Lexem == _identifier) && EquUpcase(RF1->Name, LexWord)) {
					RdLex(); 
					result = true;
					RF = RF1; 
					B = B1; 
					return result;
				}
				RF1 = (RFldD*)RF1->Chain;
			}
			B1 = (BlkD*)B1->Chain;
		}
		L = L->ChainBack;
	}
	if (first) { first = false; L = IDA[1]->FrstLvS;/*DE*/ goto label1; }
	return result;
}

FrmlPtr RdFldNameFrmlR(char& FTyp)
{
	LinkD* LD = nullptr; FileD* FD = nullptr; RFldD* RF = nullptr; LocVar* LV = nullptr;
	WORD n = 0;
	FrmlElem* Z = nullptr;
	FrmlElem* result = nullptr;
	WasIiPrefix = RdIiPrefix();
	if ((FrmlSumEl != nullptr) && FrstSumVar && (CBlk != nullptr)) {
		SumIi = 0; CBlkSave = CBlk; CBlk = nullptr;
	}
	if (IsForwPoint()) { RdDirFilVar(FTyp, &result); return result; }
	if (!WasIiPrefix && FindLocVar(&LVBD, &LV)) {
		RdLex(); TestNotSum();
		result = (FrmlElem*)(&LV->Op); FTyp = LV->FTyp; return result;
	}
	if (IsKeyWord("COUNT")) {
		TestNotSum();
		SetIi();
		result = (FrmlElem*)(&IDA[Ii]->Op);
		FTyp = 'R';
		return result;
	}
	if (IsKeyWord("GROUP")) {
		TestNotSum(); if (WasIiPrefix) OldError(41);
		result = (FrmlElem*)(&MergOpGroup); FTyp = 'R'; return result;
	}
	if (IsKeyWord("LINE")) { n = 0; goto label1; }
	if (IsKeyWord("PAGE")) { n = 1; goto label1; }
	if (IsKeyWord("PAGELIMIT")) {
		n = 2;
	label1:
		if ((FrmlSumEl == nullptr) && !WasIiPrefix) {
			Z = new FrmlElem1(_getWORDvar, 0); // GetOp(_getWORDvar, 1);
			((FrmlElem1*)Z)->N01 = n;
			result = Z;
			FTyp = 'R';
			return result;
		}
	}
	if (IsKeyWord("ERROR")) {
		Err(); result = (FrmlElem*)(&IDA[Ii]->OpErr); FTyp = 'B'; return result;
	}
	if (IsKeyWord("WARNING")) {
		Err(); result = (FrmlElem*)(&IDA[Ii]->OpWarn); FTyp = 'B'; return result;
	}
	if (IsKeyWord("ERRORTEXT")) {
		Err(); result = IDA[Ii]->ErrTxtFrml; FTyp = 'S'; return result;
	}
	if (FrmlSumEl != nullptr) {
		if (FrstSumVar) {
			if (FindInLvBlk(LvToRd->ChainBack, CBlk, RF)) {
				FTyp = RF->FrmlTyp; 
				result = RF->Frml; 
				return result;
			}
			FindInRec(FTyp, &result); 
			return result;
		}
		else if (CBlk == nullptr) { FindInRec(FTyp, &result); return result; }
		else if (OwnInBlock(FTyp, &result)) return result;
		else OldError(8);
	}
	if (OwnInBlock(FTyp, &result)) return result;
	FindInRec(FTyp, &result);
	return result;
}

bool RdIiPrefix()
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

void TestSetSumIi()
{
	if ((FrmlSumEl != nullptr) && (Ii != 0))
		if (FrstSumVar || (SumIi == 0)) SumIi = Ii;
		else if (SumIi != Ii) OldError(27);
}

FrmlPtr FindIiandFldFrml(FileD** FD, char& FTyp)
{
	integer i = 0; 
	FrmlPtr z = nullptr;;
	if (WhatToRd == 'i') {       /* search first in Ii*/
		*FD = InpFD(Oi); 
		z = TryRdFldFrml(*FD, FTyp);
		if (z != nullptr) { 
			Ii = Oi; 
			goto label1; 
		}
	}
	for (i = 1; i <= MaxIi; i++) {       /* search in  I1 .. Imax resp. Oi*/
		*FD = InpFD(i); 
		z = TryRdFldFrml(*FD, FTyp);
		if (z != nullptr) { 
			Ii = i; 
			goto label1; }
		if ((WhatToRd == 'i') && (i == Oi)) goto label1;
	}
label1:
	return z;
}

void RdDirFilVar(char& FTyp, FrmlElem** res)
{
	LinkD* LD = nullptr; 
	FileD* FD = nullptr; 
	integer I = 0; 
	FrmlPtr Z = nullptr;
	if (WasIiPrefix) {
		CFile = InpFD(Ii); 
		if (!IsRoleName(true, &FD, &LD)) Error(9);
	}
	else {
		if (!Join && (WhatToRd == 'i')) {
			Ii = Oi; 
			CFile = InpFD(Ii); 
			if (IsRoleName(true, &FD, &LD)) goto label2;
		}
		for (I = 1; I <= MaxIi; I++) {
			CFile = InpFD(I); 
			if (IsRoleName(true, &FD, &LD)) { 
				Ii = I; 
				goto label2; 
			}
			if ((WhatToRd == 'i') && (I == Oi)) goto label1;
		}
	label1:
		Error(9);
	}
label2:
	RdLex();/*'.'*/
	Z = RdFAccess(FD, LD, FTyp);
	if (LD == nullptr) Ii = 0;
	else {
		Z = FrmlContxt(Z, CFile, CFile->RecPtr); TestSetSumIi();
		if ((FrmlSumEl != nullptr) && !FrstSumVar && (CBlk != nullptr)) OldError(59);
	}
	*res = Z;
}

bool OwnInBlock(char& FTyp, FrmlElem** res)
{
	RFldD* RF;
	auto result = false;
	RF = CBlk->RFD;
	while (RF != nullptr)
	{
		if (EquUpcase(RF->Name, LexWord))
		{
			RdLex(); 
			FTyp = RF->FrmlTyp; 
			*res = RF->Frml;
			result = true; 
			return result;
		}
		RF = (RFldD*)RF->Chain;
	}
	return result;
}

void FindInRec(char& FTyp, FrmlElem** res)
{
	FileD* FD = nullptr;
	FieldDescr* F = nullptr;
	FrmlElem* Z = nullptr;
	if (WasIiPrefix) {
		FD = InpFD(Ii); 
		Z = TryRdFldFrml(FD, FTyp);
	}
	else Z = FindIiandFldFrml(&FD, FTyp);
	if (Z == nullptr) Error(8); 
	TestSetSumIi();
	*res = FrmlContxt(Z, FD, FD->RecPtr);
}

void SetIi()
{
	if (!WasIiPrefix) { 
		if (WhatToRd == 'i') Ii = Oi; 
		else Ii = 1; 
	}
}

void TestNotSum()
{
	if (FrmlSumEl != nullptr) OldError(41);
}

void Err()
{
	TestNotSum(); 
	SetIi();
	if (IDA[Ii]->ErrTxtFrml == nullptr)
	{
		IDA[Ii]->ErrTxtFrml = new FrmlElem4(_const, 0); // GetOp(_const, 256);
	}
}

void ChainSumElR()
{
	SumElem* S = nullptr; 
	if (FrstSumVar || (SumIi == 0)) SumIi = 1;
	if (FrstSumVar || (CBlk == nullptr)) S = IDA[SumIi]->Sum;
	else S = CBlk->Sum;

	// TODO: byla pridana kontrola S, tady to padalo ...
	if (S != nullptr) {
		FrmlSumEl->Chain = S->Chain;
		S->Chain = FrmlSumEl;
	}
	
	if (CBlkSave != nullptr) { CBlk = CBlkSave; CBlkSave = nullptr; }
	//Z = (FloatPtrListEl*)GetStore(sizeof(*Z));
	FloatPtrListEl* Z = new FloatPtrListEl();
	Z->RPtr = &FrmlSumEl->R;

	// TODO: byla pridana kontrola CZeroLst, tady to padalo ...
	if (CZeroLst != nullptr) {
		Z->Chain = CZeroLst->Chain;
		CZeroLst->Chain = Z;
	}
}

void ReadReport(RprtOpt* RO)
{
	FileD* FD = nullptr; KeyInD* KI = nullptr; BlkD* B = nullptr; WORD i = 0;
	InpD* ID = nullptr; LvDescr* L = nullptr; pstring s(2); 
	RprtFDListEl* FDL = nullptr;

	ResetCompilePars(); 
	RdLex();
	CBlkSave = nullptr; PgeSizeZ = nullptr; PgeLimitZ = nullptr;
	FDL = nullptr; 
	if ((RO != nullptr) && (RO->FDL.FD != nullptr)) FDL = &RO->FDL;
	ResetLVBD(); 
	if (IsKeyWord("VAR")) RdLocDcl(&LVBD, false, false, 'R');
label1:
	if (Lexem == '.') {
		RdLex(); 
		TestIdentif();
		if (IsKeyWord("PAGESIZE")) { 
			Accept(_assign); 
			PgeSizeZ = RdRealFrml(); 
		}
		else if (IsKeyWord("PAGELIMIT")) {
			Accept(_assign); 
			PgeLimitZ = RdRealFrml();
		}
		else Error(56);
		if (Lexem == ';') { RdLex(); goto label1; }
	}
	SelQuest = false; Ii = 0; 
	TestLex('#');
	do {
		Ii++; ReadChar(); 
		if (CurrChar == 'I') {
			ReadChar();
			if (isdigit(CurrChar)) {
				if (Ii != CurrChar - '0') Error(61); 
				ReadChar();
				if (CurrChar == '_') goto label2;
			}
			else if (Ii == 1) goto label2;
		}
		Error(89);
	label2:
		//ID = (InpD*)GetZStore(sizeof(*ID)); 
		ID = new InpD();
		IDA[Ii] = ID;
		RdLex(); 
		FD = RdFileName(); 
		CFile = FD;
		for (i = 1; i <= Ii - 1; i++) if (InpFD(i) == FD) OldError(26);
		if ((FDL != nullptr)) { CFile = FDL->FD; FD = CFile; }
		CViewKey = RdViewKey();
		if ((FDL != nullptr)) {
			if (FDL->ViewKey != nullptr) CViewKey = FDL->ViewKey;
		}
		if (Lexem == '!') { RdLex(); ID->AutoSort = true; }
		ID->Op = _const; ID->OpErr = _const; ID->OpWarn = _const; KI = nullptr;
		ID->ForwRecPtr = GetRecSpace(); 
		FD->RecPtr = GetRecSpace();
		if (Lexem == '(') {
			RdLex();
			if ((Ii == 1) && (Lexem == '?')) { SelQuest = true; RdLex(); }
			else ID->Bool = RdKeyInBool(KI, false, false, ID->SQLFilter);
			Accept(')');
		}
		if ((FDL != nullptr) && (FDL->LVRecPtr == nullptr) &&
			((FDL->Cond != nullptr) || (FDL->KeyIn != nullptr) || (Ii == 1) && RO->UserCondQuest)) {
			ID->Bool = RunEvalFrml(FDL->Cond);
			KI = FDL->KeyIn; 
			ID->SQLFilter = FDL->SQLFilter;
			if (Ii == 1) SelQuest = RO->UserCondQuest;
		}
		//New(ID->Scan, Init(FD, CViewKey, KI, true));
		ID->Scan = new XScan(FD, CViewKey, KI, true);
		if ((FDL != nullptr) && (FDL->LVRecPtr != nullptr)) ID->Scan->ResetLV(FDL->LVRecPtr);
		if (!(Lexem == ';' || Lexem == '#' || Lexem == 0x1A)) RdKFList(&ID->MFld, FD);
		if (Ii > 1)
			if (IDA[Ii - 1]->MFld == nullptr) {
				if (ID->MFld != nullptr) OldError(22);
			}
			else if (ID->MFld == nullptr) CopyPrevMFlds();
			else CheckMFlds(IDA[Ii - 1]->MFld, ID->MFld);
		RdAutoSortSK(ID); 
		TestLex('#');
		if (FDL != nullptr) FDL = FDL->Chain;
	} while (ForwChar == 'I');
	MaxIi = Ii; 
	FrstLvM = MakeOldMLvD();

	PageHd = nullptr; RprtHd = nullptr; PageFt = nullptr; PFZeroLst = nullptr;
label3:
	s[0] = 2; ReadChar(); 
	s[1] = toupper(CurrChar); 
	ReadChar(); 
	s[2] = toupper(CurrChar);
	ChainSumEl = nullptr; 
	RdFldNameFrml = RdFldNameFrmlR; 
	WhatToRd = 'O';
	if (s == "DE") {
		Rd_Oi(); 
		RdLex(); 
		WhatToRd = 'i';
		RdBlock(IDA[Oi]->FrstLvS->Ft); 
		goto label4;
	}
	if (s == "RH") {
		RdLex(); 
		RdBlock(RprtHd); 
		goto label4;
	}
	if (s == "PH") {
		RdLex(); 
		RdBlock(PageHd); 
		goto label4;
	}
	if (s == "DH") {
		Rd_Oi(); 
		RdLex(); 
		WhatToRd = 'i'; 
		RdBlock(IDA[Oi]->FrstLvS->Hd); 
		goto label4;
	}
	if (s == "CH") {
		Rd_Oi(); 
		L = RdKeyName(); 
		RdBlock(L->Hd); 
		goto label4;
	}
	ChainSumEl = ChainSumElR;
	if (s == "RF") {
		RdLex(); 
		LvToRd = LstLvM; 
		CZeroLst = LvToRd->ZeroLst;
		RdBlock(LvToRd->Ft); 
		goto label4;
	}
	if (s == "CF") {
		Rd_Oi(); 
		LvToRd = RdKeyName(); 
		CZeroLst = LvToRd->ZeroLst;
		RdBlock(LvToRd->Ft); 
		goto label4;
	}
	if (s == "PF") {
		RdLex(); 
		LvToRd = LstLvM; 
		CZeroLst = PFZeroLst;
		RdBlock(PageFt); 
		goto label4;
	}
	Error(57);
label4:
	if (Lexem != 0x1A) { TestLex('#'); goto label3; }

	for (i = 1; i < MaxIi; i++) {
		ID = IDA[i];
		if (ID->ErrTxtFrml != nullptr) RdChkDsFromPos(ID->Scan->FD, ID->Chk);
	}
}

void CopyPrevMFlds()
{
	KeyFldD* M = nullptr; KeyFldD* MNew = nullptr; FieldDescr* F = nullptr;
	pstring s = LexWord;
	M = IDA[Ii - 1]->MFld;
	while (M != nullptr) {
		LexWord = M->FldD->Name; F = FindFldName(InpFD(Ii));
		if (F == nullptr) OldError(8);
		if (!FldTypIdentity(M->FldD, F)) OldError(12);
		MNew = (KeyFldD*)GetStore(sizeof(*MNew));
		Move(M, MNew, sizeof(*MNew));
		MNew->FldD = F; ChainLast(IDA[Ii]->MFld, MNew); M = (KeyFldD*)M->Chain;
	}
	LexWord = s;
}

void CheckMFlds(KeyFldD* M1, KeyFldD* M2)
{
	while (M1 != nullptr) {
		if (M2 == nullptr) OldError(30);
		if (!FldTypIdentity(M1->FldD, M2->FldD) || (M1->Descend != M2->Descend)
			|| (M1->CompLex != M2->CompLex)) OldError(12);
		M1 = (KeyFldD*)M1->Chain; M2 = (KeyFldD*)M2->Chain;
	}
	if (M2 != nullptr) OldError(30);
}

LvDescr* MakeOldMLvD()
{
	LvDescr* L1 = nullptr; WORD n = 0;
	OldMFlds = nullptr; NewMFlds = nullptr;
	//LvDescr* L = (LvDescr*)GetZStore(sizeof(*L)); 
	LvDescr* L = new LvDescr();
	LstLvM = L;
	KeyFldD* M = IDA[1]->MFld;
	while (M != nullptr) {
		n = sizeof(void*) + M->FldD->NBytes + 1;
		//ConstListEl* C = (ConstListEl*)GetStore(n);
		ConstListEl* C = new ConstListEl();
		if (OldMFlds == nullptr) OldMFlds = C;
		else ChainLast(OldMFlds, C);
		//C = (ConstListEl*)GetStore(n);
		C = new ConstListEl();
		if (NewMFlds == nullptr) NewMFlds = C;
		else ChainLast(NewMFlds, C);
		//L1 = (LvDescr*)GetZStore(sizeof(*L1));
		L1 = new LvDescr();
		L->ChainBack = L1; 
		L1->Chain = L; 
		L = L1;
		L->Fld = M->FldD; 
		M = (KeyFldD*)M->Chain;
	}
	return L;
}

void RdAutoSortSK(InpD* ID)
{
	KeyFldD* M = nullptr; KeyFldD* SK = nullptr; LvDescr* L = nullptr;
	WORD n = 0; ConstListEl* C = nullptr; bool as = false;
	if (Lexem == ';') { RdLex(); RdKFList(&ID->SFld, CFile); }
	L = nullptr;
	as = ID->AutoSort;
	if (as) {
		SK = (KeyFldD*)(&ID->SK); 
		M = ID->MFld; 
		while (M != nullptr) {
			//SK->Chain = (KeyFldD*)GetStore(sizeof(*SK)); 
			SK->Chain = new KeyFldD();
			SK = (KeyFldD*)SK->Chain;
			//Move(M, SK, sizeof(*SK)); 
			*SK = *M;
			M = (KeyFldD*)M->Chain;
		}
	}
	M = ID->SFld;
	while (M != nullptr) {
		L = NewLvS(L, ID); 
		L->Fld = M->FldD;
		n = sizeof(void*) + M->FldD->NBytes + 1;
		//C = (ConstListEl*)GetStore(n);
		C = new ConstListEl();
		if (ID->OldSFlds == nullptr) ID->OldSFlds = C;
		else ChainLast(ID->OldSFlds, C);
		if (as) {
			//SK->Chain = (KeyFldD*)GetStore(sizeof(*SK));
			SK->Chain = new KeyFldD();
			SK = (KeyFldD*)SK->Chain;
			//Move(M, SK, sizeof(*SK));
			*SK = *M;
		}
		M = (KeyFldD*)M->Chain;
	}
	if (as && (ID->SK == nullptr)) OldError(60);
	ID->FrstLvS = NewLvS(L, ID);
}

LvDescr* NewLvS(LvDescr* L, InpD* ID)
{
	LvDescr* L1 = new LvDescr();
	//L1 = (LvDescr*)GetZStore(sizeof(*L1));
	L1->Chain = L;
	if (L == nullptr) ID->LstLvS = L1;
	else L->ChainBack = L1;
	return L1;
}

void RdBlock(BlkD* BB)
{
	integer LineLen = 0;
	integer NBytesStored = 0;
	BYTE* LnL = nullptr; 
	WORD* StrL = nullptr;
	integer I = 0, N = 0, L = 0, M = 0;
	bool RepeatedGrp = false;        /*RdBlock - body*/
	RFldD* RF = nullptr; 
	RFldD* RF1 = nullptr;
	BlkD* DummyB = nullptr;
	char UC = 0;
	LocVar* LV = nullptr;
	//CBlk = (BlkD*)GetZStore(sizeof(BlkD)); 
	CBlk = new BlkD();
	if (BB == nullptr) BB = CBlk;
	else ChainLast(BB, CBlk);
	RdCond();
	if (IsKeyWord("BEGIN")) { RdBeginEnd(CBlk->BeforeProc); goto label1; }
	if (Lexem == ';') goto label2;     /*read var decl.*/
label0:
	if (IsKeyWord("BEGIN")) { RdBeginEnd(CBlk->AfterProc); goto label2; }
	if (Lexem == '.') {
		RdLex();
		if (IsKeyWord("LINE"))
			if (Lexem == _le) { RdLex(); CBlk->LineBound = RdRealFrml(); }
			else {
				Accept(_assign); CBlk->AbsLine = true; CBlk->LineNo = RdRealFrml();
			}
		else if (IsKeyWord("PAGE")) {
			Accept(_assign); CBlk->SetPage = true; CBlk->PageNo = RdRealFrml();
		}
		else if (IsKeyWord("NOTATEND")) CBlk->NotAtEnd = true;
		else if (IsKeyWord("NOTSOLO")) CBlk->DHLevel = 1;
		else Error(54);
	}
	else {
		//RF = (RFldD*)GetStore(sizeof(*RF) - 1); 
		RF = new RFldD();
		SkipBlank(false);
		if (ForwChar == ':') {
			TestIdentif(); GetStore(LexWord.length());
			Move(&LexWord, &RF->Name, LexWord.length() + 1);
			if (FindInLvBlk(LstLvM, DummyB, RF1) || FindLocVar(&LVBD, &LV)) Error(26);
			RdLex(); Accept(_assign);
		}
		else RF->Name = "";
		RF->Frml = RdFrml(RF->FrmlTyp); 
		RF->BlankOrWrap = false;
		if (CBlk->RFD == nullptr) CBlk->RFD = RF;
		else ChainLast(CBlk->RFD, RF);
	}
label1:
	if (Lexem != ';') { Accept(','); goto label0; }
label2:
	RF = CBlk->RFD; RepeatedGrp = false; SkipBlank(true);
	switch (ForwChar) {
	case 0x1A: /* ^Z */ goto label4; break;
	case '\\': { CBlk->FF1 = true; ReadChar(); break; }
	}
label3:
	//LnL = (BYTE*)GetZStore(1);
	LnL = new BYTE();
	//StrL = (WORD*)GetZStore(2);
	StrL = new (WORD);
	LineLen = 0;
	NBytesStored = 0;
	if (CBlk->NTxtLines == 0) {
		CBlk->Txt = (char*)(LnL);
		while (ForwChar == ' ') RdCh(LineLen);
		CBlk->NBlksFrst = LineLen;
	}
	while (!(ForwChar == 0x0D || ForwChar == 0x1A))
		switch (ForwChar) {
		case '_':
		case '@':
		case 0x10: {
			UC = ForwChar; StoreCh(0xFF, NBytesStored);
			if (RF == nullptr)
			{
				RF = CBlk->RFD; if (RF == nullptr) Error(30); RepeatedGrp = true;
			}
			if (UC == 0x10) {
				if (RF->FrmlTyp != 'S') Error(12);
				if (!RepeatedGrp) RF->Typ = 'P';
				else if (RF->Typ != 'P') Error(73); ReadChar();
				goto label5;
			}
			L = NUnderscores(UC, LineLen);
			switch (ForwChar) {
			case ',': {
				RdCh(LineLen); if (RF->FrmlTyp != 'R') Error(12);
				M = NUnderscores(UC, LineLen);
				L = L + M + 1;
				StoreCh(char(L), NBytesStored);
				StoreCh(char(M), NBytesStored);
				TestSetRFTyp('F', RepeatedGrp, RF);
				break;
			}
			case '.': {
				RdCh(LineLen); 
				if (RF->FrmlTyp != 'R') Error(12);
				M = NUnderscores(UC, LineLen);
				if (ForwChar == '.')
				{
					RdCh(LineLen); N = NUnderscores(UC, LineLen);
					if ((L != 2) || (M != 2) || !(N == 2 || N == 4)) Error(71);
					TestSetRFTyp('D', RepeatedGrp, RF);
					if (N == 4) RF->BlankOrWrap = true;
				}
				else {
					L = L + M + 1;
					StoreCh(char(L), NBytesStored);
					StoreCh(char(M), NBytesStored);
					TestSetRFTyp('R', RepeatedGrp, RF);
				}
				break;
			}
			case ':': {
				RdCh(LineLen);
				if (RF->FrmlTyp != 'R') Error(12);
				M = NUnderscores(UC, LineLen);
				if (M != 2) Error(69); M = 3;
				if (ForwChar == ':') {
					RdCh(LineLen); N = NUnderscores(UC, LineLen);
					if (N != 2) Error(69); M = 6;
					if (ForwChar == '.') {
						RdCh(LineLen); N = NUnderscores(UC, LineLen);
						if (N != 2) Error(69); M = 9;
					}
				}
				StoreCh(char(L), NBytesStored);
				StoreCh(char(M), NBytesStored);
				TestSetRFTyp('T', RepeatedGrp, RF);
				break;
			}
			default: {
				StoreCh(char(L), NBytesStored); 
				TestSetRFTyp(RF->FrmlTyp, RepeatedGrp, RF);
				M = 0; 
				if (RF->Typ == 'S') M = LineLen - L + 1; /*current column*/
				StoreCh(char(M), NBytesStored);
				break;
			}
			}
			TestSetBlankOrWrap(RepeatedGrp, UC, RF);
		label5:
			RF = (RFldD*)RF->Chain;
			break;
		}
		case '\\': { CBlk->FF2 = true; EndString(LineLen, NBytesStored, LnL, StrL); ReadChar(); goto label4; break; }
		case '{':
		case '#':
		{
			ReleaseStore(LnL); goto label4;
			break;
		}
		default: {
			if (ForwChar == 0xff) StoreCh(' ', NBytesStored); else StoreCh(ForwChar, NBytesStored);
			RdCh(LineLen);
			break;
		}
		}
	EndString(LineLen, NBytesStored, LnL, StrL);
	SkipBlank(true); if (ForwChar != 0x1A) goto label3;
label4:
	if ((CBlk->NTxtLines == 1) && (CBlk->Txt[1] == 0) && CBlk->FF1) {
		CBlk->NTxtLines = 0; CBlk->FF1 = false; CBlk->FF2 = true;
	}
	if (RF != nullptr) Error(30); RdLex();
}

void RdCh(integer& LineLen)
{
	if (!IsPrintCtrl(ForwChar)) LineLen++; ReadChar();
}

void StoreCh(BYTE C, integer& NBytesStored)
{
	char* P = new char();
	//P = (char*)GetStore(1); 
	*P = C;
	NBytesStored++;
}

integer NUnderscores(char C, integer& LineLen)
{
	integer N;
	N = 0;
	while (ForwChar == C) { N++; RdCh(LineLen); }
	return N;
}

void EndString(integer LineLen, integer NBytesStored, BYTE* LnL, WORD* StrL)
{
	*StrL = NBytesStored;
	CBlk->NTxtLines++; if (LineLen == 0) *LnL = 255;
	else *LnL = LineLen;
}

void TestSetRFTyp(char Typ, bool RepeatedGrp, RFldD* RF)
{
	if (RepeatedGrp) { if (RF->Typ != Typ) Error(73); }
	else RF->Typ = Typ;
	if (ForwChar == '.' || ForwChar == ',' || ForwChar == ':') Error(95);
}

void TestSetBlankOrWrap(bool RepeatedGrp, char UC, RFldD* RF)
{
	if (RF->Typ == 'R' || RF->Typ == 'F' || RF->Typ == 'S')
		if (!RepeatedGrp) RF->BlankOrWrap = (UC = '@');
		else { if (RF->BlankOrWrap && (UC == '_')) Error(73); }
	else if (UC == '@') Error(80);
}

void RdBeginEnd(AssignD* ARoot)
{
label1:
	if (IsKeyWord("END")) return;
	RdAssignBlk(ARoot);
	if (Lexem == ';') { RdLex(); goto label1; }
	AcceptKeyWord("END");
}

AssignD* RdAssign2()
{
	AssignD* A = nullptr; LocVar* LV = nullptr; char FTyp = 0;
	FileD* FD = nullptr; FieldDescr* F = nullptr;
	AssignD* result = nullptr;
	A = new AssignD(); // (AssignD*)GetZStore(sizeof(*A));
	result = A;
	if (IsKeyWord("IF")) {
		A->Kind = _ifthenelseM; 
		A->Bool = RdBool();
		AcceptKeyWord("THEN"); 
		RdAssignBlk(A->Instr);
		if (IsKeyWord("ELSE")) RdAssignBlk(A->ElseInstr);
	}
	else if (ForwChar == '.') {
		A->Kind = _parfile; 
		FD = RdFileName(); 
		if (!FD->IsParFile) OldError(9);
		Accept('.'); 
		A->FD = FD; 
		F = RdFldName(FD); 
		A->PFldD = F;
		if ((F->Flg & f_Stored) == 0) OldError(14);
		RdAssignFrml(F->FrmlTyp, A->Add, &A->Frml);
	}
	else if (FindLocVar(&LVBD, &LV)) {
		RdLex(); 
		A->Kind = _locvar; 
		A->LV = LV;
		RdAssignFrml(LV->FTyp, A->Add, &A->Frml);
	}
	else Error(147);
	return result;
}

void RdAssignBlk(AssignD* ARoot)
{
	AssignD* A = nullptr;
	if (IsKeyWord("BEGIN")) RdBeginEnd(ARoot);
	else { 
		A = RdAssign2(); 
		if (ARoot == nullptr) { ARoot = A; A->Chain = nullptr; }
		else ChainLast(ARoot, A); 
	}
}

void RdCond()
{
	if (Lexem == '(') {
		RdLex(); 
		CBlk->Bool = RdBool(); 
		Accept(')');
	}
}

LvDescr* RdKeyName()
{
	LvDescr* L= nullptr; 
	FieldDescr* F = nullptr;
	ReadChar(); 
	Lexem = CurrChar; 
	Accept('_');
	if (WhatToRd == 'O') L = FrstLvM;
	else L = IDA[Oi]->FrstLvS;
	F = RdFldName(InpFD(Oi));
	while (L != nullptr) {
		if (L->Fld == F) { return L; }
		L = L->Chain;
	}
	OldError(46);
	return nullptr;
}

void Rd_Oi()
{
	Oi = 1; 
	if (isdigit(ForwChar)) {
		ReadChar(); 
		Oi = CurrChar - '0';
		if ((Oi == 0) || (Oi > MaxIi)) Error(62);
		WhatToRd = 'i';
	}
}

