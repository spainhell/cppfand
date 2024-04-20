#include "EditReader.h"

#include "../Core/ChkD.h"
#include "../Core/compile.h"
#include "../Core/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/KeyFldD.h"
#include "../Core/obaseww.h"
#include "../Core/rdfildcl.h"
#include "../Core/rdrun.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"
#include "../fandio/XWKey.h"

std::vector<EditD*> v_edits;

EditReader::EditReader()
{
	edit_ = new EditD();
}

void PushEdit()
{
	EditD* const e1 = new EditD();
	e1->V.C1 = 1;
	e1->V.R1 = 2;
	e1->V.C2 = TxtCols;
	e1->V.R2 = TxtRows - 1;

	v_edits.push_back(e1);
}

void EditReader::SToSL(StringListEl** SLRoot, pstring s)
{
	StringListEl* SL = new StringListEl();
	SL->S = s;
	if (*SLRoot == nullptr) *SLRoot = SL;
	else ChainLast(*SLRoot, SL);
}

void EditReader::StoreRT(WORD Ln, StringList SL, WORD NFlds)
{
	ERecTxtD* RT = new ERecTxtD();
	if (NFlds == 0) Error(81);
	RT->N = Ln;
	RT->SL = SL;
	if (edit_->RecTxt == nullptr) edit_->RecTxt = RT;
	else ChainLast(edit_->RecTxt, RT);
}

void EditReader::RdEForm(EditD* edit, RdbPos FormPos)
{
	EFldD* D = nullptr; EFldD* D1 = nullptr; EFldD* PrevD = nullptr;
	FieldDescr* F = nullptr; FieldListEl* FL = nullptr;
	FileD* FD1 = nullptr;
	StringListEl* SLRoot = nullptr;
	pstring s = "";
	WORD NPages = 0, Col = 0, Ln = 0, Max = 0, M = 0, N = 0, NFlds = 0, i = 0;
	bool comment = false; char c = '\0'; BYTE a = 0;
	SetInpTT(&FormPos, true);

	while (true) {
		s = "";
		while (!(ForwChar == '#' || ForwChar == 0x1A || ForwChar == 0x0D || ForwChar == '{')) {
			/* read headlines */
			s.Append(ForwChar);
			ReadChar();
		}
		switch (ForwChar) {
		case 0x1A: {
			Error(76);
			break;
		}
		case '#': {
			goto label2;
			break;
		}
		case '{': {
			SkipBlank(true);
			continue;
			break;
		}
		}
		ReadChar();
		if (ForwChar == 0x0A) ReadChar();
		SToSL(&edit->HdTxt, s);
		edit->NHdTxt++;
		if (edit->NHdTxt + 1 > edit->Rows) {
			Error(102);
		}
	}

	/* read field list */
label2:
	ReadChar(); ReadChar();
	Lexem = CurrChar;
	Accept('_');
	FD1 = RdFileName();
	if (edit->FD == nullptr) edit->FD = FD1;
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

	if (edit->FirstFld == nullptr) edit->FirstFld = D;
	else ChainLast(edit->FirstFld, D);

	if ((D1 != nullptr) && (D->ScanNr == D1->ScanNr)) Error(77);
	F = RdFldName(edit->FD);
	D->FldD = F;
	edit->Flds.push_back(F);
	if (Lexem == ',') { RdLex(); goto label3; }
	TestLex(';');
	SkipBlank(true);
	/* read record lines */
	D = edit->FirstFld;
	NPages = 0;
label4:
	NPages++; Ln = 0; NFlds = 0; SLRoot = nullptr;
label5:
	s = ""; Ln++;
	Col = edit->FrstCol;
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
			if (Col > edit->LastCol) Error(102);
			D = D->pChain;
		}
		else {
			if (!screen.SetStyleAttr(ForwChar, a)) {
				if (Col > edit->LastCol) Error(102);
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
		if ((c == '\\') || (edit->NHdTxt + Ln == edit->Rows)) {
			StoreRT(Ln, SLRoot, NFlds);
			goto label4;
		}
		else goto label5;
	}
	StoreRT(Ln, SLRoot, NFlds);
	edit->NPages = NPages;

	if (D != nullptr) Error(30);
	D = FindScanNr(1);
	D->ChainBack = nullptr;
	for (i = 2; i <= N; i++) {
		PrevD = D;
		D = FindScanNr(D->ScanNr + 1);
		D->ChainBack = PrevD;
	}
	edit->LastFld = D;
	PrevD = nullptr;
	while (D != nullptr) {
		D->pChain = PrevD;
		PrevD = D;
		D = D->ChainBack;
	}
	edit->FirstFld = PrevD;
}

