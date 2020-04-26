#include "rdmix.h"



#include "common.h"
#include "compile.h"
#include "kbdww.h"
#include "legacy.h"
#include "lexanal.h"
#include "memory.h"
#include "rdfrml.h"
#include "recacc.h"
#include "wwmix.h"

void RdLocDcl(LocVarBlkD* LVB, bool IsParList, bool WithRecVar, char CTyp)
{
	LocVar* lv; FrmlPtr Z; pstring s; double r; char typ, lx, fc; WORD sz, n;
	FileD* cf; FileD* fd; void* cr; void* p; XWKey* k; bool rp; KeyFldD* kf; KeyFldD* kf1;
	char FDTyp;
label1:
	rp = false;
	if (IsParList && IsKeyWord("VAR")) {
		if (CTyp == 'D') OldError(174); rp = true;
	}
	lv = RdVarName(LVB, IsParList);
	if (not IsParList) while (Lexem == ',') { RdLex(); RdVarName(LVB, IsParList); }
	Accept(':'); Z = nullptr;
	if (IsKeyWord("BOOLEAN")) {
		if ((Lexem == _equ) && !IsParList) {
			RdLex();
			if (IsKeyWord("TRUE")) {
				Z = GetOp(_const, sizeof(bool)); Z->B = true;
			}
			else { if (!IsKeyWord("FALSE")) Error(42); }
		}
		typ = 'B'; sz = sizeof(bool); goto label2;
	}
	else if (IsKeyWord("REAL")) {
		if ((Lexem == _equ) && !IsParList) {
			RdLex();
			r = RdRealConst();
			if (r != 0) {
				Z = GetOp(_const, sizeof(double));
				Z->R = r;
			}
		}
		typ = 'R'; sz = sizeof(double); goto label2;
	}
	else if (IsKeyWord("STRING")) {
		if ((Lexem == _equ) && !IsParList) {
			RdLex();
			s = LexWord; Accept(_quotedstr);
			if (s != "") {
				Z = GetOp(_const, s.length() + 1); Z->S = s;
			}
		}
		typ = 'S'; sz = sizeof(longint);
	label2:
		while (lv != nullptr) {
			/* !!! with lv^ do!!! */
			lv->FTyp = typ; lv->Op = _getlocvar; lv->IsRetPar = rp;
			lv->Init = Z; lv->BPOfs = LVB->Size;
			LVB->Size += sz; lv = lv->Chain;
		}
	}
	else if (rp) Error(168);
	else if (WithRecVar)
		if (TestKeyWord("FILE")) {
			lv->FTyp = 'f';
			LexWord = lv->Name;
			if (LexWord.length() > 8) OldError(2);
			fd = FindFileD(); RdLex();
			if (IsParList) {
				if (not WithRecVar) OldError(162);
				if (fd == nullptr) OldError(163); lv->FD = fd;
			}
			else {
				if (fd != nullptr) OldError(26); FDTyp = '6';
				if (Lexem == '.') {
					RdLex(); TestIdentif();
					if (EquUpcase('X')) FDTyp = 'X';
					else if (EquUpcase("DBF")) FDTyp = 'D';
					else Error(185); RdLex();
				}
				TestLex('[');
				p = SaveCompState();
				RdFileD(lv->Name, FDTyp, '$');
				TestLex(']');
				lv->FD = CFile; n = CurrPos; lx = Lexem; fc = ForwChar;
				RestoreCompState(p);
				CurrPos = n; Lexem = lx; ForwChar = fc;
				RdLex();
			}
		}
		else if (IsKeyWord("INDEX")) { typ = 'i'; goto label3; }
		else if (IsKeyWord("RECORD")) {
			typ = 'r';
		label3:
			AcceptKeyWord("OF"); cf = CFile; cr = CRecPtr; CFile = RdFileName();
			if (typ == 'i') {
				if (CFile->Typ != 'X') OldError(108); kf1 = nullptr;
				if (Lexem == '(') { RdLex(); RdKFList(kf1, CFile); Accept(')'); }
			}
			while (lv != nullptr) {
				lv->FTyp = typ; lv->FD = CFile;
				if (typ == 'r') lv->RecPtr = ptr(0, 1); /* for RdProc nullptr-tests + no Run*/
				   /* frueher bei IsParList K = nullptr; warum? */
				else {
					k = (XWKey*)GetZStore(sizeof(*k));
					k->Duplic = true; k->InWork = true;
					k->KFlds = kf1; kf = kf1;
					while (kf != nullptr) {
						k->IndexLen += kf->FldD->NBytes; kf = kf->Chain;
					}
					lv->RecPtr = k;
				}
				lv = lv->Chain;
			}
			CFile = cf; CRecPtr = cr;
		}
		else Error(137);
	else Error(39);
	if (IsParList) {
		if (Lexem == ')') return;
		else { Accept(';'); goto label1; };
	}
	Accept(';');
	if ((Lexem != '#') && (Lexem != '.') && !TestKeyWord("BEGIN"))
		goto label1;
}

