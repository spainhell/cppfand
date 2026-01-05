#include "rdproc.h"
#include "Compiler.h"
#include "../Common/CommonVariables.h"
#include "../fandio/FieldDescr.h"
#include "../Common/FileD.h"
#include "GlobalVariables.h"
#include "../fandio/KeyFldD.h"
#include "../Common/LinkD.h"
#include "rdfildcl.h"
#include "rdrun.h"
#include "runfrml.h"
#include "../Common/exprcmp.h"
#include "../Common/compare.h"
#include "models/Instr.h"
#include "../fandio/XWKey.h"
#include "../Drivers/constants.h"

struct key_names { std::string name; uint8_t brk; uint16_t code; };
static std::vector<key_names> KeyNames = {
	{"HOME", 51, __HOME},
	{"UP", 52, __UP},
	{"PGUP", 53, __PAGEUP},
	{"LEFT", 55, __LEFT},
	{"RIGHT", 57, __RIGHT},
	{"END", 59, __END},
	{"DOWN", 60, __DOWN},
	{"PGDN", 61, __PAGEDOWN},
	{"INS", 62, __INSERT},
	{"CTRLLEFT", 71, __CTRL_LEFT},
	{"CTRLRIGHT", 72, __CTRL_RIGHT},
	{"CTRLEND", 73, __CTRL_END},
	{"CTRLPGDN", 74, __CTRL_PAGEDOWN},
	{"CTRLHOME", 75, __CTRL_HOME},
	{"CTRLPGUP", 76, __CTRL_PAGEUP},
	{"TAB", 77, VK_TAB},
	{"SHIFTTAB", 78, SHIFT + VK_TAB},
	{"CTRLN", 79, CTRL + 'N'},
	{"CTRLY", 80, CTRL + 'Y'},
	{"ESC", 81, __ESC},
	{"CTRLP", 82, CTRL + 'P'} };

void AddInstr(std::vector<Instr*>& dst, std::vector<Instr*> src)
{
	for (auto& instr : src) {
		dst.push_back(instr);
	}
}

void TestCatError(Compiler* compiler, int i, const std::string& name, bool old)
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

bool IsRecVar(Compiler* compiler, LocVar** LV)
{
	if (!compiler->FindLocVar(&LVBD, LV) || ((*LV)->f_typ != 'r')) return false;
	compiler->RdLex();
	return true;
}

LocVar* RdRecVar(Compiler* compiler)
{
	LocVar* LV = nullptr;
	if (!IsRecVar(compiler, &LV)) compiler->Error(141);
	return LV;
}

LocVar* RdIdxVar(Compiler* compiler)
{
	LocVar* lv = nullptr;
	if (!compiler->FindLocVar(&LVBD, &lv) || (lv->f_typ != 'i')) compiler->Error(165);
	auto result = lv;
	compiler->RdLex();
	return result;
}

FrmlElem* RdRecVarFldFrml(Compiler* compiler, LocVar* LV, char& FTyp)
{
	FrmlElem* Z = nullptr;
	compiler->Accept('.');

	switch (LV->f_typ) {
	case 'r': {
		FrmlElemRecVarField* fe7 = new FrmlElemRecVarField(_recvarfld, 12);
		FileD* previous = compiler->processing_F;
		compiler->processing_F = LV->FD;
		fe7->File = LV->FD;
		fe7->record = LV->record;
		bool fa = FileVarsAllowed;
		FileVarsAllowed = true;
		fe7->Frml = compiler->RdFldNameFrmlF(FTyp, nullptr);
		FileVarsAllowed = fa;
		Z = fe7;
		compiler->processing_F = previous;
		break;
	}
	case 'i': {
		FrmlElem22* fe22 = new FrmlElem22(_indexnrecs, 4);
		fe22->WKey = LV->key;
		compiler->AcceptKeyWord("nrecs");
		FTyp = 'R';
		Z = fe22;
		break;
	}
	default: {
		compiler->OldError(177);
		break;
	}
	}

	return Z;
}

