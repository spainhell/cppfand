#include "rdproc.h"
#include "compile.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "legacy.h"
#include "oaccess.h"
#include "rdfildcl.h"
#include "rdrun.h"
#include "runfrml.h"
#include "../Common/exprcmp.h"
#include "../Common/compare.h"
#include "models/Instr.h"
#include "../fandio/XWKey.h"

bool IsRdUserFunc;
kNames KeyNames[NKeyNames] = {
	{"HOME", 51, VK_HOME},
	{"UP", 52, VK_UP},
	{"PGUP", 53, VK_NEXT},
	{"LEFT", 55, VK_LEFT},
	{"RIGHT", 57, VK_RIGHT},
	{"END", 59, VK_END},
	{"DOWN", 60, VK_DOWN},
	{"PGDN", 61, VK_NEXT},
	{"INS", 62, VK_INSERT},
	{"CTRLLEFT", 71, CTRL + VK_LEFT},
	{"CTRLRIGHT", 72, CTRL + VK_RIGHT},
	{"CTRLEND", 73, CTRL + VK_END},
	{"CTRLPGDN", 74, CTRL + VK_NEXT},
	{"CTRLHOME", 75, CTRL + VK_HOME},
	{"CTRLPGUP", 76, CTRL + VK_PRIOR},
	{"TAB", 77, VK_TAB},
	{"SHIFTTAB", 78, SHIFT + VK_TAB},
	{"CTRLN", 79, CTRL + 'N'},
	{"CTRLY", 80, CTRL + 'Y'},
	{"ESC", 81, VK_ESCAPE},
	{"CTRLP", 82, CTRL + 'P'} };

void TestCatError(int i, const std::string& name, bool old)
{
	if (i == 0) {
		SetMsgPar(name);
		if (old) {
			OldError(96);
		}
		else {
			Error(96);
		}
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
		auto Z = new FrmlElem7(_recvarfld, 12);
		FileD* cf = CFile;
		CFile = LV->FD;
		Z->File2 = CFile;
		Z->LD = (LinkD*)LV->RecPtr;
		bool fa = FileVarsAllowed;
		FileVarsAllowed = true;
		Z->P011 = RdFldNameFrmlF(FTyp, nullptr);
		FileVarsAllowed = fa;
		CFile = cf;
		return Z;
		break;
	}
	case 'i': {
		auto Z = new FrmlElem22(_indexnrecs, 4);
		Z->WKey = (XWKey*)LV->RecPtr;
		pstring nrecs = "nrecs";
		AcceptKeyWord(nrecs);
		FTyp = 'R';
		return Z;
		break;
	}
	default: OldError(177); break;
	}
	return nullptr;
}

char RdOwner(LinkD** LLD, LocVar** LLV)
{
	FileD* fd = nullptr;
	auto result = '\0';
	LocVar* lv = nullptr;
	std::string sLexWord;
	if (FindLocVar(&LVBD, &lv)) {
		if (!(lv->FTyp == 'i' || lv->FTyp == 'r' || lv->FTyp == 'f')) Error(177);
		LinkD* ld = nullptr;
		for (auto& ld1 : LinkDRoot) {
			if ((ld1->FromFD == CFile) && (ld1->IndexRoot != 0) && (ld1->ToFD == lv->FD)) {
				ld = ld1;
			}
		}
		if (ld == nullptr) {
			Error(116);
		}
		RdLex();
		if (lv->FTyp == 'f') {
#ifdef FandSQL
			if (ld->ToFD->typSQLFile) Error(155);
#endif
			Accept('[');
			*LLV = (LocVar*)RdRealFrml(nullptr);
			Accept(']');
			result = 'F';
			*LLD = ld;
			return result;
		}
		else {
			if (lv->FTyp == 'i') {
				KeyFldD* kf = ((XWKey*)lv->RecPtr)->KFlds;
				if (ld->FromFD->IsSQLFile || ld->ToFD->IsSQLFile) OldError(155);
				if ((kf != nullptr) && !KeyFldD::EquKFlds(kf, ld->ToKey->KFlds)) OldError(181);
			}
			*LLV = lv;
			*LLD = ld;
			result = lv->FTyp;
			return result;
		}
	}
	TestIdentif();
	for (auto& ld : LinkDRoot) {
		sLexWord = LexWord;
		if ((ld->FromFD == CFile) && EquUpCase(ld->RoleName, sLexWord)) {
			if ((ld->IndexRoot == 0)) Error(116);
			RdLex();
			fd = ld->ToFD;
			if (Lexem == '(') {
				RdLex();
				if (!FindLocVar(&LVBD, &lv) || !(lv->FTyp == 'i' || lv->FTyp == 'r')) Error(177);
				RdLex();
				Accept(')');
				if (lv->FD != fd) OldError(149);
				if (lv->FTyp == 'i') {
					KeyFldD* kf = ((XWKey*)lv->RecPtr)->KFlds;
					if (ld->FromFD->IsSQLFile || ld->ToFD->IsSQLFile) OldError(155);
					if ((kf != nullptr) && !KeyFldD::EquKFlds(kf, ld->ToKey->KFlds)) OldError(181);
				}
				*LLV = lv;
				*LLD = ld;
				result = lv->FTyp;
				return result;
			}
			else {
#ifdef FandSQL
				if (ld->ToFD->typSQLFile) Error(155);
#endif
				Accept('[');
				*LLV = (LocVar*)RdRealFrml(nullptr);
				Accept(']');
				result = 'F';
				*LLD = ld;
				return result;
			}
		}
		//ld = ld->pChain;
	}
	Error(9);
	return result;
}

FrmlElem* RdFldNameFrmlP(char& FTyp, MergeReportBase* caller)
{
	FileD* FD = nullptr; FrmlElem* Z = nullptr; LocVar* LV = nullptr;
	instr_type Op = _notdefined;
	LinkD* LD = nullptr; FieldDescr* F = nullptr;
	XKey* K = nullptr;

	FrmlElem* result = nullptr;

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
			if (FD != nullptr) {
				FName = FD->Name;
			}
			if (!linked) {
				RdLex();
			}
			RdLex();
			FTyp = 'R';
			if (IsKeyWord("LASTUPDATE")) {
				Op = _lastupdate;
				if (FD != nullptr) goto label2;
				F = nullptr;
				goto label1;
			}
			if (IsKeyWord("ARCHIVES")) {
				F = CatFD->CatalogArchiveField();
				goto label0;
			}
			if (IsKeyWord("PATH")) {
				F = CatFD->CatalogPathNameField();
				goto label0;
			}
			if (IsKeyWord("VOLUME")) {
				F = CatFD->CatalogVolumeField();
			label0:
				FTyp = 'S';
			label1:
				auto S = new FrmlElem10(_catfield, 6); // Z = GetOp(_catfield, 6);
				S->CatFld = F;
				S->CatIRec = CatFD->GetCatalogIRec(FName, true);
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
		A->PPPPP1 = RdRealFrml(nullptr);
		Accept(']');
		Accept('.');
		F = RdFldName(FD);
		A->RecFldD = F;
		FTyp = F->frml_type;
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
		Z = TryRdFldFrml(CFile, FTyp, nullptr);
		if (Z == nullptr) Error(8);
		result = Z;
		return result;
	}
	Error(8);
	return result;
}

//FileD* RdPath(bool NoFD, pstring** Path, WORD& CatIRec)
//{
//	FileD* fd = nullptr;
//	CatIRec = 0;
//	if (Lexem == _quotedstr) {
//		*Path = RdStrConst();
//		fd = nullptr;
//	}
//	else {
//		TestIdentif();
//		fd = FindFileD();
//		if (fd == nullptr) {
//			CatIRec = GetCatalogIRec(LexWord, true);
//			TestCatError(CatIRec, LexWord, false);
//		}
//		else if (NoFD) Error(97);
//		RdLex();
//	}
//	return fd;
//}

FileD* RdPath(bool NoFD, std::string& Path, WORD& CatIRec)
{
	//if (InpArrLen == 2781) {
	//	printf("");
	//}
	FileD* fd = nullptr;
	CatIRec = 0;
	if (Lexem == _quotedstr) {
		Path = RdStringConst();
		fd = nullptr;
	}
	else {
		TestIdentif();
		fd = FindFileD();
		if (fd == nullptr) {
			CatIRec = CatFD->GetCatalogIRec(LexWord, true);
			TestCatError(CatIRec, LexWord, false);
		}
		else if (NoFD) Error(97);
		RdLex();
	}
	return fd;
}

