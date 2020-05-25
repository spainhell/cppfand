#include "rdproc.h"
#include "compile.h"
#include "legacy.h"
#include "oaccess.h"
#include "rdfildcl.h"
#include "rdrun.h"
#include "runfrml.h"

bool IsRdUserFunc;
kNames KeyNames[NKeyNames] = {
	{"HOME", 51, _Home_},
	{"UP", 52, _up_},
	{"PGUP", 53, _PgUp_},
	{"LEFT", 55, _left_},
	{"RIGHT", 57, _right_},
	{"END", 59, _End_},
	{"DOWN", 60, _down_},
	{"PGDN", 61, _PgDn_},
	{"INS", 62, _Ins_},
	{"CTRLLEFT", 71, _CtrlLeft_},
	{"CTRLRIGHT", 72, _CtrlRight_},
	{"CTRLEND", 73, _CtrlEnd_},
	{"CTRLPGDN", 74, _CtrlPgDn_},
	{"CTRLHOME", 75, _CtrlHome_},
	{"CTRLPGUP", 76, _CtrlPgUp_},
	{"TAB", 77, _Tab_},
	{"SHIFTTAB", 78, _ShiftTab_},
	{"CTRLN", 79, _N_},
	{"CTRLY", 80, _Y_},
	{"ESC", 81, _ESC_},
	{"CTRLP", 82, _P_} };
Instr* RdPInstr;


void TestCatError(WORD I, pstring Nm, bool Old)
{
	if (I == 0) {
		SetMsgPar(Nm);
		if (Old) OldError(96);
		else Error(96);
	}
}

bool IsRecVar(LocVar* LV)
{
	if (!FindLocVar(LVBD.Root, LV) || (LV->FTyp != 'r')) return false;
	RdLex();
	return true;
}

LocVar* RdRecVar()
{
	LocVar* LV = nullptr;
	if (!IsRecVar(LV)) Error(141);
	return LV;
}

LocVar* RdIdxVar()
{
	LocVar* lv = nullptr;
	if (!FindLocVar(LVBD.Root, lv) || (lv->FTyp != 'i')) Error(165);
	auto result = lv;
	RdLex();
	return result;
}

FrmlPtr RdRecVarFldFrml(LocVar* LV, char& FTyp)
{
	FrmlPtr Z = nullptr;
	Accept('.');
	switch (LV->FTyp) {
	case 'r': {
		Z = GetOp(_recvarfld, 12); FileDPtr cf = CFile;
		CFile = LV->FD; Z->File2 = CFile; Z->LD = (LinkD*)LV->RecPtr;
		bool fa = FileVarsAllowed; FileVarsAllowed = true;
		Z->P1 = RdFldNameFrmlF(FTyp); FileVarsAllowed = fa; CFile = cf;
		break;
	}
	case 'i': {
		Z = GetOp(_indexnrecs, 4); Z->WKey = WKeyDPtr(LV->RecPtr);
		pstring nrecs = "nrecs";
		AcceptKeyWord(nrecs); FTyp = 'R';
		break;
	}
	default: OldError(177); break;
	}
	return Z;
}

char RdOwner(LinkD* LLD, LocVar* LLV)
{
	FileD* fd = nullptr;
	auto result = '\0';
	LinkD* ld = nullptr;
	LocVar* lv = nullptr;
	if (FindLocVar(LVBD.Root, lv)) {
		if (!(lv->FTyp == 'i' || lv->FTyp == 'r' || lv->FTyp == 'f')) Error(177);
		ld = nullptr; LinkD* ld1 = LinkDRoot; while (ld1 != nullptr) {
			if ((ld1->FromFD == CFile) && (ld1->IndexRoot != 0) && (ld1->ToFD == lv->FD))
				ld = ld1; ld1 = ld1->Chain;
		}
		if (ld == nullptr) Error(116); RdLex();
		if (lv->FTyp == 'f') goto label2; else goto label1;
	}
	TestIdentif();
	ld = LinkDRoot;
	while (ld != nullptr) {
		if ((ld->FromFD == CFile) && EquUpcase(ld->RoleName, LexWord)) {
			if ((ld->IndexRoot = 0)) Error(116);
			RdLex(); fd = ld->ToFD;
			if (Lexem == '(') {
				RdLex();
				if (!FindLocVar(LVBD.Root, lv) || !(lv->FTyp == 'i' || lv->FTyp == 'r')) Error(177);
				RdLex(); Accept(')'); if (lv->FD != fd) OldError(149);
			label1:
				if (lv->FTyp == 'i') {
					KeyFldD* kf = WKeyDPtr(lv->RecPtr)->KFlds;
					if (ld->FromFD->IsSQLFile || ld->ToFD->IsSQLFile) OldError(155);
					if ((kf != nullptr) && !EquKFlds(kf, ld->ToKey->KFlds)) OldError(181);
				}
				LLV = lv; result = lv->FTyp; goto label3;
			}
			else {
			label2:
#ifdef FandSQL
				if (ld->ToFD->typSQLFile) Error(155);
#endif

				Accept('['); LLV = (LocVar*)RdRealFrml(); Accept(']'); result = 'F';
			label3: LLD = ld; return result;
			};
		}
		ld = ld->Chain;
	}
	Error(9);
	return result;
}

FrmlPtr RdFldNameFrmlP(char& FTyp)
{
	FileD* FD = nullptr; FrmlPtr Z = nullptr; LocVar* LV = nullptr;
	char Op = 0; LinkD* LD = nullptr; FieldDescr* F = nullptr;
	KeyD* K;

	FrmlPtr result = nullptr;

	if (IsForwPoint)
		if (FindLocVar(LVBD.Root, LV) && (LV->FTyp == 'i' || LV->FTyp == 'r')) {
			RdLex();
			result = RdRecVarFldFrml(LV, FTyp);
			return result;
		}
		else {
			pstring FName = LexWord; bool linked = IsRoleName(FileVarsAllowed, FD, LD);
			if (FD != nullptr) FName = FD->Name; if (!linked) RdLex();
			RdLex();
			FTyp = 'R';
			if (IsKeyWord("LASTUPDATE")) {
				Op = _lastupdate; if (FD != nullptr) goto label2; F = nullptr; goto label1;
			}
			if (IsKeyWord("ARCHIVES")) { F = CatArchiv; goto label0; }
			if (IsKeyWord("PATH")) { F = CatPathName; goto label0; }
			if (IsKeyWord("VOLUME")) {
				F = CatVolume;
			label0: FTyp = 'S';
			label1: Z = GetOp(_catfield, 6); Z->CatFld = F;
				Z->CatIRec = GetCatIRec(FName, true); result = Z;
				TestCatError(Z->CatIRec, FName, true); exit;
			}
			if (FD != nullptr) {
				if (IsKeyWord("GENERATION")) { Op = _generation; goto label2; }
				if (IsKeyWord("NRECSABS")) { Op = _nrecsabs; goto label2; }
				if (IsKeyWord("NRECS")) {
					Op = _nrecs;
				label2: Z = GetOp(Op, sizeof(FileDPtr));
					Z->FD = FD; result = Z; return result;
				};
			}
			if (linked) { result = RdFAccess(FD, LD, FTyp); return result; }
			if (FileVarsAllowed) OldError(9); else OldError(63);
			;
		}
	if (ForwChar == '[') {
		Z = GetOp(_accrecno, 8); FD = RdFileName(); RdLex(); Z->RecFD = FD;
#ifdef FandSQL
		if (FD->typSQLFile) OldError(155);
#endif

		Z->P1 = RdRealFrml(); Accept(']'); Accept('.');
		F = RdFldName(FD); Z->RecFldD = F;
		FTyp = F->FrmlTyp; result = Z;
		return result;
	}
	if (IsKeyWord("KEYPRESSED")) { Op = _keypressed; goto label3; }
	if (IsKeyWord("ESCPROMPT")) { Op = _escprompt; goto label3; }
	if (IsKeyWord("EDUPDATED")) {
		Op = _edupdated;
	label3: result = GetOp(Op, 0); FTyp = 'B';
		return result;
	}
	if (IsKeyWord("GETPATH")) {
		result = GetOp(_getpath, 0); FTyp = 'S';
		return result;
	}
	if (FindLocVar(LVBD.Root, LV)) {
		if (LV->FTyp == 'r' || LV->FTyp == 'f' || LV->FTyp == 'i') Error(143);
		RdLex(); result = FrmlPtr(LV->Op); FTyp = LV->FTyp;
		return result;
	}
	if (FileVarsAllowed) {
		Z = TryRdFldFrml(CFile, FTyp);
		if (Z == nullptr) Error(8);
		result = Z;
		return result;
	}
	Error(8);
}

FileD* RdPath(bool NoFD, pstring* Path, WORD& CatIRec)
{
	FileD* fd;
	CatIRec = 0;
	if (Lexem == _quotedstr) { Path = RdStrConst(); fd = nullptr; }
	else {
		TestIdentif();
		fd = FindFileD();
		if (fd == nullptr) {
			CatIRec = GetCatIRec(LexWord, true); TestCatError(CatIRec, LexWord, false);
		}
		else if (NoFD) Error(97);
		RdLex();
	}
	return fd;
}

