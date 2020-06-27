#include "rdproc.h"
#include "compile.h"
#include "legacy.h"
#include "oaccess.h"
#include "rdfildcl.h"
#include "rdrun.h"
#include "runfrml.h"
#include "models/Instr.h"

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

Instr* RdPInstr();

void TestCatError(WORD I, pstring Nm, bool Old)
{
	if (I == 0) {
		SetMsgPar(Nm);
		if (Old) OldError(96);
		else Error(96);
	}
}

bool IsRecVar(LocVar** LV)
{
	if (!FindLocVar(&LVBD, LV) || ((*LV)->FTyp != 'r')) return false;
	RdLex();
	return true;
}

LocVar* RdRecVar()
{
	LocVar* LV = nullptr;
	if (!IsRecVar(&LV)) Error(141);
	return LV;
}

LocVar* RdIdxVar()
{
	LocVar* lv = nullptr;
	if (!FindLocVar(&LVBD, &lv) || (lv->FTyp != 'i')) Error(165);
	auto result = lv;
	RdLex();
	return result;
}

FrmlElem* RdRecVarFldFrml(LocVar* LV, char& FTyp)
{
	FrmlElem* Z = nullptr;
	Accept('.');
	switch (LV->FTyp) {
	case 'r': {
		auto Z = new FrmlElem7(_recvarfld, 12); // GetOp(_recvarfld, 12);
		FileD* cf = CFile;
		CFile = LV->FD;
		Z->File2 = CFile;
		Z->LD = (LinkD*)LV->RecPtr;
		bool fa = FileVarsAllowed;
		FileVarsAllowed = true;
		Z->P011 = RdFldNameFrmlF(FTyp);
		FileVarsAllowed = fa;
		CFile = cf;
		return Z;
		break;
	}
	case 'i': {
		auto Z = new FrmlElem22(_indexnrecs, 4); // GetOp(_indexnrecs, 4);
		Z->WKey = WKeyDPtr(LV->RecPtr);
		pstring nrecs = "nrecs";
		AcceptKeyWord(nrecs); FTyp = 'R';
		return Z;
		break;
	}
	default: OldError(177); break;
	}
	return nullptr;
}