FrmlElem* RdFunctionP(char& FFTyp)
{
	/*if (InpArrLen == 0x257)	{
		printf("RdFunctionP");
	}*/

	FrmlElem* Z = nullptr;
	char Typ = '\0', FTyp = '\0';
	FileD* cf = nullptr;
	FileD* FD = nullptr;
	bool b = false;
	WORD N = 0;
	FrmlElem* Arg[30];
	instr_type Op = _notdefined;
	LocVar* LV = nullptr;
	LinkD* LD = nullptr;
	void* p = nullptr;
	//WORD* pofs = (WORD*)&p;

	if (IsKeyWord("EVALB")) {
		FTyp = 'B';
		goto label4;
	}
	else if (IsKeyWord("EVALS")) {
		FTyp = 'S';
		goto label4;
	}
	else if (IsKeyWord("EVALR")) {
		FTyp = 'R';
	label4:
		RdLex();
		Z = new FrmlElem21(_eval, 5); // GetOp(_eval, 5);
		((FrmlElem21*)Z)->EvalTyp = FTyp;
		((FrmlElem21*)Z)->EvalP1 = RdStrFrml(nullptr);
	}
	else if (FileVarsAllowed) Error(75);
	else if (IsKeyWord("PROMPT")) {
		RdLex();
		Z = new FrmlElem11(_prompt, 4); // GetOp(_prompt, 4);
		((FrmlElem11*)Z)->PPP1 = RdStrFrml(nullptr);
		FieldDescr* F = RdFieldDescr("", true);
		((FrmlElem11*)Z)->FldD = F; FTyp = F->frml_type;
		if (F->field_type == FieldType::TEXT) OldError(65);
		if (Lexem == _assign) {
			RdLex();
			((FrmlElem11*)Z)->PP2 = RdFrml(Typ, nullptr);
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
		XKey* K = RdViewKeyImpl(FD);
		if (Op == _recno) {
			KeyFldD* KF = K->KFlds;
			N = 0;
			if (KF == nullptr) OldError(176);
			while (KF != nullptr) {
				Accept(',');
				if (N > 29) Error(123);
				Arg[N] = RdFrml(Typ, nullptr);
				N++;
				if (Typ != KF->FldD->frml_type) OldError(12);
				KF = (KeyFldD*)KF->pChain;
			}
		}
		else {
			Accept(',');
			N = 1;
			Arg[0] = RdRealFrml(nullptr);
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
			iZ->LinkRecFrml = RdRealFrml(nullptr);
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
			((FrmlElem14*)Z)->PPPPP1 = RdRealFrml(nullptr);
		label2: {}
#ifdef FandSQL
			if (FD->typSQLFile) Error(155);
#endif
		}
	}
	else if (IsKeyWord("GETPATH")) {
		RdLex();
		Z = new FrmlElem0(_getpath, 0); // GetOp(_getpath, 0);
		((FrmlElem0*)Z)->P1 = RdStrFrml(nullptr);
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
		RdPath(true, iZ->TxtPath, iZ->TxtCatIRec);
		if ((Z->Op == _gettxt) && (Lexem == ',')) {
			RdLex();
			iZ->PPPPPP1 = RdRealFrml(nullptr);
			if (Lexem == ',') {
				RdLex();
				iZ->PPPP2 = RdRealFrml(nullptr);
			}
		}
	}
	else if (IsKeyWord("INTTSR")) {
		RdLex();
		Z = new FrmlElem0(_inttsr, 5); // GetOp(_inttsr, 5);
		auto iZ = (FrmlElem0*)Z;
		iZ->P1 = RdRealFrml(nullptr); Accept(',');
		iZ->P2 = RdRealFrml(nullptr); Accept(',');
		Typ = 'r';
		if (IsRecVar(&LV)) iZ->P3 = (FrmlElem*)LV->RecPtr;
		else iZ->P3 = RdFrml(Typ, nullptr);
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
		((FrmlElem0*)Z)->P1 = RdStrFrml(nullptr);
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
		iZ->P1 = RdRealFrml(nullptr); Accept(',');
		iZ->P2 = RdRealFrml(nullptr); Accept(',');
		iZ->P3 = RdRealFrml(nullptr); Accept(',');
		iZ->P4 = RdRealFrml(nullptr); FTyp = 'B';
	}
	else if (IsKeyWord("PORTIN")) {
		RdLex();
		Z = new FrmlElem0(_portin, 0); // GetOp(_portin, 0);
		auto iZ = (FrmlElem0*)Z;
		iZ->P1 = RdBool(nullptr); Accept(',');
		iZ->P2 = RdRealFrml(nullptr); FTyp = 'R';
	}
	else {
		Error(75);
	}
	Accept(')');
	FrmlElem* result = Z;
	FFTyp = FTyp;
	return result;
}

XKey* RdViewKeyImpl(FileD* FD)
{
	XKey* K = nullptr;
	if (FD != nullptr) K = FD->Keys.empty() ? nullptr : FD->Keys[0];
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
	Z->P1 = RdRealFrml(nullptr); Accept(',');
	Z->P2 = RdRealFrml(nullptr); Accept(',');
	Z->P3 = RdStrFrml(nullptr);
	while (Lexem == ',') {
		RdLex();
		if (IsOpt("HEAD")) Z->P4 = RdStrFrml(nullptr);
		else if (IsOpt("FOOT")) Z->P5 = RdStrFrml(nullptr);
		else if (IsOpt("MODE")) Z->P6 = RdStrFrml(nullptr);
		else if (IsOpt("DELIM")) Z->Delim = RdQuotedChar();
		else Error(157);
	}
}

void RdPInstrAndChain(Instr** PD)
{
	Instr* PD1 = RdPInstr(); /*may be a chain itself*/
	if (*PD != nullptr) {
		Instr* last = *PD;
		while (last->Chain != nullptr) {
			last = last->Chain;
		}
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

	while (true) {
		if (IsKeyWord("ESCAPE")) {
			Accept(':');
			PD->WasESCBranch = true;
			PD->ESCInstr = RdPInstr();
		}
		else {
			CD = new ChoiceD();
			PD->Choices.push_back(CD);

			N++;
			if ((PD->Kind == _menubar) && (N > 30)) Error(102);
			CD->TxtFrml = RdStrFrml(nullptr);
			if (Lexem == ',') {
				RdLex();
				if (Lexem != ',') {
					CD->HelpName = RdHelpName();
					PD->HelpRdb = CRdb;
				}
				if (Lexem == ',') {
					RdLex();
					if (Lexem != ',') {
						CD->Condition = RdBool(nullptr);
						if (Lexem == '!') {
							CD->DisplEver = true;
							RdLex();
						}
					}
				}
			}
			Accept(':');
			CD->Instr = RdPInstr();
		}
		if (Lexem == ';') {
			RdLex();
			if (IsKeyWord("END")) return;
			continue;
		}
		break;
	}

	AcceptKeyWord("END");
}

void RdMenuAttr(Instr_menu* PD)
{
	if (Lexem != ';') return;
	RdLex();
	PD->mAttr[0] = RdAttr(); Accept(',');
	PD->mAttr[1] = RdAttr(); Accept(',');
	PD->mAttr[2] = RdAttr();
	if (Lexem == ',') {
		RdLex();
		PD->mAttr[3] = RdAttr();
	}
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
			PD->X = RdRealFrml(nullptr);
			Accept(',');
			PD->Y = RdRealFrml(nullptr);
		}
		RdMenuAttr(PD);
		Accept(')');
	}
	if (Lexem == '!') { RdLex(); PD->Shdw = true; }
	if (IsKeyWord("PULLDOWN")) PD->PullDown = true;
	if (!TestKeyWord("OF")) PD->HdLine = RdStrFrml(nullptr);
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
			PD->Y = RdRealFrml(nullptr);
			if (Lexem == ',') {
				RdLex();
				PD->X = RdRealFrml(nullptr);	Accept(',');
				PD->XSz = RdRealFrml(nullptr);
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
	PD->Bool = RdBool(nullptr);
	AcceptKeyWord("THEN");
	PD->Instr1 = RdPInstr();
	if (IsKeyWord("ELSE")) PD->ElseInstr1 = RdPInstr();
	return result;
}

Instr_loops* RdWhileDo()
{
	auto PD = new Instr_loops(_whiledo); // GetPInstr(_whiledo, 8);
	auto result = PD;
	PD->Bool = RdBool(nullptr);
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
	PD->Frml = RdRealFrml(nullptr);

	AcceptKeyWord("TO");
	auto iLoop = new Instr_loops(_whiledo); // GetPInstr(_whiledo, 8);
	PD->Chain = iLoop;
	//PD = (Instr_assign*)PD->pChain;
	auto Z1 = new FrmlElem0(_compreal, 2); // GetOp(_compreal, 2);
	Z1->P1 = nullptr;
	Z1->LV1 = LV;
	Z1->N21 = _le;
	Z1->N22 = 5;
	Z1->P2 = RdRealFrml(nullptr);
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
	Instr_loops* PD1 = nullptr;
	bool first = true;
	Instr_loops* result = nullptr;
	while (true) {
		PD1 = new Instr_loops(_ifthenelseP); // GetPInstr(_ifthenelseP, 12);
		if (first) result = PD1;
		else PD->ElseInstr1 = PD1;
		PD = PD1;
		first = false;
		PD->Bool = RdBool(nullptr);
		Accept(':');
		PD->Instr1 = RdPInstr();
		bool b = Lexem == ';';
		if (b) RdLex();
		if (!IsKeyWord("END")) {
			if (IsKeyWord("ELSE")) {
				while (!IsKeyWord("END")) {
					RdPInstrAndChain(&PD->ElseInstr1);
					if (Lexem == ';') {
						RdLex();
					}
					else {
						AcceptKeyWord("END");
						break;
					}
				}
			}
			else if (b) {
				continue;
			}
			else {
				AcceptKeyWord("END");
			}
		}
		break;
	}
	return result;
}

Instr_loops* RdRepeatUntil()
{
	auto PD = new Instr_loops(_repeatuntil); // GetPInstr(_repeatuntil, 8);
	Instr_loops* result = PD;
	while (!IsKeyWord("UNTIL")) {
		RdPInstrAndChain(&PD->Instr1);
		if (Lexem == ';') RdLex();
		else {
			AcceptKeyWord("UNTIL");
			break;
		}
	}
	PD->Bool = RdBool(nullptr);
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
		AcceptKeyWord("SQL"); Accept('('); PD->CBool = RdStrFrml();
		Accept(')'); PD->inSQL = true; goto label2;
	}
#endif
	if (IsKeyWord("OWNER")) {
		PD->COwnerTyp = RdOwner(&PD->CLD, &PD->CLV);
		CViewKey = GetFromKey(PD->CLD);
	}
	else {
		CViewKey = RdViewKey();
	}
	if (Lexem == '(') {
		RdLex();
		PD->CBool = RdKeyInBool(&PD->CKIRoot, false, true, PD->CSQLFilter, nullptr);
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
		while (true) {
			RdPInstrAndChain(&PD);
			if (Lexem == ';') {
				RdLex();
				if (!IsKeyWord("END")) {
					continue;
				}
			}
			else {
				AcceptKeyWord("END");
			}
			break;
		}
	}
	return PD;
}