FrmlPtr RdFunctionP(char& FFTyp)
{
	FrmlPtr Z = nullptr;
	char Typ, FTyp; FileDPtr cf, FD;
	bool b;
	WORD N; FrmlPtr Arg[30]; char Op;
	LocVar* LV = nullptr; LinkD* LD = nullptr;
	void* p; WORD* pofs = (WORD*)&p;

	if (IsKeyWord("EVALB")) { FTyp = 'B'; goto label4; }
	else if (IsKeyWord("EVALS")) { FTyp = 'S'; goto label4; }
	else if (IsKeyWord("EVALR")) {
		FTyp = 'R';
	label4:
		RdLex(); Z = GetOp(_eval, 5); Z->EvalTyp = FTyp; Z->P1 = RdStrFrml();
	}
	else if (FileVarsAllowed) Error(75);
	else if (IsKeyWord("PROMPT")) {
		RdLex(); Z = GetOp(_prompt, 4); Z->P1 = RdStrFrml();
		FieldDPtr F = RdFldDescr("", true);
		Z->FldD = F; FTyp = F->FrmlTyp; if (F->Typ == 'T') OldError(65);
		if (Lexem == _assign) {
			RdLex(); Z->P2 = RdFrml(Typ); if (Typ != FTyp) OldError(12);
		}
	}
	else if (IsKeyWord("KEYOF")) {
		RdLex(); FTyp = 'S'; if (!IsRecVar(LV)) { Op = _recno; goto label11; }
		Z = GetOp(_keyof, 8); Z->LV = LV;
		Z->PackKey = RdViewKeyImpl(Z->LV->FD); FTyp = 'S';
	}
	else if (IsKeyWord("RECNO")) { Op = _recno; goto label1; }
	else if (IsKeyWord("RECNOABS")) { Op = _recnoabs; goto label1; }
	else if (IsKeyWord("RECNOLOG")) {
		Op = _recnolog;
	label1:
		RdLex; FTyp = 'R';
	label11:
		FD = RdFileName(); KeyDPtr K = RdViewKeyImpl(FD);
		if (Op == _recno) {
			KeyFldDPtr KF = K->KFlds; N = 0;
			if (KF == nullptr) OldError(176);
			while (KF != nullptr) {
				Accept(','); N++;
				if (N > 30) Error(123); Arg[N] = RdFrml(Typ);
				if (Typ != KF->FldD->FrmlTyp) OldError(12); 
				KF = (KeyFldD*)KF->Chain;
			}
		}
		else { Accept(','); N = 1; Arg[1] = RdRealFrml(); }
		Z = GetOp(Op, (N + 2) * 4);
		Z->FD = FD; Z->Key = K;
		Move(Arg, Z->Arg, 4 * N);
		if (FTyp == 'R') goto label2;
	}
	else if (IsKeyWord("LINK")) {
		RdLex(); Z = GetOp(_link, 5);
		if (IsRecVar(LV)) {
			Z->LinkFromRec = true; Z->LinkLV = LV; FD = LV->FD;
		}
		else {
			FD = RdFileName(); Accept('[');
			Z->LinkRecFrml = RdRealFrml(); Accept(']');
		}
		Accept(',');
#ifdef FandSQL
		if (FD->typSQLFile) OldError(155);
#endif
		cf = CFile; CFile = FD;
		if (!IsRoleName(true, FD, LD) || (LD == nullptr)) Error(9);
		CFile = cf; Z->LinkLD = LD; FTyp = 'R'; goto label2;
	}
	else if (IsKeyWord("ISDELETED")) {
		RdLex(); FTyp = 'B';
		if (IsRecVar(LV)) {
			Z = GetOp(_lvdeleted, 4); Z->LV = LV;
		}
		else {
			Z = GetOp(_isdeleted, 4); FD = RdFileName(); Z->RecFD = FD;
			Accept(','); Z->P1 = RdRealFrml();
		label2: {}
#ifdef FandSQL
			if (FD->typSQLFile) Error(155);
#endif
		}
	}
	else if (IsKeyWord("GETPATH")) {
		RdLex(); Z = GetOp(_getpath, 0); Z->P1 = RdStrFrml(); FTyp = 'S';
	}
	else if (IsKeyWord("GETTXT")) {
		RdLex(); Z = GetOp(_gettxt, 6); FTyp = 'S'; goto label3;
	}
	else if (IsKeyWord("FILESIZE")) {
		RdLex(); Z = GetOp(_filesize, 14); FTyp = 'R';
	label3:
		RdPath(true, Z->TxtPath, Z->TxtCatIRec);
		if ((Z->Op == _gettxt) && (Lexem == ',')) {
			RdLex(); Z->P1 = RdRealFrml();
			if (Lexem == ',') { RdLex(); Z->P2 = RdRealFrml(); }
		}
	}
	else if (IsKeyWord("INTTSR")) {
		RdLex(); Z = GetOp(_inttsr, 5); Z->P1 = RdRealFrml(); Accept(',');
		Z->P2 = RdRealFrml(); Accept(','); Typ = 'r';
		if (IsRecVar(LV)) Z->P3 = (FrmlElem*)LV->RecPtr; else Z->P3 = RdFrml(Typ);
		Z->N31 = Typ;
		FTyp = 'R';
	}
#ifdef FandSQL
	else if (IsKeyWord("SQL")) {
		RdLex; Z = GetOp(_sqlfun, 0); Z->P1 = RdStrFrml(); FTyp = 'R';
	}
#endif
	else if (IsKeyWord("SELECTSTR")) {
		RdLex(); Z = GetOp(_selectstr, 13); FTyp = 'S'; RdSelectStr(Z);
	}
	else if (IsKeyWord("PROMPTYN")) {
		RdLex(); Z = GetOp(_promptyn, 0); Z->P1 = RdStrFrml(); FTyp = 'B';
	}
	else if (IsKeyWord("MOUSEEVENT")) {
		RdLex(); Z = GetOp(_mouseevent, 2); Z->W01 = RdInteger(); FTyp = 'B';
	}
	else if (IsKeyWord("ISMOUSE")) {
		RdLex(); Z = GetOp(_ismouse, 4); Z->W01 = RdInteger(); Accept(',');
		Z->W02 = RdInteger(); FTyp = 'B';
	}
	else if (IsKeyWord("MOUSEIN")) {
		RdLex(); Z = GetOp(_mousein, 4); Z->P1 = RdRealFrml(); Accept(',');
		Z->P2 = RdRealFrml(); Accept(','); Z->P3 = RdRealFrml(); Accept(',');
		Z->P4 = RdRealFrml(); FTyp = 'B';
	}
	else if (IsKeyWord("PORTIN")) {
		RdLex(); Z = GetOp(_portin, 0); Z->P1 = RdBool(); Accept(',');
		Z->P2 = RdRealFrml(); FTyp = 'R';
	}
	else Error(75);
	Accept(')');
	auto result = Z;
	FFTyp = FTyp;
	return result;
}

KeyD* RdViewKeyImpl(FileD* FD)
{
	KeyD* K = FD->Keys; if (K == nullptr) Error(24);
	if (Lexem == '/') {
		FileD* cf = CFile; CFile = FD; K = RdViewKey(); CFile = cf;
	}
	return K;
}

void RdSelectStr(FrmlPtr Z)
{
	Instr* PD;
	Z->Delim = 0x0D; // CTRL+M
	Z->P1 = RdRealFrml(); Accept(',');
	Z->P2 = RdRealFrml(); Accept(',');
	Z->P3 = RdStrFrml();
	while (Lexem == ',') {
		RdLex();
		if (IsOpt("HEAD")) Z->P4 = RdStrFrml();
		else if (IsOpt("FOOT")) Z->P5 = RdStrFrml();
		else if (IsOpt("MODE")) Z->P6 = RdStrFrml();
		else if (IsOpt("DELIM")) Z->Delim = RdQuotedChar();
		else Error(157);
	}
}

Instr* GetPInstr(PInstrCode Kind, WORD Size)
{
	//Instr* PD = (Instr*)GetZStore(Size + 5);
	Instr* PD = new Instr();
	PD->Kind = Kind;
	return PD;
}

void RdPInstrAndChain(Instr* PD)
{
	Instr* PD1 = RdPInstr; /*may be a chain itself*/
	Instr* PD2 = PD;
	if (PD2 != nullptr) {
		while (PD2->Chain != nullptr) PD2 = (Instr*)PD2->Chain;
		PD2->Chain = PD1;
	}
}

void RdChoices(Instr* PD)
{
	ChoiceD* CD; WORD N, SumL;
	AcceptKeyWord("OF"); N = 0; SumL = 0;
label1:
	if (IsKeyWord("ESCAPE")) {
		Accept(':');
		PD->WasESCBranch = true;
		PD->ESCInstr = RdPInstr;
	}
	else {
		CD = (ChoiceD*)GetZStore(sizeof(CD)); 
		ChainLast(PD->Choices, CD); N++;
		if ((PD->Kind == _menubar) && (N > 30)) Error(102);
		CD->TxtFrml = RdStrFrml();
		if (Lexem == ',') {
			RdLex();
			if (Lexem != ',') { CD->HelpName = RdHelpName(); PD->HelpRdb = CRdb; }
			if (Lexem == ',') {
				RdLex();
				if (Lexem != ',') {
					CD->Bool = RdBool();
					if (Lexem == '!') {
						CD->DisplEver = true; RdLex();
					}
				}
			};
		}
		Accept(':'); CD->Instr = RdPInstr;
	}
	if (Lexem == ';') {
		RdLex();
		if (IsKeyWord("END")) return;
		goto label1;
	}
	AcceptKeyWord("END");
}

void RdMenuAttr(Instr* PD)
{
	if (Lexem != ';') return;
	RdLex();
	/* !!! with PD^ do!!! */
	PD->mAttr[0] = RdAttr(); Accept(',');
	PD->mAttr[1] = RdAttr(); Accept(',');
	PD->mAttr[2] = RdAttr();
	if (Lexem == ',') { RdLex(); PD->mAttr[3] = RdAttr(); }
}

Instr* RdMenuBox(bool Loop)
{
	Instr* PD = nullptr; pstring* S = nullptr;
	PD = GetPInstr(_menubox, 48);
	auto result = PD;
	PD->Loop = Loop;
	if (Lexem == '(') {
		RdLex();
		if (Lexem != ';') {
			PD->X = RdRealFrml(); Accept(','); PD->Y = RdRealFrml();
		}
		RdMenuAttr(PD);
		Accept(')');
	}
	if (Lexem == '!') { RdLex(); PD->Shdw = true; }
	if (IsKeyWord("PULLDOWN")) PD->PullDown = true;
	if (!TestKeyWord("OF")) PD->HdLine = RdStrFrml();
	RdChoices(PD);
	return result;
}

Instr* RdMenuBar()
{
	Instr* PD = GetPInstr(_menubar, 48);
	auto result = PD;
	if (Lexem == '(') {
		RdLex();
		if (Lexem != ';') {
			PD->Y = RdRealFrml();
			if (Lexem == ',') {
				RdLex();
				PD->X = RdRealFrml();	Accept(',');
				PD->XSz = RdRealFrml();
			}
		}
		RdMenuAttr(PD);
		Accept(')');
	}
	RdChoices(PD);
	return result;
}

Instr* RdIfThenElse()
{
	auto PD = GetPInstr(_ifthenelseP, 12);
	auto result = PD;
	PD->Bool = RdBool();
	AcceptKeyWord("THEN");
	PD->Instr1 = RdPInstr;
	if (IsKeyWord("ELSE")) PD->ElseInstr1 = RdPInstr;
	return result;
}

Instr* RdWhileDo()
{
	Instr* PD = GetPInstr(_whiledo, 8);
	auto result = PD;
	PD->Bool = RdBool();
	AcceptKeyWord("DO");
	PD->Instr1 = RdPInstr;
	return result;
}

Instr* RdFor()
{
	LocVar* LV = nullptr;
	if (!FindLocVar(LVBD.Root, LV) || (LV->FTyp != 'R')) Error(146);
	RdLex();
	Instr* PD = GetPInstr(_asgnloc, 9);
	auto result = PD;
	PD->AssLV = LV;
	Accept(_assign);
	PD->Frml = RdRealFrml(); AcceptKeyWord("TO");
	PD->Chain = GetPInstr(_whiledo, 8);
	PD = (Instr*)PD->Chain;
	FrmlPtr Z = GetOp(_compreal, 2);
	Z->P1 = (FrmlElem*)LV->Op;
	Z->N21 = _le;
	Z->N22 = 5;
	Z->P2 = RdRealFrml();
	PD->Bool = Z;
	AcceptKeyWord("DO");
	Instr* PD1 = RdPInstr;
	PD->Instr1 = PD1;
	PD1 = GetPInstr(_asgnloc, 9);
	ChainLast(PD->Instr1, PD1);
	PD1->Add = true;
	PD1->AssLV = LV;
	Z = GetOp(_const, sizeof(double));
	Z->R = 1;
	PD1->Frml = Z;
	return result;
}

Instr* RdCase()
{
	Instr* PD = nullptr;
	bool first = true;
label1:
	Instr* PD1 = GetPInstr(_ifthenelseP, 12);
	Instr* result = nullptr;
	if (first) result = PD1;
	else PD->ElseInstr1 = PD1;
	PD = PD1; first = false;
	PD->Bool = RdBool(); Accept(':');
	PD->Instr1 = RdPInstr;
	bool b = Lexem = ';';
	if (b) RdLex();
	if (!IsKeyWord("END"))
		if (IsKeyWord("ELSE"))
			while (!IsKeyWord("END")) {
				RdPInstrAndChain(PD->ElseInstr1);
				if (Lexem == ';') RdLex();
				else goto label2;
			}
		else if (b) goto label1;
		else {
		label2:
			AcceptKeyWord("END");
		}
	return result;
}

