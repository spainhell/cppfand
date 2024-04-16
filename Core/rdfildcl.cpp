#include "rdfildcl.h"

#include "../Common/compare.h"
#include "../fandio/FandXFile.h"
#include "compile.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "ChkD.h"
#include "KeyFldD.h"
#include "rdproc.h"
#include "runfrml.h"

bool HasTT;
bool issql;

FieldDescr* RdFieldDescr(std::string name, bool Stored)
{
	const BYTE TabF[19] = { 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8 };
	
	WORD L = 0, M = 0, NBytes = 0;
	BYTE Flg = 0;
	char FrmlTyp = 0, c = 0;
	std::string sstr;

	FieldDescr* F = new FieldDescr();
	F->Name = name;
	if (Stored) Flg = f_Stored;
	else Flg = 0;
	Accept(':');
	if ((Lexem != _identifier) || (LexWord.length() > 1)) Error(10);

	FieldType Typ = FieldDescr::GetFieldType((char)LexWord[1]);

	RdLex();
	FrmlTyp = 'S';
	M = 0;
	if (Typ == FieldType::NUMERIC || Typ == FieldType::FIXED) {
		Accept(',');
		L = RdInteger();
	}
	switch (Typ) {
	case FieldType::NUMERIC: {
		NBytes = (L + 1) / 2;
		if (CurrChar == 'L') {
			RdLex();
			M = LeftJust;
		}
		break;
	}
	case FieldType::FIXED: {
		if (Lexem == ',') { Flg += f_Comma; RdLex(); }
		else Accept('.');
		M = RdInteger();
		if ((M > 15) || (L + M > 18)) OldError(3);
		NBytes = TabF[L + M];
		if (M == 0) L++;
		else L += (M + 2);
		FrmlTyp = 'R';
		break;
	}
	case FieldType::REAL: {
		NBytes = 6;
		FrmlTyp = 'R';
		L = 17;
		M = 5;
		break;
	}
	case FieldType::ALFANUM: {
		Accept(',');
		if (!Stored || (Lexem != _quotedstr)) {
			L = RdInteger();
			if (L > 255) Error(3);
			if (CurrChar == 'R') RdLex();
			else M = LeftJust;
		}
		else {
			WORD n1 = 0;
			WORD n = 0;
			sstr = LexWord;
			Accept(_quotedstr);
			L = 0; c = '?'; n = 0;
			for (size_t i = 0; i < sstr.length(); i++) {
				switch (sstr[i]) {
				case '[': {
					if (c == '?') c = '[';
					else goto label1;
					break;
				}
				case ']': {
					if (c == '[') c = '?';
					else goto label1;
					break;
				}
				case '(': {
					if (c == '?') {
						c = '(';
						n1 = 0;
						n = 0;
					}
					else goto label1;
					break;
				}
				case ')': {
					if ((c == '(') && (n1 > 0) && (n > 0)) {
						c = '?';
						L += MaxW(n1, n);
					}
					else goto label1;
					break;
				}
				case '|': {
					if ((c == '(') && (n1 > 0)) {
						n = MaxW(n1, n);
						n1 = 0;
					}
					else goto label1;
					break;
				}
				default: {
					if (c == '(') n1++;
					else L++;
					break;
				}
				}
			}
			Flg += f_Mask;
			M = LeftJust;
			if (c != '?')
				label1:
			Error(171);
		}
		NBytes = L;
		if (Stored && (Lexem == '!')) {
			RdLex();
			Flg += f_Encryp;
		}
		break;
	}
	case FieldType::DATE: {
		sstr = "";
		if (Lexem == ',') {
			RdLex();
			sstr = LexWord;
			Accept(_quotedstr);
		}
		if (sstr.empty()) sstr = "DD.MM.YY";
		// UPDATE: stejne to nefunguje, pri spusteni ulohy se to odnekud nacita znovu, tezko rict odkud
		// nazev se pak zpetne vytahne pomoci funkce FieldDMask()
		FrmlTyp = 'R'; NBytes = 6; // sizeof(float); // v Pascalu je to 6B
		L = sstr.length();
		Flg += f_Mask;
		break;
	}
	case FieldType::BOOL: {
		L = 1;
		NBytes = 1;
		FrmlTyp = 'B';
		break;
	}
	case FieldType::TEXT: {
		if (Lexem == ',') {
			RdLex();
			L = RdInteger() + 2;
		}
		else {
			L = 1;
		}
		NBytes = sizeof(int);
		HasTT = true;
		if (Stored && (Lexem == '!')) {
			RdLex();
			Flg += f_Encryp;
		}
		break;
	}
	default: {
		OldError(10);
		break;
	}
	}
	if (NBytes == 0) OldError(113);
	if ((L > TxtCols - 1) && (Typ != FieldType::ALFANUM)) OldError(3);
	F->field_type = Typ;
	F->frml_type = FrmlTyp;
	F->L = L;
	F->M = M;
	F->NBytes = NBytes;
	F->Flg = Flg;
	F->Mask = sstr;
	return F;
}

