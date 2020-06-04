#include "rdedit.h"

#include "legacy.h"
#include "obaseww.h"
#include "rdfildcl.h"
#include "runedi.h"
#include "runfrml.h"

EditD* E = EditDRoot;

void PushEdit()
{
	//EditD* E1 = (EditD*)GetZStore(sizeof(*E));
	EditD* E1 = new EditD();
	/* !!! with E1->V do!!! */
	{
		E1->V.C1 = 1; E1->V.R1 = 2; E1->V.C2 = TxtCols; E1->V.R2 = TxtRows - 1;
	}
	E1->Chain = E; 
	E = E1;
}

StringListEl* SToSL(Chained* SLRoot, pstring s)
{
	//StringList SL = (StringListEl*)GetStore(s.length() + 5);
	StringListEl* SL = new StringListEl();
	SL->S = s;
	if (SLRoot == nullptr) return SL;
	else { 
		ChainLast((Chained*)SLRoot, SL); 
		return (StringListEl*)SLRoot;
	}
}

ERecTxtD* StoreRT(WORD Ln, StringList SL, WORD NFlds)
{
	//ERecTxtD* RT = (ERecTxtD*)GetStore(sizeof(*RT));
	ERecTxtD* RT = new ERecTxtD();
	if (NFlds == 0) Error(81);
	RT->N = Ln;
	RT->SL = SL;
	if (E->RecTxt == nullptr) return RT;
	else { 
		ChainLast(E->RecTxt, RT); 
		return E->RecTxt;
	}

}

void RdEForm(FileD* ParFD, RdbPos FormPos)
{
	EFldD* D = nullptr; EFldD* D1 = nullptr; EFldD* PrevD = nullptr; 
	FieldDescr* F = nullptr; FieldListEl* FL = nullptr; 
	FileD* FD1 = nullptr;
	StringListEl* SLRoot = nullptr;
	pstring s; 
	WORD NPages = 0, Col = 0, Ln = 0, Max = 0, M = 0, N = 0, NFlds = 0, i = 0;
	bool comment = false; char c = '\0'; BYTE a = 0;
	SetInpTT(FormPos, true);
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
	SToSL(E->HdTxt, s); 
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
	//FL = (FieldListEl*)GetStore(sizeof(*FL));
	FL = new FieldListEl();
	FL->FldD = F; 
	if (E->Flds == nullptr) E->Flds = FL;
	else ChainLast(E->Flds, FL);
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
			if (D == nullptr) Error(30); NFlds++;
			D->Col = Col; D->Ln = Ln; D->Page = NPages; M = 0;
			while (ForwChar == '_') {
				s.Append(' '); 
				M++; Col++; 
				ReadChar();
			}
			F = D->FldD; D->L = F->L; 
			if (F->Typ == 'T') D->L = 1;
			if ((F->Typ == 'A') && (M < F->L)) D->L = M;
			else if (M != D->L) {
				str(D->L, 2, s); 
				Set2MsgPar(s, F->Name); 
				Error(79);
			}
			if (Col > E->LastCol) Error(102);
			D = (EFldD*)D->Chain;
		}
		else {
			if (!SetStyleAttr(ForwChar, a)) {
				if (Col > E->LastCol) Error(102); 
				Col++;
			}
			s.Append(ForwChar); 
			ReadChar();
		}
	SToSL(SLRoot, s); 
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
		D->Chain = PrevD; 
		PrevD = D; 
		D = D->ChainBack; }
	E->FirstFld = PrevD;
}

EFldD* FindScanNr(WORD N)
{
	EFldD* D = E->FirstFld; 
	EFldD* D1 = nullptr;
	WORD M = 0xffff;
	while (D != nullptr) {
		if ((D->ScanNr >= N) && (D->ScanNr < M)) { M = D->ScanNr; D1 = D; }
		D = (EFldD*)D->Chain;
	}
	return D1;
}

