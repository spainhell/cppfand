#include "rdproc.h"
#include "Compiler.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "rdfildcl.h"
#include "rdrun.h"
#include "runfrml.h"
#include "../Common/exprcmp.h"
#include "../Common/compare.h"
#include "models/Instr.h"
#include "../fandio/XWKey.h"
#include "../Drivers/constants.h"

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


void AddInstr(std::vector<Instr*>& dst, std::vector<Instr*> src)
{
	for (auto& instr : src) {
		dst.push_back(instr);
	}
}

void TestCatError(int i, const std::string& name, bool old)
{
	if (i == 0) {
		SetMsgPar(name);
		if (old) {
			g_compiler->OldError(96);
		}
		else {
			g_compiler->Error(96);
		}
	}
}

bool IsRecVar(LocVar** LV)
{
	if (!g_compiler->FindLocVar(&LVBD, LV) || ((*LV)->f_typ != 'r')) return false;
	g_compiler->RdLex();
	return true;
}

LocVar* RdRecVar()
{
	LocVar* LV = nullptr;
	if (!IsRecVar(&LV)) g_compiler->Error(141);
	return LV;
}

LocVar* RdIdxVar()
{
	LocVar* lv = nullptr;
	if (!g_compiler->FindLocVar(&LVBD, &lv) || (lv->f_typ != 'i')) g_compiler->Error(165);
	auto result = lv;
	g_compiler->RdLex();
	return result;
}

FrmlElem* RdRecVarFldFrml(LocVar* LV, char& FTyp)
{
	FrmlElem* Z = nullptr;
	g_compiler->Accept('.');

	switch (LV->f_typ) {
	case 'r': {
		FrmlElem7* fe7 = new FrmlElem7(_recvarfld, 12);
		FileD* previous = g_compiler->processing_F;
		g_compiler->processing_F = LV->FD;
		fe7->File2 = LV->FD;
		fe7->LD = (LinkD*)LV->record;
		bool fa = FileVarsAllowed;
		FileVarsAllowed = true;
		fe7->P011 = g_compiler->RdFldNameFrmlF(FTyp, nullptr);
		FileVarsAllowed = fa;
		Z = fe7;
		g_compiler->processing_F = previous;
		break;
	}
	case 'i': {
		FrmlElem22* fe22 = new FrmlElem22(_indexnrecs, 4);
		fe22->WKey = (XWKey*)LV->record;
		g_compiler->AcceptKeyWord("nrecs");
		FTyp = 'R';
		Z = fe22;
		break;
	}
	default: {
		g_compiler->OldError(177);
		break;
	}
	}

	return Z;
}

char RdOwner(FileD* file_d, LinkD** LLD, LocVar** LLV)
{
	FileD* fd = nullptr;
	auto result = '\0';
	LocVar* lv = nullptr;
	std::string sLexWord;
	if (g_compiler->FindLocVar(&LVBD, &lv)) {
		if (!(lv->f_typ == 'i' || lv->f_typ == 'r' || lv->f_typ == 'f')) {
			g_compiler->Error(177);
		}
		LinkD* ld = nullptr;
		for (LinkD* ld1 : LinkDRoot) {
			if ((ld1->FromFD == file_d) && (ld1->IndexRoot != 0) && (ld1->ToFD == lv->FD)) {
				ld = ld1;
			}
		}
		if (ld == nullptr) {
			g_compiler->Error(116);
		}
		g_compiler->RdLex();
		if (lv->f_typ == 'f') {
#ifdef FandSQL
			if (ld->ToFD->typSQLFile) Error(155);
#endif
			g_compiler->Accept('[');
			*LLV = (LocVar*)g_compiler->RdRealFrml(nullptr);
			g_compiler->Accept(']');
			result = 'F';
			*LLD = ld;
			return result;
		}
		else {
			if (lv->f_typ == 'i') {
				std::vector<KeyFldD*>* kf = &((XWKey*)lv->record)->KFlds;
				if (ld->FromFD->IsSQLFile || ld->ToFD->IsSQLFile) {
					g_compiler->OldError(155);
				}
				if (!kf->empty() && !KeyFldD::EquKFlds(*kf, ld->ToKey->KFlds)) {
					g_compiler->OldError(181);
				}
			}
			*LLV = lv;
			*LLD = ld;
			result = lv->f_typ;
			return result;
		}
	}
	g_compiler->TestIdentif();
	for (LinkD* ld : LinkDRoot) {
		sLexWord = LexWord;
		if ((ld->FromFD == CFile) && EquUpCase(ld->RoleName, sLexWord)) {
			if ((ld->IndexRoot == 0)) g_compiler->Error(116);
			g_compiler->RdLex();
			fd = ld->ToFD;
			if (Lexem == '(') {
				g_compiler->RdLex();
				if (!g_compiler->FindLocVar(&LVBD, &lv) || !(lv->f_typ == 'i' || lv->f_typ == 'r')) g_compiler->Error(177);
				g_compiler->RdLex();
				g_compiler->Accept(')');
				if (lv->FD != fd) g_compiler->OldError(149);
				if (lv->f_typ == 'i') {
					std::vector<KeyFldD*>* kf = &((XWKey*)lv->record)->KFlds;
					if (ld->FromFD->IsSQLFile || ld->ToFD->IsSQLFile) g_compiler->OldError(155);
					if (!kf->empty() && !KeyFldD::EquKFlds(*kf, ld->ToKey->KFlds)) g_compiler->OldError(181);
				}
				*LLV = lv;
				*LLD = ld;
				result = lv->f_typ;
				return result;
			}
			else {
#ifdef FandSQL
				if (ld->ToFD->typSQLFile) Error(155);
#endif
				g_compiler->Accept('[');
				*LLV = (LocVar*)g_compiler->RdRealFrml(nullptr);
				g_compiler->Accept(']');
				result = 'F';
				*LLD = ld;
				return result;
			}
		}
		//ld = ld->pChain;
	}
	g_compiler->Error(9);
	return result;
}

FrmlElem* RdFldNameFrmlP(char& FTyp, MergeReportBase* caller)
{
	FileD* FD = nullptr; FrmlElem* Z = nullptr; LocVar* LV = nullptr;
	instr_type Op = _notdefined;
	LinkD* LD = nullptr; FieldDescr* F = nullptr;
	XKey* K = nullptr;

	FrmlElem* result = nullptr;

	if (g_compiler->IsForwPoint())
		if (g_compiler->FindLocVar(&LVBD, &LV) && (LV->f_typ == 'i' || LV->f_typ == 'r')) {
			g_compiler->RdLex();
			result = RdRecVarFldFrml(LV, FTyp);
			return result;
		}
		else {
			pstring FName = LexWord;
			bool linked = g_compiler->IsRoleName(FileVarsAllowed, g_compiler->processing_F, &FD, &LD);
			if (FD != nullptr) {
				FName = FD->Name;
			}
			if (!linked) {
				g_compiler->RdLex();
			}
			g_compiler->RdLex();
			FTyp = 'R';
			if (g_compiler->IsKeyWord("LASTUPDATE")) {
				Op = _lastupdate;
				if (FD != nullptr) goto label2;
				F = nullptr;
				goto label1;
			}
			if (g_compiler->IsKeyWord("ARCHIVES")) {
				F = catalog->CatalogArchiveField();
				goto label0;
			}
			if (g_compiler->IsKeyWord("PATH")) {
				F = catalog->CatalogPathNameField();
				goto label0;
			}
			if (g_compiler->IsKeyWord("VOLUME")) {
				F = catalog->CatalogVolumeField();
			label0:
				FTyp = 'S';
			label1:
				auto S = new FrmlElemCatalogField(_catfield, 6); // Z = GetOp(_catfield, 6);
				S->CatFld = F;
				S->CatIRec = catalog->GetCatalogIRec(FName, true);
				TestCatError(S->CatIRec, FName, true);
				return S;
			}
			if (FD != nullptr) {
				if (g_compiler->IsKeyWord("GENERATION")) { Op = _generation; goto label2; }
				if (g_compiler->IsKeyWord("NRECSABS")) { Op = _nrecsabs; goto label2; }
				if (g_compiler->IsKeyWord("NRECS")) {
					Op = _nrecs;
				label2:
					auto N = new FrmlElem9(Op, 0); // Z = GetOp(oper, sizeof(FileDPtr));
					N->FD = FD;
					return N;
				}
			}
			if (linked) { result = g_compiler->RdFAccess(FD, LD, FTyp); return result; }
			if (FileVarsAllowed) g_compiler->OldError(9);
			else g_compiler->OldError(63);
		}
	if (ForwChar == '[') {
		auto A = new FrmlElem14(_accrecno, 8); // Z = GetOp(_accrecno, 8);
		FD = g_compiler->RdFileName();
		g_compiler->RdLex();
		A->RecFD = FD;
#ifdef FandSQL
		if (v_files->typSQLFile) OldError(155);
#endif
		A->P1 = g_compiler->RdRealFrml(nullptr);
		g_compiler->Accept(']');
		g_compiler->Accept('.');
		F = g_compiler->RdFldName(FD);
		A->RecFldD = F;
		FTyp = F->frml_type;
		return A;
	}
	if (g_compiler->IsKeyWord("KEYPRESSED")) { Op = _keypressed; goto label3; }
	if (g_compiler->IsKeyWord("ESCPROMPT")) { Op = _escprompt; goto label3; }
	if (g_compiler->IsKeyWord("EDUPDATED")) {
		Op = _edupdated;
	label3:
		result = new FrmlElemFunction(Op, 0); // GetOp(oper, 0);
		FTyp = 'B';
		return result;
	}
	if (g_compiler->IsKeyWord("GETPATH")) {
		result = new FrmlElemFunction(_getpath, 0); // GetOp(_getpath, 0);
		FTyp = 'S';
		return result;
	}
	if (g_compiler->FindLocVar(&LVBD, &LV)) {
		if (LV->f_typ == 'r' || LV->f_typ == 'f' || LV->f_typ == 'i') g_compiler->Error(143);
		g_compiler->RdLex();
		result = new FrmlElemLocVar(LV->oper, LV);
		//((FrmlElemLocVar*)result)->BPOfs = LV->BPOfs;
		FTyp = LV->f_typ;
		return result;
	}
	if (FileVarsAllowed) {
		Z = g_compiler->TryRdFldFrml(g_compiler->processing_F, FTyp, nullptr);
		if (Z == nullptr) g_compiler->Error(8);
		result = Z;
		return result;
	}
	g_compiler->Error(8);
	return result;
}

FileD* RdPath(bool NoFD, std::string& Path, WORD& CatIRec)
{
	FileD* fd = nullptr;
	CatIRec = 0;
	if (Lexem == _quotedstr) {
		Path = g_compiler->RdStringConst();
		fd = nullptr;
	}
	else {
		g_compiler->TestIdentif();
		fd = g_compiler->FindFileD();
		if (fd == nullptr) {
			CatIRec = catalog->GetCatalogIRec(LexWord, true);
			TestCatError(CatIRec, LexWord, false);
		}
		else if (NoFD) g_compiler->Error(97);
		g_compiler->RdLex();
	}
	return fd;
}

FrmlElemRecNo* RdKeyOfOrRecNo(instr_type Op, WORD& N, FrmlElem* Arg[30], char& Typ, char FTyp)
{
	FileD* FD = g_compiler->RdFileName();
	XKey* K = RdViewKeyImpl(FD);
	if (Op == _recno) {
		//KeyFldD* KF = K->KFlds;
		N = 0;
		if (K->KFlds.empty()) {
			g_compiler->OldError(176);
		}
		//while (KF != nullptr) {
		for (KeyFldD* KF : K->KFlds) {
			g_compiler->Accept(',');
			if (N > 29) {
				g_compiler->Error(123);
			}
			Arg[N] = g_compiler->RdFrml(Typ, nullptr);
			N++;
			if (Typ != KF->FldD->frml_type) {
				g_compiler->OldError(12);
			}
			//KF = KF->pChain;
		}
	}
	else {
		g_compiler->Accept(',');
		N = 1;
		Arg[0] = g_compiler->RdRealFrml(nullptr);
	}
	FrmlElemRecNo* Z = new FrmlElemRecNo(Op, (N + 2) * 4);
	Z->FFD = FD;
	Z->Key = K;
	Z->SaveArgs(Arg, N);
	if (FTyp == 'R') {
#ifdef FandSQL
		if (v_files->typSQLFile) Error(155);
#endif
	}
	return Z;
}

