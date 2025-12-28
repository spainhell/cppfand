#include "EditReader.h"

#include <algorithm>
#include "Dependency.h"
#include "EditableField.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"
#include "../Core/LogicControl.h"
#include "../Core/Compiler.h"
#include "../Common/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../fandio/KeyFldD.h"
#include "../Common/LinkD.h"
#include "../Core/obaseww.h"
#include "../Core/rdfildcl.h"
#include "../Core/rdrun.h"
#include "../Core/runfrml.h"
#include "../fandio/XWKey.h"
#include "../Common/Record.h"

//std::vector<EditD*> v_edits;

EditReader::EditReader()
{
	edit_ = new EditD(TxtCols, TxtRows);
}

//void PushEdit()
//{
//	EditD* const e1 = new EditD();
//	e1->V.C1 = 1;
//	e1->V.R1 = 2;
//	e1->V.C2 = TxtCols;
//	e1->V.R2 = TxtRows - 1;
//
//	v_edits.push_back(e1);
//}

//void EditReader::SToSL(StringListEl** SLRoot, pstring s)
//{
//	StringListEl* SL = new StringListEl();
//	SL->S = s;
//	if (*SLRoot == nullptr) *SLRoot = SL;
//	else ChainLast(*SLRoot, SL);
//}

void EditReader::StoreRT(WORD Ln, std::vector<std::string>& SL, WORD NFlds)
{
	ERecTxtD* RT = new ERecTxtD();
	if (NFlds == 0) {
		gc->Error(81);
	}
	RT->N = Ln;
	RT->SL = SL;
	//if (edit_->RecTxt == nullptr) edit_->RecTxt = RT;
	//else ChainLast(edit_->RecTxt, RT);
	edit_->RecTxt.push_back(RT);
}