ChkD* RdChkD(WORD Low)
{
	ChkD* C = new ChkD();
	ChkD* result = C;
	C->Bool = RdBool(nullptr);
	WORD Upper = OldErrPos;
	if (Lexem == '?') { RdLex(); C->Warning = true; }
	if (Lexem == ':') { RdLex(); C->TxtZ = RdStrFrml(nullptr); }
	else {
		WORD N = Upper - Low;
		//if (N > sizeof(pstring)) N = pred(sizeof(pstring));
		FrmlElem* Z = new FrmlElemString(_const, 0); // GetOp(_const, N + 1);
		C->TxtZ = Z;
		auto iZ = (FrmlElemString*)Z;
		iZ->S = std::string((char*)&InpArrPtr[Low], N);
		//char(N);
		//Move(&InpArrPtr[Low], &iZ->S[1], N);
	}
	if (Lexem == ',') {
		RdLex();
		C->HelpName = RdHelpName();
	}
	return result;
}

void RdChkDChain(ChkD** CRoot)
{
	WORD Low = 0;
	SkipBlank(false);
	Low = CurrPos;
	RdLex();
label1:
	if (*CRoot == nullptr) *CRoot = RdChkD(Low);
	else ChainLast(*CRoot, RdChkD(Low));
	if (Lexem == ';') {
		SkipBlank(false); Low = CurrPos; RdLex();
		if (!(Lexem == '#' || Lexem == 0x1A)) goto label1;
	}
}

void RdChkDsFromPos(FileD* FD, ChkD* C)
{
	if (FD->OrigFD != nullptr) {
		// this rdb_file was created as 'LIKE'
		RdChkDsFromPos(FD->OrigFD, C);
	}
	if (FD->ChptPos.rdb == nullptr) return;
	if (FD->TxtPosUDLI == 0) return;
	ResetCompilePars();
	SetInpTTxtPos(FD);
	RdLex();
	while (!(ForwChar == 'L' || ForwChar == 0x1A)) {
		do { RdLex(); } while (!(Lexem == 0x1A || Lexem == '#'));
	}
	if (Lexem == 0x1A) return;
	RdLex();
	FileD* cf = CFile;
	CFile = FD;
	RdChkDChain(&C);
	CFile = cf;
}

void RdBegViewDcl(EditOpt* EO)
{
	bool neg = false, all = false;
	FieldListEl* fl = nullptr;
	if ((Lexem == _identifier || Lexem == '[')) {
		RdChptName('E', &EO->FormPos, true);
		return;
	}
	neg = false; all = false;
	std::vector<FieldDescr*> fl1;
	EO->UserSelFlds = false;
	if (Lexem == '^') { RdLex(); neg = true; } Accept('(');
	if (Lexem == _identifier) RdFldList(fl1);
	else neg = true;
label1:
	switch (Lexem) {
	case '!': if (neg) { RdLex(); all = true; goto label1; } break;
	case '?': { RdLex(); EO->UserSelFlds = true; goto label1; break; }
	}
	Accept(')');
	if (!neg) { EO->Flds = fl1; return; }
	EO->Flds.clear();
	for (auto& f : CFile->FldD) {
		if (((f->isStored()) || all) && !FieldInList(f, fl1)) {
			EO->Flds.push_back(f);
		}
	}
	if (EO->Flds.empty()) OldError(117);
}

void RdByteList(pstring* s)
{
	Accept('(');
	short l = 0;

	while (true) {
		short i1 = RdInteger();
		short i2 = i1;
		if (i1 < 0) OldError(133);
		if (Lexem == _subrange) {
			RdLex();
			i2 = RdInteger();
			if (i2 < i1) OldError(133);
		}
		if ((i2 > 255) || (l + i2 - i1 >= 255)) OldError(133);
		for (short i = i1; i <= i2; i++) {
			l++;
			(*s)[l] = (char)i;
		}
		if (Lexem == ',') {
			RdLex();
			continue;
		}
		break;
	}

	(*s)[0] = (char)l;
	Accept(')');
}