Instr* RdRepeatUntil()
{
	Instr* PD = GetPInstr(_repeatuntil, 8);
	auto result = PD;
	while (!IsKeyWord("UNTIL")) {
		RdPInstrAndChain(PD->Instr1);
		if (Lexem == ';') RdLex();
		else {
			AcceptKeyWord("UNTIL");
			goto label1;
		}
	}
label1: PD->Bool = RdBool();
	return result;
}

Instr* RdForAll()
{
	LocVar* LVi = nullptr; LocVar* LVr = nullptr;
	LinkD* LD = nullptr; FrmlPtr Z = nullptr;
	if (!FindLocVar(LVBD.Root, LVi)) Error(122);
	RdLex();
	if (LVi->FTyp == 'r') { LVr = LVi; LVi = nullptr; CFile = LVr->FD; }
	else {
		TestReal(LVi->FTyp); AcceptKeyWord("IN");
		if (FindLocVar(LVBD.Root, LVr)) {
			if (LVr->FTyp == 'f') { CFile = LVr->FD; RdLex(); goto label1; }
			if (LVr->FTyp != 'r') Error(141);
			CFile = LVr->FD; RdLex();
		}
		else {
			CFile = RdFileName();
		label1:
			LVr = nullptr;
		}
#ifdef FandSQL
		if (CFile->typSQLFile) OldError(155);
#endif
	}
	Instr* PD = GetPInstr(_forall, 41);
	PD->CFD = CFile; PD->CVar = LVi; PD->CRecVar = *LVr;
#ifdef FandSQL
	if (CFile->typSQLFile && IsKeyWord("IN")) {
		AcceptKeyWord("SQL"); Accept('('); PD->CBool = RdStrFrml(); Accept(')');
		PD->inSQL = true; goto label2;
	}
#endif
	if (IsKeyWord("OWNER")) {
		/* !!! with PD^ do!!! */
		PD->COwnerTyp = RdOwner(PD->CLD, PD->CLV); CViewKey = GetFromKey(PD->CLD);
	}
	else CViewKey = RdViewKey();
	if (Lexem == '(') {
		/* !!! with PD^ do!!! */
		RdLex();
		PD->CBool = RdKeyInBool(PD->CKIRoot, false, true, PD->CSQLFilter);
		if ((PD->CKIRoot != nullptr) && (PD->CLV != nullptr)) OldError(118);
		Accept(')');
	}
	if (Lexem == '!') { RdLex(); PD->CWIdx = true; }
	if (Lexem == '%') { RdLex(); PD->CProcent = true; }
	PD->CKey = CViewKey;
label2:
	AcceptKeyWord("DO"); PD->CInstr = RdPInstr;
	return PD;
}

Instr* RdBeginEnd()
{
	Instr* PD = nullptr;
	if (!IsKeyWord("END")) {
	label1:
		RdPInstrAndChain(PD);
		if (Lexem == ';') {
			RdLex();
			if (!IsKeyWord("END")) goto label1;
		}
		else AcceptKeyWord("END");
	}
	return PD;
}

Instr* RdProcArg(char Caller)
{
	RdbPos Pos; TypAndFrml TArg[31];
	LocVar* LV = nullptr;
	if (Caller != 'C') RdChptName('P', &Pos, Caller == 'P' || Caller == 'E' || Caller == 'T');
	WORD N = 0;
	if (Caller != 'P') { if (Lexem == '(') { RdLex(); goto label1; } }
	else if (Lexem == ',') {
		RdLex(); Accept('(');
	label1:
		N++;
		if (N > 30) Error(123);
		FillChar(&TArg[N].FTyp, sizeof(TypAndFrml), 0);
		if ((ForwChar != '.') && FindLocVar(LVBD.Root, LV) && (LV->FTyp == 'i' || LV->FTyp == 'r'))
		{
			RdLex(); TArg[N].FTyp = LV->FTyp; TArg[N].FD = LV->FD; TArg[N].RecPtr = LV->RecPtr;
		}
		else if (Lexem == '@')
		{
			RdLex();
			if (Lexem == '[') {
				RdLex(); TArg[N].Name = *StoreStr(LexWord); Accept(_identifier);
				Accept(','); FrmlPtr z = GetOp(_setmybp, 0);
				z->P1 = RdStrFrml();
				TArg[N].TxtFrml = z;
				Accept(']');
			}
			else TArg[N].FD = RdFileName(); TArg[N].FTyp = 'f';
		}
		else TArg[N].Frml = RdFrml(TArg[N].FTyp);
		if (Lexem == ',') { RdLex(); goto label1; }
		Accept(')');
	}
	if (Caller == 'E') { N++; TArg[N].FTyp = 'r'; }
	WORD L = N * sizeof(TypAndFrml);
	Instr* PD = GetPInstr(_proc, sizeof(RdbPos) + 2 + L);
	PD->Pos = Pos; PD->N = N; Move(TArg, PD->TArg, L);
	PD->ExPar = (Caller = 'E');
	return PD;
}

void RdKeyCode(EdExitD* X)
{
	WORD i; EdExKeyD* E;
	E = (EdExKeyD*)GetStore(sizeof(EdExKeyD));
	E->Chain = X->Keys; X->Keys = E;
	if (NotCode('F', _F1_, 21, E)
		&& NotCode("ShiftF", _ShiftF1_, 1, E)
		&& NotCode("CtrlF", _CtrlF1_, 31, E)
		&& NotCode("AltF", _AltF1_, 41, E))
	{
		for (i = 0; i < NKeyNames; i++)
		{
			/* !!! with KeyNames[i] do!!! */
			if (EquUpcase(KeyNames[i].Nm, LexWord)) {
				E->KeyCode = KeyNames[i].Code;
				E->Break = KeyNames[i].Brk;
				RdLex();
				return;
			}
			Error(129);
		}
	}
}

bool NotCode(pstring Nm, WORD CodeBase, WORD BrkBase, EdExKeyD* E)
{
	WORD i, k;
	auto result = true;
	if (Lexem != _identifier) return result;
	if (!SEquUpcase(copy(LexWord, 1, Nm.length()), Nm)) return result;
	val(copy(LexWord, Nm.length() + 1, 2), i, k);
	if ((k != 0) || (i <= 0) || (i > 10)) return result;
	i--; RdLex();
	E->KeyCode = CodeBase + (i << 8);
	E->Break = BrkBase + i;
	result = false;
	return result;
}

bool RdHeadLast(void* AA)
{
	struct structA { FrmlPtr Head; FrmlPtr Last; FrmlPtr CtrlLast; FrmlPtr AltLast; FrmlPtr ShiftLast; };
	structA* A = (structA*)&AA;
	auto result = true;
	if (IsOpt("HEAD")) A->Head = RdStrFrml();
	else if (IsOpt("LAST")) A->Last = RdStrFrml();
	else if (IsOpt("CTRL")) A->CtrlLast = RdStrFrml();
	else if (IsOpt("ALT")) A->AltLast = RdStrFrml();
	else if (IsOpt("SHIFT")) A->ShiftLast = RdStrFrml();
	else result = false;
	return result;
}

bool RdViewOpt(EditOpt* EO)
{
	FileD* FD = nullptr;
	RprtOpt* RO = nullptr;
	bool Flgs[23];
	auto result = false;
	/* !!! with EO^ do!!! */
	RdLex(); result = true;
	CViewKey = EO->ViewKey;
	if (IsOpt("TAB")) RdNegFldList(EO->NegTab, EO->Tab);
	else if (IsOpt("DUPL")) RdNegFldList(EO->NegDupl, EO->Dupl);
	else if (IsOpt("NOED")) RdNegFldList(EO->NegNoEd, EO->NoEd);
	else if (IsOpt("MODE")) {
		SkipBlank(false);
		if ((Lexem == _quotedstr) && (ForwChar == ',' || ForwChar == ')')) {
			EditModeToFlags(LexWord, Flgs, true);
			EO->Mode = GetOp(_const, LexWord.length() + 1);
			EO->Mode->S = LexWord; RdLex();
		}
		else EO->Mode = RdStrFrml();
	}
	else if (RdHeadLast(EO->Head)) return result;
	else if (IsOpt("WATCH")) EO->WatchDelayZ = RdRealFrml();
	else if (IsOpt("WW")) {
		Accept('('); EO->WFlags = 0;
		if (Lexem == '(') { RdLex(); EO->WFlags = WNoPop; }
		RdW(EO->W); RdFrame(EO->Top, EO->WFlags);
		if (Lexem == ',') {
			RdLex();
			EO->ZAttr = RdAttr(); Accept(',');
			EO->ZdNorm = RdAttr(); Accept(',');
			EO->ZdHiLi = RdAttr();
			if (Lexem == ',') {
				RdLex();
				EO->ZdSubset = RdAttr();
				if (Lexem == ',') {
					RdLex();
					EO->ZdDel = RdAttr();
					if (Lexem == ',') {
						RdLex();
						EO->ZdTab = RdAttr();
						if (Lexem == ',') {
							RdLex();
							EO->ZdSelect = RdAttr();
						}
					}
				}
			}
		}
		Accept(')');
		if ((EO->WFlags & WNoPop) != 0) Accept(')');
	}
	else if (IsOpt("EXIT")) {
		Accept('(');
	label1:
		EdExitD* X = (EdExitD*)GetZStore(sizeof(*X));
		ChainLast(EO->ExD, X);
		RdKeyList(X);
		if (IsKeyWord("QUIT")) X->Typ = 'Q';
		else if (IsKeyWord("REPORT")) {
			if (X->AtWrRec || (EO->LVRecPtr != nullptr)) OldError(144);
			Accept('(');
			X->Typ = 'R';
			RO = GetRprtOpt();
			RdChptName('R', &RO->RprtPos, true);
			while (Lexem == ',') {
				RdLex();
				if (IsOpt("ASSIGN")) RdPath(true, RO->Path, RO->CatIRec);
				else if (IsKeyWord("EDIT")) RO->Edit = true;
				else Error(130);
			}
			X->RO = RO; Accept(')');
		}
		else if (!(Lexem == ',' || Lexem == ')')) {
			X->Typ = 'P';
			X->Proc = RdProcArg('E');
		}
		if (Lexem == ',') { RdLex(); goto label1; }
		Accept(')');
	}
	else if (EO->LVRecPtr != nullptr) result = false;
	else if (IsOpt("COND")) {
		if (Lexem == '(') {
			RdLex();
			EO->Cond = RdKeyInBool(EO->KIRoot, false, true, EO->SQLFilter);
			Accept(')');
		}
		else EO->Cond = RdKeyInBool(EO->KIRoot, false, true, EO->SQLFilter);
	}
	else if (IsOpt("JOURNAL")) {
		EO->Journal = RdFileName();
		WORD l = EO->Journal->RecLen - 13;
		if (CFile->Typ == 'X') l++;
		if (CFile->RecLen != l) OldError(111);
	}
	else if (IsOpt("SAVEAFTER")) EO->SaveAfterZ = RdRealFrml();
	else if (IsOpt("REFRESH")) EO->RefreshDelayZ = RdRealFrml();
	else result = false;
	return result;
}

void RdKeyList(EdExitD* X)
{
label1:
	if ((Lexem == '(') || (Lexem == '^')) RdNegFldList(X->NegFlds, X->Flds);
	else if (IsKeyWord("RECORD")) X->AtWrRec = true;
	else if (IsKeyWord("NEWREC")) X->AtNewRec = true;
	else RdKeyCode(X);
	if (Lexem == ',') { RdLex(); goto label1; }
	Accept(':');
}