void EditReader::RdEForm(EditD* edit, RdbPos FormPos)
{
	//EditableField* D = nullptr;
	//EditableField* D1 = nullptr;
	EditableField* PrevD = nullptr;
	FieldDescr* F = nullptr;
	std::vector<std::string> SLRoot;
	std::string s;
	WORD NPages = 0, Col = 0, Ln = 0, Max = 0, M = 0, N = 0, NFlds = 0, i = 0;
	bool comment = false; char c = '\0'; uint8_t a = 0;
	gc->SetInpTT(&FormPos, true);

	bool skipAfterHash = false;
	while (true) {
		s = "";
		while (!(gc->ForwChar == '#' || gc->ForwChar == 0x1A || gc->ForwChar == 0x0D || gc->ForwChar == '{')) {
			/* read headlines */
			s += static_cast<char>(gc->ForwChar);
			gc->ReadChar();
		}
		switch (gc->ForwChar) {
		case 0x1A: {
			gc->Error(76);
			break;
		}
		case '#': {
			skipAfterHash = true;
			break;
		}
		case '{': {
			gc->SkipBlank(true);
			continue;
			break;
		}
		}

		if (skipAfterHash) {
			break;
		}
		else {
			gc->ReadChar();
			if (gc->ForwChar == 0x0A) gc->ReadChar();
			//SToSL(&edit->HdTxt, s);
			edit->HdTxt.push_back(s);
			edit->NHdTxt++;
			if (edit->NHdTxt + 1 > edit->Rows) {
				gc->Error(102);
			}
		}
	}

	/* read field list */
	gc->ReadChar();
	gc->ReadChar();
	gc->Lexem = gc->CurrChar;
	gc->Accept('_');
	FileD* FD1 = gc->RdFileName();
	if (edit->FD == nullptr) {
		edit->FD = FD1;
	}

	while (true) {
		N++;
		EditableField* D = new EditableField();
		if (gc->Lexem == _number) {
			M = gc->RdInteger();
			if (M == 0) gc->OldError(115);
			gc->Accept(':');
			D->ScanNr = M;
		}
		else {
			D->ScanNr = N;
		}

		EditableField* D1 = FindScanNr(D->ScanNr);

		//if (edit->FirstFld == nullptr) edit->FirstFld = D;
		//else ChainLast(edit->FirstFld, D);
		edit->FirstFld.push_back(D);

		if ((D1 != nullptr) && (D->ScanNr == D1->ScanNr)) {
			gc->Error(77);
		}
		F = gc->RdFldName(edit->FD);
		D->FldD = F;
		edit->Flds.push_back(F);
		if (gc->Lexem == ',') {
			gc->RdLex();
			continue;
		}
		break;
	}

	gc->TestLex(';');
	gc->SkipBlank(true);
	/* read record lines */
	std::vector<EditableField*>::iterator D = edit->FirstFld.begin();
	NPages = 0;
	NPages++;
	Ln = 0;
	NFlds = 0;
	SLRoot.clear();

	while (true) {
		s = "";
		Ln++;
		Col = edit->FrstCol;
		while (!(gc->ForwChar == 0x0D || gc->ForwChar == 0x1A || gc->ForwChar == '\\' || gc->ForwChar == '{')) {
			if (gc->ForwChar == '_') {
				if (D == edit->FirstFld.end()) {
					gc->Error(30);
				}
				NFlds++;
				(*D)->Col = Col;
				(*D)->Ln = Ln;
				(*D)->Page = NPages;
				M = 0;
				while (gc->ForwChar == '_') {
					s += ' ';
					M++; Col++;
					gc->ReadChar();
				}
				F = (*D)->FldD;
				(*D)->L = F->L;
				if (F->field_type == FieldType::TEXT) {
					(*D)->L = 1;
				}
				if ((F->field_type == FieldType::ALFANUM) && (M < F->L)) {
					(*D)->L = M;
				}
				else if (M != (*D)->L) {
					str((*D)->L, 2, s);
					SetMsgPar(s, F->Name);
					gc->Error(79);
				}
				if (Col > edit->LastCol) gc->Error(102);
				++D; //D = D->pChain;
			}
			else {
				if (!screen.SetStyleAttr(gc->ForwChar, a)) {
					if (Col > edit->LastCol) {
						gc->Error(102);
					}
					Col++;
				}
				s += static_cast<char>(gc->ForwChar);
				gc->ReadChar();
			}
		}

		//SToSL(&SLRoot, s);
		SLRoot.push_back(s);
		c = gc->ForwChar;
		if (c == '\\') gc->ReadChar();
		gc->SkipBlank(true);
		if (gc->ForwChar != 0x1A) {
			if ((c == '\\') || (edit->NHdTxt + Ln == edit->Rows)) {
				StoreRT(Ln, SLRoot, NFlds);
				NPages++;
				Ln = 0;
				NFlds = 0;
				SLRoot.clear();
				continue;
			}
			else {
				continue;
			}
		}
		break;
	}
	StoreRT(Ln, SLRoot, NFlds);
	edit->NPages = NPages;

	if (D != edit->FirstFld.end()) {
		gc->Error(30);
	}
	// sort items in FirstFld by ScanNr
	std::ranges::sort(edit->FirstFld, [](const EditableField* a, const EditableField* b) {
		return a->ScanNr < b->ScanNr;
		});
	//D = FindScanNr(1);
	//(*D)->ChainBack = nullptr;
	//for (i = 2; i <= N; i++) {
	//	PrevD = (*D);
	//	D = FindScanNr((*D)->ScanNr + 1);
	//	(*D)->ChainBack = PrevD;
	//}
	edit->LastFld = edit->FirstFld.back();
	//PrevD = nullptr;
	//while (D != edit->FirstFld.end()) {
	//	D->pChain = PrevD;
	//	PrevD = D;
	//	D = D->ChainBack;
	//}
	//edit->FirstFld = PrevD;
}

EditD* EditReader::GetEditD()
{
	return edit_;
}