void RdByteListInStore()
{
	pstring s;
	RdByteList(&s);
	StoreStr(s);
}

bool RdUserView(FileD* file_d, std::string ViewName, EditOpt* EO)
{
	// TODO: proc je tady 'EOD' a proc se kopiruje tam a zpet z/do EO ???
	bool found = false, Fin = false, FVA = false;
	XKey* K = nullptr;

	while (true) {
		if (file_d->TxtPosUDLI == 0) {
			file_d = file_d->OrigFD;
			if ((file_d != nullptr) && !found) continue;
			break;
		}
		ResetCompilePars();
		SetInpTTxtPos(file_d);
		RdLex();
		if ((Lexem != '#') || (ForwChar != 'U')) {
			file_d = file_d->OrigFD;
			if ((file_d != nullptr) && !found) continue;
			break;
		}
		RdLex(); // #
		RdLex(); // U
		while (true) {
			std::string sLexWord = LexWord;
			if (EquUpCase(ViewName, sLexWord)) found = true;
			EO->ViewName = LexWord;
			RdLex(); /*'('*/
			do {
				RdLex();
			} while (!(Lexem == ')' || Lexem == 0x1A));
			RdLex();
			RdLex();/*"):"*/
			K = RdViewKey();
			if (K != nullptr) {
				RdLex();/*','*/
				EO->ViewKey = K;
			}
			RdBegViewDcl(EO);
			while (Lexem == ',') {
				FVA = FileVarsAllowed;
				FileVarsAllowed = false;
				if (!RdViewOpt(EO)) Error(44);
				FileVarsAllowed = FVA;
			}
			if (!found && (Lexem == ';')) {
				RdLex();
				if (!(Lexem == '#' || Lexem == 0x1A)) continue;
			}
			break;
		}

		file_d = file_d->OrigFD;
		if ((file_d != nullptr) && !found) continue;
		break;
	}
	return found;
}

void TestUserView()
{
	EditOpt EO = EditOpt();
	EO.UserSelFlds = true;
	RdLex();
	while (true) {
		TestIdentif();
		TestDupl(CFile);
		FileD* FD = FileDRoot;
		while (FD != nullptr) {
			TestDupl(FD);
			FD = (FileD*)FD->pChain;
		}
		StringListEl* S = new StringListEl();
		S->S = LexWord;
		if (CFile->ViewNames == nullptr) {
			CFile->ViewNames = S;
		}
		else {
			ChainLast(CFile->ViewNames, S);
		}
		RdLex();
		RdByteListInStore();
		Accept(':');
		// GetEditOpt(); // vytvori objekt EditOpt
		XKey* K = RdViewKey(); // nacteni klice, podle ktereho budou polozky setrideny
		if (K != nullptr) {
			Accept(',');
			EO.ViewKey = K;
		}
		RdBegViewDcl(&EO);
		while (Lexem == ',') {
			if (!RdViewOpt(&EO)) Error(44);
		}
		if (Lexem == ';') {
			RdLex();
			if (!(Lexem == '#' || Lexem == 0x1A)) continue;
		}
		break;
	}
}

void TestDupl(FileD* FD)
{
	StringList S;
	S = FD->ViewNames;
	while (S != nullptr) {
		std::string tmp = LexWord;
		if (EquUpCase(S->S, tmp)) Error(26);
		S = (StringList)S->pChain;
	}
}

void RdFieldDList(bool Stored)
{
	FieldDescr* F = nullptr;
	char FTyp = 0;
	FrmlElem* Z = nullptr;

	while (true) {
		TestIdentif();
		std::string name = LexWord;
		F = FindFldName(CFile);
		if (F != nullptr) Error(26);
		RdLex();
		if (!Stored) {
			Accept(_assign);
			Z = RdFrml(FTyp, nullptr);
		}
		F = RdFieldDescr(name, Stored);
		if ((CFile->FF->file_type == FileType::DBF) && Stored && (F->field_type == FieldType::REAL || F->field_type == FieldType::NUMERIC)) {
			OldError(86);
		}

		CFile->FldD.push_back(F);
		ChainLast(CFile->FldD.front(), F);

		if (Stored) {
			if (CFile->FF->file_type == FileType::FAND8) {
				if ((F->field_type == FieldType::REAL || F->field_type == FieldType::BOOL || F->field_type == FieldType::TEXT)) OldError(35);
				else if ((F->field_type == FieldType::FIXED) && (F->NBytes > 5)) OldError(36);
			}
		}
		else {
			F->Frml = Z;
			if (FTyp != F->frml_type) OldError(12);
		}
		if (Lexem == ';') {
			RdLex();
			if (!(Lexem == '#' || Lexem == 0x1A)) continue;
		}
		break;
	}
}

