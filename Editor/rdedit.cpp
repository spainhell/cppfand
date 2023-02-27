#include "rdedit.h"

#include "../cppfand/ChkD.h"
#include "../cppfand/compile.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/obaseww.h"
#include "../cppfand/rdfildcl.h"
#include "../cppfand/rdrun.h"
#include "../textfunc/textfunc.h"

EditD* E = EditDRoot;

void PushEdit()
{
	auto* const e1 = new EditD();
	{
		e1->V.C1 = 1;
		e1->V.R1 = 2;
		e1->V.C2 = TxtCols;
		e1->V.R2 = TxtRows - 1;
	}
	e1->pChain = E;
	E = e1;
	EditDRoot = E;
}

void SToSL(StringListEl** SLRoot, pstring s)
{
	//StringList SL = (StringListEl*)GetStore(s.length() + 5);
	StringListEl* SL = new StringListEl();
	SL->S = s;
	if (*SLRoot == nullptr) *SLRoot = SL;
	else ChainLast(*SLRoot, SL);
}

void StoreRT(WORD Ln, StringList SL, WORD NFlds)
{
	//ERecTxtD* RT = (ERecTxtD*)GetStore(sizeof(*RT));
	ERecTxtD* RT = new ERecTxtD();
	if (NFlds == 0) Error(81);
	RT->N = Ln;
	RT->SL = SL;
	if (E->RecTxt == nullptr) E->RecTxt = RT;
	else ChainLast(E->RecTxt, RT);
}

void RdEForm(FileD* ParFD, RdbPos FormPos)
{
	EFldD* D = nullptr; EFldD* D1 = nullptr; EFldD* PrevD = nullptr;
	FieldDescr* F = nullptr; FieldListEl* FL = nullptr;
	FileD* FD1 = nullptr;
	StringListEl* SLRoot = nullptr;
	pstring s = "";
	WORD NPages = 0, Col = 0, Ln = 0, Max = 0, M = 0, N = 0, NFlds = 0, i = 0;
	bool comment = false; char c = '\0'; BYTE a = 0;
	SetInpTT(&FormPos, true);
label1:
	s = "";
	while (!(ForwChar == '#' || ForwChar == 0x1A || ForwChar == 0x0D || ForwChar == '{')) {
		/* read headlines */
		s.Append(ForwChar);
		ReadChar();
	}
	switch (ForwChar) {
	case 0x1A: Error(76); break;
	case '#': goto label2; break;
	case '{': { SkipBlank(true); goto label1; break; }
	}
	ReadChar();
	if (ForwChar == 0x0A) ReadChar();
	SToSL(&E->HdTxt, s);
	E->NHdTxt++;
	if (E->NHdTxt + 1 > E->Rows) Error(102);
	goto label1;
	/* read field list */
label2:
	ReadChar(); ReadChar();
	Lexem = CurrChar;
	Accept('_');
	FD1 = RdFileName();
	if (ParFD == nullptr) CFile = FD1;
	else CFile = ParFD;
	E->FD = CFile;
label3:
	N++;
	//D = (EFldD*)GetZStore(sizeof(*D));
	D = new EFldD();
	if (Lexem == _number) {
		M = RdInteger();
		if (M == 0) OldError(115);
		Accept(':');
		D->ScanNr = M;
	}
	else D->ScanNr = N;
	D1 = FindScanNr(D->ScanNr);

	if (E->FirstFld == nullptr) E->FirstFld = D;
	else ChainLast(E->FirstFld, D);

	if ((D1 != nullptr) && (D->ScanNr == D1->ScanNr)) Error(77);
	F = RdFldName(CFile);
	D->FldD = F;
	E->Flds.push_back(F);
	if (Lexem == ',') { RdLex(); goto label3; }
	TestLex(';');
	SkipBlank(true);
	/* read record lines */
	D = E->FirstFld;
	NPages = 0;
label4:
	NPages++; Ln = 0; NFlds = 0; SLRoot = nullptr;
label5:
	s = ""; Ln++;
	Col = E->FrstCol;
	while (!(ForwChar == 0x0D || ForwChar == 0x1A || ForwChar == '\\' || ForwChar == '{'))
		if (ForwChar == '_') {
			if (D == nullptr) Error(30);
			NFlds++;
			D->Col = Col;
			D->Ln = Ln;
			D->Page = NPages;
			M = 0;
			while (ForwChar == '_') {
				s.Append(' ');
				M++; Col++;
				ReadChar();
			}
			F = D->FldD;
			D->L = F->L;
			if (F->field_type == FieldType::TEXT) {
				D->L = 1;
			}
			if ((F->field_type == FieldType::ALFANUM) && (M < F->L)) {
				D->L = M;
			}
			else if (M != D->L) {
				str(D->L, 2, s);
				SetMsgPar(s, F->Name);
				Error(79);
			}
			if (Col > E->LastCol) Error(102);
			D = (EFldD*)D->pChain;
		}
		else {
			if (!screen.SetStyleAttr(ForwChar, a)) {
				if (Col > E->LastCol) Error(102);
				Col++;
			}
			s.Append(ForwChar);
			ReadChar();
		}
	SToSL(&SLRoot, s);
	c = ForwChar;
	if (c == '\\') ReadChar();
	SkipBlank(true);
	if (ForwChar != 0x1A) {
		if ((c == '\\') || (E->NHdTxt + Ln == E->Rows)) {
			StoreRT(Ln, SLRoot, NFlds);
			goto label4;
		}
		else goto label5;
	}
	StoreRT(Ln, SLRoot, NFlds);
	E->NPages = NPages;

	if (D != nullptr) Error(30);
	D = FindScanNr(1);
	D->ChainBack = nullptr;
	for (i = 2; i <= N; i++) {
		PrevD = D;
		D = FindScanNr(D->ScanNr + 1);
		D->ChainBack = PrevD;
	}
	E->LastFld = D;
	PrevD = nullptr;
	while (D != nullptr) {
		D->pChain = PrevD;
		PrevD = D;
		D = D->ChainBack;
	}
	E->FirstFld = PrevD;
}