EditD* EditReader::GetEditD()
{
	return edit_;
}

EFldD* EditReader::FindScanNr(WORD N)
{
	EFldD* D = edit_->FirstFld;
	EFldD* D1 = nullptr;
	WORD M = 0xffff;
	while (D != nullptr) {
		if ((D->ScanNr >= N) && (D->ScanNr < M)) { M = D->ScanNr; D1 = D; }
		D = D->pChain;
	}
	return D1;
}

void EditReader::AutoDesign(FieldListEl* FL)
{
	WORD L = 0, i = 0, m = 0, FldLen = 0;
	pstring s = "";
	StringListEl* SLRoot = nullptr;
	EFldD* D = (EFldD*)(&edit_->FirstFld);
	EFldD* PrevD = nullptr;
	WORD NPages = 1; WORD Ln = 0;
	WORD Col = edit_->FrstCol;
	WORD maxcol = edit_->LastCol - edit_->FrstCol;
	while (FL != nullptr) {
		FieldDescr* F = FL->FldD;
		FL = FL->pChain;
		if (F == nullptr) continue; // tady to padalo na 1. polozce, protoze ta ma FldD = nullptr
		
		D->pChain = new EFldD();
		D = D->pChain;
		D->ChainBack = PrevD;
		PrevD = D;
		D->FldD = F;
		D->L = F->L;
		if (D->L > maxcol) D->L = maxcol;
		if ((edit_->FD->FF->file_type == FileType::CAT) && (D->L > 44)) D->L = 44; /*catalog pathname*/
		FldLen = D->L;
		if (F->field_type == FieldType::TEXT) D->L = 1;
		L = F->Name.length();
		if (FldLen > L) L = FldLen;
		if (Col + L > edit_->LastCol) {
			SToSL(&SLRoot, s);
			SToSL(&SLRoot, "");
			Ln += 2;
			if (Ln + 2 > edit_->Rows) {
				StoreRT(Ln, SLRoot, 1);
				NPages++;
				Ln = 0;
				SLRoot = nullptr;
			}
			Col = edit_->FrstCol;
			s = "";
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
	edit_->LastFld = D;
	edit_->NPages = NPages;
	if (NPages == 1) { /* !!! with E->RecTxt^ do!!! */
		auto& er = *edit_->RecTxt;
		if (er.N == 2) {
			edit_->HdTxt = er.SL;
			er.SL = er.SL->pChain;
			edit_->HdTxt->pChain = nullptr;
			edit_->NHdTxt = 1;
			er.N = 1;
			D = edit_->FirstFld;
			while (D != nullptr) {
				D->Ln--;
				D = D->pChain;
			}
			if (edit_->Rows == 1) {
				edit_->NHdTxt = 0;
				edit_->HdTxt = nullptr;
			}
		}
		else if (er.N < edit_->Rows) {
			s = "";
			for (i = edit_->FrstCol; i <= edit_->LastCol; i++) s.Append('-');
			SToSL(&er.SL, s);
			er.N++;
		}
	}
}

void EditReader::AutoDesign(std::vector<FieldDescr*>& FL)
{
	WORD L = 0, i = 0, m = 0, FldLen = 0;
	pstring s = "";
	StringListEl* SLRoot = nullptr;
	EFldD* D = (EFldD*)(&edit_->FirstFld);  // TODO: this is not correct, but it works -> refactor!
	EFldD* PrevD = nullptr;
	WORD NPages = 1; WORD Ln = 0;
	WORD Col = edit_->FrstCol;
	WORD maxcol = edit_->LastCol - edit_->FrstCol;
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
		if ((edit_->FD->FF->file_type == FileType::CAT) && (D->L > 44)) D->L = 44; /*catalog pathname*/
		FldLen = D->L;
		if (F->field_type == FieldType::TEXT) D->L = 1;
		L = F->Name.length();
		if (FldLen > L) L = FldLen;
		if (Col + L > edit_->LastCol) {
			SToSL(&SLRoot, s);
			SToSL(&SLRoot, "");
			Ln += 2;
			if (Ln + 2 > edit_->Rows) {
				StoreRT(Ln, SLRoot, 1);
				NPages++;
				Ln = 0;
				SLRoot = nullptr;
			}
			Col = edit_->FrstCol;
			s = "";
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
	edit_->LastFld = D;
	edit_->NPages = NPages;
	if (NPages == 1) {
		auto& er = *edit_->RecTxt;
		if (er.N == 2) {
			edit_->HdTxt = er.SL;
			er.SL = er.SL->pChain;
			edit_->HdTxt->pChain = nullptr;
			edit_->NHdTxt = 1;
			er.N = 1;
			D = edit_->FirstFld;
			while (D != nullptr) {
				D->Ln--;
				D = D->pChain;
			}
			if (edit_->Rows == 1) {
				edit_->NHdTxt = 0;
				edit_->HdTxt = nullptr;
			}
		}
		else if (er.N < edit_->Rows) {
			s = "";
			for (i = edit_->FrstCol; i <= edit_->LastCol; i++) s.Append('-');
			SToSL(&er.SL, s);
			er.N++;
		}
	}
}

void EditReader::RdFormOrDesign(std::vector<FieldDescr*>& FL, RdbPos FormPos)
{
	edit_->FrstCol = edit_->V.C1;
	edit_->FrstRow = edit_->V.R1;
	edit_->LastCol = edit_->V.C2;
	edit_->LastRow = edit_->V.R2;
	if ((edit_->WFlags & WHasFrame) != 0) {
		edit_->FrstCol++; edit_->LastCol--;
		edit_->FrstRow++; edit_->LastRow--;
	}
	edit_->Rows = edit_->LastRow - edit_->FrstRow + 1;
	if (FL.empty()) {
		ResetCompilePars();
		RdEForm(edit_, FormPos);
		edit_->IsUserForm = true;
	}
	else {
		//E->FD = F;
		edit_->Flds = FL;
		AutoDesign(FL);
	}
}

std::string GetStr_E(FrmlElem* Z)
{
	if (Z == nullptr) return "";
	else {
		std::string s = RunShortStr(CFile, Z, CRecPtr);
		//while (GetLengthOfStyledString(s) > TxtCols) {
		//	// smaz posledni znak z retezce
		//	s.erase(s.length() - 1);
		//}
		s = GetStyledStringOfLength(s, 0, TxtCols);
		return s;
	}
}

void EditReader::NewEditD(FileD* file_d, EditOpt* EO)
{
	edit_->FD = file_d;

	WORD i = 0;
	FieldDescr* F = nullptr;
	bool b = false, b2 = false, F2NoUpd = false;

	// TODO: remove PushEdit();
	PushEdit();

	// replace for PushEdit:
	edit_->V.C1 = 1;
	edit_->V.R1 = 2;
	edit_->V.C2 = TxtCols;
	edit_->V.R2 = TxtRows - 1;

	// move je nahrazen kopirovanim jednotlivych polozek:
	edit_->WFlags = EO->WFlags;
	edit_->ExD = EO->ExD;
	edit_->Journal = EO->Journal;
	edit_->ViewName = EO->ViewName;
	edit_->OwnerTyp = EO->OwnerTyp;
	edit_->DownLD = EO->DownLD;
	edit_->DownLV = EO->DownLV;
	edit_->DownRecPtr = EO->DownRecPtr;
	edit_->LVRecPtr = EO->LVRecPtr;
	edit_->KIRoot = EO->KIRoot;
	edit_->SQLFilter = EO->SQLFilter;
	edit_->SelKey = static_cast<XWKey*>(EO->SelKey);
	//rectxt

	edit_->Attr = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZAttr, screen.colors.dTxt, CRecPtr));
	edit_->dNorm = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdNorm, screen.colors.dNorm, CRecPtr));
	edit_->dHiLi = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdHiLi, screen.colors.dHili, CRecPtr));
	edit_->dSubSet = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdSubset, screen.colors.dSubset, CRecPtr));
	edit_->dDel = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdDel, screen.colors.dDeleted, CRecPtr));
	edit_->dTab = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdTab, edit_->Attr | 0x08, CRecPtr));
	edit_->dSelect = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdSelect, screen.colors.dSelect, CRecPtr));
	edit_->Top = RunStdStr(file_d, EO->Top, CRecPtr);
	if (EO->Mode != nullptr) {
		std::string mode = RunShortStr(file_d, EO->Mode, CRecPtr);
		int result = edit_->params_->SetFromString(mode, false);
		if (result != 0) {
			Error(92);
		}
	}
	if (spec.Prompt158) edit_->params_->Prompt158 = true;
	if (EO->SetOnlyView /*UpwEdit*/) {
		EO->Tab.clear();
		edit_->params_->OnlyTabs = true;
		edit_->params_->OnlySearch = false;
	}
	if (edit_->LVRecPtr != nullptr) {
		edit_->params_->EdRecVar = true;
		edit_->params_->Only1Record = true;
	}
	if (edit_->params_->Only1Record) {
		edit_->params_->OnlySearch = false;
	}
	if (EO->W.C1 != nullptr) {
		RunWFrml(file_d, EO->W, edit_->WFlags, edit_->V, CRecPtr);
		edit_->WwPart = true;
		if ((edit_->WFlags & WShadow) != 0) {
			edit_->ShdwX = MinW(2, TxtCols - edit_->V.C2);
			edit_->ShdwY = MinW(1, TxtRows - edit_->V.R2);
		}
	}
	else {
		if (edit_->params_->WithBoolDispl) {
			edit_->V.R1 = 3;
		}
		if (edit_->params_->Mode24) {
			edit_->V.R2--;
		}
	}
	RdFormOrDesign(EO->Flds, EO->FormPos);
	if (edit_->NPages > 1) {
		edit_->NRecs = 1;
	}
	else {
		edit_->NRecs = (edit_->Rows - edit_->NHdTxt) / edit_->RecTxt->N;
	}
	edit_->BaseRec = 1;
	edit_->IRec = 1;
	edit_->CFld = edit_->FirstFld;
	edit_->FirstEmptyFld = edit_->FirstFld;
	edit_->params_->ChkSwitch = true;
	edit_->params_->WarnSwitch = true;

	edit_->OldRecPtr = edit_->FD->GetRecSpace();
	uint8_t* record = edit_->OldRecPtr;