void FakeRdFDSegment(FileD* FD)
{
	if (Lexem != 0x1A) {
		Accept(';');
	}
	WORD i = FD->ChptPos.i_rec;
	CFile = new FileD(*FD);
	CFile->OrigFD = FD;
	CFile->TxtPosUDLI = 0;
}

void SetLDIndexRoot(/*LinkD* L,*/ std::deque<LinkD*>& L2)
{
	LinkD* l2 = nullptr;
	if (!L2.empty()) l2 = *L2.begin();

	bool computed = false;

	for (auto& L : LinkDRoot) { /* find key with equal beginning */
		if (L == l2) {
			break;
		}
		if (CFile->FF->file_type == FileType::INDEX) {
			for (auto& K : CFile->Keys) {
				KeyFldD* KF = K->KFlds;
				computed = false;
				bool continueWithNextK = false;

				for (auto& arg : L->Args) {
					// cmp CFile key fields with Arg fields
					if (KF == nullptr || arg->FldD != KF->FldD || arg->CompLex != KF->CompLex || arg->Descend != KF->Descend) {
						continueWithNextK = true;
						break;
					}
					if ((arg->FldD->Flg & f_Stored) == 0) {
						computed = true;
					}
					KF = KF->pChain;
				}
				if (continueWithNextK) {
					continue;
				}
				L->IndexRoot = K->IndexRoot;
				break;
			}
		}

		if ((L->MemberRef != 0) && ((L->IndexRoot == 0) || computed)) {
			SetMsgPar(L->RoleName);
			OldError(152);
		}
	}
}