EFldD* FindScanNr(WORD N)
{
	EFldD* D = E->FirstFld;
	EFldD* D1 = nullptr;
	WORD M = 0xffff;
	while (D != nullptr) {
		if ((D->ScanNr >= N) && (D->ScanNr < M)) { M = D->ScanNr; D1 = D; }
		D = (EFldD*)D->pChain;
	}
	return D1;
}

void AutoDesign(FieldListEl* FL)
{
	WORD L = 0, i = 0, m = 0, FldLen = 0;
	pstring s = "";
	StringListEl* SLRoot = nullptr;
	EFldD* D = (EFldD*)(&E->FirstFld);
	EFldD* PrevD = nullptr;
	WORD NPages = 1; WORD Ln = 0;
	WORD Col = E->FrstCol;
	WORD maxcol = E->LastCol - E->FrstCol;
	while (FL != nullptr) {
		FieldDescr* F = FL->FldD;
		FL = (FieldListEl*)FL->pChain;
		if (F == nullptr) continue; // tady to padalo na 1. polozce, protoze ta ma FldD = nullptr
		//D->pChain = (EFldD*)GetZStore(sizeof(*D)); 
		D->pChain = new EFldD();
		D = (EFldD*)D->pChain;
		D->ChainBack = PrevD;
		PrevD = D;
		D->FldD = F;
		D->L = F->L;
		if (D->L > maxcol) D->L = maxcol;
		if ((E->FD->file_type == FileType::CAT) && (D->L > 44)) D->L = 44; /*catalog pathname*/
		FldLen = D->L;
		if (F->field_type == FieldType::TEXT) D->L = 1;
		L = F->Name.length();
		if (FldLen > L) L = FldLen;
		if (Col + L > E->LastCol) {
			SToSL(&SLRoot, s);
			SToSL(&SLRoot, "");
			Ln += 2;
			if (Ln + 2 > E->Rows) {
				StoreRT(Ln, SLRoot, 1);
				NPages++;
				Ln = 0;
				SLRoot = nullptr;
			}
			Col = E->FrstCol; s = "";
		}
		m = (L - F->Name.length() + 1) / 2;
		for (i = 1; i <= m; i++) s.Append(' ');
		s = s + F->Name;
		m = L - F->Name.length() - m;
		for (i = 1; i <= m + 1; i++) s.Append(' ');
		D->Col = Col + (L - FldLen + 1) / 2;
		D->Ln = Ln + 2;
		D->Page = NPages;
		Col += (L + 1);
	}
	SToSL(&SLRoot, s);
	SToSL(&SLRoot, "");
	Ln += 2;
	StoreRT(Ln, SLRoot, 1);
	D->pChain = nullptr;
	E->LastFld = D;
	E->NPages = NPages;
	if (NPages == 1) { /* !!! with E->RecTxt^ do!!! */
		auto& er = *E->RecTxt;
		if (er.N == 2) {
			E->HdTxt = er.SL;
			er.SL = (StringListEl*)er.SL->pChain;
			E->HdTxt->pChain = nullptr;
			E->NHdTxt = 1;
			er.N = 1;
			D = E->FirstFld;
			while (D != nullptr) {
				D->Ln--;
				D = (EFldD*)D->pChain;
			}
			if (E->Rows == 1) {
				E->NHdTxt = 0;
				E->HdTxt = nullptr;
			}
		}
		else if (er.N < E->Rows) {
			s = "";
			for (i = E->FrstCol; i <= E->LastCol; i++) s.Append('-');
			SToSL(&er.SL, s);
			er.N++;
		}
	}
}

