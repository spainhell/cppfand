#include "rdproc.h"
#include "compile.h"
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

void TestCatError(int i, const std::string& name, bool old)
{
	if (i == 0) {
		SetMsgPar(name);
		if (old) {
			compiler->OldError(96);
		}
		else {
			compiler->Error(96);
		}
	}
}

bool IsRecVar(LocVar** LV)
{
	if (!compiler->FindLocVar(&LVBD, LV) || ((*LV)->FTyp != 'r')) return false;
	compiler->RdLex();
	return true;
}

LocVar* RdRecVar()
{
	LocVar* LV = nullptr;
	if (!IsRecVar(&LV)) compiler->Error(141);
	return LV;
}

LocVar* RdIdxVar()
{
	LocVar* lv = nullptr;
	if (!compiler->FindLocVar(&LVBD, &lv) || (lv->FTyp != 'i')) compiler->Error(165);
	auto result = lv;
	compiler->RdLex();
	return result;
}

FrmlElem* RdRecVarFldFrml(LocVar* LV, char& FTyp)
{
	FrmlElem* Z = nullptr;
	compiler->Accept('.');
	switch (LV->FTyp) {
	case 'r': {
		auto Z = new FrmlElem7(_recvarfld, 12);
		FileD* cf = CFile;
		CFile = LV->FD;
		Z->File2 = CFile;
		Z->LD = (LinkD*)LV->record;
		bool fa = FileVarsAllowed;
		FileVarsAllowed = true;
		Z->P011 = compiler->RdFldNameFrmlF(FTyp, nullptr);
		FileVarsAllowed = fa;
		CFile = cf;
		return Z;
		break;
	}
	case 'i': {
		auto Z = new FrmlElem22(_indexnrecs, 4);
		Z->WKey = (XWKey*)LV->record;
		pstring nrecs = "nrecs";
		compiler->AcceptKeyWord(nrecs);
		FTyp = 'R';
		return Z;
		break;
	}
	default: compiler->OldError(177); break;
	}
	return nullptr;
}

