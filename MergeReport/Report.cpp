#include "Report.h"

#include "ConstListEl.h"
#include "InpD.h"
#include "MergeReportBase.h"
#include "RFldD.h"
#include "../Common/compare.h"
#include "../Common/textfunc.h"
#include "../Core/Compiler.h"
#include "../Core/FieldDescr.h"
#include "../Core/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/LogicControl.h"
#include "../Core/KeyFldD.h"
#include "../Core/oaccess.h"
#include "../Core/obase.h"
#include "../Core/obaseww.h"
#include "../Core/RunMessage.h"
#include "../Core/wwmix.h"
#include "../Core/rdfildcl.h"
#include "../Core/DateTime.h"


Report::Report()
{
}

Report::~Report()
{
}

void Report::Read(RprtOpt* RO)
{
	std::vector<KeyInD*> KI;
	BlkD* B = nullptr;
	InpD* ID = nullptr;
	LvDescr* L = nullptr;
	std::string s;

	ResetCompilePars();
	g_compiler->RdLex();
	CBlkSave = nullptr;
	PgeSizeZ = nullptr;
	PgeLimitZ = nullptr;

	bool b_RprtOpt_defined = true;

	if (RO == nullptr) {
		// if RprtOpt is not defined, create a new one
		// this will be deleted at the end of this function
		RO = new RprtOpt();
		b_RprtOpt_defined = false;
	}

	std::vector<RprtFDListEl*>::iterator FDL = RO->FDL.begin();

	//if (FDL != RO->FDL.end() && (*FDL)->FD != nullptr) {
	//	++FDL;
	//}
	ResetLVBD();
	if (g_compiler->IsKeyWord("VAR")) {
		g_compiler->RdLocDcl(&LVBD, false, false, 'R');
	}

	// process .dot commands
	while (true) {
		if (Lexem == '.') {
			g_compiler->RdLex();
			g_compiler->TestIdentif();
			if (g_compiler->IsKeyWord("PAGESIZE")) {
				g_compiler->Accept(_assign);
				PgeSizeZ = g_compiler->RdRealFrml(this);
			}
			else if (g_compiler->IsKeyWord("PAGELIMIT")) {
				g_compiler->Accept(_assign);
				PgeLimitZ = g_compiler->RdRealFrml(this);
			}
			else {
				g_compiler->Error(56);
			}
			if (Lexem == ';') {
				g_compiler->RdLex();
				continue;
			}
		}
		break;
	}

	SelQuest = false;
	Ii = 0;
	g_compiler->TestLex('#');

	do {
		Ii++;
		g_compiler->ReadChar();
		bool err = true;
		if (CurrChar == 'I') {
			g_compiler->ReadChar();
			if (isdigit(CurrChar)) {
				if (Ii != CurrChar - '0') {
					g_compiler->Error(61);
				}
				g_compiler->ReadChar();
				if (CurrChar == '_') {
					err = false;
				}
			}
			else if (Ii == 1) {
				err = false;
			}
		}
		if (err) {
			g_compiler->Error(89);
		}
		else {
			ID = new InpD();
			IDA[Ii] = ID;
			g_compiler->RdLex();
			FileD* FD = g_compiler->RdFileName();

			std::unique_ptr<Compiler> report_compiler = std::make_unique<Compiler>(FD);
			report_compiler->rdFldNameType = FieldNameType::P;

			for (int16_t i = 1; i <= Ii - 1; i++) {
				if (InpFD(i) == FD) {
					report_compiler->OldError(26);
				}
			}
			if (FDL != RO->FDL.end()) {
				FD = (*FDL)->FD;
				report_compiler->processing_F = (*FDL)->FD;
			}

			CViewKey = report_compiler->RdViewKey(FD);

			if (FDL != RO->FDL.end()) {
				if ((*FDL)->ViewKey != nullptr) {
					CViewKey = (*FDL)->ViewKey;
				}
			}

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
				if ((Ii == 1) && (Lexem == '?')) {
					SelQuest = true;
					report_compiler->RdLex();
				}
				else {
					g_compiler->processing_F = report_compiler->processing_F;
					ID->Bool = report_compiler->RdKeyInBool(KI, false, false, ID->SQLFilter, this);
				}
				report_compiler->Accept(')');
			}

			if ((FDL != RO->FDL.end())
				&& ((*FDL)->LVRecPtr == nullptr)
				&&
				((*FDL)->Cond != nullptr || (!(*FDL)->KeyIn.empty()) || (Ii == 1) && RO->UserCondQuest))
			{
				ID->Bool = RunEvalFrml(FD, (*FDL)->Cond, FD->FF->RecPtr);
				KI = (*FDL)->KeyIn;
				ID->SQLFilter = (*FDL)->SQLFilter;
				if (Ii == 1) {
					SelQuest = RO->UserCondQuest;
				}
			}

			ID->Scan = new XScan(FD, CViewKey, KI, true);

			if ((FDL != RO->FDL.end()) && ((*FDL)->LVRecPtr != nullptr)) {
				ID->Scan->ResetLV((*FDL)->LVRecPtr);
			}

			if (!(Lexem == ';' || Lexem == '#' || Lexem == 0x1A)) {
				report_compiler->RdKFList(ID->MFld, FD);
			}

			if (Ii > 1) {
				if (IDA[Ii - 1]->MFld.empty()) {
					if (!ID->MFld.empty()) report_compiler->OldError(22);
				}
				else if (ID->MFld.empty()) {
					CopyPrevMFlds();
				}
				else {
					CheckMFlds(IDA[Ii - 1]->MFld, ID->MFld);
				}
			}

			RdAutoSortSK(ID, report_compiler);
			report_compiler->TestLex('#');

			if (FDL != RO->FDL.end()) {
				++FDL;
			}
			// report_compiler destroys itself here
		}
	} while (ForwChar == 'I');

	MaxIi = Ii;
	FrstLvM = MakeOldMLvD();

	PageHd.clear();
	RprtHd.clear();
	PageFt.clear();
	PFZeroLst.clear();

	while (true) {
		g_compiler->ReadChar();
		s = toupper(CurrChar);
		g_compiler->ReadChar();
		s += toupper(CurrChar);
		ChainSum = false;
		//ptrRdFldNameFrml = nullptr; // RdFldNameFrml;
		g_compiler->rdFldNameType = FieldNameType::none;
		WhatToRd = 'O';
		if (s == "DE") { // detailni vystup
			Rd_Oi();
			g_compiler->RdLex();
			WhatToRd = 'i';
			RdBlock(IDA[Oi]->FrstLvS->Ft);
		}
		else if (s == "RH") { // hlavicka sestavy
			g_compiler->RdLex();
			RdBlock(RprtHd);
		}
		else if (s == "PH") { // hlavicka stranky
			g_compiler->RdLex();
			RdBlock(PageHd);
		}
		else if (s == "DH") { // hlavicka detailu
			Rd_Oi();
			g_compiler->RdLex();
			WhatToRd = 'i';
			RdBlock(IDA[Oi]->FrstLvS->Hd);
		}
		else if (s == "CH") { // hlavicka skupiny
			Rd_Oi();
			L = RdKeyName();
			RdBlock(L->Hd);
		}
		else if (s == "RF") { // paticka sestavy
			ChainSum = true;
			g_compiler->RdLex();
			LvToRd = LstLvM;
			CZeroLst = &LvToRd->ZeroLst;
			RdBlock(LvToRd->Ft);
		}
		else if (s == "CF") { // paticka skupiny
			ChainSum = true;
			Rd_Oi();
			LvToRd = RdKeyName();
			CZeroLst = &LvToRd->ZeroLst;
			RdBlock(LvToRd->Ft);
		}
		else if (s == "PF") { // paticka stranky
			ChainSum = true;
			g_compiler->RdLex();
			LvToRd = LstLvM;
			CZeroLst = &PFZeroLst;
			RdBlock(PageFt);
		}
		else {
			g_compiler->Error(57);
		}

		if (Lexem != 0x1A) {
			g_compiler->TestLex('#');
			continue;
		}
		break;
	}

	for (int16_t i = 1; i <= MaxIi; i++) {
		ID = IDA[i];
		if (ID->ErrTxtFrml != nullptr) {
			RdChkDsFromPos(ID->Scan->FD, ID->Chk);
		}
	}

	if (!b_RprtOpt_defined) {
		// delete the RprtOpt object if it was created in this function
		delete RO;
	}
}