LocVar* RdVarName(LocVarBlkD* LVB, bool IsParList)
{
	LocVar* lv;
	TestIdentif();
	lv = LVB->Root;
	while (lv != nullptr) {
		if (EquUpcase(lv->Name)) Error(26); lv = lv->Chain;
	}
	lv = (LocVar*)GetZStore(sizeof(*lv) - 1 + LexWord.length());
	ChainLast(LVB->Root, lv);
	Move(&LexWord, &lv->Name, LexWord.length() + 1); RdLex();
	auto result = lv;
	if (IsParList) { lv->IsPar = true; LVB->NParam++; }
	return lv;
}

bool FindLocVar(LocVar* LVRoot, LocVar* LV)
{
	auto result = false; if (Lexem != _identifier) return result;
	LV = LVRoot; while (LV != nullptr) {
		if (EquUpcase(LV->Name)) { return true; }
		LV = LV->Chain;
	}
	return result;
}

bool FindChpt(char Typ, const pstring& name, bool local, RdbPos* RP)
{
	RdbDPtr R; WORD I;  FileDPtr CF; void* CR;
	CF = CFile; CR = CRecPtr; CFile = Chpt;
	CRecPtr = GetRecSpace(); R = CRdb;
	auto result = false;
	while (R != nullptr) {
		CFile = R->FD;
		for (I = 1; I < CFile->NRecs; I++) {
			ReadRec(I);
			if ((_ShortS(ChptTyp) == Typ)
				&& SEquUpcase(TrailChar(' ', _ShortS(ChptName)), name))
			{
				RP->R = R; RP->IRec = I;
				result = true;
				goto label1;
			}
		}
		if (local) goto label1;
		R = R->ChainBack;
	}
label1:
	ReleaseStore(CRecPtr); CFile = CF; CRecPtr = CR;
	return result;
}

void RdChptName(char C, RdbPos* Pos, bool TxtExpr)
{
	if (TxtExpr && (Lexem = '[')) {
		RdLex(); Pos->R = RdbDPtr(RdStrFrml); Pos->IRec = 0; Accept(']');
	}
	else {
		TestLex(_identifier);
		if (not FindChpt(C, LexWord, false, Pos)) Error(37);
		RdLex();
	}
}

FieldList AllFldsList(FileDPtr FD, bool OnlyStored)
{
	FieldList FLRoot, FL; FieldDPtr F;
	F = FD->FldD; FLRoot = nullptr;
	while (F != nullptr) {
		if ((F->Flg && f_Stored != 0) || !OnlyStored)
		{
			FL = (FieldListEl*)GetStore(sizeof(*FL));
			FL->FldD = F;
			ChainLast(FLRoot, FL);
		}
		F = F->Chain;
	}
	return FLRoot;
}

EditOpt* GetEditOpt()
{
	EditOpt* EO;
	EO = (EditOpt*)GetZStore(sizeof(*EO));
	auto result = EO;
	EO->UserSelFlds = true;
	return result;
}

RprtOpt* GetRprtOpt()
{
	RprtOpt* RO;
	RO = (RprtOpt*)GetZStore(sizeof(*RO));
	auto result = RO;
	RO->Mode = _ALstg; RO->Style = '?'; RO->Width = spec.AutoRprtWidth;
	return RO;
}

void CFileLikeFD(FileD* FD, WORD MsgNr)
{
	FileD* FD1;
	if (!CFile->IsJournal && ((CFile == FD) || (CFile->OrigFD == FD))) return;
	Set2MsgPar(CFile->Name, FD->Name);
	RunError(MsgNr);
}

pstring* RdHelpName()
{
	pstring* s;
	if (CRdb->HelpFD == nullptr) Error(132);
	if (Lexem != _identifier) TestLex(_quotedstr);
	s = StoreStr(LexWord);
	RdLex();
	return s;
}

FrmlPtr RdAttr()
{
	char c; BYTE n; FrmlPtr z;
	if (Lexem == '^') {
		RdLex();
		c = (char)toupper(Rd1Char()) - 64;
		if (!SetStyleAttr(c, n)) OldError(120);
		z = GetOp(_const, sizeof(double));
		z->R = n; return z;
	}
	return RdRealFrml();
}