void AutoDesign(std::vector<FieldDescr*>& FL)
{
	WORD L = 0, i = 0, m = 0, FldLen = 0;
	pstring s = "";
	StringListEl* SLRoot = nullptr;
	EFldD* D = (EFldD*)(&E->FirstFld);
	EFldD* PrevD = nullptr;
	WORD NPages = 1; WORD Ln = 0;
	WORD Col = E->FrstCol;
	WORD maxcol = E->LastCol - E->FrstCol;
	//while (FL != nullptr) {
	for (auto& F : FL) {
		//FieldDescr* F = FL->FldD;
		//FL = (FieldListEl*)FL->pChain;
		if (F == nullptr) continue; // tady to padalo na 1. polozce, protoze ta ma FldD = nullptr
		D->pChain = new EFldD();
		D = D->pChain;
		D->ChainBack = PrevD;
		PrevD = D;
		D->FldD = F;
		D->L = F->L;
		if (D->L > maxcol) D->L = maxcol;
		if ((E->FD->file_type == FileType::CAT) && (D->L > 44)) D->L = 44; /*catalog pathname*/
		FldLen = D->L;
		if (F->field_type == FieldType::TEXT) D->L = 1;
		L = F->Name.length();
		if (FldLen > L) L = FldLen;
		if (Col + L > E->LastCol) {
			SToSL(&SLRoot, s);
			SToSL(&SLRoot, "");
			Ln += 2;
			if (Ln + 2 > E->Rows) {
				StoreRT(Ln, SLRoot, 1);
				NPages++;
				Ln = 0;
				SLRoot = nullptr;
			}
			Col = E->FrstCol; s = "";
		}
		m = (L - F->Name.length() + 1) / 2;
		for (i = 1; i <= m; i++) s.Append(' ');
		s = s + F->Name;
		m = L - F->Name.length() - m;
		for (i = 1; i <= m + 1; i++) s.Append(' ');
		D->Col = Col + (L - FldLen + 1) / 2;
		D->Ln = Ln + 2;
		D->Page = NPages;
		Col += (L + 1);
	}
	SToSL(&SLRoot, s);
	SToSL(&SLRoot, "");
	Ln += 2;
	StoreRT(Ln, SLRoot, 1);
	D->pChain = nullptr;
	E->LastFld = D;
	E->NPages = NPages;
	if (NPages == 1) { /* !!! with E->RecTxt^ do!!! */
		auto& er = *E->RecTxt;
		if (er.N == 2) {
			E->HdTxt = er.SL;
			er.SL = er.SL->pChain;
			E->HdTxt->pChain = nullptr;
			E->NHdTxt = 1;
			er.N = 1;
			D = E->FirstFld;
			while (D != nullptr) {
				D->Ln--;
				D = D->pChain;
			}
			if (E->Rows == 1) {
				E->NHdTxt = 0;
				E->HdTxt = nullptr;
			}
		}
		else if (er.N < E->Rows) {
			s = "";
			for (i = E->FrstCol; i <= E->LastCol; i++) s.Append('-');
			SToSL(&er.SL, s);
			er.N++;
		}
	}
}