Instr* GetPD(PInstrCode Kind, WORD Size)
{
	Instr* PD = GetPInstr(Kind, Size);
	RdLex();
	RdPInstr = PD;
	return PD;
}

void RdProcCall()
{
	Instr* PD;
	if (IsKeyWord("EXEC")) RdExec();
	else if (IsKeyWord("COPYFILE")) RdCopyFile();
	else if (IsKeyWord("PROC")) { RdLex(); RdPInstr = RdProcArg('P'); }
	else if (IsKeyWord("DISPLAY")) RdDisplay();
	else if (IsKeyWord("CALL")) RdRDBCall();
	else if (IsKeyWord("WRITELN")) RdWriteln(1);
	else if (IsKeyWord("WRITE")) RdWriteln(0);
	else if (IsKeyWord("HEADLINE")) { PD = GetPD(_headline, 4); goto label1; }
	else if (IsKeyWord("SETKEYBUF")) { PD = GetPD(_setkeybuf, 4); goto label1; }
	else if (IsKeyWord("HELP")) {
		PD = GetPD(_help, 8); if (CRdb->HelpFD == nullptr) OldError(132);
		PD->HelpRdb = CRdb;
	label1: PD->Frml = RdStrFrml();
	}
	else if (IsKeyWord("MESSAGE")) RdWriteln(2);
	else if (IsKeyWord("GOTOXY")) RdGotoXY();
	else if (IsKeyWord("MERGE")) {
		PD = GetPD(_merge, sizeof(RdbPos)); RdChptName('M', &PD->Pos, true);
	}
	else if (IsKeyWord("SORT")) RdSortCall();
	else if (IsKeyWord("EDIT")) RdEditCall();
	else if (IsKeyWord("REPORT")) RdReportCall();
	else if (IsKeyWord("EDITTXT")) RdEditTxt();
	else if (IsKeyWord("PRINTTXT")) RdPrintTxt();
	else if (IsKeyWord("PUTTXT")) RdPutTxt();
	else if (IsKeyWord("TURNCAT")) RdTurnCat();
	else if (IsKeyWord("RELEASEDRIVE")) RdReleaseDrive();
	else if (IsKeyWord("SETPRINTER")) { PD = GetPD(_setprinter, 4); goto label2; }
	else if (IsKeyWord("INDEXFILE")) RdIndexfile();
	else if (IsKeyWord("GETINDEX")) RdGetIndex();
	else if (IsKeyWord("MOUNT")) RdMount();
	else if (IsKeyWord("CLRSCR")) RdClrWw();
	else if (IsKeyWord("APPENDREC")) RdMixRecAcc(_appendrec);
	else if (IsKeyWord("DELETEREC")) RdMixRecAcc(_deleterec);
	else if (IsKeyWord("RECALLREC")) RdMixRecAcc(_recallrec);
	else if (IsKeyWord("READREC")) RdMixRecAcc(_readrec);
	else if (IsKeyWord("WRITEREC")) RdMixRecAcc(_writerec);
	else if (IsKeyWord("LINKREC")) RdLinkRec();
	else if (IsKeyWord("DELAY")) { PD = GetPD(_delay, 4); goto label2; }
	else if (IsKeyWord("SOUND")) {
		PD = GetPD(_sound, 4);
	label2:
		PD->Frml = RdRealFrml();
	}
#ifdef FandProlog
	else if (IsKeyWord("LPROC")) RdCallLProc;
#endif

#ifdef FandGraph
	else if (IsKeyWord("GRAPH")) RdGraphP;
	else if (IsKeyWord("PUTPIXEL")) { PD = GetPD(_putpixel, 3 * 4); goto label3; }
	else if (IsKeyWord("LINE")) { PD = GetPD(_line, 5 * 4); goto label3; }
	else if (IsKeyWord("RECTANGLE")) { PD = GetPD(_rectangle, 5 * 4); goto label3; }
	else if (IsKeyWord("ELLIPSE")) { PD = GetPD(_ellipse, 7 * 4); goto label3; }
	else if (IsKeyWord("FLOODFILL")) { PD = GetPD(_floodfill, 5 * 4); goto label3; }
	else if (IsKeyWord("OUTTEXTXY")) {
		PD = GetPD(_outtextxy, 11 * 4);
	label3:
		PD->Par1 = RdRealFrml(); Accept(','); PD->Par2 = RdRealFrml(); Accept(',');
		if (PD->Kind == _outtextxy) {
			PD->Par3 = RdStrFrml(); Accept(',');
			PD->Par4 = RdRealFrml(); Accept(','); PD->Par5 = RdAttr();
			if (Lexem == ',') {
				RdLex(); PD->Par6 = RdRealFrml();
				if (Lexem == ',') {
					RdLex(); PD->Par7 = RdRealFrml();
					if (Lexem == ',') {
						RdLex();
						PD->Par8 = RdRealFrml(); Accept(',');
						PD->Par9 = RdRealFrml(); Accept(',');
						PD->Par10 = RdRealFrml(); Accept(',');
						PD->Par11 = RdRealFrml();
					}
				}
			}
		}
		else if (PD->Kind == _putpixel) PD->Par3 = RdAttr(); else {
			PD->Par3 = RdRealFrml(); Accept(',');
			if (PD->Kind == _floodfill) PD->Par4 = RdAttr(); else PD->Par4 = RdRealFrml();
			Accept(','); PD->Par5 = RdAttr();
			if ((PD->Kind == _ellipse) && (Lexem == ',')) {
				RdLex(); PD->Par6 = RdRealFrml(); Accept(','); PD->Par7 = RdRealFrml();
			}
		}
	}
#endif 
	else if (IsKeyWord("CLOSE")) {
		PD = GetPD(_closefds, 4); PD->clFD = RdFileName();
	}
	else if (IsKeyWord("BACKUP")) RdBackup(' ', true);
	else if (IsKeyWord("BACKUPM")) RdBackup('M', true);
	else if (IsKeyWord("RESTORE")) RdBackup(' ', false);
	else if (IsKeyWord("RESTOREM")) RdBackup('M', false);
	else if (IsKeyWord("SETEDITTXT")) RdSetEditTxt();
	else if (IsKeyWord("SETMOUSE")) {
		PD = GetPD(_setmouse, 12); PD->MouseX = RdRealFrml(); Accept(',');
		PD->MouseY = RdRealFrml(); Accept(','); PD->Show = RdBool();
	}
	else if (IsKeyWord("CHECKFILE")) {
		PD = GetPD(_checkfile, 10); PD->cfFD = RdFileName();
		/* !!! with PD->cfFD^ do!!! */
		if ((PD->cfFD->Typ == '8' || PD->cfFD->Typ == 'D')
#ifdef FandSQL
			|| PD->cfFD->typSQLFile
#endif
			) OldError(169);
		Accept(','); RdPath(true, PD->cfPath, PD->cfCatIRec);
	}
#ifdef FandSQL
	else if (IsKeyWord("SQL")) { PD = GetPD(_sql, 4); goto label1; }
	else if (IsKeyWord("LOGIN")) {
		PD = GetPD(_login, 8);
		/* !!! with PD^ do!!! */ {
			PD->liName = RdStrFrml(); Accept(','); PD->liPassWord = RdStrFrml(); }
	}
	else if (IsKeyWord("SQLRDTXT")) RdSqlRdWrTxt(true);
	else if (IsKeyWord("SQLWRTXT")) RdSqlRdWrTxt(false);
#endif 
	else if (IsKeyWord("PORTOUT")) {
		PD = GetPD(_portout, 12);
		/* !!! with PD^ do!!! */
		{
			PD->IsWord = RdBool(); Accept(',');
			PD->Port = RdRealFrml(); Accept(',');
			PD->PortWhat = RdRealFrml();
		}
	}
	else Error(34);
	Accept(')');
}

FieldList RdFlds()
{
	FieldList FLRoot, FL;
	FLRoot = nullptr;
label1:
	FL = (FieldList)GetStore(sizeof(*FL));
	ChainLast(FLRoot, FL);
	FL->FldD = RdFldName(CFile);
	if (Lexem == ',') { RdLex(); goto label1; }
	return FLRoot;
}

FieldList RdSubFldList(FieldList InFL, char Opt)
{
	FieldList FLRoot, FL, FL1; FieldDPtr F;
	Accept('('); FLRoot = nullptr;
label1:
	FL = (FieldList)GetStore(sizeof(*FL));
	ChainLast(FLRoot, FL);
	if (InFL == nullptr) F = RdFldName(CFile);
	else {
		TestIdentif(); FL1 = InFL;
		while (FL1 != nullptr)
		{
			if (EquUpcase(FL1->FldD->Name, LexWord)) goto label2;
			FL1 = (FieldList)FL1->Chain;
		}
		Error(43);
	label2:
		F = FL1->FldD; RdLex();
	}
	FL->FldD = F;
	if ((Opt == 'S') && (F->FrmlTyp != 'R')) OldError(20);
	if (Lexem == ',') { RdLex(); goto label1; }
	Accept(')');
	return FLRoot;
}

void RdSortCall()
{
	Instr* PD = GetPD(_sort, 8);
	FileD* FD = RdFileName();
	PD->SortFD = FD;
#ifdef FandSQL
	if (FD->typSQLFile) OldError(155);
#endif
	Accept(',');
	Accept('(');
	RdKFList(PD->SK, PD->SortFD);
	Accept(')');
}

void RdEditCall()
{
	Instr* PD; EditOpt* EO; void* p; bool b; KeyDPtr K;
	LocVar* lv = nullptr;
	PD = GetPD(_edit, 8);
	EO = GetEditOpt();
	PD->EO = EO;
	if (IsRecVar(lv)) { EO->LVRecPtr = lv->RecPtr; CFile = lv->FD; }
	else {
		CFile = RdFileName();
		K = RdViewKey();
		if (K == nullptr) K = CFile->Keys;
		EO->ViewKey = K;
	}
	PD->EditFD = CFile;
	Accept(',');
	if (IsOpt('U')) {
		TestIdentif();
		if (CFile->ViewNames == nullptr) Error(114);
		p = SaveCompState();
		b = RdUserView(LexWord, EO);
		RestoreCompState(p);
		if (!b) Error(114);
		RdLex();
	}
	else RdBegViewDcl(EO);
	while (Lexem == ',') {
		b = RdViewOpt(EO);
		if (!b) RdEditOpt(EO);
	}
}

void RdEditOpt(EditOpt* EO)
{
	LocVar* lv;
	/* !!! with EO^ do!!! */
	if (IsOpt("FIELD")) EO->StartFieldZ = RdStrFrml();
	else if (EO->LVRecPtr != nullptr) Error(125);
	else if (IsOpt("OWNER")) {
		if (EO->SQLFilter || (EO->KIRoot != nullptr)) OldError(179);
		EO->OwnerTyp = RdOwner(EO->DownLD, EO->DownLV);
	}
	else if (IsOpt("RECKEY")) EO->StartRecKeyZ = RdStrFrml();
	else if (
#ifdef FandSQL
		!CFile->typSQLFile &&
#endif
		IsOpt("RECNO")) EO->StartRecNoZ = RdRealFrml();
	else if (IsOpt("IREC")) EO->StartIRecZ = RdRealFrml();
	else if (IsKeyWord("CHECK")) EO->SyntxChk = true;
	else if (IsOpt("SEL")) {
		lv = RdIdxVar();
		EO->SelKey = WKeyDPtr(lv->RecPtr);
		if ((EO->ViewKey == nullptr)) OldError(108);
		if (EO->ViewKey == EO->SelKey) OldError(184);
		if ((EO->ViewKey->KFlds != nullptr)
			&& (EO->SelKey->KFlds != nullptr)
			&& !EquKFlds(EO->SelKey->KFlds, EO->ViewKey->KFlds)) OldError(178);
	}
	else Error(125);
}