FrmlElem* RdFunctionP(char& FFTyp)
{
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

	if (g_compiler->IsKeyWord("EVALB")) {
		FTyp = 'B';
		goto label4;
	}
	else if (g_compiler->IsKeyWord("EVALS")) {
		FTyp = 'S';
		goto label4;
	}
	else if (g_compiler->IsKeyWord("EVALR")) {
		FTyp = 'R';
	label4:
		g_compiler->RdLex();
		Z = new FrmlElem21(_eval, 5);
		((FrmlElem21*)Z)->EvalTyp = FTyp;
		((FrmlElem21*)Z)->EvalP1 = g_compiler->RdStrFrml(nullptr);
	}
	else if (FileVarsAllowed) {
		g_compiler->Error(75);
	}
	else if (g_compiler->IsKeyWord("PROMPT")) {
		g_compiler->RdLex();
		Z = new FrmlElem11(_prompt, 4);
		((FrmlElem11*)Z)->P1 = g_compiler->RdStrFrml(nullptr);
		FieldDescr* F = RdFieldDescr("", true);
		((FrmlElem11*)Z)->FldD = F;
		FTyp = F->frml_type;
		if (F->field_type == FieldType::TEXT) g_compiler->OldError(65);
		if (Lexem == _assign) {
			g_compiler->RdLex();
			((FrmlElem11*)Z)->P2 = g_compiler->RdFrml(Typ, nullptr);
			if (Typ != FTyp) g_compiler->OldError(12);
		}
	}
	else if (g_compiler->IsKeyWord("KEYOF")) {
		g_compiler->RdLex();
		FTyp = 'S';
		if (!IsRecVar(&LV)) {
			Op = _recno;
			Z = RdKeyOfOrRecNo(Op, N, Arg, Typ, FTyp);
		}
		else {
			Z = new FrmlElem20(_keyof, 8);
			((FrmlElem20*)Z)->LV = LV;
			((FrmlElem20*)Z)->PackKey = RdViewKeyImpl(((FrmlElem20*)Z)->LV->FD);
			FTyp = 'S';
		}
	}
	else if (g_compiler->IsKeyWord("RECNO")) {
		Op = _recno;
		g_compiler->RdLex();
		FTyp = 'R';
		Z = RdKeyOfOrRecNo(Op, N, Arg, Typ, FTyp);
	}
	else if (g_compiler->IsKeyWord("RECNOABS")) {
		Op = _recnoabs;
		g_compiler->RdLex();
		FTyp = 'R';
		Z = RdKeyOfOrRecNo(Op, N, Arg, Typ, FTyp);
	}
	else if (g_compiler->IsKeyWord("RECNOLOG")) {
		Op = _recnolog;
		g_compiler->RdLex();
		FTyp = 'R';
		Z = RdKeyOfOrRecNo(Op, N, Arg, Typ, FTyp);
	}
	else if (g_compiler->IsKeyWord("LINK")) {
		g_compiler->RdLex();
		Z = new FrmlElemLink(_link, 5); // GetOp(_link, 5);
		auto iZ = (FrmlElemLink*)Z;
		if (IsRecVar(&LV)) {
			iZ->LinkFromRec = true;
			iZ->LinkLV = LV;
			FD = LV->FD;
		}
		else {
			FD = g_compiler->RdFileName();
			g_compiler->Accept('[');
			iZ->LinkRecFrml = g_compiler->RdRealFrml(nullptr);
			g_compiler->Accept(']');
		}
		g_compiler->Accept(',');
#ifdef FandSQL
		if (v_files->typSQLFile) OldError(155);
#endif
		cf = CFile;
		CFile = FD;
		if (!g_compiler->IsRoleName(true, FD, &FD, &LD) || (LD == nullptr)) {
			g_compiler->Error(9);
		}
		CFile = cf;
		iZ->LinkLD = LD;
		FTyp = 'R';
#ifdef FandSQL
		if (v_files->typSQLFile) Error(155);
#endif
	}
	else if (g_compiler->IsKeyWord("ISDELETED")) {
		g_compiler->RdLex();
		FTyp = 'B';
		if (IsRecVar(&LV)) {
			Z = new FrmlElem20(_lvdeleted, 4); // GetOp(_lvdeleted, 4);
			((FrmlElem20*)Z)->LV = LV;
		}
		else {
			Z = new FrmlElem14(_isdeleted, 4); // GetOp(_isdeleted, 4);
			FD = g_compiler->RdFileName();
			((FrmlElem14*)Z)->RecFD = FD;
			g_compiler->Accept(',');
			((FrmlElem14*)Z)->P1 = g_compiler->RdRealFrml(nullptr);
			//label2: {}
#ifdef FandSQL
			if (v_files->typSQLFile) Error(155);
#endif
		}
	}
	else if (g_compiler->IsKeyWord("GETPATH")) {
		g_compiler->RdLex();
		Z = new FrmlElemFunction(_getpath, 0); // GetOp(_getpath, 0);
		((FrmlElemFunction*)Z)->P1 = g_compiler->RdStrFrml(nullptr);
		FTyp = 'S';
	}
	else if (g_compiler->IsKeyWord("GETTXT")) {
		g_compiler->RdLex();
		Z = new FrmlElem16(_gettxt, 6); // GetOp(_gettxt, 6);
		FTyp = 'S';
		goto label3;
	}
	else if (g_compiler->IsKeyWord("FILESIZE")) {
		g_compiler->RdLex();
		Z = new FrmlElem16(_filesize, 14); // GetOp(_filesize, 14);
		FTyp = 'R';
	label3:
		auto iZ = (FrmlElem16*)Z;
		RdPath(true, iZ->TxtPath, iZ->TxtCatIRec);
		if ((Z->Op == _gettxt) && (Lexem == ',')) {
			g_compiler->RdLex();
			iZ->P1 = g_compiler->RdRealFrml(nullptr);
			if (Lexem == ',') {
				g_compiler->RdLex();
				iZ->P2 = g_compiler->RdRealFrml(nullptr);
			}
		}
	}
	else if (g_compiler->IsKeyWord("INTTSR")) {
		g_compiler->RdLex();
		Z = new FrmlElemFunction(_inttsr, 5); // GetOp(_inttsr, 5);
		auto iZ = (FrmlElemFunction*)Z;
		iZ->P1 = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
		iZ->P2 = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
		Typ = 'r';
		if (IsRecVar(&LV)) iZ->P3 = (FrmlElem*)LV->record;
		else iZ->P3 = g_compiler->RdFrml(Typ, nullptr);
		iZ->N31 = Typ;
		FTyp = 'R';
	}
#ifdef FandSQL
	else if (IsKeyWord("SQL")) {
		RdLex(); Z = GetOp(_sqlfun, 0); Z->frml_elem = RdStrFrml(); f_typ = 'rdb';
	}
#endif
	else if (g_compiler->IsKeyWord("SELECTSTR")) {
		g_compiler->RdLex();
		Z = new FrmlElemFunction(_selectstr, 13); // GetOp(_selectstr, 13);
		FTyp = 'S';
		RdSelectStr((FrmlElemFunction*)Z);
	}
	else if (g_compiler->IsKeyWord("PROMPTYN")) {
		g_compiler->RdLex();
		Z = new FrmlElemFunction(_promptyn, 0); // GetOp(_promptyn, 0);
		((FrmlElemFunction*)Z)->P1 = g_compiler->RdStrFrml(nullptr);
		FTyp = 'B';
	}
	else if (g_compiler->IsKeyWord("MOUSEEVENT")) {
		g_compiler->RdLex();
		Z = new FrmlElem1(_mouseevent, 2); // GetOp(_mouseevent, 2);
		((FrmlElem1*)Z)->W01 = g_compiler->RdInteger();
		FTyp = 'B';
	}
	else if (g_compiler->IsKeyWord("ISMOUSE")) {
		g_compiler->RdLex();
		Z = new FrmlElem1(_ismouse, 4); // GetOp(_ismouse, 4);
		((FrmlElem1*)Z)->W01 = g_compiler->RdInteger(); g_compiler->Accept(',');
		((FrmlElem1*)Z)->W02 = g_compiler->RdInteger(); FTyp = 'B';
	}
	else if (g_compiler->IsKeyWord("MOUSEIN")) {
		g_compiler->RdLex();
		Z = new FrmlElemFunction(_mousein, 4); // GetOp(_mousein, 4);
		auto iZ = (FrmlElemFunction*)Z;
		iZ->P1 = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
		iZ->P2 = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
		iZ->P3 = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
		iZ->P4 = g_compiler->RdRealFrml(nullptr);
		FTyp = 'B';
	}
	else if (g_compiler->IsKeyWord("PORTIN")) {
		g_compiler->RdLex();
		Z = new FrmlElemFunction(_portin, 0); // GetOp(_portin, 0);
		auto iZ = (FrmlElemFunction*)Z;
		iZ->P1 = g_compiler->RdBool(nullptr);
		g_compiler->Accept(',');
		iZ->P2 = g_compiler->RdRealFrml(nullptr);
		FTyp = 'R';
	}
	else {
		g_compiler->Error(75);
	}
	g_compiler->Accept(')');
	FrmlElem* result = Z;
	FFTyp = FTyp;
	return result;
}

XKey* RdViewKeyImpl(FileD* FD)
{
	XKey* K = nullptr;
	if (FD != nullptr) K = FD->Keys.empty() ? nullptr : FD->Keys[0];
	if (K == nullptr) g_compiler->Error(24);
	if (Lexem == '/') {
		K = g_compiler->RdViewKey(FD);
	}
	return K;
}

void RdSelectStr(FrmlElemFunction* Z)
{
	Z->Delim = 0x0D; // CTRL+M
	Z->P1 = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
	Z->P2 = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
	Z->P3 = g_compiler->RdStrFrml(nullptr);
	while (Lexem == ',') {
		g_compiler->RdLex();
		if (g_compiler->IsOpt("HEAD")) Z->P4 = g_compiler->RdStrFrml(nullptr);
		else if (g_compiler->IsOpt("FOOT")) Z->P5 = g_compiler->RdStrFrml(nullptr);
		else if (g_compiler->IsOpt("MODE")) Z->P6 = g_compiler->RdStrFrml(nullptr);
		else if (g_compiler->IsOpt("DELIM")) Z->Delim = g_compiler->RdQuotedChar();
		else g_compiler->Error(157);
	}
}

void RdChoices(Instr_menu* PD)
{
	ChoiceD* CD = nullptr;
	WORD N = 0, SumL = 0;
	g_compiler->AcceptKeyWord("OF");

	while (true) {
		if (g_compiler->IsKeyWord("ESCAPE")) {
			g_compiler->Accept(':');
			PD->WasESCBranch = true;
			AddInstr(PD->ESCInstr, RdPInstr());
		}
		else {
			CD = new ChoiceD();
			PD->Choices.push_back(CD);

			N++;
			if ((PD->Kind == PInstrCode::_menubar) && (N > 30)) g_compiler->Error(102);
			CD->TxtFrml = g_compiler->RdStrFrml(nullptr);
			if (Lexem == ',') {
				g_compiler->RdLex();
				if (Lexem != ',') {
					CD->HelpName = g_compiler->RdHelpName();
					PD->HelpRdb = CRdb;
				}
				if (Lexem == ',') {
					g_compiler->RdLex();
					if (Lexem != ',') {
						CD->Condition = g_compiler->RdBool(nullptr);
						if (Lexem == '!') {
							CD->DisplEver = true;
							g_compiler->RdLex();
						}
					}
				}
			}
			g_compiler->Accept(':');
			AddInstr(CD->v_instr, RdPInstr());
		}
		if (Lexem == ';') {
			g_compiler->RdLex();
			if (g_compiler->IsKeyWord("END")) return;
			continue;
		}
		break;
	}

	g_compiler->AcceptKeyWord("END");
}

void RdMenuAttr(Instr_menu* PD)
{
	if (Lexem != ';') return;
	g_compiler->RdLex();
	PD->mAttr[0] = g_compiler->RdAttr(); g_compiler->Accept(',');
	PD->mAttr[1] = g_compiler->RdAttr(); g_compiler->Accept(',');
	PD->mAttr[2] = g_compiler->RdAttr();
	if (Lexem == ',') {
		g_compiler->RdLex();
		PD->mAttr[3] = g_compiler->RdAttr();
	}
}

Instr* RdMenuBox(bool Loop)
{
	Instr_menu* PD = nullptr; pstring* S = nullptr;
	PD = new Instr_menu(PInstrCode::_menubox);
	auto result = PD;
	PD->Loop = Loop;
	if (Lexem == '(') {
		g_compiler->RdLex();
		if (Lexem != ';') {
			PD->X = g_compiler->RdRealFrml(nullptr);
			g_compiler->Accept(',');
			PD->Y = g_compiler->RdRealFrml(nullptr);
		}
		RdMenuAttr(PD);
		g_compiler->Accept(')');
	}
	if (Lexem == '!') { g_compiler->RdLex(); PD->Shdw = true; }
	if (g_compiler->IsKeyWord("PULLDOWN")) PD->PullDown = true;
	if (!g_compiler->TestKeyWord("OF")) PD->HdLine = g_compiler->RdStrFrml(nullptr);
	RdChoices(PD);
	return result;
}

Instr* RdMenuBar()
{
	Instr_menu* PD = new Instr_menu(PInstrCode::_menubar); // GetPInstr(_menubar, 48);
	auto result = PD;
	if (Lexem == '(') {
		g_compiler->RdLex();
		if (Lexem != ';') {
			PD->Y = g_compiler->RdRealFrml(nullptr);
			if (Lexem == ',') {
				g_compiler->RdLex();
				PD->X = g_compiler->RdRealFrml(nullptr);
				g_compiler->Accept(',');
				PD->XSz = g_compiler->RdRealFrml(nullptr);
			}
		}
		RdMenuAttr(PD);
		g_compiler->Accept(')');
	}
	RdChoices(PD);
	return result;
}

Instr_loops* RdIfThenElse()
{
	auto PD = new Instr_loops(PInstrCode::_ifthenelseP);
	auto result = PD;
	PD->Bool = g_compiler->RdBool(nullptr);

	g_compiler->AcceptKeyWord("THEN");
	PD->v_instr = RdPInstr();

	if (g_compiler->IsKeyWord("ELSE")) {
		PD->v_else_instr = RdPInstr();
	}

	return result;
}

Instr_loops* RdWhileDo()
{
	auto PD = new Instr_loops(PInstrCode::_whiledo);
	auto result = PD;
	PD->Bool = g_compiler->RdBool(nullptr);
	g_compiler->AcceptKeyWord("DO");
	PD->v_instr = RdPInstr();
	return result;
}

std::vector<Instr*> RdFor()
{
	LocVar* LV = nullptr;
	if (!g_compiler->FindLocVar(&LVBD, &LV) || (LV->f_typ != 'R')) {
		g_compiler->Error(146);
	}
	g_compiler->RdLex();

	std::vector<Instr*> result;

	// read loop condition and add it as first instruction
	Instr_assign* PD = new Instr_assign(PInstrCode::_asgnloc);
	PD->AssLV = LV;
	g_compiler->Accept(_assign);
	PD->Frml = g_compiler->RdRealFrml(nullptr);
	result.push_back(PD);

	g_compiler->AcceptKeyWord("TO");
	Instr_loops* iLoop = new Instr_loops(PInstrCode::_whiledo);

	FrmlElemFunction* Z1 = new FrmlElemFunction(_compreal, 2);
	Z1->P1 = nullptr;
	Z1->LV1 = LV;
	Z1->N21 = _le;
	Z1->N22 = 5;
	Z1->P2 = g_compiler->RdRealFrml(nullptr);
	iLoop->Bool = Z1;

	g_compiler->AcceptKeyWord("DO");
	iLoop->v_instr = RdPInstr();
	result.push_back(iLoop);

	Instr_assign* iAsg = new Instr_assign(PInstrCode::_asgnloc);
	iAsg->Add = true;
	iAsg->AssLV = LV;
	FrmlElemNumber* Z2 = new FrmlElemNumber(_const, 0, 1);
	//Z->rdb = 1;
	iAsg->Frml = Z2;
	iLoop->v_instr.push_back(iAsg); //iLoop->AddInstr(iAsg);

	return result;
}