#ifdef FandSQL
	if (file_d->IsSQLFile) SetTWorkFlag;
#endif
	if (edit_->params_->EdRecVar) {
		edit_->NewRecPtr = (uint8_t*)edit_->LVRecPtr;
		edit_->params_->NoDelete = true;
		edit_->params_->NoCreate = true;
		edit_->Journal = nullptr;
		edit_->KIRoot = nullptr;
	}
	else {
		edit_->NewRecPtr = file_d->GetRecSpace();
		record = edit_->NewRecPtr;
#ifdef FandSQL
		if (file_d->IsSQLFile) SetTWorkFlag;
#endif
		edit_->params_->AddSwitch = true;
		edit_->Cond = RunEvalFrml(file_d, EO->Cond, record);
		edit_->RefreshDelay = RunWordImpl(file_d, EO->RefreshDelayZ, spec.RefreshDelay, record) * 1000;
		edit_->SaveAfter = RunWordImpl(file_d, EO->SaveAfterZ, spec.UpdCount, record) * 1000;
		if (EO->StartRecKeyZ != nullptr) {
			edit_->StartRecKey = RunShortStr(file_d, EO->StartRecKeyZ, record);
		}
		edit_->StartRecNo = RunInt(file_d, EO->StartRecNoZ, record);
		edit_->StartIRec = RunInt(file_d, EO->StartIRecZ, record);
		edit_->VK = EO->ViewKey;
		if (edit_->DownLD != nullptr) {
			edit_->DownSet = true;
			edit_->DownKey = GetFromKey(edit_->DownLD);
			if (edit_->VK == nullptr) {
				edit_->VK = edit_->DownKey;
			}
			switch (edit_->OwnerTyp) {
			case 'r': {
				edit_->DownRecPtr = edit_->DownLV->record;
				break;
			}
			case 'F': {
				edit_->OwnerRecNo = RunInt(file_d, (FrmlElem*)EO->DownLV, record);
				edit_->DownRecPtr = edit_->DownLD->ToFD->GetRecSpace();
				break;
			}
			default:;
			}
		}
		else if (edit_->VK == nullptr) {
			edit_->VK = edit_->FD->Keys.empty() ? nullptr : edit_->FD->Keys[0];
		}
#ifdef FandSQL
		if (file_d->IsSQLFile && (E->VK = nullptr)) { SetMsgPar(file_d->Name); RunError(652); }
#endif
		if (edit_->SelKey != nullptr) {
			if (edit_->SelKey->KFlds == nullptr) {
				edit_->SelKey->KFlds = edit_->VK->KFlds;
			}
			else if (!KeyFldD::EquKFlds(edit_->SelKey->KFlds, edit_->VK->KFlds)) {
				RunError(663);
			}
		}
	}
	if (EO->StartFieldZ != nullptr) {
		std::string rss = RunShortStr(file_d, EO->StartFieldZ, record);
		std::string s = TrailChar(rss, ' ');
		EFldD* D = edit_->FirstFld;
		while (D != nullptr) {
			if (EquUpCase(D->FldD->Name, s)) {
				edit_->StartFld = D;
			}
			D = D->pChain;
		}
	}
	edit_->WatchDelay = RunInt(file_d, EO->WatchDelayZ, record) * 1000;
	if (EO->Head == nullptr) {
		edit_->Head = StandardHead(edit_);
	}
	else {
		edit_->Head = GetStr_E(EO->Head);
	}
	edit_->Last = GetStr_E(EO->Last);
	edit_->AltLast = GetStr_E(EO->AltLast);
	edit_->CtrlLast = GetStr_E(EO->CtrlLast);
	edit_->ShiftLast = GetStr_E(EO->ShiftLast);
	F2NoUpd = edit_->params_->OnlyTabs && EO->Tab.empty() && !EO->NegTab && edit_->params_->OnlyAppend;

	EFldD* D = edit_->FirstFld;
	while (D != nullptr) {
		edit_->NFlds++;
		F = D->FldD;
		b = FieldInList(F, EO->Tab);
		if (EO->NegTab) b = !b;
		if (b) { D->Tab = true; edit_->NTabsSet++; }
		b2 = FieldInList(F, EO->NoEd);
		if (EO->NegNoEd) b2 = !b2;
		D->EdU = !(b2 || edit_->params_->OnlyTabs && !b);
		D->EdN = F2NoUpd;
		if (((F->Flg & f_Stored) != 0) && D->EdU) edit_->NEdSet++;
		b = FieldInList(F, EO->Dupl);
		if (EO->NegDupl) b = !b;
		if (b && ((F->Flg & f_Stored) != 0)) D->Dupl = true;
		if (b || ((F->Flg & f_Stored) != 0)) edit_->NDuplSet++;
		D = D->pChain;
	}
	if (edit_->params_->OnlyTabs && (edit_->NTabsSet == 0)) {
		edit_->params_->NoDelete = true;
		if (!edit_->params_->OnlyAppend) {
			edit_->params_->NoCreate = true;
		}
	}
	RdDepChkImpl(edit_);
	NewChkKey(file_d);
	MarkStore(edit_->AfterE);
}