// z ulohy vycte kapilotu 'F', prip. dynamickou definici 'F'
void* RdFileD(std::string FileName, FileType FDTyp, std::string Ext)
{
	std::string JournalFlds = "Upd:A,1;RecNr:F,8.0;User:F,4.0;TimeStamp:D,'DD.MM.YYYY hh:mm:ss'";
	FileD* FD = nullptr;
	FieldDescr* F = nullptr; FieldDescr* F2 = nullptr;
	void* p = nullptr;
	ChkD* C = nullptr;
	std::deque<LinkD*> LDOld;
	size_t n = 0, i = 0; bool isHlp = false;
	std::string Prefix, s;
	LiRoots* li = nullptr;
	CompInpD* ChPos = nullptr;

	ResetCompilePars();
	RdLex();
	issql = EquUpCase(Ext, ".SQL");
	isHlp = EquUpCase(Ext, ".HLP");
	if (IsKeyWord("JOURNALOF")) {
		FD = RdFileName();
		if (Lexem == ';') RdLex();
		SetMsgPar(FileName);
		if (FDTyp != FileType::FAND16) OldError(103);
		if (Lexem != 0x1A) Error(40);
#ifdef FandSQL
		if (issql || rdb_file->typSQLFile) OldError(155);
#endif
		//LDOld = LinkDRoot;

		//	RdbD* rdb = nullptr; void* cr = nullptr; FileD* cf = nullptr; bool b = false; WORD i = 0; int pos = 0;
		//	if (Lexem != 0x1A) Accept(';');	rdb = CRdb; cr = CRecPtr; RdbD* r = rdb_file->ChptPos.rdb;
		//	if ((r == nullptr) || rdb_file->IsDynFile) OldError(106); CRdb = r; i = rdb_file->ChptPos.i_rec;	CFile = CRdb->rdb_file;
		//	CRecPtr = CFile->RecPtr; CFile->ReadRec(i, CRecPtr); pos = loadT(ChptOldTxt); if (pos <= 0) Error(25);
		//	b = RdFDSegment(i, pos); cf = CFile; CRdb = rdb; if (InpRdbPos.i_rec != 0) {	CFile = rdb->rdb_file;
		//		CFile->ReadRec(InpRdbPos.i_rec, CRecPtr); CFile = cf; }
		//	CRecPtr = cr; if (!b) Error(25); CFile->OrigFD = rdb_file; CFile->TxtPosUDLI = 0;

		FakeRdFDSegment(FD);
		//LinkDRoot = LDOld;
		F = CFile->FldD.front();
		CFile->Reset();
		CFile->Name = FileName;
		CFile->IsJournal = true;
		SetHCatTyp(FDTyp);
		if (!PrevCompInp.empty()) {
			CFile->ChptPos = OrigInp()->InpRdbPos;
		}
		SetInpStr(JournalFlds);
		RdLex();
		RdFieldDList(true);
		F2 = (FieldDescr*)LastInChain(CFile->FldD.front());
		while (F != nullptr) {
			if (F->isStored()) {
				CFile->FldD.push_back(F);
				F2->pChain = F;
				F2 = F;
				if (F->field_type == FieldType::TEXT) {
					F->frml_type = 'R';
					F->field_type = FieldType::FIXED;
					F->L = 10;
					F->Flg = F->Flg & ~f_Encryp;
				}
			}
			F = F->pChain;
		}
		F2->pChain = nullptr;
		CompileRecLen();
		ChainLast(FileDRoot, CFile);
		MarkStore(p);
		goto label1;
	}
	if (IsKeyWord("LIKE")) {
		Prefix = FileName;
		FD = RdFileName();
		if (Lexem == '(') {
			RdLex();
			TestIdentif();
			Prefix = LexWord;
			RdLex();
			Accept(')');
		}
		//CallRdFDSegment(rdb_file);
		// misto nacitani objektu ze souboru budeme objekt kopirovat
		FakeRdFDSegment(FD);

		// copy LinkD records too
		for (auto& l : LinkDRoot) {
			// LinkD for OrigFD exists?
			if (l->FromFD == FD) {
				auto copiedLinkD = new LinkD(*l);
				copiedLinkD->FromFD = CFile;
				LinkDRoot.push_front(copiedLinkD);
			}
		}

		CFile->IsHlpFile = false;
		if (!(FDTyp == FileType::FAND16
			|| FDTyp == FileType::INDEX)
			|| !(CFile->FF->file_type == FileType::FAND16 || CFile->FF->file_type == FileType::INDEX)
			) {
			OldError(106);
		}

		for (auto& K : CFile->Keys) {
			if (!K->Alias.empty()) {
				s = K->Alias;
				i = s.find('_');
				if (i != std::string::npos) s = s.substr(i + 1, 255);
				s = Prefix + "_" + s;
				K->Alias = s;
			}
		}
	}
	else {
		CFile = new FileD(FType::FandFile);
	}

	CFile->Name = FileName;
	SetHCatTyp(FDTyp);
	HasTT = false;
	if ((CFile->OrigFD == nullptr) || !(Lexem == 0x1A || Lexem == '#' || Lexem == ']')) {
		RdFieldDList(true);
	}
	GetTFileD(FDTyp);
	LDOld = LinkDRoot;

	// TODO: v originale je to jinak, saha si to na nasl. promenne za PrevCompInp
	// a bere posledni ChainBack
	CFile->ChptPos = InpRdbPos;

	if (isHlp) {
		F = CFile->FldD.front();
		F2 = F->pChain;
		if ((F->field_type != FieldType::ALFANUM) || (F2 == nullptr) || (F2->field_type != FieldType::TEXT) || (F2->pChain != nullptr)) OldError(128);
		CFile->IsHlpFile = true;
	}

	while (true) {
		if ((Lexem == '#') && (ForwChar == 'C')) {
			RdLex();
			RdLex();
			RdFieldDList(false);
			continue;
		}
		if ((Lexem == '#') && (ForwChar == 'K')) {
			RdLex();
			RdKeyD();
			continue;
		}
		break;
	}

	if (issql && !CFile->Keys.empty()) {
		CFile->FF->file_type = FileType::INDEX;
	}
	GetXFileD();
	CompileRecLen();
	SetLDIndexRoot(LDOld);
	if ((CFile->FF->file_type == FileType::INDEX) && CFile->Keys.empty()) Error(107);
	if ((Lexem == '#') && (ForwChar == 'A')) {
		RdLex();
		RdKumul();
	}

	if (FileDRoot == nullptr) {
		FileDRoot = CFile;
		Chpt = FileDRoot;
	}
	else {
		ChainLast(FileDRoot, CFile);
	}

	if (Ext == "$") {
		// compile from text at run time
		CFile->IsDynFile = true;
		CFile->ChptPos.rdb = CRdb;
		MarkStore(p);
		goto label1;
	}
	if (Lexem != 0x1A) {
		CFile->TxtPosUDLI = /*OrigInp()->*/CurrPos - 1;
	}
	if ((Lexem == '#') && (ForwChar == 'U')) {
		// nacteni uzivatelskych pohledu
		// nazev musi byt jedinecny v ramci cele ulohy
		// format: #U NazevPohledu (SeznamPristupovychPrav): DruhEditace;
		RdLex();
		TestUserView();
	}
	MarkStore(p);
	li = new LiRoots();
	if ((Lexem == '#') && (ForwChar == 'D')) { RdLex(); TestDepend(); }
	if ((Lexem == '#') && (ForwChar == 'L')) { RdLex(); RdChkDChain(&li->Chks); }
	if ((Lexem == '#') && (ForwChar == 'I')) { RdLex(); RdImpl(&li->Impls); }
	// TODO: jak toto nahradit?
	//if (PtrRec(InpRdbPos.rdb).Seg == 0/*compiled from pstring*/) {
	//	CFile->LiOfs = 0; ReleaseStore(p);
	//}
	if (Lexem != 0x1A) Error(66);
label1:
	return p; // ma asi vracet HeapPtr
}