EditableField* EditReader::FindScanNr(WORD N)
{
	std::vector<EditableField*>::iterator D = edit_->FirstFld.begin();
	EditableField* result = nullptr;
	WORD M = 0xffff;
	while (D != edit_->FirstFld.end()) {
		if (((*D)->ScanNr >= N) && ((*D)->ScanNr < M)) {
			M = (*D)->ScanNr;
			result = *D;
		}
		++D; // = D->pChain;
	}
	return result;
}

void EditReader::AutoDesign(std::vector<FieldDescr*>& FL)
{
	WORD L = 0, i = 0, m = 0, FldLen = 0;
	pstring s = "";
	std::vector<std::string> SLRoot;
	WORD NPages = 1; WORD Ln = 0;
	WORD Col = edit_->FrstCol;
	WORD maxcol = edit_->LastCol - edit_->FrstCol;

	for (FieldDescr* F : FL) {
		//if (F == nullptr) continue;
		EditableField* newD = new EditableField();
		edit_->FirstFld.push_back(newD);
		newD->FldD = F;
		newD->L = F->L;
		if (newD->L > maxcol) {
			newD->L = maxcol;
		}
		if (edit_->FD->FileType == DataFileType::FandFile 
			&& edit_->FD->FF->file_type == FandFileType::CAT 
			&& newD->L > 44) {
			// catalog pathname
			newD->L = 44;
		}
		FldLen = newD->L;
		if (F->field_type == FieldType::TEXT) {
			newD->L = 1;
		}
		L = F->Name.length();
		if (FldLen > L) {
			L = FldLen;
		}
		if (Col + L > edit_->LastCol) {
			SLRoot.push_back(s);
			SLRoot.push_back("");
			Ln += 2;
			if (Ln + 2 > edit_->Rows) {
				StoreRT(Ln, SLRoot, 1);
				NPages++;
				Ln = 0;
				SLRoot.clear();
			}
			Col = edit_->FrstCol;
			s = "";
		}
		m = (L - F->Name.length() + 1) / 2;
		for (i = 1; i <= m; i++) {
			s.Append(' ');
		}
		s = s + F->Name;
		m = L - F->Name.length() - m;
		for (i = 1; i <= m + 1; i++) {
			s.Append(' ');
		}
		newD->Col = Col + (L - FldLen + 1) / 2;
		newD->Ln = Ln + 2;
		newD->Page = NPages;
		Col += (L + 1);
	}
	SLRoot.push_back(s);
	SLRoot.push_back("");
	Ln += 2;
	StoreRT(Ln, SLRoot, 1);
	edit_->LastFld = edit_->FirstFld.back();
	edit_->NPages = NPages;
	if (NPages == 1) {
		ERecTxtD* er = edit_->RecTxt[0];
		if (er->N == 2) {
			edit_->HdTxt.clear();
			edit_->HdTxt.push_back(er->SL[0]);
			edit_->NHdTxt = 1;

			er->SL.erase(er->SL.begin());
			er->N = 1;

			for (EditableField* D : edit_->FirstFld) {
				D->Ln--;
			}

			if (edit_->Rows == 1) {
				edit_->NHdTxt = 0;
				edit_->HdTxt.clear();
			}
		}
		else if (er->N < edit_->Rows) {
			s = "";
			for (i = edit_->FrstCol; i <= edit_->LastCol; i++) {
				s.Append('-');
			}
			er->SL.push_back(s);
			er->N++;
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
		edit_->FrstCol++;
		edit_->LastCol--;
		edit_->FrstRow++;
		edit_->LastRow--;
	}

	edit_->Rows = edit_->LastRow - edit_->FrstRow + 1;

	if (FL.empty()) {
		gc->ResetCompilePars();
		RdEForm(edit_, FormPos);
		edit_->IsUserForm = true;
	}
	else {
		edit_->Flds = FL;
		AutoDesign(FL);
	}
}

std::string EditReader::GetStr_E(FrmlElem* Z, Record* record)
{
	if (Z == nullptr) return "";
	else {
		std::string s = RunString(file_d_, Z, record);
		//while (GetLengthOfStyledString(s) > TxtCols) {
		//	// smaz posledni znak z retezce
		//	s.erase(s.length() - 1);
		//}
		s = GetStyledStringOfLength(s, 0, TxtCols);
		return s;
	}
}

void EditReader::NewEditD(FileD* file_d, EditOpt* EO, Record* rec)
{
	file_d_ = file_d;
	edit_->FD = file_d;

	WORD i = 0;
	FieldDescr* F = nullptr;
	bool b = false, b2 = false, F2NoUpd = false;

	// TODO: remove PushEdit();
	//PushEdit();

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
	edit_->DownRecord = EO->DownRecord == nullptr ? nullptr : EO->DownRecord->Clone(); // TODO: opravdu clone? neni to hodnota predana odkazem do procedury?
	edit_->LvRec = EO->LvRec;
	edit_->KIRoot = EO->KIRoot;
	edit_->SQLFilter = EO->SQLFilter;
	edit_->SelKey = static_cast<XWKey*>(EO->SelKey);
	//rectxt

	edit_->Attr = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZAttr, screen.colors.dTxt, rec));
	edit_->dNorm = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdNorm, screen.colors.dNorm, rec));
	edit_->dHiLi = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdHiLi, screen.colors.dHili, rec));
	edit_->dSubSet = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdSubset, screen.colors.dSubset, rec));
	edit_->dDel = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdDel, screen.colors.dDeleted, rec));
	edit_->dTab = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdTab, edit_->Attr | 0x08, rec));
	edit_->dSelect = static_cast<uint8_t>(RunWordImpl(file_d, EO->ZdSelect, screen.colors.dSelect, rec));
	edit_->Top = RunString(file_d, EO->Top, rec);
	if (EO->Mode != nullptr) {
		std::string mode = RunString(file_d, EO->Mode, rec);
		int result = edit_->params_->SetFromString(mode, false);
		if (result != 0) {
			gc->Error(92);
		}
	}
	if (spec.Prompt158) edit_->params_->Prompt158 = true;
	if (EO->SetOnlyView /*UpwEdit*/) {
		EO->Tab.clear();
		edit_->params_->OnlyTabs = true;
		edit_->params_->OnlySearch = false;
	}
	if (edit_->LvRec != nullptr) {
		edit_->params_->EdRecVar = true;
		edit_->params_->Only1Record = true;
	}
	if (edit_->params_->Only1Record) {
		edit_->params_->OnlySearch = false;
	}
	if (EO->W.C1 != nullptr) {
		RunWFrml(file_d, EO->W, edit_->WFlags, edit_->V, rec);
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
		edit_->NRecs = (edit_->Rows - edit_->NHdTxt) / edit_->RecTxt[0]->N;
	}
	edit_->BaseRec = 1;
	edit_->IRec = 1;
	edit_->CFld = edit_->FirstFld.begin();
	edit_->FirstEmptyFld = edit_->FirstFld.front();
	edit_->params_->ChkSwitch = true;
	edit_->params_->WarnSwitch = true;

	//edit_->OldRec = new Record(edit_->FD);
	//uint8_t* record = edit_->OldRec->GetRecord();
	Record* record = new Record(edit_->FD);