char RdOwner(LinkD** LLD, LocVar** LLV)
{
	FileD* fd = nullptr;
	auto result = '\0';
	LocVar* lv = nullptr;
	std::string sLexWord;
	if (compiler->FindLocVar(&LVBD, &lv)) {
		if (!(lv->FTyp == 'i' || lv->FTyp == 'r' || lv->FTyp == 'f')) compiler->Error(177);
		LinkD* ld = nullptr;
		for (auto& ld1 : LinkDRoot) {
			if ((ld1->FromFD == CFile) && (ld1->IndexRoot != 0) && (ld1->ToFD == lv->FD)) {
				ld = ld1;
			}
		}
		if (ld == nullptr) {
			compiler->Error(116);
		}
		compiler->RdLex();
		if (lv->FTyp == 'f') {
#ifdef FandSQL
			if (ld->ToFD->typSQLFile) Error(155);
#endif
			compiler->Accept('[');
			*LLV = (LocVar*)compiler->RdRealFrml(nullptr);
			compiler->Accept(']');
			result = 'F';
			*LLD = ld;
			return result;
		}
		else {
			if (lv->FTyp == 'i') {
				KeyFldD* kf = ((XWKey*)lv->record)->KFlds;
				if (ld->FromFD->IsSQLFile || ld->ToFD->IsSQLFile) compiler->OldError(155);
				if ((kf != nullptr) && !KeyFldD::EquKFlds(kf, ld->ToKey->KFlds)) compiler->OldError(181);
			}
			*LLV = lv;
			*LLD = ld;
			result = lv->FTyp;
			return result;
		}
	}
	compiler->TestIdentif();
	for (auto& ld : LinkDRoot) {
		sLexWord = LexWord;
		if ((ld->FromFD == CFile) && EquUpCase(ld->RoleName, sLexWord)) {
			if ((ld->IndexRoot == 0)) compiler->Error(116);
			compiler->RdLex();
			fd = ld->ToFD;
			if (Lexem == '(') {
				compiler->RdLex();
				if (!compiler->FindLocVar(&LVBD, &lv) || !(lv->FTyp == 'i' || lv->FTyp == 'r')) compiler->Error(177);
				compiler->RdLex();
				compiler->Accept(')');
				if (lv->FD != fd) compiler->OldError(149);
				if (lv->FTyp == 'i') {
					KeyFldD* kf = ((XWKey*)lv->record)->KFlds;
					if (ld->FromFD->IsSQLFile || ld->ToFD->IsSQLFile) compiler->OldError(155);
					if ((kf != nullptr) && !KeyFldD::EquKFlds(kf, ld->ToKey->KFlds)) compiler->OldError(181);
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
				compiler->Accept('[');
				*LLV = (LocVar*)compiler->RdRealFrml(nullptr);
				compiler->Accept(']');
				result = 'F';
				*LLD = ld;
				return result;
			}
		}
		//ld = ld->pChain;
	}
	compiler->Error(9);
	return result;
}

FrmlElem* RdFldNameFrmlP(char& FTyp, MergeReportBase* caller)
{
	FileD* FD = nullptr; FrmlElem* Z = nullptr; LocVar* LV = nullptr;
	instr_type Op = _notdefined;
	LinkD* LD = nullptr; FieldDescr* F = nullptr;
	XKey* K = nullptr;

	FrmlElem* result = nullptr;

	if (compiler->IsForwPoint())
		if (compiler->FindLocVar(&LVBD, &LV) && (LV->FTyp == 'i' || LV->FTyp == 'r')) {
			compiler->RdLex();
			result = RdRecVarFldFrml(LV, FTyp);
			return result;
		}
		else {
			pstring FName = LexWord;
			bool linked = compiler->IsRoleName(FileVarsAllowed, &FD, &LD);
			if (FD != nullptr) {
				FName = FD->Name;
			}
			if (!linked) {
				compiler->RdLex();
			}
			compiler->RdLex();
			FTyp = 'R';
			if (compiler->IsKeyWord("LASTUPDATE")) {
				Op = _lastupdate;
				if (FD != nullptr) goto label2;
				F = nullptr;
				goto label1;
			}
			if (compiler->IsKeyWord("ARCHIVES")) {
				F = CatFD->CatalogArchiveField();
				goto label0;
			}
			if (compiler->IsKeyWord("PATH")) {
				F = CatFD->CatalogPathNameField();
				goto label0;
			}
			if (compiler->IsKeyWord("VOLUME")) {
				F = CatFD->CatalogVolumeField();
			label0:
				FTyp = 'S';
			label1:
				auto S = new FrmlElemCatalogField(_catfield, 6); // Z = GetOp(_catfield, 6);
				S->CatFld = F;
				S->CatIRec = CatFD->GetCatalogIRec(FName, true);
				TestCatError(S->CatIRec, FName, true);
				return S;
			}
			if (FD != nullptr) {
				if (compiler->IsKeyWord("GENERATION")) { Op = _generation; goto label2; }
				if (compiler->IsKeyWord("NRECSABS")) { Op = _nrecsabs; goto label2; }
				if (compiler->IsKeyWord("NRECS")) {
					Op = _nrecs;
				label2:
					auto N = new FrmlElem9(Op, 0); // Z = GetOp(Op, sizeof(FileDPtr));
					N->FD = FD;
					return N;
				}
			}
			if (linked) { result = compiler->RdFAccess(FD, LD, FTyp); return result; }
			if (FileVarsAllowed) compiler->OldError(9);
			else compiler->OldError(63);
		}
	if (ForwChar == '[') {
		auto A = new FrmlElem14(_accrecno, 8); // Z = GetOp(_accrecno, 8);
		FD = compiler->RdFileName();
		compiler->RdLex();
		A->RecFD = FD;
#ifdef FandSQL
		if (rdb_file->typSQLFile) OldError(155);
#endif
		A->P1 = compiler->RdRealFrml(nullptr);
		compiler->Accept(']');
		compiler->Accept('.');
		F = compiler->RdFldName(FD);
		A->RecFldD = F;
		FTyp = F->frml_type;
		return A;
	}
	if (compiler->IsKeyWord("KEYPRESSED")) { Op = _keypressed; goto label3; }
	if (compiler->IsKeyWord("ESCPROMPT")) { Op = _escprompt; goto label3; }
	if (compiler->IsKeyWord("EDUPDATED")) {
		Op = _edupdated;
	label3:
		result = new FrmlElemFunction(Op, 0); // GetOp(Op, 0);
		FTyp = 'B';
		return result;
	}
	if (compiler->IsKeyWord("GETPATH")) {
		result = new FrmlElemFunction(_getpath, 0); // GetOp(_getpath, 0);
		FTyp = 'S';
		return result;
	}
	if (compiler->FindLocVar(&LVBD, &LV)) {
		if (LV->FTyp == 'r' || LV->FTyp == 'f' || LV->FTyp == 'i') compiler->Error(143);
		compiler->RdLex();
		result = new FrmlElem18(LV->Op, LV);
		//((FrmlElem18*)result)->BPOfs = LV->BPOfs;
		FTyp = LV->FTyp;
		return result;
	}
	if (FileVarsAllowed) {
		Z = compiler->TryRdFldFrml(CFile, FTyp, nullptr);
		if (Z == nullptr) compiler->Error(8);
		result = Z;
		return result;
	}
	compiler->Error(8);
	return result;
}

FileD* RdPath(bool NoFD, std::string& Path, WORD& CatIRec)
{
	FileD* fd = nullptr;
	CatIRec = 0;
	if (Lexem == _quotedstr) {
		Path = compiler->RdStringConst();
		fd = nullptr;
	}
	else {
		compiler->TestIdentif();
		fd = compiler->FindFileD();
		if (fd == nullptr) {
			CatIRec = CatFD->GetCatalogIRec(LexWord, true);
			TestCatError(CatIRec, LexWord, false);
		}
		else if (NoFD) compiler->Error(97);
		compiler->RdLex();
	}
	return fd;
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

	if (compiler->IsKeyWord("EVALB")) {
		FTyp = 'B';
		goto label4;
	}
	else if (compiler->IsKeyWord("EVALS")) {
		FTyp = 'S';
		goto label4;
	}
	else if (compiler->IsKeyWord("EVALR")) {
		FTyp = 'R';
	label4:
		compiler->RdLex();
		Z = new FrmlElem21(_eval, 5); // GetOp(_eval, 5);
		((FrmlElem21*)Z)->EvalTyp = FTyp;
		((FrmlElem21*)Z)->EvalP1 = compiler->RdStrFrml(nullptr);
	}
	else if (FileVarsAllowed) compiler->Error(75);
	else if (compiler->IsKeyWord("PROMPT")) {
		compiler->RdLex();
		Z = new FrmlElem11(_prompt, 4); // GetOp(_prompt, 4);
		((FrmlElem11*)Z)->P1 = compiler->RdStrFrml(nullptr);
		FieldDescr* F = RdFieldDescr("", true);
		((FrmlElem11*)Z)->FldD = F; FTyp = F->frml_type;
		if (F->field_type == FieldType::TEXT) compiler->OldError(65);
		if (Lexem == _assign) {
			compiler->RdLex();
			((FrmlElem11*)Z)->P2 = compiler->RdFrml(Typ, nullptr);
			if (Typ != FTyp) compiler->OldError(12);
		}
	}
	else if (compiler->IsKeyWord("KEYOF")) {
		compiler->RdLex();
		FTyp = 'S';
		if (!IsRecVar(&LV)) { Op = _recno; goto label11; }
		Z = new FrmlElem20(_keyof, 8); // GetOp(_keyof, 8);
		((FrmlElem20*)Z)->LV = LV;
		((FrmlElem20*)Z)->PackKey = RdViewKeyImpl(((FrmlElem20*)Z)->LV->FD);
		FTyp = 'S';
	}
	else if (compiler->IsKeyWord("RECNO")) {
		Op = _recno;
		goto label1;
	}
	else if (compiler->IsKeyWord("RECNOABS")) {
		Op = _recnoabs;
		goto label1;
	}
	else if (compiler->IsKeyWord("RECNOLOG")) {
		Op = _recnolog;
	label1:
		compiler->RdLex();
		FTyp = 'R';
	label11:
		FD = compiler->RdFileName();
		XKey* K = RdViewKeyImpl(FD);
		if (Op == _recno) {
			KeyFldD* KF = K->KFlds;
			N = 0;
			if (KF == nullptr) compiler->OldError(176);
			while (KF != nullptr) {
				compiler->Accept(',');
				if (N > 29) compiler->Error(123);
				Arg[N] = compiler->RdFrml(Typ, nullptr);
				N++;
				if (Typ != KF->FldD->frml_type) compiler->OldError(12);
				KF = (KeyFldD*)KF->pChain;
			}
		}
		else {
			compiler->Accept(',');
			N = 1;
			Arg[0] = compiler->RdRealFrml(nullptr);
		}
		Z = new FrmlElem13(Op, (N + 2) * 4); // GetOp(Op, (N + 2) * 4);
		auto iZ = (FrmlElem13*)Z;
		iZ->FFD = FD;
		iZ->Key = K;
		iZ->SaveArgs(Arg, N);
		if (FTyp == 'R') goto label2;
	}
	else if (compiler->IsKeyWord("LINK")) {
		compiler->RdLex();
		Z = new FrmlElem15(_link, 5); // GetOp(_link, 5);
		auto iZ = (FrmlElem15*)Z;
		if (IsRecVar(&LV)) {
			iZ->LinkFromRec = true;
			iZ->LinkLV = LV;
			FD = LV->FD;
		}
		else {
			FD = compiler->RdFileName();
			compiler->Accept('[');
			iZ->LinkRecFrml = compiler->RdRealFrml(nullptr);
			compiler->Accept(']');
		}
		compiler->Accept(',');
#ifdef FandSQL
		if (rdb_file->typSQLFile) OldError(155);
#endif
		cf = CFile;
		CFile = FD;
		if (!compiler->IsRoleName(true, &FD, &LD) || (LD == nullptr)) compiler->Error(9);
		CFile = cf;
		iZ->LinkLD = LD;
		FTyp = 'R';
		goto label2;
	}
	else if (compiler->IsKeyWord("ISDELETED")) {
		compiler->RdLex();
		FTyp = 'B';
		if (IsRecVar(&LV)) {
			Z = new FrmlElem20(_lvdeleted, 4); // GetOp(_lvdeleted, 4);
			((FrmlElem20*)Z)->LV = LV;
		}
		else {
			Z = new FrmlElem14(_isdeleted, 4); // GetOp(_isdeleted, 4);
			FD = compiler->RdFileName();
			((FrmlElem14*)Z)->RecFD = FD;
			compiler->Accept(',');
			((FrmlElem14*)Z)->P1 = compiler->RdRealFrml(nullptr);
		label2: {}
#ifdef FandSQL
			if (rdb_file->typSQLFile) Error(155);
#endif
		}
	}
	else if (compiler->IsKeyWord("GETPATH")) {
		compiler->RdLex();
		Z = new FrmlElemFunction(_getpath, 0); // GetOp(_getpath, 0);
		((FrmlElemFunction*)Z)->P1 = compiler->RdStrFrml(nullptr);
		FTyp = 'S';
	}
	else if (compiler->IsKeyWord("GETTXT")) {
		compiler->RdLex();
		Z = new FrmlElem16(_gettxt, 6); // GetOp(_gettxt, 6);
		FTyp = 'S';
		goto label3;
	}
	else if (compiler->IsKeyWord("FILESIZE")) {
		compiler->RdLex();
		Z = new FrmlElem16(_filesize, 14); // GetOp(_filesize, 14);
		FTyp = 'R';
	label3:
		auto iZ = (FrmlElem16*)Z;
		RdPath(true, iZ->TxtPath, iZ->TxtCatIRec);
		if ((Z->Op == _gettxt) && (Lexem == ',')) {
			compiler->RdLex();
			iZ->P1 = compiler->RdRealFrml(nullptr);
			if (Lexem == ',') {
				compiler->RdLex();
				iZ->P2 = compiler->RdRealFrml(nullptr);
			}
		}
	}
	else if (compiler->IsKeyWord("INTTSR")) {
		compiler->RdLex();
		Z = new FrmlElemFunction(_inttsr, 5); // GetOp(_inttsr, 5);
		auto iZ = (FrmlElemFunction*)Z;
		iZ->P1 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
		iZ->P2 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
		Typ = 'r';
		if (IsRecVar(&LV)) iZ->P3 = (FrmlElem*)LV->record;
		else iZ->P3 = compiler->RdFrml(Typ, nullptr);
		iZ->N31 = Typ;
		FTyp = 'R';
	}
#ifdef FandSQL
	else if (IsKeyWord("SQL")) {
		RdLex(); Z = GetOp(_sqlfun, 0); Z->P1 = RdStrFrml(); FTyp = 'rdb';
	}
#endif
	else if (compiler->IsKeyWord("SELECTSTR")) {
		compiler->RdLex();
		Z = new FrmlElemFunction(_selectstr, 13); // GetOp(_selectstr, 13);
		FTyp = 'S';
		RdSelectStr((FrmlElemFunction*)Z);
	}
	else if (compiler->IsKeyWord("PROMPTYN")) {
		compiler->RdLex();
		Z = new FrmlElemFunction(_promptyn, 0); // GetOp(_promptyn, 0);
		((FrmlElemFunction*)Z)->P1 = compiler->RdStrFrml(nullptr);
		FTyp = 'B';
	}
	else if (compiler->IsKeyWord("MOUSEEVENT")) {
		compiler->RdLex();
		Z = new FrmlElem1(_mouseevent, 2); // GetOp(_mouseevent, 2);
		((FrmlElem1*)Z)->W01 = compiler->RdInteger();
		FTyp = 'B';
	}
	else if (compiler->IsKeyWord("ISMOUSE")) {
		compiler->RdLex();
		Z = new FrmlElem1(_ismouse, 4); // GetOp(_ismouse, 4);
		((FrmlElem1*)Z)->W01 = compiler->RdInteger(); compiler->Accept(',');
		((FrmlElem1*)Z)->W02 = compiler->RdInteger(); FTyp = 'B';
	}
	else if (compiler->IsKeyWord("MOUSEIN")) {
		compiler->RdLex();
		Z = new FrmlElemFunction(_mousein, 4); // GetOp(_mousein, 4);
		auto iZ = (FrmlElemFunction*)Z;
		iZ->P1 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
		iZ->P2 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
		iZ->P3 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
		iZ->P4 = compiler->RdRealFrml(nullptr);
		FTyp = 'B';
	}
	else if (compiler->IsKeyWord("PORTIN")) {
		compiler->RdLex();
		Z = new FrmlElemFunction(_portin, 0); // GetOp(_portin, 0);
		auto iZ = (FrmlElemFunction*)Z;
		iZ->P1 = compiler->RdBool(nullptr);
		compiler->Accept(',');
		iZ->P2 = compiler->RdRealFrml(nullptr);
		FTyp = 'R';
	}
	else {
		compiler->Error(75);
	}
	compiler->Accept(')');
	FrmlElem* result = Z;
	FFTyp = FTyp;
	return result;
}

XKey* RdViewKeyImpl(FileD* FD)
{
	XKey* K = nullptr;
	if (FD != nullptr) K = FD->Keys.empty() ? nullptr : FD->Keys[0];
	if (K == nullptr) compiler->Error(24);
	if (Lexem == '/') {
		FileD* cf = CFile;
		CFile = FD;
		K = compiler->RdViewKey();
		CFile = cf;
	}
	return K;
}

void RdSelectStr(FrmlElemFunction* Z)
{
	Z->Delim = 0x0D; // CTRL+M
	Z->P1 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
	Z->P2 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
	Z->P3 = compiler->RdStrFrml(nullptr);
	while (Lexem == ',') {
		compiler->RdLex();
		if (compiler->IsOpt("HEAD")) Z->P4 = compiler->RdStrFrml(nullptr);
		else if (compiler->IsOpt("FOOT")) Z->P5 = compiler->RdStrFrml(nullptr);
		else if (compiler->IsOpt("MODE")) Z->P6 = compiler->RdStrFrml(nullptr);
		else if (compiler->IsOpt("DELIM")) Z->Delim = compiler->RdQuotedChar();
		else compiler->Error(157);
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
	compiler->AcceptKeyWord("OF");

	while (true) {
		if (compiler->IsKeyWord("ESCAPE")) {
			compiler->Accept(':');
			PD->WasESCBranch = true;
			PD->ESCInstr = RdPInstr();
		}
		else {
			CD = new ChoiceD();
			PD->Choices.push_back(CD);

			N++;
			if ((PD->Kind == PInstrCode::_menubar) && (N > 30)) compiler->Error(102);
			CD->TxtFrml = compiler->RdStrFrml(nullptr);
			if (Lexem == ',') {
				compiler->RdLex();
				if (Lexem != ',') {
					CD->HelpName = compiler->RdHelpName();
					PD->HelpRdb = CRdb;
				}
				if (Lexem == ',') {
					compiler->RdLex();
					if (Lexem != ',') {
						CD->Condition = compiler->RdBool(nullptr);
						if (Lexem == '!') {
							CD->DisplEver = true;
							compiler->RdLex();
						}
					}
				}
			}
			compiler->Accept(':');
			CD->Instr = RdPInstr();
		}
		if (Lexem == ';') {
			compiler->RdLex();
			if (compiler->IsKeyWord("END")) return;
			continue;
		}
		break;
	}

	compiler->AcceptKeyWord("END");
}

void RdMenuAttr(Instr_menu* PD)
{
	if (Lexem != ';') return;
	compiler->RdLex();
	PD->mAttr[0] = compiler->RdAttr(); compiler->Accept(',');
	PD->mAttr[1] = compiler->RdAttr(); compiler->Accept(',');
	PD->mAttr[2] = compiler->RdAttr();
	if (Lexem == ',') {
		compiler->RdLex();
		PD->mAttr[3] = compiler->RdAttr();
	}
}

Instr* RdMenuBox(bool Loop)
{
	Instr_menu* PD = nullptr; pstring* S = nullptr;
	PD = new Instr_menu(PInstrCode::_menubox); // GetPInstr(_menubox, 48);
	auto result = PD;
	PD->Loop = Loop;
	if (Lexem == '(') {
		compiler->RdLex();
		if (Lexem != ';') {
			PD->X = compiler->RdRealFrml(nullptr);
			compiler->Accept(',');
			PD->Y = compiler->RdRealFrml(nullptr);
		}
		RdMenuAttr(PD);
		compiler->Accept(')');
	}
	if (Lexem == '!') { compiler->RdLex(); PD->Shdw = true; }
	if (compiler->IsKeyWord("PULLDOWN")) PD->PullDown = true;
	if (!compiler->TestKeyWord("OF")) PD->HdLine = compiler->RdStrFrml(nullptr);
	RdChoices(PD);
	return result;
}

Instr* RdMenuBar()
{
	Instr_menu* PD = new Instr_menu(PInstrCode::_menubar); // GetPInstr(_menubar, 48);
	auto result = PD;
	if (Lexem == '(') {
		compiler->RdLex();
		if (Lexem != ';') {
			PD->Y = compiler->RdRealFrml(nullptr);
			if (Lexem == ',') {
				compiler->RdLex();
				PD->X = compiler->RdRealFrml(nullptr);
				compiler->Accept(',');
				PD->XSz = compiler->RdRealFrml(nullptr);
			}
		}
		RdMenuAttr(PD);
		compiler->Accept(')');
	}
	RdChoices(PD);
	return result;
}

Instr_loops* RdIfThenElse()
{
	auto PD = new Instr_loops(PInstrCode::_ifthenelseP); // GetPInstr(_ifthenelseP, 12);
	auto result = PD;
	PD->Bool = compiler->RdBool(nullptr);
	compiler->AcceptKeyWord("THEN");
	PD->Instr1 = RdPInstr();
	if (compiler->IsKeyWord("ELSE")) PD->ElseInstr1 = RdPInstr();
	return result;
}

Instr_loops* RdWhileDo()
{
	auto PD = new Instr_loops(PInstrCode::_whiledo); // GetPInstr(_whiledo, 8);
	auto result = PD;
	PD->Bool = compiler->RdBool(nullptr);
	compiler->AcceptKeyWord("DO");
	PD->Instr1 = RdPInstr();
	return result;
}

Instr* RdFor()
{
	LocVar* LV = nullptr;
	if (!compiler->FindLocVar(&LVBD, &LV) || (LV->FTyp != 'R')) compiler->Error(146);
	compiler->RdLex();
	auto* PD = new Instr_assign(PInstrCode::_asgnloc); // GetPInstr(_asgnloc, 9);
	auto result = PD;
	PD->AssLV = LV;
	compiler->Accept(_assign);
	PD->Frml = compiler->RdRealFrml(nullptr);

	compiler->AcceptKeyWord("TO");
	auto iLoop = new Instr_loops(PInstrCode::_whiledo); // GetPInstr(_whiledo, 8);
	PD->Chain = iLoop;
	//PD = (Instr_assign*)PD->pChain;
	auto Z1 = new FrmlElemFunction(_compreal, 2); // GetOp(_compreal, 2);
	Z1->P1 = nullptr;
	Z1->LV1 = LV;
	Z1->N21 = _le;
	Z1->N22 = 5;
	Z1->P2 = compiler->RdRealFrml(nullptr);
	iLoop->Bool = Z1;

	compiler->AcceptKeyWord("DO");
	iLoop->Instr1 = RdPInstr();

	auto iAsg = new Instr_assign(PInstrCode::_asgnloc); // GetPInstr(_asgnloc, 9);
	//ChainLast(iLoop->Instr1, iAsg);
	iLoop->AddInstr(iAsg);
	iAsg->Add = true;
	iAsg->AssLV = LV;
	auto Z2 = new FrmlElemNumber(_const, 0, 1); // GetOp(_const, sizeof(double));
	//Z->rdb = 1;
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
		PD1 = new Instr_loops(PInstrCode::_ifthenelseP); // GetPInstr(_ifthenelseP, 12);
		if (first) result = PD1;
		else PD->ElseInstr1 = PD1;
		PD = PD1;
		first = false;
		PD->Bool = compiler->RdBool(nullptr);
		compiler->Accept(':');
		PD->Instr1 = RdPInstr();
		bool b = Lexem == ';';
		if (b) compiler->RdLex();
		if (!compiler->IsKeyWord("END")) {
			if (compiler->IsKeyWord("ELSE")) {
				while (!compiler->IsKeyWord("END")) {
					RdPInstrAndChain(&PD->ElseInstr1);
					if (Lexem == ';') {
						compiler->RdLex();
					}
					else {
						compiler->AcceptKeyWord("END");
						break;
					}
				}
			}
			else if (b) {
				continue;
			}
			else {
				compiler->AcceptKeyWord("END");
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
	while (!compiler->IsKeyWord("UNTIL")) {
		RdPInstrAndChain(&PD->Instr1);
		if (Lexem == ';') compiler->RdLex();
		else {
			compiler->AcceptKeyWord("UNTIL");
			break;
		}
	}
	PD->Bool = compiler->RdBool(nullptr);
	return result;
}

Instr_forall* RdForAll()
{
	LocVar* LVi = nullptr;
	LocVar* LVr = nullptr;
	LinkD* LD = nullptr;
	FrmlElem* Z = nullptr;
	if (!compiler->FindLocVar(&LVBD, &LVi)) compiler->Error(122);
	compiler->RdLex();
	if (LVi->FTyp == 'r') {
		LVr = LVi;
		LVi = nullptr;
		CFile = LVr->FD;
	}
	else {
		compiler->TestReal(LVi->FTyp);
		compiler->AcceptKeyWord("IN");
		if (compiler->FindLocVar(&LVBD, &LVr)) {
			if (LVr->FTyp == 'f') {
				CFile = LVr->FD;
				compiler->RdLex();
				goto label1;
			}
			if (LVr->FTyp != 'r') compiler->Error(141);
			CFile = LVr->FD;
			compiler->RdLex();
		}
		else {
			CFile = compiler->RdFileName();
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
	if (compiler->IsKeyWord("OWNER")) {
		PD->COwnerTyp = RdOwner(&PD->CLD, &PD->CLV);
		CViewKey = GetFromKey(PD->CLD);
	}
	else {
		CViewKey = compiler->RdViewKey();
	}
	if (Lexem == '(') {
		compiler->RdLex();
		PD->CBool = compiler->RdKeyInBool(&PD->CKIRoot, false, true, PD->CSQLFilter, nullptr);
		if ((PD->CKIRoot != nullptr) && (PD->CLV != nullptr)) compiler->OldError(118);
		compiler->Accept(')');
	}
	if (Lexem == '!') { compiler->RdLex(); PD->CWIdx = true; }
	if (Lexem == '%') { compiler->RdLex(); PD->CProcent = true; }
	PD->CKey = CViewKey;
label2:
	compiler->AcceptKeyWord("DO");
	PD->CInstr = RdPInstr();
	return PD;
}

Instr* RdBeginEnd()
{
	Instr* PD = nullptr;
	if (!compiler->IsKeyWord("END")) {
		while (true) {
			RdPInstrAndChain(&PD);
			if (Lexem == ';') {
				compiler->RdLex();
				if (!compiler->IsKeyWord("END")) {
					continue;
				}
			}
			else {
				compiler->AcceptKeyWord("END");
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
		compiler->RdChptName('P', &Pos, Caller == 'P' || Caller == 'E' || Caller == 'T');
	}
	WORD N = 0;
	if (Caller != 'P') {
		if (Lexem == '(') {
			compiler->RdLex();
			goto label1;
		}
	}
	else if (Lexem == ',') {
		compiler->RdLex();
		compiler->Accept('(');
	label1:
		N++;
		if (N > 30) compiler->Error(123);
		TArg[N].Name = LexWord;
		if ((ForwChar != '.') && compiler->FindLocVar(&LVBD, &LV) && (LV->FTyp == 'i' || LV->FTyp == 'r')) {
			compiler->RdLex();
			TArg[N].FTyp = LV->FTyp;
			TArg[N].FD = LV->FD;
			TArg[N].RecPtr = LV->record;
		}
		else if (Lexem == '@') {
			compiler->RdLex();
			if (Lexem == '[') {
				compiler->RdLex();
				TArg[N].Name = LexWord;
				compiler->Accept(_identifier);
				compiler->Accept(',');
				auto z = new FrmlElemFunction(_setmybp, 0); // GetOp(_setmybp, 0);
				z->P1 = compiler->RdStrFrml(nullptr);
				TArg[N].TxtFrml = z;
				compiler->Accept(']');
			}
			else {
				TArg[N].FD = compiler->RdFileName();
			}
			TArg[N].FTyp = 'f';
		}
		else {
			TArg[N].Frml = compiler->RdFrml(TArg[N].FTyp, nullptr);
		}
		if (Lexem == ',') {
			compiler->RdLex();
			goto label1;
		}
		compiler->Accept(')');
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
		compiler->RdLex();
	}
	else {
		for (i = 0; i < NKeyNames; i++) {
			if (EquUpCase(KeyNames[i].Nm, LexWord)) {
				lastKey->KeyCode = KeyNames[i].Code;
				lastKey->Break = KeyNames[i].Brk;
				compiler->RdLex();
				return;
			}
		}
		compiler->Error(129);
	}
}

bool RdHeadLast(EditOpt* EO)
{
	auto result = true;
	if (compiler->IsOpt("HEAD")) EO->Head = compiler->RdStrFrml(nullptr);
	else if (compiler->IsOpt("LAST")) EO->Last = compiler->RdStrFrml(nullptr);
	else if (compiler->IsOpt("CTRL")) EO->CtrlLast = compiler->RdStrFrml(nullptr);
	else if (compiler->IsOpt("ALT")) EO->AltLast = compiler->RdStrFrml(nullptr);
	else if (compiler->IsOpt("SHIFT")) EO->ShiftLast = compiler->RdStrFrml(nullptr);
	else result = false;
	return result;
}

bool RdHeadLast(Instr_edittxt* IE)
{
	auto result = true;
	if (compiler->IsOpt("HEAD")) IE->Head = compiler->RdStrFrml(nullptr);
	else if (compiler->IsOpt("LAST")) IE->Last = compiler->RdStrFrml(nullptr);
	else if (compiler->IsOpt("CTRL")) IE->CtrlLast = compiler->RdStrFrml(nullptr);
	else if (compiler->IsOpt("ALT")) IE->AltLast = compiler->RdStrFrml(nullptr);
	else if (compiler->IsOpt("SHIFT")) IE->ShiftLast = compiler->RdStrFrml(nullptr);
	else result = false;
	return result;
}

bool RdViewOpt(EditOpt* EO)
{
	FileD* FD = nullptr;
	RprtOpt* RO = nullptr;
	bool Flgs[23]{ false };
	auto result = false;
	compiler->RdLex();
	result = true;
	CViewKey = EO->ViewKey;
	if (compiler->IsOpt("TAB")) {
		compiler->RdNegFldList(EO->NegTab, EO->Tab);
	}
	else if (compiler->IsOpt("DUPL")) {
		compiler->RdNegFldList(EO->NegDupl, EO->Dupl);
	}
	else if (compiler->IsOpt("NOED")) {
		compiler->RdNegFldList(EO->NegNoEd, EO->NoEd);
	}
	else if (compiler->IsOpt("MODE")) {
		compiler->SkipBlank(false);
		if ((Lexem == _quotedstr) && (ForwChar == ',' || ForwChar == ')')) {
			DataEditorParams params;
			int validate = params.SetFromString(LexWord, true);
			if (validate != 0) {
				compiler->Error(validate);
			}
			EO->Mode = new FrmlElemString(_const, 0); // GetOp(_const, LexWord.length() + 1);
			((FrmlElemString*)EO->Mode)->S = LexWord;
			compiler->RdLex();
		}
		else {
			EO->Mode = compiler->RdStrFrml(nullptr);
		}
	}
	else if (RdHeadLast(EO)) return result;
	else if (compiler->IsOpt("WATCH")) {
		EO->WatchDelayZ = compiler->RdRealFrml(nullptr);
	}
	else if (compiler->IsOpt("WW")) {
		compiler->Accept('(');
		EO->WFlags = 0;
		if (Lexem == '(') { compiler->RdLex(); EO->WFlags = WNoPop; }
		compiler->RdW(EO->W);
		compiler->RdFrame(&EO->Top, EO->WFlags);
		if (Lexem == ',') {
			compiler->RdLex();
			EO->ZAttr = compiler->RdAttr(); compiler->Accept(',');
			EO->ZdNorm = compiler->RdAttr(); compiler->Accept(',');
			EO->ZdHiLi = compiler->RdAttr();
			if (Lexem == ',') {
				compiler->RdLex();
				EO->ZdSubset = compiler->RdAttr();
				if (Lexem == ',') {
					compiler->RdLex();
					EO->ZdDel = compiler->RdAttr();
					if (Lexem == ',') {
						compiler->RdLex();
						EO->ZdTab = compiler->RdAttr();
						if (Lexem == ',') {
							compiler->RdLex();
							EO->ZdSelect = compiler->RdAttr();
						}
					}
				}
			}
		}
		compiler->Accept(')');
		if ((EO->WFlags & WNoPop) != 0) compiler->Accept(')');
	}
	else if (compiler->IsOpt("EXIT")) {
		compiler->Accept('(');
	label1:
		EdExitD* X = new EdExitD();
		EO->ExD.push_back(X);

		RdKeyList(X);
		if (compiler->IsKeyWord("QUIT")) X->Typ = 'Q';
		else if (compiler->IsKeyWord("REPORT")) {
			if (X->AtWrRec || (EO->LVRecPtr != nullptr)) compiler->OldError(144);
			compiler->Accept('(');
			X->Typ = 'R';
			RO = compiler->GetRprtOpt();
			compiler->RdChptName('R', &RO->RprtPos, true);
			while (Lexem == ',') {
				compiler->RdLex();
				if (compiler->IsOpt("ASSIGN")) RdPath(true, RO->Path, RO->CatIRec);
				else if (compiler->IsKeyWord("EDIT")) RO->Edit = true;
				else compiler->Error(130);
			}
			X->RO = RO; compiler->Accept(')');
		}
		else if (!(Lexem == ',' || Lexem == ')')) {
			X->Typ = 'P';
			X->Proc = RdProcArg('E');
		}
		if (Lexem == ',') { compiler->RdLex(); goto label1; }
		compiler->Accept(')');
	}
	else if (EO->LVRecPtr != nullptr) result = false;
	else if (compiler->IsOpt("COND")) {
		if (Lexem == '(') {
			compiler->RdLex();
			EO->Cond = compiler->RdKeyInBool(&EO->KIRoot, false, true, EO->SQLFilter, nullptr);
			compiler->Accept(')');
		}
		else EO->Cond = compiler->RdKeyInBool(&EO->KIRoot, false, true, EO->SQLFilter, nullptr);
	}
	else if (compiler->IsOpt("JOURNAL")) {
		EO->Journal = compiler->RdFileName();
		WORD l = EO->Journal->FF->RecLen - 13;
		if (CFile->FF->file_type == FileType::INDEX) l++;
		if (CFile->FF->RecLen != l) compiler->OldError(111);
	}
	else if (compiler->IsOpt("SAVEAFTER")) EO->SaveAfterZ = compiler->RdRealFrml(nullptr);
	else if (compiler->IsOpt("REFRESH")) EO->RefreshDelayZ = compiler->RdRealFrml(nullptr);
	else result = false;
	return result;
}

void RdKeyList(EdExitD* X)
{
	while (true) {
		if ((Lexem == '(') || (Lexem == '^')) {
			compiler->RdNegFldList(X->NegFlds, &X->Flds);
		}
		else if (compiler->IsKeyWord("RECORD")) {
			X->AtWrRec = true;
		}
		else if (compiler->IsKeyWord("NEWREC")) {
			X->AtNewRec = true;
		}
		else {
			RdKeyCode(X);
		}
		if (Lexem == ',') {
			compiler->RdLex();
			continue;
		}
		break;
	}
	compiler->Accept(':');
}

void RdProcCall(Instr** pinstr)
{
	//Instr* PD = nullptr;
	if (compiler->IsKeyWord("EXEC")) *pinstr = RdExec();
	else if (compiler->IsKeyWord("COPYFILE")) *pinstr = RdCopyFile();
	else if (compiler->IsKeyWord("PROC")) {
		compiler->RdLex();
		*pinstr = RdProcArg('P');
	}
	else if (compiler->IsKeyWord("DISPLAY")) *pinstr = RdDisplay();
	else if (compiler->IsKeyWord("CALL")) *pinstr = RdRDBCall();
	else if (compiler->IsKeyWord("WRITELN")) RdWriteln(WriteType::writeln, (Instr_writeln**)pinstr);
	else if (compiler->IsKeyWord("WRITE")) RdWriteln(WriteType::write, (Instr_writeln**)pinstr);
	else if (compiler->IsKeyWord("HEADLINE")) {
		*pinstr = new Instr_assign(PInstrCode::_headline); // GetPD(_headline, 4);
		compiler->RdLex();
		goto label1;
	}
	else if (compiler->IsKeyWord("SETKEYBUF")) {
		*pinstr = new Instr_assign(PInstrCode::_setkeybuf); //GetPD(_setkeybuf, 4);
		compiler->RdLex();
		goto label1;
	}
	else if (compiler->IsKeyWord("HELP")) {
		*pinstr = new Instr_help(); // GetPD(_help, 8);
		compiler->RdLex();
		if (CRdb->help_file == nullptr) compiler->OldError(132);
		((Instr_help*)*pinstr)->HelpRdb0 = CRdb;
	label1:
		((Instr_help*)*pinstr)->Frml0 = compiler->RdStrFrml(nullptr);
	}
	else if (compiler->IsKeyWord("MESSAGE")) RdWriteln(WriteType::message, (Instr_writeln**)pinstr);
	else if (compiler->IsKeyWord("GOTOXY")) *pinstr = RdGotoXY();
	else if (compiler->IsKeyWord("MERGE")) {
		// PD = (Instr_merge_display*)GetPD(_merge, sizeof(RdbPos));
		*pinstr = new Instr_merge_display(PInstrCode::_merge);
		compiler->RdLex();
		RdbPos rp;
		compiler->RdChptName('M', &rp, true);
		((Instr_merge_display*)*pinstr)->Pos = rp;
	}
	else if (compiler->IsKeyWord("SORT")) *pinstr = RdSortCall();
	else if (compiler->IsKeyWord("EDIT")) *pinstr = RdEditCall();
	else if (compiler->IsKeyWord("REPORT")) *pinstr = RdReportCall();
	else if (compiler->IsKeyWord("EDITTXT")) *pinstr = RdEditTxt();
	else if (compiler->IsKeyWord("PRINTTXT")) *pinstr = RdPrintTxt();
	else if (compiler->IsKeyWord("PUTTXT")) *pinstr = RdPutTxt();
	else if (compiler->IsKeyWord("TURNCAT")) *pinstr = RdTurnCat();
	else if (compiler->IsKeyWord("RELEASEDRIVE")) *pinstr = RdReleaseDrive();
	else if (compiler->IsKeyWord("SETPRINTER")) {
		*pinstr = new Instr_assign(PInstrCode::_setprinter); // GetPD(_setprinter, 4);
		compiler->RdLex();
		goto label2;
	}
	else if (compiler->IsKeyWord("INDEXFILE")) *pinstr = RdIndexfile();
	else if (compiler->IsKeyWord("GETINDEX"))*pinstr = RdGetIndex();
	else if (compiler->IsKeyWord("MOUNT")) *pinstr = RdMount();
	else if (compiler->IsKeyWord("CLRSCR")) *pinstr = RdClrWw();
	else if (compiler->IsKeyWord("APPENDREC")) *pinstr = RdMixRecAcc(PInstrCode::_appendRec);
	else if (compiler->IsKeyWord("DELETEREC")) *pinstr = RdMixRecAcc(PInstrCode::_deleterec);
	else if (compiler->IsKeyWord("RECALLREC")) *pinstr = RdMixRecAcc(PInstrCode::_recallrec);
	else if (compiler->IsKeyWord("READREC")) *pinstr = RdMixRecAcc(PInstrCode::_readrec);
	else if (compiler->IsKeyWord("WRITEREC")) *pinstr = RdMixRecAcc(PInstrCode::_writerec);
	else if (compiler->IsKeyWord("LINKREC")) *pinstr = RdLinkRec();
	else if (compiler->IsKeyWord("DELAY")) {
		*pinstr = new Instr_assign(PInstrCode::_delay); // GetPD(_delay, 4);
		compiler->RdLex();
		goto label2;
	}
	else if (compiler->IsKeyWord("SOUND")) {
		*pinstr = new Instr_assign(PInstrCode::_sound); // GetPD(_sound, 4);
		compiler->RdLex();
	label2:
		((Instr_assign*)*pinstr)->Frml = compiler->RdRealFrml(nullptr);
	}
	else if (compiler->IsKeyWord("LPROC")) *pinstr = RdCallLProc();

#ifdef FandGraph
	else if (compiler->IsKeyWord("GRAPH")) *pinstr = RdGraphP();
	else if (compiler->IsKeyWord("PUTPIXEL")) {
		*pinstr = new Instr_putpixel(PInstrCode::_putpixel); // GetPD(_putpixel, 3 * 4);
		goto label3;
	}
	else if (compiler->IsKeyWord("LINE")) {
		*pinstr = new Instr_putpixel(PInstrCode::_line); // GetPD(_line, 5 * 4);
		goto label3;
	}
	else if (compiler->IsKeyWord("RECTANGLE")) {
		*pinstr = new Instr_putpixel(PInstrCode::_rectangle); // GetPD(_rectangle, 5 * 4);
		goto label3;
	}
	else if (compiler->IsKeyWord("ELLIPSE")) {
		*pinstr = new Instr_putpixel(PInstrCode::_ellipse);  // GetPD(_ellipse, 7 * 4);
		goto label3;
	}
	else if (compiler->IsKeyWord("FLOODFILL")) {
		*pinstr = new Instr_putpixel(PInstrCode::_floodfill); // GetPD(_floodfill, 5 * 4);
		goto label3;
	}
	else if (compiler->IsKeyWord("OUTTEXTXY")) {
		*pinstr = new Instr_putpixel(PInstrCode::_outtextxy); // GetPD(_outtextxy, 11 * 4);
	label3:
		compiler->RdLex(); // read '('
		auto iPutPixel = (Instr_putpixel*)(*pinstr);
		iPutPixel->Par1 = compiler->RdRealFrml(nullptr);
		compiler->Accept(',');
		iPutPixel->Par2 = compiler->RdRealFrml(nullptr);
		compiler->Accept(',');
		if (iPutPixel->Kind == PInstrCode::_outtextxy) {
			iPutPixel->Par3 = compiler->RdStrFrml(nullptr);
			compiler->Accept(',');
			iPutPixel->Par4 = compiler->RdRealFrml(nullptr);
			compiler->Accept(',');
			iPutPixel->Par5 = compiler->RdAttr();
			if (Lexem == ',') {
				compiler->RdLex();
				iPutPixel->Par6 = compiler->RdRealFrml(nullptr);
				if (Lexem == ',') {
					compiler->RdLex();
					iPutPixel->Par7 = compiler->RdRealFrml(nullptr);
					if (Lexem == ',') {
						compiler->RdLex();
						iPutPixel->Par8 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
						iPutPixel->Par9 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
						iPutPixel->Par10 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
						iPutPixel->Par11 = compiler->RdRealFrml(nullptr);
					}
				}
			}
		}
		else if (iPutPixel->Kind == PInstrCode::_putpixel) iPutPixel->Par3 = compiler->RdAttr();
		else {
			iPutPixel->Par3 = compiler->RdRealFrml(nullptr);
			compiler->Accept(',');
			if (iPutPixel->Kind == PInstrCode::_floodfill) iPutPixel->Par4 = compiler->RdAttr();
			else iPutPixel->Par4 = compiler->RdRealFrml(nullptr);
			compiler->Accept(',');
			iPutPixel->Par5 = compiler->RdAttr();
			if ((iPutPixel->Kind == PInstrCode::_ellipse) && (Lexem == ',')) {
				compiler->RdLex();
				iPutPixel->Par6 = compiler->RdRealFrml(nullptr);
				compiler->Accept(',');
				iPutPixel->Par7 = compiler->RdRealFrml(nullptr);
			}
		}
	}
#endif 
	else if (compiler->IsKeyWord("CLOSE")) {
		*pinstr = new Instr_closefds(); // GetPD(_closefds, 4);
		compiler->RdLex();
		((Instr_closefds*)*pinstr)->clFD = compiler->RdFileName();
	}
	else if (compiler->IsKeyWord("BACKUP")) *pinstr = RdBackup(' ', true);
	else if (compiler->IsKeyWord("BACKUPM")) *pinstr = RdBackup('M', true);
	else if (compiler->IsKeyWord("RESTORE")) *pinstr = RdBackup(' ', false);
	else if (compiler->IsKeyWord("RESTOREM")) *pinstr = RdBackup('M', false);
	else if (compiler->IsKeyWord("SETEDITTXT")) *pinstr = RdSetEditTxt();
	else if (compiler->IsKeyWord("SETMOUSE")) {
		*pinstr = new Instr_setmouse(); // GetPD(_setmouse, 12);
		compiler->RdLex();
		((Instr_setmouse*)*pinstr)->MouseX = compiler->RdRealFrml(nullptr);
		compiler->Accept(',');
		((Instr_setmouse*)*pinstr)->MouseY = compiler->RdRealFrml(nullptr);
		compiler->Accept(',');
		((Instr_setmouse*)*pinstr)->Show = compiler->RdBool(nullptr);
	}
	else if (compiler->IsKeyWord("CHECKFILE")) {
		*pinstr = new Instr_checkfile();
		compiler->RdLex();
		auto iPD = (Instr_checkfile*)*pinstr;
		iPD->cfFD = compiler->RdFileName();
		if (iPD->cfFD != nullptr && (iPD->cfFD->FF->file_type == FileType::FAND8 || iPD->cfFD->FF->file_type == FileType::DBF)
#ifdef FandSQL
			|| PD->cfFD->typSQLFile
#endif
			) compiler->OldError(169);
		compiler->Accept(',');
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
	else if (compiler->IsKeyWord("PORTOUT")) {
		*pinstr = new Instr_portout(); // GetPD(_portout, 12);
		compiler->RdLex();
		auto iPD = (Instr_portout*)*pinstr;
		iPD->IsWord = compiler->RdBool(nullptr); compiler->Accept(',');
		iPD->Port = compiler->RdRealFrml(nullptr); compiler->Accept(',');
		iPD->PortWhat = compiler->RdRealFrml(nullptr);
	}
	else compiler->Error(34);
	compiler->Accept(')');
}

std::vector<FieldDescr*> RdFlds()
{
	std::vector<FieldDescr*> FLRoot;
	FieldListEl* FL = nullptr;

	while (true) {
		auto fd = compiler->RdFldName(CFile);
		FLRoot.push_back(fd);
		if (Lexem == ',') {
			compiler->RdLex();
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
	compiler->Accept('(');
label1:
	FL = new FieldListEl();
	FLRoot.push_back(FL);

	if (InFL.empty()) F = compiler->RdFldName(CFile);
	else {
		compiler->TestIdentif();
		//FL1 = InFL;
		//while (FL1 != nullptr) {
		for (auto& f : InFL) {
			std::string tmp = LexWord;
			FL1 = f;
			if (EquUpCase(f->Name, tmp)) goto label2;
			//FL1 = FL1->pChain;
		}
		compiler->Error(43);
	label2:
		F = FL1;
		compiler->RdLex();
	}
	FL->FldD = F;
	if ((Opt == 'S') && (F->frml_type != 'R')) compiler->OldError(20);
	if (Lexem == ',') { compiler->RdLex(); goto label1; }
	compiler->Accept(')');

	// transform to vector of FieldDescr*
	for (auto& fld : FLRoot) {
		result.push_back(fld->FldD);
	}
	return result;
}

Instr_sort* RdSortCall()
{
	auto PD = new Instr_sort(); // GetPD(_sort, 8);
	compiler->RdLex();
	FileD* FD = compiler->RdFileName();
	PD->SortFD = FD;
#ifdef FandSQL
	if (rdb_file->typSQLFile) OldError(155);
#endif
	compiler->Accept(',');
	compiler->Accept('(');
	compiler->RdKFList(&PD->SK, PD->SortFD);
	compiler->Accept(')');
	return PD;
}

Instr_edit* RdEditCall()
{
	LocVar* lv = nullptr;
	Instr_edit* PD = new Instr_edit(); // GetPD(_edit, 8);
	compiler->RdLex();
	EditOpt* EO = &PD->EO;
	EO->UserSelFlds = true;

	if (IsRecVar(&lv)) {
		EO->LVRecPtr = lv->record;
		CFile = lv->FD;
	}
	else {
		CFile = compiler->RdFileName();
		XKey* K = compiler->RdViewKey();
		if (K == nullptr) K = CFile->Keys.empty() ? nullptr : CFile->Keys[0];
		EO->ViewKey = K;
	}
	PD->EditFD = CFile;
	compiler->Accept(',');
	if (compiler->IsOpt("U")) {
		compiler->TestIdentif();
		if (CFile->ViewNames == nullptr) compiler->Error(114);
		stSaveState* p = compiler->SaveCompState();
		bool b = RdUserView(CFile, LexWord, EO);
		compiler->RestoreCompState(p);
		if (!b) compiler->Error(114);
		compiler->RdLex();
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
	if (compiler->IsOpt("FIELD")) EO->StartFieldZ = compiler->RdStrFrml(nullptr);
	else if (EO->LVRecPtr != nullptr) compiler->Error(125);
	else if (compiler->IsOpt("OWNER")) {
		if (EO->SQLFilter || (EO->KIRoot != nullptr)) compiler->OldError(179);
		EO->OwnerTyp = RdOwner(&EO->DownLD, &EO->DownLV);
	}
	else if (compiler->IsOpt("RECKEY")) EO->StartRecKeyZ = compiler->RdStrFrml(nullptr);
	else if (
#ifdef FandSQL
		!CFile->typSQLFile &&
#endif
		compiler->IsOpt("RECNO")) EO->StartRecNoZ = compiler->RdRealFrml(nullptr);
	else if (compiler->IsOpt("IREC")) EO->StartIRecZ = compiler->RdRealFrml(nullptr);
	else if (compiler->IsKeyWord("CHECK")) EO->SyntxChk = true;
	else if (compiler->IsOpt("SEL")) {
		LocVar* lv = RdIdxVar();
		EO->SelKey = (XWKey*)lv->record;
		if ((EO->ViewKey == nullptr)) compiler->OldError(108);
		if (EO->ViewKey == EO->SelKey) compiler->OldError(184);
		if ((EO->ViewKey->KFlds != nullptr)
			&& (EO->SelKey->KFlds != nullptr)
			&& !KeyFldD::EquKFlds(EO->SelKey->KFlds, EO->ViewKey->KFlds)) compiler->OldError(178);
	}
	else compiler->Error(125);
}

Instr* RdReportCall()
{
	LocVar* lv = nullptr;
	RprtFDListEl* FDL = nullptr;
	Instr_report* PD = new Instr_report();
	compiler->RdLex();
	RprtOpt* RO = compiler->GetRprtOpt();
	PD->RO = RO;
	bool has_first = false;

	if (Lexem != ',') {
		has_first = true;
		FDL = &RO->FDL;
		bool b = false;
		if (Lexem == '(') {
			compiler->RdLex();
			b = true;
		}

		while (true) {
			if (IsRecVar(&lv)) {
				FDL->LVRecPtr = lv->record;
				FDL->FD = lv->FD;
			}
			else {
				CFile = compiler->RdFileName();
				FDL->FD = CFile;
				CViewKey = compiler->RdViewKey();
				FDL->ViewKey = CViewKey;
				if (Lexem == '(') {
					compiler->RdLex();
					FDL->Cond = compiler->RdKeyInBool(&FDL->KeyIn, true, true, FDL->SQLFilter, nullptr);
					compiler->Accept(')');
				}
			}
			if (b && (Lexem == ',')) {
				compiler->RdLex();
				FDL->Chain = new RprtFDListEl();
				FDL = FDL->Chain;
				continue;
			}
			break;
		}

		if (b) {
			compiler->Accept(')');
		}
		CFile = RO->FDL.FD;
		CViewKey = RO->FDL.ViewKey;
	}

	compiler->Accept(',');
	if (Lexem == '[') {
		compiler->RdLex();
		RO->RprtPos.rdb = (RdbD*)compiler->RdStrFrml(nullptr);
		RO->RprtPos.i_rec = 0;
		RO->FromStr = true;
		compiler->Accept(']');
	}
	else if (!has_first || (Lexem == _identifier)) {
		compiler->TestIdentif();
		if (!compiler->FindChpt('R', LexWord, false, &RO->RprtPos)) {
			compiler->Error(37);
		}
		compiler->RdLex();
	}
	else {
		compiler->Accept('(');
		switch (Lexem) {
		case '?': {
			RO->Flds = compiler->AllFldsList(CFile, false);
			compiler->RdLex();
			RO->UserSelFlds = true;
			break;
		}
		case ')': {
			RO->Flds = compiler->AllFldsList(CFile, true);
			break;
		}
		default: {
			RO->Flds = RdFlds();
			if (Lexem == '?') {
				compiler->RdLex();
				RO->UserSelFlds = true;
			}
			break;
		}
		}
		compiler->Accept(')');
	}
	while (Lexem == ',') {
		compiler->RdLex();
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

	if (compiler->IsOpt("ASSIGN")) {
		RdPath(true, RO->Path, RO->CatIRec);
	}
	else if (compiler->IsOpt("TIMES")) {
		RO->Times = compiler->RdRealFrml(nullptr);
	}
	else if (compiler->IsOpt("MODE")) {
		if (compiler->IsKeyWord("ONLYSUM")) {
			RO->Mode = _ATotal;
		}
		else if (compiler->IsKeyWord("ERRCHECK")) {
			RO->Mode = _AErrRecs;
		}
		else {
			compiler->Error(49);
		}
	}
	else if (compiler->IsKeyWord("COND")) {
		if (!has_first) {
			compiler->OldError(51);
			compiler->Accept('(');
			compiler->RdKFList(&RO->SK, CFile);
			compiler->Accept(')');
		}
		WORD Low = CurrPos;
		compiler->Accept(_equ);
		bool br = false;
		if (Lexem == '(') {
			Low = CurrPos;
			compiler->RdLex();
			br = true;
			if (Lexem == '?') {
				compiler->RdLex();
				RO->UserCondQuest = true;
				if (br) {
					compiler->Accept(')');
				}
				return;
			}
		}
		RO->FDL.Cond = compiler->RdKeyInBool(&RO->FDL.KeyIn, true, true, RO->FDL.SQLFilter, nullptr);
		N = OldErrPos - Low;
		RO->CondTxt = std::string((const char*)&InpArrPtr[Low], N);

		if (br) {
			compiler->Accept(')');
		}
	}
	else if (compiler->IsOpt("CTRL")) {
		if (!has_first) {
			compiler->OldError(51);
			compiler->Accept('(');
			compiler->RdKFList(&RO->SK, CFile);
			compiler->Accept(')');
		}
		RO->Ctrl = RdSubFldList(RO->Flds, 'C');
	}
	else if (compiler->IsOpt("SUM")) {
		if (!has_first) {
			compiler->OldError(51);
			compiler->Accept('(');
			compiler->RdKFList(&RO->SK, CFile);
			compiler->Accept(')');
		}
		RO->Sum = RdSubFldList(RO->Flds, 'S');
	}
	else if (compiler->IsOpt("WIDTH")) {
		RO->WidthFrml = compiler->RdRealFrml(nullptr);
	}
	else if (compiler->IsOpt("STYLE")) {
		if (compiler->IsKeyWord("COMPRESSED")) {
			RO->Style = 'C';
		}
		else {
			if (compiler->IsKeyWord("NORMAL")) {
				RO->Style = 'N';
			}
			else {
				compiler->Error(50);
			}
		}
	}
	else if (compiler->IsKeyWord("EDIT")) {
		RO->Edit = true;
	}
	else if (compiler->IsKeyWord("PRINTCTRL")) {
		RO->PrintCtrl = true;
	}
	else if (compiler->IsKeyWord("CHECK")) {
		RO->SyntxChk = true;
	}
	else if (compiler->IsOpt("SORT")) {
		if (!has_first) {
			compiler->OldError(51);
		}
		compiler->Accept('(');
		compiler->RdKFList(&RO->SK, CFile);
		compiler->Accept(')');
	}
	else if (compiler->IsOpt("HEAD")) {
		RO->Head = compiler->RdStrFrml(nullptr);
	}
	else {
		compiler->Error(45);
	}
}

Instr* RdRDBCall()
{
	std::string s;
	auto PD = new Instr_call(); // GetPD(_call, 12);
	compiler->RdLex();
	//s[0] = 0;
	if (Lexem == '\\') {
		s = "\\";
		compiler->RdLex();
	}
	compiler->TestIdentif();
	if (LexWord.length() > 8) compiler->Error(2);
	PD->RdbNm = s + std::string(LexWord);
	compiler->RdLex();
	if (Lexem == ',') {
		compiler->RdLex();
		compiler->TestIdentif();
		if (LexWord.length() > 12) compiler->Error(2);
		PD->ProcNm = LexWord;
		compiler->RdLex();
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
	compiler->RdLex();
	RdPath(true, PD->ProgPath, PD->ProgCatIRec);
	compiler->Accept(',');
	PD->Param = compiler->RdStrFrml(nullptr);
	while (Lexem == ',') {
		compiler->RdLex();
		if (compiler->IsKeyWord("NOCANCEL")) PD->NoCancel = true;
		else if (compiler->IsKeyWord("FREEMEM")) PD->FreeMm = true;
		else if (compiler->IsKeyWord("LOADFONT")) PD->LdFont = true;
		else if (compiler->IsKeyWord("TEXTMODE")) PD->TextMd = true;
		else compiler->Error(101);
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
	compiler->RdLex();
	noapp = false;
	CD = new CopyD(); // (CopyD*)GetZStore(sizeof(*D));
	PD->CD = CD;
	/* !!! with D^ do!!! */
	CD->FD1 = RdPath(false, CD->Path1, CD->CatIRec1);
	CD->WithX1 = RdX(CD->FD1);
	if (Lexem == '/') {
		if (CD->FD1 != nullptr) { CFile = CD->FD1; CD->ViewKey = compiler->RdViewKey(); }
		else CD->Opt1 = RdCOpt();
	}
	compiler->Accept(',');
	CD->FD2 = RdPath(false, CD->Path2, CD->CatIRec2);
	CD->WithX2 = RdX(CD->FD2);
	if (Lexem == '/') {
		if (CD->FD2 != nullptr) compiler->Error(139);
		else CD->Opt2 = RdCOpt();
	}
	if (!TestFixVar(CD->Opt1, CD->FD1, CD->FD2) && !TestFixVar(CD->Opt2, CD->FD2, CD->FD1))
	{
		if ((CD->Opt1 == CpOption::cpTxt) && (CD->FD2 != nullptr)) compiler->OldError(139);
		noapp = (CD->FD1 == nullptr) ^ (CD->FD2 == nullptr); // XOR
#ifdef FandSQL
		if (noapp)
			if ((FD1 != nullptr) && (FD1->typSQLFile) || (FD2 != nullptr)
				&& (FD2->typSQLFile)) OldError(155);
#endif
	}
	while (Lexem == ',') {
		compiler->RdLex();
		if (compiler->IsOpt("HEAD")) {
			CD->HdFD = compiler->RdFileName();
			compiler->Accept('.');
			CD->HdF = compiler->RdFldName(CD->HdFD);
			if ((CD->HdF->frml_type != 'S') || !CD->HdFD->IsParFile
				|| (CD->Opt1 == CpOption::cpFix || CD->Opt1 == CpOption::cpVar)
				&& ((CD->HdF->Flg & f_Stored) == 0)) compiler->Error(52);
		}
		else if (compiler->IsOpt("MODE")) {
			compiler->TestLex(_quotedstr);
			for (i = 0; i < 7; i++) {
				if (EquUpCase(LexWord, ModeTxt[i])) {
					CD->Mode = i + 1;
					goto label1;
				}
			}
			compiler->Error(142);
		label1:
			compiler->RdLex();
		}
		else if (compiler->IsKeyWord("NOCANCEL")) CD->NoCancel = true;
		else if (compiler->IsKeyWord("APPEND")) {
			if (noapp) compiler->OldError(139); CD->Append = true;
		}
		else compiler->Error(52);
	}
	return PD;
}

CpOption RdCOpt()
{
	BYTE i = 0;
	pstring OptArr[3] = { "FIX", "VAR", "TXT" };
	compiler->RdLex();
	compiler->TestIdentif();
	for (i = 0; i < 3; i++)
		if (EquUpCase(OptArr[i], LexWord)) {
			compiler->RdLex();
			return CpOption(i + 1); // vracime i + 1 (CpOption ma 4 moznosti, je to posunute ...)
		}
	compiler->Error(53);
	throw std::exception("Bad value in RdCOpt() in rdproc.cpp");
}

bool RdX(FileD* FD)
{
	auto result = false;
	if ((Lexem == '.') && (FD != nullptr)) {
		compiler->RdLex();
		compiler->AcceptKeyWord("X");
		if (FD->FF->file_type != FileType::INDEX) compiler->OldError(108);
		result = true;
	}
	return result;
}

bool TestFixVar(CpOption Opt, FileD* FD1, FileD* FD2)
{
	auto result = false;
	if ((Opt != CpOption::cpNo) && (FD1 != nullptr)) compiler->OldError(139);
	result = false;
	if (Opt == CpOption::cpFix || Opt == CpOption::cpVar) {
		result = true;
		if (FD2 == nullptr) compiler->OldError(139);
	}
	return result;
}

bool RdList(pstring* S)
{
	auto result = false;
	if (Lexem != '(') return result;
	compiler->RdLex();
	// TODO: compiler !!! S = (pstring*)(compiler->RdStrFrml);
	compiler->Accept(')');
	result = true;
	return result;
}

Instr* RdPrintTxt()
{
	auto PD = new Instr_edittxt(PInstrCode::_printtxt);
	compiler->RdLex();
	if (compiler->FindLocVar(&LVBD, &PD->TxtLV)) {
		compiler->RdLex();
		compiler->TestString(PD->TxtLV->FTyp);
	}
	else RdPath(true, PD->TxtPath, PD->TxtCatIRec);
	return PD;
}

Instr* RdEditTxt()
{
	EdExitD* pX;
	auto PD = new Instr_edittxt(PInstrCode::_edittxt);
	compiler->RdLex();
	if (compiler->FindLocVar(&LVBD, &PD->TxtLV)) {
		compiler->RdLex();
		compiler->TestString(PD->TxtLV->FTyp);
	}
	else RdPath(true, PD->TxtPath, PD->TxtCatIRec);
	PD->EdTxtMode = 'T';
	while (Lexem == ',') {
		compiler->RdLex();
		if (compiler->IsOpt("WW")) {
			compiler->Accept('(');
			if (Lexem == '(') { compiler->RdLex(); PD->WFlags = WNoPop; }
			compiler->RdW(PD->Ww);
			compiler->RdFrame(&PD->Hd, PD->WFlags);
			if (Lexem == ',') { compiler->RdLex(); PD->Atr = compiler->RdAttr(); }
			compiler->Accept(')');
			if ((PD->WFlags & WNoPop) != 0) compiler->Accept(')');
		}
		else
			if (compiler->IsOpt("TXTPOS")) PD->TxtPos = compiler->RdRealFrml(nullptr);
			else if (compiler->IsOpt("TXTXY")) PD->TxtXY = compiler->RdRealFrml(nullptr);
			else if (compiler->IsOpt("ERRMSG")) PD->ErrMsg = compiler->RdStrFrml(nullptr);
			else if (compiler->IsOpt("EXIT")) {
				compiler->Accept('(');
			label1:
				pX = new EdExitD(); // (EdExitD*)GetZStore(sizeof(*pX));
				PD->ExD.push_back(pX);
			label2:
				RdKeyCode(pX);
				if (Lexem == ',') { compiler->RdLex(); goto label2; }
				compiler->Accept(':');
				if (compiler->IsKeyWord("QUIT")) pX->Typ = 'Q';
				else if (!(Lexem == ',' || Lexem == ')')) {
					pX->Typ = 'P';
					pX->Proc = RdProcArg('T');
				}
				if (Lexem == ',') { compiler->RdLex(); goto label1; }
				compiler->Accept(')');
			}
			else
				if (RdHeadLast(PD)) {}
				else if (compiler->IsKeyWord("NOEDIT")) PD->EdTxtMode = 'V';
				else compiler->Error(161);
	}
	return PD;
}

Instr* RdPutTxt()
{
	auto PD = new Instr_puttxt(); // GetPD(_puttxt, 11);
	compiler->RdLex();
	RdPath(true, PD->TxtPath1, PD->TxtCatIRec1);
	compiler->Accept(',');
	PD->Txt = compiler->RdStrFrml(nullptr);
	if (Lexem == ',') {
		compiler->RdLex();
		compiler->AcceptKeyWord("APPEND");
		PD->App = true;
	}
	return PD;
}

Instr* RdTurnCat()
{
	Instr_turncat* PD = new Instr_turncat();
	compiler->RdLex();
	compiler->TestIdentif();
	PD->NextGenFD = compiler->FindFileD();
	const int first = CatFD->GetCatalogIRec(LexWord, true);
	TestCatError(first, LexWord, true);
	compiler->RdLex();
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
		compiler->OldError(98);
	}
	PD->NCatIRecs = i - first;
	compiler->Accept(',');
	PD->TCFrml = compiler->RdRealFrml(nullptr);
	return PD;
}

void RdWriteln(WriteType OpKind, Instr_writeln** pinstr)
{
	WrLnD* d = new WrLnD();
	compiler->RdLex();
	FrmlElem* z = nullptr;
	WrLnD* w = d;
label1:
	w->Frml = compiler->RdFrml(w->Typ, nullptr);
	if (w->Typ == 'R') {
		w->Typ = 'F';
		if (Lexem == ':') {
			compiler->RdLex();
			if (Lexem == _quotedstr) {
				w->Typ = 'D';
				w->Mask = StoreStr(LexWord);
				compiler->RdLex();
			}
			else {
				w->N = compiler->RdInteger();
				if (Lexem == ':') {
					compiler->RdLex();
					if (Lexem == '-') {
						compiler->RdLex();
						w->M = -compiler->RdInteger();
					}
					else w->M = compiler->RdInteger();
				}
			}
		}
	}
	if (Lexem == ',') {
		compiler->RdLex();
		if ((OpKind == WriteType::message) && compiler->IsOpt("HELP")) {
			z = compiler->RdStrFrml(nullptr);
		}
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
	compiler->RdLex();
	PD->Drive = compiler->RdStrFrml(nullptr);
	return PD;
}

Instr* RdIndexfile()
{
	auto PD = new Instr_indexfile(); // GetPD(_indexfile, 5);
	compiler->RdLex();
	PD->IndexFD = compiler->RdFileName();
	if (PD->IndexFD->FF->file_type != FileType::INDEX) compiler->OldError(108);
	if (Lexem == ',') {
		compiler->RdLex();
		compiler->AcceptKeyWord("COMPRESS");
		PD->Compress = true;
	}
	return PD;
}

Instr* RdGetIndex()
{
	LocVar* lv2 = nullptr; bool b = false; LinkD* ld = nullptr;
	auto PD = new Instr_getindex(); // GetPD(_getindex, 31);
	compiler->RdLex();
	LocVar* lv = RdIdxVar();
	PD->loc_var1 = lv; compiler->Accept(',');
	PD->mode = ' ';
	if (Lexem == '+' || Lexem == '-') {
		PD->mode = Lexem;
		compiler->RdLex();
		compiler->Accept(',');
		PD->condition = compiler->RdRealFrml(nullptr); /*RecNr*/
		return PD;
	}
	CFile = compiler->RdFileName();
	if (lv->FD != CFile) compiler->OldError(164);
	CViewKey = compiler->RdViewKey();
	PD->keys = CViewKey;
	while (Lexem == ',') {
		compiler->RdLex();
		if (compiler->IsOpt("SORT")) {
			if (((XWKey*)lv->record)->KFlds != nullptr) compiler->OldError(175);
			compiler->Accept('(');
			compiler->RdKFList(&PD->key_fields, CFile);
			compiler->Accept(')');
		}
		else if (compiler->IsOpt("COND")) {
			compiler->Accept('(');
			PD->condition = compiler->RdKeyInBool(&PD->key_in_root, false, true, PD->sql_filter, nullptr);
			compiler->Accept(')');
		}
		else if (compiler->IsOpt("OWNER")) {
			PD->owner_type = RdOwner(&PD->link, &PD->loc_var2);
			XKey* k = GetFromKey(PD->link);
			if (CViewKey == nullptr) PD->keys = k;
			else if (CViewKey != k) compiler->OldError(178);
		}
		else compiler->Error(167);
		if ((PD->owner_type != 0) && (PD->sql_filter || (PD->key_in_root != nullptr)))
			compiler->Error(179);
	}
	return PD;
}

Instr* RdGotoXY()
{
	auto PD = new Instr_gotoxy(); // GetPD(_gotoxy, 8);
	compiler->RdLex();
	PD->GoX = compiler->RdRealFrml(nullptr);
	compiler->Accept(',');
	PD->GoY = compiler->RdRealFrml(nullptr);
	return PD;
}

Instr* RdClrWw()
{
	auto PD = new Instr_clrww(); // GetPD(_clrww, 24);
	compiler->RdLex();
	compiler->RdW(PD->W2);
	if (Lexem == ',') {
		compiler->RdLex();
		if (Lexem != ',') PD->Attr2 = compiler->RdAttr();
		if (Lexem == ',') { compiler->RdLex(); PD->FillC = compiler->RdStrFrml(nullptr); }
	}
	return PD;
}

Instr* RdMount()
{
	auto PD = new Instr_mount(); // GetPD(_mount, 3);
	compiler->RdLex();
	int i = 0;
	compiler->TestIdentif();
	FileD* FD = compiler->FindFileD();
	if (FD == nullptr) {
		i = CatFD->GetCatalogIRec(LexWord, true);
	}
	else {
		i = FD->CatIRec;
	}
	TestCatError(i, LexWord, false);
	compiler->RdLex();
	PD->MountCatIRec = i;
	if (Lexem == ',') {
		compiler->RdLex();
		compiler->AcceptKeyWord("NOCANCEL");
		PD->MountNoCancel = true;
	}
	return PD;
}

Instr* RdDisplay()
{
	auto PD = new Instr_merge_display(PInstrCode::_display); // GetPD(_display, sizeof(RdbPos));
	compiler->RdLex();
	pstring* s = nullptr;
	if ((Lexem == _identifier) && compiler->FindChpt('H', LexWord, false, &PD->Pos)) {
		compiler->RdLex();
	}
	else {
		PD->Pos.rdb = (RdbD*)compiler->RdStrFrml(nullptr);
		PD->Pos.i_rec = 0;
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
	compiler->RdLex();
	PD->GD = new GraphD();

	auto PDGD = PD->GD;
	if (compiler->IsOpt("GF")) PDGD->GF = compiler->RdStrFrml(nullptr);
	else {
		PDGD->FD = compiler->RdFileName();
		CFile = PDGD->FD;
		CViewKey = compiler->RdViewKey();
		PDGD->ViewKey = CViewKey;
		compiler->Accept(',');
		compiler->Accept('(');
		PDGD->X = compiler->RdFldName(PDGD->FD);
		i = 0;
		do {
			compiler->Accept(',');
			PDGD->ZA[i] = compiler->RdFldName(PDGD->FD);
			i++;
		} while (!((i > 9) || (Lexem != ',')));
		compiler->Accept(')');
	}
	while (Lexem == ',') {
		compiler->RdLex();
		for (i = 0; i < 11; i++) {
			if (compiler->IsOpt(Nm1[i])) {
				FrmlArr[0] = (FrmlElem*)(&PDGD->T);
				FrmlArr[i] = compiler->RdStrFrml(nullptr);
				goto label1;
			}
		}
		for (i = 0; i < 6; i++) {
			if (compiler->IsOpt(Nm2[i])) {
				FrmlArr[0] = (FrmlElem*)(&PDGD->S);
				FrmlArr[i] = compiler->RdRealFrml(nullptr);
				goto label1;
			}
		}
		if (compiler->IsDigitOpt("HEADZ", i)) PDGD->HZA[i] = compiler->RdStrFrml(nullptr);
		else if (compiler->IsKeyWord("INTERACT")) PDGD->Interact = true;
		else if (compiler->IsOpt("COND")) {
			if (Lexem == '(') {
				compiler->RdLex();
				PDGD->Cond = compiler->RdKeyInBool(&PDGD->KeyIn, false, true, PDGD->SQLFilter, nullptr);
				compiler->Accept(')');
			}
			else PDGD->Cond = compiler->RdKeyInBool(&PDGD->KeyIn, false, true, PDGD->SQLFilter, nullptr);
		}
		else if (compiler->IsOpt("TXT")) {
			VD = new GraphVD();
			ChainLast(PDGD->V, VD);
			compiler->Accept('(');
			VD->XZ = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			VD->YZ = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			VD->Velikost = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			VD->BarPis = compiler->RdStrFrml(nullptr); compiler->Accept(',');
			VD->Text = compiler->RdStrFrml(nullptr); compiler->Accept(')');
		}
		else if (compiler->IsOpt("TXTWIN")) {
			WD = new GraphWD();
			ChainLast(PDGD->W, WD); compiler->Accept('(');
			WD->XZ = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			WD->YZ = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			WD->XK = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			WD->YK = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			WD->BarPoz = compiler->RdStrFrml(nullptr); compiler->Accept(',');
			WD->BarPis = compiler->RdStrFrml(nullptr); compiler->Accept(',');
			WD->Text = compiler->RdStrFrml(nullptr); compiler->Accept(')');
		}
		else if (compiler->IsOpt("RGB")) {
			RGBD = new GraphRGBD();
			ChainLast(PDGD->RGB, RGBD);
			compiler->Accept('(');
			RGBD->Barva = compiler->RdStrFrml(nullptr);
			compiler->Accept(',');
			RGBD->R = compiler->RdRealFrml(nullptr);
			compiler->Accept(',');
			RGBD->G = compiler->RdRealFrml(nullptr);
			compiler->Accept(',');
			RGBD->B = compiler->RdRealFrml(nullptr);
			compiler->Accept(')');
		}
		else if (compiler->IsOpt("WW")) {
			Ww = new WinG();
			compiler->Accept('(');
			if (Lexem == '(') { compiler->RdLex(); Ww->WFlags = WNoPop; }
			compiler->RdW(Ww->W);
			compiler->RdFrame(Ww->Top, Ww->WFlags);
			if (Lexem == ',') {
				compiler->RdLex();
				Ww->ColBack = compiler->RdStrFrml(nullptr); compiler->Accept(',');
				Ww->ColFor = compiler->RdStrFrml(nullptr); compiler->Accept(',');
				Ww->ColFrame = compiler->RdStrFrml(nullptr);
			}
			compiler->Accept(')');
			if ((Ww->WFlags & WNoPop) != 0) compiler->Accept(')');
		}
		else {
			compiler->Error(44);
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
		// PD = GetPD(Op, 9);
		PD = new Instr_recs(Op);
		compiler->RdLex();
		CFile = compiler->RdFileName();
		PD->RecFD = CFile;
#ifdef FandSQL
		if (CFile->typSQLFile) OldError(155);
#endif
		if (Op == PInstrCode::_recallrec) {
			compiler->Accept(',');
			PD->RecNr = compiler->RdRealFrml(nullptr);
	}
}
	else {
		// PD = GetPD(Op, 15);
		PD = new Instr_recs(Op);
		compiler->RdLex();
		if (Op == PInstrCode::_deleterec) {
			CFile = compiler->RdFileName();
			PD->RecFD = CFile;
		}
		else { /*_readrec,_writerec*/
			if (!IsRecVar(&PD->LV)) compiler->Error(141);
			CFile = PD->LV->FD;
		}
		XKey* K = compiler->RdViewKey();
		compiler->Accept(',');
#ifdef FandSQL
		if (CFile->typSQLFile
			&& (Lexem == _equ || Lexem == _le || Lexem == _gt || Lexem == _lt || Lexem == _ge))
		{
			PD->CompOp = Lexem; RdLex();
		}
#endif
		Z = compiler->RdFrml(FTyp, nullptr);
		PD->RecNr = Z;
			switch (FTyp) {
			case 'B': compiler->OldError(12); break;
			case 'S': {
				PD->ByKey = true;
				if (PD->CompOp == 0) PD->CompOp = _equ;
				if (K == nullptr) K = CFile->Keys.empty() ? nullptr : CFile->Keys[0];
				PD->Key = K;
				if ((K == nullptr) && (!CFile->IsParFile || (Z->Op != _const)
					|| (((FrmlElemString*)Z)->S.length() > 0))) compiler->OldError(24);
				break;
			}
#ifdef FandSQL
			default: {
				if (PD->CompOp != 0) OldError(19);
				if (CFile->typSQLFile && ((Op == _deleterec) || (Z->Op != _const)
					|| (Z->rdb != 0))) Error(155);
				break;
			}
#endif
			}
			}
	if ((Lexem == ',') && (Op == PInstrCode::_writerec || Op == PInstrCode::_deleterec || Op == PInstrCode::_recallrec)) {
		compiler->RdLex();
		compiler->Accept('+');
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
	compiler->RdLex();
	if (!IsRecVar(&PD->RecLV1)) compiler->Error(141);
	compiler->Accept(',');
	CFile = PD->RecLV1->FD;
	if (IsRecVar(&LV)) {
		LD = compiler->FindLD(LV->FD->Name);
		if (LD == nullptr) compiler->OldError(154);
	}
	else {
		compiler->TestIdentif();
		LD = compiler->FindLD(LexWord);
		if (LD == nullptr) compiler->Error(9);
		compiler->RdLex();
		compiler->Accept('(');
		LV = RdRecVar();
		if (LD->ToFD != LV->FD) compiler->OldError(141);
		compiler->Accept(')');
	}
	PD->RecLV2 = LV;
	PD->LinkLD = LD;
	return PD;
}

Instr* RdSetEditTxt()
{
	auto PD = new Instr_setedittxt();
	compiler->RdLex();
label1:
	if (compiler->IsOpt("OVERWR")) PD->Insert = compiler->RdBool(nullptr);
	else if (compiler->IsOpt("INDENT")) PD->Indent = compiler->RdBool(nullptr);
	else if (compiler->IsOpt("WRAP")) PD->Wrap = compiler->RdBool(nullptr);
	else if (compiler->IsOpt("ALIGN")) PD->Just = compiler->RdBool(nullptr);
	else if (compiler->IsOpt("COLBLK")) PD->ColBlk = compiler->RdBool(nullptr);
	else if (compiler->IsOpt("LEFT")) PD->Left = compiler->RdRealFrml(nullptr);
	else if (compiler->IsOpt("RIGHT")) PD->Right = compiler->RdRealFrml(nullptr);
	else compiler->Error(160);
	if (Lexem == ',') { compiler->RdLex(); goto label1; }
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
	Z = new FrmlElemFunction(Op, 0); // GetOp(Op, 0);
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
			FieldDescr* F2 = compiler->FindFldName(FD2);
			if (F2 != nullptr) {
				AssignD* A = new AssignD();
				if (ARoot == nullptr) ARoot = A;
				else ChainLast(ARoot, A);
				if ((F2->frml_type != F1->frml_type)
					|| (F1->frml_type == 'R')
					&& (F1->field_type != F2->field_type)) {
					A->Kind = MInstrCode::_zero;
					A->outputFldD = F1;
				}
				else {
					A->Kind = MInstrCode::_output;
					A->OFldD = F1;
					FrmlElem* Z = compiler->MakeFldFrml(F2, FTyp);
					Z = AdjustComma(Z, F2, _divide);
					A->Frml = compiler->FrmlContxt(AdjustComma(Z, F1, _times), FD2, nullptr);
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
		if (compiler->FindLocVar(&LVBD, &LV) && (LV->FTyp == 'r' || LV->FTyp == 'i')) {
			FTyp = LV->FTyp;
			compiler->RdLex(); compiler->RdLex();
			if (FTyp == 'i') {
				compiler->AcceptKeyWord("NRECS");
				compiler->Accept(_assign);
				if ((Lexem != _number) || (LexWord != "0")) compiler->Error(183);
				compiler->RdLex();
				PD = new Instr_assign(PInstrCode::_asgnxnrecs); // GetPInstr(_asgnxnrecs, 4);
				PD->xnrIdx = (XWKey*)LV->record;
			}
			else {
				PD = new Instr_assign(PInstrCode::_asgnrecfld); // GetPInstr(_asgnrecfld, 13);
				PD->AssLV = LV;
				F = compiler->RdFldName(LV->FD);
				PD->RecFldD = F;
				if ((F->Flg & f_Stored) == 0) compiler->OldError(14);
				FTyp = F->frml_type;
			label0:
				compiler->RdAssignFrml(FTyp, PD->Add, &PD->Frml, nullptr);
			}
		}
		else {
			FName = LexWord;
			FD = compiler->FindFileD();
			if (FD->IsActiveRdb()) compiler->Error(121);
			compiler->RdLex(); compiler->RdLex();
			if (compiler->IsKeyWord("ARCHIVES")) {
				F = CatFD->CatalogArchiveField();
				goto label1;
			}
			if (compiler->IsKeyWord("PATH")) {
				F = CatFD->CatalogPathNameField();
				goto label1;
			}
			if (compiler->IsKeyWord("VOLUME")) {
				F = CatFD->CatalogVolumeField();
			label1:
				PD = new Instr_assign(PInstrCode::_asgnCatField);
				PD->FD3 = FD;
				PD->CatIRec = CatFD->GetCatalogIRec(FName, true);
				PD->CatFld = F;
				TestCatError(PD->CatIRec, FName, true);
				compiler->Accept(_assign);
				PD->Frml3 = compiler->RdStrFrml(nullptr);
			}
			else if (FD == nullptr) compiler->OldError(9);
			else if (compiler->IsKeyWord("NRECS")) {
				if (FD->FF->file_type == FileType::RDB) { compiler->OldError(127); }
				PD = new Instr_assign(PInstrCode::_asgnnrecs);
				PD->FD = FD;
				FTyp = 'R';
				goto label0;
			}
			else {
				if (!FD->IsParFile) compiler->OldError(64);
				PD = new Instr_assign(PInstrCode::_asgnpar); // GetPInstr(_asgnpar, 13);
				PD->FD = FD;
				F = compiler->RdFldName(FD);
				PD->FldD = F;
				if ((F->Flg & f_Stored) == 0) compiler->OldError(14);
				FTyp = F->frml_type;
				goto label0;
			}
		}
	else if (ForwChar == '[') {
		PD = new Instr_assign(PInstrCode::_asgnField); // GetPInstr(_asgnField, 18);
		FD = compiler->RdFileName();
		PD->FD = FD; compiler->RdLex();
#ifdef FandSQL
		if (rdb_file->typSQLFile) OldError(155);
#endif
		PD->RecFrml = compiler->RdRealFrml(nullptr);
		compiler->Accept(']');
		compiler->Accept('.');
		F = compiler->RdFldName(FD);
		PD->FldD = F;
		if ((F->Flg & f_Stored) == 0) compiler->OldError(14);
		PD->Indexarg = (FD->FF->file_type == FileType::INDEX) && compiler->IsKeyArg(F, FD);
		compiler->RdAssignFrml(F->frml_type, PD->Add, &PD->Frml, nullptr);
	}
	else if (compiler->FindLocVar(&LVBD, &LV)) {
		compiler->RdLex();
		FTyp = LV->FTyp;
		switch (FTyp) {
		case 'f':
		case 'i': compiler->OldError(140); break;
		case 'r': {
			compiler->Accept(_assign);
			if (!IsRecVar(&LV2)) compiler->Error(141);
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
	else if (compiler->IsKeyWord("USERNAME"))
	{
		PD = new Instr_assign(PInstrCode::_asgnusername); // GetPInstr(_asgnusername, 4);
		goto label2;
	}
	else if (compiler->IsKeyWord("CLIPBD"))
	{
		PD = new Instr_assign(PInstrCode::_asgnClipbd); // GetPInstr(_asgnClipbd, 4);
		goto label2;
	}
	else if (compiler->IsKeyWord("ACCRIGHT")) {
		PD = new Instr_assign(PInstrCode::_asgnAccRight); // GetPInstr(_asgnAccRight, 4);
	label2:
		compiler->Accept(_assign);
		PD->Frml = compiler->RdStrFrml(nullptr);
	}
	else if (compiler->IsKeyWord("EDOK")) {
		PD = new Instr_assign(PInstrCode::_asgnEdOk); // GetPInstr(_asgnEdOk, 4);
		compiler->Accept(_assign);
		PD->Frml = compiler->RdBool(nullptr);
	}
	else if (compiler->IsKeyWord("RANDSEED"))
	{
		PD = new Instr_assign(PInstrCode::_asgnrand); // GetPInstr(_asgnrand, 4);
		goto label3;
	}
	else if (compiler->IsKeyWord("TODAY"))
	{
		PD = new Instr_assign(PInstrCode::_asgnusertoday); // GetPInstr(_asgnusertoday, 4);
		goto label3;
	}
	else if (compiler->IsKeyWord("USERCODE")) {
		PD = new Instr_assign(PInstrCode::_asgnusercode); // GetPInstr(_asgnusercode, 4);
	label3:
		compiler->Accept(_assign);
		PD->Frml = compiler->RdRealFrml(nullptr);
	}
	else {
		compiler->RdLex();
		if (Lexem == _assign) compiler->OldError(8);
		else compiler->OldError(34);
	}
	return PD;
}

Instr* RdWith()
{
	Instr* P = nullptr; Instr* p2 = nullptr; PInstrCode Op;
	if (compiler->IsKeyWord("WINDOW")) {
		P = new Instr_window(); //GetPInstr(_window, 29);
		auto iP = (Instr_window*)P;
		compiler->Accept('(');
		if (Lexem == '(') {
			compiler->RdLex();
			iP->WithWFlags = WNoPop;
		}
		compiler->RdW(iP->W);
		compiler->RdFrame(&iP->Top, iP->WithWFlags);
		if (Lexem == ',') {
			compiler->RdLex();
			iP->Attr = compiler->RdAttr();
		}
		compiler->Accept(')');
		if ((iP->WithWFlags & WNoPop) != 0) compiler->Accept(')');
		compiler->AcceptKeyWord("DO");
		iP->WwInstr = RdPInstr();
	}
	else if (compiler->IsKeyWord("SHARED")) { Op = PInstrCode::_withshared; goto label1; }
	else if (compiler->IsKeyWord("LOCKED")) {
		Op = PInstrCode::_withlocked;
	label1:
		P = new Instr_withshared(Op); // GetPInstr(Op, 9 + sizeof(LockD));
		auto iP = (Instr_withshared*)P;
		LockD* ld = &iP->WLD;
	label2:
		ld->FD = compiler->RdFileName();
		if (Op == PInstrCode::_withlocked) {
			compiler->Accept('[');
			ld->Frml = compiler->RdRealFrml(nullptr);
			compiler->Accept(']');
		}
		else {
			compiler->Accept('(');
			for (LockMode i = NoExclMode; i <= ExclMode; i = (LockMode)(i + 1)) {
				if (compiler->IsKeyWord(LockModeTxt[i])) {
					ld->Md = i;
					goto label3;
				}
			}
			compiler->Error(100);
		label3:
			compiler->Accept(')');
		}
		if (Lexem == ',') {
			compiler->RdLex();
			ld->Chain = new LockD();
			ld = ld->Chain;
			goto label2;
		}
		compiler->AcceptKeyWord("DO");
		iP->WDoInstr = RdPInstr();
		if (compiler->IsKeyWord("ELSE")) {
			iP->WasElse = true;
			iP->WElseInstr = RdPInstr();
		}
	}
	else if (compiler->IsKeyWord("GRAPHICS")) {
		P = new Instr_withshared(PInstrCode::_withgraphics);
		compiler->AcceptKeyWord("DO");
		((Instr_withshared*)P)->WDoInstr = RdPInstr();
	}
	else {
		compiler->Error(131);
	}
	return P;
}

Instr_assign* RdUserFuncAssign()
{
	LocVar* lv = nullptr;
	if (!compiler->FindLocVar(&LVBD, &lv)) {
		compiler->Error(34);
	}
	compiler->RdLex();
	Instr_assign* pd = new Instr_assign(PInstrCode::_asgnloc);
	pd->AssLV = lv;
	compiler->RdAssignFrml(lv->FTyp, pd->Add, &pd->Frml, nullptr);
	return pd;
}

Instr* RdPInstr()
{
	Instr* result = nullptr;
	if (compiler->IsKeyWord("IF")) result = RdIfThenElse();
	else if (compiler->IsKeyWord("WHILE")) result = RdWhileDo();
	else if (compiler->IsKeyWord("REPEAT")) result = RdRepeatUntil();
	else if (compiler->IsKeyWord("CASE")) result = RdCase();
	else if (compiler->IsKeyWord("FOR")) result = RdFor();
	else if (compiler->IsKeyWord("BEGIN")) result = RdBeginEnd();
	else if (compiler->IsKeyWord("BREAK")) result = new Instr(PInstrCode::_break);
	else if (compiler->IsKeyWord("EXIT")) result = new Instr(PInstrCode::_exitP);
	else if (compiler->IsKeyWord("CANCEL")) result = new Instr(PInstrCode::_cancel);
	else if (Lexem == ';') result = nullptr;
	else if (IsRdUserFunc) result = RdUserFuncAssign();
	else if (compiler->IsKeyWord("MENULOOP")) result = RdMenuBox(true);
	else if (compiler->IsKeyWord("MENU")) result = RdMenuBox(false);
	else if (compiler->IsKeyWord("MENUBAR")) result = RdMenuBar();
	else if (compiler->IsKeyWord("WITH")) result = RdWith();
	else if (compiler->IsKeyWord("SAVE")) result = new Instr(PInstrCode::_save);
	else if (compiler->IsKeyWord("CLREOL")) result = new Instr(PInstrCode::_clreol);
	else if (compiler->IsKeyWord("FORALL")) result = RdForAll();
	else if (compiler->IsKeyWord("CLEARKEYBUF")) result = new Instr(PInstrCode::_clearkeybuf);
	else if (compiler->IsKeyWord("WAIT")) result = new Instr(PInstrCode::_wait);
	else if (compiler->IsKeyWord("BEEP")) result = new Instr(PInstrCode::_beepP);
	else if (compiler->IsKeyWord("NOSOUND")) result = new Instr(PInstrCode::_nosound);
#ifndef FandRunV
	else if (compiler->IsKeyWord("MEMDIAG")) result = new Instr(PInstrCode::_memdiag);
#endif 
	else if (compiler->IsKeyWord("RESETCATALOG")) result = new Instr(PInstrCode::_resetcat);
	else if (compiler->IsKeyWord("RANDOMIZE")) result = new Instr(PInstrCode::_randomize);
	else if (Lexem == _identifier) {
		compiler->SkipBlank(false);
		if (ForwChar == '(') RdProcCall(&result); // funkce muze ovlivnit RESULT
		else if (compiler->IsKeyWord("CLRSCR")) result = new Instr(PInstrCode::_clrscr);
		else if (compiler->IsKeyWord("GRAPH")) result = new Instr_graph();
		else if (compiler->IsKeyWord("CLOSE")) result = new Instr_closefds();
		else result = RdAssign();
	}
	else compiler->Error(34);
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
	compiler->RdLex();
	ResetLVBD();
	LVBD.FceName = name;
	if (Lexem == '(') {
		compiler->RdLex();
		compiler->RdLocDcl(&LVBD, true, true, 'P');
		compiler->Accept(')');
	}
	if (compiler->IsKeyWord("VAR")) {
		compiler->RdLocDcl(&LVBD, false, true, 'P');
	}
}

Instr* ReadProcBody()
{
	compiler->AcceptKeyWord("BEGIN");
	Instr* result = RdBeginEnd();
	compiler->Accept(';');
	if (Lexem != 0x1A) {
		std::string error40 = compiler->Error(40);
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
	compiler->RdLex();
	while (true) {
		if (compiler->IsKeyWord("FUNCTION")) {
			compiler->TestIdentif();
			fc = FuncDRoot;
			while (fc != CRdb->OldFCRoot) {
				if (EquUpCase(fc->Name, LexWord)) compiler->Error(26);
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
			compiler->RdLex();
			ResetLVBD();
			LVBD.FceName = fc->Name;
			compiler->Accept('(');
			if (Lexem != ')') compiler->RdLocDcl(&LVBD, true, false, 'D'); // nacte parametry funkce
			compiler->Accept(')');
			compiler->Accept(':');
			// nacte typ navratove hodnoty
			if (compiler->IsKeyWord("REAL")) {
				typ = 'R';
				n = sizeof(double);
			}
			else if (compiler->IsKeyWord("STRING")) {
				typ = 'S';
				n = sizeof(int);
			}
			else if (compiler->IsKeyWord("BOOLEAN")) {
				typ = 'B';
				n = sizeof(bool);
			}
			else compiler->Error(39);
			lv = new LocVar();
			LVBD.vLocVar.push_back(lv);
			lv->Name = fc->Name;
			lv->IsRetValue = true;
			lv->FTyp = typ;
			lv->Op = _getlocvar;
			fc->FTyp = typ;
			compiler->Accept(';');
			// nacte promenne
			if (compiler->IsKeyWord("VAR")) compiler->RdLocDcl(&LVBD, false, false, 'D');
			fc->LVB = LVBD;
			// nacte kod funkce (procedury)
			compiler->AcceptKeyWord("BEGIN");
			fc->pInstr = RdBeginEnd();
			compiler->Accept(';');
		}
		else if (Lexem == 0x1A) return;
		else {
			compiler->Error(40);
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
	p = compiler->SaveCompState();
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
	compiler->SetInpStdStr(s, false);
	compiler->RdLex();
	z = compiler->RdFrml(fTyp, nullptr);
	if ((fTyp != X->EvalTyp) || (Lexem != 0x1A)) z = nullptr;
	else LastExitCode = 0;
label1:
	cpos = CurrPos;
	//RestoreExit(er);
	compiler->RestoreCompState(p);
	if (LastExitCode != 0) {
		LastTxtPos = cpos;
		if (X->EvalTyp == 'B') {
			z = new FrmlElemBool(_const, 0, false); // GetOp(_const, 1);
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
		PD = new Instr_backup(PInstrCode::_backupm);
	}
	else {
		PD = new Instr_backup(PInstrCode::_backup);
	}

	compiler->RdLex();
	PD->IsBackup = IsBackup;
	compiler->TestIdentif();

	bool found = false;
	for (int i = 1; i <= CatFD->GetCatalogFile()->FF->NRecs; i++) {
		if (EquUpCase(CatFD->GetRdbName(i), "ARCHIVES") && EquUpCase(CatFD->GetFileName(i), LexWord)) {
			compiler->RdLex();
			PD->BrCatIRec = i;
			found = true;
		}
	}

	if (!found) {
		compiler->Error(88);
		return nullptr;
	}
	else {
		if (MTyp == 'M') {
			compiler->Accept(',');
			PD->bmDir = compiler->RdStrFrml(nullptr);
			if (IsBackup) {
				compiler->Accept(',');
				PD->bmMasks = compiler->RdStrFrml(nullptr);
			}
		}
		while (Lexem == ',') {
			compiler->RdLex();
			if (MTyp == 'M') {
				if (!IsBackup && compiler->IsKeyWord("OVERWRITE")) {
					PD->bmOverwr = true;
					continue;
				}
				if (compiler->IsKeyWord("SUBDIR")) {
					PD->bmSubDir = true;
					continue;
				}
			}
			if (compiler->IsKeyWord("NOCOMPRESS"))
			{
				PD->NoCompress = true;
			}
			else {
				compiler->AcceptKeyWord("NOCANCEL");
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
	compiler->RdLex();
	compiler->RdChptName('L', &pd->lpPos, true);
	if (Lexem == ',') {
		compiler->RdLex();
		compiler->TestIdentif();
		pd->lpName = LexWord;
		compiler->RdLex();
	}
	return pd;
}