void RdKeyD()
{
	FieldDescr* F = nullptr;
	FieldDescr* F2 = nullptr;
	KeyFldD* KF = nullptr;
	KeyFldD* arg = nullptr;
	FileD* FD = nullptr;
	LinkD* L = nullptr;
	XKey* K = nullptr;
	XKey* K1 = nullptr;
	pstring Name;
	WORD N = 0;

	RdLex();
	if (Lexem == '@') {
		if (!CFile->Keys.empty() || CFile->IsParFile) Error(26);
		RdLex();
		if (Lexem == '@') {
			RdLex();
			CFile->IsParFile = true;
		}
		else {
			Name = "";
		label1:
			if (CFile->Keys.empty()) {
				N = 1;
				K = new XKey(CFile);
				CFile->Keys.push_back(K);
		}
			else {
				K1 = CFile->Keys[0];
				N = 2;
				while (K1->Chain != nullptr) {
					K1 = K1->Chain;
					N++;
				}
				K = new XKey(CFile);
				K1->Chain = K;
				CFile->Keys.push_back(K);
			}

			K->Alias = Name;
			K->IntervalTest = false;
			K->Duplic = false;
			if (Lexem == _le) {
				K->IntervalTest = true;
				RdLex();
			}
			else if (Lexem == '*') {
#ifdef FandSQL
				if (CFile->typSQLFile) Error(155);
#endif
				K->Duplic = true;
				RdLex();
			}
			K->IndexRoot = N;
			K->IndexLen = RdKFList(&K->KFlds, CFile);
			if (K->IndexLen > MaxIndexLen) {
				OldError(105);
			}
	}
		goto label6;
}
label2:
	TestIdentif();
	Name = LexWord;
	SkipBlank(false);
	if (ForwChar == '(') {
		RdLex(); RdLex();
		if (Lexem == '@') {
			CheckDuplAlias(Name);
			RdLex();
			Accept(')');
			goto label1;
		}
		RdFileOrAlias(&FD, &K);
		Accept(')');
	}
	else {
		RdFileOrAlias(&FD, &K);
	}

	L = FindLD(Name);
	if (L != nullptr) {
		OldError(26);
	}

	L = new LinkD();
	L->RoleName = Name;
	L->FromFD = CFile;
	L->ToFD = FD;
	L->ToKey = K;
	LinkDRoot.push_front(L);

	if (Lexem == '!') {
		if (CFile->FF->file_type != FileType::INDEX
#ifdef FandSQL
			&& !CFile->typSQLFile
#endif
			) Error(108);
		if (K->Duplic) {
			Error(153);
		}
		RdLex();
		L->MemberRef = 1;
		if (Lexem == '!') {
			RdLex();
			L->MemberRef = 2;
		}
	}
	//Arg = &L->Args;
	KF = K->KFlds;
label3:
	F = RdFldName(CFile);
	if (F->field_type == FieldType::TEXT) OldError(84);
	arg = new KeyFldD();
	arg->FldD = F;
	arg->CompLex = KF->CompLex;
	arg->Descend = KF->Descend;
	L->Args.push_back(arg);

	F2 = KF->FldD;
	if ((F->field_type != F2->field_type) || (F->field_type != FieldType::DATE) && (F->L != F2->L) ||
		(F->field_type == FieldType::FIXED) && (F->M != F2->M)) OldError(12);

	KF = KF->pChain;
	if (KF != nullptr) {
		Accept(',');
		goto label3;
	}
label6:
	if (Lexem == ';') {
		RdLex();
		if (!(Lexem == '#' || Lexem == 0x1A)) goto label2;
	}
}