Instr* RdCase()
{
	Instr_loops* PD = nullptr;
	Instr_loops* PD1 = nullptr;
	bool first = true;
	Instr_loops* result = nullptr;
	while (true) {
		PD1 = new Instr_loops(PInstrCode::_ifthenelseP);
		if (first) {
			result = PD1;
		}
		else {
			PD->v_else_instr.push_back(PD1);
		}
		PD = PD1;
		first = false;
		PD->Bool = g_compiler->RdBool(nullptr);
		g_compiler->Accept(':');
		AddInstr(PD->v_instr, RdPInstr());
		bool b = Lexem == ';';
		if (b) g_compiler->RdLex();
		if (!g_compiler->IsKeyWord("END")) {
			if (g_compiler->IsKeyWord("ELSE")) {
				while (!g_compiler->IsKeyWord("END")) {
					AddInstr(PD->v_else_instr, RdPInstr());
					if (Lexem == ';') {
						g_compiler->RdLex();
					}
					else {
						g_compiler->AcceptKeyWord("END");
						break;
					}
				}
			}
			else if (b) {
				continue;
			}
			else {
				g_compiler->AcceptKeyWord("END");
			}
		}
		break;
	}
	return result;
}

Instr_loops* RdRepeatUntil()
{
	auto PD = new Instr_loops(PInstrCode::_repeatuntil); // GetPInstr(_repeatuntil, 8);
	Instr_loops* result = PD;
	while (!g_compiler->IsKeyWord("UNTIL")) {
		AddInstr(PD->v_instr, RdPInstr());
		if (Lexem == ';') {
			g_compiler->RdLex();
		}
		else {
			g_compiler->AcceptKeyWord("UNTIL");
			break;
		}
	}
	PD->Bool = g_compiler->RdBool(nullptr);
	return result;
}

Instr_forall* RdForAll()
{
	LocVar* LVi = nullptr;
	LocVar* LVr = nullptr;
	LinkD* LD = nullptr;
	FrmlElem* Z = nullptr;
	FileD* processed_file = nullptr;

	if (!g_compiler->FindLocVar(&LVBD, &LVi)) g_compiler->Error(122);
	g_compiler->RdLex();
	if (LVi->f_typ == 'r') {
		LVr = LVi;
		LVi = nullptr;
		processed_file = LVr->FD;
	}
	else {
		g_compiler->TestReal(LVi->f_typ);
		g_compiler->AcceptKeyWord("IN");
		if (g_compiler->FindLocVar(&LVBD, &LVr)) {
			if (LVr->f_typ == 'f') {
				processed_file = LVr->FD;
				g_compiler->RdLex();
				goto label1;
			}
			if (LVr->f_typ != 'r') g_compiler->Error(141);
			processed_file = LVr->FD;
			g_compiler->RdLex();
		}
		else {
			processed_file = g_compiler->RdFileName();
		label1:
			LVr = nullptr;
		}
#ifdef FandSQL
		if (processed_file->typSQLFile) OldError(155);
#endif
	}
	Instr_forall* PD = new Instr_forall(); // GetPInstr(_forall, 41);
	PD->CFD = processed_file;
	PD->CVar = LVi;
	// TODO: tady je podminka, by to nespadlo
	if (LVr != nullptr) {
		PD->CRecVar = LVr;
	}
#ifdef FandSQL
	if (processed_file->typSQLFile && IsKeyWord("IN")) {
		AcceptKeyWord("SQL"); Accept('('); PD->CBool = RdStrFrml();
		Accept(')'); PD->inSQL = true;
	}
	else {
#endif
		if (g_compiler->IsKeyWord("OWNER")) {
			PD->COwnerTyp = RdOwner(PD->CFD, &PD->CLD, &PD->CLV);
			CViewKey = GetFromKey(PD->CLD);
		}
		else {
			CViewKey = g_compiler->RdViewKey(processed_file);
		}
		g_compiler->processing_F = processed_file;
		if (Lexem == '(') {
			g_compiler->RdLex();
			PD->CBool = g_compiler->RdKeyInBool(PD->CKIRoot, false, true, PD->CSQLFilter, nullptr);
			if ((!PD->CKIRoot.empty()) && (PD->CLV != nullptr)) {
				g_compiler->OldError(118);
			}
			g_compiler->Accept(')');
		}
		if (Lexem == '!') {
			g_compiler->RdLex();
			PD->CWIdx = true;
		}
		if (Lexem == '%') {
			g_compiler->RdLex();
			PD->CProcent = true;
		}
		PD->CKey = CViewKey;

#ifdef FandSQL
	}
#endif

	g_compiler->AcceptKeyWord("DO");
	PD->CInstr = RdPInstr();
	return PD;
}

std::vector<Instr*> RdBeginEnd()
{
	std::vector<Instr*> instructions;
	if (!g_compiler->IsKeyWord("END")) {
		while (true) {
			// read instructions and add them to the list
			AddInstr(instructions, RdPInstr());

			if (Lexem == ';') {
				g_compiler->RdLex();
				if (!g_compiler->IsKeyWord("END")) {
					continue;
				}
			}
			else {
				g_compiler->AcceptKeyWord("END");
			}
			break;
		}
	}
	return instructions;
}

Instr_proc* RdProcArg(char Caller)
{
	std::string ProcName = LexWord;
	RdbPos Pos;
	TypAndFrml TArg[31];
	LocVar* LV = nullptr;
	if (Caller != 'C') {
		g_compiler->RdChptName('P', &Pos, Caller == 'P' || Caller == 'E' || Caller == 'T');
	}
	WORD N = 0;
	if (Caller != 'P') {
		if (Lexem == '(') {
			g_compiler->RdLex();
			goto label1;
		}
	}
	else if (Lexem == ',') {
		g_compiler->RdLex();
		g_compiler->Accept('(');
	label1:
		N++;
		if (N > 30) g_compiler->Error(123);
		TArg[N].Name = LexWord;
		if ((ForwChar != '.') && g_compiler->FindLocVar(&LVBD, &LV) && (LV->f_typ == 'i' || LV->f_typ == 'r')) {
			g_compiler->RdLex();
			TArg[N].FTyp = LV->f_typ;
			TArg[N].FD = LV->FD;
			TArg[N].RecPtr = LV->record;
		}
		else if (Lexem == '@') {
			g_compiler->RdLex();
			if (Lexem == '[') {
				g_compiler->RdLex();
				TArg[N].Name = LexWord;
				g_compiler->Accept(_identifier);
				g_compiler->Accept(',');
				auto z = new FrmlElemFunction(_setmybp, 0); // GetOp(_setmybp, 0);
				z->P1 = g_compiler->RdStrFrml(nullptr);
				TArg[N].TxtFrml = z;
				g_compiler->Accept(']');
			}
			else {
				TArg[N].FD = g_compiler->RdFileName();
			}
			TArg[N].FTyp = 'f';
		}
		else {
			TArg[N].Frml = g_compiler->RdFrml(TArg[N].FTyp, nullptr);
		}
		if (Lexem == ',') {
			g_compiler->RdLex();
			goto label1;
		}
		g_compiler->Accept(')');
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
		g_compiler->RdLex();
	}
	else {
		for (i = 0; i < NKeyNames; i++) {
			if (EquUpCase(KeyNames[i].Nm, LexWord)) {
				lastKey->KeyCode = KeyNames[i].Code;
				lastKey->Break = KeyNames[i].Brk;
				g_compiler->RdLex();
				return;
			}
		}
		g_compiler->Error(129);
	}
}

bool RdHeadLast(EditOpt* EO)
{
	auto result = true;
	if (g_compiler->IsOpt("HEAD")) EO->Head = g_compiler->RdStrFrml(nullptr);
	else if (g_compiler->IsOpt("LAST")) EO->Last = g_compiler->RdStrFrml(nullptr);
	else if (g_compiler->IsOpt("CTRL")) EO->CtrlLast = g_compiler->RdStrFrml(nullptr);
	else if (g_compiler->IsOpt("ALT")) EO->AltLast = g_compiler->RdStrFrml(nullptr);
	else if (g_compiler->IsOpt("SHIFT")) EO->ShiftLast = g_compiler->RdStrFrml(nullptr);
	else result = false;
	return result;
}

bool RdHeadLast(Instr_edittxt* IE)
{
	auto result = true;
	if (g_compiler->IsOpt("HEAD")) IE->Head = g_compiler->RdStrFrml(nullptr);
	else if (g_compiler->IsOpt("LAST")) IE->Last = g_compiler->RdStrFrml(nullptr);
	else if (g_compiler->IsOpt("CTRL")) IE->CtrlLast = g_compiler->RdStrFrml(nullptr);
	else if (g_compiler->IsOpt("ALT")) IE->AltLast = g_compiler->RdStrFrml(nullptr);
	else if (g_compiler->IsOpt("SHIFT")) IE->ShiftLast = g_compiler->RdStrFrml(nullptr);
	else result = false;
	return result;
}

bool RdViewOpt(EditOpt* EO, FileD* file_d)
{
	std::unique_ptr<Compiler> local_compiler = std::make_unique<Compiler>(file_d);
	local_compiler->rdFldNameType = FieldNameType::P;

	FileD* FD = nullptr;
	RprtOpt* RO = nullptr;
	bool Flgs[23]{ false };
	auto result = false;
	local_compiler->RdLex();
	result = true;
	CViewKey = EO->ViewKey;
	if (local_compiler->IsOpt("TAB")) {
		local_compiler->RdNegFldList(EO->NegTab, EO->Tab);
	}
	else if (local_compiler->IsOpt("DUPL")) {
		local_compiler->RdNegFldList(EO->NegDupl, EO->Dupl);
	}
	else if (local_compiler->IsOpt("NOED")) {
		local_compiler->RdNegFldList(EO->NegNoEd, EO->NoEd);
	}
	else if (local_compiler->IsOpt("MODE")) {
		local_compiler->SkipBlank(false);
		if ((Lexem == _quotedstr) && (ForwChar == ',' || ForwChar == ')')) {
			DataEditorParams params;
			int validate = params.SetFromString(LexWord, true);
			if (validate != 0) {
				local_compiler->Error(validate);
			}
			EO->Mode = new FrmlElemString(_const, 0); // GetOp(_const, LexWord.length() + 1);
			((FrmlElemString*)EO->Mode)->S = LexWord;
			local_compiler->RdLex();
		}
		else {
			EO->Mode = local_compiler->RdStrFrml(nullptr);
		}
	}
	else if (RdHeadLast(EO)) {
		return result;
	}
	else if (local_compiler->IsOpt("WATCH")) {
		EO->WatchDelayZ = local_compiler->RdRealFrml(nullptr);
	}
	else if (local_compiler->IsOpt("WW")) {
		local_compiler->Accept('(');
		EO->WFlags = 0;
		if (Lexem == '(') { local_compiler->RdLex(); EO->WFlags = WNoPop; }
		local_compiler->RdW(EO->W);
		local_compiler->RdFrame(&EO->Top, EO->WFlags);
		if (Lexem == ',') {
			local_compiler->RdLex();
			EO->ZAttr = local_compiler->RdAttr(); local_compiler->Accept(',');
			EO->ZdNorm = local_compiler->RdAttr(); local_compiler->Accept(',');
			EO->ZdHiLi = local_compiler->RdAttr();
			if (Lexem == ',') {
				local_compiler->RdLex();
				EO->ZdSubset = local_compiler->RdAttr();
				if (Lexem == ',') {
					local_compiler->RdLex();
					EO->ZdDel = local_compiler->RdAttr();
					if (Lexem == ',') {
						local_compiler->RdLex();
						EO->ZdTab = local_compiler->RdAttr();
						if (Lexem == ',') {
							local_compiler->RdLex();
							EO->ZdSelect = local_compiler->RdAttr();
						}
					}
				}
			}
		}
		local_compiler->Accept(')');
		if ((EO->WFlags & WNoPop) != 0) {
			local_compiler->Accept(')');
		}
	}
	else if (local_compiler->IsOpt("EXIT")) {
		local_compiler->Accept('(');
		while (true) {
			EdExitD* X = new EdExitD();
			EO->ExD.push_back(X);

			RdKeyList(X, local_compiler);
			if (local_compiler->IsKeyWord("QUIT")) X->Typ = 'Q';
			else if (local_compiler->IsKeyWord("REPORT")) {
				if (X->AtWrRec || (EO->LVRecPtr != nullptr)) local_compiler->OldError(144);
				local_compiler->Accept('(');
				X->Typ = 'R';
				RO = local_compiler->GetRprtOpt();
				local_compiler->RdChptName('R', &RO->RprtPos, true);
				while (Lexem == ',') {
					local_compiler->RdLex();
					if (local_compiler->IsOpt("ASSIGN")) RdPath(true, RO->Path, RO->CatIRec);
					else if (local_compiler->IsKeyWord("EDIT")) RO->Edit = true;
					else local_compiler->Error(130);
				}
				X->RO = RO; local_compiler->Accept(')');
			}
			else if (!(Lexem == ',' || Lexem == ')')) {
				X->Typ = 'P';
				X->Proc = RdProcArg('E');
			}
			if (Lexem == ',') {
				local_compiler->RdLex();
				continue;
			}
			break;
		}
		local_compiler->Accept(')');
	}
	else if (EO->LVRecPtr != nullptr) {
		result = false;
	}
	else if (local_compiler->IsOpt("COND")) {
		if (Lexem == '(') {
			local_compiler->RdLex();
			EO->Cond = local_compiler->RdKeyInBool(EO->KIRoot, false, true, EO->SQLFilter, nullptr);
			local_compiler->Accept(')');
		}
		else {
			EO->Cond = local_compiler->RdKeyInBool(EO->KIRoot, false, true, EO->SQLFilter, nullptr);
		}
	}
	else if (local_compiler->IsOpt("JOURNAL")) {
		EO->Journal = local_compiler->RdFileName();
		WORD l = EO->Journal->FF->RecLen - 13;
		if (file_d->FF->file_type == FileType::INDEX) {
			l++;
		}
		if (file_d->FF->RecLen != l) {
			local_compiler->OldError(111);
		}
	}
	else if (local_compiler->IsOpt("SAVEAFTER")) {
		EO->SaveAfterZ = local_compiler->RdRealFrml(nullptr);
	}
	else if (local_compiler->IsOpt("REFRESH")) {
		EO->RefreshDelayZ = local_compiler->RdRealFrml(nullptr);
	}
	else {
		result = false;
	}
	return result;
}

void RdKeyList(EdExitD* X, const std::unique_ptr<Compiler>& c)
{
	while (true) {
		if ((Lexem == '(') || (Lexem == '^')) {
			c->RdNegFldList(X->NegFlds, X->Flds);
		}
		else if (c->IsKeyWord("RECORD")) {
			X->AtWrRec = true;
		}
		else if (c->IsKeyWord("NEWREC")) {
			X->AtNewRec = true;
		}
		else {
			RdKeyCode(X);
		}
		if (Lexem == ',') {
			c->RdLex();
			continue;
		}
		break;
	}
	c->Accept(':');
}