void RdReportCall()
{
	Instr* PD = nullptr; RprtOpt* RO = nullptr; LocVar* lv = nullptr;
	RprtFDListEl* FDL = nullptr; bool b = false;
	bool hasfrst;
	PD = GetPD(_report, 4); RO = GetRprtOpt(); PD->RO = RO;
	hasfrst = false;
	if (Lexem == ',') goto label2; hasfrst = true;
	FDL = &RO->FDL; b = false;
	if (Lexem == '(') { RdLex(); b = true; }
label1:
	if (IsRecVar(lv)) { FDL->LVRecPtr = lv->RecPtr; FDL->FD = lv->FD; }
	else {
		CFile = RdFileName(); FDL->FD = CFile;
		CViewKey = RdViewKey(); FDL->ViewKey = CViewKey;
		if (Lexem == '(') {
			RdLex();
			FDL->Cond = RdKeyInBool(FDL->KeyIn, true, true, FDL->SQLFilter);
			Accept(')');
		}
	}
	if (b && (Lexem = ',')) {
		RdLex();
		FDL->Chain = (RprtFDListEl*)GetZStore(sizeof(RprtFDListEl));
		FDL = FDL->Chain; goto label1;
	}
	if (b) Accept(')');
	CFile = RO->FDL.FD; CViewKey = RO->FDL.ViewKey;
label2: Accept(',');
	if (Lexem == '[') {
		RdLex(); RO->RprtPos.R = RdbDPtr(RdStrFrml); RO->RprtPos.IRec = 0;
		RO->FromStr = true; Accept(']');
	}
	else if (!hasfrst || (Lexem == _identifier)) {
		TestIdentif();
		if (!FindChpt('R', LexWord, false, &RO->RprtPos)) Error(37);
		RdLex();
	}
	else {
		Accept('(');
		switch (Lexem) {
		case '?': { RO->Flds = AllFldsList(CFile, false);
				RdLex(); RO->UserSelFlds = true; break; }
		case ')': RO->Flds = AllFldsList(CFile, true); break;
		default: {
			RO->Flds = RdFlds();
			if (Lexem == '?') { RdLex(); RO->UserSelFlds = true; }
			break;
		}
			   Accept(')');
		}
	}
	while (Lexem == ',') {
		RdLex(); RdRprtOpt(RO, (hasfrst && (FDL->LVRecPtr == nullptr)));
	}
	if ((RO->Mode = _ALstg) && ((RO->Ctrl != nullptr) || (RO->Sum != nullptr)))
		RO->Mode = _ARprt;
}

void RdRprtOpt(RprtOpt* RO, bool HasFrst)
{
	FileD* FD;
	WORD N = 0;
	/* !!! with RO^ do!!! */
	if (IsOpt("ASSIGN")) RdPath(true, RO->Path, RO->CatIRec);
	else if (IsOpt("TIMES")) RO->Times = RdRealFrml();
	else if (IsOpt("MODE"))
		if (IsKeyWord("ONLYSUM")) RO->Mode = _ATotal;
		else if (IsKeyWord("ERRCHECK")) RO->Mode = _AErrRecs; else Error(49);
	else if (IsKeyWord("COND")) {
		if (!HasFrst) goto label2;
		WORD Low = CurrPos; Accept(_equ); bool br = false;
		if (Lexem == '(') {
			Low = CurrPos; RdLex(); br = true;
			if (Lexem == '?') { RdLex(); RO->UserCondQuest = true; goto label1; };
		}
		RO->FDL.Cond = RdKeyInBool(RO->FDL.KeyIn, true, true, RO->FDL.SQLFilter);
		N = OldErrPos - Low; RO->CondTxt = (pstring*)GetStore(N + 1);
		Move(&InpArrPtr[Low], &RO->CondTxt[1], N); RO->CondTxt[0] = char(N);
	label1:
		if (br) Accept(')');
	}
	else if (IsOpt("CTRL")) {
		if (!HasFrst) goto label2;
		RO->Ctrl = RdSubFldList(RO->Flds, 'C');
	}
	else if (IsOpt("SUM")) {
		if (!HasFrst) goto label2;
		RO->Sum = RdSubFldList(RO->Flds, 'S');
	}
	else if (IsOpt("WIDTH")) RO->WidthFrml = RdRealFrml();
	else if (IsOpt("STYLE"))
		if (IsKeyWord("COMPRESSED")) RO->Style = 'C'; else
			if (IsKeyWord("NORMAL")) RO->Style = 'N'; else Error(50);
	else if (IsKeyWord("EDIT")) RO->Edit = true;
	else if (IsKeyWord("PRINTCTRL")) RO->PrintCtrl = true;
	else if (IsKeyWord("CHECK")) RO->SyntxChk = true;
	else if (IsOpt("SORT")) {
		if (!HasFrst)
			label2:
		OldError(51);
		Accept('('); RdKFList(RO->SK, CFile); Accept(')');
	}
	else if (IsOpt("HEAD")) RO->Head = RdStrFrml();
	else Error(45);
}

void RdRDBCall()
{
	Instr* PD; pstring s;
	PD = GetPD(_call, 12);
	s[0] = 0;
	if (Lexem == '\\') { s = Lexem; RdLex(); }
	TestIdentif();
	if (LexWord.length() > 8) Error(2);
	PD->RdbNm = StoreStr(s + LexWord); RdLex();
	if (Lexem = ',') {
		RdLex();
		TestIdentif();
		if (LexWord.length() > 12) Error(2);
		PD->ProcNm = StoreStr(LexWord);
		RdLex();
		PD->ProcCall = RdProcArg('C');
	}
	else PD->ProcNm = StoreStr("main");
}

void RdExec()
{
	Instr* PD; FileDPtr FD;
	PD = GetPD(_exec, 14); RdPath(true, PD->ProgPath, PD->ProgCatIRec);
	Accept(','); PD->Param = RdStrFrml();
	while (Lexem == ',') {
		RdLex();
		if (IsKeyWord("NOCANCEL")) PD->NoCancel = true;
		else if (IsKeyWord("FREEMEM")) PD->FreeMm = true;
		else if (IsKeyWord("LOADFONT")) PD->LdFont = true;
		else if (IsKeyWord("TEXTMODE")) PD->TextMd = true;
		else Error(101);
	}
}

void RdCopyFile()
{
	pstring ModeTxt[7] = { "KL","LK","KN","LN","LW","KW","WL" };
	Instr* PD; FieldDPtr* F; WORD i; CopyD* D; bool noapp;
	PD = GetPD(_copyfile, 4); noapp = false;
	D = (CopyD*)GetZStore(sizeof(*D));
	PD->CD = D;
	/* !!! with D^ do!!! */
	D->FD1 = RdPath(false, D->Path1, D->CatIRec1); D->WithX1 = RdX(D->FD1);
	if (Lexem == '/')
	{
		if (D->FD1 != nullptr) { CFile = D->FD1; D->ViewKey = RdViewKey(); }
	}
	else D->Opt1 = RdCOpt();
	Accept(',');
	D->FD2 = RdPath(false, D->Path2, D->CatIRec2); D->WithX2 = RdX(D->FD2);
	if (Lexem == '/') { if (D->FD2 != nullptr) Error(139); else D->Opt2 = RdCOpt(); }
	if (!TestFixVar(D->Opt1, D->FD1, D->FD2) && !TestFixVar(D->Opt2, D->FD2, D->FD1))
	{
		if ((D->Opt1 == cpTxt) && (D->FD2 != nullptr)) OldError(139);
		noapp = (D->FD1 == nullptr) ^ (D->FD2 == nullptr); // XOR
#ifdef FandSQL
		if (noapp)
			if ((FD1 != nullptr) && (FD1->typSQLFile) || (FD2 != nullptr)
				&& (FD2->typSQLFile)) OldError(155);
#endif
	}
	while (Lexem == ',') {
		RdLex();
		if (IsOpt("HEAD")) {
			D->HdFD = RdFileName();
			Accept('.');
			D->HdF = RdFldName(D->HdFD);
			if ((D->HdF->FrmlTyp != 'S') || !D->HdFD->IsParFile
				|| (D->Opt1 == cpFix || D->Opt1 == cpVar)
				&& (D->HdF->Flg && f_Stored == 0)) Error(52);
		}
		else if (IsOpt("MODE")) {
			TestLex(_quotedstr);
			for (i = 0; i < 7; i++)
				if (SEquUpcase(LexWord, ModeTxt[i])) { D->Mode = i; goto label1; }
			Error(142);
		label1:
			RdLex();
		}
		else if (IsKeyWord("NOCANCEL")) D->NoCancel = true;
		else if (IsKeyWord("APPEND")) {
			if (noapp) OldError(139); D->Append = true;
		}
		else Error(52);
	}
}

CpOption RdCOpt()
{
	BYTE i;
	pstring OptArr[3] = { "FIX", "VAR", "TXT" };
	RdLex(); TestIdentif();
	for (i = 0; i < 3; i++)
		if (EquUpcase(OptArr[i], LexWord)) { RdLex(); return CpOption(i); }
	Error(53);
	throw std::exception("Bad value in RdCOpt() in rdproc.cpp");
}

bool RdX(FileD* FD)
{
	auto result = false; if ((Lexem == '.') && (FD != nullptr)) {
		RdLex();
		AcceptKeyWord('X');
		if (FD->Typ != 'X') OldError(108);
		result = true;
	}
	return result;
}

bool TestFixVar(CpOption Opt, FileD* FD1, FileD* FD2)
{
	auto result = false;
	if ((Opt != cpNo) && (FD1 != nullptr)) OldError(139);
	result = false;
	if (Opt == cpFix || Opt == cpVar) {
		result = true;
		if (FD2 == nullptr) OldError(139);
	}
	return result;
}

bool RdList(pstring* S)
{
	auto result = false;
	if (Lexem != '(') return result;
	RdLex();
	S = (pstring*)(RdStrFrml);
	Accept(')');
	result = true;
	return result;
}

void RdPrintTxt()
{
	Instr* PD;
	PD = GetPD(_printtxt, 10);
	/* !!! with PD^ do!!! */
	if (FindLocVar(LVBD.Root, PD->TxtLV)) { RdLex(); TestString(PD->TxtLV->FTyp); }
	else RdPath(true, PD->TxtPath, PD->TxtCatIRec);
}

