#include "rdfildcl.h"

#include "../Common/compare.h"
#include "../fandio/FandXFile.h"
#include "Compiler.h"
#include "../fandio/FieldDescr.h"
#include "../Common/FileD.h"
#include "GlobalVariables.h"
#include "../Common/LinkD.h"
#include "LogicControl.h"
#include "../fandio/KeyFldD.h"
#include "rdproc.h"
#include "runfrml.h"

bool HasTT;
bool isSql;

FieldDescr* RdFieldDescr(std::string name, bool Stored)
{
	const uint8_t TabF[19] = { 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8 };

	WORD L = 0, M = 0, NBytes = 0;
	uint8_t Flg = 0;
	char FrmlTyp = 0, c = 0;
	std::string sstr;

	FieldDescr* F = new FieldDescr();
	F->Name = name;
	if (Stored) {
		Flg = f_Stored;
	}
	else {
		Flg = 0;
	}
	gc->Accept(':');
	if ((gc->Lexem != _identifier) || (gc->LexWord.length() > 1)) {
		gc->Error(10);
	}

	FieldType Typ = FieldDescr::GetFieldType((char)gc->LexWord[1]);

	gc->RdLex();
	FrmlTyp = 'S';
	M = 0;
	if (Typ == FieldType::NUMERIC || Typ == FieldType::FIXED) {
		gc->Accept(',');
		L = gc->RdInteger();
	}
	switch (Typ) {
	case FieldType::NUMERIC: {
		NBytes = (L + 1) / 2;
		if (gc->CurrChar == 'L') {
			gc->RdLex();
			M = LeftJust;
		}
		break;
	}
	case FieldType::FIXED: {
		if (gc->Lexem == ',') {
			Flg += f_Comma;
			gc->RdLex();
		}
		else {
			gc->Accept('.');
		}

		M = gc->RdInteger();
		if ((M > 15) || (L + M > 18)) {
			gc->OldError(3);
		}
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
		gc->Accept(',');
		if (!Stored || (gc->Lexem != _quotedstr)) {
			L = gc->RdInteger();
			if (L > 255) gc->Error(3);
			if (gc->CurrChar == 'R') gc->RdLex();
			else M = LeftJust;
		}
		else {
			WORD n1 = 0;
			WORD n = 0;
			sstr = gc->LexWord;
			gc->Accept(_quotedstr);
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
			gc->Error(171);
		}
		NBytes = L;
		if (Stored && (gc->Lexem == '!')) {
			gc->RdLex();
			Flg += f_Encryp;
		}
		break;
	}
	case FieldType::DATE: {
		sstr = "";
		if (gc->Lexem == ',') {
			gc->RdLex();
			sstr = gc->LexWord;
			gc->Accept(_quotedstr);
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
		if (gc->Lexem == ',') {
			gc->RdLex();
			L = gc->RdInteger() + 2;
		}
		else {
			L = 1;
		}
		NBytes = sizeof(int);
		HasTT = true;
		if (Stored && (gc->Lexem == '!')) {
			gc->RdLex();
			Flg += f_Encryp;
		}
		break;
	}
	default: {
		gc->OldError(10);
		break;
	}
	}
	if (NBytes == 0) gc->OldError(113);
	if ((L > TxtCols - 1) && (Typ != FieldType::ALFANUM)) gc->OldError(3);
	F->field_type = Typ;
	F->frml_type = FrmlTyp;
	F->L = L;
	F->M = M;
	F->NBytes = NBytes;
	F->Flg = Flg;
	F->Mask = sstr;
	return F;
}

LogicControl* ReadLogicControl(WORD Low)
{
	LogicControl* C = new LogicControl();
	LogicControl* result = C;
	C->Bool = gc->RdBool(nullptr);
	size_t Upper = gc->input_old_err_pos;
	if (gc->Lexem == '?') {
		gc->RdLex();
		C->Warning = true;
	}
	if (gc->Lexem == ':') {
		gc->RdLex();
		C->TxtZ = gc->RdStrFrml(nullptr);
	}
	else {
		size_t N = Upper - Low;
		FrmlElem* Z = new FrmlElemString(_const, 0);
		C->TxtZ = Z;
		auto iZ = (FrmlElemString*)Z;
		iZ->S = gc->input_string.substr(Low, N); //std::string((char*)&InpArrPtr[Low], N);
	}
	if (gc->Lexem == ',') {
		gc->RdLex();
		C->HelpName = gc->RdHelpName();
	}
	return result;
}

void RdChkDChain(std::vector<LogicControl*>& C)
{
	gc->SkipBlank(false);
	uint16_t low = gc->input_pos;
	gc->RdLex();

	while (true) {
		LogicControl* check = ReadLogicControl(low);
		C.push_back(check);

		//if (*CRoot == nullptr) {
		//	*CRoot = ReadLogicControl(low);
		//}
		//else {
		//	ChainLast(*CRoot, ReadLogicControl(low));
		//}

		if (gc->Lexem == ';') {
			gc->SkipBlank(false);
			low = gc->input_pos;
			gc->RdLex();
			if (!(gc->Lexem == '#' || gc->Lexem == 0x1A)) {
				continue;
			}
		}
		break;
	}
}

void RdChkDsFromPos(FileD* FD, std::vector<LogicControl*>& C)
{
	if (FD->OrigFD != nullptr) {
		// this v_files was created as 'LIKE'
		RdChkDsFromPos(FD->OrigFD, C);
	}
	if (FD->ChptPos.rdb == nullptr) return;
	if (FD->TxtPosUDLI == 0) return;
	gc->ResetCompilePars();
	gc->SetInpTTxtPos(FD);
	gc->RdLex();
	while (!(gc->ForwChar == 'L' || gc->ForwChar == 0x1A)) {
		do {
			gc->RdLex();
		} while (!(gc->Lexem == 0x1A || gc->Lexem == '#'));
	}
	if (gc->Lexem == 0x1A) return;
	gc->RdLex();
	FileD* cf = CFile;
	CFile = FD;
	RdChkDChain(C);
	CFile = cf;
}

void RdBegViewDcl(EditOpt* EO)
{
	//FieldListEl* fl = nullptr;
	if (gc->Lexem == _identifier || gc->Lexem == '[') {
		gc->RdChptName('E', &EO->FormPos, true);
		return;
	}
	bool neg = false;
	bool all = false;
	std::vector<FieldDescr*> fl1;
	EO->UserSelFlds = false;
	if (gc->Lexem == '^') {
		gc->RdLex();
		neg = true;
	}
	gc->Accept('(');
	if (gc->Lexem == _identifier) {
		gc->RdFldList(fl1);
	}
	else {
		neg = true;
	}

	while (true) {
		switch (gc->Lexem) {
		case '!': {
			if (neg) {
				gc->RdLex();
				all = true;
				continue;
			}
			break;
		}
		case '?': {
			gc->RdLex();
			EO->UserSelFlds = true;
			continue;
			break;
		}
		}
		break;
	}

	gc->Accept(')');
	if (!neg) {
		EO->Flds = fl1;
		return;
	}
	EO->Flds.clear();
	for (auto& f : gc->processing_F->FldD) {
		if (((f->isStored()) || all) && !FieldInList(f, fl1)) {
			EO->Flds.push_back(f);
		}
	}
	if (EO->Flds.empty()) {
		gc->OldError(117);
	}
}

std::string RdByteList()
{
	gc->Accept('(');

	std::string s;
	short l = 0;

	while (true) {
		short i1 = gc->RdInteger();
		short i2 = i1;
		if (i1 < 0) gc->OldError(133);
		if (gc->Lexem == _subrange) {
			gc->RdLex();
			i2 = gc->RdInteger();
			if (i2 < i1) gc->OldError(133);
		}
		if ((i2 > 255) || (l + i2 - i1 >= 255)) gc->OldError(133);
		for (short i = i1; i <= i2; i++) {
			l++;
			s += static_cast<char>(i);
		}
		if (gc->Lexem == ',') {
			gc->RdLex();
			continue;
		}
		break;
	}

	gc->Accept(')');

	return s;
}

std::set<uint16_t> RdAccRights()
{
	std::set<uint16_t> result;
	std::string s = RdByteList();

	for (char c : s) {
		result.insert(static_cast<uint16_t>(c));
	}

	return result;
}

bool RdUserView(FileD* file_d, std::string ViewName, EditOpt* EO)
{
	// TODO: proc je tady 'EOD' a proc se kopiruje tam a zpet z/do options ???
	bool found = false, Fin = false, FVA = false;
	XKey* K = nullptr;

	while (true) {
		if (file_d->TxtPosUDLI == 0) {
			file_d = file_d->OrigFD;
			if ((file_d != nullptr) && !found) continue;
			break;
		}
		gc->ResetCompilePars();
		gc->SetInpTTxtPos(file_d);
		gc->RdLex();
		if ((gc->Lexem != '#') || (gc->ForwChar != 'U')) {
			file_d = file_d->OrigFD;
			if ((file_d != nullptr) && !found) continue;
			break;
		}
		gc->RdLex(); // #
		gc->RdLex(); // U
		while (true) {
			std::string sLexWord = gc->LexWord;
			if (EquUpCase(ViewName, sLexWord)) found = true;
			EO->ViewName = gc->LexWord;

			// skip access rights in brackets (already loaded in FileD->ViewNames)
			gc->RdLex(); // '('
			do {
				gc->RdLex();
			} while (!(gc->Lexem == ')' || gc->Lexem == 0x1A));
			gc->RdLex();
			gc->RdLex(); // "):"

			K = gc->RdViewKey(file_d);
			if (K != nullptr) {
				gc->RdLex(); // ','
				EO->ViewKey = K;
			}
			RdBegViewDcl(EO);
			while (gc->Lexem == ',') {
				FVA = FileVarsAllowed;
				FileVarsAllowed = false;
				if (!RdViewOpt(gc, EO, file_d)) gc->Error(44);
				FileVarsAllowed = FVA;
			}
			if (!found && (gc->Lexem == ';')) {
				gc->RdLex();
				if (!(gc->Lexem == '#' || gc->Lexem == 0x1A)) continue;
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
	EditOpt EO;
	EO.UserSelFlds = true;
	gc->RdLex();

	while (true) {
		gc->TestIdentif();
		TestDupl(file_d);

		for (FileD* f : CRdb->v_files) {
			TestDupl(f);
		}

		std::string view_name = gc->LexWord;
		gc->RdLex();
		std::string view_rights = RdByteList();
		// insert view name and rights into ViewNames (e.g. "VIEW1:\001\002\005")
		file_d->ViewNames.push_back(view_name + ':' + view_rights);

		gc->Accept(':');

		XKey* K = gc->RdViewKey(file_d); // nacteni klice, podle ktereho budou polozky setrideny

		if (K != nullptr) {
			gc->Accept(',');
			EO.ViewKey = K;
		}

		RdBegViewDcl(&EO);

		while (gc->Lexem == ',') {
			if (!RdViewOpt(gc, &EO, file_d)) {
				gc->Error(44);
			}
		}

		if (gc->Lexem == ';') {
			gc->RdLex();
			if (!(gc->Lexem == '#' || gc->Lexem == 0x1A)) continue;
		}

		break;
	}
}

void TestDupl(FileD* FD)
{
	for (std::string& view_name : FD->ViewNames) {
		size_t i = view_name.find_first_of(':');
		std::string name_only = view_name.substr(0, i);
		if (EquUpCase(name_only, gc->LexWord)) {
			gc->Error(26);
		}
	}
}

void RdFieldDList(FileD* file_d, bool stored)
{
	FieldDescr* F = nullptr;
	char FTyp = 0;
	FrmlElem* Z = nullptr;

	while (true) {
		gc->TestIdentif();
		std::string name = gc->LexWord;
		F = gc->FindFldName(file_d);
		if (F != nullptr) gc->Error(26);
		gc->RdLex();
		if (!stored) {
			gc->Accept(_assign);
			Z = gc->RdFrml(FTyp, nullptr);
		}
		F = RdFieldDescr(name, stored);
		if ((file_d->FileType == DataFileType::DBF) && stored && (F->field_type == FieldType::REAL || F->field_type == FieldType::NUMERIC)) {
			gc->OldError(86);
		}

		file_d->FldD.push_back(F);
		//ChainLast(file_d->FldD.front(), F);

		if (stored) {
			if (file_d->FileType == DataFileType::FandFile && file_d->FF->file_type == FandFileType::FAND8) {
				if ((F->field_type == FieldType::REAL || F->field_type == FieldType::BOOL || F->field_type == FieldType::TEXT)) gc->OldError(35);
				else if ((F->field_type == FieldType::FIXED) && (F->NBytes > 5)) gc->OldError(36);
			}
		}
		else {
			F->Frml = Z;
			if (FTyp != F->frml_type) gc->OldError(12);
		}
		if (gc->Lexem == ';') {
			gc->RdLex();
			if (!(gc->Lexem == '#' || gc->Lexem == 0x1A)) continue;
		}
		break;
	}
}

void SetLDIndexRoot(FileD* file_d, /*LinkD* L,*/ std::deque<LinkD*>& L2)
{
	LinkD* l2 = nullptr;
	if (!L2.empty()) l2 = *L2.begin();

	bool computed = false;

	for (LinkD* L : LinkDRoot) { /* find key with equal beginning */
		if (L == l2) {
			break;
		}

		if (file_d->FF->file_type == FandFileType::INDEX) {
			for (XKey* K : file_d->Keys) {
				computed = false;
				bool continueWithNextK = false;

				for (size_t i = 0; i < L->Args.size(); i++) {
					KeyFldD* arg = L->Args[i];
					KeyFldD* KF = K->KFlds[i];

					// cmp file_d key fields with Arg fields
					if (KF == nullptr || arg->FldD != KF->FldD || arg->CompLex != KF->CompLex || arg->Descend != KF->Descend) {
						continueWithNextK = true;
						break;
					}

					if ((arg->FldD->Flg & f_Stored) == 0) {
						computed = true;
					}
					//KF = KF->pChain;
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
			gc->OldError(152);
		}
	}
}


FileD* RdFileD_Journal(const std::string& FileName, FandFileType fand_file_type)
{
	FileD* FD = gc->RdFileName();
	if (gc->Lexem == ';') gc->RdLex();
	SetMsgPar(FileName);
	if (fand_file_type != FandFileType::FAND16) gc->OldError(103);
	if (gc->Lexem != 0x1A) gc->Error(40);
#ifdef FandSQL
	if (isSql || v_files->typSQLFile) OldError(155);
#endif

	//file_d = FakeRdFDSegment(FD);
	// *** replace of RdFDSegment
	if (gc->Lexem != 0x1A) {
		gc->Accept(';');
	}
	//WORD i = FD->ChptPos.i_rec;
	FileD* journal = new FileD(*FD);
	journal->OrigFD = FD;
	journal->TxtPosUDLI = 0;
	//** end of replace of RdFDSegment

	std::vector<FieldDescr*> orig_fields = journal->FldD;
	journal->Reset();
	journal->Name = FileName;
	journal->IsJournal = true;
	journal->SetHCatTyp(fand_file_type);
	if (!PrevCompInp.empty()) {
		journal->ChptPos = OrigInp()->InpRdbPos;
	}
	std::string JournalFlds = "Upd:A,1;RecNr:F,8.0;User:F,4.0;TimeStamp:D,'DD.MM.YYYY hh:mm:ss'";
	gc->SetInpStr(JournalFlds);
	gc->RdLex();
	RdFieldDList(journal, true);

	// add all stored fields from original file
	for (FieldDescr* f : orig_fields) {
		if (f->isStored()) {
			journal->FldD.push_back(f);
			if (f->field_type == FieldType::TEXT) {
				f->frml_type = 'R';
				f->field_type = FieldType::FIXED;
				f->L = 10;
				f->Flg = f->Flg & ~f_Encryp;
			}
		}
	}

	journal->CompileRecLen();

	return journal;
}

FileD* RdFileD_Like(const std::string& FileName, FandFileType FDTyp)
{
	std::string Prefix = FileName;
	FileD* FD = gc->RdFileName();
	if (gc->Lexem == '(') {
		gc->RdLex();
		gc->TestIdentif();
		Prefix = gc->LexWord;
		gc->RdLex();
		gc->Accept(')');
	}
	//CallRdFDSegment(v_files);
	// misto nacitani objektu ze souboru budeme objekt kopirovat
	//file_d = FakeRdFDSegment(FD);
	// *** replace of RdFDSegment
	if (gc->Lexem != 0x1A) {
		gc->Accept(';');
	}
	//WORD i = FD->ChptPos.i_rec;
	FileD* like = new FileD(*FD);
	like->OrigFD = FD;
	like->TxtPosUDLI = 0;
	//** end of replace of RdFDSegment

	// copy LinkD records too
	for (LinkD* l : LinkDRoot) {
		// LinkD for OrigFD exists?
		if (l->FromFD == FD) {
			LinkD* copiedLinkD = new LinkD(*l);
			copiedLinkD->FromFD = like;
			LinkDRoot.push_front(copiedLinkD);
		}
	}

	like->IsHlpFile = false;
	if (!(FDTyp == FandFileType::FAND16
		|| FDTyp == FandFileType::INDEX)
		|| !(like->FF->file_type == FandFileType::FAND16 || like->FF->file_type == FandFileType::INDEX)
		) {
		gc->OldError(106);
	}

	for (XKey* K : like->Keys) {
		if (!K->Alias.empty()) {
			std::string s = K->Alias;
			size_t i = s.find('_');
			if (i != std::string::npos) {
				s = s.substr(i + 1, 255);
			}
			K->Alias = Prefix + "_" + s;
		}
	}

	return like;
}

// z ulohy vycte kapilotu 'F', prip. dynamickou definici 'F'
// vraci ukazatel na FileD, protoze se muze v metode vytvorit novy objekt!!!
FileD* RdFileD(std::string FileName, DataFileType data_file_type, FandFileType fand_file_type, std::string Ext)
{
	void* p = nullptr;
	FileD* file_d = nullptr; // new created FileD; will be returned from this method

	gc->ResetCompilePars();
	gc->RdLex();
	isSql = EquUpCase(Ext, ".SQL");
	bool isHlp = EquUpCase(Ext, ".HLP");
	bool isJournal = false;

	if (gc->IsKeyWord("JOURNALOF")) {
		isJournal = true;
		file_d = RdFileD_Journal(FileName, fand_file_type);
		CRdb->v_files.push_back(file_d);
		//ChainLast(FileDRoot, file_d);
		MarkStore(p);
		//goto label1;
	}
	else if (gc->IsKeyWord("LIKE")) {
		file_d = RdFileD_Like(FileName, fand_file_type);
	}
	else {
		file_d = new FileD(data_file_type);
	}

	if (!isJournal) {
		file_d->Name = FileName;
		gc->processing_F = file_d;
		file_d->SetHCatTyp(fand_file_type);
		HasTT = false;
		if ((file_d->OrigFD == nullptr) || !(gc->Lexem == 0x1A || gc->Lexem == '#' || gc->Lexem == ']')) {
			RdFieldDList(file_d, true);
		}
		file_d->GetTFileD(HasTT);
		std::deque<LinkD*> LDOld = LinkDRoot;

		// TODO: v originale je to jinak, saha si to na nasl. promenne za PrevCompInp
		// a bere posledni ChainBack
		file_d->ChptPos = InpRdbPos;

		if (isHlp) {
			FieldDescr* F0 = file_d->FldD[0];
			FieldDescr* F1 = file_d->FldD[1];
			if (F0->field_type != FieldType::ALFANUM
				|| F1 == nullptr
				|| F1->field_type != FieldType::TEXT
				|| file_d->FldD.size() != 2) {
				gc->OldError(128);
			}
			file_d->IsHlpFile = true;
		}

		while (true) {
			if ((gc->Lexem == '#') && (gc->ForwChar == 'C')) {
				gc->RdLex();
				gc->RdLex();
				RdFieldDList(file_d, false);
				continue;
			}
			if ((gc->Lexem == '#') && (gc->ForwChar == 'K')) {
				gc->RdLex();
				RdKeyD(file_d);
				continue;
			}
			break;
		}

		if (isSql && !file_d->Keys.empty()) {
			file_d->FF->file_type = FandFileType::INDEX;
		}

		int32_t result = file_d->GetXFileD();
		if (result != 0) {
			gc->OldError(result);
		}

		file_d->CompileRecLen();
		SetLDIndexRoot(file_d, LDOld);
		if (file_d->FileType == DataFileType::FandFile
			&& file_d->FF->file_type == FandFileType::INDEX
			&& file_d->Keys.empty()) {
			gc->Error(107);
		}
		if ((gc->Lexem == '#') && (gc->ForwChar == 'A')) {
			gc->RdLex();
			RdKumul();
		}

		//if (CRdb->v_files.empty()) {
		//	Chpt = file_d;
		//}
		//CRdb->v_files.push_back(file_d);

		if (Ext == "$") {
			// compile from text at run time
			file_d->IsDynFile = true;
			file_d->ChptPos.rdb = CRdb;
			MarkStore(p);
			//goto label1;
		}
		else {
			if (gc->Lexem != 0x1A) {
				file_d->TxtPosUDLI = /*OrigInp()->*/ gc->input_pos - 1;
			}
			if ((gc->Lexem == '#') && (gc->ForwChar == 'U')) {
				// nacteni uzivatelskych pohledu
				// nazev musi byt jedinecny v ramci cele ulohy
				// format: #U NazevPohledu (SeznamPristupovychPrav): DruhEditace;
				gc->RdLex();
				TestUserView(file_d);
			}
			MarkStore(p);

			LiRoots* li = new LiRoots();
			if ((gc->Lexem == '#') && (gc->ForwChar == 'D')) {
				gc->RdLex();
				TestDepend();
			}
			if ((gc->Lexem == '#') && (gc->ForwChar == 'L')) {
				gc->RdLex();
				RdChkDChain(li->Chks);
			}
			if ((gc->Lexem == '#') && (gc->ForwChar == 'I')) {
				gc->RdLex();
				ReadImplicit(file_d, li->Impls);
			}

			// TODO: delete 'li'?

			// TODO: jak toto nahradit?
			//if (PtrRec(InpRdbPos.rdb).Seg == 0/*compiled from pstring*/) {
			//	CFile->LiOfs = 0; ReleaseStore(p);
			//}

			if (gc->Lexem != 0x1A) {
				gc->Error(66);
			}
		}
	}
	//label1:
	gc->processing_F = nullptr;
	return file_d;
}

void ReadAndAddKey(FileD* file_d, std::string alias_name)
{
	XKey* K = new XKey(file_d);
	file_d->Keys.push_back(K);

	K->Alias = alias_name;
	K->IntervalTest = false;
	K->Duplic = false;
	if (gc->Lexem == _le) {
		K->IntervalTest = true;
		gc->RdLex();
	}
	else if (gc->Lexem == '*') {
#ifdef FandSQL
		if (file_d->typSQLFile) Error(155);
#endif
		K->Duplic = true;
		gc->RdLex();
	}
	K->IndexRoot = file_d->Keys.size();
	K->IndexLen = gc->RdKFList(K->KFlds, file_d);
	if (K->IndexLen > MaxIndexLen) {
		gc->OldError(105);
	}
}

void RdKeyD(FileD* file_d)
{
	FieldDescr* F = nullptr;
	FieldDescr* F2 = nullptr;
	std::vector<KeyFldD*>::iterator KF;
	KeyFldD* arg = nullptr;
	FileD* FD = nullptr;
	LinkD* L = nullptr;
	XKey* K = nullptr;
	XKey* K1 = nullptr;
	std::string name;

	gc->RdLex();

	if (gc->Lexem == '@') {
		if (!file_d->Keys.empty() || file_d->IsParFile) {
			gc->Error(26);
		}
		gc->RdLex();
		if (gc->Lexem == '@') {
			gc->RdLex();
			file_d->IsParFile = true;
		}
		else {
			name = "";
		label1:
			ReadAndAddKey(file_d, name);
		}
	}
	else {
	label2:
		gc->TestIdentif();
		name = gc->LexWord;
		gc->SkipBlank(false);
		if (gc->ForwChar == '(') {
			gc->RdLex();
			gc->RdLex();
			if (gc->Lexem == '@') {
				CheckDuplAlias(file_d, name);
				gc->RdLex();
				gc->Accept(')');
				goto label1;
			}
			RdFileOrAlias(file_d, &FD, &K);
			gc->Accept(')');
		}
		else {
			RdFileOrAlias(file_d, &FD, &K);
		}

		L = gc->FindLD(file_d, name);
		if (L != nullptr) {
			gc->OldError(26);
		}

		L = new LinkD();

		L->RoleName = name;
		L->FromFD = file_d;
		L->ToFD = FD;
		L->ToKey = K;
		LinkDRoot.push_front(L);

		if (gc->Lexem == '!') {
			if (file_d->FF->file_type != FandFileType::INDEX
#ifdef FandSQL
				&& !file_d->typSQLFile
#endif
				) gc->Error(108);
			if (K->Duplic) {
				gc->Error(153);
			}
			gc->RdLex();
			L->MemberRef = 1;
			if (gc->Lexem == '!') {
				gc->RdLex();
				L->MemberRef = 2;
			}
	}
		//Arg = &L->Args;
		KF = K->KFlds.begin();

		while (true) {
			F = gc->RdFldName(file_d);
			if (F->field_type == FieldType::TEXT) gc->OldError(84);
			arg = new KeyFldD();
			arg->FldD = F;
			arg->CompLex = (*KF)->CompLex;
			arg->Descend = (*KF)->Descend;
			L->Args.push_back(arg);

			F2 = (*KF)->FldD;
			if (F->field_type != F2->field_type || F->field_type != FieldType::DATE
				&& F->L != F2->L || F->field_type == FieldType::FIXED
				&& F->M != F2->M) {
				gc->OldError(12);
			}

			++KF; // = KF->pChain;
			if (KF != K->KFlds.end()) {
				gc->Accept(',');
				continue;
			}
			break;
		}
}

	if (gc->Lexem == ';') {
		gc->RdLex();
		if (!(gc->Lexem == '#' || gc->Lexem == 0x1A)) goto label2;
	}
}

void CheckDuplAlias(FileD* file_d, pstring name)
{
	if (file_d->FF->file_type != FandFileType::INDEX
#ifdef FandSQL
		&& !file_d->typSQLFile
#endif
		) gc->Error(108);
	LookForK(file_d, &name, file_d);

	/*FileD* F = FileDRoot;
	while (F != nullptr) {
		LookForK(file_d, &name, F);
		F = F->pChain;
	}*/
	for (FileD* f : CRdb->v_files) {
		LookForK(file_d, &name, f);
	}
}

void LookForK(FileD* file_d, pstring* Name, FileD* F)
{
	std::string name = *Name;
	if (EquUpCase(F->Name, name)) {
		gc->Error(26);
	}

	for (XKey* K : file_d->Keys) {
		if (EquUpCase(K->Alias, *Name)) {
			gc->Error(26);
		}
	}
}

XKey* RdFileOrAlias1(FileD* F)
{
	if (F->Keys.empty()) return nullptr;

	std::string lw = gc->LexWord;
	XKey* result = nullptr;

	if (!EquUpCase(F->Name, lw)) {
		for (XKey* k : F->Keys) {
			if (EquUpCase(k->Alias, lw)) {
				result = k;
				break;
			}
		}
	}
	else {
		result = F->Keys[0];
	}

	return result;
}

void RdFileOrAlias(FileD* file_d, FileD** FD, XKey** KD)
{
	RdbD* r = nullptr;
	gc->TestIdentif();
	XKey* k = RdFileOrAlias1(file_d);
	FileD* found_f = nullptr;

	if (k != nullptr) goto label1;
	r = CRdb;

	while (r != nullptr) {
		/*file_d = r->v_files;
		while (file_d != nullptr) {
			k = RdFileOrAlias1(file_d);
			if (k != nullptr) goto label1;
			file_d = file_d->pChain;
		}*/
		for (FileD* f : r->v_files) {
			k = RdFileOrAlias1(f);
			found_f = f;
			if (k != nullptr) goto label1;
		}
		r = r->ChainBack;
	}
	gc->Error(9);
label1:
	if (k == nullptr) gc->Error(24);
	gc->RdLex();
	*FD = found_f == nullptr ? file_d : found_f;
	*KD = k;
}

void TestDepend()
{
	FrmlElem* ZBool = nullptr;
	FrmlElem* Z = nullptr;
	FieldDescr* F = nullptr;
	char FTyp;
	uint8_t* p = nullptr;
	gc->RdLex();
	MarkStore(p);
label1:
	gc->Accept('(');
	ZBool = gc->RdBool(nullptr);
	gc->Accept(')');
label2:
	F = gc->RdFldName(gc->processing_F);
	if ((F->Flg & f_Stored) == 0) gc->OldError(14);
	gc->Accept(_assign);
	Z = gc->RdFrml(FTyp, nullptr);
	if (F->frml_type != FTyp) {
		gc->Error(12);
	}
	if (gc->Lexem == ';') {
		gc->RdLex();
		if (!(gc->Lexem == '#' || gc->Lexem == 0x1A)) {
			if (gc->Lexem == '(') {
				goto label1;
			}
			else {
				goto label2;
			}
		}
	}
	ReleaseStore(&p);
}

/// <summary>
/// read implicit values - #I
/// </summary>
/// <param name="file_d"></param>
/// <param name="IDRoot"></param>
void ReadImplicit(FileD* file_d, std::vector<Implicit*>& IDRoot)
{
	char FTyp = '\0';
	gc->RdLex();

	while (true) {
		FieldDescr* F = gc->RdFldName(file_d);

		if ((F->Flg & f_Stored) == 0) {
			gc->OldError(14);
		}

		gc->Accept(_assign);
		FrmlElem* Z = gc->RdFrml(FTyp, nullptr);

		if (FTyp != F->frml_type) {
			gc->OldError(12);
		}

		Implicit* ID = new Implicit(F, Z);
		IDRoot.push_back(ID);

		if (gc->Lexem == ';') {
			gc->RdLex();
			if (!(gc->Lexem == '#' || gc->Lexem == 0x1A)) {
				continue;
			}
		}

		break;
	}
}

void RdKumul()
{
	WORD Low = 0;
	gc->RdLex();

	while (true) {
		Additive* AD = new Additive();
		gc->processing_F->Add.push_back(AD);

		if (gc->IsKeyWord("IF")) {
			AD->Bool = gc->RdBool(nullptr);
			gc->AcceptKeyWord("THEN");
			RdRoleField(AD);
			RdImper(AD);
			RdAssign(AD);
		}
		else {
			RdRoleField(AD);

			if (gc->Lexem == '(') {
				Low = gc->input_pos;
				gc->RdLex();
				FileD* previous = gc->processing_F;
				gc->processing_F = AD->File2;
				AD->Chk = ReadLogicControl(Low);
				gc->processing_F = previous;
				gc->Accept(')');
			}
			RdImper(AD);

			if ((AD->Chk == nullptr) && (gc->Lexem == _assign)) {
				RdAssign(AD);
			}
			else {
				gc->Accept(_addass);
				AD->Assign = false;
				gc->TestReal(AD->Field->frml_type);
				AD->Frml = gc->RdRealFrml(nullptr);
			}
		}

		if (gc->Lexem == ';') {
			gc->RdLex();
			if (!(gc->Lexem == '#' || gc->Lexem == 0x1A)) continue;
		}

		break;
	}
}

void RdRoleField(Additive* AD)
{
	if (!gc->IsRoleName(true, gc->processing_F, &AD->File2, &AD->LD)) {
		gc->Error(9);
	}
	gc->Accept('.');
	FieldDescr* F = gc->RdFldName(AD->File2);
	AD->Field = F;
	if ((F->Flg & f_Stored) == 0) {
		gc->OldError(14);
	}
	if (gc->IsKeyArg(F, AD->File2)) {
		gc->OldError(135);
	}
}

void RdImper(Additive* AD)
{
	if (gc->Lexem == '!') {
		gc->RdLex();
		AD->Create = 1;
		if (AD->LD != nullptr) {
			for (KeyFldD* KF : AD->LD->ToKey->KFlds) {
				if ((KF->FldD->Flg & f_Stored) == 0) {
					gc->OldError(148);
				}
			}
		}
		if (gc->Lexem == '!') {
			gc->RdLex();
			AD->Create = 2;
		}
		if (gc->Lexem == '!') {
			gc->RdLex();
			AD->Create = 2;
		}
	}
}

void RdAssign(Additive* AD)
{
	FrmlElem* Z = nullptr; char FTyp = '\0';
#ifdef FandSQL
	if (AD->File2->typSQLFile) Error(155);
#endif
	gc->Accept(_assign);
	AD->Assign = true;
	AD->Frml = gc->RdFrml(FTyp, nullptr);
	if (FTyp != AD->Field->frml_type) gc->OldError(12);
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