void RdProcCall(Instr** pinstr)
{
	//Instr* PD = nullptr;
	if (g_compiler->IsKeyWord("EXEC")) *pinstr = RdExec();
	else if (g_compiler->IsKeyWord("COPYFILE")) *pinstr = RdCopyFile();
	else if (g_compiler->IsKeyWord("PROC")) {
		g_compiler->RdLex();
		*pinstr = RdProcArg('P');
	}
	else if (g_compiler->IsKeyWord("DISPLAY")) *pinstr = RdDisplay();
	else if (g_compiler->IsKeyWord("CALL")) *pinstr = RdRDBCall();
	else if (g_compiler->IsKeyWord("WRITELN")) RdWriteln(WriteType::writeln, (Instr_writeln**)pinstr);
	else if (g_compiler->IsKeyWord("WRITE")) RdWriteln(WriteType::write, (Instr_writeln**)pinstr);
	else if (g_compiler->IsKeyWord("HEADLINE")) {
		*pinstr = new Instr_assign(PInstrCode::_headline); // GetPD(_headline, 4);
		g_compiler->RdLex();
		goto label1;
	}
	else if (g_compiler->IsKeyWord("SETKEYBUF")) {
		*pinstr = new Instr_assign(PInstrCode::_setkeybuf); //GetPD(_setkeybuf, 4);
		g_compiler->RdLex();
		goto label1;
	}
	else if (g_compiler->IsKeyWord("HELP")) {
		*pinstr = new Instr_help(); // GetPD(_help, 8);
		g_compiler->RdLex();
		if (CRdb->help_file == nullptr) g_compiler->OldError(132);
		((Instr_help*)*pinstr)->HelpRdb0 = CRdb;
	label1:
		((Instr_help*)*pinstr)->Frml0 = g_compiler->RdStrFrml(nullptr);
	}
	else if (g_compiler->IsKeyWord("MESSAGE")) RdWriteln(WriteType::message, (Instr_writeln**)pinstr);
	else if (g_compiler->IsKeyWord("GOTOXY")) *pinstr = RdGotoXY();
	else if (g_compiler->IsKeyWord("MERGE")) {
		// PD = (Instr_merge_display*)GetPD(_merge, sizeof(RdbPos));
		*pinstr = new Instr_merge_display(PInstrCode::_merge);
		g_compiler->RdLex();
		RdbPos rp;
		g_compiler->RdChptName('M', &rp, true);
		((Instr_merge_display*)*pinstr)->Pos = rp;
	}
	else if (g_compiler->IsKeyWord("SORT")) *pinstr = RdSortCall();
	else if (g_compiler->IsKeyWord("EDIT")) *pinstr = RdEditCall();
	else if (g_compiler->IsKeyWord("REPORT")) *pinstr = RdReportCall();
	else if (g_compiler->IsKeyWord("EDITTXT")) *pinstr = RdEditTxt();
	else if (g_compiler->IsKeyWord("PRINTTXT")) *pinstr = RdPrintTxt();
	else if (g_compiler->IsKeyWord("PUTTXT")) *pinstr = RdPutTxt();
	else if (g_compiler->IsKeyWord("TURNCAT")) *pinstr = RdTurnCat();
	else if (g_compiler->IsKeyWord("RELEASEDRIVE")) *pinstr = RdReleaseDrive();
	else if (g_compiler->IsKeyWord("SETPRINTER")) {
		*pinstr = new Instr_assign(PInstrCode::_setprinter); // GetPD(_setprinter, 4);
		g_compiler->RdLex();
		goto label2;
	}
	else if (g_compiler->IsKeyWord("INDEXFILE")) *pinstr = RdIndexfile();
	else if (g_compiler->IsKeyWord("GETINDEX"))*pinstr = RdGetIndex();
	else if (g_compiler->IsKeyWord("MOUNT")) *pinstr = RdMount();
	else if (g_compiler->IsKeyWord("CLRSCR")) *pinstr = RdClrWw();
	else if (g_compiler->IsKeyWord("APPENDREC")) *pinstr = RdMixRecAcc(PInstrCode::_appendRec);
	else if (g_compiler->IsKeyWord("DELETEREC")) *pinstr = RdMixRecAcc(PInstrCode::_deleterec);
	else if (g_compiler->IsKeyWord("RECALLREC")) *pinstr = RdMixRecAcc(PInstrCode::_recallrec);
	else if (g_compiler->IsKeyWord("READREC")) *pinstr = RdMixRecAcc(PInstrCode::_readrec);
	else if (g_compiler->IsKeyWord("WRITEREC")) *pinstr = RdMixRecAcc(PInstrCode::_writerec);
	else if (g_compiler->IsKeyWord("LINKREC")) *pinstr = RdLinkRec();
	else if (g_compiler->IsKeyWord("DELAY")) {
		*pinstr = new Instr_assign(PInstrCode::_delay); // GetPD(_delay, 4);
		g_compiler->RdLex();
		goto label2;
	}
	else if (g_compiler->IsKeyWord("SOUND")) {
		*pinstr = new Instr_assign(PInstrCode::_sound); // GetPD(_sound, 4);
		g_compiler->RdLex();
	label2:
		((Instr_assign*)*pinstr)->Frml = g_compiler->RdRealFrml(nullptr);
	}
	else if (g_compiler->IsKeyWord("LPROC")) *pinstr = RdCallLProc();

#ifdef FandGraph
	else if (g_compiler->IsKeyWord("GRAPH")) *pinstr = RdGraphP();
	else if (g_compiler->IsKeyWord("PUTPIXEL")) {
		*pinstr = new Instr_putpixel(PInstrCode::_putpixel); // GetPD(_putpixel, 3 * 4);
		goto label3;
	}
	else if (g_compiler->IsKeyWord("LINE")) {
		*pinstr = new Instr_putpixel(PInstrCode::_line); // GetPD(_line, 5 * 4);
		goto label3;
	}
	else if (g_compiler->IsKeyWord("RECTANGLE")) {
		*pinstr = new Instr_putpixel(PInstrCode::_rectangle); // GetPD(_rectangle, 5 * 4);
		goto label3;
	}
	else if (g_compiler->IsKeyWord("ELLIPSE")) {
		*pinstr = new Instr_putpixel(PInstrCode::_ellipse);  // GetPD(_ellipse, 7 * 4);
		goto label3;
	}
	else if (g_compiler->IsKeyWord("FLOODFILL")) {
		*pinstr = new Instr_putpixel(PInstrCode::_floodfill); // GetPD(_floodfill, 5 * 4);
		goto label3;
	}
	else if (g_compiler->IsKeyWord("OUTTEXTXY")) {
		*pinstr = new Instr_putpixel(PInstrCode::_outtextxy); // GetPD(_outtextxy, 11 * 4);
	label3:
		g_compiler->RdLex(); // read '('
		auto iPutPixel = (Instr_putpixel*)(*pinstr);
		iPutPixel->Par1 = g_compiler->RdRealFrml(nullptr);
		g_compiler->Accept(',');
		iPutPixel->Par2 = g_compiler->RdRealFrml(nullptr);
		g_compiler->Accept(',');
		if (iPutPixel->Kind == PInstrCode::_outtextxy) {
			iPutPixel->Par3 = g_compiler->RdStrFrml(nullptr);
			g_compiler->Accept(',');
			iPutPixel->Par4 = g_compiler->RdRealFrml(nullptr);
			g_compiler->Accept(',');
			iPutPixel->Par5 = g_compiler->RdAttr();
			if (Lexem == ',') {
				g_compiler->RdLex();
				iPutPixel->Par6 = g_compiler->RdRealFrml(nullptr);
				if (Lexem == ',') {
					g_compiler->RdLex();
					iPutPixel->Par7 = g_compiler->RdRealFrml(nullptr);
					if (Lexem == ',') {
						g_compiler->RdLex();
						iPutPixel->Par8 = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
						iPutPixel->Par9 = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
						iPutPixel->Par10 = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
						iPutPixel->Par11 = g_compiler->RdRealFrml(nullptr);
					}
				}
			}
		}
		else if (iPutPixel->Kind == PInstrCode::_putpixel) iPutPixel->Par3 = g_compiler->RdAttr();
		else {
			iPutPixel->Par3 = g_compiler->RdRealFrml(nullptr);
			g_compiler->Accept(',');
			if (iPutPixel->Kind == PInstrCode::_floodfill) iPutPixel->Par4 = g_compiler->RdAttr();
			else iPutPixel->Par4 = g_compiler->RdRealFrml(nullptr);
			g_compiler->Accept(',');
			iPutPixel->Par5 = g_compiler->RdAttr();
			if ((iPutPixel->Kind == PInstrCode::_ellipse) && (Lexem == ',')) {
				g_compiler->RdLex();
				iPutPixel->Par6 = g_compiler->RdRealFrml(nullptr);
				g_compiler->Accept(',');
				iPutPixel->Par7 = g_compiler->RdRealFrml(nullptr);
			}
		}
	}
#endif 
	else if (g_compiler->IsKeyWord("CLOSE")) {
		*pinstr = new Instr_closefds(); // GetPD(_closefds, 4);
		g_compiler->RdLex();
		((Instr_closefds*)*pinstr)->clFD = g_compiler->RdFileName();
	}
	else if (g_compiler->IsKeyWord("BACKUP")) *pinstr = RdBackup(' ', true);
	else if (g_compiler->IsKeyWord("BACKUPM")) *pinstr = RdBackup('M', true);
	else if (g_compiler->IsKeyWord("RESTORE")) *pinstr = RdBackup(' ', false);
	else if (g_compiler->IsKeyWord("RESTOREM")) *pinstr = RdBackup('M', false);
	else if (g_compiler->IsKeyWord("SETEDITTXT")) *pinstr = RdSetEditTxt();
	else if (g_compiler->IsKeyWord("SETMOUSE")) {
		*pinstr = new Instr_setmouse(); // GetPD(_setmouse, 12);
		g_compiler->RdLex();
		((Instr_setmouse*)*pinstr)->MouseX = g_compiler->RdRealFrml(nullptr);
		g_compiler->Accept(',');
		((Instr_setmouse*)*pinstr)->MouseY = g_compiler->RdRealFrml(nullptr);
		g_compiler->Accept(',');
		((Instr_setmouse*)*pinstr)->Show = g_compiler->RdBool(nullptr);
	}
	else if (g_compiler->IsKeyWord("CHECKFILE")) {
		*pinstr = new Instr_checkfile();
		g_compiler->RdLex();
		auto iPD = (Instr_checkfile*)*pinstr;
		iPD->cfFD = g_compiler->RdFileName();
		if (iPD->cfFD != nullptr && (iPD->cfFD->FF->file_type == FileType::FAND8 || iPD->cfFD->FF->file_type == FileType::DBF)
#ifdef FandSQL
			|| PD->cfFD->typSQLFile
#endif
			) g_compiler->OldError(169);
		g_compiler->Accept(',');
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
	else if (g_compiler->IsKeyWord("PORTOUT")) {
		*pinstr = new Instr_portout(); // GetPD(_portout, 12);
		g_compiler->RdLex();
		auto iPD = (Instr_portout*)*pinstr;
		iPD->IsWord = g_compiler->RdBool(nullptr); g_compiler->Accept(',');
		iPD->Port = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
		iPD->PortWhat = g_compiler->RdRealFrml(nullptr);
	}
	else g_compiler->Error(34);
	g_compiler->Accept(')');
}

std::vector<FieldDescr*> RdFlds()
{
	std::vector<FieldDescr*> FLRoot;
	//FieldListEl* FL = nullptr;

	while (true) {
		auto fd = g_compiler->RdFldName(g_compiler->processing_F);
		FLRoot.push_back(fd);
		if (Lexem == ',') {
			g_compiler->RdLex();
			continue;
		}
		break;
	}

	return FLRoot;
}

std::vector<FieldDescr*> RdSubFldList(const std::vector<FieldDescr*>& v_fields, char Opt)
{
	std::vector<FieldDescr*> result;
	g_compiler->Accept('(');

	while (true) {
		FieldDescr* field = nullptr;
		if (v_fields.empty()) {
			field = g_compiler->RdFldName(g_compiler->processing_F);
			if (field == nullptr) {
				g_compiler->Error(43);
			}
		}
		else {
			bool found = false;
			g_compiler->TestIdentif();
			for (FieldDescr* f : v_fields) {
				if (EquUpCase(f->Name, LexWord)) {
					found = true;
					field = f;
					result.push_back(f);
					break;
				}
			}
			if (!found) {
				g_compiler->Error(43);
			}
			g_compiler->RdLex();
		}

		if ((Opt == 'S') && (field->frml_type != 'R')) {
			g_compiler->OldError(20);
		}

		if (Lexem == ',') {
			g_compiler->RdLex();
			continue;
		}

		break;
	}

	g_compiler->Accept(')');

	return result;
}

Instr_sort* RdSortCall()
{
	auto PD = new Instr_sort(); // GetPD(_sort, 8);
	g_compiler->RdLex();
	FileD* FD = g_compiler->RdFileName();
	PD->SortFD = FD;
#ifdef FandSQL
	if (v_files->typSQLFile) OldError(155);
#endif
	g_compiler->Accept(',');
	g_compiler->Accept('(');
	g_compiler->RdKFList(PD->SK, PD->SortFD);
	g_compiler->Accept(')');
	return PD;
}

Instr_edit* RdEditCall()
{
	LocVar* lv = nullptr;
	Instr_edit* instr_edit = new Instr_edit();
	g_compiler->RdLex();
	instr_edit->options.UserSelFlds = true;

	if (IsRecVar(&lv)) {
		instr_edit->options.LVRecPtr = lv->record;
		instr_edit->EditFD = lv->FD;
	}
	else {
		instr_edit->EditFD = g_compiler->RdFileName();
		XKey* K = g_compiler->RdViewKey(instr_edit->EditFD);
		if (K == nullptr) K = instr_edit->EditFD->Keys.empty() ? nullptr : instr_edit->EditFD->Keys[0];
		instr_edit->options.ViewKey = K;
	}
	//PD->EditFD = CFile;
	g_compiler->Accept(',');
	if (g_compiler->IsOpt("U")) {
		g_compiler->TestIdentif();
		if (instr_edit->EditFD->ViewNames.empty()) {
			g_compiler->Error(114);
		}
		stSaveState* p = g_compiler->SaveCompState();
		bool b = RdUserView(instr_edit->EditFD, LexWord, &instr_edit->options);
		g_compiler->RestoreCompState(p);
		if (!b) g_compiler->Error(114);
		g_compiler->RdLex();
	}
	else {
		g_compiler->processing_F = instr_edit->EditFD;
		RdBegViewDcl(&instr_edit->options);
	}
	while (Lexem == ',') {
		bool b = RdViewOpt(&instr_edit->options, instr_edit->EditFD);
		if (!b) RdEditOpt(&instr_edit->options, instr_edit->EditFD);
	}
	return instr_edit;
}

void RdEditOpt(EditOpt* EO, FileD* file_d)
{
	if (g_compiler->IsOpt("FIELD")) {
		EO->StartFieldZ = g_compiler->RdStrFrml(nullptr);
	}
	else if (EO->LVRecPtr != nullptr) {
		g_compiler->Error(125);
	}
	else if (g_compiler->IsOpt("OWNER")) {
		if (EO->SQLFilter || (!EO->KIRoot.empty())) {
			g_compiler->OldError(179);
		}
		EO->OwnerTyp = RdOwner(file_d, &EO->DownLD, &EO->DownLV);
	}
	else if (g_compiler->IsOpt("RECKEY")) {
		EO->StartRecKeyZ = g_compiler->RdStrFrml(nullptr);
	}
	else if (
#ifdef FandSQL
		!file_d->typSQLFile &&
#endif
		g_compiler->IsOpt("RECNO")) {
		EO->StartRecNoZ = g_compiler->RdRealFrml(nullptr);
	}
	else if (g_compiler->IsOpt("IREC")) {
		EO->StartIRecZ = g_compiler->RdRealFrml(nullptr);
	}
	else if (g_compiler->IsKeyWord("CHECK")) {
		EO->SyntxChk = true;
	}
	else if (g_compiler->IsOpt("SEL")) {
		LocVar* lv = RdIdxVar();
		EO->SelKey = (XWKey*)lv->record;
		if ((EO->ViewKey == nullptr)) {
			g_compiler->OldError(108);
		}
		if (EO->ViewKey == EO->SelKey) {
			g_compiler->OldError(184);
		}
		if ((!EO->ViewKey->KFlds.empty())
			&& (!EO->SelKey->KFlds.empty())
			&& !KeyFldD::EquKFlds(EO->SelKey->KFlds, EO->ViewKey->KFlds)) {
			g_compiler->OldError(178);
		}
	}
	else {
		g_compiler->Error(125);
	}
}

Instr* RdReportCall()
{
	LocVar* lv = nullptr;
	Instr_report* PD = new Instr_report();
	g_compiler->RdLex();
	RprtOpt* RO = g_compiler->GetRprtOpt();
	PD->RO = RO;
	bool has_first = false;
	FileD* processing_file = nullptr;

	if (Lexem != ',') {
		has_first = true;
		bool b = false;
		if (Lexem == '(') {
			g_compiler->RdLex();
			b = true;
		}

		while (true) {
			RprtFDListEl* FDL = new RprtFDListEl();
			RO->FDL.push_back(FDL);

			if (IsRecVar(&lv)) {
				FDL->LVRecPtr = lv->record;
				FDL->FD = lv->FD;
			}
			else {
				processing_file = g_compiler->RdFileName();
				FDL->FD = processing_file;
				g_compiler->processing_F = processing_file;
				CViewKey = g_compiler->RdViewKey(FDL->FD);
				FDL->ViewKey = CViewKey;
				if (Lexem == '(') {
					g_compiler->RdLex();
					FDL->Cond = g_compiler->RdKeyInBool(FDL->KeyIn, true, true, FDL->SQLFilter, nullptr);
					g_compiler->Accept(')');
				}
			}

			if (b && (Lexem == ',')) {
				g_compiler->RdLex();
				continue;
			}

			break;
		}

		if (b) {
			g_compiler->Accept(')');
		}
		processing_file = RO->FDL[0]->FD;
		g_compiler->processing_F = processing_file;
		CViewKey = RO->FDL[0]->ViewKey;
	}

	g_compiler->Accept(',');
	if (Lexem == '[') {
		g_compiler->RdLex();
		RO->RprtPos.rdb = (RdbD*)g_compiler->RdStrFrml(nullptr);
		RO->RprtPos.i_rec = 0;
		RO->FromStr = true;
		g_compiler->Accept(']');
	}
	else if (!has_first || (Lexem == _identifier)) {
		g_compiler->TestIdentif();
		if (!g_compiler->FindChpt('R', LexWord, false, &RO->RprtPos)) {
			g_compiler->Error(37);
		}
		g_compiler->RdLex();
	}
	else {
		g_compiler->Accept('(');
		switch (Lexem) {
		case '?': {
			RO->Flds = g_compiler->AllFldsList(processing_file, false);
			g_compiler->RdLex();
			RO->UserSelFlds = true;
			break;
		}
		case ')': {
			RO->Flds = g_compiler->AllFldsList(processing_file, true);
			break;
		}
		default: {
			RO->Flds = RdFlds();
			if (Lexem == '?') {
				g_compiler->RdLex();
				RO->UserSelFlds = true;
			}
			break;
		}
		}
		g_compiler->Accept(')');
	}
	while (Lexem == ',') {
		g_compiler->RdLex();
		if (has_first) {
			RprtFDListEl* FDL = RO->FDL[RO->FDL.size() - 1]; // last element
			RdRprtOpt(RO, FDL->LVRecPtr == nullptr);
		}
		else {
			RdRprtOpt(RO, false);
		}
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

	if (g_compiler->IsOpt("ASSIGN")) {
		RdPath(true, RO->Path, RO->CatIRec);
	}
	else if (g_compiler->IsOpt("TIMES")) {
		RO->Times = g_compiler->RdRealFrml(nullptr);
	}
	else if (g_compiler->IsOpt("MODE")) {
		if (g_compiler->IsKeyWord("ONLYSUM")) {
			RO->Mode = _ATotal;
		}
		else if (g_compiler->IsKeyWord("ERRCHECK")) {
			RO->Mode = _AErrRecs;
		}
		else {
			g_compiler->Error(49);
		}
	}
	else if (g_compiler->IsKeyWord("COND")) {
		if (!has_first) {
			g_compiler->OldError(51);
			g_compiler->Accept('(');
			g_compiler->RdKFList(RO->SK, CFile);
			g_compiler->Accept(')');
		}
		WORD Low = CurrPos;
		g_compiler->Accept(_equ);
		bool br = false;
		if (Lexem == '(') {
			Low = CurrPos;
			g_compiler->RdLex();
			br = true;
			if (Lexem == '?') {
				g_compiler->RdLex();
				RO->UserCondQuest = true;
				if (br) {
					g_compiler->Accept(')');
				}
				return;
			}
		}
		RO->FDL[0]->Cond = g_compiler->RdKeyInBool(RO->FDL[0]->KeyIn, true, true, RO->FDL[0]->SQLFilter, nullptr);
		N = OldErrPos - Low;
		RO->CondTxt = std::string((const char*)&InpArrPtr[Low], N);

		if (br) {
			g_compiler->Accept(')');
		}
	}
	else if (g_compiler->IsOpt("CTRL")) {
		if (!has_first) {
			g_compiler->OldError(51);
			g_compiler->Accept('(');
			g_compiler->RdKFList(RO->SK, CFile);
			g_compiler->Accept(')');
		}
		RO->Ctrl = RdSubFldList(RO->Flds, 'C');
	}
	else if (g_compiler->IsOpt("SUM")) {
		if (!has_first) {
			g_compiler->OldError(51);
			g_compiler->Accept('(');
			g_compiler->RdKFList(RO->SK, CFile);
			g_compiler->Accept(')');
		}
		RO->Sum = RdSubFldList(RO->Flds, 'S');
	}
	else if (g_compiler->IsOpt("WIDTH")) {
		RO->WidthFrml = g_compiler->RdRealFrml(nullptr);
	}
	else if (g_compiler->IsOpt("STYLE")) {
		if (g_compiler->IsKeyWord("COMPRESSED")) {
			RO->Style = 'C';
		}
		else {
			if (g_compiler->IsKeyWord("NORMAL")) {
				RO->Style = 'N';
			}
			else {
				g_compiler->Error(50);
			}
		}
	}
	else if (g_compiler->IsKeyWord("EDIT")) {
		RO->Edit = true;
	}
	else if (g_compiler->IsKeyWord("PRINTCTRL")) {
		RO->PrintCtrl = true;
	}
	else if (g_compiler->IsKeyWord("CHECK")) {
		RO->SyntxChk = true;
	}
	else if (g_compiler->IsOpt("SORT")) {
		if (!has_first) {
			g_compiler->OldError(51);
		}
		g_compiler->Accept('(');
		g_compiler->RdKFList(RO->SK, g_compiler->processing_F);
		g_compiler->Accept(')');
	}
	else if (g_compiler->IsOpt("HEAD")) {
		RO->Head = g_compiler->RdStrFrml(nullptr);
	}
	else {
		g_compiler->Error(45);
	}
}

Instr* RdRDBCall()
{
	std::string s;
	auto PD = new Instr_call(); // GetPD(_call, 12);
	g_compiler->RdLex();
	//s[0] = 0;
	if (Lexem == '\\') {
		s = "\\";
		g_compiler->RdLex();
	}
	g_compiler->TestIdentif();
	if (LexWord.length() > 8) g_compiler->Error(2);
	PD->RdbNm = s + std::string(LexWord);
	g_compiler->RdLex();
	if (Lexem == ',') {
		g_compiler->RdLex();
		g_compiler->TestIdentif();
		if (LexWord.length() > 12) g_compiler->Error(2);
		PD->ProcNm = LexWord;
		g_compiler->RdLex();
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
	g_compiler->RdLex();
	RdPath(true, PD->ProgPath, PD->ProgCatIRec);
	g_compiler->Accept(',');
	PD->Param = g_compiler->RdStrFrml(nullptr);
	while (Lexem == ',') {
		g_compiler->RdLex();
		if (g_compiler->IsKeyWord("NOCANCEL")) PD->NoCancel = true;
		else if (g_compiler->IsKeyWord("FREEMEM")) PD->FreeMm = true;
		else if (g_compiler->IsKeyWord("LOADFONT")) PD->LdFont = true;
		else if (g_compiler->IsKeyWord("TEXTMODE")) PD->TextMd = true;
		else g_compiler->Error(101);
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
	g_compiler->RdLex();
	noapp = false;
	CD = new CopyD(); // (CopyD*)GetZStore(sizeof(*D));
	PD->CD = CD;
	/* !!! with D^ do!!! */
	CD->FD1 = RdPath(false, CD->Path1, CD->CatIRec1);
	CD->WithX1 = RdX(CD->FD1);
	if (Lexem == '/') {
		if (CD->FD1 != nullptr) { CFile = CD->FD1; CD->ViewKey = g_compiler->RdViewKey(CD->FD1); }
		else CD->Opt1 = RdCOpt();
	}
	g_compiler->Accept(',');
	CD->FD2 = RdPath(false, CD->Path2, CD->CatIRec2);
	CD->WithX2 = RdX(CD->FD2);
	if (Lexem == '/') {
		if (CD->FD2 != nullptr) g_compiler->Error(139);
		else CD->Opt2 = RdCOpt();
	}
	if (!TestFixVar(CD->Opt1, CD->FD1, CD->FD2) && !TestFixVar(CD->Opt2, CD->FD2, CD->FD1))
	{
		if ((CD->Opt1 == CpOption::cpTxt) && (CD->FD2 != nullptr)) g_compiler->OldError(139);
		noapp = (CD->FD1 == nullptr) ^ (CD->FD2 == nullptr); // XOR
#ifdef FandSQL
		if (noapp)
			if ((FD1 != nullptr) && (FD1->typSQLFile) || (FD2 != nullptr)
				&& (FD2->typSQLFile)) OldError(155);
#endif
	}
	while (Lexem == ',') {
		g_compiler->RdLex();
		if (g_compiler->IsOpt("HEAD")) {
			CD->HdFD = g_compiler->RdFileName();
			g_compiler->Accept('.');
			CD->HdF = g_compiler->RdFldName(CD->HdFD);
			if ((CD->HdF->frml_type != 'S') || !CD->HdFD->IsParFile
				|| (CD->Opt1 == CpOption::cpFix || CD->Opt1 == CpOption::cpVar)
				&& ((CD->HdF->Flg & f_Stored) == 0)) g_compiler->Error(52);
		}
		else if (g_compiler->IsOpt("MODE")) {
			g_compiler->TestLex(_quotedstr);
			for (i = 0; i < 7; i++) {
				if (EquUpCase(LexWord, ModeTxt[i])) {
					CD->Mode = i + 1;
					goto label1;
				}
			}
			g_compiler->Error(142);
		label1:
			g_compiler->RdLex();
		}
		else if (g_compiler->IsKeyWord("NOCANCEL")) CD->NoCancel = true;
		else if (g_compiler->IsKeyWord("APPEND")) {
			if (noapp) g_compiler->OldError(139); CD->Append = true;
		}
		else g_compiler->Error(52);
	}
	return PD;
}

CpOption RdCOpt()
{
	BYTE i = 0;
	pstring OptArr[3] = { "FIX", "VAR", "TXT" };
	g_compiler->RdLex();
	g_compiler->TestIdentif();
	for (i = 0; i < 3; i++)
		if (EquUpCase(OptArr[i], LexWord)) {
			g_compiler->RdLex();
			return CpOption(i + 1); // vracime i + 1 (CpOption ma 4 moznosti, je to posunute ...)
		}
	g_compiler->Error(53);
	throw std::exception("Bad value in RdCOpt() in rdproc.cpp");
}

bool RdX(FileD* FD)
{
	auto result = false;
	if ((Lexem == '.') && (FD != nullptr)) {
		g_compiler->RdLex();
		g_compiler->AcceptKeyWord("X");
		if (FD->FF->file_type != FileType::INDEX) g_compiler->OldError(108);
		result = true;
	}
	return result;
}

bool TestFixVar(CpOption Opt, FileD* FD1, FileD* FD2)
{
	auto result = false;
	if ((Opt != CpOption::cpNo) && (FD1 != nullptr)) g_compiler->OldError(139);
	result = false;
	if (Opt == CpOption::cpFix || Opt == CpOption::cpVar) {
		result = true;
		if (FD2 == nullptr) g_compiler->OldError(139);
	}
	return result;
}

bool RdList(pstring* S)
{
	auto result = false;
	if (Lexem != '(') return result;
	g_compiler->RdLex();
	// TODO: g_compiler !!! S = (pstring*)(g_compiler->RdStrFrml);
	g_compiler->Accept(')');
	result = true;
	return result;
}

Instr* RdPrintTxt()
{
	auto PD = new Instr_edittxt(PInstrCode::_printtxt);
	g_compiler->RdLex();
	if (g_compiler->FindLocVar(&LVBD, &PD->TxtLV)) {
		g_compiler->RdLex();
		g_compiler->TestString(PD->TxtLV->f_typ);
	}
	else RdPath(true, PD->TxtPath, PD->TxtCatIRec);
	return PD;
}

Instr* RdEditTxt()
{
	EdExitD* pX;
	auto PD = new Instr_edittxt(PInstrCode::_edittxt);
	g_compiler->RdLex();
	if (g_compiler->FindLocVar(&LVBD, &PD->TxtLV)) {
		g_compiler->RdLex();
		g_compiler->TestString(PD->TxtLV->f_typ);
	}
	else RdPath(true, PD->TxtPath, PD->TxtCatIRec);
	PD->EdTxtMode = 'T';
	while (Lexem == ',') {
		g_compiler->RdLex();
		if (g_compiler->IsOpt("WW")) {
			g_compiler->Accept('(');
			if (Lexem == '(') { g_compiler->RdLex(); PD->WFlags = WNoPop; }
			g_compiler->RdW(PD->Ww);
			g_compiler->RdFrame(&PD->Hd, PD->WFlags);
			if (Lexem == ',') { g_compiler->RdLex(); PD->Atr = g_compiler->RdAttr(); }
			g_compiler->Accept(')');
			if ((PD->WFlags & WNoPop) != 0) g_compiler->Accept(')');
		}
		else
			if (g_compiler->IsOpt("TXTPOS")) PD->TxtPos = g_compiler->RdRealFrml(nullptr);
			else if (g_compiler->IsOpt("TXTXY")) PD->TxtXY = g_compiler->RdRealFrml(nullptr);
			else if (g_compiler->IsOpt("ERRMSG")) PD->ErrMsg = g_compiler->RdStrFrml(nullptr);
			else if (g_compiler->IsOpt("EXIT")) {
				g_compiler->Accept('(');
			label1:
				pX = new EdExitD(); // (EdExitD*)GetZStore(sizeof(*pX));
				PD->ExD.push_back(pX);
			label2:
				RdKeyCode(pX);
				if (Lexem == ',') { g_compiler->RdLex(); goto label2; }
				g_compiler->Accept(':');
				if (g_compiler->IsKeyWord("QUIT")) pX->Typ = 'Q';
				else if (!(Lexem == ',' || Lexem == ')')) {
					pX->Typ = 'P';
					pX->Proc = RdProcArg('T');
				}
				if (Lexem == ',') { g_compiler->RdLex(); goto label1; }
				g_compiler->Accept(')');
			}
			else
				if (RdHeadLast(PD)) {}
				else if (g_compiler->IsKeyWord("NOEDIT")) PD->EdTxtMode = 'V';
				else g_compiler->Error(161);
	}
	return PD;
}

Instr* RdPutTxt()
{
	auto PD = new Instr_puttxt(); // GetPD(_puttxt, 11);
	g_compiler->RdLex();
	RdPath(true, PD->TxtPath1, PD->TxtCatIRec1);
	g_compiler->Accept(',');
	PD->Txt = g_compiler->RdStrFrml(nullptr);
	if (Lexem == ',') {
		g_compiler->RdLex();
		g_compiler->AcceptKeyWord("APPEND");
		PD->App = true;
	}
	return PD;
}

Instr* RdTurnCat()
{
	Instr_turncat* PD = new Instr_turncat();
	g_compiler->RdLex();
	g_compiler->TestIdentif();
	PD->NextGenFD = g_compiler->FindFileD();
	const int first = catalog->GetCatalogIRec(LexWord, true);
	TestCatError(first, LexWord, true);
	g_compiler->RdLex();
	PD->FrstCatIRec = first;
	const std::string rdb_name = catalog->GetRdbName(first);
	const std::string file_name = catalog->GetFileName(first);
	int i = first + 1;
	while (catalog->GetCatalogFile()->FF->NRecs >= i
		&& EquUpCase(rdb_name, catalog->GetRdbName(i))
		&& EquUpCase(file_name, catalog->GetFileName(i))) {
		i++;
	}
	if (i == first + 1) {
		g_compiler->OldError(98);
	}
	PD->NCatIRecs = i - first;
	g_compiler->Accept(',');
	PD->TCFrml = g_compiler->RdRealFrml(nullptr);
	return PD;
}

void RdWriteln(WriteType OpKind, Instr_writeln** pinstr)
{
	std::vector<WrLnD*> d;
	g_compiler->RdLex();
	FrmlElem* z = nullptr;
	WrLnD* w = new WrLnD();
	d.push_back(w);
//label1:
	while (true) {
		w->Frml = g_compiler->RdFrml(w->Typ, nullptr);

		if (w->Typ == 'R') {
			w->Typ = 'F';
			if (Lexem == ':') {
				g_compiler->RdLex();
				if (Lexem == _quotedstr) {
					w->Typ = 'D';
					w->Mask = LexWord;
					g_compiler->RdLex();
				}
				else {
					w->N = g_compiler->RdInteger();
					if (Lexem == ':') {
						g_compiler->RdLex();
						if (Lexem == '-') {
							g_compiler->RdLex();
							w->M = -g_compiler->RdInteger();
						}
						else {
							w->M = g_compiler->RdInteger();
						}
					}
				}
			}
		}

		if (Lexem == ',') {
			g_compiler->RdLex();
			if ((OpKind == WriteType::message) && g_compiler->IsOpt("HELP")) {
				z = g_compiler->RdStrFrml(nullptr);
			}
			else {
				w = new WrLnD();
				//if (d == nullptr) d = w;
				//else ChainLast(d, w);
				d.push_back(w);
				//goto label1;
				continue;
			}
		}

		break;
	} // while

	WORD N = 1 + sizeof(WrLnD);
	if (z != nullptr) {
		OpKind = WriteType::msgAndHelp;
		N += 8;
	}

	Instr_writeln* pd = new Instr_writeln();
	pd->LF = OpKind;
	pd->WD = d;
	if (OpKind == WriteType::msgAndHelp) {
		pd->mHlpRdb = CRdb;
		pd->mHlpFrml = z;
	}

	*pinstr = pd;
}

Instr* RdReleaseDrive()
{
	auto PD = new Instr_releasedrive(); // GetPD(_releasedrive, 4);
	g_compiler->RdLex();
	PD->Drive = g_compiler->RdStrFrml(nullptr);
	return PD;
}

Instr* RdIndexfile()
{
	auto PD = new Instr_indexfile(); // GetPD(_indexfile, 5);
	g_compiler->RdLex();
	PD->IndexFD = g_compiler->RdFileName();
	if (PD->IndexFD->FF->file_type != FileType::INDEX) g_compiler->OldError(108);
	if (Lexem == ',') {
		g_compiler->RdLex();
		g_compiler->AcceptKeyWord("COMPRESS");
		PD->Compress = true;
	}
	return PD;
}

Instr* RdGetIndex()
{
	LocVar* lv2 = nullptr; bool b = false; LinkD* ld = nullptr;
	auto PD = new Instr_getindex(); // GetPD(_getindex, 31);
	g_compiler->RdLex();
	LocVar* lv = RdIdxVar();
	PD->loc_var1 = lv; g_compiler->Accept(',');
	PD->mode = ' ';
	if (Lexem == '+' || Lexem == '-') {
		PD->mode = Lexem;
		g_compiler->RdLex();
		g_compiler->Accept(',');
		PD->condition = g_compiler->RdRealFrml(nullptr); /*RecNr*/
		return PD;
	}
	g_compiler->processing_F = g_compiler->RdFileName();
	if (lv->FD != g_compiler->processing_F) g_compiler->OldError(164);
	CViewKey = g_compiler->RdViewKey(lv->FD);
	PD->keys = CViewKey;
	while (Lexem == ',') {
		g_compiler->RdLex();
		if (g_compiler->IsOpt("SORT")) {
			if (!((XWKey*)lv->record)->KFlds.empty()) {
				g_compiler->OldError(175);
			}
			g_compiler->Accept('(');
			g_compiler->RdKFList(PD->key_fields, g_compiler->processing_F);
			g_compiler->Accept(')');
		}
		else if (g_compiler->IsOpt("COND")) {
			g_compiler->Accept('(');
			PD->condition = g_compiler->RdKeyInBool(PD->key_in_root, false, true, PD->sql_filter, nullptr);
			g_compiler->Accept(')');
		}
		else if (g_compiler->IsOpt("OWNER")) {
			PD->owner_type = RdOwner(g_compiler->processing_F, &PD->link, &PD->loc_var2);
			XKey* k = GetFromKey(PD->link);
			if (CViewKey == nullptr) PD->keys = k;
			else if (CViewKey != k) g_compiler->OldError(178);
		}
		else g_compiler->Error(167);
		if ((PD->owner_type != 0) && (PD->sql_filter || !PD->key_in_root.empty())) {
			g_compiler->Error(179);
		}
	}
	return PD;
}

Instr* RdGotoXY()
{
	auto PD = new Instr_gotoxy(); // GetPD(_gotoxy, 8);
	g_compiler->RdLex();
	PD->GoX = g_compiler->RdRealFrml(nullptr);
	g_compiler->Accept(',');
	PD->GoY = g_compiler->RdRealFrml(nullptr);
	return PD;
}

Instr* RdClrWw()
{
	auto PD = new Instr_clrww(); // GetPD(_clrww, 24);
	g_compiler->RdLex();
	g_compiler->RdW(PD->W2);
	if (Lexem == ',') {
		g_compiler->RdLex();
		if (Lexem != ',') PD->Attr2 = g_compiler->RdAttr();
		if (Lexem == ',') { g_compiler->RdLex(); PD->FillC = g_compiler->RdStrFrml(nullptr); }
	}
	return PD;
}

Instr* RdMount()
{
	auto PD = new Instr_mount(); // GetPD(_mount, 3);
	g_compiler->RdLex();
	int i = 0;
	g_compiler->TestIdentif();
	FileD* FD = g_compiler->FindFileD();
	if (FD == nullptr) {
		i = catalog->GetCatalogIRec(LexWord, true);
	}
	else {
		i = FD->CatIRec;
	}
	TestCatError(i, LexWord, false);
	g_compiler->RdLex();
	PD->MountCatIRec = i;
	if (Lexem == ',') {
		g_compiler->RdLex();
		g_compiler->AcceptKeyWord("NOCANCEL");
		PD->MountNoCancel = true;
	}
	return PD;
}

Instr* RdDisplay()
{
	auto PD = new Instr_merge_display(PInstrCode::_display); // GetPD(_display, sizeof(RdbPos));
	g_compiler->RdLex();
	pstring* s = nullptr;
	if ((Lexem == _identifier) && g_compiler->FindChpt('H', LexWord, false, &PD->Pos)) {
		g_compiler->RdLex();
	}
	else {
		PD->Pos.rdb = (RdbD*)g_compiler->RdStrFrml(nullptr);
		PD->Pos.i_rec = 0;
	}
	return PD;
}

Instr_graph* RdGraphP()
{
	FrmlElem* FrmlArr[15];
	WORD i;

	pstring Nm1[11] = { "TYPE", "HEAD", "HEADX", "HEADY", "HEADZ", "FILL", "DIRX", "GRID", "PRINT", "PALETTE", "ASSIGN" };
	pstring Nm2[6] = { "WIDTH", "RECNO", "NRECS", "MAX", "MIN", "GRPOLY" };

	Instr_graph* PD = new Instr_graph();
	g_compiler->RdLex();
	PD->GD = new GraphD();

	GraphD* PDGD = PD->GD;
	if (g_compiler->IsOpt("GF")) PDGD->GF = g_compiler->RdStrFrml(nullptr);
	else {
		PDGD->FD = g_compiler->RdFileName();
		CFile = PDGD->FD;
		CViewKey = g_compiler->RdViewKey(PDGD->FD);
		PDGD->ViewKey = CViewKey;
		g_compiler->Accept(',');
		g_compiler->Accept('(');
		PDGD->X = g_compiler->RdFldName(PDGD->FD);
		i = 0;
		do {
			g_compiler->Accept(',');
			PDGD->ZA[i] = g_compiler->RdFldName(PDGD->FD);
			i++;
		} while (!((i > 9) || (Lexem != ',')));
		g_compiler->Accept(')');
	}
	while (Lexem == ',') {
		g_compiler->RdLex();
		for (i = 0; i < 11; i++) {
			if (g_compiler->IsOpt(Nm1[i])) {
				FrmlArr[0] = (FrmlElem*)(&PDGD->T);
				FrmlArr[i] = g_compiler->RdStrFrml(nullptr);
				goto label1;
			}
		}
		for (i = 0; i < 6; i++) {
			if (g_compiler->IsOpt(Nm2[i])) {
				FrmlArr[0] = (FrmlElem*)(&PDGD->S);
				FrmlArr[i] = g_compiler->RdRealFrml(nullptr);
				goto label1;
			}
		}
		if (g_compiler->IsDigitOpt("HEADZ", i)) PDGD->HZA[i] = g_compiler->RdStrFrml(nullptr);
		else if (g_compiler->IsKeyWord("INTERACT")) PDGD->Interact = true;
		else if (g_compiler->IsOpt("COND")) {
			if (Lexem == '(') {
				g_compiler->RdLex();
				PDGD->Cond = g_compiler->RdKeyInBool(PDGD->KeyIn, false, true, PDGD->SQLFilter, nullptr);
				g_compiler->Accept(')');
			}
			else PDGD->Cond = g_compiler->RdKeyInBool(PDGD->KeyIn, false, true, PDGD->SQLFilter, nullptr);
		}
		else if (g_compiler->IsOpt("TXT")) {
			GraphVD* VD = new GraphVD();
			//ChainLast(PDGD->V, VD);
			PDGD->V.push_back(VD);
			g_compiler->Accept('(');
			VD->XZ = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
			VD->YZ = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
			VD->Velikost = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
			VD->BarPis = g_compiler->RdStrFrml(nullptr); g_compiler->Accept(',');
			VD->Text = g_compiler->RdStrFrml(nullptr); g_compiler->Accept(')');
		}
		else if (g_compiler->IsOpt("TXTWIN")) {
			GraphWD* WD = new GraphWD();
			//ChainLast(PDGD->W, WD);
			PDGD->W.push_back(WD);
			g_compiler->Accept('(');
			WD->XZ = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
			WD->YZ = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
			WD->XK = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
			WD->YK = g_compiler->RdRealFrml(nullptr); g_compiler->Accept(',');
			WD->BarPoz = g_compiler->RdStrFrml(nullptr); g_compiler->Accept(',');
			WD->BarPis = g_compiler->RdStrFrml(nullptr); g_compiler->Accept(',');
			WD->Text = g_compiler->RdStrFrml(nullptr); g_compiler->Accept(')');
		}
		else if (g_compiler->IsOpt("RGB")) {
			GraphRGBD* RGBD = new GraphRGBD();
			//ChainLast(PDGD->RGB, RGBD);
			PDGD->RGB.push_back(RGBD);
			g_compiler->Accept('(');
			RGBD->Barva = g_compiler->RdStrFrml(nullptr);
			g_compiler->Accept(',');
			RGBD->R = g_compiler->RdRealFrml(nullptr);
			g_compiler->Accept(',');
			RGBD->G = g_compiler->RdRealFrml(nullptr);
			g_compiler->Accept(',');
			RGBD->B = g_compiler->RdRealFrml(nullptr);
			g_compiler->Accept(')');
		}
		else if (g_compiler->IsOpt("WW")) {
			WinG* Ww = new WinG();
			g_compiler->Accept('(');
			if (Lexem == '(') { g_compiler->RdLex(); Ww->WFlags = WNoPop; }
			g_compiler->RdW(Ww->W);
			g_compiler->RdFrame(Ww->Top, Ww->WFlags);
			if (Lexem == ',') {
				g_compiler->RdLex();
				Ww->ColBack = g_compiler->RdStrFrml(nullptr); g_compiler->Accept(',');
				Ww->ColFor = g_compiler->RdStrFrml(nullptr); g_compiler->Accept(',');
				Ww->ColFrame = g_compiler->RdStrFrml(nullptr);
			}
			g_compiler->Accept(')');
			if ((Ww->WFlags & WNoPop) != 0) g_compiler->Accept(')');
		}
		else {
			g_compiler->Error(44);
		}
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
	if ((Op == PInstrCode::_appendRec) || (Op == PInstrCode::_recallrec)) {
		// PD = GetPD(oper, 9);
		PD = new Instr_recs(Op);
		g_compiler->RdLex();
		CFile = g_compiler->RdFileName();
		PD->RecFD = CFile;
#ifdef FandSQL
		if (CFile->typSQLFile) OldError(155);
#endif
		if (Op == PInstrCode::_recallrec) {
			g_compiler->Accept(',');
			PD->RecNr = g_compiler->RdRealFrml(nullptr);
		}
	}
	else {
		// PD = GetPD(oper, 15);
		PD = new Instr_recs(Op);
		g_compiler->RdLex();
		if (Op == PInstrCode::_deleterec) {
			CFile = g_compiler->RdFileName();
			PD->RecFD = CFile;
		}
		else { /*_readrec,_writerec*/
			if (!IsRecVar(&PD->LV)) g_compiler->Error(141);
			CFile = PD->LV->FD;
		}
		XKey* K = g_compiler->RdViewKey(CFile);
		g_compiler->Accept(',');
#ifdef FandSQL
		if (CFile->typSQLFile
			&& (Lexem == _equ || Lexem == _le || Lexem == _gt || Lexem == _lt || Lexem == _ge))
		{
			PD->CompOp = Lexem; RdLex();
	}
#endif
		Z = g_compiler->RdFrml(FTyp, nullptr);
		PD->RecNr = Z;
		switch (FTyp) {
		case 'B': g_compiler->OldError(12); break;
		case 'S': {
			PD->ByKey = true;
			if (PD->CompOp == 0) PD->CompOp = _equ;
			if (K == nullptr) K = CFile->Keys.empty() ? nullptr : CFile->Keys[0];
			PD->Key = K;
			if ((K == nullptr) && (!CFile->IsParFile || (Z->Op != _const)
				|| (((FrmlElemString*)Z)->S.length() > 0))) g_compiler->OldError(24);
			break;
		}
#ifdef FandSQL
		default: {
			if (PD->CompOp != 0) OldError(19);
			if (CFile->typSQLFile && ((oper == _deleterec) || (Z->oper != _const)
				|| (Z->rdb != 0))) Error(155);
			break;
		}
#endif
		}
}
	if ((Lexem == ',') && (Op == PInstrCode::_writerec || Op == PInstrCode::_deleterec || Op == PInstrCode::_recallrec)) {
		g_compiler->RdLex();
		g_compiler->Accept('+');
		PD->AdUpd = true;
	}
	CFile = cf;
	return PD;
}

Instr* RdLinkRec()
{
	LocVar* LV = nullptr;
	LinkD* LD = nullptr;
	auto PD = new Instr_assign(PInstrCode::_linkrec); // GetPD(_linkrec, 12);
	g_compiler->RdLex();
	if (!IsRecVar(&PD->RecLV1)) g_compiler->Error(141);
	g_compiler->Accept(',');
	//CFile = PD->RecLV1->FD;
	if (IsRecVar(&LV)) {
		LD = g_compiler->FindLD(PD->RecLV1->FD, LV->FD->Name);
		if (LD == nullptr) g_compiler->OldError(154);
	}
	else {
		g_compiler->TestIdentif();
		LD = g_compiler->FindLD(PD->RecLV1->FD, LexWord);
		if (LD == nullptr) g_compiler->Error(9);
		g_compiler->RdLex();
		g_compiler->Accept('(');
		LV = RdRecVar();
		if (LD->ToFD != LV->FD) g_compiler->OldError(141);
		g_compiler->Accept(')');
	}
	PD->RecLV2 = LV;
	PD->LinkLD = LD;
	return PD;
}

Instr* RdSetEditTxt()
{
	auto PD = new Instr_setedittxt();
	g_compiler->RdLex();
label1:
	if (g_compiler->IsOpt("OVERWR")) PD->Insert = g_compiler->RdBool(nullptr);
	else if (g_compiler->IsOpt("INDENT")) PD->Indent = g_compiler->RdBool(nullptr);
	else if (g_compiler->IsOpt("WRAP")) PD->Wrap = g_compiler->RdBool(nullptr);
	else if (g_compiler->IsOpt("ALIGN")) PD->Just = g_compiler->RdBool(nullptr);
	else if (g_compiler->IsOpt("COLBLK")) PD->ColBlk = g_compiler->RdBool(nullptr);
	else if (g_compiler->IsOpt("LEFT")) PD->Left = g_compiler->RdRealFrml(nullptr);
	else if (g_compiler->IsOpt("RIGHT")) PD->Right = g_compiler->RdRealFrml(nullptr);
	else g_compiler->Error(160);
	if (Lexem == ',') { g_compiler->RdLex(); goto label1; }
	return PD;
}

FrmlElem* AdjustComma(FrmlElem* Z1, FieldDescr* F, instr_type Op)
{
	FrmlElemFunction* Z = nullptr;
	FrmlElemNumber* Z2 = nullptr;
	auto result = Z1;
	if (F->field_type != FieldType::FIXED) return result;
	if ((F->Flg & f_Comma) == 0) return result;
	Z2 = new FrmlElemNumber(_const, 0, Power10[F->M]); // GetOp(_const, sizeof(double));
	//Z2->rdb = Power10[F->M];
	Z = new FrmlElemFunction(Op, 0); // GetOp(oper, 0);
	Z->P1 = Z1;
	Z->P2 = Z2;
	result = Z;
	return result;
}

std::vector<AssignD*> MakeImplAssign(FileD* FD1, FileD* FD2)
{
	std::vector<AssignD*> ARoot;
	char FTyp;
	pstring S = LexWord;
	for (FieldDescr* F1 : FD1->FldD) {
		if ((F1->Flg & f_Stored) != 0) {
			LexWord = F1->Name;
			FieldDescr* F2 = g_compiler->FindFldName(FD2);
			if (F2 != nullptr) {
				AssignD* A = new AssignD();
				//if (ARoot == nullptr) ARoot = A;
				//else ChainLast(ARoot, A);
				ARoot.push_back(A);
				if ((F2->frml_type != F1->frml_type)
					|| (F1->frml_type == 'R')
					&& (F1->field_type != F2->field_type)) {
					A->Kind = MInstrCode::_zero;
					A->outputFldD = F1;
				}
				else {
					A->Kind = MInstrCode::_output;
					A->OFldD = F1;
					FrmlElem* Z = g_compiler->MakeFldFrml(F2, FTyp);
					Z = AdjustComma(Z, F2, _divide);
					A->Frml = g_compiler->FrmlContxt(AdjustComma(Z, F1, _times), FD2, nullptr);
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
		if (g_compiler->FindLocVar(&LVBD, &LV) && (LV->f_typ == 'r' || LV->f_typ == 'i')) {
			FTyp = LV->f_typ;
			g_compiler->RdLex(); g_compiler->RdLex();
			if (FTyp == 'i') {
				g_compiler->AcceptKeyWord("NRECS");
				g_compiler->Accept(_assign);
				if ((Lexem != _number) || (LexWord != "0")) g_compiler->Error(183);
				g_compiler->RdLex();
				PD = new Instr_assign(PInstrCode::_asgnxnrecs); // GetPInstr(_asgnxnrecs, 4);
				PD->xnrIdx = (XWKey*)LV->record;
			}
			else {
				PD = new Instr_assign(PInstrCode::_asgnrecfld); // GetPInstr(_asgnrecfld, 13);
				PD->AssLV = LV;
				F = g_compiler->RdFldName(LV->FD);
				PD->RecFldD = F;
				if ((F->Flg & f_Stored) == 0) g_compiler->OldError(14);
				FTyp = F->frml_type;
			label0:
				g_compiler->RdAssignFrml(FTyp, PD->Add, &PD->Frml, nullptr);
			}
		}
		else {
			FName = LexWord;
			FD = g_compiler->FindFileD();
			if (FD->IsActiveRdb()) g_compiler->Error(121);
			g_compiler->RdLex(); g_compiler->RdLex();
			if (g_compiler->IsKeyWord("ARCHIVES")) {
				F = catalog->CatalogArchiveField();
				goto label1;
			}
			if (g_compiler->IsKeyWord("PATH")) {
				F = catalog->CatalogPathNameField();
				goto label1;
			}
			if (g_compiler->IsKeyWord("VOLUME")) {
				F = catalog->CatalogVolumeField();
			label1:
				PD = new Instr_assign(PInstrCode::_asgnCatField);
				PD->FD3 = FD;
				PD->CatIRec = catalog->GetCatalogIRec(FName, true);
				PD->CatFld = F;
				TestCatError(PD->CatIRec, FName, true);
				g_compiler->Accept(_assign);
				PD->Frml3 = g_compiler->RdStrFrml(nullptr);
			}
			else if (FD == nullptr) g_compiler->OldError(9);
			else if (g_compiler->IsKeyWord("NRECS")) {
				if (FD->FF->file_type == FileType::RDB) { g_compiler->OldError(127); }
				PD = new Instr_assign(PInstrCode::_asgnnrecs);
				PD->FD = FD;
				FTyp = 'R';
				goto label0;
			}
			else {
				if (!FD->IsParFile) g_compiler->OldError(64);
				PD = new Instr_assign(PInstrCode::_asgnpar); // GetPInstr(_asgnpar, 13);
				PD->FD = FD;
				F = g_compiler->RdFldName(FD);
				PD->FldD = F;
				if ((F->Flg & f_Stored) == 0) g_compiler->OldError(14);
				FTyp = F->frml_type;
				goto label0;
			}
		}
	else if (ForwChar == '[') {
		PD = new Instr_assign(PInstrCode::_asgnField); // GetPInstr(_asgnField, 18);
		FD = g_compiler->RdFileName();
		PD->FD = FD; g_compiler->RdLex();
#ifdef FandSQL
		if (v_files->typSQLFile) OldError(155);
#endif
		PD->RecFrml = g_compiler->RdRealFrml(nullptr);
		g_compiler->Accept(']');
		g_compiler->Accept('.');
		F = g_compiler->RdFldName(FD);
		PD->FldD = F;
		if ((F->Flg & f_Stored) == 0) g_compiler->OldError(14);
		PD->Indexarg = (FD->FF->file_type == FileType::INDEX) && g_compiler->IsKeyArg(F, FD);
		g_compiler->RdAssignFrml(F->frml_type, PD->Add, &PD->Frml, nullptr);
	}
	else if (g_compiler->FindLocVar(&LVBD, &LV)) {
		g_compiler->RdLex();
		FTyp = LV->f_typ;
		switch (FTyp) {
		case 'f':
		case 'i': g_compiler->OldError(140); break;
		case 'r': {
			g_compiler->Accept(_assign);
			if (!IsRecVar(&LV2)) g_compiler->Error(141);
			PD = new Instr_assign(PInstrCode::_asgnrecvar); // GetPInstr(_asgnrecvar, 12);
			PD->RecLV1 = LV;
			PD->RecLV2 = LV2;
			PD->Ass = MakeImplAssign(LV->FD, LV2->FD);
			break;
		}
		default: {
			PD = new Instr_assign(PInstrCode::_asgnloc); // GetPInstr(_asgnloc, 9);
			PD->AssLV = LV; goto label0;
			break;
		}
		}
	}
	else if (g_compiler->IsKeyWord("USERNAME"))
	{
		PD = new Instr_assign(PInstrCode::_asgnusername); // GetPInstr(_asgnusername, 4);
		goto label2;
	}
	else if (g_compiler->IsKeyWord("CLIPBD"))
	{
		PD = new Instr_assign(PInstrCode::_asgnClipbd); // GetPInstr(_asgnClipbd, 4);
		goto label2;
	}
	else if (g_compiler->IsKeyWord("ACCRIGHT")) {
		PD = new Instr_assign(PInstrCode::_asgnAccRight); // GetPInstr(_asgnAccRight, 4);
	label2:
		g_compiler->Accept(_assign);
		PD->Frml = g_compiler->RdStrFrml(nullptr);
	}
	else if (g_compiler->IsKeyWord("EDOK")) {
		PD = new Instr_assign(PInstrCode::_asgnEdOk); // GetPInstr(_asgnEdOk, 4);
		g_compiler->Accept(_assign);
		PD->Frml = g_compiler->RdBool(nullptr);
	}
	else if (g_compiler->IsKeyWord("RANDSEED"))
	{
		PD = new Instr_assign(PInstrCode::_asgnrand); // GetPInstr(_asgnrand, 4);
		goto label3;
	}
	else if (g_compiler->IsKeyWord("TODAY"))
	{
		PD = new Instr_assign(PInstrCode::_asgnusertoday); // GetPInstr(_asgnusertoday, 4);
		goto label3;
	}
	else if (g_compiler->IsKeyWord("USERCODE")) {
		PD = new Instr_assign(PInstrCode::_asgnusercode); // GetPInstr(_asgnusercode, 4);
	label3:
		g_compiler->Accept(_assign);
		PD->Frml = g_compiler->RdRealFrml(nullptr);
	}
	else {
		g_compiler->RdLex();
		if (Lexem == _assign) g_compiler->OldError(8);
		else g_compiler->OldError(34);
	}
	return PD;
}

Instr* RdWith()
{
	Instr* P = nullptr;
	PInstrCode Op;

	if (g_compiler->IsKeyWord("WINDOW")) {
		P = new Instr_window(); //GetPInstr(_window, 29);
		auto iP = (Instr_window*)P;
		g_compiler->Accept('(');
		if (Lexem == '(') {
			g_compiler->RdLex();
			iP->WithWFlags = WNoPop;
		}
		g_compiler->RdW(iP->W);
		g_compiler->RdFrame(&iP->Top, iP->WithWFlags);
		if (Lexem == ',') {
			g_compiler->RdLex();
			iP->Attr = g_compiler->RdAttr();
		}
		g_compiler->Accept(')');
		if ((iP->WithWFlags & WNoPop) != 0) g_compiler->Accept(')');
		g_compiler->AcceptKeyWord("DO");
		iP->v_ww_instr = RdPInstr();
	}
	else if (g_compiler->IsKeyWord("SHARED")) {
		Op = PInstrCode::_withshared;
		goto label1;
	}
	else if (g_compiler->IsKeyWord("LOCKED")) {
		Op = PInstrCode::_withlocked;
	label1:
		P = new Instr_withshared(Op);
		Instr_withshared* iP = (Instr_withshared*)P;

		while (true) {
			LockD* ld = new LockD();
			iP->WLD.push_back(ld);

			ld->FD = g_compiler->RdFileName();
			if (Op == PInstrCode::_withlocked) {
				g_compiler->Accept('[');
				ld->Frml = g_compiler->RdRealFrml(nullptr);
				g_compiler->Accept(']');
			}
			else {
				g_compiler->Accept('(');
				for (LockMode i = NoExclMode; i <= ExclMode; i = (LockMode)(i + 1)) {
					if (g_compiler->IsKeyWord(LockModeTxt[i])) {
						ld->Md = i;
						goto label3;
					}
				}
				g_compiler->Error(100);
			label3:
				g_compiler->Accept(')');
			}
			if (Lexem == ',') {
				g_compiler->RdLex();
				continue;
			}
			break;
		}
		g_compiler->AcceptKeyWord("DO");
		iP->WDoInstr = RdPInstr();
		if (g_compiler->IsKeyWord("ELSE")) {
			iP->WasElse = true;
			iP->WElseInstr = RdPInstr();
		}
	}
	else if (g_compiler->IsKeyWord("GRAPHICS")) {
		P = new Instr_withshared(PInstrCode::_withgraphics);
		g_compiler->AcceptKeyWord("DO");
		((Instr_withshared*)P)->WDoInstr = RdPInstr();
	}
	else {
		g_compiler->Error(131);
	}
	return P;
}

Instr_assign* RdUserFuncAssign()
{
	LocVar* lv = nullptr;
	if (!g_compiler->FindLocVar(&LVBD, &lv)) {
		g_compiler->Error(34);
	}
	g_compiler->RdLex();
	Instr_assign* pd = new Instr_assign(PInstrCode::_asgnloc);
	pd->AssLV = lv;
	g_compiler->RdAssignFrml(lv->f_typ, pd->Add, &pd->Frml, nullptr);
	return pd;
}

std::vector<Instr*> RdPInstr()
{
	Instr* single_instr = nullptr;
	std::vector<Instr*> result;

	if (g_compiler->IsKeyWord("IF")) single_instr = RdIfThenElse();
	else if (g_compiler->IsKeyWord("WHILE")) single_instr = RdWhileDo();
	else if (g_compiler->IsKeyWord("REPEAT")) single_instr = RdRepeatUntil();
	else if (g_compiler->IsKeyWord("CASE")) single_instr = RdCase();
	else if (g_compiler->IsKeyWord("FOR")) { result = RdFor(); }			// creates vector of instructions
	else if (g_compiler->IsKeyWord("BEGIN")) { result = RdBeginEnd(); }	// creates vector of instructions
	else if (g_compiler->IsKeyWord("BREAK")) single_instr = new Instr(PInstrCode::_break);
	else if (g_compiler->IsKeyWord("EXIT")) single_instr = new Instr(PInstrCode::_exitP);
	else if (g_compiler->IsKeyWord("CANCEL")) single_instr = new Instr(PInstrCode::_cancel);
	else if (Lexem == ';') single_instr = nullptr;
	else if (IsRdUserFunc) single_instr = RdUserFuncAssign();
	else if (g_compiler->IsKeyWord("MENULOOP")) single_instr = RdMenuBox(true);
	else if (g_compiler->IsKeyWord("MENU")) single_instr = RdMenuBox(false);
	else if (g_compiler->IsKeyWord("MENUBAR")) single_instr = RdMenuBar();
	else if (g_compiler->IsKeyWord("WITH")) single_instr = RdWith();
	else if (g_compiler->IsKeyWord("SAVE")) single_instr = new Instr(PInstrCode::_save);
	else if (g_compiler->IsKeyWord("CLREOL")) single_instr = new Instr(PInstrCode::_clreol);
	else if (g_compiler->IsKeyWord("FORALL")) single_instr = RdForAll();
	else if (g_compiler->IsKeyWord("CLEARKEYBUF")) single_instr = new Instr(PInstrCode::_clearkeybuf);
	else if (g_compiler->IsKeyWord("WAIT")) single_instr = new Instr(PInstrCode::_wait);
	else if (g_compiler->IsKeyWord("BEEP")) single_instr = new Instr(PInstrCode::_beepP);
	else if (g_compiler->IsKeyWord("NOSOUND")) single_instr = new Instr(PInstrCode::_nosound);
#ifndef FandRunV
	else if (g_compiler->IsKeyWord("MEMDIAG")) single_instr = new Instr(PInstrCode::_memdiag);
#endif 
	else if (g_compiler->IsKeyWord("RESETCATALOG")) single_instr = new Instr(PInstrCode::_resetcat);
	else if (g_compiler->IsKeyWord("RANDOMIZE")) single_instr = new Instr(PInstrCode::_randomize);
	else if (Lexem == _identifier) {
		g_compiler->SkipBlank(false);
		if (ForwChar == '(') {
			RdProcCall(&single_instr); // funkce muze ovlivnit single_instruction
		}
		else if (g_compiler->IsKeyWord("CLRSCR")) single_instr = new Instr(PInstrCode::_clrscr);
		else if (g_compiler->IsKeyWord("GRAPH")) single_instr = new Instr_graph();
		else if (g_compiler->IsKeyWord("CLOSE")) single_instr = new Instr_closefds();
		else single_instr = RdAssign();
	}
	else g_compiler->Error(34);

	if (result.empty() && single_instr != nullptr) {
		result.push_back(single_instr);
	}

	return result;
}

void ReadProcHead(const std::string& name)
{
	ResetCompilePars();
	g_compiler->rdFldNameType = FieldNameType::P;
	//ptrRdFldNameFrml = RdFldNameFrmlP;
	RdFunction = RdFunctionP;
	FileVarsAllowed = false;
	IdxLocVarAllowed = true;
	IsRdUserFunc = false;
	g_compiler->RdLex();
	ResetLVBD();
	LVBD.FceName = name;
	if (Lexem == '(') {
		g_compiler->RdLex();
		g_compiler->RdLocDcl(&LVBD, true, true, 'P');
		g_compiler->Accept(')');
	}
	if (g_compiler->IsKeyWord("VAR")) {
		g_compiler->RdLocDcl(&LVBD, false, true, 'P');
	}
}

std::vector<Instr*> ReadProcBody()
{
	g_compiler->AcceptKeyWord("BEGIN");
	std::vector<Instr*> result = RdBeginEnd();
	g_compiler->Accept(';');
	if (Lexem != 0x1A) {
		std::string error40 = g_compiler->Error(40);
		std::string err_msg = "ReadProcBody exception: " + error40;
		throw std::exception(err_msg.c_str());
	}
	return result;
}

// metoda nacita funkce a procedury z InpArrPtr a postupne je zpracovava
// nacte nazev, parametry, navr. hodnotu, promenne, konstanty i kod
void ReadDeclChpt()
{
	char typ = '\0';
	g_compiler->RdLex();
	while (true) {
		if (g_compiler->IsKeyWord("FUNCTION")) {
			g_compiler->TestIdentif();

			//FuncD* fc = FuncDRoot;
			//while (fc != CRdb->OldFCRoot) {
			//	if (EquUpCase(fc->name, LexWord)) {
			//		g_compiler->Error(26);
			//	}
			//	fc = fc->Chain;
			//}

			int32_t items = FuncDRoot.size() - CRdb->OldFCRoot.size();
			std::deque<FuncD*>::iterator it0 = FuncDRoot.begin();
			for (int32_t i = 0; i < items; i++) {
				if (EquUpCase((*it0)->name, LexWord)) {
					g_compiler->Error(26);
				}
				++it0;
			}

			FuncD* fc = new FuncD();
			//fc->Chain = FuncDRoot;
			//FuncDRoot = fc;
			FuncDRoot.push_front(fc);
			fc->name = LexWord;
			g_compiler->rdFldNameType = FieldNameType::P;
			//ptrRdFldNameFrml = RdFldNameFrmlP;
			RdFunction = RdFunctionP;
			//ptrChainSumEl = nullptr;
			FileVarsAllowed = false;
			IsRdUserFunc = true;
			g_compiler->RdLex();
			ResetLVBD();
			LVBD.FceName = fc->name;
			g_compiler->Accept('(');
			if (Lexem != ')') {
				g_compiler->RdLocDcl(&LVBD, true, false, 'D'); // nacte parametry funkce
			}
			g_compiler->Accept(')');
			g_compiler->Accept(':');
			// nacte typ navratove hodnoty
			if (g_compiler->IsKeyWord("REAL")) {
				typ = 'R';
			}
			else if (g_compiler->IsKeyWord("STRING")) {
				typ = 'S';
			}
			else if (g_compiler->IsKeyWord("BOOLEAN")) {
				typ = 'B';
			}
			else {
				g_compiler->Error(39);
			}
			LocVar* lv = new LocVar();
			LVBD.vLocVar.push_back(lv);
			lv->name = fc->name;
			lv->is_return_value = true;
			lv->f_typ = typ;
			lv->oper = _getlocvar;
			fc->FTyp = typ;
			g_compiler->Accept(';');
			// nacte promenne
			if (g_compiler->IsKeyWord("VAR")) {
				g_compiler->RdLocDcl(&LVBD, false, false, 'D');
			}
			fc->LVB = LVBD;
			// nacte kod funkce (procedury)
			g_compiler->AcceptKeyWord("BEGIN");
			fc->v_instr = RdBeginEnd();
			g_compiler->Accept(';');
		}
		else if (Lexem == 0x1A) {
			return;
		}
		else {
			g_compiler->Error(40);
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
	std::string s = RunString(CFile, X->EvalP1, CRecPtr);
	if (s.empty()) {
		LastExitCode = 0;
	}
	else {
		LastExitCode = 1;
		p = g_compiler->SaveCompState();
		ResetCompilePars();
		g_compiler->rdFldNameType = FieldNameType::P;
		//ptrRdFldNameFrml = RdFldNameFrmlP;
		RdFunction = RdFunctionP;

		if (X->EvalFD == nullptr) {
			FileVarsAllowed = false;
		}
		else {
			CFile = X->EvalFD;
			FileVarsAllowed = true;
		}

		try {
			g_compiler->SetInpStdStr(s, false);
			g_compiler->RdLex();
			z = g_compiler->RdFrml(fTyp, nullptr);
			if ((fTyp != X->EvalTyp) || (Lexem != 0x1A)) z = nullptr;
			else LastExitCode = 0;
		}
		catch (const std::exception& e) {
			// std::string err_msg = "GetEvalFrml exception: " + std::string(e.what());
			// throw std::exception(err_msg.c_str());
		}

		cpos = CurrPos;

		g_compiler->RestoreCompState(p);
		if (LastExitCode != 0) {
			LastTxtPos = cpos;
			if (X->EvalTyp == 'B') {
				z = new FrmlElemBool(_const, 0, false);
				// z->B = false;
			}
		}
	}

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
		PD = new Instr_backup(PInstrCode::_backupm);
	}
	else {
		PD = new Instr_backup(PInstrCode::_backup);
	}

	g_compiler->RdLex();
	PD->IsBackup = IsBackup;
	g_compiler->TestIdentif();

	bool found = false;
	for (int i = 1; i <= catalog->GetCatalogFile()->FF->NRecs; i++) {
		if (EquUpCase(catalog->GetRdbName(i), "ARCHIVES") && EquUpCase(catalog->GetFileName(i), LexWord)) {
			g_compiler->RdLex();
			PD->BrCatIRec = i;
			found = true;
		}
	}

	if (!found) {
		g_compiler->Error(88);
		return nullptr;
	}
	else {
		if (MTyp == 'M') {
			g_compiler->Accept(',');
			PD->bmDir = g_compiler->RdStrFrml(nullptr);
			if (IsBackup) {
				g_compiler->Accept(',');
				PD->bmMasks = g_compiler->RdStrFrml(nullptr);
			}
		}
		while (Lexem == ',') {
			g_compiler->RdLex();
			if (MTyp == 'M') {
				if (!IsBackup && g_compiler->IsKeyWord("OVERWRITE")) {
					PD->bmOverwr = true;
					continue;
				}
				if (g_compiler->IsKeyWord("SUBDIR")) {
					PD->bmSubDir = true;
					continue;
				}
			}
			if (g_compiler->IsKeyWord("NOCOMPRESS"))
			{
				PD->NoCompress = true;
			}
			else {
				g_compiler->AcceptKeyWord("NOCANCEL");
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
	g_compiler->RdLex();
	g_compiler->RdChptName('L', &pd->lpPos, true);
	if (Lexem == ',') {
		g_compiler->RdLex();
		g_compiler->TestIdentif();
		pd->lpName = LexWord;
		g_compiler->RdLex();
	}
	return pd;
}