Instr_proc* RdProcArg(char Caller)
{
	std::string ProcName = LexWord;
	RdbPos Pos;
	TypAndFrml TArg[31];
	LocVar* LV = nullptr;
	if (Caller != 'C') {
		RdChptName('P', &Pos, Caller == 'P' || Caller == 'E' || Caller == 'T');
	}
	WORD N = 0;
	if (Caller != 'P') {
		if (Lexem == '(') {
			RdLex();
			goto label1;
		}
	}
	else if (Lexem == ',') {
		RdLex();
		Accept('(');
	label1:
		N++;
		if (N > 30) Error(123);
		TArg[N].Name = LexWord;
		if ((ForwChar != '.') && FindLocVar(&LVBD, &LV) && (LV->FTyp == 'i' || LV->FTyp == 'r')) {
			RdLex();
			TArg[N].FTyp = LV->FTyp;
			TArg[N].FD = LV->FD;
			TArg[N].RecPtr = LV->RecPtr;
		}
		else if (Lexem == '@') {
			RdLex();
			if (Lexem == '[') {
				RdLex();
				TArg[N].Name = LexWord;
				Accept(_identifier);
				Accept(',');
				auto z = new FrmlElem0(_setmybp, 0); // GetOp(_setmybp, 0);
				z->P1 = RdStrFrml(nullptr);
				TArg[N].TxtFrml = z;
				Accept(']');
			}
			else {
				TArg[N].FD = RdFileName();
			}
			TArg[N].FTyp = 'f';
		}
		else {
			TArg[N].Frml = RdFrml(TArg[N].FTyp, nullptr);
		}
		if (Lexem == ',') {
			RdLex();
			goto label1;
		}
		Accept(')');
	}
	if (Caller == 'E') {
		N++;
		TArg[N].FTyp = 'r';
	}
	auto* PD = new Instr_proc(N);
	PD->PPos = Pos;
	PD->N = N;
	PD->ProcName = ProcName;
	for (size_t i = 0; i < N; i++) {
		auto targ = TArg[i + 1];
		PD->TArg.push_back(targ); // do TArg ukladame od 1 - pozustatek
	}
	PD->ExPar = (Caller == 'E');
	return PD;
}

void SetCode(std::string keyName, BYTE fnNr, EdExKeyD* E)
{
	E->KeyCode = __F1 + fnNr - 1;
	if (keyName.empty()) {
		// pouze F1-F12
		//E->KeyCode = VIRTUAL + __F1 + fnNr - 1;
		E->Break = 20 + fnNr;
	}
	else if (keyName == "shift") {
		// Shift F1-F12
		E->KeyCode += SHIFT;
		E->Break = 0 + fnNr;
	}
	else if (keyName == "ctrl") {
		// Ctrl F1-F12
		E->KeyCode += CTRL;
		E->Break = 30 + fnNr;
	}
	else if (keyName == "alt") {
		// Alt F1-F12
		E->KeyCode += ALT;
		E->Break = 40 + fnNr;
	}
}

void RdKeyCode(EdExitD* X)
{
	WORD i = 0;
	X->Keys.push_back(EdExKeyD());
	EdExKeyD* lastKey = &X->Keys.back();

	std::string key; // tady bude "shift" | "ctrl" | "alt"
	BYTE fnNr; // tady bude cislo funkci klavesy

	lastKey->KeyName = LexWord;
	if (FindShiftCtrlAltFxx(LexWord, key, fnNr))
	{
		SetCode(key, fnNr, lastKey);
		RdLex();
	}
	else {
		for (i = 0; i < NKeyNames; i++) {
			if (EquUpCase(KeyNames[i].Nm, LexWord)) {
				lastKey->KeyCode = KeyNames[i].Code;
				lastKey->Break = KeyNames[i].Brk;
				RdLex();
				return;
			}
		}
		Error(129);
	}
}

bool RdHeadLast(EditOpt* EO)
{
	auto result = true;
	if (IsOpt("HEAD")) EO->Head = RdStrFrml(nullptr);
	else if (IsOpt("LAST")) EO->Last = RdStrFrml(nullptr);
	else if (IsOpt("CTRL")) EO->CtrlLast = RdStrFrml(nullptr);
	else if (IsOpt("ALT")) EO->AltLast = RdStrFrml(nullptr);
	else if (IsOpt("SHIFT")) EO->ShiftLast = RdStrFrml(nullptr);
	else result = false;
	return result;
}

bool RdHeadLast(Instr_edittxt* IE)
{
	auto result = true;
	if (IsOpt("HEAD")) IE->Head = RdStrFrml(nullptr);
	else if (IsOpt("LAST")) IE->Last = RdStrFrml(nullptr);
	else if (IsOpt("CTRL")) IE->CtrlLast = RdStrFrml(nullptr);
	else if (IsOpt("ALT")) IE->AltLast = RdStrFrml(nullptr);
	else if (IsOpt("SHIFT")) IE->ShiftLast = RdStrFrml(nullptr);
	else result = false;
	return result;
}

bool RdViewOpt(EditOpt* EO)
{
	FileD* FD = nullptr;
	RprtOpt* RO = nullptr;
	bool Flgs[23]{ false };
	auto result = false;
	RdLex();
	result = true;
	CViewKey = EO->ViewKey;
	if (IsOpt("TAB")) {
		RdNegFldList(EO->NegTab, EO->Tab);
	}
	else if (IsOpt("DUPL")) {
		RdNegFldList(EO->NegDupl, EO->Dupl);
	}
	else if (IsOpt("NOED")) {
		RdNegFldList(EO->NegNoEd, EO->NoEd);
	}
	else if (IsOpt("MODE")) {
		SkipBlank(false);
		if ((Lexem == _quotedstr) && (ForwChar == ',' || ForwChar == ')')) {
			EditModeToFlags(LexWord, Flgs, true);
			EO->Mode = new FrmlElem4(_const, 0); // GetOp(_const, LexWord.length() + 1);
			((FrmlElem4*)EO->Mode)->S = LexWord;
			RdLex();
		}
		else EO->Mode = RdStrFrml(nullptr);
	}
	else if (RdHeadLast(EO)) return result;
	else if (IsOpt("WATCH")) EO->WatchDelayZ = RdRealFrml(nullptr);
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
		EdExitD* X = new EdExitD();
		EO->ExD.push_back(X);

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
			EO->Cond = RdKeyInBool(&EO->KIRoot, false, true, EO->SQLFilter, nullptr);
			Accept(')');
		}
		else EO->Cond = RdKeyInBool(&EO->KIRoot, false, true, EO->SQLFilter, nullptr);
	}
	else if (IsOpt("JOURNAL")) {
		EO->Journal = RdFileName();
		WORD l = EO->Journal->FF->RecLen - 13;
		if (CFile->FF->file_type == FileType::INDEX) l++;
		if (CFile->FF->RecLen != l) OldError(111);
	}
	else if (IsOpt("SAVEAFTER")) EO->SaveAfterZ = RdRealFrml(nullptr);
	else if (IsOpt("REFRESH")) EO->RefreshDelayZ = RdRealFrml(nullptr);
	else result = false;
	return result;
}

