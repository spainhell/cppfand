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
	g_compiler->Accept(':');
	if ((Lexem != _identifier) || (LexWord.length() > 1)) g_compiler->Error(10);

	FieldType Typ = FieldDescr::GetFieldType((char)LexWord[1]);

	g_compiler->RdLex();
	FrmlTyp = 'S';
	M = 0;
	if (Typ == FieldType::NUMERIC || Typ == FieldType::FIXED) {
		g_compiler->Accept(',');
		L = g_compiler->RdInteger();
	}
	switch (Typ) {
	case FieldType::NUMERIC: {
		NBytes = (L + 1) / 2;
		if (CurrChar == 'L') {
			g_compiler->RdLex();
			M = LeftJust;
		}
		break;
	}
	case FieldType::FIXED: {
		if (Lexem == ',') { Flg += f_Comma; g_compiler->RdLex(); }
		else g_compiler->Accept('.');
		M = g_compiler->RdInteger();
		if ((M > 15) || (L + M > 18)) g_compiler->OldError(3);
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
		g_compiler->Accept(',');
		if (!Stored || (Lexem != _quotedstr)) {
			L = g_compiler->RdInteger();
			if (L > 255) g_compiler->Error(3);
			if (CurrChar == 'R') g_compiler->RdLex();
			else M = LeftJust;
		}
		else {
			WORD n1 = 0;
			WORD n = 0;
			sstr = LexWord;
			g_compiler->Accept(_quotedstr);
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
			g_compiler->Error(171);
		}
		NBytes = L;
		if (Stored && (Lexem == '!')) {
			g_compiler->RdLex();
			Flg += f_Encryp;
		}
		break;
	}
	case FieldType::DATE: {
		sstr = "";
		if (Lexem == ',') {
			g_compiler->RdLex();
			sstr = LexWord;
			g_compiler->Accept(_quotedstr);
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
			g_compiler->RdLex();
			L = g_compiler->RdInteger() + 2;
		}
		else {
			L = 1;
		}
		NBytes = sizeof(int);
		HasTT = true;
		if (Stored && (Lexem == '!')) {
			g_compiler->RdLex();
			Flg += f_Encryp;
		}
		break;
	}
	default: {
		g_compiler->OldError(10);
		break;
	}
	}
	if (NBytes == 0) g_compiler->OldError(113);
	if ((L > TxtCols - 1) && (Typ != FieldType::ALFANUM)) g_compiler->OldError(3);
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
	C->Bool = g_compiler->RdBool(nullptr);
	WORD Upper = OldErrPos;
	if (Lexem == '?') {
		g_compiler->RdLex();
		C->Warning = true;
	}
	if (Lexem == ':') {
		g_compiler->RdLex();
		C->TxtZ = g_compiler->RdStrFrml(nullptr);
	}
	else {
		WORD N = Upper - Low;
		FrmlElem* Z = new FrmlElemString(_const, 0);
		C->TxtZ = Z;
		auto iZ = (FrmlElemString*)Z;
		iZ->S = std::string((char*)&InpArrPtr[Low], N);
	}
	if (Lexem == ',') {
		g_compiler->RdLex();
		C->HelpName = g_compiler->RdHelpName();
	}
	return result;
}