void Report::Run(RprtOpt* RO)
{
	std::string ReportString;
	wwmix ww;
	LvDescr* L = nullptr;
	std::string* s = nullptr;
	WORD i = 0;
	bool frst = false, isLPT1 = false;
	WORD Times = 0;
	bool ex = false, b = false;
	BlkD* BD = nullptr;
	std::vector<BlkD*> RFb;
	LockMode md;
	if (SelQuest) {
		CFile = IDA[1]->Scan->FD;
		if (!ww.PromptFilter("", &IDA[1]->Bool, s)) {
			PrintView = false;
			return;
		}
	}
	if (PgeLimitZ != nullptr) {
		PgeLimit = RunInt(CFile, PgeLimitZ, CRecPtr);
	}
	else {
		PgeLimit = spec.AutoRprtLimit;
	}

	if (PgeSizeZ != nullptr) {
		PgeSize = RunInt(CFile, PgeSizeZ, CRecPtr);
	}
	else {
		PgeSize = spec.AutoRprtLimit + spec.CpLines;
	}

	if (PgeSize < 2) PgeSize = 2;
	if ((PgeLimit > PgeSize) || (PgeLimit == 0)) PgeLimit = PgeSize - 1;
	if (!RewriteRprt(RO, PgeSize, Times, isLPT1)) return;  // pouze zajisti otevreni souboru
	//MarkStore(Store2Ptr);
	ex = true;
	Compiler::ProcStack.push_front(&LVBD); //PushProcStk();
	//NewExit(Ovr(), er);
	//goto label3;
	OpenInp();
	MergOpGroup.Group = 1.0; frst = true; NLinesOutp = 0; PrintDH = 2;
label0:
	RunMsgOn('R', NRecsAll);
	RecCount = 0;
	for (i = 1; i <= MaxIi; i++) {
		if (frst) frst = false;
		else IDA[i]->Scan->SeekRec(0);
		ReadInpFile(IDA[i]);
	}
	RprtPage = 1; RprtLine = 1; SetPage = false; FirstLines = true; WasDot = false;
	WasFF2 = false; NoFF = false; LineLenLst = 0; FrstBlk = true;
	ResetY();
	L = LstLvM;
	RFb = LstLvM->Ft;
	ZeroSumFlds(L);
	GetMinKey();
	ZeroCount();
	MoveFrstRecs();
	if (!RprtHd.empty()) {
		if (RprtHd[0]->FF1) {
			FormFeed(ReportString);
		}
		RprtPage = 1;
		PrintBlkChn(RprtHd, ReportString, false, false);
		TruncLine(ReportString);
		if (WasFF2) FormFeed(ReportString);
		if (SetPage) {
			SetPage = false;
			RprtPage = PageNo;
		}
		else RprtPage = 1;
	}
	if (NEof == MaxIi) goto label2;
label1:
	if (WasFF2) PrintPageHd(ReportString);
	Headings(L, nullptr, ReportString);
	MergeProc(ReportString);
	MoveMFlds(NewMFlds, OldMFlds);
	GetMinKey();
	if (NEof == MaxIi) {
		if (FrstLvM != LstLvM) Footings(FrstLvM, LstLvM->ChainBack, ReportString);
	label2:
		WasFF2 = false;
		TruncLine(ReportString);
		PrintBlkChn(RFb, ReportString, false, true);
		b = WasFF2;
		if (!PageFt.empty() && !PageFt[0]->NotAtEnd) {
			PrintPageFt(ReportString);
		}
		TruncLine(ReportString);
		RunMsgOff();
		if (Times > 1 /*only LPT1*/) {
			Times--;
			printf("%s%c", Rprt.c_str(), 0x0C);
			goto label0;
		}
		if (b) FormFeed(ReportString);
		ex = false;
	label3:
		//RestoreExit(er);
		if (PrintView && (NLinesOutp == 0) && (LineLenLst == 0)) {
			ReadMessage(159);
			printf("%s\n", ReportString.c_str());
			printf("%s%s", ReportString.c_str(), MsgLine.c_str());
		}
		Rprt.Close(ReportString.c_str());
		// if (isLPT1) ClosePrinter(0);
		CloseInp();
		LVBD = *Compiler::ProcStack.front(); Compiler::ProcStack.pop_front(); //PopProcStk();
		if (ex) {
			RunMsgOff();
			if (!WasLPTCancel) GoExit();
		}
		return;
	}
	L = GetDifLevel();
	Footings(FrstLvM, L, ReportString);
	if (WasFF2) PrintPageFt(ReportString);
	ZeroSumFlds(L);
	ZeroCount();
	MoveFrstRecs();
	MergOpGroup.Group = MergOpGroup.Group + 1.0;
	goto label1;
}