void RdKeyList(EdExitD* X)
{
	while (true) {
		if ((Lexem == '(') || (Lexem == '^')) {
			RdNegFldList(X->NegFlds, &X->Flds);
		}
		else if (IsKeyWord("RECORD")) {
			X->AtWrRec = true;
		}
		else if (IsKeyWord("NEWREC")) {
			X->AtNewRec = true;
		}
		else {
			RdKeyCode(X);
		}
		if (Lexem == ',') {
			RdLex();
			continue;
		}
		break;
	}
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
	else if (IsKeyWord("WRITELN")) RdWriteln(WriteType::writeln, (Instr_writeln**)pinstr);
	else if (IsKeyWord("WRITE")) RdWriteln(WriteType::write, (Instr_writeln**)pinstr);
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
		((Instr_help*)*pinstr)->Frml0 = RdStrFrml(nullptr);
	}
	else if (IsKeyWord("MESSAGE")) RdWriteln(WriteType::message, (Instr_writeln**)pinstr);
	else if (IsKeyWord("GOTOXY")) *pinstr = RdGotoXY();
	else if (IsKeyWord("MERGE")) {
		// PD = (Instr_merge_display*)GetPD(_merge, sizeof(RdbPos));
		*pinstr = new Instr_merge_display(_merge);
		RdLex();
		RdbPos rp;
		RdChptName('M', &rp, true);
		((Instr_merge_display*)*pinstr)->Pos = rp;
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
		((Instr_assign*)*pinstr)->Frml = RdRealFrml(nullptr);
	}
	else if (IsKeyWord("LPROC")) *pinstr = RdCallLProc();

#ifdef FandGraph
	else if (IsKeyWord("GRAPH")) *pinstr = RdGraphP();
	else if (IsKeyWord("PUTPIXEL")) {
		*pinstr = new Instr_putpixel(_putpixel); // GetPD(_putpixel, 3 * 4);
		goto label3;
	}
	else if (IsKeyWord("LINE")) {
		*pinstr = new Instr_putpixel(_line); // GetPD(_line, 5 * 4);
		goto label3;
	}
	else if (IsKeyWord("RECTANGLE")) {
		*pinstr = new Instr_putpixel(_rectangle); // GetPD(_rectangle, 5 * 4);
		goto label3;
	}
	else if (IsKeyWord("ELLIPSE")) {
		*pinstr = new Instr_putpixel(_ellipse);  // GetPD(_ellipse, 7 * 4);
		goto label3;
	}
	else if (IsKeyWord("FLOODFILL")) {
		*pinstr = new Instr_putpixel(_floodfill); // GetPD(_floodfill, 5 * 4);
		goto label3;
	}
	else if (IsKeyWord("OUTTEXTXY")) {
		*pinstr = new Instr_putpixel(_outtextxy); // GetPD(_outtextxy, 11 * 4);
	label3:
		RdLex(); // read '('
		auto iPutPixel = (Instr_putpixel*)(*pinstr);
		iPutPixel->Par1 = RdRealFrml(nullptr);
		Accept(',');
		iPutPixel->Par2 = RdRealFrml(nullptr);
		Accept(',');
		if (iPutPixel->Kind == _outtextxy) {
			iPutPixel->Par3 = RdStrFrml(nullptr);
			Accept(',');
			iPutPixel->Par4 = RdRealFrml(nullptr);
			Accept(',');
			iPutPixel->Par5 = RdAttr();
			if (Lexem == ',') {
				RdLex();
				iPutPixel->Par6 = RdRealFrml(nullptr);
				if (Lexem == ',') {
					RdLex();
					iPutPixel->Par7 = RdRealFrml(nullptr);
					if (Lexem == ',') {
						RdLex();
						iPutPixel->Par8 = RdRealFrml(nullptr); Accept(',');
						iPutPixel->Par9 = RdRealFrml(nullptr); Accept(',');
						iPutPixel->Par10 = RdRealFrml(nullptr); Accept(',');
						iPutPixel->Par11 = RdRealFrml(nullptr);
					}
				}
			}
		}
		else if (iPutPixel->Kind == _putpixel) iPutPixel->Par3 = RdAttr();
		else {
			iPutPixel->Par3 = RdRealFrml(nullptr);
			Accept(',');
			if (iPutPixel->Kind == _floodfill) iPutPixel->Par4 = RdAttr();
			else iPutPixel->Par4 = RdRealFrml(nullptr);
			Accept(',');
			iPutPixel->Par5 = RdAttr();
			if ((iPutPixel->Kind == _ellipse) && (Lexem == ',')) {
				RdLex();
				iPutPixel->Par6 = RdRealFrml(nullptr);
				Accept(',');
				iPutPixel->Par7 = RdRealFrml(nullptr);
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
		((Instr_setmouse*)*pinstr)->MouseX = RdRealFrml(nullptr); Accept(',');
		((Instr_setmouse*)*pinstr)->MouseY = RdRealFrml(nullptr); Accept(',');
		((Instr_setmouse*)*pinstr)->Show = RdBool(nullptr);
	}
	else if (IsKeyWord("CHECKFILE")) {
		*pinstr = new Instr_checkfile(); // GetPD(_checkfile, 10);
		RdLex();
		auto iPD = (Instr_checkfile*)*pinstr;
		iPD->cfFD = RdFileName();
		/* !!! with PD->cfFD^ do!!! */
		if (iPD->cfFD != nullptr && (iPD->cfFD->FF->file_type == FileType::FAND8 || iPD->cfFD->FF->file_type == FileType::DBF)
#ifdef FandSQL
			|| PD->cfFD->typSQLFile
#endif
			) OldError(169);
		Accept(',');
		RdPath(true, iPD->cfPath, iPD->cfCatIRec);
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
			iPD->IsWord = RdBool(nullptr); Accept(',');
			iPD->Port = RdRealFrml(nullptr); Accept(',');
			iPD->PortWhat = RdRealFrml(nullptr);
		}
	}
	else Error(34);
	Accept(')');
}

std::vector<FieldDescr*> RdFlds()
{
	std::vector<FieldDescr*> FLRoot;
	FieldListEl* FL = nullptr;

	while (true) {
		auto fd = RdFldName(CFile);
		FLRoot.push_back(fd);
		if (Lexem == ',') {
			RdLex();
			continue;
		}
		break;
	}

	return FLRoot;
}

std::vector<FieldDescr*> RdSubFldList(std::vector<FieldDescr*>& InFL, char Opt)
{
	// TODO: this method is probably badly transformed -> check it!

	std::vector<FieldListEl*> FLRoot;
	std::vector<FieldDescr*> result;
	FieldListEl* FL = nullptr;
	FieldDescr* FL1 = nullptr;
	FieldDescr* F = nullptr;
	Accept('(');
label1:
	FL = new FieldListEl();
	FLRoot.push_back(FL);

	if (InFL.empty()) F = RdFldName(CFile);
	else {
		TestIdentif();
		//FL1 = InFL;
		//while (FL1 != nullptr) {
		for (auto& f : InFL) {
			std::string tmp = LexWord;
			FL1 = f;
			if (EquUpCase(f->Name, tmp)) goto label2;
			//FL1 = FL1->pChain;
		}
		Error(43);
	label2:
		F = FL1;
		RdLex();
	}
	FL->FldD = F;
	if ((Opt == 'S') && (F->frml_type != 'R')) OldError(20);
	if (Lexem == ',') { RdLex(); goto label1; }
	Accept(')');

	// transform to vector of FieldDescr*
	for (auto& fld : FLRoot) {
		result.push_back(fld->FldD);
	}
	return result;
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
	LocVar* lv = nullptr;
	Instr_edit* PD = new Instr_edit(); // GetPD(_edit, 8);
	RdLex();
	EditOpt* EO = &PD->EO;
	EO->UserSelFlds = true;

	if (IsRecVar(&lv)) {
		EO->LVRecPtr = lv->RecPtr;
		CFile = lv->FD;
	}
	else {
		CFile = RdFileName();
		XKey* K = RdViewKey();
		if (K == nullptr) K = CFile->Keys.empty() ? nullptr : CFile->Keys[0];
		EO->ViewKey = K;
	}
	PD->EditFD = CFile;
	Accept(',');
	if (IsOpt("U")) {
		TestIdentif();
		if (CFile->ViewNames == nullptr) Error(114);
		stSaveState* p = SaveCompState();
		bool b = RdUserView(LexWord, EO);
		RestoreCompState(p);
		if (!b) Error(114);
		RdLex();
	}
	else {
		RdBegViewDcl(EO);
	}
	while (Lexem == ',') {
		bool b = RdViewOpt(EO);
		if (!b) RdEditOpt(EO);
	}
	return PD;
}

