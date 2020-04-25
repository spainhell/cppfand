#include "rdproc.h"



#include "common.h"
#include "legacy.h"
#include "lexanal.h"
#include "memory.h"
#include "rdfrml.h"
#include "rdrun.h"

void Ovr()
{
	__asm {
		pop ax;
		pop ax;
		pop ax; /*bp*/
		push ax;
		push ax;
		call StackOvr;
		pop bp;
		pop ds;
		pop ax;
		pop dx;
		pop sp;
		push cs;
		push ax;
	}
}

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
	LocVar* lv;
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
		if ((ld->FromFD == CFile) && EquUpCase(ld->RoleName)) {
			if ((ld->IndexRoot = 0)) Error(116);
			RdLex(); FileD* fd = ld->ToFD;
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

				Accept('['); LLV = LocVarPtr(RdRealFrml); Accept(']'); result = 'F';
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
	FileD* FD; FrmlPtr Z; LocVar* LV; char Op; LinkD* LD; FieldDPtr F;
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
		Z = GetOp(_accrecno, 8); FD = RdFileName; RdLex(); Z->RecFD = FD;
#ifdef FandSQL
		if (FD->typSQLFile) OldError(155);
#endif

		Z->P1 = RdRealFrml; Accept(']'); Accept('.');
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
		fd = FindFileD;
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
	LocVar* LV; LinkDPtr LD;
	void* p; WORD* pofs = (WORD*)&p;

	if (IsKeyWord("EVALB")) { FTyp = 'B'; goto label4; }
	else if (IsKeyWord("EVALS")) { FTyp = 'S'; goto label4; }
	else if (IsKeyWord("EVALR")) {
		FTyp = 'R';
	label4:
		RdLex(); Z = GetOp(_eval, 5); Z->EvalTyp = FTyp; Z->P1 = RdStrFrml;
	}
	else if (FileVarsAllowed) Error(75);
	else if (IsKeyWord("PROMPT")) {
		RdLex(); Z = GetOp(_prompt, 4); Z->P1 = RdStrFrml; FieldDPtr F = RdFldDescr("", true);
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
		FD = RdFileName; KeyDPtr K = RdViewKeyImpl(FD);
		if (Op == _recno) {
			KeyFldDPtr KF = K->KFlds; N = 0;
			if (KF == nullptr) OldError(176);
			while (KF != nullptr) {
				Accept(','); N++;
				if (N > 30) Error(123); Arg[N] = RdFrml(Typ);
				if (Typ != KF->FldD->FrmlTyp) OldError(12); KF = KF->Chain;
			}
		}
		else { Accept(','); N = 1; Arg[1] = RdRealFrml; }
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
			FD = RdFileName; Accept('[');
			Z->LinkRecFrml = RdRealFrml; Accept(']');
		}
		Accept(',');
#ifdef FandSQL
		if (FD->typSQLFile) OldError(155);
#endif
		cf = CFile; CFile = FD;
		if (not IsRoleName(true, FD, LD) || (LD = nullptr)) Error(9);
		CFile = cf; Z->LinkLD = LD; FTyp = 'R'; goto label2;
	}
	else if (IsKeyWord("ISDELETED")) {
		RdLex(); FTyp = 'B';
		if (IsRecVar(LV)) {
			Z = GetOp(_lvdeleted, 4); Z->LV = LV;
		}
		else {
			Z = GetOp(_isdeleted, 4); FD = RdFileName; Z->RecFD = FD;
			Accept(','); Z->P1 = RdRealFrml;
		label2:
#ifdef FandSQL
			if (FD->typSQLFile) Error(155);
#endif
		}
	}
	else if (IsKeyWord("GETPATH")) {
		RdLex(); Z = GetOp(_getpath, 0); Z->P1 = RdStrFrml; FTyp = 'S';
	}
	else if (IsKeyWord("GETTXT")) {
		RdLex(); Z = GetOp(_gettxt, 6); FTyp = 'S'; goto label3;
	}
	else if (IsKeyWord("FILESIZE")) {
		RdLex(); Z = GetOp(_filesize, 14); FTyp = 'R';
	label3:
		RdPath(true, Z->TxtPath, Z->TxtCatIRec);
		if ((Z->Op == _gettxt) && (Lexem == ',')) {
			RdLex(); Z->P1 = RdRealFrml;
			if (Lexem == ',') { RdLex(); Z->P2 = RdRealFrml; }
		}
	}
	else if (IsKeyWord("INTTSR")) {
		RdLex(); Z = GetOp(_inttsr, 5); Z->P1 = RdRealFrml; Accept(',');
		Z->P2 = RdRealFrml; Accept(','); Typ = 'r';
		if (IsRecVar(LV)) Z->P3 = LV->RecPtr; else Z->P3 = RdFrml(Typ);
		Z->N31 = Typ;
		FTyp = 'R';
	}