void RdFormOrDesign(FileD* F, std::vector<FieldDescr*>& FL, RdbPos FormPos)
{
	E->FrstCol = E->V.C1; E->FrstRow = E->V.R1; E->LastCol = E->V.C2; E->LastRow = E->V.R2;
	if ((E->WFlags & WHasFrame) != 0) {
		E->FrstCol++; E->LastCol--; E->FrstRow++; E->LastRow--;
	}
	E->Rows = E->LastRow - E->FrstRow + 1;
	if (FL.empty()) {
		ResetCompilePars();
		RdEForm(F, FormPos);
		E->IsUserForm = true;
	}
	else {
		E->FD = F;
		E->Flds = FL;
		AutoDesign(FL);
	}
}

void NewEditD(FileD* ParFD, EditOpt* EO)
{
	EFldD* D = nullptr;
	FieldList FL = nullptr;
	WORD i = 0; //pstring s;
	FieldDescr* F = nullptr;
	bool b = false, b2 = false, F2NoUpd = false;
	PushEdit();
	/* !!! with E^ do!!! */
	//Move(&EO->WFlags, &E->WFlags, (uintptr_t)(E->SelKey) - (uintptr_t)(E->WFlags) + 4);
	// move je nahrazen kopirovanim jednotlivych polozek:
	E->WFlags = EO->WFlags;
	E->ExD = EO->ExD;
	E->Journal = EO->Journal;
	E->ViewName = EO->ViewName;
	E->OwnerTyp = EO->OwnerTyp;
	E->DownLD = EO->DownLD;
	E->DownLV = EO->DownLV;
	E->DownRecPtr = EO->DownRecPtr;
	E->LVRecPtr = EO->LVRecPtr;
	E->KIRoot = EO->KIRoot;
	E->SQLFilter = EO->SQLFilter;
	E->SelKey = (XWKey*)EO->SelKey;
	//rectxt

	E->Attr = RunWordImpl(EO->ZAttr, screen.colors.dTxt);
	E->dNorm = RunWordImpl(EO->ZdNorm, screen.colors.dNorm);
	E->dHiLi = RunWordImpl(EO->ZdHiLi, screen.colors.dHili);
	E->dSubSet = RunWordImpl(EO->ZdSubset, screen.colors.dSubset);
	E->dDel = RunWordImpl(EO->ZdDel, screen.colors.dDeleted);
	E->dTab = RunWordImpl(EO->ZdTab, E->Attr | 0x08);
	E->dSelect = RunWordImpl(EO->ZdSelect, screen.colors.dSelect);
	E->Top = RunStdStr(EO->Top);
	if (EO->Mode != nullptr) EditModeToFlags(RunShortStr(EO->Mode), &E->NoDelete, false);
	if (spec.Prompt158) E->Prompt158 = true;
	if (EO->SetOnlyView /*UpwEdit*/) {
		EO->Tab.clear();
		E->OnlyTabs = true;
		E->OnlySearch = false;
	}
	if (E->LVRecPtr != nullptr) { E->EdRecVar = true; E->Only1Record = true; }
	if (E->Only1Record) E->OnlySearch = false;
	if (EO->W.C1 != nullptr) {
		RunWFrml(EO->W, E->WFlags, E->V);
		E->WwPart = true;
		if ((E->WFlags & WShadow) != 0) {
			E->ShdwX = MinW(2, TxtCols - E->V.C2);
			E->ShdwY = MinW(1, TxtRows - E->V.R2);
		}
	}
	else {
		if (E->WithBoolDispl) E->V.R1 = 3;
		if (E->Mode24) E->V.R2--;
	}
	RdFormOrDesign(ParFD, EO->Flds, EO->FormPos);
	if (E->NPages > 1) {
		E->NRecs = 1;
	}
	else {
		E->NRecs = (E->Rows - E->NHdTxt) / E->RecTxt->N;
	}
	E->BaseRec = 1;
	E->IRec = 1;
	E->CFld = E->FirstFld;
	E->FirstEmptyFld = E->FirstFld;
	E->ChkSwitch = true;
	E->WarnSwitch = true;
	CFile = E->FD;
	CRecPtr = GetRecSpace();
	E->OldRecPtr = CRecPtr;
#ifdef FandSQL
	if (CFile->IsSQLFile) SetTWorkFlag;
#endif
	if (E->EdRecVar) {
		E->NewRecPtr = E->LVRecPtr;
		E->NoDelete = true;
		E->NoCreate = true;
		E->Journal = nullptr;
		E->KIRoot = nullptr;
	}
	else {
		CRecPtr = GetRecSpace();
		E->NewRecPtr = CRecPtr;
#ifdef FandSQL
		if (CFile->IsSQLFile) SetTWorkFlag;
#endif
		E->AddSwitch = true;
		E->Cond = RunEvalFrml(EO->Cond);
		E->RefreshDelay = RunWordImpl(EO->RefreshDelayZ, spec.RefreshDelay) * 18;
		E->SaveAfter = RunWordImpl(EO->SaveAfterZ, spec.UpdCount);
		if (EO->StartRecKeyZ != nullptr) {
			E->StartRecKey = RunShortStr(EO->StartRecKeyZ);
		}
		E->StartRecNo = RunInt(EO->StartRecNoZ);
		E->StartIRec = RunInt(EO->StartIRecZ);
		E->VK = EO->ViewKey;
		if (E->DownLD != nullptr) {
			E->DownSet = true;
			E->DownKey = GetFromKey(E->DownLD);
			if (E->VK == nullptr) {
				E->VK = E->DownKey;
			}
			switch (E->OwnerTyp) {
			case 'r': {
				E->DownRecPtr = E->DownLV->RecPtr;
				break;
			}
			case 'F': {
				E->OwnerRecNo = RunInt((FrmlElem*)EO->DownLV);
				CFile = E->DownLD->ToFD;
				E->DownRecPtr = GetRecSpace();
				CFile = E->FD;
				break;
			}
			default:;
			}
		}
		else if (E->VK == nullptr) {
			E->VK = E->FD->Keys.empty() ? nullptr : E->FD->Keys[0];
		}
#ifdef FandSQL
		if (CFile->IsSQLFile && (E->VK = nullptr)) { SetMsgPar(CFile->Name); RunError(652); }
#endif
		if (E->SelKey != nullptr) {
			if (E->SelKey->KFlds == nullptr) {
				E->SelKey->KFlds = E->VK->KFlds;
			}
			else if (!EquKFlds(E->SelKey->KFlds, E->VK->KFlds)) {
				RunError(663);
			}
		}
	}
	if (EO->StartFieldZ != nullptr) {
		std::string rss = RunShortStr(EO->StartFieldZ);
		std::string s = TrailChar(rss, ' ');
		D = E->FirstFld;
		while (D != nullptr) {
			if (EquUpCase(D->FldD->Name, s)) E->StartFld = D;
			D = (EFldD*)D->pChain;
		}
	}
	E->WatchDelay = RunInt(EO->WatchDelayZ) * 18;
	if (EO->Head == nullptr) {
		E->Head = StandardHead();
	}
	else {
		E->Head = GetStr_E(EO->Head);
	}
	E->Last = GetStr_E(EO->Last);
	E->AltLast = GetStr_E(EO->AltLast);
	E->CtrlLast = GetStr_E(EO->CtrlLast);
	E->ShiftLast = GetStr_E(EO->ShiftLast);
	F2NoUpd = E->OnlyTabs && EO->Tab.empty() && !EO->NegTab && E->OnlyAppend;
	/* END WITH */

	D = E->FirstFld;
	while (D != nullptr) {
		E->NFlds++; F = D->FldD;
		b = FieldInList(F, EO->Tab);
		if (EO->NegTab) b = !b;
		if (b) { D->Tab = true; E->NTabsSet++; }
		b2 = FieldInList(F, EO->NoEd);
		if (EO->NegNoEd) b2 = !b2;
		D->EdU = !(b2 || E->OnlyTabs && !b);
		D->EdN = F2NoUpd;
		if (((F->Flg & f_Stored) != 0) && D->EdU) E->NEdSet++;
		b = FieldInList(F, EO->Dupl);
		if (EO->NegDupl) b = !b;
		if (b && ((F->Flg & f_Stored) != 0)) D->Dupl = true;
		if (b || ((F->Flg & f_Stored) != 0)) E->NDuplSet++;
		D = (EFldD*)D->pChain;
	}
	if (E->OnlyTabs && (E->NTabsSet == 0)) {
		E->NoDelete = true;
		if (!E->OnlyAppend) {
			E->NoCreate = true;
		}
	}
	RdDepChkImpl();
	NewChkKey();
	MarkStore(E->AfterE);
}