#ifdef FandSQL
	if (file_d->IsSQLFile) { SetTWorkFlag; }
#endif
	if (edit_->params_->EdRecVar) {
		//edit_->NewRec = new Record(edit_->FD, (uint8_t*)edit_->LVRecPtr);
		edit_->params_->NoDelete = true;
		edit_->params_->NoCreate = true;
		edit_->Journal = nullptr;
		edit_->KIRoot.clear();
	}
	else {
		if (file_d == nullptr) {
			// if edit is run from a debug mode, no particular file is selected
			// then we have to use file from the 'E' chapter itself
			file_d = edit_->FD;
		}

		//edit_->NewRec = new Record(file_d);
		//record = edit_->NewRec->GetRecord();

#ifdef FandSQL
		if (file_d->IsSQLFile) SetTWorkFlag;
#endif

		edit_->params_->AddSwitch = true;
		edit_->Cond = RunEvalFrml(file_d, EO->Cond, record);
		edit_->RefreshDelay = RunWordImpl(file_d, EO->RefreshDelayZ, spec.RefreshDelay, record) * 1000;
		edit_->SaveAfter = RunWordImpl(file_d, EO->SaveAfterZ, spec.UpdCount, record) * 1000;
		if (EO->StartRecKeyZ != nullptr) {
			edit_->StartRecKey = RunString(file_d, EO->StartRecKeyZ, record);
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
				edit_->DownRecord = edit_->DownLV->record == nullptr 
					? nullptr : edit_->DownLV->record->Clone(); // TODO: ??? // new Record(nullptr, edit_->DownLV->record);
				break;
			}
			case 'F': {
				edit_->OwnerRecNo = RunInt(file_d, (FrmlElem*)EO->DownLV, record);
				edit_->DownRecord = new Record(edit_->DownLD->ToFD);
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
			if (edit_->SelKey->KFlds.empty()) {
				edit_->SelKey->KFlds = edit_->VK->KFlds;
			}
			else if (!KeyFldD::EquKFlds(edit_->SelKey->KFlds, edit_->VK->KFlds)) {
				RunError(663);
			}
		}
	}
	if (EO->StartFieldZ != nullptr) {
		std::string rss = RunString(file_d, EO->StartFieldZ, record);
		std::string s = TrailChar(rss, ' ');
		//EditableField* D = edit_->FirstFld;
		//while (D != nullptr) {
		//	if (EquUpCase(D->FldD->Name, s)) {
		//		edit_->StartFld = D;
		//	}
		//	D = D->pChain;
		//}
		for (EditableField* ef : edit_->FirstFld) {
			if (EquUpCase(ef->FldD->Name, s)) {
				edit_->StartFld = ef;
			}
		}
	}
	edit_->WatchDelay = RunInt(file_d, EO->WatchDelayZ, record) * 1000;
	if (EO->Head == nullptr) {
		edit_->Head = StandardHead(edit_);
	}
	else {
		edit_->Head = GetStr_E(EO->Head, record);
	}
	edit_->Last = GetStr_E(EO->Last, record);
	edit_->AltLast = GetStr_E(EO->AltLast, record);
	edit_->CtrlLast = GetStr_E(EO->CtrlLast, record);
	edit_->ShiftLast = GetStr_E(EO->ShiftLast, record);
	F2NoUpd = edit_->params_->OnlyTabs && EO->Tab.empty() && !EO->NegTab && edit_->params_->OnlyAppend;

	//EditableField* D = edit_->FirstFld;
	//while (D != nullptr) {
	for (EditableField* D : edit_->FirstFld) {
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
		//D = D->pChain;
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

EditableField* EditReader::FindEFld_E(FieldDescr* F)
{
	EditableField* result = nullptr;
	//EditableField* D = edit_->FirstFld;
	//while (D != nullptr) {
	for (EditableField* D : edit_->FirstFld) {
		if (D->FldD == F) {
			result = D;
			break;
		}
		//D = D->pChain;
	}
	return result;
}

void EditReader::ZeroUsed()
{
	//EditableField* D = edit_->FirstFld;
	//while (D != nullptr) {
	//	D->Used = false;
	//	D = D->pChain;
	//}
	for (EditableField* D : edit_->FirstFld) {
		D->Used = false;
	}
}

EditableField* EditReader::LstUsedFld()
{
	EditableField* result = nullptr;
	//EditableField* D = edit_->LastFld;
	//while (D != nullptr) {
	//	if (D->Used) break;
	//	D = D->ChainBack;
	//}
	//return D;
	std::vector<EditableField*>::reverse_iterator D = edit_->FirstFld.rbegin();
	for (; D != edit_->FirstFld.rend(); ++D) {
		if ((*D)->Used) {
			result = *D;
			break;
		}
	}
	return result;
}

void EditReader::RdDepChkImpl(EditD* edit)
{
	FileD* file_d = edit->FD;
	std::string s;

	if (file_d->FileType == DataFileType::FandFile) {
		switch (file_d->FF->file_type) {
		case FandFileType::RDB: {
			ReadMessage(53);
			s = MsgLine;
			gc->ResetCompilePars();
			gc->SetInpStr(s);
			RdUDLI(file_d);
			break;
		}
		case FandFileType::CAT: {
			ReadMessage(54);
			s = MsgLine;
			if (spec.CPMdrive != ' ') s = s + ',' + spec.CPMdrive + ':';
			ReadMessage(55);
			s = s + MsgLine;
			if (spec.CPMdrive != ' ') s = s + ',' + spec.CPMdrive + ':';
			s = s + "'";
			gc->ResetCompilePars();
			gc->SetInpStr(s);
			RdUDLI(file_d);
			break;
		}
		default: {
			RdAllUDLIs(file_d);
			break;
		}
		}
	}
	else {
		RdAllUDLIs(file_d);
	}
}

void EditReader::TestedFlagOff()
{
	for (auto& F : file_d_->FldD) {
		F->field_flag = false;
	}
}

void EditReader::SetFrmlFlags(FrmlElem* Z)
{
	auto iZ0 = (FrmlElemFunction*)Z;
	auto iZ7 = (FrmlElemAccess*)Z;
	if (Z == nullptr) return;

	switch (Z->Op) {
	case _field: {
		SetFlag(iZ7->Field);
		break;
	}
	case _access: {
		if (iZ7->Link != nullptr) {
			//KeyFldD* Arg = iZ7->Link->Args;
			//while (Arg != nullptr) {
			for (auto& arg : iZ7->Link->Args) {
				SetFlag(arg->FldD);
				//Arg = Arg->pChain;
			}
		}
		break;
	}
	case _userfunc: {
		auto frml_elems = ((FrmlElemUserFunc*)Z)->FrmlL;
		/*while (fl != nullptr) {
			SetFrmlFlags(fl->Frml);
			fl = fl->pChain;
		}*/
		for (FrmlElem* frml : frml_elems) {
			SetFrmlFlags(frml);
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
		else if (Z->Op >= 0xF0 /*&& Z->oper <= 0xFF*/) {
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
			EditableField* D = FindEFld_E(F);
			if (D != nullptr) {
				D->Used = true;
			}
		}
		else {
			SetFrmlFlags(F->Frml);
		}
	}
}

/// <summary>
/// Read dependencies - #D
/// </summary>
/// <param name="file_d"></param>
void EditReader::ReadDependencies(FileD* file_d)
{
	char FTyp = '\0';

	gc->processing_F = file_d;
	gc->RdLex();

	gc->Accept('(');
	FrmlElem* Bool = gc->RdBool(nullptr);
	gc->Accept(')');

	while (true) {
		EditableField* D = FindEFld_E(gc->RdFldName(file_d));
		gc->Accept(_assign);
		FrmlElem* Z = gc->RdFrml(FTyp, nullptr);

		if (D != nullptr) {
			Dependency* dep = new Dependency(Bool, Z);
			D->Dependencies.push_back(dep);
		}

		if (gc->Lexem == ';') {
			gc->RdLex();
			if (!(gc->Lexem == '#' || gc->Lexem == 0x1A)) {
				if (gc->Lexem == '(') {
					gc->Accept('(');
					Bool = gc->RdBool(nullptr);
					gc->Accept(')');
				}
				continue;
			}
		}
		break;
	}
}

void EditReader::RdCheck()
{
	gc->SkipBlank(false);
	size_t Low = gc->input_pos;
	gc->RdLex();
	while (true) {
		LogicControl* C = ReadLogicControl(Low);
		ZeroUsed();
		SetFrmlFlags(C->Bool);
		TestedFlagOff();
		EditableField* D = LstUsedFld();

		if (D != nullptr) {
			D->Checks.push_back(C);
		}
		else {
			delete C; C = nullptr;
		}

		if (gc->Lexem == ';') {
			gc->SkipBlank(false);
			Low = gc->input_pos;
			gc->RdLex();
			if (!(gc->Lexem == '#' || gc->Lexem == 0x1A)) continue;
		}
		break;
	}
}

void EditReader::RdImpl(FileD* file_d)
{
	// TODO: nema byt f_typ vstupnim parametrem?
	char FTyp = '\0';
	gc->RdLex();

	while (true) {
		FieldDescr* F = gc->RdFldName(file_d);
		gc->Accept(_assign);
		FrmlElem* Z = gc->RdFrml(FTyp, nullptr);
		EditableField* D = FindEFld_E(F);

		if (D != nullptr) {
			// implicit for editable field
			D->Impl = Z;
		}
		else {
			// implicit for non-editable field
			Implicit* ID = new Implicit(F, Z);
			edit_->Impl.push_back(ID);
		}

		if (gc->Lexem == ';') {
			gc->RdLex();
			if (!(gc->Lexem == '#' || gc->Lexem == 0x1A)) continue;
		}

		break;
	}
}

void EditReader::RdUDLI(FileD* file_d)
{
	// CFile = file_d; // to be sure
	gc->processing_F = file_d;

	gc->RdLex();
	if ((gc->Lexem == '#') && (gc->ForwChar == 'U')) {
		do {
			gc->RdLex();
		} while (!(gc->Lexem == '#' || gc->Lexem == 0x1A));
	}
	if ((gc->Lexem == '#') && (gc->ForwChar == 'D')) {
		gc->RdLex();
		ReadDependencies(file_d);
	}
	if ((gc->Lexem == '#') && (gc->ForwChar == 'L')) {
		gc->RdLex();
		RdCheck();
	}
	if ((gc->Lexem == '#') && (gc->ForwChar == 'I')) {
		gc->RdLex();
		RdImpl(file_d);
	}
}

void EditReader::RdAllUDLIs(FileD* FD)
{
	Project* r = nullptr;
	if (FD->OrigFD != nullptr) {
		// this v_files was created as 'LIKE'
		RdAllUDLIs(FD->OrigFD);
	}
	if (FD->TxtPosUDLI != 0) {
		gc->ResetCompilePars();
		gc->SetInpTTxtPos(FD);
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
		if (edit->FD->FileType == DataFileType::FandFile) {
			switch (edit->FD->FF->file_type) {
			case FandFileType::INDEX: {
				if (!edit->VK->Alias.empty()) s = s + "/" + edit->VK->Alias;
				break;
			}
			case FandFileType::RDB: s += ".RDB"; break;
			case FandFileType::FAND8: s += ".DTA"; break;
			default:;
			}
		}
		else {
			// other DataFileTypes don't show an extension
		}
	}
	s = s.substr(0, 16); // max. length is 16 chars
	s = c.substr(16, 20 - s.length()) + s + c;
	return s;
}

void EditReader::NewChkKey(FileD* file_d)
{
	for (XKey* K : file_d->Keys) {
		if (!K->Duplic) {
			ZeroUsed();

			for (KeyFldD* KF : K->KFlds) {
				EditableField* D = FindEFld_E(KF->FldD);
				if (D != nullptr) {
					D->Used = true;
				}
			}

			EditableField* D = LstUsedFld();

			if (D != nullptr) {
				D->KL.push_back(K);
			}
		}
	}
}