void AutoDesign(FieldList FL)
{
	WORD NPages = 0, Col = 0, Ln = 0, L = 0, i = 0, m = 0, FldLen = 0, maxcol = 0;
	pstring s; 
	StringListEl* SLRoot = nullptr;
	EFldD* D = nullptr; EFldD* PrevD = nullptr; FieldDescr* F = nullptr;
	D = (EFldD*)(&E->FirstFld); 
	PrevD = nullptr;
	NPages = 1; s = ""; Ln = 0; SLRoot = nullptr;
	Col = E->FrstCol;
	maxcol = E->LastCol - E->FrstCol;
	while (FL != nullptr) {
		F = FL->FldD; 
		FL = (FieldListEl*)FL->Chain;
		if (F == nullptr) continue; // tady to padalo na 1. polozce, protoze ta ma FldD = nullptr
		//D->Chain = (EFldD*)GetZStore(sizeof(*D)); 
		D->Chain = new EFldD();
		D = (EFldD*)D->Chain; 
		D->ChainBack = PrevD;
		PrevD = D;
		D->FldD = F; 
		D->L = F->L; 
		if (D->L > maxcol) D->L = maxcol;
		if ((E->FD->Typ == 'C') && (D->L > 44)) D->L = 44; /*catalog pathname*/
		FldLen = D->L; 
		if (F->Typ == 'T') D->L = 1;
		L = F->Name.length(); 
		if (FldLen > L) L = FldLen;
		if (Col + L > E->LastCol) {
			SToSL(SLRoot, s); 
			SToSL(SLRoot, ""); 
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
	SLRoot = SToSL(SLRoot, s);
	SLRoot = SToSL(SLRoot, "");
	Ln += 2; 
	E->RecTxt = StoreRT(Ln, SLRoot, 1);
	D->Chain = nullptr; 
	E->LastFld = D; 
	E->NPages = NPages;
	if (NPages == 1) { /* !!! with E->RecTxt^ do!!! */
		auto& er = *E->RecTxt;
		if (er.N == 2) {
			E->HdTxt = er.SL; 
			er.SL = (StringList)er.SL->Chain;
			E->HdTxt->Chain = nullptr;
			E->NHdTxt = 1; 
			er.N = 1; 
			D = E->FirstFld;
			while (D != nullptr) { 
				D->Ln--; 
				D = (EFldD*)D->Chain; 
			}
			if (E->Rows == 1) { 
				E->NHdTxt = 0; 
				E->HdTxt = nullptr; 
			}
		}
		else if (er.N < E->Rows) {
			s = ""; 
			for (i = E->FrstCol; i <= E->LastCol; i++) s.Append('-');
			SToSL(er.SL, s); 
			er.N++;
		}
	}
}

void RdFormOrDesign(FileD* F, FieldList FL, RdbPos FormPos)
{
	/* !!! with E^ do!!! */
	E->FrstCol = E->V.C1; E->FrstRow = E->V.R1; E->LastCol = E->V.C2; E->LastRow = E->V.R2;
	if ((E->WFlags & WHasFrame) != 0) {
		E->FrstCol++; E->LastCol--; E->FrstRow++; E->LastRow--;
	}
	E->Rows = E->LastRow - E->FrstRow + 1;
	if (FL == nullptr) {
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
	void* p = nullptr;
	WORD i = 0; pstring s; 
	FieldDescr* F = nullptr;
	bool b = false, b2 = false, F2NoUpd = false;
	PushEdit(); MarkStore2(p);
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

	E->Attr = RunWordImpl(EO->ZAttr, colors.dTxt);
	E->dNorm = RunWordImpl(EO->ZdNorm, colors.dNorm);
	E->dHiLi = RunWordImpl(EO->ZdHiLi, colors.dHili);
	E->dSubSet = RunWordImpl(EO->ZdSubset, colors.dSubset);
	E->dDel = RunWordImpl(EO->ZdDel, colors.dDeleted);
	E->dTab = RunWordImpl(EO->ZdTab, E->Attr | 0x08);
	E->dSelect = RunWordImpl(EO->ZdSelect, colors.dSelect);
	E->Top = StoreStr(RunShortStr(EO->Top));
	if (EO->Mode != nullptr)
		EditModeToFlags(RunShortStr(EO->Mode), &E->NoDelete, false);
	if (spec.Prompt158) E->Prompt158 = true;
	if (EO->SetOnlyView /*UpwEdit*/) {
		EO->Tab = nullptr; E->OnlyTabs = true; E->OnlySearch = false;
	}
	if (E->LVRecPtr != nullptr) { E->EdRecVar = true; E->Only1Record = true; }
	if (E->Only1Record) E->OnlySearch = false;
	if (EO->W.C1 != nullptr) {
		RunWFrml(EO->W, E->WFlags, E->V); E->WwPart = true;
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
	if (E->NPages > 1) { E->NRecs = 1; }
	else { E->NRecs = (E->Rows - E->NHdTxt) / E->RecTxt->N; }
	E->BaseRec = 1; 
	E->IRec = 1;
	CFld = E->FirstFld; 
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
		E->NewRecPtr = E->LVRecPtr; E->NoDelete = true; E->NoCreate = true;
		E->Journal = nullptr; E->KIRoot = nullptr;
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
		if (EO->StartRecKeyZ != nullptr) E->StartRecKey = StoreStr(RunShortStr(EO->StartRecKeyZ));
		E->StartRecNo = RunInt(EO->StartRecNoZ); E->StartIRec = RunInt(EO->StartIRecZ);
		E->VK = EO->ViewKey;
		if (E->DownLD != nullptr) {
			E->DownSet = true; E->DownKey = GetFromKey(E->DownLD);
			if (E->VK == nullptr) E->VK = E->DownKey;
			switch (E->OwnerTyp) {
			case 'r': E->DownRecPtr = E->DownLV->RecPtr; break;
			case 'F': { 
				E->OwnerRecNo = RunInt(FrmlPtr(EO->DownLV));
				CFile = E->DownLD->ToFD; 
				E->DownRecPtr = GetRecSpace(); 
				CFile = E->FD; 
				break; 
			}
			}
		}
		else if (E->VK == nullptr) E->VK = E->FD->Keys;
#ifdef FandSQL
		if (CFile->IsSQLFile && (E->VK = nullptr)) { SetMsgPar(CFile->Name); RunError(652); }
#endif
		if (E->SelKey != nullptr)
			if (E->SelKey->KFlds == nullptr) E->SelKey->KFlds = E->VK->KFlds;
			else if (!EquKFlds(E->SelKey->KFlds, E->VK->KFlds)) RunError(663);
	}
	if (EO->StartFieldZ != nullptr) {
		s = TrailChar(' ', RunShortStr(EO->StartFieldZ)); 
		D = E->FirstFld;
		while (D != nullptr) {
			if (SEquUpcase(D->FldD->Name, s)) E->StartFld = D; 
			D = (EFldD*)D->Chain;
		}
	}
	E->WatchDelay = RunInt(EO->WatchDelayZ) * 18;
	if (EO->Head == nullptr) { E->Head = StandardHead(); }
	else E->Head = GetStr_E(EO->Head);
	E->Last = GetStr_E(EO->Last); 
	E->AltLast = GetStr_E(EO->AltLast);
	E->CtrlLast = GetStr_E(EO->CtrlLast); 
	E->ShiftLast = GetStr_E(EO->ShiftLast);
	F2NoUpd = E->OnlyTabs && (EO->Tab == nullptr) && !EO->NegTab && E->OnlyAppend;
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
		D = (EFldD*)D->Chain;
	}
	if (E->OnlyTabs && (E->NTabsSet == 0)) {
		E->NoDelete = true; 
		if (!E->OnlyAppend) E->NoCreate = true;
	}
	RdDepChkImpl();
	NewChkKey();
	MarkStore(E->AfterE);
	ReleaseStore2(p);
}

EFldD* FindEFld_E(FieldDescr* F)
{
	EFldD* D = E->FirstFld;
	while (D != nullptr) { if (D->FldD == F) goto label1; D = (EFldD*)D->Chain; }
label1:
	return D;
}

void ZeroUsed()
{
	EFldD* D = E->FirstFld; 
	while (D != nullptr) { D->Used = false; D = (EFldD*)D->Chain; };
}

EFldD* LstUsedFld()
{
	EFldD* D = E->LastFld;
	while (D != nullptr)
	{
		if (D->Used) goto label1;
		D = D->ChainBack;
	}
label1:
	return D;
}

void RdDepChkImpl()
{
	pstring s;
	CFile = E->FD;
	switch (CFile->Typ) {
	case '0': { RdMsg(53); s = MsgLine; goto label1; break; }
	case 'C': {
		RdMsg(54); s = MsgLine;
		if (spec.CPMdrive != ' ') s = s + ',' + spec.CPMdrive + ':';
		RdMsg(55); s = s + MsgLine;
		if (spec.CPMdrive != ' ') s = s + ',' + spec.CPMdrive + ':';
		s = s + "''";
	label1:
		ResetCompilePars();
		SetInpStr(s);
		RdUDLI();
		return;
		break;
	}
	default: RdAllUDLIs(CFile);
	}
}

void TestedFlagOff()
{
	FieldDescr* F = CFile->FldD;
	while (F != nullptr)
	{
		F->Typ = char(F->Typ & 0x7F);
		F = (FieldDescr*)F->Chain;
	}
}

void SetFrmlFlags(FrmlPtr Z)
{
	KeyFldDPtr Arg; FrmlList fl;
	if (Z == nullptr) return;
	switch (Z->Op) {
	case _field: SetFlag(Z->Field); break;
	case _access: {
		if (Z->LD != nullptr)
		{
			Arg = Z->LD->Args;
			while (Arg != nullptr) { SetFlag(Arg->FldD); Arg = (KeyFldD*)Arg->Chain; }
		}
		break;
	}
	case _userfunc: {
		fl = Z->FrmlL; while (fl != nullptr) {
			SetFrmlFlags(fl->Frml); fl = (FrmlListEl*)fl->Chain;
		}
		break;
	}
	default: {
		if (Z->Op >= 0x60 && Z->Op <= 0xaf) /*1-ary*/ SetFrmlFlags(Z->P1);
		if (Z->Op >= 0xB0 && Z->Op <= 0xEf) /*2-ary*/
		{
			SetFrmlFlags(Z->P1); SetFrmlFlags(Z->P2);
		}
		if (Z->Op >= 0xF0 && Z->Op <= 0xFf) /*3-ary*/ {
			SetFrmlFlags(Z->P1); SetFrmlFlags(Z->P2);
			SetFrmlFlags(Z->P3);
		}
		break;
	}
	}
}

void SetFlag(FieldDescr* F)
{
	EFldD* D;
	if ((F->Typ & 0x80) != 0) return;
	F->Typ = F->Typ | 0x80;
	if (F->Flg && f_Stored != 0) { D = FindEFld_E(F); if (D != nullptr) D->Used = true; }
	else SetFrmlFlags(F->Frml);
}

void RdDep()
{
	FrmlPtr Bool, Z; EFldD* D; char FTyp; DepDPtr Dp;
	RdLex();
label1:
	Accept('('); Bool = RdBool(); Accept(')');
label2:
	D = FindEFld_E(RdFldName(CFile)); Accept(_assign); Z = RdFrml(FTyp);
	if (D != nullptr)
	{
		Dp = (DepD*)GetStore(sizeof(*Dp)); Dp->Bool = Bool; Dp->Frml = Z;
		ChainLast(D->Dep, Dp);
	}
	if (Lexem == ';')
	{
		RdLex(); if (!(Lexem == '#' || Lexem == 0x1A))
		{
			if (Lexem == '(') goto label1; else goto label2;
		}
	}
}

void RdCheck()
{
	WORD Low; ChkDPtr C; EFldD* D;
	SkipBlank(false); Low = CurrPos; RdLex();
label1:
	C = RdChkD(Low);
	ZeroUsed(); SetFrmlFlags(C->Bool); TestedFlagOff();
	D = LstUsedFld();
	if (D != nullptr) ChainLast(D->Chk, C); else ReleaseStore(C);
	if (Lexem == ';')
	{
		SkipBlank(false); Low = CurrPos; RdLex();
		if (!(Lexem == '#' || Lexem == 0x1A)) goto label1;
	}
}

void RdImpl()
{
	EFldD* D; FrmlPtr Z; char FTyp; FieldDPtr F; ImplDPtr ID;
	RdLex();
label1:
	F = RdFldName(CFile); Accept(_assign); Z = RdFrml(FTyp);
	D = FindEFld_E(F);
	if (D != nullptr) D->Impl = Z;
	else {
		ID = (ImplD*)GetStore(sizeof(*ID)); ID->FldD = F; ID->Frml = Z;
		ChainLast(E->Impl, ID);
	}
	if (Lexem == ';')
	{
		RdLex(); if (!(Lexem == '#' || Lexem == 0x1A)) goto label1;
	}
}

void RdUDLI()
{
	RdLex();
	if ((Lexem == '#') && (ForwChar == 'U'))
		do { RdLex(); } while (!(Lexem == '#' || Lexem == 0x1A));
	if ((Lexem == '#') && (ForwChar == 'D')) { RdLex(); RdDep(); }
	if ((Lexem == '#') && (ForwChar == 'L')) { RdLex(); RdCheck(); }
	if ((Lexem == '#') && (ForwChar == 'I')) { RdLex(); RdImpl(); }
}

void RdAllUDLIs(FileD* FD)
{
	RdbD* r = nullptr;
	if (FD->OrigFD != nullptr) RdAllUDLIs(FD->OrigFD);
	if (FD->TxtPosUDLI != 0) {
		ResetCompilePars(); SetInpTTxtPos(FD);
		r = CRdb; CRdb = FD->ChptPos.R; RdUDLI(); CRdb = r;
	}
}

pstring StandardHead()
{
	pstring s; 
	pstring c(59);
	c = "          ______                                 __.__.____";
	if (E->ViewName != nullptr) s = *E->ViewName; 
	else if (E->EdRecVar) s = "";
	else {
		s = E->FD->Name;
		switch (E->FD->Typ) {
		case 'X': {
			pstring* p = E->VK->Alias;
			if ((p != nullptr) && (*p != "")) s = s + "/" + *E->VK->Alias;
			break; }
		case '0': s = s + ".RDB"; break;
		case '8': s = s + ".DTA"; break;
		}
	}
	if (s.length() > 16) s[0] = 16;
	auto str = copy(c, 17, 20 - s.length()) + s + c;
	return str;
}

pstring GetStr_E(FrmlPtr Z)
{
	pstring s;
	if (Z == nullptr) return "";
	else {
		s = RunShortStr(Z);
		while (LenStyleStr(s) > TxtCols) s[0]--;
		return s;
	}
}

void NewChkKey()
{
	KeyD* K; KeyFldD* KF; EFldD* D; KeyList KL;
	K = CFile->Keys; while (K != nullptr) {
		if (!K->Duplic) {
			ZeroUsed(); KF = K->KFlds;
			while (KF != nullptr) {
				D = FindEFld_E(KF->FldD);
				if (D != nullptr) D->Used = true;
				KF = (KeyFldD*)KF->Chain;
			}
			D = LstUsedFld(); if (D != nullptr) {
				KL = (KeyListEl*)GetStore(sizeof(*KL));
				ChainLast(D->KL, KL); KL->Key = K;
			}
			K = K->Chain;
		}
	}
}