EFldD* FindEFld_E(FieldDescr* F)
{
	EFldD* D = E->FirstFld;
	while (D != nullptr) {
		if (D->FldD == F) break;
		D = (EFldD*)D->pChain;
	}
	return D;
}

void ZeroUsed()
{
	EFldD* D = E->FirstFld;
	while (D != nullptr) {
		D->Used = false;
		D = (EFldD*)D->pChain;
	}
}

EFldD* LstUsedFld()
{
	EFldD* D = E->LastFld;
	while (D != nullptr)
	{
		if (D->Used) break;
		D = D->ChainBack;
	}
	return D;
}

void RdDepChkImpl()
{
	std::string s;
	CFile = E->FD;
	switch (CFile->file_type) {
	case FileType::RDB: {
		RdMsg(53);
		s = MsgLine;
		ResetCompilePars();
		SetInpStr(s);
		RdUDLI();
		break;
	}
	case FileType::CAT: {
		RdMsg(54);
		s = MsgLine;
		if (spec.CPMdrive != ' ') s = s + ',' + spec.CPMdrive + ':';
		RdMsg(55);
		s = s + MsgLine;
		if (spec.CPMdrive != ' ') s = s + ',' + spec.CPMdrive + ':';
		s = s + "'";
		ResetCompilePars();
		SetInpStr(s);
		RdUDLI();
		break;
	}
	default: {
		RdAllUDLIs(CFile);
		break;
	}
	}
}

