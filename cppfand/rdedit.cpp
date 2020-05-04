#include "rdedit.h"

#include "common.h"
#include "legacy.h"
#include "lexanal.h"
#include "memory.h"
#include "rdfildcl.h"
#include "rdfrml.h"
#include "runedi.h"

void PushEdit()
{
	EditD* E1 = (EditD*)GetZStore(sizeof(*E));
	/* !!! with E1->V do!!! */
	{
		E1->V.C1 = 1; E1->V.R1 = 2; E1->V.C2 = TxtCols; E1->V.R2 = TxtRows - 1;
	}
	E1->PrevE = E; E = E1;
}

void SToSL(void* SLRoot, pstring s)
{
	StringList SL;
	SL = (StringListEl*)GetStore(s.length() + 5);
	SL->S = s;
	ChainLast(SLRoot, SL);
}

void StoreRT(WORD Ln, StringList SL, WORD NFlds)
{
	ERecTxtD* RT;
	if (NFlds == 0) Error(81);
	RT = (ERecTxtD*)GetStore(sizeof(*RT));
	ChainLast(E->RecTxt, RT);
	RT->N = Ln;
	RT->SL = SL;
}

void RdEForm(FileD* ParFD, RdbPos FormPos)
{
	EFldD* D; EFldD* D1; EFldD* PrevD; FieldDPtr F; FieldList FL; StringList SLRoot;
	pstring s; WORD NPages, Col, Ln, Max, M, N, NFlds, i; FileDPtr FD1;
	bool comment; char c; BYTE a;
	SetInpTT(FormPos, true); N = 0; Max = 0;
label1:
	s = "";
	while (!(ForwChar == '#' || ForwChar == 0x1A || ForwChar == 0x0D || ForwChar == '{')) {
		/* read headlines */
		s = s + ForwChar; ReadChar();
	}
	switch (ForwChar) {
	case 0x1A: Error(76); break;
	case '#': goto label2; break;
	case '{': { SkipBlank(true); goto label1; break; }
	}
	ReadChar();
	if (ForwChar == 0x0A) ReadChar();
	SToSL(E->HdTxt, s); E->NHdTxt++;
	if (E->NHdTxt + 1 > E->Rows) Error(102); goto label1;
	/* read field list */
label2:
	ReadChar(); ReadChar(); Lexem = CurrChar; Accept('_'); FD1 = RdFileName();
	if (ParFD == nullptr) CFile = FD1; else CFile = ParFD;
	E->FD = CFile;
label3:
	N++; D = (EFldD*)GetZStore(sizeof(*D));
	if (Lexem == _number) {
		M = RdInteger(); if (M == 0) OldError(115); Accept(':'); D->ScanNr = M;
	}
	else D->ScanNr = N;
	D1 = FindScanNr(D->ScanNr); ChainLast(E->FirstFld, D);
	if ((D1 != nullptr) && (D->ScanNr == D1->ScanNr)) Error(77);
	F = RdFldName(CFile); D->FldD = F;
	FL = (FieldListEl*)GetStore(sizeof(*FL));
	FL->FldD = F; ChainLast(E->Flds, FL);
	if (Lexem == ',') { RdLex(); goto label3; }
	TestLex(';'); SkipBlank(true);
	/* read record lines */
	D = E->FirstFld; NPages = 0;
label4:
	NPages++; Ln = 0; NFlds = 0; SLRoot = nullptr;
label5:
	s = ""; Ln++; Col = E->FrstCol;
	while (!(ForwChar == 0x0D || ForwChar == 0x1A || ForwChar == '\\' || ForwChar == '{'))
		if (ForwChar == '_') {
			if (D == nullptr) Error(30); NFlds++;
			D->Col = Col; D->Ln = Ln; D->Page = NPages; M = 0;
			while (ForwChar == '_') {
				s = s + ' '; M++; Col++; ReadChar();
			}
			F = D->FldD; D->L = F->L; if (F->Typ == 'T') D->L = 1;
			if ((F->Typ == 'A') && (M < F->L)) D->L = M;
			else if (M != D->L) {
				str(D->L, 2, s); Set2MsgPar(s, F->Name); Error(79);
			}
			if (Col > E->LastCol) Error(102);
			D = D->Chain;
		}
		else {
			if (!SetStyleAttr(ForwChar, a)) {
				if (Col > E->LastCol) Error(102); Col++;
			}
			s = s + ForwChar; ReadChar();
		}
	SToSL(SLRoot, s); c = ForwChar; if (c == '\\') ReadChar();
	SkipBlank(true);
	if (ForwChar != 0x1A)
		if ((c == '\\') || (E->NHdTxt + Ln == E->Rows)) {
			StoreRT(Ln, SLRoot, NFlds); goto label4;
		}
		else goto label5;
	StoreRT(Ln, SLRoot, NFlds); E->NPages = NPages;

	if (D != nullptr) Error(30);
	D = FindScanNr(1); D->ChainBack = nullptr;
	for (i = 2; i < N; i++) {
		PrevD = D; D = FindScanNr(D->ScanNr + 1); D->ChainBack = PrevD;
	}
	E->LastFld = *D; PrevD = nullptr;
	while (D != nullptr) { D->Chain = PrevD; PrevD = D; D = D->ChainBack; }
	E->FirstFld = PrevD;
}

EFldD* FindScanNr(WORD N)
{
	EFldD* D; EFldD* D1; WORD M;
	D = E->FirstFld; M = 0xffff; D1 = nullptr;
	while (D != nullptr) {
		if ((D->ScanNr >= N) && (D->ScanNr < M)) { M = D->ScanNr; D1 = D; }
		D = D->Chain;
	}
	return D1;
}