#ifdef FandSQL
	else if (IsKeyWord("SQL")) {
		RdLex; Z = GetOp(_sqlfun, 0); Z->P1 = RdStrFrml; FTyp = 'R';
	}
#endif
	else if (IsKeyWord("SELECTSTR")) {
		RdLex(); Z = GetOp(_selectstr, 13); FTyp = 'S'; RdSelectStr(Z);
	}
	else if (IsKeyWord("PROMPTYN")) {
		RdLex(); Z = GetOp(_promptyn, 0); Z->P1 = RdStrFrml; FTyp = 'B';
	}
	else if (IsKeyWord("MOUSEEVENT")) {
		RdLex(); Z = GetOp(_mouseevent, 2); Z->W01 = RdInteger(); FTyp = 'B';
	}
	else if (IsKeyWord("ISMOUSE")) {
		RdLex(); Z = GetOp(_ismouse, 4); Z->W01 = RdInteger(); Accept(',');
		Z->W02 = RdInteger(); FTyp = 'B';
	}
	else if (IsKeyWord("MOUSEIN")) {
		RdLex(); Z = GetOp(_mousein, 4); Z->P1 = RdRealFrml; Accept(',');
		Z->P2 = RdRealFrml; Accept(','); Z->P3 = RdRealFrml; Accept(',');
		Z->P4 = RdRealFrml; FTyp = 'B';
	}
	else if (IsKeyWord("PORTIN")) {
		RdLex(); Z = GetOp(_portin, 0); Z->P1 = RdBool; Accept(',');
		Z->P2 = RdRealFrml; FTyp = 'R';
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
		FileD* cf = CFile; CFile = FD; K = RdViewKey; CFile = cf;
	}
	return K;
}

void RdSelectStr(FrmlPtr Z)
{
	Instr* PD;
	Z->Delim = *m;
	Z->P1 = RdRealFrml; Accept(',');
	Z->P2 = RdRealFrml; Accept(',');
	Z->P3 = RdStrFrml;
	while (Lexem == ',') {
		RdLex();
		if (IsOpt("HEAD")) Z->P4 = RdStrFrml;
		else if (IsOpt("FOOT")) Z->P5 = RdStrFrml;
		else if (IsOpt("MODE")) Z->P6 = RdStrFrml;
		else if (IsOpt("DELIM")) Z->Delim = RdQuotedChar();
		else Error(157);
	}
}

Instr* GetPInstr(PInstrCode Kind, WORD Size)
{
	Instr* PD = (Instr*)GetZStore(Size + 5);
	PD->Kind = Kind;
	return PD;
}

void RdPInstrAndChain(Instr* PD)
{
	Instr* PD1 = RdPInstr(); /*may be a chain itself*/
	Instr* PD2 = PD;
	while (PD2->Chain != nullptr) PD2 = PD2->Chain;
	PD2->Chain = PD1;
}