void TestedFlagOff()
{
	for (auto& F : CFile->FldD) {
		F->field_flag = false;
	}
}

void SetFrmlFlags(FrmlElem* Z)
{
	auto iZ0 = (FrmlElem0*)Z;
	auto iZ7 = (FrmlElem7*)Z;
	if (Z == nullptr) return;

	switch (Z->Op) {
	case _field: {
		SetFlag(iZ7->Field);
		break;
	}
	case _access: {
		if (iZ7->LD != nullptr) {
			//KeyFldD* Arg = iZ7->LD->Args;
			//while (Arg != nullptr) {
			for (auto& arg : iZ7->LD->Args) {
				SetFlag(arg->FldD);
				//Arg = Arg->pChain;
			}
		}
		break;
	}
	case _userfunc: {
		FrmlList fl = ((FrmlElem19*)Z)->FrmlL;
		while (fl != nullptr) {
			SetFrmlFlags(fl->Frml);
			fl = fl->pChain;
		}
		break;
	}
	default: {
		if (Z->Op >= 0x60 && Z->Op <= 0xAF) {
			/*1-ary*/
			SetFrmlFlags(iZ0->P1);
		}
		else if (Z->Op >= 0xB0 && Z->Op <= 0xEF) {
			/*2-ary*/
			SetFrmlFlags(iZ0->P1);
			SetFrmlFlags(iZ0->P2);
		}
		else if (Z->Op >= 0xF0 /*&& Z->Op <= 0xFF*/) {
			/*3-ary*/
			SetFrmlFlags(iZ0->P1);
			SetFrmlFlags(iZ0->P2);
			SetFrmlFlags(iZ0->P3);
		}
		break;
	}
	}
}

void SetFlag(FieldDescr* F)
{
	if (F->field_flag) {
		return;
	}
	else {
		F->field_flag = true;
		if ((F->Flg & f_Stored) != 0) {
			EFldD* D = FindEFld_E(F);
			if (D != nullptr) {
				D->Used = true;
			}
		}
		else {
			SetFrmlFlags(F->Frml);
		}
	}
}

void RdDep()
{
	FrmlElem* Bool = nullptr; FrmlElem* Z = nullptr;
	EFldD* D = nullptr; char FTyp = '\0'; DepD* Dp = nullptr;

	RdLex();
label1:
	Accept('(');
	Bool = RdBool();
	Accept(')');
label2:
	D = FindEFld_E(RdFldName(CFile));
	Accept(_assign);
	Z = RdFrml(FTyp);
	if (D != nullptr)
	{
		Dp = new DepD();
		Dp->Bool = Bool;
		Dp->Frml = Z;
		if (D->Dep == nullptr) D->Dep = Dp;
		else ChainLast(D->Dep, Dp);
	}
	if (Lexem == ';')
	{
		RdLex();
		if (!(Lexem == '#' || Lexem == 0x1A))
		{
			if (Lexem == '(') goto label1;
			else goto label2;
		}
	}
}