void RdW(WRectFrml& W)
{
	W.C1 = RdRealFrml(); Accept(','); W.R1 = RdRealFrml();
	Accept(',');
	W.C2 = RdRealFrml(); Accept(','); W.R2 = RdRealFrml();
}

void RdFrame(FrmlPtr Z, BYTE& WFlags)
{
	if (Lexem != ',') return;
	RdLex();
	if (Lexem == '@') { WFlags = WFlags | WNoClrScr; RdLex(); }
	if (Lexem == '*') { WFlags = WFlags | WPushPixel; RdLex(); }
	if (!(Lexem == ',' || Lexem == ')' || Lexem == '!')) {
		WFlags = WFlags | WHasFrame;
		if (Lexem == _equ) {
			RdLex(); WFlags = WFlags | WDoubleFrame;
		}
		Z = RdStrFrml();
	}
	if (Lexem == '!') { WFlags = WFlags | WShadow; RdLex(); }
}

bool PromptSortKeys(FieldList FL, KeyFldD* SKRoot)
{
	KeyFldD* SK;
	auto result = true;
	SKRoot = nullptr;
	while (FL != nullptr) {
		/* !!! with FL->FldD^ do!!! */
		if (FL->FldD->Typ != 'T') PutSelect(FL->FldD->Name); FL = FL->Chain;
	}
	if (ss.empty) return result;
	ss.ascdesc = true;
	ss.subset = true;
	SelectStr(0, 0, 25, "");
	if (KbdChar == _ESC_) { return false; }
label1:
	LexWord = GetSelect; if (LexWord != "") {
		SK = (KeyFldD*)GetZStore(sizeof(*SK)); ChainLast(SKRoot, SK);
		SK->FldD = FindFldName(CFile);
		if (ss.Tag == '>') SK->Descend = true;
		if (SK->FldD->Typ == 'A') SK->CompLex = true;
		goto label1;
	}
	return result;
}

void RdAssignFrml(char FTyp, bool& Add, FrmlPtr Z)
{
	char Typ;
	if (Lexem == _addass) { RdLex(); Add = true; }
	else Accept(_assign);
	Z = RdFrml(Typ);
	if ((FTyp != Typ) || Add && (Typ != 'R')) OldError(12);
}

bool FldTypIdentity(FieldDescr* F1, FieldDescr* F2)
{
	auto result = false;
	if (F1->Typ != F2->Typ) return result;
	if ((F1->Typ == 'F') && (F1->M != F2->M)) return result;
	if ((F1->Typ == 'N' || F1->Typ == 'A' || F1->Typ == 'F') && (F1->L != F2->L)) return result;
	return true;
}

void RdFldList(FieldListEl* FLRoot)
{
	FieldDPtr F; FieldList FL;
label1:
	F = RdFldName(CFile);
	FL = (FieldListEl*)GetStore(sizeof(*FL));
	FL->FldD = F;
	ChainLast(FLRoot, FL);
	if (Lexem == ',') { RdLex(); goto label1; };
}

void RdNegFldList(bool& Neg, FieldList FLRoot)
{
	if (Lexem == '^') { RdLex(); Neg = true; }
	Accept('(');
	if (Lexem == ')') Neg = true;
	else RdFldList(FLRoot);
	Accept(')');
}

void EditModeToFlags(pstring Mode, void* Flgs, bool Err)
{
	pstring FlgTxt[24] = { "^Y","?Y","^N","F1","F2","F3","01",
	"!!","??","?E","?N","<=","R2","24","CO","LI",
	"->","^M","EX","WX","S7","#A","#L","SL" };
	pstring s = "xx";
	bool* Flags = (bool*)Flgs;
	WORD i, j;
	i = 0;
	while (i < Mode.length()) {
		s[1] = toupper(Mode[i]);
		s[2] = toupper(Mode[i + 1]);
		i += 2;
		for (j = 0; j < 24; j++)
			if (s == FlgTxt[j]) { Flags[j] = true; goto label1; }
		goto label2;
	label1:
		{}
	}
	if (i == Mode.length())
		label2:
	if (Err) Error(92);
}

