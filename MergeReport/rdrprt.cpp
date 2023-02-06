#include "rdrprt.h"
#include "rdmerg.h"
#include "shared.h"
#include "../cppfand/compile.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/obase.h"
#include "../cppfand/rdfildcl.h"


BlkD* CBlk;
std::vector<FrmlElemSum*>* CZeroLst = nullptr;
LvDescr* LvToRd;           /*all used while translating frml*/
BlkD* CBlkSave;

FileD* InpFD(WORD I)
{
	return IDA[I]->Scan->FD;
}

bool FindInLvBlk(LvDescr* L, BlkD** B, RFldD** RF)
{
	bool first = true;
	auto result = false;
label1:
	while (L != nullptr) {
		BlkD* B1 = L->Ft;
		while (B1 != nullptr) {
			//RFldD* RF1 = B1->RFD;
			//while (RF1 != nullptr) {
			for (auto rf : B1->ReportFields) {
				std::string sLexWord = LexWord;
				if ((Lexem == _identifier) && EquUpCase(rf->Name, sLexWord)) {
					RdLex();
					result = true;
					*RF = rf;
					*B = B1;
					return result;
				}
				//RF1 = (RFldD*)RF1->pChain;
			}
			B1 = (BlkD*)B1->pChain;
		}
		L = L->ChainBack;
	}
	if (first) {
		first = false;
		L = IDA[1]->FrstLvS;/*DE*/
		goto label1;
	}
	return result;
}

FrmlElem* RdFldNameFrmlR(char& FTyp)
{
	LinkD* LD = nullptr; FileD* FD = nullptr; RFldD* RF = nullptr; LocVar* LV = nullptr;
	WORD n = 0;
	FrmlElem* Z = nullptr;
	FrmlElem* result = nullptr;
	bool WasIiPrefix = RdIiPrefix();
	if ((FrmlSumEl != nullptr) && FrstSumVar && (CBlk != nullptr)) {
		SumIi = 0; CBlkSave = CBlk; CBlk = nullptr;
	}
	if (IsForwPoint()) {
		RdDirFilVar(FTyp, &result, WasIiPrefix);
		return result;
	}
	if (!WasIiPrefix && FindLocVar(&LVBD, &LV)) {
		RdLex();
		TestNotSum();
		result = new FrmlElem18(_getlocvar, LV);
		FTyp = LV->FTyp;
		return result;
	}
	if (IsKeyWord("COUNT")) {
		TestNotSum();
		SetIi_Report(WasIiPrefix);
		result = new FrmlElemInp(_count, IDA[Ii]);
		FTyp = 'R';
		return result;
	}
	if (IsKeyWord("GROUP")) {
		TestNotSum();
		if (WasIiPrefix) OldError(41);
		result = new FrmlElemMerge(_mergegroup, &MergOpGroup);
		FTyp = 'R';
		return result;
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
		Err('r', WasIiPrefix);
		result = (FrmlElem*)(&IDA[Ii]->OpErr);
		FTyp = 'B';
		return result;
	}
	if (IsKeyWord("WARNING")) {
		Err('r', WasIiPrefix);
		result = (FrmlElem*)(&IDA[Ii]->OpWarn);
		FTyp = 'B';
		return result;
	}
	if (IsKeyWord("ERRORTEXT")) {
		Err('r', WasIiPrefix);
		result = IDA[Ii]->ErrTxtFrml;
		FTyp = 'S';
		return result;
	}
	if (FrmlSumEl != nullptr) {
		//if (InpArrLen == 1099) {
		//	printf("");
		//}
		if (FrstSumVar) {
			if (FindInLvBlk(LvToRd->ChainBack, &CBlk, &RF)) {
				FTyp = RF->FrmlTyp;
				result = RF->Frml;
				return result;
			}
			FindInRec(FTyp, &result, WasIiPrefix);
			return result;
		}
		else if (CBlk == nullptr) { FindInRec(FTyp, &result, WasIiPrefix); return result; }
		else if (OwnInBlock(FTyp, &result)) return result;
		else OldError(8);
	}
	if (OwnInBlock(FTyp, &result)) return result;
	FindInRec(FTyp, &result, WasIiPrefix);
	return result;
}