void RdCheck()
{
	SkipBlank(false);
	size_t Low = CurrPos;
	RdLex();
	while (true) {
		ChkD* C = RdChkD(Low);
		ZeroUsed();
		SetFrmlFlags(C->Bool);
		TestedFlagOff();
		EFldD* D = LstUsedFld();
		if (D != nullptr) {
			if (D->Chk == nullptr) D->Chk = C;
			else ChainLast(D->Chk, C);
		}
		else ReleaseStore(C);
		if (Lexem == ';')
		{
			SkipBlank(false);
			Low = CurrPos;
			RdLex();
			if (!(Lexem == '#' || Lexem == 0x1A)) continue;
		}
		break;
	}
}

void RdImpl()
{
	// TODO: nema byt FTyp vstupnim parametrem?
	char FTyp = '\0';
	RdLex();
	while (true) {
		FieldDescr* F = RdFldName(CFile);
		Accept(_assign);
		FrmlElem* Z = RdFrml(FTyp);
		EFldD* D = FindEFld_E(F);
		if (D != nullptr) D->Impl = Z;
		else {
			ImplD* ID = new ImplD(); // (ImplD*)GetStore(sizeof(*ID));
			ID->FldD = F;
			ID->Frml = Z;
			if (E->Impl == nullptr) E->Impl = ID;
			else ChainLast(E->Impl, ID);
		}
		if (Lexem == ';') {
			RdLex();
			if (!(Lexem == '#' || Lexem == 0x1A)) continue;
		}
		break;
	}
}

void RdUDLI()
{
	RdLex();
	if ((Lexem == '#') && (ForwChar == 'U')) {
		do { RdLex(); } while (!(Lexem == '#' || Lexem == 0x1A));
	}
	if ((Lexem == '#') && (ForwChar == 'D')) {
		RdLex(); RdDep();
	}
	if ((Lexem == '#') && (ForwChar == 'L')) {
		RdLex(); RdCheck();
	}
	if ((Lexem == '#') && (ForwChar == 'I')) {
		RdLex(); RdImpl();
	}
}

void RdAllUDLIs(FileD* FD)
{
	RdbD* r = nullptr;
	if (FD->OrigFD != nullptr) {
		// this FD was created as 'LIKE'
		RdAllUDLIs(FD->OrigFD);
	}
	if (FD->TxtPosUDLI != 0) {
		ResetCompilePars();
		SetInpTTxtPos(FD);
		r = CRdb;
		CRdb = FD->ChptPos.R;
		RdUDLI();
		CRdb = r;
	}
}

std::string StandardHead()
{
	std::string s;
	std::string c = "          ______                                 __.__.____";
	if (!E->ViewName.empty()) s = E->ViewName;
	else if (E->EdRecVar) s = "";
	else {
		s = E->FD->Name;
		switch (E->FD->file_type) {
		case FileType::INDEX: {
			if (!E->VK->Alias.empty()) s = s + "/" + E->VK->Alias;
			break;
		}
		case FileType::RDB: s += ".RDB"; break;
		case FileType::FAND8: s += ".DTA"; break;
		}
	}
	//if (s.length() > 16) s[0] = 16;
	s = s.substr(0, 16); // max. length is 16 chars
	//auto str = copy(c, 17, 20 - s.length()) + s + c;
	s = c.substr(16, 20 - s.length()) + s + c;
	return s;
}

pstring GetStr_E(FrmlElem* Z)
{
	if (Z == nullptr) return "";
	else {
		std::string s = RunShortStr(Z);
		//while (GetLengthOfStyledString(s) > TxtCols) {
		//	// smaz posledni znak z retezce
		//	s.erase(s.length() - 1);
		//}
		s = GetStyledStringOfLength(s, 0, TxtCols);
		return s;
	}
}

void NewChkKey()
{
	//XKey* K = CFile->Keys;
	KeyFldD* KF = nullptr;
	EFldD* D = nullptr;
	KeyListEl* KL = nullptr;
	for (auto& K : CFile->Keys) {
		//while (K != nullptr) {
		if (!K->Duplic) {
			ZeroUsed();
			KF = K->KFlds;
			while (KF != nullptr) {
				D = FindEFld_E(KF->FldD);
				if (D != nullptr) D->Used = true;
				KF = KF->pChain;
			}
			D = LstUsedFld();
			if (D != nullptr) {
				KL = new KeyListEl();
				if (D->KL == nullptr) D->KL = KL;
				else ChainLast(D->KL, KL);
				KL->Key = K;
			}
		}
		//K = K->Chain;
	}
}