KeyDPtr RdViewKey()
{
	KeyDPtr k; LocVar* lv; pstring s; integer i;
	KeyDPtr result = nullptr;
	if (Lexem != '/') return result;
	RdLex();
	k = CFile->Keys;
	if (Lexem == '@') goto label1;
	TestIdentif();
	while (k != nullptr) {
		if (EquUpcase(*k->Alias)) goto label1;
		k = k->Chain;
	}
	s = LexWord;
	i = s.first('_');
	if (i != 0) s = copy(s, i + 1, 255);
	s = CFile->Name + '_' + s; k = CFile->Keys;
	while (k != nullptr) {
		if (SEquUpcase(s, *k->Alias)) goto label1;
		k = k->Chain;
	}
	if (IdxLocVarAllowed && FindLocVar(LVBD.Root, lv) && (lv->FTyp == 'i'))
	{
		if (lv->FD != CFile) Error(164);
		k = KeyDPtr(lv->RecPtr);
		goto label1;
	}
	Error(109);
label1:
	if (CFile->Typ != 'X')
#ifdef FandSQL
		if (CFile->typSQLFile) Error(24); else
#endif
			Error(108);
	RdLex();
	result = k;
	return result;
}

void SrchF(FieldDPtr F)
{
	if (F == KeyArgFld) { KeyArgFound = true; return; }
	if (F->Flg && f_Stored == 0) SrchZ(F->Frml);
}

void SrchZ(FrmlPtr Z)
{
	KeyFldDPtr KF; FrmlList fl;
	if (Z == nullptr) return;
	switch (Z->Op) {
	case _field: SrchF(Z->Field); break;
	case _access: {
		if (Z->LD != nullptr) {
			KF = Z->LD->Args;
			while (KF != nullptr) {
				SrchF(KF->FldD); KF = KF->Chain;
			}
		}
		break;
	}
	case _userfunc: {
		fl = Z->FrmlL;
		while (fl != nullptr) {
			SrchZ(fl->Frml); fl = fl->Chain;
		}
		break;
	}
	default: {
		if (Z->Op >= 0x60 && Z->Op <= 0xAF)SrchZ(Z->P1); /*1-ary*/
		else if (Z->Op >= 0xB0 && Z->Op <= 0xEF) /*2-ary*/
		{
			SrchZ(Z->P1); SrchZ(Z->P2);
		}
		else if (Z->Op >= 0xB0 && Z->Op <= 0xEF) /*3-ary*/
		{
			SrchZ(Z->P1); SrchZ(Z->P2); SrchZ(Z->P3);
		}
		break;
	}
	}
}

bool IsKeyArg(FieldDPtr F, FileDPtr FD)
{
	KeyDPtr k; KeyFldDPtr kf;
	k = FD->Keys;
	while (k != nullptr) {
		KeyArgFld = F;
		kf = k->KFlds;
		while (kf != nullptr) {
			SrchF(kf->FldD);
			if (KeyArgFound) { return true; }
			kf = kf->Chain;
		}
		k = k->Chain;
	}
	return false;
}

KeyFldD* RdKF(FileDPtr FD)
{
	KeyFldDPtr KF; FieldDPtr F;
	KF = (KeyFldD*)GetZStore(sizeof(KeyFldD));
	KeyFldD* result = KF;
	if (Lexem == _gt) { RdLex(); KF->Descend = true; }
	if (Lexem == '~') { RdLex(); KF->CompLex = true; }
	F = RdFldName(FD); KF->FldD = F;
	if (F->Typ == 'T') OldError(84);
	if (KF->CompLex && (F->Typ != 'A')) OldError(94);
	return result;
}

WORD RdKFList(KeyFldDPtr KFRoot, FileDPtr FD)
{
	WORD n; KeyFldDPtr KF;
label1:
	ChainLast(KFRoot, RdKF(FD));
	if (Lexem == ',') { RdLex(); goto label1; }
	n = 0; KF = KFRoot;   /*looping over all fields, !only the last read*/
	while (KF != nullptr) { n += KF->FldD->NBytes; KF = KF->Chain; }
	if (n > 255) OldError(126);
	return n;
}

void CompileRecLen()
{
	/* !!! with CFile^ do!!! */
	FieldDPtr F = CFile->FldD;
	WORD l = 0;
	WORD n = 0;
	if ((CFile->Typ == 'X' || CFile->Typ == 'D')) l = 1;
	while (F != nullptr) {
		switch (CFile->Typ) {
		case '8': if (F->Typ == 'D') F->NBytes = 2; break;
		case 'D': {
			switch (F->Typ) {
			case 'F': F->NBytes = F->L - 1; break;
			case 'D': F->NBytes = 8; break;
			case 'T': F->NBytes = 10; break;
			}
			break;
		}
		}
		if (F->Flg && f_Stored != 0) { F->Displ = l; l += F->NBytes; n++; }
		F = F->Chain;
	}
	CFile->RecLen = l;
	switch (CFile->Typ) {
	case '8': CFile->FrstDispl = 4; break;
	case 'D': CFile->FrstDispl = (n + 1) * 32 + 1; break;
	default:  CFile->FrstDispl = 6; break;
	}
}