void CheckDuplAlias(pstring Name)
{
	if (CFile->FF->file_type != FileType::INDEX
#ifdef FandSQL
		&& !CFile->typSQLFile
#endif
		) Error(108);
	LookForK(&Name, CFile);
	FileD* F = FileDRoot;
	while (F != nullptr) {
		LookForK(&Name, F);
		F = (FileD*)F->pChain;
	}
}

void LookForK(pstring* Name, FileD* F)
{
	std::string name = *Name;
	if (EquUpCase(F->Name, name)) Error(26);

	for (auto& K : CFile->Keys) {
		if (EquUpCase(K->Alias, *Name)) Error(26);
	}
}

XKey* RdFileOrAlias1(FileD* F)
{
	if (F->Keys.empty()) return nullptr;

	XKey* k = F->Keys[0];
	std::string lw = LexWord;
	if (!EquUpCase(F->Name, lw))
		while (k != nullptr) {
			std::string lw = LexWord;
			if (EquUpCase(k->Alias, lw)) goto label1;
			k = k->Chain;
		}
label1:
	return k;
}

void RdFileOrAlias(FileD** FD, XKey** KD)
{
	FileD* f = nullptr; RdbD* r = nullptr; XKey* k = nullptr;
	TestIdentif();
	f = CFile;
	k = RdFileOrAlias1(f);
	if (k != nullptr) goto label1;
	r = CRdb;
	while (r != nullptr) {
		f = r->rdb_file;
		while (f != nullptr) {
			k = RdFileOrAlias1(f);
			if (k != nullptr) goto label1;
			f = f->pChain;
		}
		r = r->ChainBack;
	}
	Error(9);
label1:
	if (k == nullptr) Error(24);
	RdLex();
	*FD = f;
	*KD = k;
}

void TestDepend()
{
	FrmlElem* ZBool = nullptr;
	FrmlElem* Z = nullptr;
	FieldDescr* F = nullptr;
	char FTyp;
	void* p = nullptr;
	RdLex();
	MarkStore(p);
label1:
	Accept('(');
	ZBool = RdBool(nullptr);
	Accept(')');
label2:
	F = RdFldName(CFile);
	if ((F->Flg & f_Stored) == 0) OldError(14);
	Accept(_assign);
	Z = RdFrml(FTyp, nullptr);
	if (F->frml_type != FTyp) {
		Error(12);
	}
	if (Lexem == ';') {
		RdLex();
		if (!(Lexem == '#' || Lexem == 0x1A)) {
			if (Lexem == '(') {
				goto label1;
			}
			else {
				goto label2;
			}
		}
	}
	ReleaseStore(&p);
}

void RdImpl(ImplD** IDRoot)
{
	FrmlElem* Z = nullptr;
	FieldDescr* F = nullptr;
	char FTyp = '\0';
	ImplD* ID = nullptr;
	RdLex();
label1:
	F = RdFldName(CFile);
	if ((F->Flg & f_Stored) == 0) OldError(14);
	Accept(_assign);
	Z = RdFrml(FTyp, nullptr);
	if (FTyp != F->frml_type) OldError(12);
	//ID = (ImplD*)GetStore(sizeof(*ID)); 
	ID = new ImplD();
	ID->FldD = F;
	ID->Frml = Z;
	if (*IDRoot == nullptr) *IDRoot = ID;
	else ChainLast(*IDRoot, ID);
	if (Lexem == ';') {
		RdLex();
		if (!(Lexem == '#' || Lexem == 0x1A)) goto label1;
	}
}

void RdKumul()
{
	WORD Low = 0; FileD* CF = nullptr;
	RdLex();

	while (true) {
		AddD* AD = new AddD();
		CFile->Add.push_back(AD);

		if (IsKeyWord("IF")) {
			AD->Bool = RdBool(nullptr);
			AcceptKeyWord("THEN");
			RdRoleField(AD);
			RdImper(AD);
			RdAssign(AD);
		}
		else {
			RdRoleField(AD);
			if (Lexem == '(')
			{
				Low = CurrPos;
				RdLex();
				CF = CFile;
				CFile = AD->File2;
				AD->Chk = RdChkD(Low);
				CFile = CF;
				Accept(')');
			}
			RdImper(AD);
			if ((AD->Chk == nullptr) && (Lexem == _assign)) RdAssign(AD);
			else {
				Accept(_addass);
				AD->Assign = false;
				TestReal(AD->Field->frml_type);
				AD->Frml = RdRealFrml(nullptr);
			}
		}
		if (Lexem == ';') {
			RdLex();
			if (!(Lexem == '#' || Lexem == 0x1A)) continue;
		}
		break;
	}
}