void RdEditOpt(EditOpt* EO)
{
	/* !!! with EO^ do!!! */
	if (IsOpt("FIELD")) EO->StartFieldZ = RdStrFrml(nullptr);
	else if (EO->LVRecPtr != nullptr) Error(125);
	else if (IsOpt("OWNER")) {
		if (EO->SQLFilter || (EO->KIRoot != nullptr)) OldError(179);
		EO->OwnerTyp = RdOwner(&EO->DownLD, &EO->DownLV);
	}
	else if (IsOpt("RECKEY")) EO->StartRecKeyZ = RdStrFrml(nullptr);
	else if (
#ifdef FandSQL
		!CFile->typSQLFile &&
#endif
		IsOpt("RECNO")) EO->StartRecNoZ = RdRealFrml(nullptr);
	else if (IsOpt("IREC")) EO->StartIRecZ = RdRealFrml(nullptr);
	else if (IsKeyWord("CHECK")) EO->SyntxChk = true;
	else if (IsOpt("SEL")) {
		LocVar* lv = RdIdxVar();
		EO->SelKey = (XWKey*)lv->RecPtr;
		if ((EO->ViewKey == nullptr)) OldError(108);
		if (EO->ViewKey == EO->SelKey) OldError(184);
		if ((EO->ViewKey->KFlds != nullptr)
			&& (EO->SelKey->KFlds != nullptr)
			&& !KeyFldD::EquKFlds(EO->SelKey->KFlds, EO->ViewKey->KFlds)) OldError(178);
	}
	else Error(125);
}

Instr* RdReportCall()
{
	LocVar* lv = nullptr;
	RprtFDListEl* FDL = nullptr;
	Instr_report* PD = new Instr_report();
	RdLex();
	RprtOpt* RO = GetRprtOpt();
	PD->RO = RO;
	bool has_first = false;

	if (Lexem != ',') {
		has_first = true;
		FDL = &RO->FDL;
		bool b = false;
		if (Lexem == '(') {
			RdLex();
			b = true;
		}

		while (true) {
			if (IsRecVar(&lv)) {
				FDL->LVRecPtr = lv->RecPtr;
				FDL->FD = lv->FD;
			}
			else {
				CFile = RdFileName();
				FDL->FD = CFile;
				CViewKey = RdViewKey();
				FDL->ViewKey = CViewKey;
				if (Lexem == '(') {
					RdLex();
					FDL->Cond = RdKeyInBool(&FDL->KeyIn, true, true, FDL->SQLFilter, nullptr);
					Accept(')');
				}
			}
			if (b && (Lexem == ',')) {
				RdLex();
				FDL->Chain = new RprtFDListEl();
				FDL = FDL->Chain;
				continue;
			}
			break;
		}

		if (b) {
			Accept(')');
		}
		CFile = RO->FDL.FD;
		CViewKey = RO->FDL.ViewKey;
	}

	Accept(',');
	if (Lexem == '[') {
		RdLex();
		RO->RprtPos.R = (RdbD*)RdStrFrml(nullptr);
		RO->RprtPos.IRec = 0;
		RO->FromStr = true;
		Accept(']');
	}
	else if (!has_first || (Lexem == _identifier)) {
		TestIdentif();
		if (!FindChpt('R', LexWord, false, &RO->RprtPos)) {
			Error(37);
		}
		RdLex();
	}
	else {
		Accept('(');
		switch (Lexem) {
		case '?': {
			RO->Flds = AllFldsList(CFile, false);
			RdLex();
			RO->UserSelFlds = true;
			break;
		}
		case ')': {
			RO->Flds = AllFldsList(CFile, true);
			break;
		}
		default: {
			RO->Flds = RdFlds();
			if (Lexem == '?') {
				RdLex();
				RO->UserSelFlds = true;
			}
			break;
		}
		}
		Accept(')');
	}
	while (Lexem == ',') {
		RdLex();
		RdRprtOpt(RO, (has_first && (FDL->LVRecPtr == nullptr)));
	}
	if ((RO->Mode == _ALstg) && ((!RO->Ctrl.empty()) || (!RO->Sum.empty()))) {
		RO->Mode = _ARprt;
	}
	return PD;
}

void RdRprtOpt(RprtOpt* RO, bool has_first)
{
	FileD* FD = nullptr;
	WORD N = 0;

	if (IsOpt("ASSIGN")) {
		RdPath(true, RO->Path, RO->CatIRec);
	}
	else if (IsOpt("TIMES")) {
		RO->Times = RdRealFrml(nullptr);
	}
	else if (IsOpt("MODE")) {
		if (IsKeyWord("ONLYSUM")) {
			RO->Mode = _ATotal;
		}
		else if (IsKeyWord("ERRCHECK")) {
			RO->Mode = _AErrRecs;
		}
		else {
			Error(49);
		}
	}
	else if (IsKeyWord("COND")) {
		if (!has_first) {
			OldError(51);
			Accept('(');
			RdKFList(&RO->SK, CFile);
			Accept(')');
		}
		WORD Low = CurrPos;
		Accept(_equ);
		bool br = false;
		if (Lexem == '(') {
			Low = CurrPos;
			RdLex();
			br = true;
			if (Lexem == '?') {
				RdLex();
				RO->UserCondQuest = true;
				if (br) {
					Accept(')');
				}
				return;
			}
		}
		RO->FDL.Cond = RdKeyInBool(&RO->FDL.KeyIn, true, true, RO->FDL.SQLFilter, nullptr);
		N = OldErrPos - Low;
		RO->CondTxt = std::string((const char*)&InpArrPtr[Low], N);

		if (br) {
			Accept(')');
		}
	}
	else if (IsOpt("CTRL")) {
		if (!has_first) {
			OldError(51);
			Accept('(');
			RdKFList(&RO->SK, CFile);
			Accept(')');
		}
		RO->Ctrl = RdSubFldList(RO->Flds, 'C');
	}
	else if (IsOpt("SUM")) {
		if (!has_first) {
			OldError(51);
			Accept('(');
			RdKFList(&RO->SK, CFile);
			Accept(')');
		}
		RO->Sum = RdSubFldList(RO->Flds, 'S');
	}
	else if (IsOpt("WIDTH")) {
		RO->WidthFrml = RdRealFrml(nullptr);
	}
	else if (IsOpt("STYLE")) {
		if (IsKeyWord("COMPRESSED")) {
			RO->Style = 'C';
		}
		else {
			if (IsKeyWord("NORMAL")) {
				RO->Style = 'N';
			}
			else {
				Error(50);
			}
		}
	}
	else if (IsKeyWord("EDIT")) {
		RO->Edit = true;
	}
	else if (IsKeyWord("PRINTCTRL")) {
		RO->PrintCtrl = true;
	}
	else if (IsKeyWord("CHECK")) {
		RO->SyntxChk = true;
	}
	else if (IsOpt("SORT")) {
		if (!has_first) {
			OldError(51);
		}
		Accept('(');
		RdKFList(&RO->SK, CFile);
		Accept(')');
	}
	else if (IsOpt("HEAD")) {
		RO->Head = RdStrFrml(nullptr);
	}
	else {
		Error(45);
	}
}

Instr* RdRDBCall()
{
	std::string s;
	auto PD = new Instr_call(); // GetPD(_call, 12);
	RdLex();
	//s[0] = 0;
	if (Lexem == '\\') {
		s = "\\";
		RdLex();
	}
	TestIdentif();
	if (LexWord.length() > 8) Error(2);
	PD->RdbNm = s + std::string(LexWord);
	RdLex();
	if (Lexem == ',') {
		RdLex();
		TestIdentif();
		if (LexWord.length() > 12) Error(2);
		PD->ProcNm = LexWord;
		RdLex();
		PD->ProcCall = RdProcArg('C');
	}
	else {
		PD->ProcNm = "main";
	}
	return PD;
}