EFldD* EditReader::FindEFld_E(FieldDescr* F)
{
	EFldD* D = edit_->FirstFld;
	while (D != nullptr) {
		if (D->FldD == F) break;
		D = D->pChain;
	}
	return D;
}

void EditReader::ZeroUsed()
{
	EFldD* D = edit_->FirstFld;
	while (D != nullptr) {
		D->Used = false;
		D = D->pChain;
	}
}

EFldD* EditReader::LstUsedFld()
{
	EFldD* D = edit_->LastFld;
	while (D != nullptr) {
		if (D->Used) break;
		D = D->ChainBack;
	}
	return D;
}

void EditReader::RdDepChkImpl(EditD* edit)
{
	std::string s;
	switch (edit->FD->FF->file_type) {
	case FileType::RDB: {
		ReadMessage(53);
		s = MsgLine;
		ResetCompilePars();
		SetInpStr(s);
		RdUDLI(edit->FD);
		break;
	}
	case FileType::CAT: {
		ReadMessage(54);
		s = MsgLine;
		if (spec.CPMdrive != ' ') s = s + ',' + spec.CPMdrive + ':';
		ReadMessage(55);
		s = s + MsgLine;
		if (spec.CPMdrive != ' ') s = s + ',' + spec.CPMdrive + ':';
		s = s + "'";
		ResetCompilePars();
		SetInpStr(s);
		RdUDLI(edit->FD);
		break;
	}
	default: {
		RdAllUDLIs(edit->FD);
		break;
	}
	}
}