void RdRoleField(AddD* AD)
{
	if (!IsRoleName(true, &AD->File2, &AD->LD)) {
		Error(9);
	}
	Accept('.');
	FieldDescr* F = RdFldName(AD->File2);
	AD->Field = F;
	if ((F->Flg & f_Stored) == 0) OldError(14);
	if (IsKeyArg(F, AD->File2)) OldError(135);
}

void RdImper(AddD* AD)
{
	if (Lexem == '!') {
		RdLex(); AD->Create = 1;
		if (AD->LD != nullptr) {
			KeyFldD* KF = AD->LD->ToKey->KFlds;
			while (KF != nullptr) {
				if ((KF->FldD->Flg & f_Stored) == 0) OldError(148);
				KF = (KeyFldD*)KF->pChain;
			}
		}
		if (Lexem == '!') { RdLex(); AD->Create = 2; }
			}
		}

void RdAssign(AddD* AD)
{
	FrmlElem* Z = nullptr; char FTyp = '\0';
#ifdef FandSQL
	if (AD->File2->typSQLFile) Error(155);
#endif
	Accept(_assign);
	AD->Assign = true;
	AD->Frml = RdFrml(FTyp, nullptr);
	if (FTyp != AD->Field->frml_type) OldError(12);
}

/// smaze CFile->Handle, nastavi typ na FDTyp a ziska CatIRec z GetCatalogIRec() - musi existovat CatFD
void SetHCatTyp(FileType FDTyp)
{
	CFile->FF->Handle = nullptr;
	CFile->FF->file_type = FDTyp;
	CFile->CatIRec = CatFD->GetCatalogIRec(CFile->Name, CFile->FF->file_type == FileType::RDB/*multilevel*/);
#ifdef FandSQL
	typSQLFile = issql;
	SetIsSQLFile();
#endif
}

void GetTFileD(FileType file_type)
{
	if (!HasTT && (CFile->FF->TF == nullptr)) return;
	if (CFile->FF->TF == nullptr) {
		CFile->FF->TF = new FandTFile(CFile->FF);
	}
	CFile->FF->TF->Handle = nullptr;
	if (file_type == FileType::DBF) {
		CFile->FF->TF->Format = FandTFile::DbtFormat;
	}
}

void GetXFileD()
{
	if (CFile->FF->file_type != FileType::INDEX) {
		if (CFile->FF->XF != nullptr) {
			OldError(104);
		}
	}
	else {
		if (CFile->FF->XF == nullptr) {
			CFile->FF->XF = new FandXFile(CFile->FF);
		}
		CFile->FF->XF->Handle = nullptr;
	}
}

//void CallRdFDSegment(FileD* rdb_file)
//{
//	RdbD* rdb = nullptr;
//	void* cr = nullptr;
//	FileD* cf = nullptr;
//	bool b = false;
//	WORD i = 0; int pos = 0;
//	if (Lexem != 0x1A) Accept(';');
//	rdb = CRdb; cr = CRecPtr;
//	RdbD* r = rdb_file->ChptPos.rdb;
//	if ((r == nullptr) || rdb_file->IsDynFile) OldError(106);
//	CRdb = r;
//	i = rdb_file->ChptPos.i_rec;
//	CFile = CRdb->rdb_file;
//	CRecPtr = CFile->RecPtr;
//	CFile->ReadRec(i, CRecPtr);
//	pos = loadT(ChptOldTxt);
//	if (pos <= 0) Error(25);
//	b = RdFDSegment(i, pos);
//	cf = CFile; CRdb = rdb;
//	if (InpRdbPos.i_rec != 0) {
//		CFile = rdb->rdb_file;
//		CFile->ReadRec(InpRdbPos.i_rec, CRecPtr);
//		CFile = cf;
//	}
//	CRecPtr = cr;
//	if (!b) Error(25);
//	CFile->OrigFD = rdb_file;
//	CFile->TxtPosUDLI = 0;
//}

CompInpD* OrigInp()
{
	if (PrevCompInp.empty()) {
		return nullptr;
	}
	else {
		return &PrevCompInp.front();
	}
}