Instr* RdExec()
{
	auto PD = new Instr_exec(); // GetPD(_exec, 14);
	RdLex();
	RdPath(true, PD->ProgPath, PD->ProgCatIRec);
	Accept(',');
	PD->Param = RdStrFrml(nullptr);
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
	std::string ModeTxt[7] = { "KL","LK","KN","LN","LW","KW","WL" };
	WORD i = 0;
	CopyD* CD = nullptr;
	bool noapp = false;
	auto PD = new Instr_copyfile(); // GetPD(_copyfile, 4);
	RdLex();
	noapp = false;
	CD = new CopyD(); // (CopyD*)GetZStore(sizeof(*D));
	PD->CD = CD;
	/* !!! with D^ do!!! */
	CD->FD1 = RdPath(false, CD->Path1, CD->CatIRec1);
	CD->WithX1 = RdX(CD->FD1);
	if (Lexem == '/') {
		if (CD->FD1 != nullptr) { CFile = CD->FD1; CD->ViewKey = RdViewKey(); }
		else CD->Opt1 = RdCOpt();
	}
	Accept(',');
	CD->FD2 = RdPath(false, CD->Path2, CD->CatIRec2);
	CD->WithX2 = RdX(CD->FD2);
	if (Lexem == '/') {
		if (CD->FD2 != nullptr) Error(139);
		else CD->Opt2 = RdCOpt();
	}
	if (!TestFixVar(CD->Opt1, CD->FD1, CD->FD2) && !TestFixVar(CD->Opt2, CD->FD2, CD->FD1))
	{
		if ((CD->Opt1 == CpOption::cpTxt) && (CD->FD2 != nullptr)) OldError(139);
		noapp = (CD->FD1 == nullptr) ^ (CD->FD2 == nullptr); // XOR
#ifdef FandSQL
		if (noapp)
			if ((FD1 != nullptr) && (FD1->typSQLFile) || (FD2 != nullptr)
				&& (FD2->typSQLFile)) OldError(155);
#endif
	}
	while (Lexem == ',') {
		RdLex();
		if (IsOpt("HEAD")) {
			CD->HdFD = RdFileName();
			Accept('.');
			CD->HdF = RdFldName(CD->HdFD);
			if ((CD->HdF->frml_type != 'S') || !CD->HdFD->IsParFile
				|| (CD->Opt1 == CpOption::cpFix || CD->Opt1 == CpOption::cpVar)
				&& ((CD->HdF->Flg & f_Stored) == 0)) Error(52);
		}
		else if (IsOpt("MODE")) {
			TestLex(_quotedstr);
			for (i = 0; i < 7; i++) {
				if (EquUpCase(LexWord, ModeTxt[i])) {
					CD->Mode = i + 1;
					goto label1;
				}
			}
			Error(142);
		label1:
			RdLex();
		}
		else if (IsKeyWord("NOCANCEL")) CD->NoCancel = true;
		else if (IsKeyWord("APPEND")) {
			if (noapp) OldError(139); CD->Append = true;
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
		if (EquUpCase(OptArr[i], LexWord)) {
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
		AcceptKeyWord("X");
		if (FD->FF->file_type != FileType::INDEX) OldError(108);
		result = true;
	}
	return result;
}

bool TestFixVar(CpOption Opt, FileD* FD1, FileD* FD2)
{
	auto result = false;
	if ((Opt != CpOption::cpNo) && (FD1 != nullptr)) OldError(139);
	result = false;
	if (Opt == CpOption::cpFix || Opt == CpOption::cpVar) {
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
	else RdPath(true, PD->TxtPath, PD->TxtCatIRec);
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
	else RdPath(true, PD->TxtPath, PD->TxtCatIRec);
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
			if (IsOpt("TXTPOS")) PD->TxtPos = RdRealFrml(nullptr);
			else if (IsOpt("TXTXY")) PD->TxtXY = RdRealFrml(nullptr);
			else if (IsOpt("ERRMSG")) PD->ErrMsg = RdStrFrml(nullptr);
			else if (IsOpt("EXIT")) {
				Accept('(');
			label1:
				pX = new EdExitD(); // (EdExitD*)GetZStore(sizeof(*pX));
				PD->ExD.push_back(pX);
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
				if (RdHeadLast(PD)) {}
				else if (IsKeyWord("NOEDIT")) PD->EdTxtMode = 'V';
				else Error(161);
	}
	return PD;
}

Instr* RdPutTxt()
{
	auto PD = new Instr_puttxt(); // GetPD(_puttxt, 11);
	RdLex();
	RdPath(true, PD->TxtPath1, PD->TxtCatIRec1);
	Accept(',');
	PD->Txt = RdStrFrml(nullptr);
	if (Lexem == ',') {
		RdLex();
		AcceptKeyWord("APPEND");
		PD->App = true;
	}
	return PD;
}

Instr* RdTurnCat()
{
	Instr_turncat* PD = new Instr_turncat();
	RdLex();
	TestIdentif();
	PD->NextGenFD = FindFileD();
	const int first = CatFD->GetCatalogIRec(LexWord, true);
	TestCatError(first, LexWord, true);
	RdLex();
	PD->FrstCatIRec = first;
	const std::string rdb_name = CatFD->GetRdbName(first);
	const std::string file_name = CatFD->GetFileName(first);
	int i = first + 1;
	while (CatFD->GetCatalogFile()->FF->NRecs >= i
		&& EquUpCase(rdb_name, CatFD->GetRdbName(i))
		&& EquUpCase(file_name, CatFD->GetFileName(i))) {
		i++;
	}
	if (i == first + 1) {
		OldError(98);
	}
	PD->NCatIRecs = i - first;
	Accept(',');
	PD->TCFrml = RdRealFrml(nullptr);
	return PD;
}

void RdWriteln(WriteType OpKind, Instr_writeln** pinstr)
{
	WrLnD* d = new WrLnD();
	RdLex();
	FrmlElem* z = nullptr;
	//FillChar(&d, sizeof(d), 0); 
	WrLnD* w = d;
label1:
	/* !!! with w^ do!!! */
	w->Frml = RdFrml(w->Typ, nullptr);
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
		if ((OpKind == WriteType::message) && IsOpt("HELP")) z = RdStrFrml(nullptr);
		else {
			//w = (WrLnD*)GetZStore(sizeof(d));
			w = new WrLnD();
			if (d == nullptr) d = w;
			else ChainLast(d, w);
			goto label1;
		}
	}
	WORD N = 1 + sizeof(d);
	if (z != nullptr) { OpKind = WriteType::msgAndHelp; N += 8; }
	auto pd = new Instr_writeln(); // GetPInstr(_writeln, N);
	/* !!! with pd^ do!!! */
	pd->LF = OpKind;
	pd->WD = *d;
	if (OpKind == WriteType::msgAndHelp) {
		pd->mHlpRdb = CRdb;
		pd->mHlpFrml = z;
	}
	*pinstr = pd;
}

Instr* RdReleaseDrive()
{
	auto PD = new Instr_releasedrive(); // GetPD(_releasedrive, 4);
	RdLex();
	PD->Drive = RdStrFrml(nullptr);
	return PD;
}

Instr* RdIndexfile()
{
	auto PD = new Instr_indexfile(); // GetPD(_indexfile, 5);
	RdLex();
	PD->IndexFD = RdFileName();
	if (PD->IndexFD->FF->file_type != FileType::INDEX) OldError(108);
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
		PD->giCond = RdRealFrml(nullptr); /*RecNr*/
		return PD;
	}
	CFile = RdFileName();
	if (lv->FD != CFile) OldError(164);
	CViewKey = RdViewKey();
	PD->giKD = CViewKey;
	while (Lexem == ',') {
		RdLex();
		if (IsOpt("SORT")) {
			if (((XWKey*)lv->RecPtr)->KFlds != nullptr) OldError(175);
			Accept('(');
			RdKFList(&PD->giKFlds, CFile);
			Accept(')');
		}
		else if (IsOpt("COND")) {
			Accept('(');
			PD->giCond = RdKeyInBool(&PD->giKIRoot, false, true, PD->giSQLFilter, nullptr);
			Accept(')');
		}
		else if (IsOpt("OWNER")) {
			PD->giOwnerTyp = RdOwner(&PD->giLD, &PD->giLV2);
			XKey* k = GetFromKey(PD->giLD);
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
	PD->GoX = RdRealFrml(nullptr);
	Accept(',');
	PD->GoY = RdRealFrml(nullptr);
	return PD;
}

Instr* RdClrWw()
{
	auto PD = new Instr_clrww(); // GetPD(_clrww, 24);
	RdLex();
	RdW(PD->W2);
	if (Lexem == ',') {
		RdLex();
		if (Lexem != ',') PD->Attr2 = RdAttr();
		if (Lexem == ',') { RdLex(); PD->FillC = RdStrFrml(nullptr); }
	}
	return PD;
}

Instr* RdMount()
{
	auto PD = new Instr_mount(); // GetPD(_mount, 3);
	RdLex();
	int i = 0;
	TestIdentif();
	FileD* FD = FindFileD();
	if (FD == nullptr) {
		i = CatFD->GetCatalogIRec(LexWord, true);
	}
	else {
		i = FD->CatIRec;
	}
	TestCatError(i, LexWord, false);
	RdLex();
	PD->MountCatIRec = i;
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
		PD->Pos.R = (RdbD*)RdStrFrml(nullptr);
		PD->Pos.IRec = 0;
	}
	return PD;
}

Instr_graph* RdGraphP()
{
	FrmlElem* FrmlArr[15];
	WORD i;
	GraphVD* VD; GraphWD* WD; GraphRGBD* RGBD; WinG* Ww;

	pstring Nm1[11] = { "TYPE", "HEAD", "HEADX", "HEADY", "HEADZ", "FILL", "DIRX", "GRID", "PRINT", "PALETTE", "ASSIGN" };
	pstring Nm2[6] = { "WIDTH", "RECNO", "NRECS", "MAX", "MIN", "GRPOLY" };

	Instr_graph* PD = new Instr_graph();
	RdLex();
	PD->GD = new GraphD();

	auto PDGD = PD->GD;
	if (IsOpt("GF")) PDGD->GF = RdStrFrml(nullptr);
	else {
		PDGD->FD = RdFileName();
		CFile = PDGD->FD;
		CViewKey = RdViewKey();
		PDGD->ViewKey = CViewKey;
		Accept(',');
		Accept('(');
		PDGD->X = RdFldName(PDGD->FD);
		i = 0;
		do {
			Accept(',');
			PDGD->ZA[i] = RdFldName(PDGD->FD);
			i++;
		} while (!((i > 9) || (Lexem != ',')));
		Accept(')');
	}
	while (Lexem == ',') {
		RdLex();
		for (i = 0; i < 11; i++) if (IsOpt(Nm1[i])) {
			FrmlArr[0] = (FrmlElem*)(&PDGD->T);
			FrmlArr[i] = RdStrFrml(nullptr);
			goto label1;
		}
		for (i = 0; i < 6; i++) if (IsOpt(Nm2[i])) {
			FrmlArr[0] = (FrmlElem*)(&PDGD->S);
			FrmlArr[i] = RdRealFrml(nullptr);
			goto label1;
		}
		if (IsDigitOpt("HEADZ", i)) PDGD->HZA[i] = RdStrFrml(nullptr);
		else if (IsKeyWord("INTERACT")) PDGD->Interact = true;
		else if (IsOpt("COND")) {
			if (Lexem == '(')
			{
				RdLex();
				PDGD->Cond = RdKeyInBool(&PDGD->KeyIn, false, true, PDGD->SQLFilter, nullptr);
				Accept(')');
			}
			else PDGD->Cond = RdKeyInBool(&PDGD->KeyIn, false, true, PDGD->SQLFilter, nullptr);
		}
		else if (IsOpt("TXT")) {
			VD = new GraphVD();
			ChainLast(PDGD->V, VD);
			{
				/* !!! with VD^ do!!! */
				Accept('('); VD->XZ = RdRealFrml(nullptr); Accept(','); VD->YZ = RdRealFrml(nullptr); Accept(',');
				VD->Velikost = RdRealFrml(nullptr); Accept(','); VD->BarPis = RdStrFrml(nullptr); Accept(',');
				VD->Text = RdStrFrml(nullptr); Accept(')');
			}
		}
		else if (IsOpt("TXTWIN")) {
			WD = new GraphWD();
			ChainLast(PDGD->W, WD);
			{
				Accept('('); WD->XZ = RdRealFrml(nullptr); Accept(','); WD->YZ = RdRealFrml(nullptr); Accept(',');
				WD->XK = RdRealFrml(nullptr); Accept(','); WD->YK = RdRealFrml(nullptr); Accept(',');
				WD->BarPoz = RdStrFrml(nullptr); Accept(','); WD->BarPis = RdStrFrml(nullptr); Accept(',');
				WD->Text = RdStrFrml(nullptr); Accept(')');
			}
		}
		else if (IsOpt("RGB")) {
			RGBD = new GraphRGBD();
			ChainLast(PDGD->RGB, RGBD);
			{
				Accept('(');
				RGBD->Barva = RdStrFrml(nullptr);
				Accept(',');
				RGBD->R = RdRealFrml(nullptr);
				Accept(',');
				RGBD->G = RdRealFrml(nullptr);
				Accept(',');
				RGBD->B = RdRealFrml(nullptr);
				Accept(')');
			}
		}
		else if (IsOpt("WW")) {
			Ww = new WinG();
			Accept('(');
			if (Lexem == '(') { RdLex(); Ww->WFlags = WNoPop; }
			RdW(Ww->W);
			RdFrame(Ww->Top, Ww->WFlags);
			if (Lexem == ',') {
				RdLex(); Ww->ColBack = RdStrFrml(nullptr); Accept(',');
				Ww->ColFor = RdStrFrml(nullptr);
				Accept(','); Ww->ColFrame = RdStrFrml(nullptr);
			}
			Accept(')');
			if ((Ww->WFlags & WNoPop) != 0) Accept(')');
		}
		else Error(44);
	label1: {}
	}
	return PD;
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
			PD->RecNr = RdRealFrml(nullptr);
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
		XKey* K = RdViewKey();
		Accept(',');
#ifdef FandSQL
		if (CFile->typSQLFile
			&& (Lexem == _equ || Lexem == _le || Lexem == _gt || Lexem == _lt || Lexem == _ge))
		{
			PD->CompOp = Lexem; RdLex();
		}
#endif
		Z = RdFrml(FTyp, nullptr);
		PD->RecNr = Z;
		switch (FTyp) {
		case 'B': OldError(12); break;
		case 'S': {
			PD->ByKey = true;
			if (PD->CompOp == 0) PD->CompOp = _equ;
			if (K == nullptr) K = CFile->Keys.empty() ? nullptr : CFile->Keys[0];
			PD->Key = K;
			if ((K == nullptr) && (!CFile->IsParFile || (Z->Op != _const)
				|| (((FrmlElem4*)Z)->S.length() > 0))) OldError(24);
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
	if (IsOpt("OVERWR")) PD->Insert = RdBool(nullptr);
	else if (IsOpt("INDENT")) PD->Indent = RdBool(nullptr);
	else if (IsOpt("WRAP")) PD->Wrap = RdBool(nullptr);
	else if (IsOpt("ALIGN")) PD->Just = RdBool(nullptr);
	else if (IsOpt("COLBLK")) PD->ColBlk = RdBool(nullptr);
	else if (IsOpt("LEFT")) PD->Left = RdRealFrml(nullptr);
	else if (IsOpt("RIGHT")) PD->Right = RdRealFrml(nullptr);
	else Error(160);
	if (Lexem == ',') { RdLex(); goto label1; }
	return PD;
}

FrmlElem* AdjustComma(FrmlElem* Z1, FieldDescr* F, instr_type Op)
{
	FrmlElem0* Z = nullptr;
	FrmlElem2* Z2 = nullptr;
	auto result = Z1;
	if (F->field_type != FieldType::FIXED) return result;
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
	char FTyp;
	pstring S = LexWord;
	for (FieldDescr* F1 : FD1->FldD) {
		if ((F1->Flg & f_Stored) != 0) {
			LexWord = F1->Name;
			FieldDescr* F2 = FindFldName(FD2);
			if (F2 != nullptr) {
				AssignD* A = new AssignD();
				if (ARoot == nullptr) ARoot = A;
				else ChainLast(ARoot, A);
				if ((F2->frml_type != F1->frml_type)
					|| (F1->frml_type == 'R')
					&& (F1->field_type != F2->field_type)) {
					A->Kind = _zero;
					A->outputFldD = F1;
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
				if ((Lexem != _number) || (LexWord != "0")) Error(183);
				RdLex();
				PD = new Instr_assign(_asgnxnrecs); // GetPInstr(_asgnxnrecs, 4);
				PD->xnrIdx = (XWKey*)LV->RecPtr;
			}
			else {
				PD = new Instr_assign(_asgnrecfld); // GetPInstr(_asgnrecfld, 13);
				PD->AssLV = LV;
				F = RdFldName(LV->FD);
				PD->RecFldD = F;
				if ((F->Flg & f_Stored) == 0) OldError(14);
				FTyp = F->frml_type;
			label0:
				RdAssignFrml(FTyp, PD->Add, &PD->Frml, nullptr);
			}
		}
		else {
			FName = LexWord;
			FD = FindFileD();
			if (FD->IsActiveRdb()) Error(121);
			RdLex(); RdLex();
			if (IsKeyWord("ARCHIVES")) {
				F = CatFD->CatalogArchiveField();
				goto label1;
			}
			if (IsKeyWord("PATH")) {
				F = CatFD->CatalogPathNameField();
				goto label1;
			}
			if (IsKeyWord("VOLUME")) {
				F = CatFD->CatalogVolumeField();
			label1:
				PD = new Instr_assign(_asgncatfield);
				PD->FD3 = FD;
				PD->CatIRec = CatFD->GetCatalogIRec(FName, true);
				PD->CatFld = F;
				TestCatError(PD->CatIRec, FName, true);
				Accept(_assign);
				PD->Frml3 = RdStrFrml(nullptr);
			}
			else if (FD == nullptr) OldError(9);
			else if (IsKeyWord("NRECS")) {
				if (FD->FF->file_type == FileType::RDB) { OldError(127); }
				PD = new Instr_assign(_asgnnrecs);
				PD->FD = FD;
				FTyp = 'R';
				goto label0;
			}
			else {
				if (!FD->IsParFile) OldError(64);
				PD = new Instr_assign(_asgnpar); // GetPInstr(_asgnpar, 13);
				PD->FD = FD;
				F = RdFldName(FD);
				PD->FldD = F;
				if ((F->Flg & f_Stored) == 0) OldError(14);
				FTyp = F->frml_type;
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
		PD->RecFrml = RdRealFrml(nullptr);
		Accept(']');
		Accept('.');
		F = RdFldName(FD);
		PD->FldD = F;
		if ((F->Flg & f_Stored) == 0) OldError(14);
		PD->Indexarg = (FD->FF->file_type == FileType::INDEX) && IsKeyArg(F, FD);
		RdAssignFrml(F->frml_type, PD->Add, &PD->Frml, nullptr);
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
		PD->Frml = RdStrFrml(nullptr);
	}
	else if (IsKeyWord("EDOK")) {
		PD = new Instr_assign(_asgnedok); // GetPInstr(_asgnedok, 4);
		Accept(_assign);
		PD->Frml = RdBool(nullptr);
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
		PD->Frml = RdRealFrml(nullptr);
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
		if (Lexem == '(') {
			RdLex();
			iP->WithWFlags = WNoPop;
		}
		RdW(iP->W);
		RdFrame(&iP->Top, iP->WithWFlags);
		if (Lexem == ',') {
			RdLex();
			iP->Attr = RdAttr();
		}
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
			ld->Frml = RdRealFrml(nullptr);
			Accept(']');
		}
		else {
			Accept('(');
			for (LockMode i = NoExclMode; i <= ExclMode; i = (LockMode)(i + 1)) {
				if (IsKeyWord(LockModeTxt[i])) {
					ld->Md = i;
					goto label3;
				}
			}
			Error(100);
		label3:
			Accept(')');
		}
		if (Lexem == ',') {
			RdLex();
			ld->Chain = new LockD();
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
		P = new Instr_withshared(_withgraphics);
		AcceptKeyWord("DO");
		((Instr_withshared*)P)->WDoInstr = RdPInstr();
	}
	else {
		Error(131);
	}
	return P;
}

Instr_assign* RdUserFuncAssign()
{
	LocVar* lv = nullptr;
	if (!FindLocVar(&LVBD, &lv)) {
		Error(34);
	}
	RdLex();
	Instr_assign* pd = new Instr_assign(_asgnloc);
	pd->AssLV = lv;
	RdAssignFrml(lv->FTyp, pd->Add, &pd->Frml, nullptr);
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
	else if (IsKeyWord("MEMDIAG")) result = new Instr(_memdiag);
#endif 
	else if (IsKeyWord("RESETCATALOG")) result = new Instr(_resetcat);
	else if (IsKeyWord("RANDOMIZE")) result = new Instr(_randomize);
	else if (Lexem == _identifier) {
		SkipBlank(false);
		if (ForwChar == '(') RdProcCall(&result); // funkce muze ovlivnit RESULT
		else if (IsKeyWord("CLRSCR")) result = new Instr(_clrscr);
		else if (IsKeyWord("GRAPH")) result = new Instr_graph();
		else if (IsKeyWord("CLOSE")) result = new Instr_closefds();
		else result = RdAssign();
	}
	else Error(34);
	return result;
}

void ReadProcHead(const std::string& name)
{
	ResetCompilePars();
	ptrRdFldNameFrml = RdFldNameFrmlP;
	RdFunction = RdFunctionP;
	FileVarsAllowed = false;
	IdxLocVarAllowed = true;
	IsRdUserFunc = false;
	RdLex();
	ResetLVBD();
	LVBD.FceName = name;
	if (Lexem == '(') {
		RdLex();
		RdLocDcl(&LVBD, true, true, 'P');
		Accept(')');
	}
	if (IsKeyWord("VAR")) {
		RdLocDcl(&LVBD, false, true, 'P');
	}
}

Instr* ReadProcBody()
{
	AcceptKeyWord("BEGIN");
	Instr* result = RdBeginEnd();
	Accept(';');
	if (Lexem != 0x1A) {
		std::string error40 = Error(40);
		std::string err_msg = "ReadProcBody exception: " + error40;
		throw std::exception(err_msg.c_str());
	}
	return result;
}

// metoda nacita funkce a procedury z InpArrPtr a postupne je zpracovava
// nacte nazev, parametry, navr. hodnotu, promenne, konstanty i kod
void ReadDeclChpt()
{
	FuncD* fc = nullptr;
	char typ = '\0';
	WORD n = 0;
	LocVar* lv = nullptr;
	RdLex();
	while (true) {
		if (IsKeyWord("FUNCTION")) {
			TestIdentif();
			fc = FuncDRoot;
			while (fc != CRdb->OldFCRoot) {
				if (EquUpCase(fc->Name, LexWord)) Error(26);
				fc = fc->Chain;
			}
			//fc = (FuncD*)GetStore(sizeof(FuncD) - 1 + LexWord.length());
			fc = new FuncD();
			fc->Chain = FuncDRoot;
			FuncDRoot = fc;
			//Move(&LexWord, &fc->Name, LexWord.length() + 1);
			fc->Name = LexWord;
			ptrRdFldNameFrml = RdFldNameFrmlP;
			RdFunction = RdFunctionP;
			//ptrChainSumEl = nullptr;
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
				n = sizeof(int);
			}
			else if (IsKeyWord("BOOLEAN")) {
				typ = 'B';
				n = sizeof(bool);
			}
			else Error(39);
			lv = new LocVar();
			LVBD.vLocVar.push_back(lv);
			lv->Name = fc->Name;
			lv->IsRetValue = true;
			lv->FTyp = typ;
			lv->Op = _getlocvar;
			fc->FTyp = typ;
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
		else {
			Error(40);
			return;
		}
	}
}

FrmlElem* GetEvalFrml(FileD* file_d, FrmlElem21* X, void* record)
{
	stSaveState* p = nullptr;
	void* cr = nullptr;
	char fTyp;
	WORD cpos = 0;
	//ExitRecord er = ExitRecord();
	//ProcStkD* oldbp;

	LocVarBlkD oldLVBD = LVBD;
	//oldbp = MyBP;
	//SetMyBP(ProcMyBP);
	FrmlElem* z = nullptr;
	FileD* cf = CFile;
	cr = CRecPtr;
	auto s = RunStdStr(CFile, X->EvalP1, CRecPtr);
	if (s.empty()) {
		LastExitCode = 0;
		goto label2;
	}
	LastExitCode = 1;
	p = SaveCompState();
	ResetCompilePars();
	ptrRdFldNameFrml = RdFldNameFrmlP;
	RdFunction = RdFunctionP;
	if (X->EvalFD == nullptr) FileVarsAllowed = false;
	else {
		CFile = X->EvalFD;
		FileVarsAllowed = true;
	}
	//NewExit(Ovr, er);
	//goto label1;
	SetInpStdStr(s, false);
	RdLex();
	z = RdFrml(fTyp, nullptr);
	if ((fTyp != X->EvalTyp) || (Lexem != 0x1A)) z = nullptr;
	else LastExitCode = 0;
label1:
	cpos = CurrPos;
	//RestoreExit(er);
	RestoreCompState(p);
	if (LastExitCode != 0) {
		LastTxtPos = cpos;
		if (X->EvalTyp == 'B') {
			z = new FrmlElem5(_const, 0, false); // GetOp(_const, 1);
			// z->B = false;
		}
	}

label2:
	auto result = z;
	CFile = cf; CRecPtr = cr;
	//SetMyBP(oldbp);
	LVBD = oldLVBD; /*for cond before cycle called when PushProcStk is !ready*/
	return result;
}

Instr* RdBackup(char MTyp, bool IsBackup)
{
	Instr_backup* PD;
	if (MTyp == 'M') {
		PD = new Instr_backup(_backupm);
	}
	else {
		PD = new Instr_backup(_backup);
	}

	RdLex();
	PD->IsBackup = IsBackup;
	TestIdentif();

	bool found = false;
	for (int i = 1; i <= CatFD->GetCatalogFile()->FF->NRecs; i++) {
		if (EquUpCase(CatFD->GetRdbName(i), "ARCHIVES") && EquUpCase(CatFD->GetFileName(i), LexWord)) {
			RdLex();
			PD->BrCatIRec = i;
			found = true;
		}
	}

	if (!found) {
		Error(88);
		return nullptr;
	}
	else {
		if (MTyp == 'M') {
			Accept(',');
			PD->bmDir = RdStrFrml(nullptr);
			if (IsBackup) {
				Accept(',');
				PD->bmMasks = RdStrFrml(nullptr);
			}
		}
		while (Lexem == ',') {
			RdLex();
			if (MTyp == 'M') {
				if (!IsBackup && IsKeyWord("OVERWRITE")) {
					PD->bmOverwr = true;
					continue;
				}
				if (IsKeyWord("SUBDIR")) {
					PD->bmSubDir = true;
					continue;
				}
			}
			if (IsKeyWord("NOCOMPRESS"))
			{
				PD->NoCompress = true;
			}
			else {
				AcceptKeyWord("NOCANCEL");
				PD->BrNoCancel = true;
			}
		}
		return PD;
	}
}

#ifdef FandSQL
void RdSqlRdWrTxt(bool Rd)
{
	Instr* pd = GetPD(_sqlrdwrtxt, 23);
	/* !!! with pd^ do!!! */
	pd->IsRead = Rd; RdPath(true, pd->TxtPath, pd->TxtCatIRec);
	Accept(','); CFile = RdFileName(); pd->sqlFD = CFile;
	XKey* k = RdViewKey(); if (k == nullptr) k = CFile->Keys; pd->sqlKey = k; Accept(',');
	pd->sqlFldD = RdFldName(CFile); Accept(','); pd->sqlXStr = RdStrFrml();
	if (!pd->sqlFD->typSQLFile || (pd->sqlFldD->field_type != 'T')) OldError(170);
}
#endif

Instr* RdCallLProc()
{
	Instr_lproc* pd = new Instr_lproc();
	RdLex();
	RdChptName('L', &pd->lpPos, true);
	if (Lexem == ',') {
		RdLex();
		TestIdentif();
		pd->lpName = LexWord;
		RdLex();
	}
	return pd;
}