void RdEditTxt()
{
	Instr* PD; EdExitD* pX;
	PD = GetPD(_edittxt, 73);
	/* !!! with PD^ do!!! */
	if (FindLocVar(LVBD.Root, PD->TxtLV)) { RdLex(); TestString(PD->TxtLV->FTyp); }
	else RdPath(true, PD->TxtPath, PD->TxtCatIRec);
	PD->EdTxtMode = 'T';
	while (Lexem == ',') {
		RdLex();
		if (IsOpt("WW")) {
			Accept('(');
			if (Lexem == '(') { RdLex(); PD->WFlags = WNoPop; }
			RdW(PD->Ww); RdFrame(PD->Hd, PD->WFlags);
			if (Lexem == ',') { RdLex(); PD->Atr = RdAttr(); }
			Accept(')');
			if ((PD->WFlags && WNoPop) != 0) Accept(')');
		}
		else
			if (IsOpt("TXTPOS")) PD->TxtPos = RdRealFrml();
			else if (IsOpt("TXTXY")) PD->TxtXY = RdRealFrml();
			else if (IsOpt("ERRMSG")) PD->ErrMsg = RdStrFrml();
			else if (IsOpt("EXIT")) {
				Accept('(');
			label1:
				pX = (EdExitD*)GetZStore(sizeof(*pX));
				ChainLast(PD->ExD, pX);
			label2:
				RdKeyCode(pX); if (Lexem == ',') { RdLex(); goto label2; }
				Accept(':');
				if (IsKeyWord("QUIT")) pX->Typ = 'Q';
				else if (!(Lexem == ',' || Lexem == ')')) {
					pX->Typ = 'P';
					pX->Proc = RdProcArg('T');
				}
				if (Lexem == ',') { RdLex(); goto label1; }
				Accept(')');
			}
			else
				if (RdHeadLast(PD->Head)) {}
				else if (IsKeyWord("NOEDIT")) PD->EdTxtMode = 'V';
				else Error(161);
	}
}

void RdPutTxt()
{
	Instr* PD = GetPD(_puttxt, 11);
	RdPath(true, PD->TxtPath, PD->TxtCatIRec);
	Accept(',');
	PD->Txt = RdStrFrml();
	if (Lexem == ',') { RdLex(); AcceptKeyWord("APPEND"); PD->App = true; }
}

void RdTurnCat()
{
	Instr* PD = GetPD(_turncat, 12);
	TestIdentif();
	PD->NextGenFD = FindFileD();
	WORD Frst = GetCatIRec(LexWord, true);
	TestCatError(Frst, LexWord, true);
	RdLex();
	PD->FrstCatIRec = Frst;
	pstring RN = RdCatField(Frst, CatRdbName);
	pstring FN = RdCatField(Frst, CatFileName);
	WORD I = Frst + 1;
	while ((CatFD->NRecs >= I) && SEquUpcase(RN, RdCatField(I, CatRdbName))
		&& SEquUpcase(FN, RdCatField(I, CatFileName))) I++;
	if (I == Frst + 1) OldError(98);
	PD->NCatIRecs = I - Frst;
	Accept(',');
	PD->TCFrml = RdRealFrml();
}

void RdWriteln(BYTE OpKind)
{
	WrLnD* d;
	RdLex();
	FrmlPtr z = nullptr; FillChar(&d, sizeof(d), 0); WrLnD* w = d;
label1:
	/* !!! with w^ do!!! */
	w->Frml = RdFrml(w->Typ);
	if (w->Typ == 'R') {
		w->Typ = 'F';
		if (Lexem == ':') {
			RdLex();
			if (Lexem == _quotedstr) {
				w->Typ = 'D';
				w->Mask = StoreStr(LexWord); RdLex();
			}
			else {
				w->N = RdInteger();
				if (Lexem == ':') {
					RdLex();
					if (Lexem == '-') { RdLex(); w->M = -RdInteger(); }
					else w->M = RdInteger();
				}
			}
		}
	}
	if (Lexem == ',') {
		RdLex();
		if ((OpKind == 2) && IsOpt("HELP")) z = RdStrFrml();
		else {
			w = (WrLnD*)GetZStore(sizeof(d));
			ChainLast(d, w);  // TODO: tady bylo (&d, w)
			goto label1;
		}
	}
	WORD N = 1 + sizeof(d);
	if (z != nullptr) { OpKind = 3; N += 8; }
	Instr* pd = GetPInstr(_writeln, N);
	/* !!! with pd^ do!!! */
	pd->LF = OpKind;
	pd->WD = *d;
	if (OpKind == 3) { pd->mHlpRdb = CRdb; pd->mHlpFrml = z; }
	RdPInstr = pd;
}

void RdReleaseDrive()
{
	Instr* PD = GetPD(_releasedrive, 4);
	PD->Drive = RdStrFrml();
}

void RdIndexfile()
{
	Instr* PD = GetPD(_indexfile, 5);
	PD->IndexFD = RdFileName();
	if (PD->IndexFD->Typ != 'X') OldError(108);
	if (Lexem == ',') {
		RdLex();
		AcceptKeyWord("COMPRESS");
		PD->Compress = true;
	}
}

void RdGetIndex()
{
	LocVar* lv2; bool b; LinkD* ld;
	Instr* PD = GetPD(_getindex, 31); LocVar* lv = RdIdxVar();
	PD->giLV = lv; Accept(',');
	PD->giMode = ' ';
	if (Lexem == '+' || Lexem == '-') {
		PD->giMode = Lexem;
		RdLex();
		Accept(',');
		PD->giCond = RdRealFrml(); /*RecNr*/
		return;
	}
	CFile = RdFileName();
	if (lv->FD != CFile) OldError(164);
	CViewKey = RdViewKey();
	PD->giKD = CViewKey;
	while (Lexem == ',') {
		RdLex();
		if (IsOpt("SORT")) {
			if (WKeyDPtr(lv->RecPtr)->KFlds != nullptr) OldError(175);
			Accept('(');
			RdKFList(PD->giKFlds, CFile); Accept(')');
		}
		else if (IsOpt("COND")) {
			Accept('(');
			PD->giCond = RdKeyInBool(PD->giKIRoot, false, true, PD->giSQLFilter);
			Accept(')');
		}
		else if (IsOpt("OWNER")) {
			PD->giOwnerTyp = RdOwner(PD->giLD, PD->giLV2);
			KeyD* k = GetFromKey(PD->giLD);
			if (CViewKey == nullptr) PD->giKD = k;
			else if (CViewKey != k) OldError(178);
		}
		else Error(167);
		if ((PD->giOwnerTyp != 0) && (PD->giSQLFilter || (PD->giKIRoot != nullptr)))
			Error(179);
	}
}

void RdGotoXY()
{
	Instr* PD = GetPD(_gotoxy, 8);
	PD->GoX = RdRealFrml();
	Accept(',');
	PD->GoY = RdRealFrml();
}

void RdClrWw()
{
	Instr* PD = GetPD(_clrww, 24);
	/* !!! with PD^ do!!! */
	RdW(PD->W);
	if (Lexem == ',') {
		RdLex();
		if (Lexem != ',') PD->Attr = RdAttr();
		if (Lexem == ',') { RdLex(); PD->FillC = RdStrFrml(); }
	}
}

void RdMount()
{
	Instr* PD; FileD* FD; WORD I;
	PD = GetPD(_mount, 3);
	TestIdentif();
	FD = FindFileD();
	if (FD == nullptr) I = GetCatIRec(LexWord, true); else I = FD->CatIRec;
	TestCatError(I, LexWord, false); RdLex(); PD->MountCatIRec = I;
	if (Lexem == ',') {
		RdLex();
		AcceptKeyWord("NOCANCEL");
		PD->MountNoCancel = true;
	}
}

void RdDisplay()
{
	Instr* PD; pstring* s;
	PD = GetPD(_display, sizeof(RdbPos));
	if ((Lexem == _identifier) && FindChpt('H', LexWord, false, &PD->Pos)) RdLex();
	else {
		/* !!! with PD->Pos do!!! */
		PD->Pos.R = RdbDPtr(RdStrFrml); PD->Pos.IRec = 0;
	}
}

void RdGraphP()
{
	Instr* PD;
	FrmlPtr FrmlArr[15];
	WORD i;
	GraphVD* VD; GraphWD* WD; GraphRGBD* RGBD; WinG* Ww;

	pstring Nm1[11] = { "TYPE","HEAD","HEADX","HEADY","HEADZ","FILL","DIRX","GRID","PRINT","PALETTE","ASSIGN" };
	pstring Nm2[6] = { "WIDTH","RECNO","NRECS","MAX","MIN","GRPOLY" };

	PD = GetPD(_graph, 4);
	PD->GD = (GraphD*)GetZStore(sizeof(GraphD));
	/* !!! with PD->GD^ do!!! */
	auto PDGD = PD->GD;
	if (IsOpt("GF")) PDGD->GF = RdStrFrml();
	else {
		PDGD->FD = RdFileName();
		CFile = PDGD->FD;
		CViewKey = RdViewKey();
		PDGD->ViewKey = CViewKey;
		Accept(',');
		Accept('(');
		PDGD->X = RdFldName(PDGD->FD);
		i = 0;
		do { Accept(','); PDGD->ZA[i] = RdFldName(PDGD->FD); i++; } while (!((i > 9) || (Lexem != ',')));
		Accept(')');
	}
	while (Lexem == ',') {
		RdLex();
		for (i = 0; i < 11; i++) if (IsOpt(Nm1[i])) {
			FrmlArr[0] = (FrmlPtr)(&PDGD->T); FrmlArr[i] = RdStrFrml(); goto label1;
		}
		for (i = 0; i < 6; i++) if (IsOpt(Nm2[i])) {
			FrmlArr[0] = (FrmlPtr)(&PDGD->S); FrmlArr[i] = RdRealFrml(); goto label1;
		}
		if (IsDigitOpt("HEADZ", i)) PDGD->HZA[i] = RdStrFrml();
		else if (IsKeyWord("INTERACT")) PDGD->Interact = true;
		else if (IsOpt("COND")) {
			if (Lexem == '(')
			{
				RdLex();
				PDGD->Cond = RdKeyInBool(PDGD->KeyIn, false, true, PDGD->SQLFilter);
				Accept(')');
			}
			else PDGD->Cond = RdKeyInBool(PDGD->KeyIn, false, true, PDGD->SQLFilter);
		}
		else if (IsOpt("TXT")) {
			VD = (GraphVD*)GetZStore(sizeof(*VD));
			ChainLast(PDGD->V, VD);
			{
				/* !!! with VD^ do!!! */
				Accept('('); VD->XZ = RdRealFrml(); Accept(','); VD->YZ = RdRealFrml(); Accept(',');
				VD->Velikost = RdRealFrml(); Accept(','); VD->BarPis = RdStrFrml(); Accept(',');
				VD->Text = RdStrFrml(); Accept(')');
			}
		}
		else if (IsOpt("TXTWIN")) {
			WD = (GraphWD*)GetZStore(sizeof(*WD));
			ChainLast(PDGD->W, WD);
			{
				/* !!! with WD^ do!!! */
				Accept('('); WD->XZ = RdRealFrml(); Accept(','); WD->YZ = RdRealFrml(); Accept(',');
				WD->XK = RdRealFrml(); Accept(','); WD->YK = RdRealFrml(); Accept(',');
				WD->BarPoz = RdStrFrml(); Accept(','); WD->BarPis = RdStrFrml(); Accept(',');
				WD->Text = RdStrFrml(); Accept(')');
			}
		}
		else if (IsOpt("RGB")) {
			RGBD = (GraphRGBD*)GetZStore(sizeof(*RGBD));
			ChainLast(PDGD->RGB, RGBD);
			{
				/* !!! with RGBD^ do!!! */
				Accept('(');
				RGBD->Barva = RdStrFrml();
				Accept(',');
				RGBD->R = RdRealFrml();
				Accept(',');
				RGBD->G = RdRealFrml();
				Accept(',');
				RGBD->B = RdRealFrml();
				Accept(')');
			}
		}
		else if (IsOpt("WW")) {
			Ww = (WinG*)GetZStore(sizeof(*Ww));
			Accept('(');
			if (Lexem == '(') { RdLex; Ww->WFlags = WNoPop; }
			RdW(Ww->W); RdFrame(Ww->Top, Ww->WFlags);
			if (Lexem == ',') {
				RdLex(); Ww->ColBack = RdStrFrml(); Accept(',');
				Ww->ColFor = RdStrFrml();
				Accept(','); Ww->ColFrame = RdStrFrml();
			}
			Accept(')');
			if ((Ww->WFlags && WNoPop) != 0) Accept(')');
		}
		else Error(44);
	label1: {}
	}
}