void EditReader::TestedFlagOff()
{
	for (auto& F : CFile->FldD) {
		F->field_flag = false;
	}
}

void EditReader::SetFrmlFlags(FrmlElem* Z)
{
	auto iZ0 = (FrmlElemFunction*)Z;
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

void EditReader::SetFlag(FieldDescr* F)
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

void EditReader::RdDep(FileD* file_d)
{
	FrmlElem* Bool = nullptr;
	FrmlElem* Z = nullptr;
	EFldD* D = nullptr;
	char FTyp = '\0';
	DepD* Dp = nullptr;

	RdLex();
label1:
	Accept('(');
	Bool = RdBool(nullptr);
	Accept(')');
label2:
	D = FindEFld_E(RdFldName(file_d));
	Accept(_assign);
	Z = RdFrml(FTyp, nullptr);
	if (D != nullptr) {
		Dp = new DepD();
		Dp->Bool = Bool;
		Dp->Frml = Z;
		D->Dep.push_back(Dp);
	}
	if (Lexem == ';') {
		RdLex();
		if (!(Lexem == '#' || Lexem == 0x1A))
		{
			if (Lexem == '(') goto label1;
			else goto label2;
		}
	}
}

void EditReader::RdCheck()
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
		else {
			delete C; C = nullptr;
		}
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

void EditReader::RdImpl(FileD* file_d)
{
	// TODO: nema byt FTyp vstupnim parametrem?
	char FTyp = '\0';
	RdLex();
	while (true) {
		FieldDescr* F = RdFldName(file_d);
		Accept(_assign);
		FrmlElem* Z = RdFrml(FTyp, nullptr);
		EFldD* D = FindEFld_E(F);
		if (D != nullptr) D->Impl = Z;
		else {
			ImplD* ID = new ImplD(); // (ImplD*)GetStore(sizeof(*ID));
			ID->FldD = F;
			ID->Frml = Z;
			if (edit_->Impl == nullptr) edit_->Impl = ID;
			else ChainLast(edit_->Impl, ID);
		}
		if (Lexem == ';') {
			RdLex();
			if (!(Lexem == '#' || Lexem == 0x1A)) continue;
		}
		break;
	}
}

void EditReader::RdUDLI(FileD* file_d)
{
	CFile = file_d; // to be sure

	RdLex();
	if ((Lexem == '#') && (ForwChar == 'U')) {
		do {
			RdLex();
		} while (!(Lexem == '#' || Lexem == 0x1A));
	}
	if ((Lexem == '#') && (ForwChar == 'D')) {
		RdLex();
		RdDep(file_d);
	}
	if ((Lexem == '#') && (ForwChar == 'L')) {
		RdLex();
		RdCheck();
	}
	if ((Lexem == '#') && (ForwChar == 'I')) {
		RdLex();
		RdImpl(file_d);
	}
}

void EditReader::RdAllUDLIs(FileD* FD)
{
	RdbD* r = nullptr;
	if (FD->OrigFD != nullptr) {
		// this rdb_file was created as 'LIKE'
		RdAllUDLIs(FD->OrigFD);
	}
	if (FD->TxtPosUDLI != 0) {
		ResetCompilePars();
		SetInpTTxtPos(FD);
		r = CRdb;
		CRdb = FD->ChptPos.rdb;
		RdUDLI(FD);
		CRdb = r;
	}
}

std::string EditReader::StandardHead(EditD* edit)
{
	std::string s;
	std::string c = "          ______                                 __.__.____";
	if (!edit->ViewName.empty()) s = edit->ViewName;
	else if (edit->params_->EdRecVar) s = "";
	else {
		s = edit->FD->Name;
		switch (edit->FD->FF->file_type) {
		case FileType::INDEX: {
			if (!edit->VK->Alias.empty()) s = s + "/" + edit->VK->Alias;
			break;
		}
		case FileType::RDB: s += ".RDB"; break;
		case FileType::FAND8: s += ".DTA"; break;
		}
	}
	s = s.substr(0, 16); // max. length is 16 chars
	s = c.substr(16, 20 - s.length()) + s + c;
	return s;
}

void EditReader::NewChkKey(FileD* file_d)
{
	//XKey* K = CFile->Keys;
	KeyFldD* KF = nullptr;
	EFldD* D = nullptr;
	KeyListEl* KL = nullptr;
	for (auto& K : file_d->Keys) {
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
	}
}