void RdChkDChain(ChkD** CRoot)
{
	WORD Low = 0;
	g_compiler->SkipBlank(false);
	Low = CurrPos;
	g_compiler->RdLex();
label1:
	if (*CRoot == nullptr) *CRoot = RdChkD(Low);
	else ChainLast(*CRoot, RdChkD(Low));
	if (Lexem == ';') {
		g_compiler->SkipBlank(false); Low = CurrPos; g_compiler->RdLex();
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
	g_compiler->SetInpTTxtPos(FD);
	g_compiler->RdLex();
	while (!(ForwChar == 'L' || ForwChar == 0x1A)) {
		do { g_compiler->RdLex(); } while (!(Lexem == 0x1A || Lexem == '#'));
	}
	if (Lexem == 0x1A) return;
	g_compiler->RdLex();
	FileD* cf = CFile;
	CFile = FD;
	RdChkDChain(&C);
	CFile = cf;
}

void RdBegViewDcl(EditOpt* EO)
{
	FieldListEl* fl = nullptr;
	if (Lexem == _identifier || Lexem == '[') {
		g_compiler->RdChptName('E', &EO->FormPos, true);
		return;
	}
	bool neg = false;
	bool all = false;
	std::vector<FieldDescr*> fl1;
	EO->UserSelFlds = false;
	if (Lexem == '^') {
		g_compiler->RdLex();
		neg = true;
	}
	g_compiler->Accept('(');
	if (Lexem == _identifier) {
		g_compiler->RdFldList(fl1);
	}
	else {
		neg = true;
	}

	while (true) {
		switch (Lexem) {
		case '!': {
			if (neg) {
				g_compiler->RdLex();
				all = true;
				continue;
			}
			break;
		}
		case '?': {
			g_compiler->RdLex();
			EO->UserSelFlds = true;
			continue;
			break;
		}
		}
		break;
	}

	g_compiler->Accept(')');
	if (!neg) {
		EO->Flds = fl1;
		return;
	}
	EO->Flds.clear();
	for (auto& f : g_compiler->processing_F->FldD) {
		if (((f->isStored()) || all) && !FieldInList(f, fl1)) {
			EO->Flds.push_back(f);
		}
	}
	if (EO->Flds.empty()) {
		g_compiler->OldError(117);
	}
}

void RdByteList(pstring* s)
{
	g_compiler->Accept('(');
	short l = 0;

	while (true) {
		short i1 = g_compiler->RdInteger();
		short i2 = i1;
		if (i1 < 0) g_compiler->OldError(133);
		if (Lexem == _subrange) {
			g_compiler->RdLex();
			i2 = g_compiler->RdInteger();
			if (i2 < i1) g_compiler->OldError(133);
		}
		if ((i2 > 255) || (l + i2 - i1 >= 255)) g_compiler->OldError(133);
		for (short i = i1; i <= i2; i++) {
			l++;
			(*s)[l] = (char)i;
		}
		if (Lexem == ',') {
			g_compiler->RdLex();
			continue;
		}
		break;
	}

	(*s)[0] = (char)l;
	g_compiler->Accept(')');
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
		g_compiler->SetInpTTxtPos(file_d);
		g_compiler->RdLex();
		if ((Lexem != '#') || (ForwChar != 'U')) {
			file_d = file_d->OrigFD;
			if ((file_d != nullptr) && !found) continue;
			break;
		}
		g_compiler->RdLex(); // #
		g_compiler->RdLex(); // U
		while (true) {
			std::string sLexWord = LexWord;
			if (EquUpCase(ViewName, sLexWord)) found = true;
			EO->ViewName = LexWord;
			g_compiler->RdLex(); /*'('*/
			do {
				g_compiler->RdLex();
			} while (!(Lexem == ')' || Lexem == 0x1A));
			g_compiler->RdLex();
			g_compiler->RdLex();/*"):"*/
			K = g_compiler->RdViewKey(file_d);
			if (K != nullptr) {
				g_compiler->RdLex();/*','*/
				EO->ViewKey = K;
			}
			RdBegViewDcl(EO);
			while (Lexem == ',') {
				FVA = FileVarsAllowed;
				FileVarsAllowed = false;
				if (!RdViewOpt(EO, file_d)) g_compiler->Error(44);
				FileVarsAllowed = FVA;
			}
			if (!found && (Lexem == ';')) {
				g_compiler->RdLex();
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

void TestUserView(FileD* file_d)
{
	EditOpt EO = EditOpt();
	EO.UserSelFlds = true;
	g_compiler->RdLex();
	while (true) {
		g_compiler->TestIdentif();
		TestDupl(file_d);
		FileD* FD = FileDRoot;
		while (FD != nullptr) {
			TestDupl(FD);
			FD = FD->pChain;
		}
		StringListEl* S = new StringListEl();
		S->S = LexWord;
		if (file_d->ViewNames == nullptr) {
			file_d->ViewNames = S;
		}
		else {
			ChainLast(file_d->ViewNames, S);
		}
		g_compiler->RdLex();
		RdByteListInStore();
		g_compiler->Accept(':');
		// GetEditOpt(); // vytvori objekt EditOpt
		XKey* K = g_compiler->RdViewKey(file_d); // nacteni klice, podle ktereho budou polozky setrideny
		if (K != nullptr) {
			g_compiler->Accept(',');
			EO.ViewKey = K;
		}
		RdBegViewDcl(&EO);
		while (Lexem == ',') {
			if (!RdViewOpt(&EO, file_d)) g_compiler->Error(44);
		}
		if (Lexem == ';') {
			g_compiler->RdLex();
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
		if (EquUpCase(S->S, tmp)) g_compiler->Error(26);
		S = (StringList)S->pChain;
	}
}

void RdFieldDList(FileD* file_d, bool stored)
{
	FieldDescr* F = nullptr;
	char FTyp = 0;
	FrmlElem* Z = nullptr;

	while (true) {
		g_compiler->TestIdentif();
		std::string name = LexWord;
		F = g_compiler->FindFldName(file_d);
		if (F != nullptr) g_compiler->Error(26);
		g_compiler->RdLex();
		if (!stored) {
			g_compiler->Accept(_assign);
			Z = g_compiler->RdFrml(FTyp, nullptr);
		}
		F = RdFieldDescr(name, stored);
		if ((file_d->FF->file_type == FileType::DBF) && stored && (F->field_type == FieldType::REAL || F->field_type == FieldType::NUMERIC)) {
			g_compiler->OldError(86);
		}

		file_d->FldD.push_back(F);
		ChainLast(file_d->FldD.front(), F);

		if (stored) {
			if (file_d->FF->file_type == FileType::FAND8) {
				if ((F->field_type == FieldType::REAL || F->field_type == FieldType::BOOL || F->field_type == FieldType::TEXT)) g_compiler->OldError(35);
				else if ((F->field_type == FieldType::FIXED) && (F->NBytes > 5)) g_compiler->OldError(36);
			}
		}
		else {
			F->Frml = Z;
			if (FTyp != F->frml_type) g_compiler->OldError(12);
		}
		if (Lexem == ';') {
			g_compiler->RdLex();
			if (!(Lexem == '#' || Lexem == 0x1A)) continue;
		}
		break;
	}
}

FileD* FakeRdFDSegment(FileD* FD)
{
	if (Lexem != 0x1A) {
		g_compiler->Accept(';');
	}
	WORD i = FD->ChptPos.i_rec;
	FileD* newFile = new FileD(*FD);
	newFile->OrigFD = FD;
	newFile->TxtPosUDLI = 0;

	return newFile;
}

void SetLDIndexRoot(FileD* file_d, /*LinkD* L,*/ std::deque<LinkD*>& L2)
{
	LinkD* l2 = nullptr;
	if (!L2.empty()) l2 = *L2.begin();

	bool computed = false;

	for (auto& L : LinkDRoot) { /* find key with equal beginning */
		if (L == l2) {
			break;
		}
		if (file_d->FF->file_type == FileType::INDEX) {
			for (auto& K : file_d->Keys) {
				KeyFldD* KF = K->KFlds;
				computed = false;
				bool continueWithNextK = false;

				for (auto& arg : L->Args) {
					// cmp file_d key fields with Arg fields
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
			g_compiler->OldError(152);
		}
	}
}

// z ulohy vycte kapilotu 'F', prip. dynamickou definici 'F'
// vraci ukazatel na FileD, protoze se muze v metode vytvorit novy objekt!!!
FileD* RdFileD(std::string FileName, FileType FDTyp, std::string Ext)
{
	std::string JournalFlds = "Upd:A,1;RecNr:F,8.0;User:F,4.0;TimeStamp:D,'DD.MM.YYYY hh:mm:ss'";
	FieldDescr* F = nullptr; FieldDescr* F2 = nullptr;
	void* p = nullptr;
	ChkD* C = nullptr;
	std::deque<LinkD*> LDOld;
	size_t n = 0, i = 0; bool isHlp = false;
	std::string Prefix, s;
	LiRoots* li = nullptr;
	CompInpD* ChPos = nullptr;
	FileD* FD = nullptr;
	FileD* file_d = nullptr;

	ResetCompilePars();
	g_compiler->RdLex();
	issql = EquUpCase(Ext, ".SQL");
	isHlp = EquUpCase(Ext, ".HLP");

	if (g_compiler->IsKeyWord("JOURNALOF")) {
		FD = g_compiler->RdFileName();
		if (Lexem == ';') g_compiler->RdLex();
		SetMsgPar(FileName);
		if (FDTyp != FileType::FAND16) g_compiler->OldError(103);
		if (Lexem != 0x1A) g_compiler->Error(40);
#ifdef FandSQL
		if (issql || rdb_file->typSQLFile) OldError(155);
#endif
		file_d = FakeRdFDSegment(FD);
		F = file_d->FldD.front();
		file_d->Reset();
		file_d->Name = FileName;
		file_d->IsJournal = true;
		SetHCatTyp(file_d, FDTyp);
		if (!PrevCompInp.empty()) {
			file_d->ChptPos = OrigInp()->InpRdbPos;
		}
		g_compiler->SetInpStr(JournalFlds);
		g_compiler->RdLex();
		RdFieldDList(file_d, true);
		F2 = (FieldDescr*)LastInChain(file_d->FldD.front());
		while (F != nullptr) {
			if (F->isStored()) {
				file_d->FldD.push_back(F);
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
		g_compiler->CompileRecLen(file_d);
		ChainLast(FileDRoot, file_d);
		MarkStore(p);
		goto label1;
	}
	if (g_compiler->IsKeyWord("LIKE")) {
		Prefix = FileName;
		FD = g_compiler->RdFileName();
		if (Lexem == '(') {
			g_compiler->RdLex();
			g_compiler->TestIdentif();
			Prefix = LexWord;
			g_compiler->RdLex();
			g_compiler->Accept(')');
		}
		//CallRdFDSegment(rdb_file);
		// misto nacitani objektu ze souboru budeme objekt kopirovat
		file_d = FakeRdFDSegment(FD);

		// copy LinkD records too
		for (auto& l : LinkDRoot) {
			// LinkD for OrigFD exists?
			if (l->FromFD == FD) {
				auto copiedLinkD = new LinkD(*l);
				copiedLinkD->FromFD = file_d;
				LinkDRoot.push_front(copiedLinkD);
			}
		}

		file_d->IsHlpFile = false;
		if (!(FDTyp == FileType::FAND16
			|| FDTyp == FileType::INDEX)
			|| !(file_d->FF->file_type == FileType::FAND16 || file_d->FF->file_type == FileType::INDEX)
			) {
			g_compiler->OldError(106);
		}

		for (auto& K : file_d->Keys) {
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
		file_d = new FileD(FType::FandFile);
	}

	file_d->Name = FileName;
	g_compiler->processing_F = file_d;
	SetHCatTyp(file_d, FDTyp);
	HasTT = false;
	if ((file_d->OrigFD == nullptr) || !(Lexem == 0x1A || Lexem == '#' || Lexem == ']')) {
		RdFieldDList(file_d, true);
	}
	GetTFileD(file_d, FDTyp);
	LDOld = LinkDRoot;

	// TODO: v originale je to jinak, saha si to na nasl. promenne za PrevCompInp
	// a bere posledni ChainBack
	file_d->ChptPos = InpRdbPos;

	if (isHlp) {
		F = file_d->FldD.front();
		F2 = F->pChain;
		if ((F->field_type != FieldType::ALFANUM) || (F2 == nullptr) || (F2->field_type != FieldType::TEXT) || (F2->pChain != nullptr)) g_compiler->OldError(128);
		file_d->IsHlpFile = true;
	}

	while (true) {
		if ((Lexem == '#') && (ForwChar == 'C')) {
			g_compiler->RdLex();
			g_compiler->RdLex();
			RdFieldDList(file_d, false);
			continue;
		}
		if ((Lexem == '#') && (ForwChar == 'K')) {
			g_compiler->RdLex();
			RdKeyD(file_d);
			continue;
		}
		break;
	}

	if (issql && !file_d->Keys.empty()) {
		file_d->FF->file_type = FileType::INDEX;
	}
	GetXFileD(file_d);
	g_compiler->CompileRecLen(file_d);
	SetLDIndexRoot(file_d, LDOld);
	if ((file_d->FF->file_type == FileType::INDEX) && file_d->Keys.empty()) g_compiler->Error(107);
	if ((Lexem == '#') && (ForwChar == 'A')) {
		g_compiler->RdLex();
		RdKumul();
	}

	if (FileDRoot == nullptr) {
		FileDRoot = file_d;
		Chpt = FileDRoot;
	}
	else {
		ChainLast(FileDRoot, file_d);
	}

	if (Ext == "$") {
		// compile from text at run time
		file_d->IsDynFile = true;
		file_d->ChptPos.rdb = CRdb;
		MarkStore(p);
		goto label1;
	}
	if (Lexem != 0x1A) {
		file_d->TxtPosUDLI = /*OrigInp()->*/CurrPos - 1;
	}
	if ((Lexem == '#') && (ForwChar == 'U')) {
		// nacteni uzivatelskych pohledu
		// nazev musi byt jedinecny v ramci cele ulohy
		// format: #U NazevPohledu (SeznamPristupovychPrav): DruhEditace;
		g_compiler->RdLex();
		TestUserView(file_d);
	}
	MarkStore(p);
	li = new LiRoots();
	if ((Lexem == '#') && (ForwChar == 'D')) { g_compiler->RdLex(); TestDepend(); }
	if ((Lexem == '#') && (ForwChar == 'L')) { g_compiler->RdLex(); RdChkDChain(&li->Chks); }
	if ((Lexem == '#') && (ForwChar == 'I')) { g_compiler->RdLex(); RdImpl(file_d, &li->Impls); }
	// TODO: jak toto nahradit?
	//if (PtrRec(InpRdbPos.rdb).Seg == 0/*compiled from pstring*/) {
	//	CFile->LiOfs = 0; ReleaseStore(p);
	//}
	if (Lexem != 0x1A) g_compiler->Error(66);
label1:
	g_compiler->processing_F = nullptr;
	return file_d;
}

void RdKeyD(FileD* file_d)
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

	g_compiler->RdLex();
	if (Lexem == '@') {
		if (!file_d->Keys.empty() || file_d->IsParFile) g_compiler->Error(26);
		g_compiler->RdLex();
		if (Lexem == '@') {
			g_compiler->RdLex();
			file_d->IsParFile = true;
		}
		else {
			Name = "";
		label1:
			if (file_d->Keys.empty()) {
				N = 1;
				K = new XKey(file_d);
				file_d->Keys.push_back(K);
			}
			else {
				K1 = file_d->Keys[0];
				N = 2;
				while (K1->Chain != nullptr) {
					K1 = K1->Chain;
					N++;
				}
				K = new XKey(file_d);
				K1->Chain = K;
				file_d->Keys.push_back(K);
			}

			K->Alias = Name;
			K->IntervalTest = false;
			K->Duplic = false;
			if (Lexem == _le) {
				K->IntervalTest = true;
				g_compiler->RdLex();
			}
			else if (Lexem == '*') {
#ifdef FandSQL
				if (file_d->typSQLFile) Error(155);
#endif
				K->Duplic = true;
				g_compiler->RdLex();
			}
			K->IndexRoot = N;
			K->IndexLen = g_compiler->RdKFList(&K->KFlds, file_d);
			if (K->IndexLen > MaxIndexLen) {
				g_compiler->OldError(105);
			}
		}
		goto label6;
	}
label2:
	g_compiler->TestIdentif();
	Name = LexWord;
	g_compiler->SkipBlank(false);
	if (ForwChar == '(') {
		g_compiler->RdLex();
		g_compiler->RdLex();
		if (Lexem == '@') {
			CheckDuplAlias(file_d, Name);
			g_compiler->RdLex();
			g_compiler->Accept(')');
			goto label1;
		}
		RdFileOrAlias(file_d, &FD, &K);
		g_compiler->Accept(')');
	}
	else {
		RdFileOrAlias(file_d, &FD, &K);
	}

	L = g_compiler->FindLD(file_d, Name);
	if (L != nullptr) {
		g_compiler->OldError(26);
	}

	L = new LinkD();
	L->RoleName = Name;
	L->FromFD = file_d;
	L->ToFD = FD;
	L->ToKey = K;
	LinkDRoot.push_front(L);

	if (Lexem == '!') {
		if (file_d->FF->file_type != FileType::INDEX
#ifdef FandSQL
			&& !file_d->typSQLFile
#endif
			) g_compiler->Error(108);
		if (K->Duplic) {
			g_compiler->Error(153);
		}
		g_compiler->RdLex();
		L->MemberRef = 1;
		if (Lexem == '!') {
			g_compiler->RdLex();
			L->MemberRef = 2;
		}
	}
	//Arg = &L->Args;
	KF = K->KFlds;
label3:
	F = g_compiler->RdFldName(file_d);
	if (F->field_type == FieldType::TEXT) g_compiler->OldError(84);
	arg = new KeyFldD();
	arg->FldD = F;
	arg->CompLex = KF->CompLex;
	arg->Descend = KF->Descend;
	L->Args.push_back(arg);

	F2 = KF->FldD;
	if ((F->field_type != F2->field_type) || (F->field_type != FieldType::DATE) && (F->L != F2->L) ||
		(F->field_type == FieldType::FIXED) && (F->M != F2->M)) g_compiler->OldError(12);

	KF = KF->pChain;
	if (KF != nullptr) {
		g_compiler->Accept(',');
		goto label3;
	}
label6:
	if (Lexem == ';') {
		g_compiler->RdLex();
		if (!(Lexem == '#' || Lexem == 0x1A)) goto label2;
	}
}

void CheckDuplAlias(FileD* file_d, pstring name)
{
	if (file_d->FF->file_type != FileType::INDEX
#ifdef FandSQL
		&& !file_d->typSQLFile
#endif
		) g_compiler->Error(108);
	LookForK(file_d, &name, file_d);
	FileD* F = FileDRoot;
	while (F != nullptr) {
		LookForK(file_d, &name, F);
		F = F->pChain;
	}
}

void LookForK(FileD* file_d, pstring* Name, FileD* F)
{
	std::string name = *Name;
	if (EquUpCase(F->Name, name)) g_compiler->Error(26);

	for (auto& K : file_d->Keys) {
		if (EquUpCase(K->Alias, *Name)) g_compiler->Error(26);
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

void RdFileOrAlias(FileD* file_d, FileD** FD, XKey** KD)
{
	RdbD* r = nullptr;
	XKey* k = nullptr;
	g_compiler->TestIdentif();
	k = RdFileOrAlias1(file_d);
	if (k != nullptr) goto label1;
	r = CRdb;
	while (r != nullptr) {
		file_d = r->rdb_file;
		while (file_d != nullptr) {
			k = RdFileOrAlias1(file_d);
			if (k != nullptr) goto label1;
			file_d = file_d->pChain;
		}
		r = r->ChainBack;
	}
	g_compiler->Error(9);
label1:
	if (k == nullptr) g_compiler->Error(24);
	g_compiler->RdLex();
	*FD = file_d;
	*KD = k;
}

void TestDepend()
{
	FrmlElem* ZBool = nullptr;
	FrmlElem* Z = nullptr;
	FieldDescr* F = nullptr;
	char FTyp;
	void* p = nullptr;
	g_compiler->RdLex();
	MarkStore(p);
label1:
	g_compiler->Accept('(');
	ZBool = g_compiler->RdBool(nullptr);
	g_compiler->Accept(')');
label2:
	F = g_compiler->RdFldName(g_compiler->processing_F);
	if ((F->Flg & f_Stored) == 0) g_compiler->OldError(14);
	g_compiler->Accept(_assign);
	Z = g_compiler->RdFrml(FTyp, nullptr);
	if (F->frml_type != FTyp) {
		g_compiler->Error(12);
	}
	if (Lexem == ';') {
		g_compiler->RdLex();
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

void RdImpl(FileD* file_d, ImplD** IDRoot)
{
	FrmlElem* Z = nullptr;
	FieldDescr* F = nullptr;
	char FTyp = '\0';
	ImplD* ID = nullptr;
	g_compiler->RdLex();
label1:
	F = g_compiler->RdFldName(file_d);
	if ((F->Flg & f_Stored) == 0) g_compiler->OldError(14);
	g_compiler->Accept(_assign);
	Z = g_compiler->RdFrml(FTyp, nullptr);
	if (FTyp != F->frml_type) g_compiler->OldError(12);
	ID = new ImplD();
	ID->FldD = F;
	ID->Frml = Z;
	if (*IDRoot == nullptr) *IDRoot = ID;
	else ChainLast(*IDRoot, ID);
	if (Lexem == ';') {
		g_compiler->RdLex();
		if (!(Lexem == '#' || Lexem == 0x1A)) goto label1;
	}
}

void RdKumul()
{
	WORD Low = 0;
	g_compiler->RdLex();

	while (true) {
		AddD* AD = new AddD();
		g_compiler->processing_F->Add.push_back(AD);

		if (g_compiler->IsKeyWord("IF")) {
			AD->Bool = g_compiler->RdBool(nullptr);
			g_compiler->AcceptKeyWord("THEN");
			RdRoleField(AD);
			RdImper(AD);
			RdAssign(AD);
		}
		else {
			RdRoleField(AD);
			if (Lexem == '(')
			{
				Low = CurrPos;
				g_compiler->RdLex();
				FileD* previous = g_compiler->processing_F;
				g_compiler->processing_F = AD->File2;
				AD->Chk = RdChkD(Low);
				g_compiler->processing_F = previous;
				g_compiler->Accept(')');
			}
			RdImper(AD);
			if ((AD->Chk == nullptr) && (Lexem == _assign)) RdAssign(AD);
			else {
				g_compiler->Accept(_addass);
				AD->Assign = false;
				g_compiler->TestReal(AD->Field->frml_type);
				AD->Frml = g_compiler->RdRealFrml(nullptr);
			}
		}
		if (Lexem == ';') {
			g_compiler->RdLex();
			if (!(Lexem == '#' || Lexem == 0x1A)) continue;
		}
		break;
	}
}

void RdRoleField(AddD* AD)
{
	if (!g_compiler->IsRoleName(true, &AD->File2, &AD->LD)) {
		g_compiler->Error(9);
	}
	g_compiler->Accept('.');
	FieldDescr* F = g_compiler->RdFldName(AD->File2);
	AD->Field = F;
	if ((F->Flg & f_Stored) == 0) g_compiler->OldError(14);
	if (g_compiler->IsKeyArg(F, AD->File2)) g_compiler->OldError(135);
}

void RdImper(AddD* AD)
{
	if (Lexem == '!') {
		g_compiler->RdLex(); AD->Create = 1;
		if (AD->LD != nullptr) {
			KeyFldD* KF = AD->LD->ToKey->KFlds;
			while (KF != nullptr) {
				if ((KF->FldD->Flg & f_Stored) == 0) g_compiler->OldError(148);
				KF = (KeyFldD*)KF->pChain;
			}
		}
		if (Lexem == '!') { g_compiler->RdLex(); AD->Create = 2; }
		if (Lexem == '!') { g_compiler->RdLex(); AD->Create = 2; }
	}
}

void RdAssign(AddD* AD)
{
	FrmlElem* Z = nullptr; char FTyp = '\0';
#ifdef FandSQL
	if (AD->File2->typSQLFile) Error(155);
#endif
	g_compiler->Accept(_assign);
	AD->Assign = true;
	AD->Frml = g_compiler->RdFrml(FTyp, nullptr);
	if (FTyp != AD->Field->frml_type) g_compiler->OldError(12);
}

/// smaze Handle, nastavi typ na FDTyp a ziska CatIRec z GetCatalogIRec() - musi existovat CatFD
void SetHCatTyp(FileD* file_d, FileType FDTyp)
{
	file_d->FF->Handle = nullptr;
	file_d->FF->file_type = FDTyp;
	file_d->CatIRec = CatFD->GetCatalogIRec(file_d->Name, file_d->FF->file_type == FileType::RDB/*multilevel*/);
#ifdef FandSQL
	typSQLFile = issql;
	SetIsSQLFile();
#endif
}

void GetTFileD(FileD* file_d, FileType file_type)
{
	if (!HasTT && (file_d->FF->TF == nullptr)) return;
	if (file_d->FF->TF == nullptr) {
		file_d->FF->TF = new FandTFile(file_d->FF);
	}
	file_d->FF->TF->Handle = nullptr;
	if (file_type == FileType::DBF) {
		file_d->FF->TF->Format = FandTFile::DbtFormat;
	}
}

void GetXFileD(FileD* file_d)
{
	if (file_d->FF->file_type != FileType::INDEX) {
		if (file_d->FF->XF != nullptr) {
			g_compiler->OldError(104);
		}
	}
	else {
		if (file_d->FF->XF == nullptr) {
			file_d->FF->XF = new FandXFile(file_d->FF);
		}
		file_d->FF->XF->Handle = nullptr;
	}
}

CompInpD* OrigInp()
{
	if (PrevCompInp.empty()) {
		return nullptr;
	}
	else {
		return &PrevCompInp.front();
	}
}