void RdMixRecAcc(PInstrCode Op)
{
	Instr* PD;
	FrmlPtr Z;
	char FTyp;
	FileD* cf = CFile;
	if ((Op == _appendrec) || (Op == _recallrec)) {
		PD = GetPD(Op, 9); CFile = RdFileName(); PD->RecFD = CFile;
#ifdef FandSQL
		if (CFile->typSQLFile) OldError(155);
#endif

		if (Op = _recallrec) { Accept(','); PD->RecNr = RdRealFrml(); }
	}
	else {
		PD = GetPD(Op, 15);
		if (Op = _deleterec) { CFile = RdFileName(); PD->RecFD = CFile; }
		else { /*_readrec,_writerec*/
			if (!IsRecVar(PD->LV)) Error(141); CFile = PD->LV->FD;
		}
		KeyD* K = RdViewKey(); Accept(',');
#ifdef FandSQL
		if (CFile->typSQLFile
			&& (Lexem == _equ || Lexem == _le || Lexem == _gt || Lexem == _lt || Lexem == _ge))
		{
			PD->CompOp = Lexem; RdLex();
		}
#endif
		Z = RdFrml(FTyp);
		PD->RecNr = Z;
		switch (FTyp) {
		case 'B': OldError(12); break;
		case 'S': {
			PD->ByKey = true;
			if (PD->CompOp == 0) PD->CompOp = _equ;
			if (K == nullptr) K = CFile->Keys;
			PD->Key = K;
			if ((K == nullptr) && (!CFile->IsParFile || (Z->Op != _const)
				|| (Z->S[0] != 0))) OldError(24);
			break;
		}
#ifdef FandSQL
		default: {
			if (PD->CompOp != 0) OldError(19);
			if (CFile->typSQLFile && ((Op == _deleterec) || (Z->Op != _const)
				|| (Z->R != 0))) Error(155); break; }
#endif
		}
	}
	if ((Lexem == ',') && (Op == _writerec || Op == _deleterec || Op == _recallrec)) {
		RdLex(); Accept('+'); PD->AdUpd = true;
	}
	CFile = cf;
}

void RdLinkRec()
{
	LocVar* LV = nullptr; LinkD* LD = nullptr;
	Instr* PD = GetPD(_linkrec, 12);
	if (!IsRecVar(PD->RecLV1)) Error(141);
	Accept(','); CFile = PD->RecLV1->FD;
	if (IsRecVar(LV)) {
		LD = FindLD(LV->FD->Name);
		if (LD == nullptr) OldError(154);
	}
	else {
		TestIdentif(); LD = FindLD(LexWord);
		if (LD == nullptr) Error(9); RdLex();
		Accept('('); LV = RdRecVar();
		if (LD->ToFD != LV->FD) OldError(141);
		Accept(')');
	}
	PD->RecLV2 = LV;
	PD->LinkLD = LD;
}

void RdSetEditTxt()
{
	Instr* PD = GetPD(_setedittxt, 7 * 4);
	/* !!! with PD^ do!!! */
label1:
	if (IsOpt("OVERWR")) PD->Insert = RdBool();
	else if (IsOpt("INDENT")) PD->Indent = RdBool();
	else if (IsOpt("WRAP")) PD->Wrap = RdBool();
	else if (IsOpt("ALIGN")) PD->Just = RdBool();
	else if (IsOpt("COLBLK")) PD->ColBlk = RdBool();
	else if (IsOpt("LEFT")) PD->Left = RdRealFrml();
	else if (IsOpt("RIGHT")) PD->Right = RdRealFrml();
	else Error(160);
	if (Lexem == ',') { RdLex(); goto label1; }
}

FrmlPtr AdjustComma(FrmlPtr Z1, FieldDPtr F, char Op)
{
	FrmlPtr Z, Z2;
	auto result = Z1;
	if (F->Typ != 'F') return result;
	if (F->Flg && f_Comma == 0) return result;
	Z2 = GetOp(_const, sizeof(double));
	Z2->R = Power10[F->M];
	Z = GetOp(Op, 0);
	Z->P1 = Z1;
	Z->P2 = Z2;
	result = Z;
	return result;
}

AssignD* MakeImplAssign(FileD* FD1, FileD* FD2)
{
	AssignD* ARoot = nullptr;
	AssignD* A = nullptr;
	char FTyp;
	pstring S = LexWord;
	ARoot = nullptr;
	FieldDPtr F1 = FD1->FldD;
	while (F1 != nullptr) {
		if (F1->Flg && f_Stored != 0) {
			LexWord = F1->Name; 
			FieldDPtr F2 = FindFldName(FD2);
			if (F2 != nullptr) {
				A = (AssignD*)GetZStore(sizeof(*A)); 
				ChainLast(ARoot, A);
				if ((F2->FrmlTyp != F1->FrmlTyp) || (F1->FrmlTyp == 'R')
					&& (F1->Typ != F2->Typ)) {
					A->Kind = _zero; A->FldD = F1;
				}
				else {
					A->Kind = _output; A->OFldD = F1;
					FrmlPtr Z = MakeFldFrml(F2, FTyp); 
					Z = AdjustComma(Z, F2, _divide);
					A->Frml = FrmlContxt(AdjustComma(Z, F1, _times), FD2, nullptr);
				}
			}
		}
		F1 = (FieldDescr*)F1->Chain;
	}
	LexWord = S;
	return ARoot;
}

void RdAssign()
{
	FileDPtr FD = nullptr; FieldDPtr F = nullptr;
	LocVar* LV = nullptr; LocVar* LV2 = nullptr; char PV;
	Instr* PD = nullptr; pstring FName; char FTyp = 0;
	if (ForwChar == '.')
		if (FindLocVar(LVBD.Root, LV) && (LV->FTyp == 'r' || LV->FTyp == 'i')) {
			FTyp = LV->FTyp; RdLex(); RdLex();
			if (FTyp == 'i') {
				AcceptKeyWord("NRECS"); Accept(_assign);
				if ((Lexem != _number) || (LexWord != '0')) Error(183); RdLex();
				PD = GetPInstr(_asgnxnrecs, 4); PD->xnrIdx = WKeyDPtr(LV->RecPtr);
			}
			else {
				PD = GetPInstr(_asgnrecfld, 13); PD->AssLV = LV;
				F = RdFldName(LV->FD); PD->RecFldD = F;
				if (F->Flg & f_Stored == 0) OldError(14);
				FTyp = F->FrmlTyp;
			label0:
				RdAssignFrml(FTyp, PD->Add, PD->Frml);
			}
		}
		else {
			FName = LexWord; FD = FindFileD();
			if (IsActiveRdb(FD)) Error(121); RdLex(); RdLex();
			if (IsKeyWord("ARCHIVES")) { F = CatArchiv; goto label1; }
			if (IsKeyWord("PATH")) { F = CatPathName; goto label1; }
			if (IsKeyWord("VOLUME")) {
				F = CatVolume;
			label1:
				PD = GetPInstr(_asgncatfield, 16); PD->FD3 = FD;
				PD->CatIRec = GetCatIRec(FName, true); PD->CatFld = F;
				TestCatError(PD->CatIRec, FName, true);
				Accept(_assign); PD->Frml3 = RdStrFrml();
			}
			else if (FD == nullptr) OldError(9);
			else if (IsKeyWord("NRECS")) {
				if (FD->Typ == '0') OldError(127); PD = GetPInstr(_asgnnrecs, 9);
				PD->FD = FD; FTyp = 'R'; goto label0;
			}
			else {
				if (!FD->IsParFile) OldError(64);
				PD = GetPInstr(_asgnpar, 13);
				PD->FD = FD;
				F = RdFldName(FD);
				PD->FldD = F;
				if (F->Flg & f_Stored == 0) OldError(14);
				FTyp = F->FrmlTyp;
				goto label0;
			}
		}
	else if (ForwChar == '[') {
		PD = GetPInstr(_asgnfield, 18); FD = RdFileName(); PD->FD = FD; RdLex();
#ifdef FandSQL
		if (FD->typSQLFile) OldError(155);
#endif

		PD->RecFrml = RdRealFrml(); Accept(']'); Accept('.'); F = RdFldName(FD);
		PD->FldD = F; if (F->Flg & f_Stored == 0) OldError(14);
		PD->Indexarg = (FD->Typ = 'X') && IsKeyArg(F, FD);
		RdAssignFrml(F->FrmlTyp, PD->Add, PD->Frml);
	}
	else if (FindLocVar(LVBD.Root, LV)) {
		RdLex(); FTyp = LV->FTyp;
		switch (FTyp) {
		case 'f':
		case 'i': OldError(140); break;
		case 'r': { Accept(_assign); if (!IsRecVar(LV2)) Error(141);
			PD = GetPInstr(_asgnrecvar, 12); PD->RecLV1 = LV; PD->RecLV2 = LV2;
			PD->Ass = MakeImplAssign(LV->FD, LV2->FD); }
		default: PD = GetPInstr(_asgnloc, 9); PD->AssLV = LV; goto label0;
		}
	}
	else if (IsKeyWord("USERNAME")) { PD = GetPInstr(_asgnusername, 4); goto label2; }
	else if (IsKeyWord("CLIPBD")) { PD = GetPInstr(_asgnclipbd, 4); goto label2; }
	else if (IsKeyWord("ACCRIGHT")) {
		PD = GetPInstr(_asgnaccright, 4);
	label2:
		Accept(_assign); PD->Frml = RdStrFrml();
	}
	else if (IsKeyWord("EDOK")) {
		PD = GetPInstr(_asgnedok, 4); Accept(_assign); PD->Frml = RdBool();
	}
	else if (IsKeyWord("RANDSEED")) { PD = GetPInstr(_asgnrand, 4); goto label3; }
	else if (IsKeyWord("TODAY")) { PD = GetPInstr(_asgnusertoday, 4); goto label3; }
	else if (IsKeyWord("USERCODE")) {
		PD = GetPInstr(_asgnusercode, 4);
	label3:
		Accept(_assign); PD->Frml = RdRealFrml();
	}
	else {
		RdLex();
		if (Lexem == _assign) OldError(8); else OldError(34);
	}
	RdPInstr = PD;
}

Instr* RdWith()
{
	Instr* P = nullptr; Instr* p2; PInstrCode Op;
	if (IsKeyWord("WINDOW")) {
		P = GetPInstr(_window, 29); Accept('(');
		if (Lexem == '(') { RdLex(); P->WithWFlags = WNoPop; }
		RdW(P->W); RdFrame(P->Top, P->WithWFlags);
		if (Lexem == ',') { RdLex(); P->Attr = RdAttr(); }
		Accept(')'); if ((P->WithWFlags & WNoPop) != 0) Accept(')');
		AcceptKeyWord("DO"); P->WwInstr = RdPInstr;
	}
	else if (IsKeyWord("SHARED")) { Op = _withshared; goto label1; }
	else if (IsKeyWord("LOCKED")) {
		Op = _withlocked;
	label1:
		P = GetPInstr(Op, 9 + sizeof(LockD));
		LockD* ld = &P->WLD;
	label2:
		ld->FD = RdFileName();
		if (Op == _withlocked) {
			Accept('['); ld->Frml = RdRealFrml(); Accept(']');
		}
		else {
			Accept('(');
			for (LockMode i = NoExclMode; i <= ExclMode; i = LockMode(i + 1))
				if (IsKeyWord(LockModeTxt[i])) { ld->Md = i; goto label3; }
			Error(100);
		label3:
			Accept(')');
		}
		if (Lexem == ',') {
			RdLex();
			ld->Chain = (LockD*)GetZStore(sizeof(LockD));
			ld = ld->Chain;
			goto label2;
		}
		AcceptKeyWord("DO"); P->WDoInstr = RdPInstr;
		if (IsKeyWord("ELSE")) {
			P->WasElse = true; P->WElseInstr = RdPInstr;
		}
	}
	else if (IsKeyWord("GRAPHICS")) {
		P = GetPInstr(_withgraphics, 4); AcceptKeyWord("DO");
		P->WDoInstr = RdPInstr;
	}
	else Error(131);
	return P;
}