char RdOwner(LinkD* LLD, LocVar* LLV)
{
	FileD* fd = nullptr;
	auto result = '\0';
	LinkD* ld = nullptr;
	LocVar* lv = nullptr;
	if (FindLocVar(&LVBD, &lv)) {
		if (!(lv->FTyp == 'i' || lv->FTyp == 'r' || lv->FTyp == 'f')) Error(177);
		ld = nullptr; LinkD* ld1 = LinkDRoot; while (ld1 != nullptr) {
			if ((ld1->FromFD == CFile) && (ld1->IndexRoot != 0) && (ld1->ToFD == lv->FD))
				ld = ld1; ld1 = ld1->Chain;
		}
		if (ld == nullptr) Error(116);
		RdLex();
		if (lv->FTyp == 'f') goto label2; else goto label1;
	}
	TestIdentif();
	ld = LinkDRoot;
	while (ld != nullptr) {
		if ((ld->FromFD == CFile) && EquUpcase(ld->RoleName, LexWord)) {
			if ((ld->IndexRoot == 0)) Error(116);
			RdLex(); fd = ld->ToFD;
			if (Lexem == '(') {
				RdLex();
				if (!FindLocVar(&LVBD, &lv) || !(lv->FTyp == 'i' || lv->FTyp == 'r')) Error(177);
				RdLex();
				Accept(')');
				if (lv->FD != fd) OldError(149);
			label1:
				if (lv->FTyp == 'i') {
					KeyFldD* kf = WKeyDPtr(lv->RecPtr)->KFlds;
					if (ld->FromFD->IsSQLFile || ld->ToFD->IsSQLFile) OldError(155);
					if ((kf != nullptr) && !EquKFlds(kf, ld->ToKey->KFlds)) OldError(181);
				}
				LLV = lv;
				result = lv->FTyp;
				goto label3;
			}
			else {
			label2:
#ifdef FandSQL
				if (ld->ToFD->typSQLFile) Error(155);
#endif

				Accept('[');
				LLV = (LocVar*)RdRealFrml();
				Accept(']');
				result = 'F';
			label3:
				LLD = ld;
				return result;
			}
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
	KeyD* K = nullptr;

	FrmlPtr result = nullptr;

	//if (InpArrLen == 0x0571) {
	//	printf("RdFldNameFrmlP() %i", CurrPos);
	//}

	if (IsForwPoint())
		if (FindLocVar(&LVBD, &LV) && (LV->FTyp == 'i' || LV->FTyp == 'r')) {
			RdLex();
			result = RdRecVarFldFrml(LV, FTyp);
			return result;
		}
		else {
			pstring FName = LexWord;
			bool linked = IsRoleName(FileVarsAllowed, &FD, &LD);
			if (FD != nullptr) FName = FD->Name;
			if (!linked) RdLex();
			RdLex();
			FTyp = 'R';
			if (IsKeyWord("LASTUPDATE")) {
				Op = _lastupdate;
				if (FD != nullptr) goto label2;
				F = nullptr;
				goto label1;
			}
			if (IsKeyWord("ARCHIVES")) { F = CatArchiv; goto label0; }
			if (IsKeyWord("PATH")) { F = CatPathName; goto label0; }
			if (IsKeyWord("VOLUME")) {
				F = CatVolume;
			label0:
				FTyp = 'S';
			label1:
				auto S = new FrmlElem10(_catfield, 6); // Z = GetOp(_catfield, 6);
				S->CatFld = F;
				S->CatIRec = GetCatIRec(FName, true);
				TestCatError(S->CatIRec, FName, true);
				return S;
			}
			if (FD != nullptr) {
				if (IsKeyWord("GENERATION")) { Op = _generation; goto label2; }
				if (IsKeyWord("NRECSABS")) { Op = _nrecsabs; goto label2; }
				if (IsKeyWord("NRECS")) {
					Op = _nrecs;
				label2:
					auto N = new FrmlElem9(Op, 0); // Z = GetOp(Op, sizeof(FileDPtr));
					N->FD = FD;
					return N;
				}
			}
			if (linked) { result = RdFAccess(FD, LD, FTyp); return result; }
			if (FileVarsAllowed) OldError(9);
			else OldError(63);
		}
	if (ForwChar == '[') {
		auto A = new FrmlElem14(_accrecno, 8); // Z = GetOp(_accrecno, 8);
		FD = RdFileName();
		RdLex();
		A->RecFD = FD;
#ifdef FandSQL
		if (FD->typSQLFile) OldError(155);
#endif
		A->PPPPP1 = RdRealFrml();
		Accept(']');
		Accept('.');
		F = RdFldName(FD);
		A->RecFldD = F;
		FTyp = F->FrmlTyp;
		return A;
	}
	if (IsKeyWord("KEYPRESSED")) { Op = _keypressed; goto label3; }
	if (IsKeyWord("ESCPROMPT")) { Op = _escprompt; goto label3; }
	if (IsKeyWord("EDUPDATED")) {
		Op = _edupdated;
	label3:
		result = new FrmlElem0(Op, 0); // GetOp(Op, 0);
		FTyp = 'B';
		return result;
	}
	if (IsKeyWord("GETPATH")) {
		result = new FrmlElem0(_getpath, 0); // GetOp(_getpath, 0);
		FTyp = 'S';
		return result;
	}
	if (FindLocVar(&LVBD, &LV)) {
		if (LV->FTyp == 'r' || LV->FTyp == 'f' || LV->FTyp == 'i') Error(143);
		RdLex();
		result = new FrmlElem18(LV->Op, LV);
		//((FrmlElem18*)result)->BPOfs = LV->BPOfs;
		FTyp = LV->FTyp;
		return result;
	}
	if (FileVarsAllowed) {
		Z = TryRdFldFrml(CFile, FTyp);
		if (Z == nullptr) Error(8);
		result = Z;
		return result;
	}
	Error(8);
	return result;
}

FileD* RdPath(bool NoFD, pstring** Path, WORD& CatIRec)
{
	FileD* fd = nullptr;
	CatIRec = 0;
	if (Lexem == _quotedstr) { 
		*Path = RdStrConst(); 
		fd = nullptr; 
	}
	else {
		TestIdentif();
		fd = FindFileD();
		if (fd == nullptr) {
			CatIRec = GetCatIRec(LexWord, true);
			TestCatError(CatIRec, LexWord, false);
		}
		else if (NoFD) Error(97);
		RdLex();
	}
	return fd;
}

FrmlPtr RdFunctionP(char& FFTyp)
{
	FrmlElem* Z = nullptr;
	char Typ = '\0', FTyp = '\0';
	FileD* cf = nullptr;
	FileD* FD = nullptr;
	bool b = false;
	WORD N = 0;
	FrmlElem* Arg[30];
	char Op = '\0';
	LocVar* LV = nullptr;
	LinkD* LD = nullptr;
	void* p = nullptr;
	//WORD* pofs = (WORD*)&p;

	if (IsKeyWord("EVALB")) { FTyp = 'B'; goto label4; }
	else if (IsKeyWord("EVALS")) { FTyp = 'S'; goto label4; }
	else if (IsKeyWord("EVALR")) {
		FTyp = 'R';
	label4:
		RdLex();
		Z = new FrmlElem21(_eval, 5); // GetOp(_eval, 5);
		((FrmlElem21*)Z)->EvalTyp = FTyp;
		((FrmlElem21*)Z)->EvalP1 = RdStrFrml();
	}
	else if (FileVarsAllowed) Error(75);
	else if (IsKeyWord("PROMPT")) {
		RdLex();
		Z = new FrmlElem11(_prompt, 4); // GetOp(_prompt, 4);
		((FrmlElem11*)Z)->PPP1 = RdStrFrml();
		FieldDescr* F = RdFldDescr("", true);
		((FrmlElem11*)Z)->FldD = F; FTyp = F->FrmlTyp;
		if (F->Typ == 'T') OldError(65);
		if (Lexem == _assign) {
			RdLex();
			((FrmlElem11*)Z)->PP2 = RdFrml(Typ);
			if (Typ != FTyp) OldError(12);
		}
	}
	else if (IsKeyWord("KEYOF")) {
		RdLex();
		FTyp = 'S';
		if (!IsRecVar(&LV)) { Op = _recno; goto label11; }
		Z = new FrmlElem20(_keyof, 8); // GetOp(_keyof, 8);
		((FrmlElem20*)Z)->LV = LV;
		((FrmlElem20*)Z)->PackKey = RdViewKeyImpl(((FrmlElem20*)Z)->LV->FD);
		FTyp = 'S';
	}
	else if (IsKeyWord("RECNO")) {
		Op = _recno;
		goto label1;
	}
	else if (IsKeyWord("RECNOABS")) {
		Op = _recnoabs;
		goto label1;
	}
	else if (IsKeyWord("RECNOLOG")) {
		Op = _recnolog;
	label1:
		RdLex();
		FTyp = 'R';
	label11:
		FD = RdFileName();
		KeyD* K = RdViewKeyImpl(FD);
		if (Op == _recno) {
			KeyFldD* KF = K->KFlds;
			N = 0;
			if (KF == nullptr) OldError(176);
			while (KF != nullptr) {
				Accept(',');
				if (N > 29) Error(123);
				Arg[N] = RdFrml(Typ);
				N++;
				if (Typ != KF->FldD->FrmlTyp) OldError(12);
				KF = (KeyFldD*)KF->Chain;
			}
		}
		else {
			Accept(',');
			N = 1;
			Arg[1] = RdRealFrml();
		}
		Z = new FrmlElem13(Op, (N + 2) * 4); // GetOp(Op, (N + 2) * 4);
		auto iZ = (FrmlElem13*)Z;
		iZ->FFD = FD;
		iZ->Key = K;
		iZ->SaveArgs(Arg, N);
		//Move(Arg, iZ->Arg, 4 * N);
		//Z->Arg = Arg;
		if (FTyp == 'R') goto label2;
	}
	else if (IsKeyWord("LINK")) {
		RdLex();
		Z = new FrmlElem15(_link, 5); // GetOp(_link, 5);
		auto iZ = (FrmlElem15*)Z;
		if (IsRecVar(&LV)) {
			iZ->LinkFromRec = true;
			iZ->LinkLV = LV;
			FD = LV->FD;
		}
		else {
			FD = RdFileName();
			Accept('[');
			iZ->LinkRecFrml = RdRealFrml();
			Accept(']');
		}
		Accept(',');
#ifdef FandSQL
		if (FD->typSQLFile) OldError(155);
#endif
		cf = CFile;
		CFile = FD;
		if (!IsRoleName(true, &FD, &LD) || (LD == nullptr)) Error(9);
		CFile = cf;
		iZ->LinkLD = LD;
		FTyp = 'R';
		goto label2;
	}
	else if (IsKeyWord("ISDELETED")) {
		RdLex();
		FTyp = 'B';
		if (IsRecVar(&LV)) {
			Z = new FrmlElem20(_lvdeleted, 4); // GetOp(_lvdeleted, 4);
			((FrmlElem20*)Z)->LV = LV;
		}
		else {
			Z = new FrmlElem14(_isdeleted, 4); // GetOp(_isdeleted, 4);
			FD = RdFileName();
			((FrmlElem14*)Z)->RecFD = FD;
			Accept(',');
			((FrmlElem14*)Z)->PPPPP1 = RdRealFrml();
		label2: {}
#ifdef FandSQL
			if (FD->typSQLFile) Error(155);
#endif
		}
	}
	else if (IsKeyWord("GETPATH")) {
		RdLex();
		Z = new FrmlElem0(_getpath, 0); // GetOp(_getpath, 0);
		((FrmlElem0*)Z)->P1 = RdStrFrml();
		FTyp = 'S';
	}
	else if (IsKeyWord("GETTXT")) {
		RdLex();
		Z = new FrmlElem16(_gettxt, 6); // GetOp(_gettxt, 6);
		FTyp = 'S';
		goto label3;
	}
	else if (IsKeyWord("FILESIZE")) {
		RdLex();
		Z = new FrmlElem16(_filesize, 14); // GetOp(_filesize, 14);
		FTyp = 'R';
	label3:
		auto iZ = (FrmlElem16*)Z;
		RdPath(true, &iZ->TxtPath, iZ->TxtCatIRec);
		if ((Z->Op == _gettxt) && (Lexem == ',')) {
			RdLex();
			iZ->PPPPPP1 = RdRealFrml();
			if (Lexem == ',') {
				RdLex();
				iZ->PPPP2 = RdRealFrml();
			}
		}
	}
	else if (IsKeyWord("INTTSR")) {
		RdLex();
		Z = new FrmlElem0(_inttsr, 5); // GetOp(_inttsr, 5);
		auto iZ = (FrmlElem0*)Z;
		iZ->P1 = RdRealFrml(); Accept(',');
		iZ->P2 = RdRealFrml(); Accept(',');
		Typ = 'r';
		if (IsRecVar(&LV)) iZ->P3 = (FrmlElem*)LV->RecPtr;
		else iZ->P3 = RdFrml(Typ);
		iZ->N31 = Typ;
		FTyp = 'R';
	}
#ifdef FandSQL
	else if (IsKeyWord("SQL")) {
		RdLex(); Z = GetOp(_sqlfun, 0); Z->P1 = RdStrFrml(); FTyp = 'R';
	}
#endif
	else if (IsKeyWord("SELECTSTR")) {
		RdLex();
		Z = new FrmlElem0(_selectstr, 13); // GetOp(_selectstr, 13);
		FTyp = 'S';
		RdSelectStr((FrmlElem0*)Z);
	}
	else if (IsKeyWord("PROMPTYN")) {
		RdLex();
		Z = new FrmlElem0(_promptyn, 0); // GetOp(_promptyn, 0);
		((FrmlElem0*)Z)->P1 = RdStrFrml();
		FTyp = 'B';
	}
	else if (IsKeyWord("MOUSEEVENT")) {
		RdLex();
		Z = new FrmlElem1(_mouseevent, 2); // GetOp(_mouseevent, 2);
		((FrmlElem1*)Z)->W01 = RdInteger();
		FTyp = 'B';
	}
	else if (IsKeyWord("ISMOUSE")) {
		RdLex();
		Z = new FrmlElem1(_ismouse, 4); // GetOp(_ismouse, 4);
		((FrmlElem1*)Z)->W01 = RdInteger(); Accept(',');
		((FrmlElem1*)Z)->W02 = RdInteger(); FTyp = 'B';
	}
	else if (IsKeyWord("MOUSEIN")) {
		RdLex();
		Z = new FrmlElem0(_mousein, 4); // GetOp(_mousein, 4);
		auto iZ = (FrmlElem0*)Z;
		iZ->P1 = RdRealFrml(); Accept(',');
		iZ->P2 = RdRealFrml(); Accept(',');
		iZ->P3 = RdRealFrml(); Accept(',');
		iZ->P4 = RdRealFrml(); FTyp = 'B';
	}
	else if (IsKeyWord("PORTIN")) {
		RdLex();
		Z = new FrmlElem0(_portin, 0); // GetOp(_portin, 0);
		auto iZ = (FrmlElem0*)Z;
		iZ->P1 = RdBool(); Accept(',');
		iZ->P2 = RdRealFrml(); FTyp = 'R';
	}
	else Error(75);
	Accept(')');
	auto result = Z;
	FFTyp = FTyp;
	return result;
}

KeyD* RdViewKeyImpl(FileD* FD)
{
	KeyD* K = nullptr;
	if (FD != nullptr) K = FD->Keys;
	if (K == nullptr) Error(24);
	if (Lexem == '/') {
		FileD* cf = CFile;
		CFile = FD;
		K = RdViewKey();
		CFile = cf;
	}
	return K;
}

void RdSelectStr(FrmlElem0* Z)
{
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

//Instr* GetPInstr(PInstrCode Kind, WORD Size)
//{
//	Instr* PD = new Instr();
//	PD->Kind = Kind;
//	return PD;
//}

void RdPInstrAndChain(Instr** PD)
{

	Instr* PD1 = RdPInstr(); /*may be a chain itself*/
	if (*PD != nullptr) {
		Instr* last = *PD;
		while (last->Chain != nullptr) last = (Instr*)last->Chain;
		last->Chain = PD1;
	}
	else {
		*PD = PD1;
	}
}

void RdChoices(Instr_menu* PD)
{
	ChoiceD* CD = nullptr;
	WORD N = 0, SumL = 0;
	AcceptKeyWord("OF");
label1:
	if (IsKeyWord("ESCAPE")) {
		Accept(':');
		PD->WasESCBranch = true;
		PD->ESCInstr = RdPInstr();
	}
	else {
		//CD = (ChoiceD*)GetZStore(sizeof(CD));
		CD = new ChoiceD();
		if (PD->Choices == nullptr) PD->Choices = CD;
		else ChainLast(PD->Choices, CD);
		N++;
		if ((PD->Kind == _menubar) && (N > 30)) Error(102);
		CD->TxtFrml = RdStrFrml();
		if (Lexem == ',') {
			RdLex();
			if (Lexem != ',') {
				CD->HelpName = RdHelpName();
				PD->HelpRdb = CRdb;
			}
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
		Accept(':');
		CD->Instr = RdPInstr();
	}
	if (Lexem == ';') {
		RdLex();
		if (IsKeyWord("END")) return;
		goto label1;
	}
	AcceptKeyWord("END");
}

void RdMenuAttr(Instr_menu* PD)
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
	Instr_menu* PD = nullptr; pstring* S = nullptr;
	PD = new Instr_menu(_menubox); // GetPInstr(_menubox, 48);
	auto result = PD;
	PD->Loop = Loop;
	if (Lexem == '(') {
		RdLex();
		if (Lexem != ';') {
			PD->X = RdRealFrml();
			Accept(',');
			PD->Y = RdRealFrml();
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
	Instr_menu* PD = new Instr_menu(_menubar); // GetPInstr(_menubar, 48);
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

Instr_loops* RdIfThenElse()
{
	auto PD = new Instr_loops(_ifthenelseP); // GetPInstr(_ifthenelseP, 12);
	auto result = PD;
	PD->Bool = RdBool();
	AcceptKeyWord("THEN");
	PD->Instr1 = RdPInstr();
	if (IsKeyWord("ELSE")) PD->ElseInstr1 = RdPInstr();
	return result;
}

Instr_loops* RdWhileDo()
{
	auto PD = new Instr_loops(_whiledo); // GetPInstr(_whiledo, 8);
	auto result = PD;
	PD->Bool = RdBool();
	AcceptKeyWord("DO");
	PD->Instr1 = RdPInstr();
	return result;
}

Instr* RdFor()
{
	LocVar* LV = nullptr;
	if (!FindLocVar(&LVBD, &LV) || (LV->FTyp != 'R')) Error(146);
	RdLex();
	auto* PD = new Instr_assign(_asgnloc); // GetPInstr(_asgnloc, 9);
	auto result = PD;
	PD->AssLV = LV;
	Accept(_assign);
	PD->Frml = RdRealFrml();

	AcceptKeyWord("TO");
	auto iLoop = new Instr_loops(_whiledo); // GetPInstr(_whiledo, 8);
	PD->Chain = iLoop;
	//PD = (Instr_assign*)PD->Chain;
	auto Z1 = new FrmlElem0(_compreal, 2); // GetOp(_compreal, 2);
	Z1->P1 = nullptr;
	Z1->LV1 = LV;
	Z1->N21 = _le;
	Z1->N22 = 5;
	Z1->P2 = RdRealFrml();
	iLoop->Bool = Z1;

	AcceptKeyWord("DO");
	iLoop->Instr1 = RdPInstr();

	auto iAsg = new Instr_assign(_asgnloc); // GetPInstr(_asgnloc, 9);
	//ChainLast(iLoop->Instr1, iAsg);
	iLoop->AddInstr(iAsg);
	iAsg->Add = true;
	iAsg->AssLV = LV;
	auto Z2 = new FrmlElem2(_const, 0, 1); // GetOp(_const, sizeof(double));
	//Z->R = 1;
	iAsg->Frml = Z2;
	return result;
}

Instr* RdCase()
{
	Instr_loops* PD = nullptr;
	bool first = true;
label1:
	auto PD1 = new Instr_loops(_ifthenelseP); // GetPInstr(_ifthenelseP, 12);
	Instr* result = nullptr;
	if (first) result = PD1;
	else PD->ElseInstr1 = PD1;
	PD = PD1;
	first = false;
	PD->Bool = RdBool();
	Accept(':');
	PD->Instr1 = RdPInstr();
	bool b = Lexem == ';';
	if (b) RdLex();
	if (!IsKeyWord("END"))
		if (IsKeyWord("ELSE"))
			while (!IsKeyWord("END")) {
				RdPInstrAndChain(&PD->ElseInstr1);
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

Instr_loops* RdRepeatUntil()
{
	auto PD = new Instr_loops(_repeatuntil); // GetPInstr(_repeatuntil, 8);
	auto result = PD;
	while (!IsKeyWord("UNTIL")) {
		RdPInstrAndChain(&PD->Instr1);
		if (Lexem == ';') RdLex();
		else {
			AcceptKeyWord("UNTIL");
			goto label1;
		}
	}
label1: 
	PD->Bool = RdBool();
	return result;
}

Instr_forall* RdForAll()
{
	LocVar* LVi = nullptr;
	LocVar* LVr = nullptr;
	LinkD* LD = nullptr;
	FrmlElem* Z = nullptr;
	if (!FindLocVar(&LVBD, &LVi)) Error(122);
	RdLex();
	if (LVi->FTyp == 'r') {
		LVr = LVi;
		LVi = nullptr;
		CFile = LVr->FD;
	}
	else {
		TestReal(LVi->FTyp);
		AcceptKeyWord("IN");
		if (FindLocVar(&LVBD, &LVr)) {
			if (LVr->FTyp == 'f') {
				CFile = LVr->FD;
				RdLex();
				goto label1;
			}
			if (LVr->FTyp != 'r') Error(141);
			CFile = LVr->FD;
			RdLex();
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
	auto PD = new Instr_forall(); // GetPInstr(_forall, 41);
	PD->CFD = CFile;
	PD->CVar = LVi;
	// TODO: tady je podminka, by to nespadlo
	if (LVr != nullptr)	PD->CRecVar = LVr;
#ifdef FandSQL
	if (CFile->typSQLFile && IsKeyWord("IN")) {
		AcceptKeyWord("SQL"); Accept('('); PD->CBool = RdStrFrml(); Accept(')');
		PD->inSQL = true; goto label2;
	}
#endif
	if (IsKeyWord("OWNER")) {
		/* !!! with PD^ do!!! */
		PD->COwnerTyp = RdOwner(PD->CLD, PD->CLV);
		CViewKey = GetFromKey(PD->CLD);
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
	AcceptKeyWord("DO");
	PD->CInstr = RdPInstr();
	return PD;
	}

Instr* RdBeginEnd()
{
	Instr* PD = nullptr;
	if (!IsKeyWord("END")) {
	label1:
		/*if (InpArrLen == 1602 && CurrPos >= 880) {
			printf("RdBeginEnd()\n");
		}*/
		RdPInstrAndChain(&PD);
		if (Lexem == ';') {
			RdLex();
			if (!IsKeyWord("END")) goto label1;
		}
		else AcceptKeyWord("END");
	}
	return PD;
}

Instr_proc* RdProcArg(char Caller)
{
	RdbPos Pos;
	TypAndFrml TArg[31];
	LocVar* LV = nullptr;
	if (Caller != 'C') RdChptName('P', &Pos, Caller == 'P' || Caller == 'E' || Caller == 'T');
	WORD N = 0;
	if (Caller != 'P') { if (Lexem == '(') { RdLex(); goto label1; } }
	else if (Lexem == ',') {
		RdLex();
		Accept('(');
	label1:
		N++;
		if (N > 30) Error(123);
		if ((ForwChar != '.') && FindLocVar(&LVBD, &LV) && (LV->FTyp == 'i' || LV->FTyp == 'r'))
		{
			RdLex();
			TArg[N].FTyp = LV->FTyp; TArg[N].FD = LV->FD; TArg[N].RecPtr = LV->RecPtr;
		}
		else if (Lexem == '@')
		{
			RdLex();
			if (Lexem == '[') {
				RdLex();
				TArg[N].Name = *StoreStr(LexWord);
				Accept(_identifier);
				Accept(',');
				auto z = new FrmlElem0(_setmybp, 0); // GetOp(_setmybp, 0);
				z->P1 = RdStrFrml();
				TArg[N].TxtFrml = z;
				Accept(']');
			}
			else TArg[N].FD = RdFileName();
			TArg[N].FTyp = 'f';
		}
		else TArg[N].Frml = RdFrml(TArg[N].FTyp);
		if (Lexem == ',') { RdLex(); goto label1; }
		Accept(')');
	}
	if (Caller == 'E') { N++; TArg[N].FTyp = 'r'; }
	auto* PD = new Instr_proc(N); //GetPInstr(_proc, sizeof(RdbPos) + 2 + L);
	PD->PPos = Pos;
	PD->N = N;
	for (size_t i = 0; i < N; i++) {
		auto targ = TArg[i + 1];
		PD->TArg.push_back(targ); // do TArg ukladame od 1 - pozustatek
	}
	PD->ExPar = (Caller == 'E');
	return PD;
}

void RdKeyCode(EdExitD* X)
{
	WORD i = 0;
	EdExKeyD* E = new EdExKeyD();
	//E = (EdExKeyD*)GetStore(sizeof(EdExKeyD));
	E->Chain = X->Keys;
	X->Keys = E;
	if (NotCode("F", _F1_, 21, E)
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
		}
		Error(129);
	}
}

bool NotCode(pstring Nm, WORD CodeBase, WORD BrkBase, EdExKeyD* E)
{
	WORD i = 0, k = 0;
	auto result = true;
	if (Lexem != _identifier) return result;
	if (!SEquUpcase(copy(LexWord, 1, Nm.length()), Nm)) return result;
	val(copy(LexWord, Nm.length() + 1, 2), i, k);
	if ((k != 0) || (i <= 0) || (i > 10)) return result;
	i--;
	RdLex();
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
			EO->Mode = new FrmlElem4(_const, 0); // GetOp(_const, LexWord.length() + 1);
			((FrmlElem4*)EO->Mode)->S = LexWord;
			RdLex();
		}
		else EO->Mode = RdStrFrml();
	}
	else if (RdHeadLast(EO->Head)) return result;
	else if (IsOpt("WATCH")) EO->WatchDelayZ = RdRealFrml();
	else if (IsOpt("WW")) {
		Accept('('); EO->WFlags = 0;
		if (Lexem == '(') { RdLex(); EO->WFlags = WNoPop; }
		RdW(EO->W);
		RdFrame(&EO->Top, EO->WFlags);
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
		//EdExitD* X = (EdExitD*)GetZStore(sizeof(*X));
		EdExitD* X = new EdExitD();
		if (EO->ExD == nullptr) EO->ExD = X;
		else ChainLast(EO->ExD, X);
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
				if (IsOpt("ASSIGN")) RdPath(true, &RO->Path, RO->CatIRec);
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
	else if (IsKeyWord("RECORD")) {
		X->AtWrRec = true;
	}
	else if (IsKeyWord("NEWREC")) X->AtNewRec = true;
	else RdKeyCode(X);
	if (Lexem == ',') { RdLex(); goto label1; }
	Accept(':');
}

//Instr* GetPD(PInstrCode Kind, WORD Size)
//{
//	Instr* PD = GetPInstr(Kind, Size);
//	RdLex();
//	// TODO: toto tady bude muset být, musí ovlivnit výsledek RdPinstr!
//	// RdPInstr = PD;
//	return PD;
//}

void RdProcCall(Instr** pinstr)
{
	//Instr* PD = nullptr;
	if (IsKeyWord("EXEC")) *pinstr = RdExec();
	else if (IsKeyWord("COPYFILE")) *pinstr = RdCopyFile();
	else if (IsKeyWord("PROC")) {
		RdLex();
		*pinstr = RdProcArg('P');
	}
	else if (IsKeyWord("DISPLAY")) *pinstr = RdDisplay();
	else if (IsKeyWord("CALL")) *pinstr = RdRDBCall();
	else if (IsKeyWord("WRITELN")) RdWriteln(1, (Instr_writeln**)pinstr);
	else if (IsKeyWord("WRITE")) RdWriteln(0, (Instr_writeln**)pinstr);
	else if (IsKeyWord("HEADLINE")) {
		*pinstr = new Instr_assign(_headline); // GetPD(_headline, 4);
		RdLex();
		goto label1;
	}
	else if (IsKeyWord("SETKEYBUF")) {
		*pinstr = new Instr_assign(_setkeybuf); //GetPD(_setkeybuf, 4);
		RdLex();
		goto label1;
	}
	else if (IsKeyWord("HELP")) {
		*pinstr = new Instr_help(); // GetPD(_help, 8);
		RdLex();
		if (CRdb->HelpFD == nullptr) OldError(132);
		((Instr_help*)*pinstr)->HelpRdb0 = CRdb;
	label1:
		((Instr_help*)*pinstr)->Frml0 = RdStrFrml();
	}
	else if (IsKeyWord("MESSAGE")) RdWriteln(2, (Instr_writeln**)pinstr);
	else if (IsKeyWord("GOTOXY")) *pinstr = RdGotoXY();
	else if (IsKeyWord("MERGE")) {
		// PD = (Instr_merge_display*)GetPD(_merge, sizeof(RdbPos));
		*pinstr = new Instr_merge_display(_merge);
		RdLex();
		RdChptName('M', &((Instr_merge_display*)*pinstr)->Pos, true);
	}
	else if (IsKeyWord("SORT")) *pinstr = RdSortCall();
	else if (IsKeyWord("EDIT")) *pinstr = RdEditCall();
	else if (IsKeyWord("REPORT")) *pinstr = RdReportCall();
	else if (IsKeyWord("EDITTXT")) *pinstr = RdEditTxt();
	else if (IsKeyWord("PRINTTXT")) *pinstr = RdPrintTxt();
	else if (IsKeyWord("PUTTXT")) *pinstr = RdPutTxt();
	else if (IsKeyWord("TURNCAT")) *pinstr = RdTurnCat();
	else if (IsKeyWord("RELEASEDRIVE")) *pinstr = RdReleaseDrive();
	else if (IsKeyWord("SETPRINTER")) {
		*pinstr = new Instr_assign(_setprinter); // GetPD(_setprinter, 4);
		RdLex();
		goto label2;
	}
	else if (IsKeyWord("INDEXFILE")) *pinstr = RdIndexfile();
	else if (IsKeyWord("GETINDEX"))*pinstr = RdGetIndex();
	else if (IsKeyWord("MOUNT")) *pinstr = RdMount();
	else if (IsKeyWord("CLRSCR")) *pinstr = RdClrWw();
	else if (IsKeyWord("APPENDREC")) *pinstr = RdMixRecAcc(_appendrec);
	else if (IsKeyWord("DELETEREC")) *pinstr = RdMixRecAcc(_deleterec);
	else if (IsKeyWord("RECALLREC")) *pinstr = RdMixRecAcc(_recallrec);
	else if (IsKeyWord("READREC")) *pinstr = RdMixRecAcc(_readrec);
	else if (IsKeyWord("WRITEREC")) *pinstr = RdMixRecAcc(_writerec);
	else if (IsKeyWord("LINKREC")) *pinstr = RdLinkRec();
	else if (IsKeyWord("DELAY")) {
		*pinstr = new Instr_assign(_delay); // GetPD(_delay, 4);
		RdLex();
		goto label2;
	}
	else if (IsKeyWord("SOUND")) {
		*pinstr = new Instr_assign(_sound); // GetPD(_sound, 4);
		RdLex();
	label2:
		((Instr_assign*)*pinstr)->Frml = RdRealFrml();
	}
#ifdef FandProlog
	else if (IsKeyWord("LPROC")) *pinstr = RdCallLProc();
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
		*pinstr = new Instr_closefds(); // GetPD(_closefds, 4);
		RdLex();
		((Instr_closefds*)*pinstr)->clFD = RdFileName();
	}
	else if (IsKeyWord("BACKUP")) *pinstr = RdBackup(' ', true);
	else if (IsKeyWord("BACKUPM")) *pinstr = RdBackup('M', true);
	else if (IsKeyWord("RESTORE")) *pinstr = RdBackup(' ', false);
	else if (IsKeyWord("RESTOREM")) *pinstr = RdBackup('M', false);
	else if (IsKeyWord("SETEDITTXT")) *pinstr = RdSetEditTxt();
	else if (IsKeyWord("SETMOUSE")) {
		*pinstr = new Instr_setmouse(); // GetPD(_setmouse, 12);
		RdLex();
		((Instr_setmouse*)*pinstr)->MouseX = RdRealFrml(); Accept(',');
		((Instr_setmouse*)*pinstr)->MouseY = RdRealFrml(); Accept(',');
		((Instr_setmouse*)*pinstr)->Show = RdBool();
	}
	else if (IsKeyWord("CHECKFILE")) {
		*pinstr = new Instr_checkfile(); // GetPD(_checkfile, 10);
		RdLex();
		auto iPD = (Instr_checkfile*)*pinstr;
		iPD->cfFD = RdFileName();
		/* !!! with PD->cfFD^ do!!! */
		if ((iPD->cfFD->Typ == '8' || iPD->cfFD->Typ == 'D')
#ifdef FandSQL
			|| PD->cfFD->typSQLFile
#endif
			) OldError(169);
		Accept(',');
		RdPath(true, &iPD->cfPath, iPD->cfCatIRec);
	}
#ifdef FandSQL
	else if (IsKeyWord("SQL")) { PD = GetPD(_sql, 4); goto label1; }
	else if (IsKeyWord("LOGIN")) {
		PD = GetPD(_login, 8);
		/* !!! with PD^ do!!! */ {
			PD->liName = RdStrFrml(); Accept(','); PD->liPassWord = RdStrFrml();
		}
	}
	else if (IsKeyWord("SQLRDTXT")) RdSqlRdWrTxt(true);
	else if (IsKeyWord("SQLWRTXT")) RdSqlRdWrTxt(false);
#endif 
	else if (IsKeyWord("PORTOUT")) {
		*pinstr = new Instr_portout(); // GetPD(_portout, 12);
		RdLex();
		auto iPD = (Instr_portout*)*pinstr;
		/* !!! with PD^ do!!! */
		{
			iPD->IsWord = RdBool(); Accept(',');
			iPD->Port = RdRealFrml(); Accept(',');
			iPD->PortWhat = RdRealFrml();
		}
	}
	else Error(34);
	Accept(')');
}

FieldList RdFlds()
{
	FieldListEl* FLRoot = nullptr; FieldListEl* FL = nullptr;
label1:
	FL = new FieldListEl(); // (FieldList)GetStore(sizeof(*FL));
	if (FLRoot == nullptr) { FLRoot = FL; FL->Chain = nullptr; }
	else ChainLast(FLRoot, FL);
	FL->FldD = RdFldName(CFile);
	if (Lexem == ',') { RdLex(); goto label1; }
	return FLRoot;
}

FieldList RdSubFldList(FieldList InFL, char Opt)
{
	FieldListEl* FLRoot = nullptr;
	FieldListEl* FL = nullptr;
	FieldListEl* FL1 = nullptr;
	FieldDescr* F = nullptr;
	Accept('(');
label1:
	FL = new FieldListEl(); // (FieldList)GetStore(sizeof(*FL));
	if (FLRoot == nullptr) { FLRoot = FL; FL->Chain = nullptr; }
	else ChainLast(FLRoot, FL);
	if (InFL == nullptr) F = RdFldName(CFile);
	else {
		TestIdentif();
		FL1 = InFL;
		while (FL1 != nullptr)
		{
			if (EquUpcase(FL1->FldD->Name, LexWord)) goto label2;
			FL1 = (FieldList)FL1->Chain;
		}
		Error(43);
	label2:
		F = FL1->FldD;
		RdLex();
	}
	FL->FldD = F;
	if ((Opt == 'S') && (F->FrmlTyp != 'R')) OldError(20);
	if (Lexem == ',') { RdLex(); goto label1; }
	Accept(')');
	return FLRoot;
}

Instr_sort* RdSortCall()
{
	auto PD = new Instr_sort(); // GetPD(_sort, 8);
	RdLex();
	FileD* FD = RdFileName();
	PD->SortFD = FD;
#ifdef FandSQL
	if (FD->typSQLFile) OldError(155);
#endif
	Accept(',');
	Accept('(');
	RdKFList(&PD->SK, PD->SortFD);
	Accept(')');
	return PD;
}

Instr_edit* RdEditCall()
{
	stSaveState* p = nullptr;
	bool b = false;
	KeyD* K = nullptr;
	LocVar* lv = nullptr;
	Instr_edit* PD = new Instr_edit(); // GetPD(_edit, 8);
	RdLex();
	EditOpt* EO = GetEditOpt();
	PD->EO = EO;
	if (IsRecVar(&lv)) { EO->LVRecPtr = lv->RecPtr; CFile = lv->FD; }
	else {
		CFile = RdFileName();
		K = RdViewKey();
		if (K == nullptr) K = CFile->Keys;
		EO->ViewKey = K;
	}
	PD->EditFD = CFile;
	Accept(',');
	if (IsOpt("U")) {
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
	return PD;
}

void RdEditOpt(EditOpt* EO)
{
	LocVar* lv = nullptr;
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
		EO->SelKey = (XWKey*)lv->RecPtr;
		if ((EO->ViewKey == nullptr)) OldError(108);
		if (EO->ViewKey == EO->SelKey) OldError(184);
		if ((EO->ViewKey->KFlds != nullptr)
			&& (EO->SelKey->KFlds != nullptr)
			&& !EquKFlds(EO->SelKey->KFlds, EO->ViewKey->KFlds)) OldError(178);
	}
	else Error(125);
}

Instr* RdReportCall()
{
	Instr_report* PD = nullptr;
	RprtOpt* RO = nullptr;
	LocVar* lv = nullptr;
	RprtFDListEl* FDL = nullptr;
	bool b = false;
	bool hasfrst;
	PD = new Instr_report(); // GetPD(_report, 4);
	RdLex();
	RO = GetRprtOpt();
	PD->RO = RO;
	hasfrst = false;
	if (Lexem == ',') goto label2;
	hasfrst = true;
	FDL = &RO->FDL;
	b = false;
	if (Lexem == '(') { RdLex(); b = true; }
label1:
	if (IsRecVar(&lv)) { FDL->LVRecPtr = lv->RecPtr; FDL->FD = lv->FD; }
	else {
		CFile = RdFileName();
		FDL->FD = CFile;
		CViewKey = RdViewKey();
		FDL->ViewKey = CViewKey;
		if (Lexem == '(') {
			RdLex();
			FDL->Cond = RdKeyInBool(FDL->KeyIn, true, true, FDL->SQLFilter);
			Accept(')');
		}
	}
	if (b && (Lexem == ',')) {
		RdLex();
		FDL->Chain = new RprtFDListEl(); // (RprtFDListEl*)GetZStore(sizeof(RprtFDListEl));
		FDL = FDL->Chain;
		goto label1;
	}
	if (b) Accept(')');
	CFile = RO->FDL.FD; CViewKey = RO->FDL.ViewKey;
label2:
	Accept(',');
	if (Lexem == '[') {
		RdLex();
		RO->RprtPos.R = (RdbD*)RdStrFrml();
		RO->RprtPos.IRec = 0;
		RO->FromStr = true;
		Accept(']');
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
		}
		Accept(')');
	}
	while (Lexem == ',') {
		RdLex(); RdRprtOpt(RO, (hasfrst && (FDL->LVRecPtr == nullptr)));
	}
	if ((RO->Mode == _ALstg) && ((RO->Ctrl != nullptr) || (RO->Sum != nullptr)))
		RO->Mode = _ARprt;
	return PD;
}

void RdRprtOpt(RprtOpt* RO, bool HasFrst)
{
	FileD* FD = nullptr;
	WORD N = 0;
	/* !!! with RO^ do!!! */
	if (IsOpt("ASSIGN")) RdPath(true, &RO->Path, RO->CatIRec);
	else if (IsOpt("TIMES")) RO->Times = RdRealFrml();
	else if (IsOpt("MODE"))
		if (IsKeyWord("ONLYSUM")) RO->Mode = _ATotal;
		else if (IsKeyWord("ERRCHECK")) RO->Mode = _AErrRecs; else Error(49);
	else if (IsKeyWord("COND")) {
		if (!HasFrst) goto label2;
		WORD Low = CurrPos;
		Accept(_equ);
		bool br = false;
		if (Lexem == '(') {
			Low = CurrPos;
			RdLex();
			br = true;
			if (Lexem == '?') { RdLex(); RO->UserCondQuest = true; goto label1; };
		}
		RO->FDL.Cond = RdKeyInBool(RO->FDL.KeyIn, true, true, RO->FDL.SQLFilter);
		N = OldErrPos - Low;
		RO->CondTxt = new pstring(); // (pstring*)GetStore(N + 1);
		Move(&InpArrPtr[Low], &(*RO->CondTxt)[1], N);
		(*RO->CondTxt)[0] = N;
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
		Accept('('); RdKFList(&RO->SK, CFile); Accept(')');
	}
	else if (IsOpt("HEAD")) RO->Head = RdStrFrml();
	else Error(45);
}

Instr* RdRDBCall()
{
	pstring s;
	auto PD = new Instr_call(); // GetPD(_call, 12);
	RdLex();
	s[0] = 0;
	if (Lexem == '\\') { s = Lexem; RdLex(); }
	TestIdentif();
	if (LexWord.length() > 8) Error(2);
	PD->RdbNm = StoreStr(s + LexWord);
	RdLex();
	if (Lexem == ',') {
		RdLex();
		TestIdentif();
		if (LexWord.length() > 12) Error(2);
		PD->ProcNm = StoreStr(LexWord);
		RdLex();
		PD->ProcCall = RdProcArg('C');
	}
	else PD->ProcNm = StoreStr("main");
	return PD;
}

Instr* RdExec()
{
	auto PD = new Instr_exec(); // GetPD(_exec, 14);
	RdLex();
	RdPath(true, &PD->ProgPath, PD->ProgCatIRec);
	Accept(',');
	PD->Param = RdStrFrml();
	while (Lexem == ',') {
		RdLex();
		if (IsKeyWord("NOCANCEL")) PD->NoCancel = true;
		else if (IsKeyWord("FREEMEM")) PD->FreeMm = true;
		else if (IsKeyWord("LOADFONT")) PD->LdFont = true;
		else if (IsKeyWord("TEXTMODE")) PD->TextMd = true;
		else Error(101);
	}
	return PD;
}

Instr* RdCopyFile()
{
	pstring ModeTxt[7] = { "KL","LK","KN","LN","LW","KW","WL" };
	FieldDPtr* F = nullptr;
	WORD i = 0;
	CopyD* D = nullptr;
	bool noapp = false;
	auto PD = new Instr_copyfile(); // GetPD(_copyfile, 4);
	RdLex();
	noapp = false;
	D = new CopyD(); // (CopyD*)GetZStore(sizeof(*D));
	PD->CD = D;
	/* !!! with D^ do!!! */
	D->FD1 = RdPath(false, &D->Path1, D->CatIRec1);
	D->WithX1 = RdX(D->FD1);
	if (Lexem == '/')
	{
		if (D->FD1 != nullptr) { CFile = D->FD1; D->ViewKey = RdViewKey(); }
		else D->Opt1 = RdCOpt();
	}
	Accept(',');
	D->FD2 = RdPath(false, &D->Path2, D->CatIRec2);
	D->WithX2 = RdX(D->FD2);
	if (Lexem == '/') {
		if (D->FD2 != nullptr) Error(139);
		else D->Opt2 = RdCOpt();
	}
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
				&& ((D->HdF->Flg & f_Stored) == 0)) Error(52);
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
	return PD;
}

CpOption RdCOpt()
{
	BYTE i = 0;
	pstring OptArr[3] = { "FIX", "VAR", "TXT" };
	RdLex();
	TestIdentif();
	for (i = 0; i < 3; i++)
		if (EquUpcase(OptArr[i], LexWord)) {
			RdLex();
			return CpOption(i + 1); // vracime i + 1 (CpOption ma 4 moznosti, je to posunute ...)
		}
	Error(53);
	throw std::exception("Bad value in RdCOpt() in rdproc.cpp");
}

bool RdX(FileD* FD)
{
	auto result = false;
	if ((Lexem == '.') && (FD != nullptr)) {
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

Instr* RdPrintTxt()
{
	auto PD = new Instr_edittxt(_printtxt); // GetPD(_printtxt, 10);
	RdLex();
	/* !!! with PD^ do!!! */
	if (FindLocVar(&LVBD, &PD->TxtLV)) { RdLex(); TestString(PD->TxtLV->FTyp); }
	else RdPath(true, &PD->TxtPath, PD->TxtCatIRec);
	return PD;
}

Instr* RdEditTxt()
{
	//Instr* PD;
	EdExitD* pX;
	auto PD = new Instr_edittxt(_edittxt); // GetPD(_edittxt, 73);
	RdLex();
	/* !!! with PD^ do!!! */
	if (FindLocVar(&LVBD, &PD->TxtLV)) { RdLex(); TestString(PD->TxtLV->FTyp); }
	else RdPath(true, &PD->TxtPath, PD->TxtCatIRec);
	PD->EdTxtMode = 'T';
	while (Lexem == ',') {
		RdLex();
		if (IsOpt("WW")) {
			Accept('(');
			if (Lexem == '(') { RdLex(); PD->WFlags = WNoPop; }
			RdW(PD->Ww);
			RdFrame(&PD->Hd, PD->WFlags);
			if (Lexem == ',') { RdLex(); PD->Atr = RdAttr(); }
			Accept(')');
			if ((PD->WFlags & WNoPop) != 0) Accept(')');
		}
		else
			if (IsOpt("TXTPOS")) PD->TxtPos = RdRealFrml();
			else if (IsOpt("TXTXY")) PD->TxtXY = RdRealFrml();
			else if (IsOpt("ERRMSG")) PD->ErrMsg = RdStrFrml();
			else if (IsOpt("EXIT")) {
				Accept('(');
			label1:
				pX = new EdExitD(); // (EdExitD*)GetZStore(sizeof(*pX));
				if (PD->ExD == nullptr) PD->ExD = pX;
				else ChainLast(PD->ExD, pX);
			label2:
				RdKeyCode(pX);
				if (Lexem == ',') { RdLex(); goto label2; }
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
	return PD;
}

Instr* RdPutTxt()
{
	auto PD = new Instr_puttxt(); // GetPD(_puttxt, 11);
	RdLex();
	RdPath(true, &PD->TxtPath1, PD->TxtCatIRec1);
	Accept(',');
	PD->Txt = RdStrFrml();
	if (Lexem == ',')
	{
		RdLex();
		AcceptKeyWord("APPEND");
		PD->App = true;
	}
	return PD;
}

Instr* RdTurnCat()
{
	auto PD = new Instr_turncat(); // GetPD(_turncat, 12);
	RdLex();
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
	return PD;
}

void RdWriteln(BYTE OpKind, Instr_writeln** pinstr)
{
	WrLnD* d = new WrLnD();
	RdLex();
	FrmlElem* z = nullptr;
	//FillChar(&d, sizeof(d), 0); 
	WrLnD* w = d;
label1:
	/* !!! with w^ do!!! */
	w->Frml = RdFrml(w->Typ);
	if (w->Typ == 'R') {
		w->Typ = 'F';
		if (Lexem == ':') {
			RdLex();
			if (Lexem == _quotedstr) {
				w->Typ = 'D';
				w->Mask = StoreStr(LexWord);
				RdLex();
			}
			else {
				w->N = RdInteger();
				if (Lexem == ':') {
					RdLex();
					if (Lexem == '-') {
						RdLex();
						w->M = -RdInteger();
					}
					else w->M = RdInteger();
				}
			}
		}
	}
	if (Lexem == ',') {
		RdLex();
		if ((OpKind == 2) && IsOpt("HELP")) z = RdStrFrml();
		else {
			//w = (WrLnD*)GetZStore(sizeof(d));
			w = new WrLnD();
			if (d == nullptr) d = w;
			else ChainLast(d, w);
			goto label1;
		}
	}
	WORD N = 1 + sizeof(d);
	if (z != nullptr) { OpKind = 3; N += 8; }
	auto pd = new Instr_writeln(); // GetPInstr(_writeln, N);
	/* !!! with pd^ do!!! */
	pd->LF = OpKind;
	pd->WD = *d;
	if (OpKind == 3) { pd->mHlpRdb = CRdb; pd->mHlpFrml = z; }
	*pinstr = pd;
}

Instr* RdReleaseDrive()
{
	auto PD = new Instr_releasedrive(); // GetPD(_releasedrive, 4);
	RdLex();
	PD->Drive = RdStrFrml();
	return PD;
}

Instr* RdIndexfile()
{
	auto PD = new Instr_indexfile(); // GetPD(_indexfile, 5);
	RdLex();
	PD->IndexFD = RdFileName();
	if (PD->IndexFD->Typ != 'X') OldError(108);
	if (Lexem == ',') {
		RdLex();
		AcceptKeyWord("COMPRESS");
		PD->Compress = true;
	}
	return PD;
}

Instr* RdGetIndex()
{
	LocVar* lv2 = nullptr; bool b = false; LinkD* ld = nullptr;
	auto PD = new Instr_getindex(); // GetPD(_getindex, 31);
	RdLex();
	LocVar* lv = RdIdxVar();
	PD->giLV = lv; Accept(',');
	PD->giMode = ' ';
	if (Lexem == '+' || Lexem == '-') {
		PD->giMode = Lexem;
		RdLex();
		Accept(',');
		PD->giCond = RdRealFrml(); /*RecNr*/
		return PD;
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
			RdKFList(&PD->giKFlds, CFile); Accept(')');
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
	return PD;
}

Instr* RdGotoXY()
{
	auto PD = new Instr_gotoxy(); // GetPD(_gotoxy, 8);
	RdLex();
	PD->GoX = RdRealFrml();
	Accept(',');
	PD->GoY = RdRealFrml();
	return PD;
}

Instr* RdClrWw()
{
	auto PD = new Instr_clrww(); // GetPD(_clrww, 24);
	RdLex();
	/* !!! with PD^ do!!! */
	RdW(PD->W2);
	if (Lexem == ',') {
		RdLex();
		if (Lexem != ',') PD->Attr2 = RdAttr();
		if (Lexem == ',') { RdLex(); PD->FillC = RdStrFrml(); }
	}
	return PD;
}

Instr* RdMount()
{
	auto PD = new Instr_mount(); // GetPD(_mount, 3);
	RdLex();
	WORD I = 0;
	TestIdentif();
	FileD* FD = FindFileD();
	if (FD == nullptr) I = GetCatIRec(LexWord, true);
	else I = FD->CatIRec;
	TestCatError(I, LexWord, false);
	RdLex();
	PD->MountCatIRec = I;
	if (Lexem == ',') {
		RdLex();
		AcceptKeyWord("NOCANCEL");
		PD->MountNoCancel = true;
	}
	return PD;
}

Instr* RdDisplay()
{
	auto PD = new Instr_merge_display(_display); // GetPD(_display, sizeof(RdbPos));
	RdLex();
	pstring* s = nullptr;
	if ((Lexem == _identifier) && FindChpt('H', LexWord, false, &PD->Pos)) RdLex();
	else {
		/* !!! with PD->Pos do!!! */
		PD->Pos.R = RdbDPtr(RdStrFrml);
		PD->Pos.IRec = 0;
	}
	return PD;
}

void RdGraphP()
{
	Instr_graph* PD;
	FrmlPtr FrmlArr[15];
	WORD i;
	GraphVD* VD; GraphWD* WD; GraphRGBD* RGBD; WinG* Ww;

	pstring Nm1[11] = { "TYPE","HEAD","HEADX","HEADY","HEADZ","FILL","DIRX","GRID","PRINT","PALETTE","ASSIGN" };
	pstring Nm2[6] = { "WIDTH","RECNO","NRECS","MAX","MIN","GRPOLY" };

	PD = new Instr_graph(); // GetPD(_graph, 4);
	RdLex();
	PD->GD = new GraphD(); // GetZStore(sizeof(GraphD));
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
			//Ww = (WinG*)GetZStore(sizeof(*Ww));
			Ww = new WinG();
			Accept('(');
			if (Lexem == '(') { RdLex(); Ww->WFlags = WNoPop; }
			RdW(Ww->W);
			RdFrame(&Ww->Top, Ww->WFlags);
			if (Lexem == ',') {
				RdLex(); Ww->ColBack = RdStrFrml(); Accept(',');
				Ww->ColFor = RdStrFrml();
				Accept(','); Ww->ColFrame = RdStrFrml();
			}
			Accept(')');
			if ((Ww->WFlags & WNoPop) != 0) Accept(')');
		}
		else Error(44);
	label1: {}
	}
}

Instr_recs* RdMixRecAcc(PInstrCode Op)
{
	Instr_recs* PD = nullptr;
	FrmlElem* Z = nullptr;
	char FTyp = '\0';
	FileD* cf = CFile;
	if ((Op == _appendrec) || (Op == _recallrec)) {
		// PD = GetPD(Op, 9);
		PD = new Instr_recs(Op);
		RdLex();
		CFile = RdFileName();
		PD->RecFD = CFile;
#ifdef FandSQL
		if (CFile->typSQLFile) OldError(155);
#endif
		if (Op == _recallrec) {
			Accept(',');
			PD->RecNr = RdRealFrml();
		}
}
	else {
		// PD = GetPD(Op, 15);
		PD = new Instr_recs(Op);
		RdLex();
		if (Op == _deleterec) {
			CFile = RdFileName();
			PD->RecFD = CFile;
		}
		else { /*_readrec,_writerec*/
			if (!IsRecVar(&PD->LV)) Error(141);
			CFile = PD->LV->FD;
		}
		KeyD* K = RdViewKey();
		Accept(',');
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
				|| (((FrmlElem4*)Z)->S[0] != 0))) OldError(24);
			break;
		}
#ifdef FandSQL
		default: {
			if (PD->CompOp != 0) OldError(19);
			if (CFile->typSQLFile && ((Op == _deleterec) || (Z->Op != _const)
				|| (Z->R != 0))) Error(155);
			break;
		}
#endif
		}
		}
	if ((Lexem == ',') && (Op == _writerec || Op == _deleterec || Op == _recallrec)) {
		RdLex();
		Accept('+');
		PD->AdUpd = true;
	}
	CFile = cf;
	return PD;
	}

Instr* RdLinkRec()
{
	LocVar* LV = nullptr;
	LinkD* LD = nullptr;
	auto PD = new Instr_assign(_linkrec); // GetPD(_linkrec, 12);
	RdLex();
	if (!IsRecVar(&PD->RecLV1)) Error(141);
	Accept(',');
	CFile = PD->RecLV1->FD;
	if (IsRecVar(&LV)) {
		LD = FindLD(LV->FD->Name);
		if (LD == nullptr) OldError(154);
	}
	else {
		TestIdentif();
		LD = FindLD(LexWord);
		if (LD == nullptr) Error(9);
		RdLex();
		Accept('(');
		LV = RdRecVar();
		if (LD->ToFD != LV->FD) OldError(141);
		Accept(')');
	}
	PD->RecLV2 = LV;
	PD->LinkLD = LD;
	return PD;
}

Instr* RdSetEditTxt()
{
	auto PD = new Instr_setedittxt(); // GetPD(_setedittxt, 7 * 4);
	RdLex();
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
	return PD;
}

FrmlElem* AdjustComma(FrmlElem* Z1, FieldDescr* F, char Op)
{
	FrmlElem0* Z = nullptr;
	FrmlElem2* Z2 = nullptr;
	auto result = Z1;
	if (F->Typ != 'F') return result;
	if ((F->Flg & f_Comma) == 0) return result;
	Z2 = new FrmlElem2(_const, 0, Power10[F->M]); // GetOp(_const, sizeof(double));
	//Z2->R = Power10[F->M];
	Z = new FrmlElem0(Op, 0); // GetOp(Op, 0);
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
		if ((F1->Flg & f_Stored) != 0) {
			LexWord = F1->Name;
			FieldDescr* F2 = FindFldName(FD2);
			if (F2 != nullptr) {
				//A = (AssignD*)GetZStore(sizeof(*A));
				A = new AssignD();
				if (ARoot == nullptr) ARoot = A;
				else ChainLast(ARoot, A);
				if ((F2->FrmlTyp != F1->FrmlTyp)
					|| (F1->FrmlTyp == 'R')
					&& (F1->Typ != F2->Typ)) {
					A->Kind = _zero;
					A->FldD = F1;
				}
				else {
					A->Kind = _output;
					A->OFldD = F1;
					FrmlElem* Z = MakeFldFrml(F2, FTyp);
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

Instr_assign* RdAssign()
{
	FileD* FD = nullptr; FieldDescr* F = nullptr;
	LocVar* LV = nullptr; LocVar* LV2 = nullptr; char PV;
	Instr_assign* PD = nullptr; pstring FName; char FTyp = 0;
	if (ForwChar == '.')
		if (FindLocVar(&LVBD, &LV) && (LV->FTyp == 'r' || LV->FTyp == 'i')) {
			FTyp = LV->FTyp;
			RdLex(); RdLex();
			if (FTyp == 'i') {
				AcceptKeyWord("NRECS");
				Accept(_assign);
				if ((Lexem != _number) || (LexWord != '0')) Error(183);
				RdLex();
				PD = new Instr_assign(_asgnxnrecs); // GetPInstr(_asgnxnrecs, 4);
				PD->xnrIdx = (WKeyDPtr)LV->RecPtr;
			}
			else {
				PD = new Instr_assign(_asgnrecfld); // GetPInstr(_asgnrecfld, 13);
				PD->AssLV = LV;
				F = RdFldName(LV->FD);
				PD->RecFldD = F;
				if ((F->Flg & f_Stored) == 0) OldError(14);
				FTyp = F->FrmlTyp;
			label0:
				RdAssignFrml(FTyp, PD->Add, &PD->Frml);
			}
		}
		else {
			FName = LexWord;
			FD = FindFileD();
			if (IsActiveRdb(FD)) Error(121);
			RdLex(); RdLex();
			if (IsKeyWord("ARCHIVES")) { F = CatArchiv; goto label1; }
			if (IsKeyWord("PATH")) { F = CatPathName; goto label1; }
			if (IsKeyWord("VOLUME")) {
				F = CatVolume;
			label1:
				PD = new Instr_assign(_asgncatfield); // GetPInstr(_asgncatfield, 16);
				PD->FD3 = FD;
				PD->CatIRec = GetCatIRec(FName, true);
				PD->CatFld = F;
				TestCatError(PD->CatIRec, FName, true);
				Accept(_assign);
				PD->Frml3 = RdStrFrml();
			}
			else if (FD == nullptr) OldError(9);
			else if (IsKeyWord("NRECS")) {
				if (FD->Typ == '0') OldError(127);
				PD = new Instr_assign(_asgnnrecs); // GetPInstr(_asgnnrecs, 9);
				PD->FD = FD; FTyp = 'R'; goto label0;
			}
			else {
				if (!FD->IsParFile) OldError(64);
				PD = new Instr_assign(_asgnpar); // GetPInstr(_asgnpar, 13);
				PD->FD = FD;
				F = RdFldName(FD);
				PD->FldD = F;
				if ((F->Flg & f_Stored) == 0) OldError(14);
				FTyp = F->FrmlTyp;
				goto label0;
			}
		}
	else if (ForwChar == '[') {
		PD = new Instr_assign(_asgnfield); // GetPInstr(_asgnfield, 18);
		FD = RdFileName();
		PD->FD = FD; RdLex();
#ifdef FandSQL
		if (FD->typSQLFile) OldError(155);
#endif
		PD->RecFrml = RdRealFrml();
		Accept(']');
		Accept('.');
		F = RdFldName(FD);
		PD->FldD = F;
		if ((F->Flg & f_Stored) == 0) OldError(14);
		PD->Indexarg = (FD->Typ == 'X') && IsKeyArg(F, FD);
		RdAssignFrml(F->FrmlTyp, PD->Add, &PD->Frml);
	}
	else if (FindLocVar(&LVBD, &LV)) {
		RdLex();
		FTyp = LV->FTyp;
		switch (FTyp) {
		case 'f':
		case 'i': OldError(140); break;
		case 'r': {
			Accept(_assign);
			if (!IsRecVar(&LV2)) Error(141);
			PD = new Instr_assign(_asgnrecvar); // GetPInstr(_asgnrecvar, 12);
			PD->RecLV1 = LV;
			PD->RecLV2 = LV2;
			PD->Ass = MakeImplAssign(LV->FD, LV2->FD);
			break;
		}
		default: {
			PD = new Instr_assign(_asgnloc); // GetPInstr(_asgnloc, 9);
			PD->AssLV = LV; goto label0;
			break;
		}
		}
	}
	else if (IsKeyWord("USERNAME"))
	{
		PD = new Instr_assign(_asgnusername); // GetPInstr(_asgnusername, 4);
		goto label2;
	}
	else if (IsKeyWord("CLIPBD"))
	{
		PD = new Instr_assign(_asgnclipbd); // GetPInstr(_asgnclipbd, 4);
		goto label2;
	}
	else if (IsKeyWord("ACCRIGHT")) {
		PD = new Instr_assign(_asgnaccright); // GetPInstr(_asgnaccright, 4);
	label2:
		Accept(_assign);
		PD->Frml = RdStrFrml();
	}
	else if (IsKeyWord("EDOK")) {
		PD = new Instr_assign(_asgnedok); // GetPInstr(_asgnedok, 4);
		Accept(_assign);
		PD->Frml = RdBool();
	}
	else if (IsKeyWord("RANDSEED"))
	{
		PD = new Instr_assign(_asgnrand); // GetPInstr(_asgnrand, 4);
		goto label3;
	}
	else if (IsKeyWord("TODAY"))
	{
		PD = new Instr_assign(_asgnusertoday); // GetPInstr(_asgnusertoday, 4);
		goto label3;
	}
	else if (IsKeyWord("USERCODE")) {
		PD = new Instr_assign(_asgnusercode); // GetPInstr(_asgnusercode, 4);
	label3:
		Accept(_assign);
		PD->Frml = RdRealFrml();
	}
	else {
		RdLex();
		if (Lexem == _assign) OldError(8);
		else OldError(34);
	}
	return PD;
}

Instr* RdWith()
{
	Instr* P = nullptr; Instr* p2 = nullptr; PInstrCode Op;
	if (IsKeyWord("WINDOW")) {
		P = new Instr_window(); //GetPInstr(_window, 29);
		auto iP = (Instr_window*)P;
		Accept('(');
		if (Lexem == '(') { RdLex(); iP->WithWFlags = WNoPop; }
		RdW(iP->W);
		RdFrame(&iP->Top, iP->WithWFlags);
		if (Lexem == ',') { RdLex(); iP->Attr = RdAttr(); }
		Accept(')');
		if ((iP->WithWFlags & WNoPop) != 0) Accept(')');
		AcceptKeyWord("DO");
		iP->WwInstr = RdPInstr();
	}
	else if (IsKeyWord("SHARED")) { Op = _withshared; goto label1; }
	else if (IsKeyWord("LOCKED")) {
		Op = _withlocked;
	label1:
		P = new Instr_withshared(Op); // GetPInstr(Op, 9 + sizeof(LockD));
		auto iP = (Instr_withshared*)P;
		LockD* ld = &iP->WLD;
	label2:
		ld->FD = RdFileName();
		if (Op == _withlocked) {
			Accept('[');
			ld->Frml = RdRealFrml();
			Accept(']');
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
			ld->Chain = new LockD(); // GetZStore(sizeof(LockD));
			ld = ld->Chain;
			goto label2;
		}
		AcceptKeyWord("DO");
		iP->WDoInstr = RdPInstr();
		if (IsKeyWord("ELSE")) {
			iP->WasElse = true;
			iP->WElseInstr = RdPInstr();
		}
	}
	else if (IsKeyWord("GRAPHICS")) {
		P = new Instr_withshared(_withgraphics); // GetPInstr(_withgraphics, 4);
		AcceptKeyWord("DO");
		((Instr_withshared*)P)->WDoInstr = RdPInstr();
	}
	else Error(131);
	return P;
}

Instr_assign* RdUserFuncAssign()
{
	Instr_assign* pd = nullptr;
	LocVar* lv = nullptr;
	if (!FindLocVar(&LVBD, &lv)) Error(34);
	RdLex();
	pd = new Instr_assign(_asgnloc); // GetPInstr(_asgnloc, 9);
	pd->AssLV = lv;
	RdAssignFrml(lv->FTyp, pd->Add, &pd->Frml);
	return pd;
}

Instr* RdPInstr()
{
	Instr* result = nullptr;
	if (IsKeyWord("IF")) result = RdIfThenElse();
	else if (IsKeyWord("WHILE")) result = RdWhileDo();
	else if (IsKeyWord("REPEAT")) result = RdRepeatUntil();
	else if (IsKeyWord("CASE")) result = RdCase();
	else if (IsKeyWord("FOR")) result = RdFor();
	else if (IsKeyWord("BEGIN")) result = RdBeginEnd();
	else if (IsKeyWord("BREAK")) result = new Instr(_break);
	else if (IsKeyWord("EXIT")) result = new Instr(_exitP);
	else if (IsKeyWord("CANCEL")) result = new Instr(_cancel);
	else if (Lexem == ';') result = nullptr;
	else if (IsRdUserFunc) result = RdUserFuncAssign();
	else if (IsKeyWord("MENULOOP")) result = RdMenuBox(true);
	else if (IsKeyWord("MENU")) result = RdMenuBox(false);
	else if (IsKeyWord("MENUBAR")) result = RdMenuBar();
	else if (IsKeyWord("WITH")) result = RdWith();
	else if (IsKeyWord("SAVE")) result = new Instr(_save);
	else if (IsKeyWord("CLREOL")) result = new Instr(_clreol);
	else if (IsKeyWord("FORALL")) result = RdForAll();
	else if (IsKeyWord("CLEARKEYBUF")) result = new Instr(_clearkeybuf);
	else if (IsKeyWord("WAIT")) result = new Instr(_wait);
	else if (IsKeyWord("BEEP")) result = new Instr(_beepP);
	else if (IsKeyWord("NOSOUND")) result = new Instr(_nosound);
#ifndef FandRunV
	else if (IsKeyWord("MEMDIAG")) RdPInstr = GetPInstr(_memdiag, 0);
#endif 
	else if (IsKeyWord("RESETCATALOG")) result = new Instr(_resetcat);
	else if (IsKeyWord("RANDOMIZE")) result = new Instr(_randomize);
	else if (Lexem == _identifier) {
		SkipBlank(false);
		if (ForwChar == '(') RdProcCall(&result); // funkce mùže ovlivnit result
		else if (IsKeyWord("CLRSCR")) result = new Instr(_clrscr);
		else if (IsKeyWord("GRAPH")) result = new Instr_graph();
		else if (IsKeyWord("CLOSE")) result = new Instr_closefds();
		else result = RdAssign();
	}
	else Error(34);
	return result;
}

void ReadProcHead()
{
	ResetCompilePars();
	RdFldNameFrml = RdFldNameFrmlP; RdFunction = RdFunctionP;
	FileVarsAllowed = false; IdxLocVarAllowed = true; IsRdUserFunc = false;
	RdLex();
	ResetLVBD();
	if (Lexem == '(') {
		RdLex();
		RdLocDcl(&LVBD, true, true, 'P');
		Accept(')');
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

// metoda nacita funkce a procedury z InpArrPtr a postupne je zpracovava
// nacte nazev, parametry, navr. hodnotu, promenne, konstanty i kód
void ReadDeclChpt()
{
	FuncD* fc = nullptr;
	char typ = '\0';
	WORD n = 0;
	LocVar* lv = nullptr;
	RdLex();
label1:
	if (IsKeyWord("FUNCTION")) {
		TestIdentif();
		fc = FuncDRoot;
		while (fc != CRdb->OldFCRoot) {
			if (EquUpcase(fc->Name, LexWord)) Error(26);
			fc = fc->Chain;
		}
		//fc = (FuncD*)GetStore(sizeof(FuncD) - 1 + LexWord.length());
		fc = new FuncD();
		fc->Chain = FuncDRoot;
		FuncDRoot = fc;
		//Move(&LexWord, &fc->Name, LexWord.length() + 1);
		fc->Name = LexWord;
		RdFldNameFrml = RdFldNameFrmlP;
		RdFunction = RdFunctionP;
		ChainSumEl = nullptr;
		FileVarsAllowed = false; IsRdUserFunc = true;
		RdLex();
		ResetLVBD();
		LVBD.FceName = fc->Name;
		Accept('(');
		if (Lexem != ')') RdLocDcl(&LVBD, true, false, 'D'); // nacte parametry funkce
		Accept(')');
		Accept(':');
		// nacte typ navratove hodnoty
		if (IsKeyWord("REAL")) {
			typ = 'R';
			n = sizeof(double);
		}
		else if (IsKeyWord("STRING")) {
			typ = 'S';
			n = sizeof(longint);
		}
		else if (IsKeyWord("BOOLEAN")) {
			typ = 'B';
			n = sizeof(bool);
		}
		else Error(39);
		//lv = (LocVar*)GetZStore(sizeof(*lv) - 1 + (fc->Name).length());
		lv = new LocVar();
		//if (LVBD.Root == nullptr) LVBD.Root = lv;
		//else ChainLast(LVBD.Root, lv);
		LVBD.vLocVar.push_back(lv);
		//Move(&fc->Name, &lv->Name, (fc->Name).length() + 1);
		lv->Name = fc->Name;
		/* !!! with lv^ do!!! */
		{ lv->FTyp = typ; lv->Op = _getlocvar; lv->BPOfs = LVBD.Size; }
		fc->FTyp = typ;
		LVBD.Size += n;
		Accept(';');
		// nacte promenne
		if (IsKeyWord("VAR")) RdLocDcl(&LVBD, false, false, 'D');
		fc->LVB = LVBD;
		// nacte kod funkce (procedury)
		AcceptKeyWord("BEGIN");
		fc->pInstr = RdBeginEnd();
		Accept(';');
	}
	else if (Lexem == 0x1A) return;
	else Error(40);
	goto label1;
}

FrmlElem* GetEvalFrml(FrmlElem21* X)
{
	stSaveState* p = nullptr;
	void* cr = nullptr;
	char fTyp;
	WORD cpos = 0;
	ExitRecord er = ExitRecord();
	ProcStkD* oldbp;

	LocVarBlkD oldLVBD = LVBD; oldbp = MyBP;
	SetMyBP(ProcMyBP);
	FrmlPtr z = nullptr;
	FileDPtr cf = CFile; cr = CRecPtr;
	LongStr* s = RunLongStr(X->EvalP1);
	if (s->LL == 0) { LastExitCode = 0; goto label2; }
	LastExitCode = 1;
	p = SaveCompState();
	ResetCompilePars();
	RdFldNameFrml = RdFldNameFrmlP;
	RdFunction = RdFunctionP;
	if (X->EvalFD == nullptr) FileVarsAllowed = false;
	else { CFile = X->EvalFD; FileVarsAllowed = true; }
	//NewExit(Ovr, er);
	//goto label1;
	SetInpLongStr(s, false);
	RdLex();
	z = RdFrml(fTyp);
	if ((fTyp != X->EvalTyp) || (Lexem != 0x1A)) z = nullptr;
	else LastExitCode = 0;
label1:
	cpos = CurrPos;
	RestoreExit(er);
	RestoreCompState(p);
	if (LastExitCode != 0) {
		LastTxtPos = cpos;
		if (X->EvalTyp == 'B')
		{
			z = new FrmlElem5(_const, 0, false); // GetOp(_const, 1);
			// z->B = false;
		}
	}
	if (z != nullptr) {
		FrmlPtr z1 = z;
		z = new FrmlElem0(_setmybp, 0); // GetOp(_setmybp, 0);
		((FrmlElem0*)z)->P1 = z1;
	}
label2:
	auto result = z;
	CFile = cf; CRecPtr = cr;
	SetMyBP(oldbp);
	LVBD = oldLVBD; /*for cond before cycle called when PushProcStk is !ready*/
	return result;
}

Instr* RdBackup(char MTyp, bool IsBackup)
{
	Instr_backup* PD = nullptr;
	if (MTyp == 'M') PD = new Instr_backup(_backupm); // GetPD(_backupm, 15);
	else PD = new Instr_backup(_backup); // GetPD(_backup, 5);
	RdLex();
	PD->IsBackup = IsBackup;
	TestIdentif();
	FileDPtr cf = CFile;
	void* cr = CRecPtr;
	CFile = CatFD;
	CRecPtr = GetRecSpace();
	for (WORD i = 1; i <= CatFD->NRecs; i++) {
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
	return PD;
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
Instr* RdCallLProc()
{
	auto pd = new Instr_lproc(); // GetPD(_lproc, sizeof(RdbPos) + 4);
	RdLex();
	RdChptName('L', &pd->lpPos, true);
	if (Lexem == ',') {
		RdLex();
		TestIdentif();
		pd->lpName = StoreStr(LexWord);
		RdLex();
	}
	return pd;
}
#endif