FrmlElem* FindIiandFldFrml(FileD** FD, char& FTyp)
{
	integer i = 0;
	FrmlElem* z = nullptr;;
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
			goto label1;
		}
		if ((WhatToRd == 'i') && (i == Oi)) goto label1;
	}
label1:
	return z;
}

void RdDirFilVar(char& FTyp, FrmlElem** res, bool wasIiPrefix)
{
	LinkD* LD = nullptr;
	FileD* FD = nullptr;
	integer I = 0;
	FrmlElem* Z = nullptr;
	if (wasIiPrefix) {
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
	// RFldD* RF;
	auto result = false;
	// RF = CBlk->RFD;
	// while (RF != nullptr) {
	for (auto rf : CBlk->ReportFields) {
		std::string sLexWord = LexWord;
		if (EquUpCase(rf->Name, sLexWord)) {
			RdLex();
			FTyp = rf->FrmlTyp;
			*res = rf->Frml;
			result = true;
			return result;
		}
		// RF = (RFldD*)RF->pChain;
	}
	return result;
}

void FindInRec(char& FTyp, FrmlElem** res, bool wasIiPrefix)
{
	FileD* FD = nullptr;
	FieldDescr* F = nullptr;
	FrmlElem* Z = nullptr;
	if (wasIiPrefix) {
		FD = InpFD(Ii);
		Z = TryRdFldFrml(FD, FTyp);
	}
	else Z = FindIiandFldFrml(&FD, FTyp);
	if (Z == nullptr) Error(8);
	TestSetSumIi();
	*res = FrmlContxt(Z, FD, FD->RecPtr);
}

void ChainSumElR()
{
	CZeroLst->push_back(FrmlSumEl->at(0));

	if (FrstSumVar || (SumIi == 0)) SumIi = 1;

	if (FrstSumVar || (CBlk == nullptr)) {
		if (IDA[SumIi]->Sum == nullptr) {
			IDA[SumIi]->Sum = FrmlSumEl;
		}
		else {
			for (size_t i = 0; i < FrmlSumEl->size(); i++) {
				auto* el = FrmlSumEl->at(i);
				IDA[SumIi]->Sum->push_back(el);
			}
			FrmlSumEl->clear();
		}
	}
	else if (CBlk != nullptr && CBlk->Sum != nullptr) {
		//std::vector<FrmlElemSum*>* S = CBlk->Sum;
		for (size_t i = 0; i < CBlk->Sum->size(); i++) {
			auto* el = CBlk->Sum->at(i);
			FrmlSumEl->push_back(el);
			CBlk->Sum->clear();
		}
	}

	if (CBlkSave != nullptr) {
		CBlk = CBlkSave;
		CBlkSave = nullptr;
	}
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

void ReadReport(RprtOpt* RO)
{
	FileD* FD = nullptr; KeyInD* KI = nullptr; BlkD* B = nullptr; WORD i = 0;
	InpD* ID = nullptr; LvDescr* L = nullptr;
	std::string s;

	ResetCompilePars();
	RdLex();
	CBlkSave = nullptr;
	PgeSizeZ = nullptr; PgeLimitZ = nullptr;
	RprtFDListEl* FDL = nullptr;
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
		if (Lexem == ';') {
			RdLex();
			goto label1;
		}
	}
	SelQuest = false;
	Ii = 0;
	TestLex('#');
	do {
		Ii++;
		ReadChar();
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
		ID = new InpD();
		IDA[Ii] = ID;
		RdLex();
		FD = RdFileName();
		CFile = FD;
		for (i = 1; i <= Ii - 1; i++) {
			if (InpFD(i) == FD) OldError(26);
		}
		if ((FDL != nullptr)) {
			CFile = FDL->FD;
			FD = CFile;
		}
		CViewKey = RdViewKey();
		if ((FDL != nullptr)) {
			if (FDL->ViewKey != nullptr) CViewKey = FDL->ViewKey;
		}
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
			if ((Ii == 1) && (Lexem == '?')) {
				SelQuest = true;
				RdLex();
			}
			else {
				ID->Bool = RdKeyInBool(&KI, false, false, ID->SQLFilter);
			}
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
		if (!(Lexem == ';' || Lexem == '#' || Lexem == 0x1A)) {
			RdKFList(&ID->MFld, FD);
		}
		if (Ii > 1) {
			if (IDA[Ii - 1]->MFld == nullptr) {
				if (ID->MFld != nullptr) OldError(22);
			}
			else if (ID->MFld == nullptr) {
				CopyPrevMFlds();
			}
			else {
				CheckMFlds(IDA[Ii - 1]->MFld, ID->MFld);
			}
		}
		RdAutoSortSK(ID);
		TestLex('#');
		if (FDL != nullptr) FDL = FDL->Chain;
	} while (ForwChar == 'I');
	MaxIi = Ii;
	FrstLvM = MakeOldMLvD();

	PageHd = nullptr; RprtHd = nullptr; PageFt = nullptr;
	PFZeroLst.clear();
	while (true) {
		ReadChar();
		s = toupper(CurrChar);
		ReadChar();
		s += toupper(CurrChar);
		ChainSumEl = nullptr;
		RdFldNameFrml = RdFldNameFrmlR;
		WhatToRd = 'O';
		if (s == "DE") { // detailni vystup
			Rd_Oi();
			RdLex();
			WhatToRd = 'i';
			RdBlock(&IDA[Oi]->FrstLvS->Ft);
		}
		else if (s == "RH") { // hlavicka sestavy
			RdLex();
			RdBlock(&RprtHd);
		}
		else if (s == "PH") { // hlavicka stranky
			RdLex();
			RdBlock(&PageHd);
		}
		else if (s == "DH") { // hlavicka detailu
			Rd_Oi();
			RdLex();
			WhatToRd = 'i';
			RdBlock(&IDA[Oi]->FrstLvS->Hd);
		}
		else if (s == "CH") { // hlavicka skupiny
			Rd_Oi();
			L = RdKeyName();
			RdBlock(&L->Hd);
		}
		else if (s == "RF") { // paticka sestavy
			ChainSumEl = ChainSumElR;
			RdLex();
			LvToRd = LstLvM;
			CZeroLst = &LvToRd->ZeroLst;
			RdBlock(&LvToRd->Ft);
		}
		else if (s == "CF") { // paticka skupiny
			ChainSumEl = ChainSumElR;
			Rd_Oi();
			LvToRd = RdKeyName();
			CZeroLst = &LvToRd->ZeroLst;
			RdBlock(&LvToRd->Ft);
		}
		else if (s == "PF") { // paticka stranky
			ChainSumEl = ChainSumElR;
			RdLex();
			LvToRd = LstLvM;
			CZeroLst = &PFZeroLst;
			RdBlock(&PageFt);
		}
		else {
			Error(57);
		}

		if (Lexem != 0x1A) {
			TestLex('#');
			continue;
		}
		break;
	}

	for (i = 1; i <= MaxIi; i++) {
		ID = IDA[i];
		if (ID->ErrTxtFrml != nullptr) {
			RdChkDsFromPos(ID->Scan->FD, ID->Chk);
		}
	}
}

LvDescr* MakeOldMLvD()
{
	LvDescr* L1 = nullptr; WORD n = 0;
	OldMFlds.clear();
	NewMFlds.clear();
	//LvDescr* L = (LvDescr*)GetZStore(sizeof(*L)); 
	LvDescr* L = new LvDescr();
	LstLvM = L;
	KeyFldD* M = IDA[1]->MFld;
	while (M != nullptr) {
		OldMFlds.push_back(ConstListEl());
		NewMFlds.push_back(ConstListEl());
		L1 = new LvDescr();
		L->ChainBack = L1;
		L1->Chain = L;
		L = L1;
		L->Fld = M->FldD;
		M = (KeyFldD*)M->pChain;
	}
	return L;
}

void RdAutoSortSK(InpD* ID)
{
	KeyFldD* M = nullptr; KeyFldD* SK = nullptr; LvDescr* L = nullptr;
	WORD n = 0; bool as = false;
	if (Lexem == ';') { RdLex(); RdKFList(&ID->SFld, CFile); }
	L = nullptr;
	as = ID->AutoSort;
	if (as) {
		SK = (KeyFldD*)(&ID->SK);
		M = ID->MFld;
		while (M != nullptr) {
			//SK->pChain = (KeyFldD*)GetStore(sizeof(*SK)); 
			SK->pChain = new KeyFldD();
			SK = (KeyFldD*)SK->pChain;
			//Move(M, SK, sizeof(*SK)); 
			*SK = *M;
			M = (KeyFldD*)M->pChain;
		}
	}
	M = ID->SFld;
	while (M != nullptr) {
		L = NewLvS(L, ID);
		L->Fld = M->FldD;
		n = sizeof(void*) + M->FldD->NBytes + 1;
		ID->OldSFlds.push_back(ConstListEl());
		if (as) {
			SK->pChain = new KeyFldD();
			SK = (KeyFldD*)SK->pChain;
			*SK = *M;
		}
		M = (KeyFldD*)M->pChain;
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

void RdBeginEnd(std::vector<AssignD*>* ARoot);

void RdAssignBlk(std::vector<AssignD*>* ARoot)
{
	if (IsKeyWord("BEGIN")) RdBeginEnd(ARoot);
	else {
		auto A = RdAssign2();
		for (auto assign : A) {
			ARoot->push_back(assign);
		}
	}
}

void RdBeginEnd(std::vector<AssignD*>* ARoot)
{
label1:
	if (IsKeyWord("END")) return;
	RdAssignBlk(ARoot);
	if (Lexem == ';') {
		RdLex();
		goto label1;
	}
	AcceptKeyWord("END");
}

void RdBlock(BlkD** BB)
{
	// metoda pouzivala metodu StoreCh, ta byla nahrazena promennou storedCh, ktera slouzi k ukladani nactenych dat

	std::string storedCh; // pridano pro zprovozneni StoreCh(char, &integer)

	BYTE rep[256]{ 0 };
	size_t offset = 0;

	integer LineLen = 0;
	//WORD* StrL = nullptr;
	integer I = 0, N = 0, L = 0, M = 0;
	bool RepeatedGrp = false;        /*RdBlock - body*/
	RFldD* RF = nullptr;
	RFldD* RF1 = nullptr;
	BlkD* DummyB = nullptr;
	char UC = 0;
	LocVar* LV = nullptr;
	//CBlk = (BlkD*)GetZStore(sizeof(BlkD)); 
	CBlk = new BlkD();
	if (*BB == nullptr) *BB = CBlk;
	else ChainLast(*BB, CBlk);
	size_t reportFieldsVectorIndex = 0;
	RdCond();
	if (IsKeyWord("BEGIN")) {
		RdBeginEnd(&CBlk->BeforeProc);
		goto label1;
	}
	if (Lexem == ';') goto label2;     /*read var decl.*/
label0:
	if (IsKeyWord("BEGIN")) {
		RdBeginEnd(&CBlk->AfterProc);
		goto label2;
	}
	if (Lexem == '.') {
		RdLex();
		if (IsKeyWord("LINE"))
			if (Lexem == _le) {
				RdLex();
				CBlk->LineBound = RdRealFrml();
			}
			else {
				Accept(_assign);
				CBlk->AbsLine = true;
				CBlk->LineNo = RdRealFrml();
			}
		else if (IsKeyWord("PAGE")) {
			Accept(_assign);
			CBlk->SetPage = true;
			CBlk->PageNo = RdRealFrml();
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
			TestIdentif();
			RF->Name = LexWord;
			if (FindInLvBlk(LstLvM, &DummyB, &RF1) || FindLocVar(&LVBD, &LV)) Error(26);
			RdLex();
			Accept(_assign);
		}
		else RF->Name = "";
		RF->Frml = RdFrml(RF->FrmlTyp);
		RF->BlankOrWrap = false;
		//if (CBlk->RFD == nullptr) CBlk->RFD = RF;
		//else ChainLast(CBlk->RFD, RF);
		CBlk->ReportFields.push_back(RF);
	}
label1:
	if (Lexem != ';') {
		Accept(',');
		goto label0;
	}
label2:
	//RF = CBlk->RFD;
	reportFieldsVectorIndex = 0;
	if (CBlk->ReportFields.empty()) {
		RF = nullptr;
	}
	else {
		reportFieldsVectorIndex = 0;
		RF = CBlk->ReportFields.at(reportFieldsVectorIndex);
	}

	RepeatedGrp = false;
	SkipBlank(true);
	switch (ForwChar) {
	case 0x1A: /* ^Z */ goto label4; break;
	case '\\': {
		CBlk->FF1 = true;
		ReadChar();
		break;
	}
	}
label3:
	//LnL = (BYTE*)GetZStore(1);
	//LnL = new BYTE;
	//StrL = (WORD*)GetZStore(2);
	//StrL = new WORD;
	LineLen = 0;
	storedCh = "";
	if (CBlk->NTxtLines == 0) {
		CBlk->lineLength = 0;
		while (ForwChar == ' ') RdCh(LineLen);
		CBlk->NBlksFrst = LineLen;
	}
	while (!(ForwChar == 0x0D || ForwChar == 0x1A))
		switch (ForwChar) {
		case '_':
		case '@':
		case 0x10: {
			UC = ForwChar;
			storedCh += static_cast<char>(0xFF);
			rep[offset++] = 0xFF;
			if (RF == nullptr) {
				if (CBlk->ReportFields.empty()) {
					RF = nullptr;
				}
				else {
					reportFieldsVectorIndex = 0;
					RF = CBlk->ReportFields.at(reportFieldsVectorIndex);
				}
				
				if (RF == nullptr) Error(30);
				RepeatedGrp = true;
			}
			if (UC == 0x10) {
				if (RF->FrmlTyp != 'S') Error(12);
				if (!RepeatedGrp) RF->Typ = 'P';
				else if (RF->Typ != 'P') Error(73);
				ReadChar();
				goto label5;
			}
			L = NUnderscores(UC, LineLen);
			switch (ForwChar) {
			case ',': {
				RdCh(LineLen);
				if (RF->FrmlTyp != 'R') Error(12);
				M = NUnderscores(UC, LineLen);
				L = L + M + 1;
				storedCh += static_cast<char>(L);
				rep[offset++] = L;
				storedCh += static_cast<char>(M);
				rep[offset++] = M;
				TestSetRFTyp('F', RepeatedGrp, RF);
				break;
			}
			case '.': {
				RdCh(LineLen);
				if (RF->FrmlTyp != 'R') Error(12);
				M = NUnderscores(UC, LineLen);
				if (ForwChar == '.')
				{
					RdCh(LineLen);
					N = NUnderscores(UC, LineLen);
					if ((L != 2) || (M != 2) || !(N == 2 || N == 4)) Error(71);
					TestSetRFTyp('D', RepeatedGrp, RF);
					if (N == 4) RF->BlankOrWrap = true;
				}
				else {
					L = L + M + 1;
					storedCh += static_cast<char>(L);
					rep[offset++] = L;
					storedCh += static_cast<char>(M);
					rep[offset++] = M;
					TestSetRFTyp('R', RepeatedGrp, RF);
				}
				break;
			}
			case ':': {
				RdCh(LineLen);
				if (RF->FrmlTyp != 'R') Error(12);
				M = NUnderscores(UC, LineLen);
				if (M != 2) Error(69);
				M = 3;
				if (ForwChar == ':') {
					RdCh(LineLen);
					N = NUnderscores(UC, LineLen);
					if (N != 2) Error(69);
					M = 6;
					if (ForwChar == '.') {
						RdCh(LineLen);
						N = NUnderscores(UC, LineLen);
						if (N != 2) Error(69);
						M = 9;
					}
				}
				storedCh += static_cast<char>(L);
				rep[offset++] = L;
				storedCh += static_cast<char>(M);
				rep[offset++] = M;
				TestSetRFTyp('T', RepeatedGrp, RF);
				break;
			}
			default: {
				storedCh += (char)L;
				rep[offset++] = L;
				TestSetRFTyp(RF->FrmlTyp, RepeatedGrp, RF);
				M = 0;
				if (RF->Typ == 'S') M = LineLen - L + 1; /*current column*/
				storedCh += static_cast<char>(M);
				rep[offset++] = M;
				break;
			}
			}
			TestSetBlankOrWrap(RepeatedGrp, UC, RF);
		label5:
			//RF = (RFldD*)RF->pChain;
			if (reportFieldsVectorIndex < CBlk->ReportFields.size() - 1) {
				RF = CBlk->ReportFields.at(++reportFieldsVectorIndex);
			}
			else {
				RF = nullptr;
			}

			break;
		}
		case '\\': {
			CBlk->FF2 = true;
			EndString(CBlk, rep, LineLen, storedCh.length());
			offset = 0;
			ReadChar();
			goto label4;
			break;
		}
		case '{':
		case '#': {
			//ReleaseStore(LnL);
			goto label4;
			break;
		}
		default: {
			if (ForwChar == 0xff) {
				storedCh += ' ';
				rep[offset++] = ' ';
			}
			else {
				storedCh += ForwChar;
				rep[offset++] = ForwChar;
			}
			RdCh(LineLen);
			break;
		}
		}
	EndString(CBlk, rep, LineLen, storedCh.length());
	offset = 0;
	SkipBlank(true);
	if (ForwChar != 0x1A) goto label3;
label4:
	if ((CBlk->NTxtLines == 1) && (CBlk->lineLength == 0) && CBlk->FF1) {
		CBlk->NTxtLines = 0;
		CBlk->FF1 = false;
		CBlk->FF2 = true;
	}
	if (RF != nullptr) Error(30);
	RdLex();

	// TODO: co bude se storedCh? ulozi se nekam?
	// vypada to, ze v EndString se se vstupem pracuje, string je mozna zbytecny
}

void RdCh(integer& LineLen)
{
	if (!IsPrintCtrl(ForwChar)) LineLen++;
	ReadChar();
}

integer NUnderscores(char C, integer& LineLen)
{
	integer N = 0;
	while (ForwChar == static_cast<BYTE>(C)) {
		N++;
		RdCh(LineLen);
	}
	return N;
}

void EndString(BlkD* block, BYTE* buffer, size_t LineLen, size_t NBytesStored)
{
	// vlozime buffer do vectoru stringu
	block->lines.push_back(std::string((char*)buffer, NBytesStored));

	//*StrL = NBytesStored;
	CBlk->NTxtLines++;
	if (LineLen == 0) {
		block->lineLength = 255;
	}
	else {
		block->lineLength = LineLen;
	}
}

void TestSetRFTyp(char Typ, bool RepeatedGrp, RFldD* RF)
{
	if (RepeatedGrp) { if (RF->Typ != Typ) Error(73); }
	else RF->Typ = Typ;
	if (ForwChar == '.' || ForwChar == ',' || ForwChar == ':') Error(95);
}

void TestSetBlankOrWrap(bool RepeatedGrp, char UC, RFldD* RF)
{
	if (RF->Typ == 'R' || RF->Typ == 'F' || RF->Typ == 'S') {
		if (!RepeatedGrp) {
			RF->BlankOrWrap = (UC == '@');
		}
		else {
			if (RF->BlankOrWrap && (UC == '_')) Error(73);
		}
	}
	else if (UC == '@') Error(80);
}

std::vector<AssignD*> RdAssign2()
{
	LocVar* LV = nullptr; char FTyp = 0;
	FileD* FD = nullptr; FieldDescr* F = nullptr;

	std::vector<AssignD*> result;
	AssignD* A = new AssignD();

	if (IsKeyWord("IF")) {
		A->Kind = _ifthenelseM;
		A->Bool = RdBool();
		AcceptKeyWord("THEN");
		RdAssignBlk(&A->Instr);
		if (IsKeyWord("ELSE")) {
			RdAssignBlk(&A->ElseInstr);
		}
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

	result.push_back(A);
	return result;
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
	LvDescr* L = nullptr;
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