FrmlElem* Report::RdFldNameFrml(char& FTyp)
{
	LinkD* LD = nullptr; FileD* FD = nullptr; RFldD* RF = nullptr; LocVar* LV = nullptr;
	WORD n = 0;
	FrmlElem* Z = nullptr;
	FrmlElem* result = nullptr;
	bool WasIiPrefix = RdIiPrefix();
	if ((FrmlSumEl != nullptr) && FrstSumVar && (CBlk != nullptr)) {
		SumIi = 0; CBlkSave = CBlk; CBlk = nullptr;
	}
	if (g_compiler->IsForwPoint()) {
		RdDirFilVar(FTyp, &result, WasIiPrefix);
		return result;
	}
	if (!WasIiPrefix && g_compiler->FindLocVar(&LVBD, &LV)) {
		g_compiler->RdLex();
		TestNotSum();
		result = new FrmlElemLocVar(_getlocvar, LV);
		FTyp = LV->f_typ;
		return result;
	}
	if (g_compiler->IsKeyWord("COUNT")) {
		TestNotSum();
		SetIi_Report(WasIiPrefix);
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
	if (g_compiler->IsKeyWord("LINE")) { n = 0; goto label1; }
	if (g_compiler->IsKeyWord("PAGE")) { n = 1; goto label1; }
	if (g_compiler->IsKeyWord("PAGELIMIT")) {
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
	if (g_compiler->IsKeyWord("ERROR")) {
		Err('r', WasIiPrefix);
		result = (FrmlElem*)(&IDA[Ii]->OpErr);
		FTyp = 'B';
		return result;
	}
	if (g_compiler->IsKeyWord("WARNING")) {
		Err('r', WasIiPrefix);
		result = (FrmlElem*)(&IDA[Ii]->OpWarn);
		FTyp = 'B';
		return result;
	}
	if (g_compiler->IsKeyWord("ERRORTEXT")) {
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
		else g_compiler->OldError(8);
	}
	if (OwnInBlock(FTyp, &result)) return result;
	FindInRec(FTyp, &result, WasIiPrefix);
	return result;
}

void Report::ChainSumEl()
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

bool Report::FindInLvBlk(LvDescr* L, BlkD** B, RFldD** RF)
{
	bool first = true;
	bool result = false;
label1:
	while (L != nullptr) {
		for (BlkD* B1 : L->Ft) {
			for (RFldD* rf : B1->ReportFields) {
				std::string sLexWord = LexWord;
				if ((Lexem == _identifier) && EquUpCase(rf->Name, sLexWord)) {
					g_compiler->RdLex();
					result = true;
					*RF = rf;
					*B = B1;
					return result;
				}
			}
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

FrmlElem* Report::FindIiandFldFrml(FileD** FD, char& FTyp)
{
	short i = 0;
	FrmlElem* z = nullptr;
	if (WhatToRd == 'i') {       /* search first in Ii*/
		*FD = InpFD(Oi);
		z = g_compiler->TryRdFldFrml(*FD, FTyp, this);
		if (z != nullptr) {
			Ii = Oi;
			goto label1;
		}
	}
	for (i = 1; i <= MaxIi; i++) {       /* search in  I1 .. Imax resp. Oi*/
		*FD = InpFD(i);
		z = g_compiler->TryRdFldFrml(*FD, FTyp, this);
		if (z != nullptr) {
			Ii = i;
			goto label1;
		}
		if ((WhatToRd == 'i') && (i == Oi)) goto label1;
	}
label1:
	return z;
}

void Report::RdDirFilVar(char& FTyp, FrmlElem** res, bool wasIiPrefix)
{
	LinkD* LD = nullptr;
	FileD* FD = nullptr;
	FrmlElem* Z = nullptr;
	FileD* processed_file = nullptr;

	if (wasIiPrefix) {
		processed_file = InpFD(Ii);
		g_compiler->processing_F = processed_file;
		if (!g_compiler->IsRoleName(true, processed_file, &FD, &LD)) {
			g_compiler->Error(9);
		}
	}
	else {
		if (!Join && (WhatToRd == 'i')) {
			Ii = Oi;
			processed_file = InpFD(Ii);
			g_compiler->processing_F = processed_file;
			if (g_compiler->IsRoleName(true, processed_file, &FD, &LD)) {
				goto label2;
			}
		}
		for (int16_t i = 1; i <= MaxIi; i++) {
			processed_file = InpFD(i);
			g_compiler->processing_F = processed_file;
			if (g_compiler->IsRoleName(true, processed_file, &FD, &LD)) {
				Ii = i;
				goto label2;
			}
			if ((WhatToRd == 'i') && (i == Oi)) goto label1;
		}
	label1:
		g_compiler->Error(9);
	}
label2:
	g_compiler->RdLex(); // '.'
	Z = g_compiler->RdFAccess(FD, LD, FTyp);
	if (LD == nullptr) {
		Ii = 0;
	}
	else {
		Z = g_compiler->FrmlContxt(Z, processed_file, processed_file->FF->RecPtr);
		TestSetSumIi();
		if ((FrmlSumEl != nullptr) && !FrstSumVar && (CBlk != nullptr)) {
			g_compiler->OldError(59);
		}
	}
	*res = Z;
}

bool Report::OwnInBlock(char& FTyp, FrmlElem** res)
{
	// RFldD* RF;
	auto result = false;
	// RF = CBlk->RFD;
	// while (RF != nullptr) {
	for (auto rf : CBlk->ReportFields) {
		std::string sLexWord = LexWord;
		if (EquUpCase(rf->Name, sLexWord)) {
			g_compiler->RdLex();
			FTyp = rf->FrmlTyp;
			*res = rf->Frml;
			result = true;
			return result;
		}
		// RF = (RFldD*)RF->pChain;
	}
	return result;
}

void Report::FindInRec(char& FTyp, FrmlElem** res, bool wasIiPrefix)
{
	FileD* FD = nullptr;
	FieldDescr* F = nullptr;
	FrmlElem* Z = nullptr;
	if (wasIiPrefix) {
		FD = InpFD(Ii);
		Z = g_compiler->TryRdFldFrml(FD, FTyp, this);
	}
	else Z = FindIiandFldFrml(&FD, FTyp);
	if (Z == nullptr) g_compiler->Error(8);
	TestSetSumIi();
	*res = g_compiler->FrmlContxt(Z, FD, FD->FF->RecPtr);
}

void Report::Rd_Oi()
{
	Oi = 1;
	if (isdigit(ForwChar)) {
		g_compiler->ReadChar();
		Oi = CurrChar - '0';
		if ((Oi == 0) || (Oi > MaxIi)) g_compiler->Error(62);
		WhatToRd = 'i';
	}
}

LvDescr* Report::MakeOldMLvD()
{
	LvDescr* L1 = nullptr; WORD n = 0;
	OldMFlds.clear();
	NewMFlds.clear();
	//LvDescr* L = (LvDescr*)GetZStore(sizeof(*L)); 
	LvDescr* L = new LvDescr();
	LstLvM = L;
	//KeyFldD* M = IDA[1]->MFld;
	//while (M != nullptr) {
	for (KeyFldD* M : IDA[1]->MFld) {
		OldMFlds.push_back(ConstListEl());
		NewMFlds.push_back(ConstListEl());
		L1 = new LvDescr();
		L->ChainBack = L1;
		L1->Chain = L;
		L = L1;
		L->Fld = M->FldD;
		//M = (KeyFldD*)M->pChain;
	}
	return L;
}

void Report::RdAutoSortSK(InpD* ID, std::unique_ptr<Compiler>& compiler)
{
	KeyFldD* SK = nullptr;
	//WORD n = 0;
	if (Lexem == ';') {
		compiler->RdLex();
		compiler->RdKFList(ID->SFld, compiler->processing_F);
	}
	LvDescr* L = nullptr;
	bool auto_sort = ID->AutoSort;

	if (auto_sort) {
		SK = (KeyFldD*)(&ID->SK);
		//M = ID->MFld;
		//while (M != nullptr) {
		for (KeyFldD* M : ID->MFld) {
			//SK->pChain = new KeyFldD();
			//SK = SK->pChain;
			//*SK = *M;
			//M = M->pChain;

			// TODO: this is probably wrong:
			ID->SK.push_back(M);
		}
	}

	//M = ID->SFld;
	//while (M != nullptr) {
	for (KeyFldD* M : ID->SFld) {
		L = NewLvS(L, ID);
		L->Fld = M->FldD;
		//WORD n = sizeof(void*) + M->FldD->NBytes + 1;
		ID->OldSFlds.push_back(ConstListEl());
		if (auto_sort) {
			//SK->pChain = new KeyFldD();
			//SK = SK->pChain;
			//*SK = *M;

			// TODO: this is probably wrong:
			ID->SK.push_back(M);
		}
		//M = M->pChain;
	}

	if (auto_sort && (ID->SK.empty())) {
		compiler->OldError(60);
	}

	ID->FrstLvS = NewLvS(L, ID);
}

LvDescr* Report::NewLvS(LvDescr* L, InpD* ID)
{
	LvDescr* L1 = new LvDescr();
	L1->Chain = L;
	if (L == nullptr) {
		ID->LstLvS = L1;
	}
	else {
		L->ChainBack = L1;
	}
	return L1;
}

void Report::RdAssignBlk(std::vector<AssignD*>* ARoot)
{
	if (g_compiler->IsKeyWord("BEGIN")) RdBeginEnd(ARoot);
	else {
		std::vector<AssignD*> A = RdAssign2();
		for (auto assign : A) {
			ARoot->push_back(assign);
		}
	}
}

void Report::RdBeginEnd(std::vector<AssignD*>* ARoot)
{
label1:
	if (g_compiler->IsKeyWord("END")) return;
	RdAssignBlk(ARoot);
	if (Lexem == ';') {
		g_compiler->RdLex();
		goto label1;
	}
	g_compiler->AcceptKeyWord("END");
}

void Report::RdBlock(std::vector<BlkD*>& BB)
{
	// metoda pouzivala metodu StoreCh, ta byla nahrazena promennou storedCh, ktera slouzi k ukladani nactenych dat

	std::string storedCh; // pridano pro zprovozneni StoreCh(char, &short)

	BYTE rep[256]{ 0 };
	size_t offset = 0;

	short LineLen = 0;
	short I = 0, N = 0, L = 0, M = 0;
	bool RepeatedGrp = false;        /*RdBlock - body*/
	RFldD* RF = nullptr;
	RFldD* RF1 = nullptr;
	BlkD* DummyB = nullptr;
	char UC = 0;
	LocVar* LV = nullptr;

	CBlk = new BlkD();
	//if (*BB == nullptr) *BB = CBlk;
	//else ChainLast(*BB, CBlk);
	BB.push_back(CBlk);
	size_t reportFieldsVectorIndex = 0;
	RdCond();
	if (g_compiler->IsKeyWord("BEGIN")) {
		RdBeginEnd(&CBlk->BeforeProc);
		goto label1;
	}
	if (Lexem == ';') {
		goto label2; // read var decl.
	}
label0:
	if (g_compiler->IsKeyWord("BEGIN")) {
		RdBeginEnd(&CBlk->AfterProc);
		goto label2;
	}
	if (Lexem == '.') {
		g_compiler->RdLex();
		if (g_compiler->IsKeyWord("LINE"))
			if (Lexem == _le) {
				g_compiler->RdLex();
				CBlk->LineBound = g_compiler->RdRealFrml(this);
			}
			else {
				g_compiler->Accept(_assign);
				CBlk->AbsLine = true;
				CBlk->LineNo = g_compiler->RdRealFrml(this);
			}
		else if (g_compiler->IsKeyWord("PAGE")) {
			g_compiler->Accept(_assign);
			CBlk->SetPage = true;
			CBlk->PageNo = g_compiler->RdRealFrml(this);
		}
		else if (g_compiler->IsKeyWord("NOTATEND")) {
			CBlk->NotAtEnd = true;
		}
		else if (g_compiler->IsKeyWord("NOTSOLO")) {
			CBlk->DHLevel = 1;
		}
		else {
			g_compiler->Error(54);
		}
	}
	else {
		RF = new RFldD();
		g_compiler->SkipBlank(false);
		if (ForwChar == ':') {
			g_compiler->TestIdentif();
			RF->Name = LexWord;
			if (FindInLvBlk(LstLvM, &DummyB, &RF1) || g_compiler->FindLocVar(&LVBD, &LV)) {
				g_compiler->Error(26);
			}
			g_compiler->RdLex();
			g_compiler->Accept(_assign);
		}
		else {
			RF->Name = "";
		}
		RF->Frml = g_compiler->RdFrml(RF->FrmlTyp, this);
		RF->BlankOrWrap = false;
		CBlk->ReportFields.push_back(RF);
	}
label1:
	if (Lexem != ';') {
		g_compiler->Accept(',');
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
	g_compiler->SkipBlank(true);
	switch (ForwChar) {
	case 0x1A: {
		// ^Z
		goto label4;
		break;
	}
	case '\\': {
		CBlk->FF1 = true;
		g_compiler->ReadChar();
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
		while (ForwChar == ' ') {
			RdCh(LineLen);
		}
		CBlk->NBlksFrst = LineLen;
	}
	while (!(ForwChar == 0x0D || ForwChar == 0x1A))
		switch (ForwChar) {
		case '_':
		case '@':
		case 0x10: {
			UC = (char)ForwChar;
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

				if (RF == nullptr) g_compiler->Error(30);
				RepeatedGrp = true;
			}
			if (UC == 0x10) {
				if (RF->FrmlTyp != 'S') g_compiler->Error(12);
				if (!RepeatedGrp) RF->Typ = 'P';
				else if (RF->Typ != 'P') g_compiler->Error(73);
				g_compiler->ReadChar();
				goto label5;
			}
			L = NUnderscores(UC, LineLen);
			switch (ForwChar) {
			case ',': {
				RdCh(LineLen);
				if (RF->FrmlTyp != 'R') g_compiler->Error(12);
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
				if (RF->FrmlTyp != 'R') g_compiler->Error(12);
				M = NUnderscores(UC, LineLen);
				if (ForwChar == '.')
				{
					RdCh(LineLen);
					N = NUnderscores(UC, LineLen);
					if ((L != 2) || (M != 2) || !(N == 2 || N == 4)) g_compiler->Error(71);
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
				if (RF->FrmlTyp != 'R') g_compiler->Error(12);
				M = NUnderscores(UC, LineLen);
				if (M != 2) g_compiler->Error(69);
				M = 3;
				if (ForwChar == ':') {
					RdCh(LineLen);
					N = NUnderscores(UC, LineLen);
					if (N != 2) g_compiler->Error(69);
					M = 6;
					if (ForwChar == '.') {
						RdCh(LineLen);
						N = NUnderscores(UC, LineLen);
						if (N != 2) g_compiler->Error(69);
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
			g_compiler->ReadChar();
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
	g_compiler->SkipBlank(true);
	if (ForwChar != 0x1A) goto label3;
label4:
	if ((CBlk->NTxtLines == 1) && (CBlk->lineLength == 0) && CBlk->FF1) {
		CBlk->NTxtLines = 0;
		CBlk->FF1 = false;
		CBlk->FF2 = true;
	}
	if (RF != nullptr) g_compiler->Error(30);
	g_compiler->RdLex();

	// TODO: co bude se storedCh? ulozi se nekam?
	// vypada to, ze v EndString se se vstupem pracuje, string je mozna zbytecny
}

void Report::RdCh(short& LineLen)
{
	if (!IsPrintCtrl(ForwChar)) LineLen++;
	g_compiler->ReadChar();
}

short Report::NUnderscores(char C, short& LineLen)
{
	short N = 0;
	while (ForwChar == static_cast<BYTE>(C)) {
		N++;
		RdCh(LineLen);
	}
	return N;
}

void Report::EndString(BlkD* block, BYTE* buffer, size_t LineLen, size_t NBytesStored)
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

void Report::TestSetRFTyp(char Typ, bool RepeatedGrp, RFldD* RF)
{
	if (RepeatedGrp) {
		if (RF->Typ != Typ) {
			g_compiler->Error(73);
		}
	}
	else {
		RF->Typ = Typ;
	}
	if (ForwChar == '.' || ForwChar == ',' || ForwChar == ':') {
		g_compiler->Error(95);
	}
}

void Report::TestSetBlankOrWrap(bool RepeatedGrp, char UC, RFldD* RF)
{
	if (RF->Typ == 'R' || RF->Typ == 'F' || RF->Typ == 'S') {
		if (!RepeatedGrp) {
			RF->BlankOrWrap = (UC == '@');
		}
		else {
			if (RF->BlankOrWrap && (UC == '_')) {
				g_compiler->Error(73);
			}
		}
	}
	else if (UC == '@') {
		g_compiler->Error(80);
	}
}

std::vector<AssignD*> Report::RdAssign2()
{
	LocVar* LV = nullptr; char FTyp = 0;
	FileD* FD = nullptr; FieldDescr* F = nullptr;

	std::vector<AssignD*> result;
	AssignD* A = new AssignD();

	if (g_compiler->IsKeyWord("IF")) {
		A->Kind = MInstrCode::_ifThenElse;
		A->Bool = g_compiler->RdBool(this);
		g_compiler->AcceptKeyWord("THEN");
		RdAssignBlk(&A->Instr);
		if (g_compiler->IsKeyWord("ELSE")) {
			RdAssignBlk(&A->ElseInstr);
		}
	}
	else if (ForwChar == '.') {
		A->Kind = MInstrCode::_parfile;
		FD = g_compiler->RdFileName();
		if (!FD->IsParFile) g_compiler->OldError(9);
		g_compiler->Accept('.');
		A->FD = FD;
		F = g_compiler->RdFldName(FD);
		A->PFldD = F;
		if ((F->Flg & f_Stored) == 0) g_compiler->OldError(14);
		g_compiler->RdAssignFrml(F->frml_type, A->Add, &A->Frml, this);
	}
	else if (g_compiler->FindLocVar(&LVBD, &LV)) {
		g_compiler->RdLex();
		A->Kind = MInstrCode::_locvar;
		A->LV = LV;
		g_compiler->RdAssignFrml(LV->f_typ, A->Add, &A->Frml, this);
	}
	else g_compiler->Error(147);

	result.push_back(A);
	return result;
}

void Report::RdCond()
{
	if (Lexem == '(') {
		g_compiler->RdLex();
		CBlk->Bool = g_compiler->RdBool(this);
		g_compiler->Accept(')');
	}
}

LvDescr* Report::RdKeyName()
{
	LvDescr* L = nullptr;
	FieldDescr* F = nullptr;
	g_compiler->ReadChar();
	Lexem = CurrChar;
	g_compiler->Accept('_');
	if (WhatToRd == 'O') {
		L = FrstLvM;
	}
	else {
		L = IDA[Oi]->FrstLvS;
	}
	F = g_compiler->RdFldName(InpFD(Oi));
	while (L != nullptr) {
		if (L->Fld == F) {
			return L;
		}
		L = L->Chain;
	}
	g_compiler->OldError(46);
	return nullptr;
}

void Report::IncPage()
{
	if (SetPage) {
		SetPage = false;
		RprtPage = PageNo;
	}
	else RprtPage++;
}

void Report::NewLine(std::string& text)
{
	//printf("%s\n", Rprt.c_str());
	text += '\n';
	if (WasDot) WasDot = false;
	else {
		RprtLine++;
		NLinesOutp++;
		FirstLines = false;
	}
	LineLenLst = 0;
	if (RprtLine > PgeSize) {
		RprtLine -= PgeSize;
		IncPage();
	}
}

void Report::FinishTuple(std::string& text)
{
	while (Y.Blk != nullptr) {
		Print1NTupel(text, true);
	}
}

void Report::TruncLine(std::string& text)
{
	FinishTuple(text);
	if (LineLenLst > 0) NewLine(text);
}

void Report::ResetY()
{
	Y.Blk = nullptr;
	Y.ChkPg = false;
	Y.I = 0;
	Y.Ln = 0;
	Y.P = nullptr;
	Y.Sz = 0;
	Y.TD.clear();
	Y.TLn = 0;
}

void Report::FormFeed(std::string& text)
{
	short I = 0;
	if (NoFF) NoFF = false;
	else {
		text += (char)0x0C; // ^L 012 0x0C Form feed
		RprtLine = 1;
		IncPage();
	}
	LineLenLst = 0;
	WasFF2 = false;
}

void Report::Print1NTupel(std::string& text, bool Skip)
{
	WORD L;
	double R = 0.0;
	std::string mask;
	LongStr* S = nullptr;
	if (Y.Ln == 0) return;
	RFldD* RF = nullptr;
	auto reportFieldsIt = Y.Blk->ReportFields.begin();
label1:
	WasOutput = true;
	while (Y.I < Y.Sz) {
		char buffer[256]{ '\0' };
		BYTE C = (BYTE)Y.P[Y.I];
		if (C == 0xFF) {
			//if (RF == nullptr) RF = Y.Blk->RFD;
			//else RF = (RFldD*)RF->pChain;
			//if (RF == nullptr) return;
			if (RF == nullptr) {
				RF = *reportFieldsIt; // 1st time
			}
			else {
				++reportFieldsIt;
				if (reportFieldsIt == Y.Blk->ReportFields.end()) return;
				RF = *reportFieldsIt;
			}

			L = (BYTE)Y.P[Y.I + 1];
			WORD M = (BYTE)Y.P[Y.I + 2];
			if (RF->FrmlTyp == 'R') {
				if (!Skip) R = RunReal(CFile, RF->Frml, CRecPtr);
				switch (RF->Typ) {
				case 'R':
				case 'F': {
					if (Skip) {
						snprintf(buffer, sizeof(buffer), "%*c", L, ' ');
						//printf("%s%*c", Rprt.c_str(), L, ' ');
						text += buffer;
					}
					else {
						if (RF->Typ == 'F') R = R / Power10[M];
						if (RF->BlankOrWrap && (R == 0)) {
							if (M == 0) {
								snprintf(buffer, sizeof(buffer), "%*c", L, ' ');
								//printf("%s%*c", Rprt.c_str(), L, ' ');
							}
							else {
								snprintf(buffer, sizeof(buffer), "%*c.%*c", L - M - 1, ' ', M, ' ');
								//printf("%s%*c.%*c", Rprt.c_str(), L - M - 1, ' ', M, ' ');
							}
							text += buffer;
						}
						else {
							snprintf(buffer, sizeof(buffer), "%*.*f", L, M, R);
							text += buffer;
							//printf("%s%*.*f", Rprt.c_str(), L, M, rdb);
						}
					}
					Y.I += 2;
					break;
				}
				case 'D': {
					if (RF->BlankOrWrap) mask = "DD.MM.YYYY";
					else mask = "DD.MM.YY";
					goto label2;
					break;
				}
				case 'T': {
					mask = copy("hhhhhh", 1, L) + copy(":ss mm.tt", 1, M);
					Y.I += 2;
				label2:
					if (Skip) {
						snprintf(buffer, sizeof(buffer), "%*c", mask.length(), ' ');
						//printf("%s%*c", Rprt.c_str(), Mask.length(), ' ');
						text += buffer;
					}
					else {
						snprintf(buffer, sizeof(buffer), "%s", StrDate(R, mask).c_str());
						//printf("%s%s", Rprt.c_str(), StrDate(rdb, Mask).c_str());
						text += buffer;
					}
					break;
				}
				}
			}
			else {
				if (RF->Typ == 'P') {
					std::string s = RunString(CFile, RF->Frml, CRecPtr);
					//printf("%s%c", Rprt.c_str(), 0x10);
					text += 0x10;
					for (size_t i = 0; i < s.length(); i++) {
						//printf("%s%c", Rprt.c_str(), *(char*)(S->A[i]));
						text += s[i];
					}
					goto label3;
				}
				Y.I += 2;
				if (Skip) {
					snprintf(buffer, sizeof(buffer), "%*c", L, ' ');
					//printf("%s%*c", Rprt.c_str(), L, ' ');
					text += buffer;
				}
				else
					switch (RF->FrmlTyp) {
					case 'S': {
						std::string S = RunString(CFile, RF->Frml, CRecPtr);
						S = TrailChar(S, ' ');
						text += NewTxtCol(S, M, L, RF->BlankOrWrap);
						break;
					}
					case 'B':
						if (RunBool(CFile, RF->Frml, CRecPtr)) {
							//printf("%s%c", Rprt.c_str(), AbbrYes);
							text += AbbrYes;
						}
						else {
							//printf("%s%c", Rprt.c_str(), AbbrNo);
							text += AbbrNo;
						}
					}
			}
		}
		else {
			if ((C == '.') && (Y.I == 0) && FirstLines) WasDot = true;
			//printf("%s%c", Rprt.c_str(), (char)C);
			text += C;
		}
	label3:
		Y.I++;
	}
	PendingTT(text);
	Y.Ln--;
	if (Y.Ln > 0) {
		//Y.P += Y.Sz;
		Y.P = Y.Blk->lines[Y.Blk->NTxtLines - Y.Ln].c_str();
		NewLine(text);
		L = Y.Blk->lines[Y.Blk->NTxtLines - Y.Ln].length();
		//*(Y.P)++;
		//Y.Sz = *(WORD*)(Y.P);
		Y.Sz = Y.Blk->lines[Y.Blk->NTxtLines - Y.Ln].length();
		//*Y.P += 2;
		Y.I = 0;
		CheckPgeLimit(text);
		LineLenLst = L;
		// jedna se o posledni radek a je prazdny? -> pridame prazdny radek
		if (Y.Blk->NTxtLines - Y.Ln + 1 == Y.Blk->lines.size()) {
			if (Y.Blk->lines[Y.Blk->NTxtLines - Y.Ln].empty()) NewLine(text);
		}
		goto label1;
	}
	Y.Blk = nullptr;
}

void Report::RunAProc(std::vector<AssignD*> vAssign)
{
	for (AssignD* A : vAssign) {
		switch (A->Kind) {
		case MInstrCode::_locvar: { LVAssignFrml(CFile, A->LV, A->Add, A->Frml, CRecPtr); break; }
		case MInstrCode::_parfile: { AsgnParFldFrml(A->FD, A->PFldD, A->Frml, A->Add); break; }
		case MInstrCode::_ifThenElse: {
			if (RunBool(CFile, A->Bool, CRecPtr)) {
				RunAProc(A->Instr);
			}
			else {
				RunAProc(A->ElseInstr);
			}
			break;
		}
		default:
			break;
		}
	}
}

void Report::PrintTxt(BlkD* B, std::string& text, bool ChkPg)
{
	if (B == nullptr) return;
	if (B->SetPage) {
		PageNo = RunInt(CFile, B->PageNo, CRecPtr);
		SetPage = true;
	}
	if (B != Y.Blk) {
		FinishTuple(text);
		if (B->AbsLine) {
			for (int i = RprtLine; i <= RunInt(CFile, B->LineNo, CRecPtr) - 1; i++) {
				NewLine(text);
			}
		}
		if (B->NTxtLines > 0) {
			if (B->NBlksFrst < LineLenLst) NewLine(text);
			if (B->NBlksFrst - LineLenLst > 0) {
				char buffer[256]{ '\0' };
				snprintf(buffer, sizeof(buffer), "%*c", B->NBlksFrst - LineLenLst, ' ');
				text += buffer;
			}
			//for (int i = 1; i <= B->NBlksFrst - LineLenLst; i++) {
			//	text += ' ';
			//}
		}
		ResetY();
		Y.Ln = B->NTxtLines;
		if (Y.Ln != 0) {
			Y.Blk = B;
			Y.P = B->lines[0].c_str();
			Y.ChkPg = ChkPg;
			LineLenLst = B->lineLength;
			Y.Sz = B->lines[0].length();
		}
	}
	RunAProc(B->BeforeProc);
	Print1NTupel(text, false);
	RunAProc(B->AfterProc);
}

bool Report::OutOfLineBound(BlkD* B)
{
	return ((B->LineBound != nullptr) && (RprtLine > RunReal(CFile, B->LineBound, CRecPtr))
		|| B->AbsLine && (RunInt(CFile, B->LineNo, CRecPtr) < RprtLine));
}

void Report::PrintBlkChn(std::vector<BlkD*>& block, std::string& text, bool ChkPg, bool ChkLine)
{
	//while (B != nullptr) {
	for (BlkD* b : block) {
		if (RunBool(CFile, b->Bool, CRecPtr)) {
			if (ChkLine) {
				if (OutOfLineBound(b)) WasFF2 = true;
				if (b->FF1 || WasFF2) NewPage(text);
			}
			PrintTxt(b, text, ChkPg);
			WasFF2 = b->FF2;
		}
		//B = B->pChain;
	}
}

void Report::PrintPageFt(std::string& text)
{
	if (!FrstBlk) {
		bool b = WasFF2;
		TruncLine(text);
		WORD Ln = RprtLine;
		PrintBlkChn(PageFt, text, false, false);
		TruncLine(text);
		NoFF = RprtLine < Ln;
		PFZeroLst.clear();
		WasFF2 = b;
	}
}

void Report::PrintPageHd(std::string& text)
{
	bool b = FrstBlk;
	if (!b) FormFeed(text);
	PrintBlkChn(PageHd, text, false, false);
	if (!b) PrintDH = 2;
}

void Report::NewPage(std::string& text)
{
	PrintPageFt(text);
	PrintPageHd(text);
	TruncLine(text);
	FrstBlk = false;
}

void Report::WriteNBlks(std::string& text, short N)
{
	if (N > 0) {
		char buffer[256]{ '\0' };
		snprintf(buffer, sizeof(buffer), "%*c", N, ' ');
		//printf("%s%*c", Rprt.c_str(), N, ' ');
		text += buffer;
	}
}

/// <summary>
/// 
/// </summary>
/// <param name="S"></param>
/// <param name="Width"></param>
/// <param name="Wrap"></param>
/// <param name="paragraph">odstavec (v originale Absatz)</param>
/// <returns></returns>
std::string Report::GetLine(std::string& S, WORD Width, bool Wrap, bool& paragraph)
{
	WORD TAOff = 0;
	short j = 0, iWrdEnd = 0, i2WrdEnd = 0, wWrdEnd = 0, nWrdEnd = 0;
	pstring s;
	pstring s1;
	char c = '\0';
	short i = 0; short i2 = 0; short w = 0;

	bool WasWrd = false;
	short nWords = 0;
	bool Fill = false;
	while ((i < S.length()) && (i2 < 255)) {
		c = S[TAOff + i];
		if (c == 0x0D) {
			paragraph = true;
			goto label1;
		}
		if ((w >= Width) && (c == ' ') && Wrap) goto label1;
		if ((c != ' ') || WasWrd || !Wrap || paragraph) {
			i2++;
			s[TAOff + i2] = c;
			if (!IsPrintCtrl(c)) w++;
		}
		if (c == ' ') {
			if (WasWrd)
			{
				WasWrd = false;
				i2WrdEnd = i2 - 1;
				iWrdEnd = i;
				wWrdEnd = w - 1;
				nWrdEnd = nWords;
			}
		}
		else if (!WasWrd) {
			WasWrd = true;
			paragraph = false;
			nWords++;
		}
		i++;
	}
label1:
	if (Wrap && (nWords >= 2) && (w > Width)) {
		i2 = i2WrdEnd;
		i = iWrdEnd;
		w = wWrdEnd;
		nWords = nWrdEnd;
		Fill = true;
		paragraph = false;
	}
	if ((i < S.length()) && (S[TAOff + i] == 0x0D)) {
		i++;
		if ((i < S.length()) && (S[TAOff + i] == 0x0A)) i++;
	}
	TAOff += i;
	S = S.erase(0, i); // TLen -= i;
	short l2 = i2;
	if (w < Width) l2 += (Width - w);
	short n = l2 - i2;
	if ((nWords <= 1) || (n == 0) || !Fill)
	{
		FillChar(&s[i2 + 1], n, ' ');
		s[0] = (char)l2;
	}
	else {
		short n1 = n / (nWords - 1);
		short n2 = n % (nWords - 1);
		s[0] = (char)i2;
		s1 = s;
		s[0] = (char)l2;
		i2 = 1;
		WasWrd = false;
		for (i = 1; i <= s1.length(); i++) {
			s[i2] = s1[i];
			if (s[i2] != ' ') WasWrd = true;
			else if (WasWrd) {
				WasWrd = false;
				for (j = 1; j <= n1; j++) {
					i2++;
					s[i2] = ' ';
				}
				if (n2 > 0) {
					n2--;
					i2++;
					s[i2] = ' ';
				}
			}
			i2++;
		}
	}
	return s;
}

/// <summary>
/// Vlozi text na dany radek na dane misto o max. definovane delce
/// </summary>
/// <param name="S">Vstupni text</param>
/// <param name="Col">Pozice, kam text vlozit</param>
/// <param name="Width">Delka textu</param>
/// <param name="Wrap"></param>
/// <returns></returns>
std::string Report::NewTxtCol(std::string S, WORD Col, WORD Width, bool Wrap)
{
	WORD Ln = 0;
	TTD TD;
	std::string ss;
	std::vector<std::string> SL;

	Ln = 0;
	bool Absatz = true;
	if (Wrap) for (size_t i = 0; i < S.length(); i++) {
		if ((S[i] == 0x0D) && ((i == S.length()) || (S[i + 1] != 0x0A))) S[i] = ' ';
	}

	ss = GetLine(S, Width, Wrap, Absatz);
	//printf("%s%s", Rprt.c_str(), ss.c_str());

	while (S.length() > 0) {
		std::string strLine = GetLine(S, Width, Wrap, Absatz);
		Ln++;
		if (Ln == 1) {
			//TD = new TTD();
			//TD->SL.clear();
			TD.Col = Col;
			TD.Width = Width;
		}
		//SL = new StringListEl();
		//SL->S = strLine;
		//if (TD->SL == nullptr) TD->SL = SL;
		//else ChainLast(TD->SL, SL);
		TD.SL.push_back(strLine);
	}
	if (Ln > 0) {
		TD.Ln = Ln;
		//if (Y.TD == nullptr) Y.TD = TD;
		//else ChainLast(Y.TD, TD);
		Y.TD.push_back(TD);

		if (Ln > Y.TLn) Y.TLn = Ln;
	}
	return ss;
}

void Report::CheckPgeLimit(std::string& text)
{
	if (Y.ChkPg && (RprtLine > PgeLimit) && (PgeLimit < PgeSize)) {
		//void* p2 = Store2Ptr;
		//MarkStore(Store2Ptr);
		YRec YY = Y;
		ResetY();
		NewPage(text);
		Y = YY;
		//Store2Ptr = p2;
	}
}

void Report::PendingTT(std::string& text)
{
	WORD lll = LineLenLst;
	WORD Col = LineLenLst + 1;

	if (Y.TLn > 0) {
		std::vector<std::string>::iterator it0 = Y.TD[0].SL.begin();

		while (Y.TLn > 0) {
			NewLine(text);
			CheckPgeLimit(text);
			Col = 1;

			//TTD* TD = Y.TD;
			//while (TD != nullptr) {
			for (TTD& TD : Y.TD) {
				if (TD.Ln > 0) {
					WriteNBlks(text, TD.Col - Col);
					text += *it0;
					Col = TD.Col + GetLengthOfStyledString(*it0);
					TD.Ln--;
					++it0;
				}
				//TD = TD->pChain;
			}

			Y.TLn--;
			LineLenLst = lll;
		}
	}

	WriteNBlks(text, LineLenLst + 1 - Col);

	//if (Y.TD != nullptr) {
	//	ReleaseStore(&Store2Ptr);
	Y.TD.clear();
	Y.TLn = 0;
	//}
}

void Report::PrintBlock(std::vector<BlkD*>& block, std::string& text, std::vector<BlkD*>& dh)
{
	WORD LAfter = 0;
	std::vector<BlkD*>::iterator DH = dh.begin();
	//BlkD* B1 = nullptr;
	bool pdh = false;
	//while (b != nullptr) {
	for (BlkD* b : block) {
		if (RunBool(CFile, b->Bool, CRecPtr)) {
			if (b != Y.Blk) {
				if ((b->NTxtLines > 0) && (b->NBlksFrst < LineLenLst)) {
					TruncLine(text);
				}
				if (OutOfLineBound(b)) {
					WasFF2 = true;
				}
				LAfter = RprtLine + MaxI(0, b->NTxtLines - 1);
				if ((DH != dh.end()) && (PrintDH >= (*DH)->DHLevel + 1)) {
					std::vector<BlkD*>::iterator B1 = DH;
					while (B1 != dh.end()) {
						if (RunBool(CFile, (*B1)->Bool, CRecPtr)) LAfter += (*B1)->NTxtLines;
						++B1; // = B1->pChain;
					}
				}
				if (b->FF1 || WasFF2 || FrstBlk && (b->NTxtLines > 0) ||
					(PgeLimit < PgeSize) && (LAfter > PgeLimit))
					NewPage(text);
				if ((DH != dh.end()) && (PrintDH >= (*DH)->DHLevel + 1)) {
					// create new vector from DH to the end
					std::vector<BlkD*> dh2(DH, dh.end());
					PrintBlkChn(dh2, text, true, false);
					PrintDH = 0;
				}
			}
			WasOutput = false;
			PrintTxt(b, text, true);
			WasFF2 = b->FF2;
			if ((DH == dh.end()) && WasOutput) {
				pdh = true;
			}
			SumUp(CFile, b->Sum, CRecPtr);
		}
		//b = b->pChain;
	}
	if (pdh) PrintDH = 2;
}

void Report::Footings(LvDescr* L, LvDescr* L2, std::string& text)
{
	std::vector<BlkD*> unused;
	while (L != nullptr) {
		PrintBlock(L->Ft, text, unused);
		if (L == L2) return;
		L = L->Chain;
	}
}

void Report::Headings(LvDescr* L, LvDescr* L2, std::string& text)
{
	std::vector<BlkD*> unused;
	while ((L != nullptr) && (L != L2)) {
		PrintBlock(L->Hd, text, unused);
		L = L->ChainBack;
	}
}

void Report::ReadInpFile(InpD* ID)
{
	CRecPtr = ID->ForwRecPtr;
label1:
	ID->Scan->GetRec(CRecPtr);
	if (ID->Scan->eof) return;
	if (ESCPressed() && PromptYN(24)) {
		WasLPTCancel = true;
		GoExit();
	}
	RecCount++;
	RunMsgN(RecCount);
	if (!RunBool(CFile, ID->Bool, CRecPtr)) goto label1;
}

void Report::OpenInp()
{
	NRecsAll = 0;
	for (short i = 1; i <= MaxIi; i++) {
		CFile = IDA[i]->Scan->FD;
		if (IDA[i]->Scan->Kind == 5) IDA[i]->Scan->SeekRec(0);
		else {
			IDA[i]->Md = CFile->NewLockMode(RdMode);
			IDA[i]->Scan->ResetSort(IDA[i]->SK, IDA[i]->Bool, IDA[i]->Md, IDA[i]->SQLFilter, CRecPtr);
		}
		NRecsAll += IDA[i]->Scan->NRecs;
	}
}

void Report::CloseInp()
{
	for (WORD i = 1; i <= MaxIi; i++) {
		if (IDA[i]->Scan->Kind != 5) {
			IDA[i]->Scan->Close();
			CFile->ClearRecSpace(IDA[i]->ForwRecPtr);
			CFile->OldLockMode(IDA[i]->Md);
		}
	}
}

WORD Report::CompMFlds(std::vector<ConstListEl>& C, std::vector<KeyFldD*>& M, short& NLv)
{
	XString x;
	NLv = 0;
	//for (ConstListEl& c : C) {
	for (size_t i = 0; i < C.size(); i++) {
		ConstListEl& c = C[i];
		KeyFldD* m = M[i];
		NLv++;
		x.Clear();
		x.StoreKF(CFile, m, CRecPtr);
		std::string s = x.S;
		int res = CompStr(s, c.S);
		if (res != _equ) {
			return res;
		}
		//M = M->pChain;
	}
	return _equ;
}

void Report::GetMFlds(std::vector<ConstListEl>& C, std::vector<KeyFldD*>& M)
{
	//for (auto& c : C) {
	for (size_t i = 0; i < C.size(); i++) {
		ConstListEl& c = C[i];
		KeyFldD* m = M[i];
		XString x;
		x.Clear();
		x.StoreKF(CFile, m, CRecPtr);
		c.S = x.S;
		//M = M->pChain;
	}
}

void Report::MoveMFlds(std::vector<ConstListEl>& C1, std::vector<ConstListEl>& C2)
{
	for (size_t i = 0; i < C2.size(); i++) {
		// puvodne se kopiroval jen pstring z C1 do C2
		C2[i] = C1[i];
	}
}

void Report::PutMFlds(std::vector<KeyFldD*>& M)
{
	if (MinID == nullptr) return;
	FileD* cf = CFile;
	FileD* cf1 = MinID->Scan->FD;
	void* cr = CRecPtr;
	void* cr1 = MinID->ForwRecPtr;
	//KeyFldD* m1 = MinID->MFld;

	//while (M != nullptr) {
	for (size_t i = 0; i < M.size(); i++) {
		FieldDescr* f = M[i]->FldD;
		FieldDescr* f1 = MinID->MFld[i]->FldD;
		CFile = cf1;
		CRecPtr = cr1;
		switch (f->frml_type) {
		case 'S': {
			std::string s = CFile->loadS(f1, CRecPtr);
			CFile = cf;
			CRecPtr = cr;
			CFile->saveS(f, s, CRecPtr);
			break;
		}
		case 'R': {
			double r = CFile->loadR(f1, CRecPtr);
			CFile = cf;
			CRecPtr = cr;
			CFile->saveR(f, r, CRecPtr);
			break;
		}
		default: {
			bool b = CFile->loadB(f1, CRecPtr);
			CFile = cf;
			CRecPtr = cr;
			CFile->saveB(f, b, CRecPtr);
			break;
		}
		}
		//M = M->pChain;
		//m1 = m1->pChain;
	}
}

void Report::GetMinKey()
{
	short i, nlv;
	short mini = 0; NEof = 0;
	for (i = 1; i <= MaxIi; i++) {
		CFile = IDA[i]->Scan->FD;
		if (IDA[i]->Scan->eof) NEof++;
		if (OldMFlds.empty()) {
			IDA[i]->Exist = !IDA[i]->Scan->eof;
			mini = 1;
		}
		else {
			CRecPtr = IDA[i]->ForwRecPtr;
			IDA[i]->Exist = false;
			if (!IDA[i]->Scan->eof) {
				WORD res;
				if (mini == 0) goto label1;
				res = CompMFlds(NewMFlds, IDA[i]->MFld, nlv);
				if (res != _gt) {
					if (res == _lt)
					{
					label1:
						GetMFlds(NewMFlds, IDA[i]->MFld);
						mini = i;
					}
					IDA[i]->Exist = true;
				}
			}
		}
	}
	if (mini > 0) {
		for (i = 1; i <= mini - 1; i++) {
			IDA[i]->Exist = false;
		}
		MinID = IDA[mini];
	}
	else {
		MinID = nullptr;
	}
}

void Report::ZeroCount()
{
	for (short i = 1; i <= MaxIi; i++) {
		IDA[i]->Count = 0.0;
	}
}

LvDescr* Report::GetDifLevel()
{
	//ConstListEl* C1 = NewMFlds;
	//ConstListEl* C2 = OldMFlds;
	//KeyFldD* M = IDA[1]->MFld;
	LvDescr* L = LstLvM->ChainBack;
	size_t vIndex = 0;
	//while (M != nullptr) {
	for (KeyFldD* M : IDA[1]->MFld) {
		if (NewMFlds[vIndex].S != OldMFlds[vIndex].S) {
			return L;
		}
		vIndex++;
		//M = (KeyFldD*)M->pChain;
		L = L->ChainBack;
	}
	return nullptr;
}

void Report::MoveForwToRec(InpD* ID)
{
	CFile = ID->Scan->FD;
	CRecPtr = CFile->FF->RecPtr;
	Move(ID->ForwRecPtr, CRecPtr, CFile->FF->RecLen + 1);
	ID->Count = ID->Count + 1;
	// LogicControl* C = ID->Checks;
	if (!ID->Chk.empty()) { //if (C != nullptr) {
		ID->Error = false;
		ID->Warning = false;
		ID->ErrTxtFrml->S = ""; // ID->ErrTxtFrml->S[0] = 0;
		for (LogicControl* C : ID->Chk) { //while (C != nullptr) {
			if (!RunBool(CFile, C->Bool, CRecPtr)) {
				ID->Warning = true;
				ID->ErrTxtFrml->S = RunString(CFile, C->TxtZ, CRecPtr);
				if (!C->Warning) {
					ID->Error = true;
					return;
				}
			}
			//C = (LogicControl*)C->pChain;
		}
	}
}

void Report::MoveFrstRecs()
{
	for (short i = 1; i <= MaxIi; i++) {
		if (IDA[i]->Exist) {
			MoveForwToRec(IDA[i]);
		}
		else {
			CFile = IDA[i]->Scan->FD;
			CRecPtr = CFile->FF->RecPtr;
			CFile->ZeroAllFlds(CRecPtr, false);
			PutMFlds(IDA[i]->MFld);
		}
	}
}

void Report::MergeProc(std::string& text)
{
	short nlv = 0;
	short res = 0;
	for (short i = 1; i <= MaxIi; i++) {
		InpD* ID = IDA[i];
		if (ID->Exist) {
			CFile = ID->Scan->FD;
			CRecPtr = CFile->FF->RecPtr;
			LvDescr* L = ID->LstLvS;
		label1:
			ZeroSumFlds(L);
			GetMFlds(ID->OldSFlds, ID->SFld);
			if (WasFF2) PrintPageHd(text);
			Headings(L, ID->FrstLvS, text);
			if (PrintDH == 0) PrintDH = 1;
		label2:
			PrintBlock(ID->FrstLvS->Ft, text, ID->FrstLvS->Hd); /*DE*/
			SumUp(CFile, ID->Sum, CRecPtr);
			ReadInpFile(ID);
			if (ID->Scan->eof) goto label4;
			res = CompMFlds(NewMFlds, ID->MFld, nlv);
			if ((res == _lt) && (MaxIi > 1)) {
				SetMsgPar(ID->Scan->FD->Name);
				RunError(607);
			}
			if (res != _equ) goto label4;
			res = CompMFlds(ID->OldSFlds, ID->SFld, nlv);
			if (res == _equ) {
				MoveForwToRec(ID);
				goto label2;
			}
			L = ID->LstLvS;
			while (nlv > 1) {
				L = L->ChainBack;
				nlv--;
			}
			Footings(ID->FrstLvS->Chain, L, text);
			if (WasFF2) PrintPageFt(text);
			MoveForwToRec(ID);
			goto label1;
		label4:
			Footings(ID->FrstLvS->Chain, ID->LstLvS, text);
		}
	}
}

bool Report::RewriteRprt(RprtOpt* RO, WORD pageLimit, WORD& Times, bool& IsLPT1)
{
	auto result = false;
	WasLPTCancel = false;
	IsLPT1 = false;
	Times = 1;
	bool PrintCtrl = false;

	if (RO != nullptr) {
		PrintCtrl = RO->PrintCtrl;
		if (RO->Times != nullptr) Times = (WORD)RunInt(CFile, RO->Times, CRecPtr);
	}

	if (RO == nullptr || RO->Path.empty() && RO->CatIRec == 0) {
		SetPrintTxtPath();
		PrintView = true;
		PrintCtrl = false;
	}
	else {
		if (!RO->Path.empty() && EquUpCase(RO->Path, "LPT1")) {
			CPath = "LPT1";
			CVol = "";
			IsLPT1 = true;
			result = ResetPrinter(pageLimit, 0, true, true) && RewriteTxt(CPath, &Rprt, false);
			return result;
		}
		SetTxtPathVol(RO->Path, RO->CatIRec);
	}
	TestMountVol(CPath[0]);
	if (!RewriteTxt(CPath, &Rprt, PrintCtrl)) {
		SetMsgPar(CPath);
		WrLLF10Msg(700 + HandleError);
		PrintView = false;
		return result;
	}
	if (Times > 1) {
		printf("%s.ti %1i\n", Rprt.c_str(), Times);
		Times = 1;
	}
	if (pageLimit != 72) {
		printf("%s.pl %i\n", Rprt.c_str(), pageLimit);
	}
	result = true;
	return result;
}