Instr* RdUserFuncAssign()
{
	Instr* pd = nullptr; LocVar* lv = nullptr;
	if (!FindLocVar(LVBD.Root, lv)) Error(34);
	RdLex(); pd = GetPInstr(_asgnloc, 9); pd->AssLV = lv;
	RdAssignFrml(lv->FTyp, pd->Add, pd->Frml);
	auto result = pd;

	if (IsKeyWord("IF")) RdPInstr = RdIfThenElse();
	else if (IsKeyWord("WHILE")) RdPInstr = RdWhileDo();
	else if (IsKeyWord("REPEAT")) RdPInstr = RdRepeatUntil();
	else if (IsKeyWord("CASE")) RdPInstr = RdCase();
	else if (IsKeyWord("FOR")) RdPInstr = RdFor();
	else if (IsKeyWord("BEGIN")) RdPInstr = RdBeginEnd();
	else if (IsKeyWord("BREAK")) RdPInstr = GetPInstr(_break, 0);
	else if (IsKeyWord("EXIT")) RdPInstr = GetPInstr(_exitP, 0);
	else if (IsKeyWord("CANCEL")) RdPInstr = GetPInstr(_cancel, 0);
	else if (Lexem == ';') RdPInstr = nullptr;
	else if (IsRdUserFunc) RdPInstr = RdUserFuncAssign();
	else if (IsKeyWord("MENULOOP")) RdPInstr = RdMenuBox(true);
	else if (IsKeyWord("MENU")) RdPInstr = RdMenuBox(false);
	else if (IsKeyWord("MENUBAR")) RdPInstr = RdMenuBar();
	else if (IsKeyWord("WITH")) RdPInstr = RdWith();
	else if (IsKeyWord("SAVE")) RdPInstr = GetPInstr(_save, 0);
	else if (IsKeyWord("CLREOL")) RdPInstr = GetPInstr(_clreol, 0);
	else if (IsKeyWord("FORALL")) RdPInstr = RdForAll();
	else if (IsKeyWord("CLEARKEYBUF")) RdPInstr = GetPInstr(_clearkeybuf, 0);
	else if (IsKeyWord("WAIT")) RdPInstr = GetPInstr(_wait, 0);
	else if (IsKeyWord("BEEP")) RdPInstr = GetPInstr(_beepP, 0);
	else if (IsKeyWord("NOSOUND")) RdPInstr = GetPInstr(_nosound, 0);
#ifndef FandRunV
	else if (IsKeyWord("MEMDIAG")) RdPInstr = GetPInstr(_memdiag, 0);
#endif 
	else if (IsKeyWord("RESETCATALOG")) RdPInstr = GetPInstr(_resetcat, 0);
	else if (IsKeyWord("RANDOMIZE")) RdPInstr = GetPInstr(_randomize, 0);
	else if (Lexem == _identifier) {
		SkipBlank(false);
		if (ForwChar == '(') RdProcCall();
		else if (IsKeyWord("CLRSCR")) RdPInstr = GetPInstr(_clrscr, 0);
		else if (IsKeyWord("GRAPH")) RdPInstr = GetPInstr(_graph, 4);
		else if (IsKeyWord("CLOSE")) RdPInstr = GetPInstr(_closefds, 4);
		else RdAssign();
	}
	else Error(34);
	return result;
}

void ReadProcHead()
{
	ResetCompilePars(); RdFldNameFrml = RdFldNameFrmlP; RdFunction = RdFunctionP;
	FileVarsAllowed = false; IdxLocVarAllowed = true; IsRdUserFunc = false;
	RdLex(); ResetLVBD();
	if (Lexem == '(') {
		RdLex(); RdLocDcl(&LVBD, true, true, 'P'); Accept(')');
	}
	if (IsKeyWord("VAR")) RdLocDcl(&LVBD, false, true, 'P');
}

Instr* ReadProcBody()
{
	AcceptKeyWord("BEGIN");
	Instr* result = RdBeginEnd();
	Accept(';');
	if (Lexem != 0x1A) Error(40);
	return result;
}

void ReadDeclChpt()
{
	FuncDPtr fc;
	char typ = '\0';
	WORD n = 0;
	LocVar* lv;
	RdLex();
label1:
	if (IsKeyWord("FUNCTION")) {
		TestIdentif(); fc = FuncDRoot; while (fc != CRdb->OldFCRoot) {
			if (EquUpcase(fc->Name, LexWord)) Error(26); fc = fc->Chain;
		}
		//fc = (FuncD*)GetStore(sizeof(FuncD) - 1 + LexWord.length());
		fc = new FuncD();
		fc->Chain = FuncDRoot; FuncDRoot = fc;
		Move(&LexWord, &fc->Name, LexWord.length() + 1);
		RdFldNameFrml = RdFldNameFrmlP; RdFunction = RdFunctionP;
		ChainSumEl = nullptr;
		FileVarsAllowed = false; IsRdUserFunc = true;
		RdLex(); ResetLVBD();
		Accept('(');
		if (Lexem != ')') RdLocDcl(&LVBD, true, false, 'D'); Accept(')');
		Accept(':');
		if (IsKeyWord("REAL")) { typ = 'R'; n = sizeof(double); }
		else if (IsKeyWord("STRING")) { typ = 'S'; n = sizeof(longint); }
		else if (IsKeyWord("BOOLEAN")) { typ = 'B'; n = sizeof(bool); }
		else Error(39);
		lv = (LocVar*)GetZStore(sizeof(*lv) - 1 + (fc->Name).length());
		ChainLast(LVBD.Root, lv);
		Move(&fc->Name, &lv->Name, (fc->Name).length() + 1);
		/* !!! with lv^ do!!! */
		{ lv->FTyp = typ; lv->Op = _getlocvar; lv->BPOfs = LVBD.Size; }
		fc->FTyp = typ; LVBD.Size += n;
		Accept(';');
		if (IsKeyWord("VAR")) RdLocDcl(&LVBD, false, false, 'D'); fc->LVB = LVBD;
		AcceptKeyWord("BEGIN"); fc->Instr = RdBeginEnd; Accept(';');
	}
	else if (Lexem == 0x1A) return;
	else Error(40);
	goto label1;
}

FrmlPtr GetEvalFrml(FrmlPtr X)
{
	void* p = nullptr;
	void* cr = nullptr;
	char fTyp;
	WORD cpos = 0;
	ExitRecord er = ExitRecord();
	ProcStkD* oldbp;

	LocVarBlkD oldLVBD = LVBD; oldbp = MyBP; SetMyBP(ProcMyBP);
	FrmlPtr z = nullptr; FileDPtr cf = CFile; cr = CRecPtr;
	LongStr* s = RunLongStr(X->P1);
	if (s->LL == 0) { LastExitCode = 0; goto label2; }
	LastExitCode = 1; p = SaveCompState(); ResetCompilePars();
	RdFldNameFrml = RdFldNameFrmlP; RdFunction = RdFunctionP;
	if (X->EvalFD == nullptr) FileVarsAllowed = false; else {
		CFile = X->EvalFD; FileVarsAllowed = true;
	}
	//NewExit(Ovr, er);
	goto label1;
	SetInpLongStr(s, false); RdLex(); z = RdFrml(fTyp);
	if ((fTyp != X->EvalTyp) || (Lexem != 0x1A)) z = nullptr; else LastExitCode = 0;
label1:
	cpos = CurrPos; RestoreExit(er); RestoreCompState(p);
	if (LastExitCode != 0) {
		LastTxtPos = cpos;
		if (X->EvalTyp == 'B') { z = GetOp(_const, 1); z->B = false; }
	}
	if (z != nullptr) { FrmlPtr z1 = z; z = GetOp(_setmybp, 0); z->P1 = z1; }
label2:
	auto result = z; CFile = cf; CRecPtr = cr; SetMyBP(oldbp);
	LVBD = oldLVBD; /*for cond before cycle called when PushProcStk is !ready*/
	return result;
}

void RdBackup(char MTyp, bool IsBackup)
{
	Instr* PD;
	if (MTyp == 'M') PD = GetPD(_backupm, 15); else PD = GetPD(_backup, 5);
	PD->IsBackup = IsBackup;
	TestIdentif();
	FileDPtr cf = CFile;
	void* cr = CRecPtr;
	CFile = CatFD;
	CRecPtr = GetRecSpace();
	for (WORD i = 1; i < CatFD->NRecs; i++) {
		ReadRec(i);
		if (SEquUpcase(TrailChar(' ', _ShortS(CatRdbName)), "ARCHIVES") &&
			SEquUpcase(TrailChar(' ', _ShortS(CatFileName)), LexWord)) {
			RdLex(); PD->BrCatIRec = i; ReleaseStore(CRecPtr);
			CFile = cf; CRecPtr = cr;
			goto label1;
		}
	}
	Error(88);
label1:
	if (MTyp == 'M') {
		Accept(','); PD->bmDir = RdStrFrml();
		if (IsBackup) { Accept(','); PD->bmMasks = RdStrFrml(); }
	}
	while (Lexem == ',') {
		RdLex();
		if (MTyp == 'M') {
			if (!IsBackup && IsKeyWord("OVERWRITE")) { PD->bmOverwr = true; goto label2; }
			if (IsKeyWord("SUBDIR")) { PD->bmSubDir = true; goto label2; };
		}
		if (IsKeyWord("NOCOMPRESS")) PD->NoCompress = true;
		else { AcceptKeyWord("NOCANCEL"); PD->BrNoCancel = true; }
	label2: {}
	}
}

#ifdef FandSQL
void RdSqlRdWrTxt(bool Rd)
{
	Instr* pd = GetPD(_sqlrdwrtxt, 23);
	/* !!! with pd^ do!!! */
	pd->IsRead = Rd; RdPath(true, pd->TxtPath, pd->TxtCatIRec);
	Accept(','); CFile = RdFileName(); pd->sqlFD = CFile;
	KeyDPtr k = RdViewKey(); if (k == nullptr) k = CFile->Keys; pd->sqlKey = k; Accept(',');
	pd->sqlFldD = RdFldName(CFile); Accept(','); pd->sqlXStr = RdStrFrml();
	if (!pd->sqlFD->typSQLFile || (pd->sqlFldD->Typ != 'T')) OldError(170);
}
#endif
#ifdef FandProlog
void RdCallLProc()
{
	Instr* pd = GetPD(_lproc, sizeof(RdbPos) + 4); RdChptName('L', pd->lpPos, true);
	if (Lexem == ',') {
		RdLex(); TestIdentif(); pd->lpName = StoreStr(LexWord); RdLex();
	}
}
#endif