void RdChoices(Instr* PD)
{
	ChoiceD* CD; WORD N, SumL;
	AcceptKeyWord("OF"); N = 0; SumL = 0;
label1:
	if (IsKeyWord("ESCAPE")) {
		Accept(':');
		PD->WasESCBranch = true;
		PD->ESCInstr = RdPInstr();
	}
	else {
		CD = GetZStore(sizeof(CD)); ChainLast(PD->Choices, CD); N++;
		if ((PD->Kind == _menubar) && (N > 30)) Error(102);
		CD->TxtFrml = RdStrFrml;
		if (Lexem == ',') {
			RdLex();
			if (Lexem != ',') { CD->HelpName = RdHelpName; PD->HelpRdb = CRdb; }
			if (Lexem == ',') {
				RdLex();
				if (Lexem != ',') {
					CD->Bool = RdBool;
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
	PD->mAttr[0] = RdAttr; Accept(',');
	PD->mAttr[1] = RdAttr; Accept(',');
	PD->mAttr[2] = RdAttr;
	if (Lexem == ',') { RdLex(); PD->mAttr[3] = RdAttr; }
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
			PD->X = RdRealFrml; Accept(','); PD->Y = RdRealFrml;
		}
		RdMenuAttr(PD);
		Accept(')');
	}
	if (Lexem == '!') { RdLex(); PD->Shdw = true; }
	if (IsKeyWord("PULLDOWN")) PD->PullDown = true;
	if (!TestKeyWord("OF")) PD->HdLine = RdStrFrml;
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
			PD->Y = RdRealFrml;
			if (Lexem == ',') {
				RdLex();
				PD->X = RdRealFrml;	Accept(',');
				PD->XSz = RdRealFrml;
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
	PD->Bool = RdBool;
	AcceptKeyWord("THEN");
	PD->Instr1 = RdPInstr();
	if (IsKeyWord("ELSE")) PD->ElseInstr1 = RdPInstr();
	return result;
}

Instr* RdWhileDo()
{
	Instr* PD = GetPInstr(_whiledo, 8);
	auto result = PD;
	PD->Bool = RdBool;
	AcceptKeyWord("DO");
	PD->Instr1 = RdPInstr();
	return result;
}

Instr* RdFor()
{
	LocVar* LV;
	if (!FindLocVar(LVBD.Root, LV) || (LV->FTyp != 'R')) Error(146);
	RdLex();
	Instr* PD = GetPInstr(_asgnloc, 9);
	auto result = PD;
	PD->AssLV = LV;
	Accept(_assign);
	PD->Frml = RdRealFrml; AcceptKeyWord("TO");
	PD->Chain = GetPInstr(_whiledo, 8);
	PD = PD->Chain;
	FrmlPtr Z = GetOp(_compfloat, 2);
	Z->P1 = (FrmlElem*)LV->Op;
	Z->N21 = _le;
	Z->N22 = 5;
	Z->P2 = RdRealFrml;
	PD->Bool = Z;
	AcceptKeyWord("DO");
	Instr* PD1 = RdPInstr();
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
	Instr* PD;
	bool first = true;
label1:
	Instr* PD1 = GetPInstr(_ifthenelseP, 12);
	Instr* result = nullptr;
	if (first) result = PD1;
	else PD->ElseInstr1 = PD1;
	PD = PD1; first = false;
	PD->Bool = RdBool; Accept(':');
	PD->Instr1 = RdPInstr();
	bool b = Lexem = ';';
	if (b) RdLex();
	if (not IsKeyWord("END"))
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
label1: PD->Bool = RdBool;
	return result;
}

Instr* RdForAll()
{
	LocVar* LVi; LocVar* LVr; LinkD* LD; FrmlPtr Z;
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
			CFile = RdFileName;
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
		AcceptKeyWord("SQL"); Accept('('); PD->CBool = RdStrFrml; Accept(')');
		PD->inSQL = true; goto label2;
	}
#endif
	if (IsKeyWord("OWNER")) {
		/* !!! with PD^ do!!! */
		PD->COwnerTyp = RdOwner(PD->CLD, PD->CLV); CViewKey = GetFromKey(PD->CLD);
	}
	else CViewKey = RdViewKey;
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
	AcceptKeyWord("DO"); PD->CInstr = RdPInstr();
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
	LocVar* LV;
	if (Caller != 'C') RdChptName('P', Pos, Caller == 'P' || Caller == 'E' || Caller == 'T');
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
				z->P1 = RdStrFrml;
				TArg[N].TxtFrml = z;
				Accept(']');
			}
			else TArg[N].FD = RdFileName; TArg[N].FTyp = 'f';
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
			if (EquUpcase(KeyNames[i].Nm)) {
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
	if (IsOpt("HEAD")) A->Head = RdStrFrml;
	else if (IsOpt("LAST")) A->Last = RdStrFrml;
	else if (IsOpt("CTRL")) A->CtrlLast = RdStrFrml;
	else if (IsOpt("ALT")) A->AltLast = RdStrFrml;
	else if (IsOpt("SHIFT")) A->ShiftLast = RdStrFrml;
	else result = false;
	return result;
}

bool RdViewOpt(EditOpt* EO)
{
	FileD* FD = nullptr;
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
		else Mode = RdStrFrml;
	}
	else if (RdHeadLast(EO->Head)) return result;
	else if (IsOpt("WATCH")) EO->WatchDelayZ = RdRealFrml;
	else if (IsOpt("WW")) {
		Accept('('); EO->WFlags = 0;
		if (Lexem == '(') { RdLex(); EO->WFlags = WNoPop; }
		RdW(EO->W); RdFrame(EO->Top, EO->WFlags);
		if (Lexem == ',') {
			RdLex();
			EO->ZAttr = RdAttr; Accept(',');
			EO->ZdNorm = RdAttr; Accept(',');
			EO->ZdHiLi = RdAttr;
			if (Lexem == ',') {
				RdLex();
				EO->ZdSubset = RdAttr;
				if (Lexem == ',') {
					RdLex();
					EO->ZdDel = RdAttr;
					if (Lexem == ',') {
						RdLex();
						EO->ZdTab = RdAttr;
						if (Lexem == ',') {
							RdLex();
							EO->ZdSelect = RdAttr;
						}
					}
				}
			}
		}
		Accept(')');
		if ((EO->WFlags && WNoPop) != 0) Accept(')');
	}
	else if (IsOpt("EXIT")) {
		Accept('(');
	label1:
		EdExitD* X = (EdExitD*)GetZStore(sizeof(*X));
		ChainLast(EO->ExD, X);
		RdKeyList(X);
		if (IsKeyWord("QUIT")) X->Typ = 'Q';
		else if (IsKeyWord("REPORT")) {
			if (X->AtWrRec || (LVRecPtr != nullptr)) OldError(144);
			Accept('(');
			X->Typ = 'R';
			RprtOpt* RO = GetRprtOpt;
			RdChptName('R', RO->RprtPos, true);
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
	else if (LVRecPtr != nullptr) result = false;
	else if (IsOpt("COND")) {
		if (Lexem == '(') {
			RdLex();
			EO->Cond = RdKeyInBool(EO->KIRoot, false, true, EO->SQLFilter);
			Accept(')');
		}
		else EO->Cond = RdKeyInBool(EO->KIRoot, false, true, EO->SQLFilter);
	}
	else if (IsOpt("JOURNAL")) {
		EO->Journal = RdFileName;
		WORD l = EO->Journal->RecLen - 13;
		if (CFile->Typ == 'X') l++;
		if (CFile->RecLen != l) OldError(111);
	}
	else if (IsOpt("SAVEAFTER")) EO->SaveAfterZ = RdRealFrml;
	else if (IsOpt("REFRESH")) EO->RefreshDelayZ = RdRealFrml;
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
	// TODO: proè to pøiøazuje nìco nìjaké funkci? RdPInstr = PD;
	return PD;
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
			if (EquUpcase(FL1->FldD->Name)) goto label2;
			FL1 = FL1->Chain;
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
	InstrPtr PD = GetPD(_sort, 8);
	FileDPtr FD = RdFileName;
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
	InstrPtr PD; EditOpt* EO; void* p; bool b; KeyDPtr K;
	LocVar* lv = nullptr;
	PD = GetPD(_edit, 8);
	EO = GetEditOpt;
	PD->EO = EO;
	if (IsRecVar(lv)) { EO->LVRecPtr = lv->RecPtr; CFile = lv->FD; }
	else {
		CFile = RdFileName;
		K = RdViewKey;
		if (K == nullptr) K = CFile->Keys;
		EO->ViewKey = K;
	}
	PD->EditFD = CFile;
	Accept(',');
	if (IsOpt('U')) {
		TestIdentif();
		if (CFile->ViewNames == nullptr) Error(114);
		p = SaveCompState;
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
	if (IsOpt("FIELD")) EO->StartFieldZ = RdStrFrml;
	else if (EO->LVRecPtr != nullptr) Error(125);
	else if (IsOpt("OWNER")) {
		if (EO->SQLFilter || (EO->KIRoot != nullptr)) OldError(179);
		EO->OwnerTyp = RdOwner(EO->DownLD, EO->DownLV);
	}
	else if (IsOpt("RECKEY")) StartRecKeyZ = RdStrFrml;
	else if (
#ifdef FandSQL
		!CFile->typSQLFile &&
#endif
		IsOpt("RECNO")) EO->StartRecNoZ = RdRealFrml;
	else if (IsOpt("IREC")) EO->StartIRecZ = RdRealFrml;
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





