void AutoDesign(FieldList FL)
{
	WORD NPages, Col, Ln, L, i, m, FldLen, maxcol;
	pstring s; StringList SLRoot;
	EFldD* D; EFldD* PrevD; FieldDPtr F;
	D = (EFldD*)(&E->FirstFld); PrevD = nullptr; NPages = 1; s = ""; Ln = 0; SLRoot = nullptr;
	Col = E->FrstCol; maxcol = E->LastCol - E->FrstCol;
	while (FL != nullptr) {
		F = FL->FldD; FL = FL->Chain;
		D->Chain = (EFldD*)GetZStore(sizeof(*D)); D = D->Chain; D->ChainBack = PrevD;
		PrevD = D;
		D->FldD = F; D->L = F->L; if (D->L > maxcol) D->L = maxcol;
		if ((E->FD->Typ == 'C') && (D->L > 44)) D->L = 44; /*catalog pathname*/
		FldLen = D->L; if (F->Typ == 'T') D->L = 1;
		L = F->Name.length(); if (FldLen > L) L = FldLen;
		if (Col + L > E->LastCol) {
			SToSL(SLRoot, s); SToSL(SLRoot, ""); Ln += 2;
			if (Ln + 2 > E->Rows) {
				StoreRT(Ln, SLRoot, 1); NPages++; Ln = 0; SLRoot = nullptr;
			}
			Col = E->FrstCol; s = "";
		}
		m = (L - F->Name.length() + 1) / 2;
		for (i = 1; i < m; i++) s = s + ' ';
		s = s + F->Name;
		m = L - F->Name.length() - m;
		for (i = 1; i < m + 1; i++) s = s + ' ';
		D->Col = Col + (L - FldLen + 1) / 2; D->Ln = Ln + 2; D->Page = NPages;
		Col += (L + 1);
	}
	SToSL(SLRoot, s); SToSL(SLRoot, ""); Ln += 2; StoreRT(Ln, SLRoot, 1);
	D->Chain = nullptr; E->LastFld = *D; E->NPages = NPages;
	if (NPages == 1) { /* !!! with E->RecTxt^ do!!! */
		auto er = *E->RecTxt;
		if (er.N == 2) {
			E->HdTxt = er.SL; er.SL = er.SL->Chain;
			E->HdTxt->Chain = nullptr;
			E->NHdTxt = 1; er.N = 1; D = E->FirstFld;
			while (D != nullptr) { D->Ln--; D = D->Chain; }
			if (E->Rows == 1) { E->NHdTxt = 0; E->HdTxt = nullptr; }
		}
		else if (er.N < E->Rows) {
			s = ""; for (i = E->FrstCol; i < E->LastCol; i++) s = s + '-';
			SToSL(er.SL, s); er.N++;
		}
	}
}

void RdFormOrDesign(FileD* F, FieldList FL, RdbPos FormPos)
{
	/* !!! with E^ do!!! */
	E->FrstCol = E->V.C1; E->FrstRow = E->V.R1; E->LastCol = E->V.C2; E->LastRow = E->V.R2;
	if ((E->WFlags && WHasFrame) != 0) {
		E->FrstCol++; E->LastCol--; E->FrstRow++; E->LastRow--;
	}
	E->Rows = E->LastRow - E->FrstRow + 1;
	if (FL == nullptr) {
		ResetCompilePars(); RdEForm(F, FormPos); E->IsUserForm = true;
	}
	else {
		E->FD = F; E->Flds = FL; AutoDesign(FL);
	}
}

EFldD* FindEFld_E(FieldDescr* F)
{
	EFldD* D;
	D = E->FirstFld;
	while (D != nullptr) { if (D->FldD == F) goto label1; D = D->Chain; }
label1:
	return D;
}

void ZeroUsed()
{
	EFldD* D = E->FirstFld; while (D != nullptr) { D->Used = false; D = D->Chain; };
}

EFldD* LstUsedFld()
{
	EFldD* D = &E->LastFld;
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
		if (spec.CPMDrive != " ") s = s + ',' + spec.CPMDrive + ':';
		RdMsg(55); s = s + MsgLine;
		if (spec.CPMDrive != ' ') s = s + ',' + spec.CPMDrive + ':';
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
		F = F->Chain;
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
			while (Arg != nullptr) { SetFlag(Arg->FldD); Arg = Arg->Chain; }
		}
		break;
	}
	case _userfunc: {
		fl = Z->FrmlL; while (fl != nullptr) {
			SetFrmlFlags(fl->Frml); fl = fl->Chain;
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
		RdLex(); if (!(Lexem == '#' || Lexem == 0x1A]))
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
		if (!(Lexem == '#' || Lexem == 0x1A])) goto label1;
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

pstring* StandardHead()
{
	pstring s; pstring* p;
	pstring c(59);
	c = "          ______                                 __.__.____";
	if (E->ViewName != nullptr) s = *E->ViewName; else if (E->EdRecVar) s = "";
	else {
		s = E->FD->Name;
		switch (E->FD->Typ) {
		case 'X': {
			p = E->VK->Alias;
			if ((p != nullptr) && (*p != "")) s = s + '/' + *E->VK->Alias;
			break; }
		case '0': s = s + ".RDB"; break;
		case '8': s = s + ".DTA"; break;
		}
	}
	if (s.length() > 16) s[0] = 16;
	return StoreStr(copy(c, 17, 20 - s.length()) + s + c);
}