char RdOwner(Compiler* compiler, FileD* file_d, LinkD** LLD, LocVar** LLV)
{
	FileD* fd = nullptr;
	auto result = '\0';
	LocVar* lv = nullptr;
	std::string sLexWord;
	if (compiler->FindLocVar(&LVBD, &lv)) {
		if (!(lv->f_typ == 'i' || lv->f_typ == 'r' || lv->f_typ == 'f')) {
			compiler->Error(177);
		}
		LinkD* ld = nullptr;
		for (LinkD* ld1 : LinkDRoot) {
			if ((ld1->FromFile == file_d) && (ld1->IndexRoot != 0) && (ld1->ToFile == lv->FD)) {
				ld = ld1;
			}
		}
		if (ld == nullptr) {
			compiler->Error(116);
		}
		compiler->RdLex();
		if (lv->f_typ == 'f') {
#ifdef FandSQL
			if (ld->ToFile->typSQLFile) Error(155);
#endif
			compiler->Accept('[');
			*LLV = (LocVar*)compiler->RdRealFrml(nullptr);
			compiler->Accept(']');
			result = 'F';
			*LLD = ld;
			return result;
		}
		else {
			if (lv->f_typ == 'i') {
				if (ld->FromFile->IsSQLFile || ld->ToFile->IsSQLFile) {
					compiler->OldError(155);
				}
				if (!lv->key->KFlds.empty() && !KeyFldD::EquKFlds(lv->key->KFlds, ld->ToKey->KFlds)) {
					compiler->OldError(181);
				}
			}
			*LLV = lv;
			*LLD = ld;
			result = lv->f_typ;
			return result;
		}
	}
	compiler->TestIdentif();
	for (LinkD* ld : LinkDRoot) {
		sLexWord = gc->LexWord;
		if ((ld->FromFile == compiler->processing_F) && EquUpCase(ld->RoleName, sLexWord)) {
			if ((ld->IndexRoot == 0)) compiler->Error(116);
			compiler->RdLex();
			fd = ld->ToFile;
			if (gc->Lexem == '(') {
				compiler->RdLex();
				if (!compiler->FindLocVar(&LVBD, &lv) || !(lv->f_typ == 'i' || lv->f_typ == 'r')) compiler->Error(177);
				compiler->RdLex();
				compiler->Accept(')');
				if (lv->FD != fd) compiler->OldError(149);
				if (lv->f_typ == 'i') {
					if (ld->FromFile->IsSQLFile || ld->ToFile->IsSQLFile) compiler->OldError(155);
					if (!lv->key->KFlds.empty() && !KeyFldD::EquKFlds(lv->key->KFlds, ld->ToKey->KFlds)) compiler->OldError(181);
				}
				*LLV = lv;
				*LLD = ld;
				result = lv->f_typ;
				return result;
			}
			else {
#ifdef FandSQL
				if (ld->ToFile->typSQLFile) Error(155);
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

FrmlElem* RdFldNameFrmlP(Compiler* compiler, char& FTyp, MergeReportBase* caller)
{
	FileD* FD = nullptr;
	FrmlElem* Z = nullptr;
	LocVar* LV = nullptr;
	instr_type Op = _notdefined;
	LinkD* LD = nullptr;
	FieldDescr* F = nullptr;
	XKey* K = nullptr;

	FrmlElem* result = nullptr;

	if (compiler->IsForwPoint()) {
		if (compiler->FindLocVar(&LVBD, &LV) && (LV->f_typ == 'i' || LV->f_typ == 'r')) {
			compiler->RdLex();
			result = RdRecVarFldFrml(compiler, LV, FTyp);
			return result;
		}
		else {
			std::string f_name = compiler->LexWord;
			bool linked = compiler->IsRoleName(FileVarsAllowed, compiler->processing_F, &FD, &LD);

			if (FD != nullptr) {
				f_name = FD->Name;
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
				F = catalog->CatalogArchiveField();
				goto label0;
			}

			if (compiler->IsKeyWord("PATH")) {
				F = catalog->CatalogPathNameField();
				goto label0;
			}

			if (compiler->IsKeyWord("VOLUME")) {
				F = catalog->CatalogVolumeField();
			label0:
				FTyp = 'S';
			label1:
				auto S = new FrmlElemCatalogField(_catfield, 6); // Z = GetOp(_catfield, 6);
				S->CatFld = F;
				S->CatIRec = catalog->GetCatalogIRec(f_name, true);
				TestCatError(compiler, S->CatIRec, f_name, true);
				return S;
			}

			if (FD != nullptr) {
				if (compiler->IsKeyWord("GENERATION")) { Op = _generation; goto label2; }
				if (compiler->IsKeyWord("NRECSABS")) { Op = _nrecsabs; goto label2; }
				if (compiler->IsKeyWord("NRECS")) {
					Op = _nrecs;
				label2:
					auto N = new FrmlElem9(Op, 0); // Z = GetOp(oper, sizeof(FileDPtr));
					N->FD = FD;
					return N;
				}
			}

			if (linked) {
				result = compiler->RdFAccess(FD, LD, FTyp);
				return result;
			}

			if (FileVarsAllowed) {
				compiler->OldError(9);
			}
			else {
				compiler->OldError(63);
			}
		}
	}

	if (compiler->ForwChar == '[') {
		auto A = new FrmlElem14(_accrecno, 8); // Z = GetOp(_accrecno, 8);
		FD = compiler->RdFileName();
		compiler->RdLex();
		A->RecFD = FD;
#ifdef FandSQL
		if (v_files->typSQLFile) OldError(155);
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
		result = new FrmlElemFunction(Op, 0); // GetOp(oper, 0);
		FTyp = 'B';
		return result;
	}
	if (compiler->IsKeyWord("GETPATH")) {
		result = new FrmlElemFunction(_getpath, 0); // GetOp(_getpath, 0);
		FTyp = 'S';
		return result;
	}
	if (compiler->FindLocVar(&LVBD, &LV)) {
		if (LV->f_typ == 'r' || LV->f_typ == 'f' || LV->f_typ == 'i') compiler->Error(143);
		compiler->RdLex();
		result = new FrmlElemLocVar(LV->oper, LV);
		//((FrmlElemLocVar*)result)->BPOfs = LV->BPOfs;
		FTyp = LV->f_typ;
		return result;
	}
	if (FileVarsAllowed) {
		Z = compiler->TryRdFldFrml(compiler->processing_F, FTyp, nullptr);
		if (Z == nullptr) compiler->Error(8);
		result = Z;
		return result;
	}
	compiler->Error(8);
	return result;
}

FileD* RdPath(Compiler* compiler, bool NoFD, std::string& Path, WORD& CatIRec)
{
	FileD* fd = nullptr;
	CatIRec = 0;
	if (compiler->Lexem == _quotedstr) {
		Path = compiler->RdStringConst();
		fd = nullptr;
	}
	else {
		compiler->TestIdentif();
		fd = compiler->FindFileD();
		if (fd == nullptr) {
			CatIRec = catalog->GetCatalogIRec(compiler->LexWord, true);
			TestCatError(compiler, CatIRec, compiler->LexWord, false);
		}
		else if (NoFD) {
			compiler->Error(97);
		}
		compiler->RdLex();
	}
	return fd;
}

FrmlElemRecNo* RdKeyOfOrRecNo(Compiler* compiler, instr_type Op, WORD& N, FrmlElem* Arg[30], char& Typ, char FTyp)
{
	FileD* FD = compiler->RdFileName();
	XKey* K = RdViewKeyImpl(compiler, FD);
	if (Op == _recno) {
		//KeyFldD* KF = K->KFlds;
		N = 0;
		if (K->KFlds.empty()) {
			compiler->OldError(176);
		}
		//while (KF != nullptr) {
		for (KeyFldD* KF : K->KFlds) {
			compiler->Accept(',');
			if (N > 29) {
				compiler->Error(123);
			}
			Arg[N] = compiler->RdFrml(Typ, nullptr);
			N++;
			if (Typ != KF->FldD->frml_type) {
				compiler->OldError(12);
			}
			//KF = KF->pChain;
		}
	}
	else {
		compiler->Accept(',');
		N = 1;
		Arg[0] = compiler->RdRealFrml(nullptr);
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

FrmlElem* RdFunctionP(Compiler* compiler, char& FFTyp)
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
		Z = new FrmlElemEval(_eval, 5);
		((FrmlElemEval*)Z)->eval_type = FTyp;
		((FrmlElemEval*)Z)->eval_elem = compiler->RdStrFrml(nullptr);
	}
	else if (FileVarsAllowed) {
		compiler->Error(75);
	}
	else if (compiler->IsKeyWord("PROMPT")) {
		compiler->RdLex();
		Z = new FrmlElemPrompt(_prompt, 4);
		((FrmlElemPrompt*)Z)->P1 = compiler->RdStrFrml(nullptr);
		FieldDescr* F = RdFieldDescr("", true);
		((FrmlElemPrompt*)Z)->FldD = F;
		FTyp = F->frml_type;

		if (F->field_type == FieldType::TEXT) {
			compiler->OldError(65);
		}

		if (compiler->Lexem == _assign) {
			compiler->RdLex();
			((FrmlElemPrompt*)Z)->P2 = compiler->RdFrml(Typ, nullptr);

			if (Typ != FTyp) {
				compiler->OldError(12);
			}
		}
	}
	else if (compiler->IsKeyWord("KEYOF")) {
		compiler->RdLex();
		FTyp = 'S';

		if (!IsRecVar(compiler, &LV)) {
			Op = _recno;
			Z = RdKeyOfOrRecNo(compiler, Op, N, Arg, Typ, FTyp);
		}
		else {
			Z = new FrmlElem20(_keyof, 8);
			((FrmlElem20*)Z)->LV = LV;
			((FrmlElem20*)Z)->PackKey = RdViewKeyImpl(compiler, ((FrmlElem20*)Z)->LV->FD);
			FTyp = 'S';
		}
	}
	else if (compiler->IsKeyWord("RECNO")) {
		Op = _recno;
		compiler->RdLex();
		FTyp = 'R';
		Z = RdKeyOfOrRecNo(compiler, Op, N, Arg, Typ, FTyp);
	}
	else if (compiler->IsKeyWord("RECNOABS")) {
		Op = _recnoabs;
		compiler->RdLex();
		FTyp = 'R';
		Z = RdKeyOfOrRecNo(compiler, Op, N, Arg, Typ, FTyp);
	}
	else if (compiler->IsKeyWord("RECNOLOG")) {
		Op = _recnolog;
		compiler->RdLex();
		FTyp = 'R';
		Z = RdKeyOfOrRecNo(compiler, Op, N, Arg, Typ, FTyp);
	}
	else if (compiler->IsKeyWord("LINK")) {
		compiler->RdLex();
		Z = new FrmlElemLink(_link, 5); // GetOp(_link, 5);
		auto iZ = (FrmlElemLink*)Z;

		if (IsRecVar(compiler, &LV)) {
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
		if (v_files->typSQLFile) OldError(155);
#endif
		//cf = CFile;
		//CFile = FD;
		if (!compiler->IsRoleName(true, FD, &FD, &LD) || (LD == nullptr)) {
			compiler->Error(9);
		}
		//CFile = cf;
		iZ->LinkLD = LD;
		FTyp = 'R';
#ifdef FandSQL
		if (v_files->typSQLFile) Error(155);
#endif
	}
	else if (compiler->IsKeyWord("ISDELETED")) {
		compiler->RdLex();
		FTyp = 'B';

		if (IsRecVar(compiler, &LV)) {
			Z = new FrmlElem20(_lvdeleted, 4); // GetOp(_lvdeleted, 4);
			((FrmlElem20*)Z)->LV = LV;
		}
		else {
			Z = new FrmlElem14(_isdeleted, 4); // GetOp(_isdeleted, 4);
			FD = compiler->RdFileName();
			((FrmlElem14*)Z)->RecFD = FD;
			compiler->Accept(',');
			((FrmlElem14*)Z)->P1 = compiler->RdRealFrml(nullptr);
			//label2: {}
#ifdef FandSQL
			if (v_files->typSQLFile) Error(155);
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
		RdPath(compiler, true, iZ->TxtPath, iZ->TxtCatIRec);
		if ((Z->Op == _gettxt) && (compiler->Lexem == ',')) {
			compiler->RdLex();
			iZ->P1 = compiler->RdRealFrml(nullptr);
			if (compiler->Lexem == ',') {
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
		if (IsRecVar(compiler, &LV)) {
			iZ->P3 = LV->frml;
		}
		else {
			iZ->P3 = compiler->RdFrml(Typ, nullptr);
		}
		iZ->N31 = Typ;
		FTyp = 'R';
	}
#ifdef FandSQL
	else if (IsKeyWord("SQL")) {
		RdLex(); Z = GetOp(_sqlfun, 0); Z->frml_elem = RdStrFrml(); f_typ = 'rdb';
	}
#endif
	else if (compiler->IsKeyWord("SELECTSTR")) {
		compiler->RdLex();
		Z = new FrmlElemFunction(_selectstr, 13); // GetOp(_selectstr, 13);
		FTyp = 'S';
		RdSelectStr(compiler, (FrmlElemFunction*)Z);
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

XKey* RdViewKeyImpl(Compiler* compiler, FileD* FD)
{
	XKey* K = nullptr;
	if (FD != nullptr) K = FD->Keys.empty() ? nullptr : FD->Keys[0];
	if (K == nullptr) compiler->Error(24);
	if (compiler->Lexem == '/') {
		K = compiler->RdViewKey(FD);
	}
	return K;
}

void RdSelectStr(Compiler* compiler, FrmlElemFunction* Z)
{
	Z->Delim = 0x0D; // CTRL+M
	Z->P1 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
	Z->P2 = compiler->RdRealFrml(nullptr); compiler->Accept(',');
	Z->P3 = compiler->RdStrFrml(nullptr);
	while (compiler->Lexem == ',') {
		compiler->RdLex();
		if (compiler->IsOpt("HEAD")) Z->P4 = compiler->RdStrFrml(nullptr);
		else if (compiler->IsOpt("FOOT")) Z->P5 = compiler->RdStrFrml(nullptr);
		else if (compiler->IsOpt("MODE")) Z->P6 = compiler->RdStrFrml(nullptr);
		else if (compiler->IsOpt("DELIM")) Z->Delim = compiler->RdQuotedChar();
		else compiler->Error(157);
	}
}

void RdChoices(Compiler* compiler, Instr_menu* PD)
{
	ChoiceD* CD = nullptr;
	WORD N = 0, SumL = 0;
	compiler->AcceptKeyWord("OF");

	while (true) {
		if (compiler->IsKeyWord("ESCAPE")) {
			compiler->Accept(':');
			PD->WasESCBranch = true;
			AddInstr(PD->ESCInstr, RdPInstr(compiler));
		}
		else {
			CD = new ChoiceD();
			PD->Choices.push_back(CD);

			N++;
			if ((PD->Kind == PInstrCode::_menubar) && (N > 30)) compiler->Error(102);
			CD->TxtFrml = compiler->RdStrFrml(nullptr);
			if (compiler->Lexem == ',') {
				compiler->RdLex();
				if (compiler->Lexem != ',') {
					CD->HelpName = compiler->RdHelpName();
					PD->HelpRdb = CRdb;
				}
				if (compiler->Lexem == ',') {
					compiler->RdLex();
					if (compiler->Lexem != ',') {
						CD->Condition = compiler->RdBool(nullptr);
						if (compiler->Lexem == '!') {
							CD->DisplEver = true;
							compiler->RdLex();
						}
					}
				}
			}
			compiler->Accept(':');
			AddInstr(CD->v_instr, RdPInstr(compiler));
		}
		if (compiler->Lexem == ';') {
			compiler->RdLex();
			if (compiler->IsKeyWord("END")) return;
			continue;
		}
		break;
	}

	compiler->AcceptKeyWord("END");
}

void RdMenuAttr(Compiler* compiler, Instr_menu* PD)
{
	if (compiler->Lexem != ';') return;
	compiler->RdLex();
	PD->mAttr[0] = compiler->RdAttr(); compiler->Accept(',');
	PD->mAttr[1] = compiler->RdAttr(); compiler->Accept(',');
	PD->mAttr[2] = compiler->RdAttr();
	if (compiler->Lexem == ',') {
		compiler->RdLex();
		PD->mAttr[3] = compiler->RdAttr();
	}
}

Instr* RdMenuBox(Compiler* compiler, bool Loop)
{
	Instr_menu* result = new Instr_menu(PInstrCode::_menubox);
	
	result->Loop = Loop;
	if (compiler->Lexem == '(') {
		compiler->RdLex();
		if (compiler->Lexem != ';') {
			result->X = compiler->RdRealFrml(nullptr);
			compiler->Accept(',');
			result->Y = compiler->RdRealFrml(nullptr);
		}
		RdMenuAttr(compiler, result);
		compiler->Accept(')');
	}
	if (compiler->Lexem == '!') { compiler->RdLex(); result->Shdw = true; }
	if (compiler->IsKeyWord("PULLDOWN")) result->PullDown = true;
	if (!compiler->TestKeyWord("OF")) result->HdLine = compiler->RdStrFrml(nullptr);
	RdChoices(compiler, result);

	return result;
}

Instr* RdMenuBar(Compiler* compiler)
{
	Instr_menu* PD = new Instr_menu(PInstrCode::_menubar); // GetPInstr(_menubar, 48);
	auto result = PD;
	if (compiler->Lexem == '(') {
		compiler->RdLex();
		if (compiler->Lexem != ';') {
			PD->Y = compiler->RdRealFrml(nullptr);
			if (compiler->Lexem == ',') {
				compiler->RdLex();
				PD->X = compiler->RdRealFrml(nullptr);
				compiler->Accept(',');
				PD->XSz = compiler->RdRealFrml(nullptr);
			}
		}
		RdMenuAttr(compiler, PD);
		compiler->Accept(')');
	}
	RdChoices(compiler, PD);
	return result;
}

Instr_loops* RdIfThenElse(Compiler* compiler)
{
	Instr_loops* result = new Instr_loops(PInstrCode::_ifthenelseP);
	result->Bool = compiler->RdBool(nullptr);
	compiler->AcceptKeyWord("THEN");
	result->v_instr = RdPInstr(compiler);

	if (compiler->IsKeyWord("ELSE")) {
		result->v_else_instr = RdPInstr(compiler);
	}

	return result;
}

Instr_loops* RdWhileDo(Compiler* compiler)
{
	Instr_loops* result = new Instr_loops(PInstrCode::_whiledo);
	result->Bool = compiler->RdBool(nullptr);
	compiler->AcceptKeyWord("DO");
	result->v_instr = RdPInstr(compiler);
	return result;
}

std::vector<Instr*> RdFor(Compiler* compiler)
{
	LocVar* LV = nullptr;
	if (!compiler->FindLocVar(&LVBD, &LV) || (LV->f_typ != 'R')) {
		compiler->Error(146);
	}
	compiler->RdLex();

	std::vector<Instr*> result;

	// read loop condition and add it as first instruction
	Instr_assign* PD = new Instr_assign(PInstrCode::_asgnloc);
	PD->AssLV = LV;
	compiler->Accept(_assign);
	PD->Frml = compiler->RdRealFrml(nullptr);
	result.push_back(PD);

	compiler->AcceptKeyWord("TO");
	Instr_loops* iLoop = new Instr_loops(PInstrCode::_whiledo);

	FrmlElemFunction* Z1 = new FrmlElemFunction(_compreal, 2);
	Z1->P1 = nullptr;
	Z1->LV1 = LV;
	Z1->N21 = _le;
	Z1->N22 = 5;
	Z1->P2 = compiler->RdRealFrml(nullptr);
	iLoop->Bool = Z1;

	compiler->AcceptKeyWord("DO");
	iLoop->v_instr = RdPInstr(compiler);
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

Instr* RdCase(Compiler* compiler)
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
		PD->Bool = compiler->RdBool(nullptr);
		compiler->Accept(':');
		AddInstr(PD->v_instr, RdPInstr(compiler));
		bool b = compiler->Lexem == ';';
		if (b) compiler->RdLex();
		if (!compiler->IsKeyWord("END")) {
			if (compiler->IsKeyWord("ELSE")) {
				while (!compiler->IsKeyWord("END")) {
					AddInstr(PD->v_else_instr, RdPInstr(compiler));
					if (compiler->Lexem == ';') {
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

Instr_loops* RdRepeatUntil(Compiler* compiler)
{
	auto PD = new Instr_loops(PInstrCode::_repeatuntil); // GetPInstr(_repeatuntil, 8);
	Instr_loops* result = PD;
	while (!compiler->IsKeyWord("UNTIL")) {
		AddInstr(PD->v_instr, RdPInstr(compiler));
		if (compiler->Lexem == ';') {
			compiler->RdLex();
		}
		else {
			compiler->AcceptKeyWord("UNTIL");
			break;
		}
	}
	PD->Bool = compiler->RdBool(nullptr);
	return result;
}

Instr_forall* RdForAll(Compiler* compiler)
{
	LocVar* LVi = nullptr;
	LocVar* LVr = nullptr;
	LinkD* LD = nullptr;
	FrmlElem* Z = nullptr;
	FileD* processed_file = nullptr;

	if (!compiler->FindLocVar(&LVBD, &LVi)) compiler->Error(122);
	compiler->RdLex();
	if (LVi->f_typ == 'r') {
		LVr = LVi;
		LVi = nullptr;
		processed_file = LVr->FD;
	}
	else {
		compiler->TestReal(LVi->f_typ);
		compiler->AcceptKeyWord("IN");
		if (compiler->FindLocVar(&LVBD, &LVr)) {
			if (LVr->f_typ == 'f') {
				processed_file = LVr->FD;
				compiler->RdLex();
				goto label1;
			}
			if (LVr->f_typ != 'r') compiler->Error(141);
			processed_file = LVr->FD;
			compiler->RdLex();
		}
		else {
			processed_file = compiler->RdFileName();
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
		if (compiler->IsKeyWord("OWNER")) {
			PD->COwnerTyp = RdOwner(compiler, PD->CFD, &PD->CLD, &PD->CLV);
			CViewKey = GetFromKey(PD->CLD);
		}
		else {
			CViewKey = compiler->RdViewKey(processed_file);
		}
		compiler->processing_F = processed_file;
		if (compiler->Lexem == '(') {
			compiler->RdLex();
			PD->CBool = compiler->RdKeyInBool(PD->CKIRoot, false, true, PD->CSQLFilter, nullptr);
			if ((!PD->CKIRoot.empty()) && (PD->CLV != nullptr)) {
				compiler->OldError(118);
			}
			compiler->Accept(')');
		}
		if (compiler->Lexem == '!') {
			compiler->RdLex();
			PD->CWIdx = true;
		}
		if (compiler->Lexem == '%') {
			compiler->RdLex();
			PD->CProcent = true;
		}
		PD->CKey = CViewKey;

#ifdef FandSQL
	}
#endif

	compiler->AcceptKeyWord("DO");
	PD->CInstr = RdPInstr(compiler);
	return PD;
}

std::vector<Instr*> RdBeginEnd(Compiler* compiler)
{
	std::vector<Instr*> instructions;
	if (!compiler->IsKeyWord("END")) {
		while (true) {
			// read instructions and add them to the list
			AddInstr(instructions, RdPInstr(compiler));

			if (compiler->Lexem == ';') {
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
	return instructions;
}

Instr_proc* RdProcArg(Compiler* compiler, char Caller)
{
	std::string ProcName = compiler->LexWord;
	RdbPos Pos;
	TypAndFrml TArg[31];
	LocVar* LV = nullptr;
	if (Caller != 'C') {
		compiler->RdChptName('P', &Pos, Caller == 'P' || Caller == 'E' || Caller == 'T');
	}
	WORD N = 0;
	if (Caller != 'P') {
		if (compiler->Lexem == '(') {
			compiler->RdLex();
			goto label1;
		}
	}
	else if (compiler->Lexem == ',') {
		compiler->RdLex();
		compiler->Accept('(');
	label1:
		N++;
		if (N > 30) compiler->Error(123);
		TArg[N].Name = compiler->LexWord;
		if ((compiler->ForwChar != '.') && compiler->FindLocVar(&LVBD, &LV) && (LV->f_typ == 'i' || LV->f_typ == 'r')) {
			compiler->RdLex();
			TArg[N].FTyp = LV->f_typ;
			TArg[N].FD = LV->FD;
			TArg[N].record = LV->record;
		}
		else if (compiler->Lexem == '@') {
			compiler->RdLex();
			if (compiler->Lexem == '[') {
				compiler->RdLex();
				TArg[N].Name = compiler->LexWord;
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
		if (compiler->Lexem == ',') {
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

void SetCode(std::string keyName, uint8_t fnNr, EdExKeyD* E)
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

void RdKeyCode(Compiler* compiler, EdExitD* X)
{
	EdExKeyD lastKey;
	std::string key; // tady bude "shift" | "ctrl" | "alt"
	uint8_t fnNr; // tady bude cislo funkci klavesy

	lastKey.KeyName = compiler->LexWord;

	if (FindShiftCtrlAltFxx(compiler->LexWord, key, fnNr)) {
		SetCode(key, fnNr, &lastKey);
		compiler->RdLex();
	}
	else {
		bool found = false;
		for (size_t i = 0; i < KeyNames.size(); i++) {
			if (EquUpCase(KeyNames[i].name, compiler->LexWord)) {
				lastKey.KeyCode = KeyNames[i].code;
				lastKey.Break = KeyNames[i].brk;
				compiler->RdLex();
				found = true;
				break;
			}
		}

		if (!found) {
			compiler->Error(129);
		}
	}

	X->Keys.push_back(std::move(lastKey));
}

void ReplaceEmptyStringWithSingleSpace(FrmlElemString* el)
{
	if (el->Op == _const && el->S.empty()) {
		el->S = " ";
	}
}

bool RdHeadLast(Compiler* compiler, EditOpt* EO)
{
	bool result = true;

	if (compiler->IsOpt("HEAD")) {
		EO->Head = compiler->RdStrFrml(nullptr);
		ReplaceEmptyStringWithSingleSpace((FrmlElemString*)EO->Head);
	}
	else if (compiler->IsOpt("LAST")) {
		EO->Last = compiler->RdStrFrml(nullptr);
		ReplaceEmptyStringWithSingleSpace((FrmlElemString*)EO->Last);
	}
	else if (compiler->IsOpt("CTRL")) {
		EO->CtrlLast = compiler->RdStrFrml(nullptr);
		ReplaceEmptyStringWithSingleSpace((FrmlElemString*)EO->CtrlLast);
	}
	else if (compiler->IsOpt("ALT")) {
		EO->AltLast = compiler->RdStrFrml(nullptr);
		ReplaceEmptyStringWithSingleSpace((FrmlElemString*)EO->AltLast);
	}
	else if (compiler->IsOpt("SHIFT")) {
		EO->ShiftLast = compiler->RdStrFrml(nullptr);
		ReplaceEmptyStringWithSingleSpace((FrmlElemString*)EO->ShiftLast);
	}
	else {
		result = false;
	}

	return result;
}

bool RdHeadLast(Compiler* compiler, Instr_edittxt* IE)
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

bool RdViewOpt(Compiler* compiler, EditOpt* EO, FileD* file_d)
{
	FileD* prev_file = compiler->processing_F;
	FieldNameType prev_field = compiler->rdFldNameType;

	compiler->rdFldNameType = FieldNameType::P;

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
		if ((compiler->Lexem == _quotedstr) && (compiler->ForwChar == ',' || compiler->ForwChar == ')')) {
			DataEditorParams params;
			int validate = params.SetFromString(compiler->LexWord, true);
			if (validate != 0) {
				compiler->Error(validate);
			}
			EO->Mode = new FrmlElemString(_const, 0); // GetOp(_const, LexWord.length() + 1);
			((FrmlElemString*)EO->Mode)->S = compiler->LexWord;
			compiler->RdLex();
		}
		else {
			EO->Mode = compiler->RdStrFrml(nullptr);
		}
	}
	else if (RdHeadLast(compiler, EO)) {
		return result;
	}
	else if (compiler->IsOpt("WATCH")) {
		EO->WatchDelayZ = compiler->RdRealFrml(nullptr);
	}
	else if (compiler->IsOpt("WW")) {
		compiler->Accept('(');
		EO->WFlags = 0;
		if (compiler->Lexem == '(') { compiler->RdLex(); EO->WFlags = WNoPop; }
		compiler->RdW(EO->W);
		compiler->RdFrame(&EO->Top, EO->WFlags);
		if (compiler->Lexem == ',') {
			compiler->RdLex();
			EO->ZAttr = compiler->RdAttr(); compiler->Accept(',');
			EO->ZdNorm = compiler->RdAttr(); compiler->Accept(',');
			EO->ZdHiLi = compiler->RdAttr();
			if (compiler->Lexem == ',') {
				compiler->RdLex();
				EO->ZdSubset = compiler->RdAttr();
				if (compiler->Lexem == ',') {
					compiler->RdLex();
					EO->ZdDel = compiler->RdAttr();
					if (compiler->Lexem == ',') {
						compiler->RdLex();
						EO->ZdTab = compiler->RdAttr();
						if (compiler->Lexem == ',') {
							compiler->RdLex();
							EO->ZdSelect = compiler->RdAttr();
						}
					}
				}
			}
		}
		compiler->Accept(')');
		if ((EO->WFlags & WNoPop) != 0) {
			compiler->Accept(')');
		}
	}
	else if (compiler->IsOpt("EXIT")) {
		compiler->Accept('(');
		while (true) {
			EdExitD* X = new EdExitD();
			EO->ExD.push_back(X);

			RdKeyList(compiler, X);
			if (compiler->IsKeyWord("QUIT")) X->Typ = 'Q';
			else if (compiler->IsKeyWord("REPORT")) {
				if (X->AtWrRec || (EO->LvRec != nullptr)) {
					compiler->OldError(144);
				}
				compiler->Accept('(');
				X->Typ = 'R';
				RO = compiler->GetRprtOpt();
				compiler->RdChptName('R', &RO->RprtPos, true);
				while (compiler->Lexem == ',') {
					compiler->RdLex();
					if (compiler->IsOpt("ASSIGN")) {
						RdPath(compiler, true, RO->Path, RO->CatIRec);
					}
					else if (compiler->IsKeyWord("EDIT")) {
						RO->Edit = true;
					}
					else {
						compiler->Error(130);
					}
				}
				X->RO = RO;
				compiler->Accept(')');
			}
			else if (!(compiler->Lexem == ',' || compiler->Lexem == ')')) {
				X->Typ = 'P';
				X->Proc = RdProcArg(compiler, 'E');
			}
			if (compiler->Lexem == ',') {
				compiler->RdLex();
				continue;
			}
			break;
		}
		compiler->Accept(')');
	}
	else if (EO->LvRec != nullptr) {
		result = false;
	}
	else if (compiler->IsOpt("COND")) {
		if (compiler->Lexem == '(') {
			compiler->RdLex();
			EO->Cond = compiler->RdKeyInBool(EO->KIRoot, false, true, EO->SQLFilter, nullptr);
			compiler->Accept(')');
		}
		else {
			EO->Cond = compiler->RdKeyInBool(EO->KIRoot, false, true, EO->SQLFilter, nullptr);
		}
	}
	else if (compiler->IsOpt("JOURNAL")) {
		EO->Journal = compiler->RdFileName();
		WORD l = EO->Journal->FF->RecLen - 13;
		if (file_d->FF->file_type == FandFileType::INDEX) {
			l++;
		}
		if (file_d->FF->RecLen != l) {
			compiler->OldError(111);
		}
	}
	else if (compiler->IsOpt("SAVEAFTER")) {
		EO->SaveAfterZ = compiler->RdRealFrml(nullptr);
	}
	else if (compiler->IsOpt("REFRESH")) {
		EO->RefreshDelayZ = compiler->RdRealFrml(nullptr);
	}
	else {
		result = false;
	}

	// revert compiler changes:
	compiler->processing_F = prev_file;
	compiler->rdFldNameType = prev_field;
	
	return result;
}

void RdKeyList(Compiler* compiler, EdExitD* X)
{
	while (true) {
		if ((compiler->Lexem == '(') || (compiler->Lexem == '^')) {
			compiler->RdNegFldList(X->NegFlds, X->Flds);
		}
		else if (compiler->IsKeyWord("RECORD")) {
			X->AtWrRec = true;
		}
		else if (compiler->IsKeyWord("NEWREC")) {
			X->AtNewRec = true;
		}
		else {
			RdKeyCode(compiler, X);
		}
		if (compiler->Lexem == ',') {
			compiler->RdLex();
			continue;
		}
		break;
	}
	compiler->Accept(':');
}

void RdProcCall(Compiler* compiler, Instr** pinstr)
{
	//Instr* PD = nullptr;
	if (compiler->IsKeyWord("EXEC")) *pinstr = RdExec(compiler);
	else if (compiler->IsKeyWord("COPYFILE")) *pinstr = RdCopyFile(compiler);
	else if (compiler->IsKeyWord("PROC")) {
		compiler->RdLex();
		*pinstr = RdProcArg(compiler, 'P');
	}
	else if (compiler->IsKeyWord("DISPLAY")) *pinstr = RdDisplay(compiler);
	else if (compiler->IsKeyWord("CALL")) *pinstr = RdRDBCall(compiler);
	else if (compiler->IsKeyWord("WRITELN")) RdWriteln(compiler, WriteType::writeln, (Instr_writeln**)pinstr);
	else if (compiler->IsKeyWord("WRITE")) RdWriteln(compiler, WriteType::write, (Instr_writeln**)pinstr);
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
	else if (compiler->IsKeyWord("MESSAGE")) RdWriteln(compiler, WriteType::message, (Instr_writeln**)pinstr);
	else if (compiler->IsKeyWord("GOTOXY")) *pinstr = RdGotoXY(compiler);
	else if (compiler->IsKeyWord("MERGE")) {
		// PD = (Instr_merge_display*)GetPD(_merge, sizeof(RdbPos));
		*pinstr = new Instr_merge_display(PInstrCode::_merge);
		compiler->RdLex();
		RdbPos rp;
		compiler->RdChptName('M', &rp, true);
		((Instr_merge_display*)*pinstr)->Pos = rp;
	}
	else if (compiler->IsKeyWord("SORT")) *pinstr = RdSortCall(compiler);
	else if (compiler->IsKeyWord("EDIT")) *pinstr = RdEditCall(compiler);
	else if (compiler->IsKeyWord("REPORT")) *pinstr = RdReportCall(compiler);
	else if (compiler->IsKeyWord("EDITTXT")) *pinstr = RdEditTxt(compiler);
	else if (compiler->IsKeyWord("PRINTTXT")) *pinstr = RdPrintTxt(compiler);
	else if (compiler->IsKeyWord("PUTTXT")) *pinstr = RdPutTxt(compiler);
	else if (compiler->IsKeyWord("TURNCAT")) *pinstr = RdTurnCat(compiler);
	else if (compiler->IsKeyWord("RELEASEDRIVE")) *pinstr = RdReleaseDrive(compiler);
	else if (compiler->IsKeyWord("SETPRINTER")) {
		*pinstr = new Instr_assign(PInstrCode::_setprinter); // GetPD(_setprinter, 4);
		compiler->RdLex();
		goto label2;
	}
	else if (compiler->IsKeyWord("INDEXFILE")) *pinstr = RdIndexfile(compiler);
	else if (compiler->IsKeyWord("GETINDEX"))*pinstr = RdGetIndex(compiler);
	else if (compiler->IsKeyWord("MOUNT")) *pinstr = RdMount(compiler);
	else if (compiler->IsKeyWord("CLRSCR")) *pinstr = RdClrWw(compiler);
	else if (compiler->IsKeyWord("APPENDREC")) *pinstr = RdMixRecAcc(compiler, PInstrCode::_appendRec);
	else if (compiler->IsKeyWord("DELETEREC")) *pinstr = RdMixRecAcc(compiler, PInstrCode::_deleterec);
	else if (compiler->IsKeyWord("RECALLREC")) *pinstr = RdMixRecAcc(compiler, PInstrCode::_recallrec);
	else if (compiler->IsKeyWord("READREC")) *pinstr = RdMixRecAcc(compiler, PInstrCode::_readrec);
	else if (compiler->IsKeyWord("WRITEREC")) *pinstr = RdMixRecAcc(compiler, PInstrCode::_writerec);
	else if (compiler->IsKeyWord("LINKREC")) *pinstr = RdLinkRec(compiler);
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
	else if (compiler->IsKeyWord("LPROC")) *pinstr = RdCallLProc(compiler);

#ifdef FandGraph
	else if (compiler->IsKeyWord("GRAPH")) *pinstr = RdGraphP(compiler);
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
			if (compiler->Lexem == ',') {
				compiler->RdLex();
				iPutPixel->Par6 = compiler->RdRealFrml(nullptr);
				if (compiler->Lexem == ',') {
					compiler->RdLex();
					iPutPixel->Par7 = compiler->RdRealFrml(nullptr);
					if (compiler->Lexem == ',') {
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
			if ((iPutPixel->Kind == PInstrCode::_ellipse) && (compiler->Lexem == ',')) {
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
	else if (compiler->IsKeyWord("BACKUP")) *pinstr = RdBackup(compiler, ' ', true);
	else if (compiler->IsKeyWord("BACKUPM")) *pinstr = RdBackup(compiler, 'M', true);
	else if (compiler->IsKeyWord("RESTORE")) *pinstr = RdBackup(compiler, ' ', false);
	else if (compiler->IsKeyWord("RESTOREM")) *pinstr = RdBackup(compiler, 'M', false);
	else if (compiler->IsKeyWord("SETEDITTXT")) *pinstr = RdSetEditTxt(compiler);
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
		if (iPD->cfFD != nullptr && (iPD->cfFD->FileType == DataFileType::DBF || iPD->cfFD->FF->file_type == FandFileType::FAND8)
#ifdef FandSQL
			|| PD->cfFD->typSQLFile
#endif
			) compiler->OldError(169);
		compiler->Accept(',');
		RdPath(compiler, true, iPD->cfPath, iPD->cfCatIRec);
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

std::vector<FieldDescr*> RdFlds(Compiler* compiler)
{
	std::vector<FieldDescr*> FLRoot;
	//FieldListEl* FL = nullptr;

	while (true) {
		auto fd = compiler->RdFldName(compiler->processing_F);
		FLRoot.push_back(fd);
		if (compiler->Lexem == ',') {
			compiler->RdLex();
			continue;
		}
		break;
	}

	return FLRoot;
}

std::vector<FieldDescr*> RdSubFldList(Compiler* compiler, const std::vector<FieldDescr*>& v_fields, char Opt)
{
	std::vector<FieldDescr*> result;
	compiler->Accept('(');

	while (true) {
		FieldDescr* field = nullptr;
		if (v_fields.empty()) {
			field = compiler->RdFldName(compiler->processing_F);
			if (field == nullptr) {
				compiler->Error(43);
			}
		}
		else {
			bool found = false;
			compiler->TestIdentif();
			for (FieldDescr* f : v_fields) {
				if (EquUpCase(f->Name, compiler->LexWord)) {
					found = true;
					field = f;
					result.push_back(f);
					break;
				}
			}
			if (!found) {
				compiler->Error(43);
			}
			compiler->RdLex();
		}

		if ((Opt == 'S') && (field->frml_type != 'R')) {
			compiler->OldError(20);
		}

		if (compiler->Lexem == ',') {
			compiler->RdLex();
			continue;
		}

		break;
	}

	compiler->Accept(')');

	return result;
}

Instr_sort* RdSortCall(Compiler* compiler)
{
	auto PD = new Instr_sort(); // GetPD(_sort, 8);
	compiler->RdLex();
	FileD* FD = compiler->RdFileName();
	PD->SortFD = FD;
#ifdef FandSQL
	if (v_files->typSQLFile) OldError(155);
#endif
	compiler->Accept(',');
	compiler->Accept('(');
	compiler->RdKFList(PD->SK, PD->SortFD);
	compiler->Accept(')');
	return PD;
}

Instr_edit* RdEditCall(Compiler* compiler)
{
	LocVar* lv = nullptr;
	Instr_edit* instr_edit = new Instr_edit();
	compiler->RdLex();
	instr_edit->options.UserSelFlds = true;

	if (IsRecVar(compiler, &lv)) {
		instr_edit->options.LvRec = lv->record;
		instr_edit->EditFD = lv->FD;
	}
	else {
		instr_edit->EditFD = compiler->RdFileName();
		XKey* K = compiler->RdViewKey(instr_edit->EditFD);
		if (K == nullptr) K = instr_edit->EditFD->Keys.empty() ? nullptr : instr_edit->EditFD->Keys[0];
		instr_edit->options.ViewKey = K;
	}
	//PD->EditFD = CFile;
	compiler->Accept(',');
	if (compiler->IsOpt("U")) {
		compiler->TestIdentif();
		if (instr_edit->EditFD->ViewNames.empty()) {
			compiler->Error(114);
		}
		stSaveState* p = compiler->SaveCompState();
		bool b = RdUserView(instr_edit->EditFD, compiler->LexWord, &instr_edit->options);
		compiler->RestoreCompState(p);
		if (!b) compiler->Error(114);
		compiler->RdLex();
	}
	else {
		compiler->processing_F = instr_edit->EditFD;
		RdBegViewDcl(&instr_edit->options);
	}
	while (compiler->Lexem == ',') {
		bool b = RdViewOpt(compiler, &instr_edit->options, instr_edit->EditFD);
		if (!b) RdEditOpt(compiler, &instr_edit->options, instr_edit->EditFD);
	}
	return instr_edit;
}

void RdEditOpt(Compiler* compiler, EditOpt* EO, FileD* file_d)
{
	if (compiler->IsOpt("FIELD")) {
		EO->StartFieldZ = compiler->RdStrFrml(nullptr);
	}
	else if (EO->LvRec != nullptr) {
		compiler->Error(125);
	}
	else if (compiler->IsOpt("OWNER")) {
		if (EO->SQLFilter || (!EO->KIRoot.empty())) {
			compiler->OldError(179);
		}
		EO->OwnerTyp = RdOwner(compiler, file_d, &EO->DownLD, &EO->DownLV);
	}
	else if (compiler->IsOpt("RECKEY")) {
		EO->StartRecKeyZ = compiler->RdStrFrml(nullptr);
	}
	else if (
#ifdef FandSQL
		!file_d->typSQLFile &&
#endif
		compiler->IsOpt("RECNO")) {
		EO->StartRecNoZ = compiler->RdRealFrml(nullptr);
	}
	else if (compiler->IsOpt("IREC")) {
		EO->StartIRecZ = compiler->RdRealFrml(nullptr);
	}
	else if (compiler->IsKeyWord("CHECK")) {
		EO->SyntxChk = true;
	}
	else if (compiler->IsOpt("SEL")) {
		LocVar* lv = RdIdxVar(compiler);
		EO->SelKey = lv->key;
		if ((EO->ViewKey == nullptr)) {
			compiler->OldError(108);
		}
		if (EO->ViewKey == EO->SelKey) {
			compiler->OldError(184);
		}
		if ((!EO->ViewKey->KFlds.empty())
			&& (!EO->SelKey->KFlds.empty())
			&& !KeyFldD::EquKFlds(EO->SelKey->KFlds, EO->ViewKey->KFlds)) {
			compiler->OldError(178);
		}
	}
	else {
		compiler->Error(125);
	}
}

Instr* RdReportCall(Compiler* compiler)
{
	LocVar* lv = nullptr;
	Instr_report* PD = new Instr_report();
	compiler->RdLex();
	RprtOpt* RO = compiler->GetRprtOpt();
	PD->RO = RO;
	bool has_first = false;
	FileD* processing_file = nullptr;

	if (compiler->Lexem != ',') {
		has_first = true;
		bool b = false;
		if (compiler->Lexem == '(') {
			compiler->RdLex();
			b = true;
		}

		while (true) {
			RprtFDListEl* FDL = new RprtFDListEl();
			RO->FDL.push_back(FDL);

			if (IsRecVar(compiler, &lv)) {
				FDL->LVRecPtr = lv->record;
				FDL->FD = lv->FD;
			}
			else {
				processing_file = compiler->RdFileName();
				FDL->FD = processing_file;
				compiler->processing_F = processing_file;
				CViewKey = compiler->RdViewKey(FDL->FD);
				FDL->ViewKey = CViewKey;
				if (compiler->Lexem == '(') {
					compiler->RdLex();
					FDL->Cond = compiler->RdKeyInBool(FDL->KeyIn, true, true, FDL->SQLFilter, nullptr);
					compiler->Accept(')');
				}
			}

			if (b && (compiler->Lexem == ',')) {
				compiler->RdLex();
				continue;
			}

			break;
		}

		if (b) {
			compiler->Accept(')');
		}
		processing_file = RO->FDL[0]->FD;
		compiler->processing_F = processing_file;
		CViewKey = RO->FDL[0]->ViewKey;
	}

	compiler->Accept(',');
	if (compiler->Lexem == '[') {
		compiler->RdLex();
		RO->RprtPos.rdb = (Project*)compiler->RdStrFrml(nullptr);
		RO->RprtPos.i_rec = 0;
		RO->FromStr = true;
		compiler->Accept(']');
	}
	else if (!has_first || (compiler->Lexem == _identifier)) {
		compiler->TestIdentif();
		if (!compiler->FindChpt('R', compiler->LexWord, false, &RO->RprtPos)) {
			compiler->Error(37);
		}
		compiler->RdLex();
	}
	else {
		compiler->Accept('(');
		switch (compiler->Lexem) {
		case '?': {
			RO->Flds = compiler->AllFldsList(processing_file, false);
			compiler->RdLex();
			RO->UserSelFlds = true;
			break;
		}
		case ')': {
			RO->Flds = compiler->AllFldsList(processing_file, true);
			break;
		}
		default: {
			RO->Flds = RdFlds(compiler);
			if (compiler->Lexem == '?') {
				compiler->RdLex();
				RO->UserSelFlds = true;
			}
			break;
		}
		}
		compiler->Accept(')');
	}
	while (compiler->Lexem == ',') {
		compiler->RdLex();
		if (has_first) {
			RprtFDListEl* FDL = RO->FDL[RO->FDL.size() - 1]; // last element
			RdRprtOpt(compiler, RO, FDL->LVRecPtr == nullptr);
		}
		else {
			RdRprtOpt(compiler, RO, false);
		}
	}
	if ((RO->Mode == _ALstg) && ((!RO->Ctrl.empty()) || (!RO->Sum.empty()))) {
		RO->Mode = _ARprt;
	}
	return PD;
}

void RdRprtOpt(Compiler* compiler, RprtOpt* RO, bool has_first)
{
	FileD* FD = nullptr;
	size_t N = 0;

	if (compiler->IsOpt("ASSIGN")) {
		RdPath(compiler, true, RO->Path, RO->CatIRec);
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
			compiler->RdKFList(RO->SK, compiler->processing_F);
			compiler->Accept(')');
		}
		WORD Low = compiler->input_pos;
		compiler->Accept(_equ);
		bool br = false;
		if (compiler->Lexem == '(') {
			Low = compiler->input_pos;
			compiler->RdLex();
			br = true;
			if (compiler->Lexem == '?') {
				compiler->RdLex();
				RO->UserCondQuest = true;
				if (br) {
					compiler->Accept(')');
				}
				return;
			}
		}
		RO->FDL[0]->Cond = compiler->RdKeyInBool(RO->FDL[0]->KeyIn, true, true, RO->FDL[0]->SQLFilter, nullptr);
		N = compiler->input_old_err_pos - Low;
		RO->CondTxt = compiler->input_string.substr(Low, N); //std::string((const char*)&InpArrPtr[Low], N);

		if (br) {
			compiler->Accept(')');
		}
	}
	else if (compiler->IsOpt("CTRL")) {
		if (!has_first) {
			compiler->OldError(51);
			compiler->Accept('(');
			compiler->RdKFList(RO->SK, compiler->processing_F);
			compiler->Accept(')');
		}
		RO->Ctrl = RdSubFldList(compiler, RO->Flds, 'C');
	}
	else if (compiler->IsOpt("SUM")) {
		if (!has_first) {
			compiler->OldError(51);
			compiler->Accept('(');
			compiler->RdKFList(RO->SK, compiler->processing_F);
			compiler->Accept(')');
		}
		RO->Sum = RdSubFldList(compiler, RO->Flds, 'S');
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
		compiler->RdKFList(RO->SK, compiler->processing_F);
		compiler->Accept(')');
	}
	else if (compiler->IsOpt("HEAD")) {
		RO->Head = compiler->RdStrFrml(nullptr);
	}
	else {
		compiler->Error(45);
	}
}

Instr* RdRDBCall(Compiler* compiler)
{
	std::string s;
	auto PD = new Instr_call(); // GetPD(_call, 12);
	compiler->RdLex();
	//s[0] = 0;
	if (compiler->Lexem == '\\') {
		s = "\\";
		compiler->RdLex();
	}
	compiler->TestIdentif();
	if (compiler->LexWord.length() > 8) compiler->Error(2);
	PD->RdbNm = s + std::string(compiler->LexWord);
	compiler->RdLex();
	if (compiler->Lexem == ',') {
		compiler->RdLex();
		compiler->TestIdentif();
		if (compiler->LexWord.length() > 12) compiler->Error(2);
		PD->ProcNm = compiler->LexWord;
		compiler->RdLex();
		PD->ProcCall = RdProcArg(compiler, 'C');
	}
	else {
		PD->ProcNm = "main";
	}
	return PD;
}

Instr* RdExec(Compiler* compiler)
{
	auto PD = new Instr_exec(); // GetPD(_exec, 14);
	compiler->RdLex();
	RdPath(compiler, true, PD->ProgPath, PD->ProgCatIRec);
	compiler->Accept(',');
	PD->Param = compiler->RdStrFrml(nullptr);
	while (compiler->Lexem == ',') {
		compiler->RdLex();
		if (compiler->IsKeyWord("NOCANCEL")) PD->NoCancel = true;
		else if (compiler->IsKeyWord("FREEMEM")) PD->FreeMm = true;
		else if (compiler->IsKeyWord("LOADFONT")) PD->LdFont = true;
		else if (compiler->IsKeyWord("TEXTMODE")) PD->TextMd = true;
		else compiler->Error(101);
	}
	return PD;
}

Instr* RdCopyFile(Compiler* compiler)
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
	CD->FD1 = RdPath(compiler, false, CD->Path1, CD->CatIRec1);
	CD->WithX1 = RdX(compiler, CD->FD1);
	if (compiler->Lexem == '/') {
		if (CD->FD1 != nullptr) {
			//CFile = CD->FD1; 
			CD->ViewKey = compiler->RdViewKey(CD->FD1);
		}
		else {
			CD->Opt1 = RdCOpt(compiler);
		}
	}
	compiler->Accept(',');
	CD->FD2 = RdPath(compiler, false, CD->Path2, CD->CatIRec2);
	CD->WithX2 = RdX(compiler, CD->FD2);
	if (compiler->Lexem == '/') {
		if (CD->FD2 != nullptr) compiler->Error(139);
		else CD->Opt2 = RdCOpt(compiler);
	}
	if (!TestFixVar(compiler, CD->Opt1, CD->FD1, CD->FD2) && !TestFixVar(compiler, CD->Opt2, CD->FD2, CD->FD1))
	{
		if ((CD->Opt1 == CpOption::cpTxt) && (CD->FD2 != nullptr)) compiler->OldError(139);
		noapp = (CD->FD1 == nullptr) ^ (CD->FD2 == nullptr); // XOR
#ifdef FandSQL
		if (noapp)
			if ((FD1 != nullptr) && (FD1->typSQLFile) || (FD2 != nullptr)
				&& (FD2->typSQLFile)) OldError(155);
#endif
	}
	while (compiler->Lexem == ',') {
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
				if (EquUpCase(compiler->LexWord, ModeTxt[i])) {
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

CpOption RdCOpt(Compiler* compiler)
{
	uint8_t i = 0;
	pstring OptArr[3] = { "FIX", "VAR", "TXT" };
	compiler->RdLex();
	compiler->TestIdentif();
	for (i = 0; i < 3; i++)
		if (EquUpCase(OptArr[i], compiler->LexWord)) {
			compiler->RdLex();
			return CpOption(i + 1); // vracime i + 1 (CpOption ma 4 moznosti, je to posunute ...)
		}
	compiler->Error(53);
	throw std::exception("Bad value in RdCOpt() in rdproc.cpp");
}

bool RdX(Compiler* compiler, FileD* FD)
{
	bool result = false;
	if ((compiler->Lexem == '.') && (FD != nullptr)) {
		compiler->RdLex();
		compiler->AcceptKeyWord("X");
		if (!FD->IsIndexFile()) {
			compiler->OldError(108);
		}
		result = true;
	}
	return result;
}

bool TestFixVar(Compiler* compiler, CpOption Opt, FileD* FD1, FileD* FD2)
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

bool RdList(Compiler* compiler, pstring* S)
{
	auto result = false;
	if (compiler->Lexem != '(') return result;
	compiler->RdLex();
	// TODO: compiler !!! S = (pstring*)(compiler->RdStrFrml);
	compiler->Accept(')');
	result = true;
	return result;
}

Instr* RdPrintTxt(Compiler* compiler)
{
	auto PD = new Instr_edittxt(PInstrCode::_printtxt);
	compiler->RdLex();
	if (compiler->FindLocVar(&LVBD, &PD->TxtLV)) {
		compiler->RdLex();
		compiler->TestString(PD->TxtLV->f_typ);
	}
	else {
		RdPath(compiler, true, PD->TxtPath, PD->TxtCatIRec);
	}
	return PD;
}

Instr* RdEditTxt(Compiler* compiler)
{
	EdExitD* pX;
	auto PD = new Instr_edittxt(PInstrCode::_edittxt);
	compiler->RdLex();
	if (compiler->FindLocVar(&LVBD, &PD->TxtLV)) {
		compiler->RdLex();
		compiler->TestString(PD->TxtLV->f_typ);
	}
	else RdPath(compiler, true, PD->TxtPath, PD->TxtCatIRec);
	PD->EdTxtMode = EditorMode::Text;
	while (compiler->Lexem == ',') {
		compiler->RdLex();
		if (compiler->IsOpt("WW")) {
			compiler->Accept('(');
			if (compiler->Lexem == '(') { compiler->RdLex(); PD->WFlags = WNoPop; }
			compiler->RdW(PD->Ww);
			compiler->RdFrame(&PD->Hd, PD->WFlags);
			if (compiler->Lexem == ',') { compiler->RdLex(); PD->Atr = compiler->RdAttr(); }
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
				RdKeyCode(compiler, pX);
				if (compiler->Lexem == ',') { compiler->RdLex(); goto label2; }
				compiler->Accept(':');
				if (compiler->IsKeyWord("QUIT")) pX->Typ = 'Q';
				else if (!(compiler->Lexem == ',' || compiler->Lexem == ')')) {
					pX->Typ = 'P';
					pX->Proc = RdProcArg(compiler, 'T');
				}
				if (compiler->Lexem == ',') { compiler->RdLex(); goto label1; }
				compiler->Accept(')');
			}
			else
				if (RdHeadLast(compiler, PD)) {}
				else if (compiler->IsKeyWord("NOEDIT")) {
					PD->EdTxtMode = EditorMode::View;
				}
				else {
					compiler->Error(161);
				}
	}
	return PD;
}

Instr* RdPutTxt(Compiler* compiler)
{
	auto PD = new Instr_puttxt(); // GetPD(_puttxt, 11);
	compiler->RdLex();
	RdPath(compiler, true, PD->TxtPath1, PD->TxtCatIRec1);
	compiler->Accept(',');
	PD->Txt = compiler->RdStrFrml(nullptr);
	if (compiler->Lexem == ',') {
		compiler->RdLex();
		compiler->AcceptKeyWord("APPEND");
		PD->App = true;
	}
	return PD;
}

Instr* RdTurnCat(Compiler* compiler)
{
	Instr_turncat* PD = new Instr_turncat();
	compiler->RdLex();
	compiler->TestIdentif();
	PD->NextGenFD = compiler->FindFileD();
	const int first = catalog->GetCatalogIRec(compiler->LexWord, true);
	TestCatError(compiler, first, compiler->LexWord, true);
	compiler->RdLex();
	PD->FrstCatIRec = first;
	const std::string rdb_name = catalog->GetRdbName(first);
	const std::string file_name = catalog->GetFileName(first);
	int i = first + 1;
	while (catalog->GetCatalogFile()->GetNRecs() >= i
		&& EquUpCase(rdb_name, catalog->GetRdbName(i))
		&& EquUpCase(file_name, catalog->GetFileName(i))) {
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

void RdWriteln(Compiler* compiler, WriteType OpKind, Instr_writeln** pinstr)
{
	std::vector<WrLnD*> d;
	compiler->RdLex();
	FrmlElem* z = nullptr;
	WrLnD* w = new WrLnD();
	d.push_back(w);
//label1:
	while (true) {
		w->Frml = compiler->RdFrml(w->Typ, nullptr);

		if (w->Typ == 'R') {
			w->Typ = 'F';
			if (compiler->Lexem == ':') {
				compiler->RdLex();
				if (compiler->Lexem == _quotedstr) {
					w->Typ = 'D';
					w->Mask = compiler->LexWord;
					compiler->RdLex();
				}
				else {
					w->N = compiler->RdInteger();
					if (compiler->Lexem == ':') {
						compiler->RdLex();
						if (compiler->Lexem == '-') {
							compiler->RdLex();
							w->M = -compiler->RdInteger();
						}
						else {
							w->M = compiler->RdInteger();
						}
					}
				}
			}
		}

		if (compiler->Lexem == ',') {
			compiler->RdLex();
			if ((OpKind == WriteType::message) && compiler->IsOpt("HELP")) {
				z = compiler->RdStrFrml(nullptr);
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

Instr* RdReleaseDrive(Compiler* compiler)
{
	auto PD = new Instr_releasedrive(); // GetPD(_releasedrive, 4);
	compiler->RdLex();
	PD->Drive = compiler->RdStrFrml(nullptr);
	return PD;
}

Instr* RdIndexfile(Compiler* compiler)
{
	auto PD = new Instr_indexfile(); // GetPD(_indexfile, 5);
	compiler->RdLex();
	PD->IndexFD = compiler->RdFileName();
	if (!PD->IndexFD->IsIndexFile()) {
		compiler->OldError(108);
	}
	if (compiler->Lexem == ',') {
		compiler->RdLex();
		compiler->AcceptKeyWord("COMPRESS");
		PD->Compress = true;
	}
	return PD;
}

Instr* RdGetIndex(Compiler* compiler)
{
	LocVar* lv2 = nullptr; bool b = false; LinkD* ld = nullptr;
	auto PD = new Instr_getindex(); // GetPD(_getindex, 31);
	compiler->RdLex();
	LocVar* lv = RdIdxVar(compiler);
	PD->loc_var1 = lv; compiler->Accept(',');
	PD->mode = ' ';
	if (compiler->Lexem == '+' || compiler->Lexem == '-') {
		PD->mode = compiler->Lexem;
		compiler->RdLex();
		compiler->Accept(',');
		PD->condition = compiler->RdRealFrml(nullptr); /*RecNr*/
		return PD;
	}
	compiler->processing_F = compiler->RdFileName();
	if (lv->FD != compiler->processing_F) compiler->OldError(164);
	CViewKey = compiler->RdViewKey(lv->FD);
	PD->keys = CViewKey;
	while (compiler->Lexem == ',') {
		compiler->RdLex();
		if (compiler->IsOpt("SORT")) {
			if (!lv->key->KFlds.empty()) {
				compiler->OldError(175);
			}
			compiler->Accept('(');
			compiler->RdKFList(PD->key_fields, compiler->processing_F);
			compiler->Accept(')');
		}
		else if (compiler->IsOpt("COND")) {
			compiler->Accept('(');
			PD->condition = compiler->RdKeyInBool(PD->key_in_root, false, true, PD->sql_filter, nullptr);
			compiler->Accept(')');
		}
		else if (compiler->IsOpt("OWNER")) {
			PD->owner_type = RdOwner(compiler, compiler->processing_F, &PD->link, &PD->loc_var2);
			XKey* k = GetFromKey(PD->link);
			if (CViewKey == nullptr) PD->keys = k;
			else if (CViewKey != k) compiler->OldError(178);
		}
		else compiler->Error(167);
		if ((PD->owner_type != 0) && (PD->sql_filter || !PD->key_in_root.empty())) {
			compiler->Error(179);
		}
	}
	return PD;
}

Instr* RdGotoXY(Compiler* compiler)
{
	auto PD = new Instr_gotoxy(); // GetPD(_gotoxy, 8);
	compiler->RdLex();
	PD->GoX = compiler->RdRealFrml(nullptr);
	compiler->Accept(',');
	PD->GoY = compiler->RdRealFrml(nullptr);
	return PD;
}

Instr* RdClrWw(Compiler* compiler)
{
	auto PD = new Instr_clrww(); // GetPD(_clrww, 24);
	compiler->RdLex();
	compiler->RdW(PD->W2);
	if (compiler->Lexem == ',') {
		compiler->RdLex();
		if (compiler->Lexem != ',') PD->Attr2 = compiler->RdAttr();
		if (compiler->Lexem == ',') { compiler->RdLex(); PD->FillC = compiler->RdStrFrml(nullptr); }
	}
	return PD;
}

Instr* RdMount(Compiler* compiler)
{
	auto PD = new Instr_mount(); // GetPD(_mount, 3);
	compiler->RdLex();
	int i = 0;
	compiler->TestIdentif();
	FileD* FD = compiler->FindFileD();
	if (FD == nullptr) {
		i = catalog->GetCatalogIRec(compiler->LexWord, true);
	}
	else {
		i = FD->CatIRec;
	}
	TestCatError(compiler, i, compiler->LexWord, false);
	compiler->RdLex();
	PD->MountCatIRec = i;
	if (compiler->Lexem == ',') {
		compiler->RdLex();
		compiler->AcceptKeyWord("NOCANCEL");
		PD->MountNoCancel = true;
	}
	return PD;
}

Instr* RdDisplay(Compiler* compiler)
{
	auto PD = new Instr_merge_display(PInstrCode::_display); // GetPD(_display, sizeof(RdbPos));
	compiler->RdLex();
	pstring* s = nullptr;
	if ((compiler->Lexem == _identifier) && compiler->FindChpt('H', compiler->LexWord, false, &PD->Pos)) {
		compiler->RdLex();
	}
	else {
		PD->Pos.rdb = (Project*)compiler->RdStrFrml(nullptr);
		PD->Pos.i_rec = 0;
	}
	return PD;
}

Instr_graph* RdGraphP(Compiler* compiler)
{
	FrmlElem* FrmlArr[15];
	WORD i;

	pstring Nm1[11] = { "TYPE", "HEAD", "HEADX", "HEADY", "HEADZ", "FILL", "DIRX", "GRID", "PRINT", "PALETTE", "ASSIGN" };
	pstring Nm2[6] = { "WIDTH", "RECNO", "NRECS", "MAX", "MIN", "GRPOLY" };

	Instr_graph* PD = new Instr_graph();
	compiler->RdLex();
	PD->GD = new GraphD();

	GraphD* PDGD = PD->GD;
	if (compiler->IsOpt("GF")) PDGD->GF = compiler->RdStrFrml(nullptr);
	else {
		PDGD->FD = compiler->RdFileName();
		//CFile = PDGD->FD;
		CViewKey = compiler->RdViewKey(PDGD->FD);
		PDGD->ViewKey = CViewKey;
		compiler->Accept(',');
		compiler->Accept('(');
		PDGD->X = compiler->RdFldName(PDGD->FD);
		i = 0;
		do {
			compiler->Accept(',');
			PDGD->ZA[i] = compiler->RdFldName(PDGD->FD);
			i++;
		} while (!((i > 9) || (compiler->Lexem != ',')));
		compiler->Accept(')');
	}
	while (compiler->Lexem == ',') {
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
			if (compiler->Lexem == '(') {
				compiler->RdLex();
				PDGD->Cond = compiler->RdKeyInBool(PDGD->KeyIn, false, true, PDGD->SQLFilter, nullptr);
				compiler->Accept(')');
			}
			else PDGD->Cond = compiler->RdKeyInBool(PDGD->KeyIn, false, true, PDGD->SQLFilter, nullptr);
		}
		else if (compiler->IsOpt("TXT")) {
			GraphVD* VD = new GraphVD();
			//ChainLast(PDGD->V, VD);
			PDGD->V.push_back(VD);
			compiler->Accept('(');
			VD->XZ = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			VD->YZ = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			VD->Velikost = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			VD->BarPis = compiler->RdStrFrml(nullptr); compiler->Accept(',');
			VD->Text = compiler->RdStrFrml(nullptr); compiler->Accept(')');
		}
		else if (compiler->IsOpt("TXTWIN")) {
			GraphWD* WD = new GraphWD();
			//ChainLast(PDGD->W, WD);
			PDGD->W.push_back(WD);
			compiler->Accept('(');
			WD->XZ = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			WD->YZ = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			WD->XK = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			WD->YK = compiler->RdRealFrml(nullptr); compiler->Accept(',');
			WD->BarPoz = compiler->RdStrFrml(nullptr); compiler->Accept(',');
			WD->BarPis = compiler->RdStrFrml(nullptr); compiler->Accept(',');
			WD->Text = compiler->RdStrFrml(nullptr); compiler->Accept(')');
		}
		else if (compiler->IsOpt("RGB")) {
			GraphRGBD* RGBD = new GraphRGBD();
			//ChainLast(PDGD->RGB, RGBD);
			PDGD->RGB.push_back(RGBD);
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
			WinG* Ww = new WinG();
			compiler->Accept('(');
			if (compiler->Lexem == '(') { compiler->RdLex(); Ww->WFlags = WNoPop; }
			compiler->RdW(Ww->W);
			compiler->RdFrame(Ww->Top, Ww->WFlags);
			if (compiler->Lexem == ',') {
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

Instr_recs* RdMixRecAcc(Compiler* compiler, PInstrCode Op)
{
	Instr_recs* PD = nullptr;
	FrmlElem* Z = nullptr;
	char FTyp = '\0';
	//FileD* cf = CFile;
	if ((Op == PInstrCode::_appendRec) || (Op == PInstrCode::_recallrec)) {
		// PD = GetPD(oper, 9);
		PD = new Instr_recs(Op);
		compiler->RdLex();
		//CFile = compiler->RdFileName();
		PD->RecFD = compiler->RdFileName();
#ifdef FandSQL
		if (CFile->typSQLFile) OldError(155);
#endif
		if (Op == PInstrCode::_recallrec) {
			compiler->Accept(',');
			PD->RecNr = compiler->RdRealFrml(nullptr);
		}
	}
	else {
		// PD = GetPD(oper, 15);
		PD = new Instr_recs(Op);
		compiler->RdLex();
		FileD* file = nullptr;
		if (Op == PInstrCode::_deleterec) {
			file = compiler->RdFileName();
			PD->RecFD = file;
		}
		else { /*_readrec,_writerec*/
			if (!IsRecVar(compiler, &PD->LV)) compiler->Error(141);
			file = PD->LV->FD;
		}
		XKey* K = compiler->RdViewKey(file);
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
			if (K == nullptr) K = file->Keys.empty() ? nullptr : file->Keys[0];
			PD->Key = K;
			if ((K == nullptr) && (!file->IsParFile || (Z->Op != _const)
				|| (((FrmlElemString*)Z)->S.length() > 0))) compiler->OldError(24);
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
	if ((compiler->Lexem == ',') && (Op == PInstrCode::_writerec || Op == PInstrCode::_deleterec || Op == PInstrCode::_recallrec)) {
		compiler->RdLex();
		compiler->Accept('+');
		PD->AdUpd = true;
	}
	//CFile = cf;
	return PD;
}

Instr* RdLinkRec(Compiler* compiler)
{
	LocVar* LV = nullptr;
	LinkD* LD = nullptr;
	auto PD = new Instr_assign(PInstrCode::_linkrec); // GetPD(_linkrec, 12);
	compiler->RdLex();
	if (!IsRecVar(compiler, &PD->RecLV1)) compiler->Error(141);
	compiler->Accept(',');
	//CFile = PD->RecLV1->FD;
	if (IsRecVar(compiler, &LV)) {
		LD = compiler->FindLD(PD->RecLV1->FD, LV->FD->Name);
		if (LD == nullptr) compiler->OldError(154);
	}
	else {
		compiler->TestIdentif();
		LD = compiler->FindLD(PD->RecLV1->FD, compiler->LexWord);
		if (LD == nullptr) compiler->Error(9);
		compiler->RdLex();
		compiler->Accept('(');
		LV = RdRecVar(compiler);
		if (LD->ToFile != LV->FD) compiler->OldError(141);
		compiler->Accept(')');
	}
	PD->RecLV2 = LV;
	PD->LinkLD = LD;
	return PD;
}

Instr* RdSetEditTxt(Compiler* compiler)
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
	if (compiler->Lexem == ',') { compiler->RdLex(); goto label1; }
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

std::vector<AssignD*> MakeImplAssign(Compiler* compiler, FileD* FD1, FileD* FD2)
{
	std::vector<AssignD*> ARoot;
	char FTyp;
	pstring S = compiler->LexWord;
	for (FieldDescr* F1 : FD1->FldD) {
		if ((F1->Flg & f_Stored) != 0) {
			compiler->LexWord = F1->Name;
			FieldDescr* F2 = compiler->FindFldName(FD2);
			if (F2 != nullptr) {
				AssignD* A = new AssignD();
				//if (ARoot == nullptr) ARoot = A;
				//else ChainLast(ARoot, A);
				ARoot.push_back(A);
				if ((F2->frml_type != F1->frml_type)
					|| (F1->frml_type == 'R')
					&& (F1->field_type != F2->field_type)) {
					A->Kind = MInstrCode::_zero;
					A->outputField = F1;
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
	compiler->LexWord = S;
	return ARoot;
}

Instr_assign* RdAssign(Compiler* compiler)
{
	FileD* FD = nullptr; FieldDescr* F = nullptr;
	LocVar* LV = nullptr; LocVar* LV2 = nullptr; 
	char PV;
	Instr_assign* PD = nullptr; 
	std::string FName; 
	char FTyp = 0;

	if (compiler->ForwChar == '.')
		if (compiler->FindLocVar(&LVBD, &LV) && (LV->f_typ == 'r' || LV->f_typ == 'i')) {
			FTyp = LV->f_typ;
			compiler->RdLex(); compiler->RdLex();
			if (FTyp == 'i') {
				compiler->AcceptKeyWord("NRECS");
				compiler->Accept(_assign);
				if ((compiler->Lexem != _number) || (compiler->LexWord != "0")) compiler->Error(183);
				compiler->RdLex();
				PD = new Instr_assign(PInstrCode::_asgnxnrecs);
				PD->xnrIdx = LV->key;
			}
			else {
				PD = new Instr_assign(PInstrCode::_asgnrecfld);
				PD->AssLV = LV;
				F = compiler->RdFldName(LV->FD);
				PD->RecFldD = F;
				if ((F->Flg & f_Stored) == 0) compiler->OldError(14);
				FTyp = F->frml_type;
				compiler->RdAssignFrml(FTyp, PD->Add, &PD->Frml, nullptr);
			}
		}
		else {
			FName = compiler->LexWord;
			FD = compiler->FindFileD();
			if (FD->IsActiveRdb()) {
				compiler->Error(121);
			}
			compiler->RdLex(); compiler->RdLex();
			if (compiler->IsKeyWord("ARCHIVES")) {
				F = catalog->CatalogArchiveField();
				goto label1;
			}
			if (compiler->IsKeyWord("PATH")) {
				F = catalog->CatalogPathNameField();
				goto label1;
			}
			if (compiler->IsKeyWord("VOLUME")) {
				F = catalog->CatalogVolumeField();
			label1:
				PD = new Instr_assign(PInstrCode::_asgnCatField);
				PD->FD3 = FD;
				PD->CatIRec = catalog->GetCatalogIRec(FName, true);
				PD->CatFld = F;
				TestCatError(compiler, PD->CatIRec, FName, true);
				compiler->Accept(_assign);
				PD->Frml3 = compiler->RdStrFrml(nullptr);
			}
			else if (FD == nullptr) compiler->OldError(9);
			else if (compiler->IsKeyWord("NRECS")) {
				if (FD->FF->file_type == FandFileType::RDB) { compiler->OldError(127); }
				PD = new Instr_assign(PInstrCode::_asgnnrecs);
				PD->FD = FD;
				FTyp = 'R';
				compiler->RdAssignFrml(FTyp, PD->Add, &PD->Frml, nullptr);
			}
			else {
				if (!FD->IsParFile) compiler->OldError(64);
				PD = new Instr_assign(PInstrCode::_asgnpar);
				PD->FD = FD;
				F = compiler->RdFldName(FD);
				PD->FldD = F;
				if ((F->Flg & f_Stored) == 0) compiler->OldError(14);
				FTyp = F->frml_type;
				compiler->RdAssignFrml(FTyp, PD->Add, &PD->Frml, nullptr);
			}
		}
	else if (compiler->ForwChar == '[') {
		PD = new Instr_assign(PInstrCode::_asgnField);
		FD = compiler->RdFileName();
		PD->FD = FD; compiler->RdLex();
#ifdef FandSQL
		if (v_files->typSQLFile) OldError(155);
#endif
		PD->RecFrml = compiler->RdRealFrml(nullptr);
		compiler->Accept(']');
		compiler->Accept('.');
		F = compiler->RdFldName(FD);
		PD->FldD = F;
		if (!F->isStored()) {
			compiler->OldError(14);
		}
		PD->Indexarg = FD->IsIndexFile() && compiler->IsKeyArg(F, FD);
		compiler->RdAssignFrml(F->frml_type, PD->Add, &PD->Frml, nullptr);
	}
	else if (compiler->FindLocVar(&LVBD, &LV)) {
		compiler->RdLex();
		FTyp = LV->f_typ;
		switch (FTyp) {
		case 'f':
		case 'i': compiler->OldError(140); break;
		case 'r': {
			compiler->Accept(_assign);
			if (!IsRecVar(compiler, &LV2)) compiler->Error(141);
			PD = new Instr_assign(PInstrCode::_asgnrecvar);
			PD->RecLV1 = LV;
			PD->RecLV2 = LV2;
			PD->Ass = MakeImplAssign(compiler, LV->FD, LV2->FD);
			break;
		}
		default: {
			PD = new Instr_assign(PInstrCode::_asgnloc);
			PD->AssLV = LV; 
			compiler->RdAssignFrml(FTyp, PD->Add, &PD->Frml, nullptr);
			break;
		}
		}
	}
	else if (compiler->IsKeyWord("USERNAME")) {
		PD = new Instr_assign(PInstrCode::_asgnusername);
		compiler->Accept(_assign);
		PD->Frml = compiler->RdStrFrml(nullptr);
	}
	else if (compiler->IsKeyWord("CLIPBD")) {
		PD = new Instr_assign(PInstrCode::_asgnClipbd);
		compiler->Accept(_assign);
		PD->Frml = compiler->RdStrFrml(nullptr);
	}
	else if (compiler->IsKeyWord("ACCRIGHT")) {
		PD = new Instr_assign(PInstrCode::_asgnAccRight);
		compiler->Accept(_assign);
		PD->Frml = compiler->RdStrFrml(nullptr);
	}
	else if (compiler->IsKeyWord("EDOK")) {
		PD = new Instr_assign(PInstrCode::_asgnEdOk);
		compiler->Accept(_assign);
		PD->Frml = compiler->RdBool(nullptr);
	}
	else if (compiler->IsKeyWord("RANDSEED")) {
		PD = new Instr_assign(PInstrCode::_asgnrand);
		compiler->Accept(_assign);
		PD->Frml = compiler->RdRealFrml(nullptr);
	}
	else if (compiler->IsKeyWord("TODAY")) {
		PD = new Instr_assign(PInstrCode::_asgnusertoday);
		compiler->Accept(_assign);
		PD->Frml = compiler->RdRealFrml(nullptr);
	}
	else if (compiler->IsKeyWord("USERCODE")) {
		PD = new Instr_assign(PInstrCode::_asgnusercode);
		compiler->Accept(_assign);
		PD->Frml = compiler->RdRealFrml(nullptr);
	}
	else {
		compiler->RdLex();
		if (compiler->Lexem == _assign) compiler->OldError(8);
		else compiler->OldError(34);
	}
	return PD;
}

Instr* RdWith(Compiler* compiler)
{
	Instr* P = nullptr;
	PInstrCode Op;

	if (compiler->IsKeyWord("WINDOW")) {
		P = new Instr_window(); //GetPInstr(_window, 29);
		auto iP = (Instr_window*)P;
		compiler->Accept('(');
		if (compiler->Lexem == '(') {
			compiler->RdLex();
			iP->WithWFlags = WNoPop;
		}
		compiler->RdW(iP->W);
		compiler->RdFrame(&iP->Top, iP->WithWFlags);
		if (compiler->Lexem == ',') {
			compiler->RdLex();
			iP->Attr = compiler->RdAttr();
		}
		compiler->Accept(')');
		if ((iP->WithWFlags & WNoPop) != 0) compiler->Accept(')');
		compiler->AcceptKeyWord("DO");
		iP->v_ww_instr = RdPInstr(compiler);
	}
	else if (compiler->IsKeyWord("SHARED")) {
		Op = PInstrCode::_withshared;
		goto label1;
	}
	else if (compiler->IsKeyWord("LOCKED")) {
		Op = PInstrCode::_withlocked;
	label1:
		P = new Instr_withshared(Op);
		Instr_withshared* iP = (Instr_withshared*)P;

		while (true) {
			LockD* ld = new LockD();
			iP->WLD.push_back(ld);

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
			if (compiler->Lexem == ',') {
				compiler->RdLex();
				continue;
			}
			break;
		}
		compiler->AcceptKeyWord("DO");
		iP->WDoInstr = RdPInstr(compiler);
		if (compiler->IsKeyWord("ELSE")) {
			iP->WasElse = true;
			iP->WElseInstr = RdPInstr(compiler);
		}
	}
	else if (compiler->IsKeyWord("GRAPHICS")) {
		P = new Instr_withshared(PInstrCode::_withgraphics);
		compiler->AcceptKeyWord("DO");
		((Instr_withshared*)P)->WDoInstr = RdPInstr(compiler);
	}
	else {
		compiler->Error(131);
	}
	return P;
}

Instr_assign* RdUserFuncAssign(Compiler* compiler)
{
	LocVar* lv = nullptr;
	if (!compiler->FindLocVar(&LVBD, &lv)) {
		compiler->Error(34);
	}
	compiler->RdLex();
	Instr_assign* pd = new Instr_assign(PInstrCode::_asgnloc);
	pd->AssLV = lv;
	compiler->RdAssignFrml(lv->f_typ, pd->Add, &pd->Frml, nullptr);
	return pd;
}

std::vector<Instr*> RdPInstr(Compiler* compiler)
{
	Instr* single_instr = nullptr;
	std::vector<Instr*> result;

	if (compiler->IsKeyWord("IF")) single_instr = RdIfThenElse(compiler);
	else if (compiler->IsKeyWord("WHILE")) single_instr = RdWhileDo(compiler);
	else if (compiler->IsKeyWord("REPEAT")) single_instr = RdRepeatUntil(compiler);
	else if (compiler->IsKeyWord("CASE")) single_instr = RdCase(compiler);
	else if (compiler->IsKeyWord("FOR")) { result = RdFor(compiler); }			// creates vector of instructions
	else if (compiler->IsKeyWord("BEGIN")) { result = RdBeginEnd(compiler); }	// creates vector of instructions
	else if (compiler->IsKeyWord("BREAK")) single_instr = new Instr(PInstrCode::_break);
	else if (compiler->IsKeyWord("EXIT")) single_instr = new Instr(PInstrCode::_exitP);
	else if (compiler->IsKeyWord("CANCEL")) single_instr = new Instr(PInstrCode::_cancel);
	else if (compiler->Lexem == ';') single_instr = nullptr;
	else if (IsRdUserFunc) single_instr = RdUserFuncAssign(compiler);
	else if (compiler->IsKeyWord("MENULOOP")) single_instr = RdMenuBox(compiler, true);
	else if (compiler->IsKeyWord("MENU")) single_instr = RdMenuBox(compiler, false);
	else if (compiler->IsKeyWord("MENUBAR")) single_instr = RdMenuBar(compiler);
	else if (compiler->IsKeyWord("WITH")) single_instr = RdWith(compiler);
	else if (compiler->IsKeyWord("SAVE")) single_instr = new Instr(PInstrCode::_save);
	else if (compiler->IsKeyWord("CLREOL")) single_instr = new Instr(PInstrCode::_clreol);
	else if (compiler->IsKeyWord("FORALL")) single_instr = RdForAll(compiler);
	else if (compiler->IsKeyWord("CLEARKEYBUF")) single_instr = new Instr(PInstrCode::_clearkeybuf);
	else if (compiler->IsKeyWord("WAIT")) single_instr = new Instr(PInstrCode::_wait);
	else if (compiler->IsKeyWord("BEEP")) single_instr = new Instr(PInstrCode::_beepP);
	else if (compiler->IsKeyWord("NOSOUND")) single_instr = new Instr(PInstrCode::_nosound);
#ifndef FandRunV
	else if (compiler->IsKeyWord("MEMDIAG")) single_instr = new Instr(PInstrCode::_memdiag);
#endif 
	else if (compiler->IsKeyWord("RESETCATALOG")) single_instr = new Instr(PInstrCode::_resetcat);
	else if (compiler->IsKeyWord("RANDOMIZE")) single_instr = new Instr(PInstrCode::_randomize);
	else if (compiler->Lexem == _identifier) {
		compiler->SkipBlank(false);
		if (compiler->ForwChar == '(') {
			RdProcCall(compiler , &single_instr); // funkce muze ovlivnit single_instruction
		}
		else if (compiler->IsKeyWord("CLRSCR")) single_instr = new Instr(PInstrCode::_clrscr);
		else if (compiler->IsKeyWord("GRAPH")) single_instr = new Instr_graph();
		else if (compiler->IsKeyWord("CLOSE")) single_instr = new Instr_closefds();
		else single_instr = RdAssign(compiler);
	}
	else compiler->Error(34);

	if (result.empty() && single_instr != nullptr) {
		result.push_back(single_instr);
	}

	return result;
}

void ReadProcHead(Compiler* compiler, const std::string& name)
{
	gc->ResetCompilePars();
	compiler->rdFldNameType = FieldNameType::P;
	compiler->rdFuncType = ReadFuncType::P;
	//ptrRdFldNameFrml = RdFldNameFrmlP;
	//RdFunction = RdFunctionP;
	FileVarsAllowed = false;
	IdxLocVarAllowed = true;
	IsRdUserFunc = false;
	compiler->RdLex();
	ResetLVBD();
	LVBD.func_name = name;
	if (compiler->Lexem == '(') {
		compiler->RdLex();
		compiler->RdLocDcl(&LVBD, true, true, 'P');
		compiler->Accept(')');
	}
	if (compiler->IsKeyWord("VAR")) {
		compiler->RdLocDcl(&LVBD, false, true, 'P');
	}
}

std::vector<Instr*> ReadProcBody(Compiler* compiler)
{
	compiler->AcceptKeyWord("BEGIN");
	std::vector<Instr*> result = RdBeginEnd(compiler);
	compiler->Accept(';');
	if (compiler->Lexem != 0x1A) {
		std::string error40 = compiler->Error(40);
		std::string err_msg = "ReadProcBody exception: " + error40;
		throw std::exception(err_msg.c_str());
	}
	return result;
}

// metoda nacita funkce a procedury z InpArrPtr a postupne je zpracovava
// nacte nazev, parametry, navr. hodnotu, promenne, konstanty i kod
void ReadDeclChpt(Compiler* compiler)
{
	char typ = '\0';
	compiler->RdLex();
	while (true) {
		if (compiler->IsKeyWord("FUNCTION")) {
			compiler->TestIdentif();

			//FuncD* fc = FuncDRoot;
			//while (fc != CRdb->OldFCRoot) {
			//	if (EquUpCase(fc->name, LexWord)) {
			//		compiler->Error(26);
			//	}
			//	fc = fc->Chain;
			//}

			int32_t items = FuncDRoot.size() - CRdb->OldFCRoot.size();
			std::deque<FuncD*>::iterator it0 = FuncDRoot.begin();
			for (int32_t i = 0; i < items; i++) {
				if (EquUpCase((*it0)->name, compiler->LexWord)) {
					compiler->Error(26);
				}
				++it0;
			}

			FuncD* fc = new FuncD();
			//fc->Chain = FuncDRoot;
			//FuncDRoot = fc;
			FuncDRoot.push_front(fc);
			fc->name = compiler->LexWord;
			compiler->rdFldNameType = FieldNameType::P;
			compiler->rdFuncType = ReadFuncType::P;
			//ptrChainSumEl = nullptr;
			FileVarsAllowed = false;
			IsRdUserFunc = true;
			compiler->RdLex();
			ResetLVBD();
			LVBD.func_name = fc->name;
			compiler->Accept('(');
			if (compiler->Lexem != ')') {
				compiler->RdLocDcl(&LVBD, true, false, 'D'); // nacte parametry funkce
			}
			compiler->Accept(')');
			compiler->Accept(':');
			// nacte typ navratove hodnoty
			if (compiler->IsKeyWord("REAL")) {
				typ = 'R';
			}
			else if (compiler->IsKeyWord("STRING")) {
				typ = 'S';
			}
			else if (compiler->IsKeyWord("BOOLEAN")) {
				typ = 'B';
			}
			else {
				compiler->Error(39);
			}
			LocVar* lv = new LocVar();
			LVBD.variables.push_back(lv);
			lv->name = fc->name;
			lv->is_return_value = true;
			lv->f_typ = typ;
			lv->oper = _getlocvar;
			fc->FTyp = typ;
			compiler->Accept(';');
			// nacte promenne
			if (compiler->IsKeyWord("VAR")) {
				compiler->RdLocDcl(&LVBD, false, false, 'D');
			}
			fc->LVB = LVBD;
			// nacte kod funkce (procedury)
			compiler->AcceptKeyWord("BEGIN");
			fc->v_instr = RdBeginEnd(compiler);
			compiler->Accept(';');
		}
		else if (compiler->Lexem == 0x1A) {
			return;
		}
		else {
			compiler->Error(40);
			return;
		}
	}
}

FrmlElem* GetEvalFrml(FileD* file_d, FrmlElemEval* X, Record* record)
{
	//FileD* cf = CFile;
	//CFile = file_d;
	//uint8_t* cr = CRecPtr;
	//CRecPtr = record;

	LocVarBlock oldLVBD = LVBD;
	//LVBD = Compiler::ProcStack.front();

	FrmlElem* result = nullptr;

	std::string s = RunString(file_d, X->eval_elem, record);
	if (s.empty()) {
		LastExitCode = 0;
	}
	else {
		std::unique_ptr<Compiler> local_compiler = std::make_unique<Compiler>();
		LastExitCode = 1;
		local_compiler->rdFldNameType = FieldNameType::P;
		local_compiler->rdFuncType = ReadFuncType::P;

		if (X->eval_file == nullptr) {
			FileVarsAllowed = false;
		}
		else {
			local_compiler->processing_F = X->eval_file;
			FileVarsAllowed = true;
		}

		try {
			char fTyp = '\0';
			local_compiler->SetInpStdStr(s, false);
			local_compiler->RdLex();
			result = local_compiler->RdFrml(fTyp, nullptr);
			if ((fTyp != X->eval_type) || (gc->Lexem != 0x1A)) {
				result = nullptr;
			}
			else {
				LastExitCode = 0;
			}
		}
		catch (const std::exception& e) {
			// std::string err_msg = "GetEvalFrml exception: " + std::string(e.what());
			// throw std::exception(err_msg.c_str());
		}

		size_t cpos = gc->input_pos;

		if (LastExitCode != 0) {
			LastTxtPos = cpos;
			if (X->eval_type == 'B') {
				result = new FrmlElemBool(_const, 0, false);
				// z->B = false;
			}
		}
	}

	//CFile = cf; CRecPtr = cr;
	LVBD = oldLVBD; /*for cond before cycle called when PushProcStk is !ready*/

	return result;
}

Instr* RdBackup(Compiler* compiler, char MTyp, bool IsBackup)
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
	for (int i = 1; i <= catalog->GetCatalogFile()->GetNRecs(); i++) {
		if (EquUpCase(catalog->GetRdbName(i), "ARCHIVES") && EquUpCase(catalog->GetFileName(i), compiler->LexWord)) {
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
		while (compiler->Lexem == ',') {
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

Instr* RdCallLProc(Compiler* compiler)
{
	Instr_lproc* pd = new Instr_lproc();
	compiler->RdLex();
	pd->name = compiler->RdChptName('L', &pd->lpPos, true);
	if (compiler->Lexem == ',') {
		compiler->RdLex();
		compiler->TestIdentif();
		//pd->lpName = LexWord;
		compiler->RdLex();
	}
	return pd;
}
