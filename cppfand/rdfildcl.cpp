#include "rdfildcl.h"
#include "ChkD.h"
#include "compile.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "legacy.h"
#include "oaccess.h"
#include "rdproc.h"
#include "runfrml.h"
#include "runproj.h"
#include "XFile.h"
#include "../Indexes/XPage.h"

bool HasTT;
bool issql;

FieldDescr* RdFldDescr(pstring Name, bool Stored)
{
	const BYTE TabF[19] = { 0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8 };
	FieldDPtr F = new FieldDescr();
	pstring* S = nullptr;
	WORD L = 0, M = 0, NBytes = 0;
	BYTE Flg = 0;
	char Typ = 0, FrmlTyp = 0, c = 0;
	WORD i = 0, n = 0, n1 = 0;
	pstring ss;
	std::string sstr;

	F->Name = Name;
	if (Stored) Flg = f_Stored;
	else Flg = 0;
	Accept(':');
	if ((Lexem != _identifier) || (LexWord.length() > 1)) Error(10);
	Typ = static_cast<char>(LexWord[1]);
	RdLex();
	FrmlTyp = 'S'; M = 0;
	if (Typ == 'N' || Typ == 'F') {
		Accept(',');
		L = RdInteger();
	}
	switch (Typ) {
	case 'N': {
		NBytes = (L + 1) / 2;
		if (CurrChar == 'L') {
			RdLex();
			M = LeftJust;
		}
		break;
	}
	case 'F': {
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
	case 'R': { NBytes = 6; FrmlTyp = 'R'; L = 17; M = 5; break; }
	case 'A': {
		Accept(',');
		if (!Stored || (Lexem != _quotedstr)) {
			L = RdInteger();
			if (L > 255) Error(3);
			if (CurrChar == 'R') RdLex();
			else M = LeftJust;
		}
		else {
			S = RdStrConst();
			sstr = *S;
			L = 0; c = '?'; n = 0;
			for (i = 1; i <= S->length(); i++) {
				switch ((*S)[i]) {
				case '[': { if (c == '?') c = '['; else goto label1; break; }
				case ']': { if (c == '[') c = '?'; else goto label1; break; }
				case '(': {
					if (c == '?') { c = '('; n1 = 0; n = 0; }
					else goto label1;
					break;
				}
				case ')': {
					if ((c == '(') && (n1 > 0) && (n > 0)) {
						c = '?'; L += MaxW(n1, n);
					}
					else goto label1;
					break;
				}
				case '|': {
					if ((c == '(') && (n1 > 0)) { n = MaxW(n1, n); n1 = 0; }
					else goto label1;
					break;
				}
				default: { if (c == '(') n1++; else L++; break; }
				}
			}
			Flg += f_Mask;
			M = LeftJust;
			if (c != '?')
				label1:
			Error(171);
		}
		NBytes = L;
		goto label2;
		break;
	}
	case 'D': {
		ss[0] = 0;
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
		L = sstr.length(); Flg += f_Mask;
		break;
	}
	case 'B': {
		L = 1; NBytes = 1; FrmlTyp = 'B';
		break;
	}
	case 'T': {
		if (Lexem == ',') {
			RdLex();
			L = RdInteger() + 2;
		}
		else L = 1;
		NBytes = sizeof(longint);
		HasTT = true;
	label2:
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
	if ((L > TxtCols - 1) && (Typ != 'A')) OldError(3);
	F->Typ = Typ; F->FrmlTyp = FrmlTyp; F->L = L; F->M = M;
	F->NBytes = NBytes;	F->Flg = Flg; F->Mask = sstr;
	return F;
}

ChkD* RdChkD(WORD Low)
{
	WORD Upper = 0, N = 0;
	FrmlElem* Z = nullptr;
	//C = (ChkD*)GetZStore(sizeof(*C));
	ChkD* C = new ChkD();
	ChkD* result = C;
	C->Bool = RdBool();
	Upper = OldErrPos;
	if (Lexem == '?') { RdLex(); C->Warning = true; }
	if (Lexem == ':') { RdLex(); C->TxtZ = RdStrFrml(); }
	else {
		N = Upper - Low;
		//if (N > sizeof(pstring)) N = pred(sizeof(pstring));
		Z = new FrmlElem4(_const, 0); // GetOp(_const, N + 1);
		C->TxtZ = Z;
		auto iZ = (FrmlElem4*)Z;
		((FrmlElem4*)Z)->S = std::string((char*)&InpArrPtr[Low], N);
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
		// this FD was created as 'LIKE'
		RdChkDsFromPos(FD->OrigFD, C);
	}
	if (FD->ChptPos.R == nullptr) return;
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
	FieldListEl* fl1 = new FieldListEl(); // FieldListEl* fl1 = nullptr;
	EO->UserSelFlds = false;
	if (Lexem == '^') { RdLex(); neg = true; } Accept('(');
	if (Lexem == _identifier) RdFldList(&fl1);
	else neg = true;
label1:
	switch (Lexem) {
	case '!': if (neg) { RdLex(); all = true; goto label1; } break;
	case '?': { RdLex(); EO->UserSelFlds = true; goto label1; break; };
	}
	Accept(')');
	if (!neg) { EO->Flds = fl1; return; }
	EO->Flds = nullptr;
	for (auto& f : CFile->FldD) {
		if ((((f->Flg & f_Stored) != 0) || all) && !FieldInList(f, fl1)) {
			fl = new FieldListEl(); // (FieldListEl*)GetStore(sizeof(*fl));
			fl->FldD = f;
			if (EO->Flds == nullptr) { EO->Flds = fl; fl->Chain = nullptr; }
			else ChainLast(EO->Flds, fl);
		}
	}
	if (EO->Flds == nullptr) OldError(117);
}

void RdByteList(pstring* s)
{
	integer i, i1, i2, l;
	Accept('('); l = 0;
label1:
	i1 = RdInteger();
	i2 = i1;
	if (i1 < 0) OldError(133);
	if (Lexem == _subrange) {
		RdLex();
		i2 = RdInteger();
		if (i2 < i1) OldError(133);
	}
	if ((i2 > 255) || (l + i2 - i1 >= 255)) OldError(133);
	for (i = i1; i < i2; i++)
	{
		l++;
		s[l] = char(i);
	}
	if (Lexem == ',') { RdLex(); goto label1; }
	s[0] = char(l);
	Accept(')');
}

void RdByteListInStore()
{
	pstring s;
	RdByteList(&s); StoreStr(s);
}

bool RdUserView(std::string ViewName, EditOpt* EO)
{
	// TODO: proc je tady 'EOD' a proc se kopiruje tam a zpet z/do EO ???
	bool found = false, Fin = false, FVA = false;
	XKey* K = nullptr;
	//EditOpt EOD;
	FileD* fd = CFile;
	//Move(EO, &EOD, sizeof(EOD));
	while (true) {
		if (fd->TxtPosUDLI == 0) {
			fd = fd->OrigFD;
			if ((fd != nullptr) && !found) continue;
			break;
		}
		ResetCompilePars();
		SetInpTTxtPos(fd);
		RdLex();
		if ((Lexem != '#') || (ForwChar != 'U')) {
			fd = fd->OrigFD;
			if ((fd != nullptr) && !found) continue;
			break;
		}
		RdLex(); // #
		RdLex(); // U
		while (true) {
			//Move(&EOD, EO, sizeof(*EO));
			std::string sLexWord = LexWord;
			if (EquUpcase(ViewName, sLexWord)) found = true;
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

		fd = fd->OrigFD;
		if ((fd != nullptr) && !found) continue;
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
			FD = (FileD*)FD->Chain;
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
		if (EquUpcase(S->S, tmp)) Error(26);
		S = (StringList)S->Chain;
	}
}

void RdFieldDList(bool Stored)
{
	pstring Name; FieldDescr* F = nullptr; char FTyp = 0; FrmlPtr Z = nullptr;
label1:
	//if (InpArrLen == 0x0ce2) {
	//	printf("D\n");
	//}
	TestIdentif();
	Name = LexWord;
	F = FindFldName(CFile);
	if (F != nullptr) Error(26);
	RdLex();
	if (!Stored) { Accept(_assign); Z = RdFrml(FTyp); }
	F = RdFldDescr(Name, Stored);
	if ((CFile->Typ == 'D') && Stored && (F->Typ == 'R' || F->Typ == 'N')) OldError(86);

	CFile->FldD.push_back(F);
	ChainLast(CFile->FldD.front(), F);

	if (Stored) {
		if (CFile->Typ == '8') { if ((F->Typ == 'R' || F->Typ == 'B' || F->Typ == 'T')) OldError(35); }
		else if ((F->Typ == 'F') && (F->NBytes > 5)) OldError(36);
	}
	else { F->Frml = Z; if (FTyp != F->FrmlTyp) OldError(12); }
	if (Lexem == ';') {
		RdLex();
		if (!(Lexem == '#' || Lexem == 0x1A)) goto label1;
	}
}

void FakeRdFDSegment(FileD* FD)
{
	if (Lexem != 0x1A) {
		Accept(';');
	}
	WORD i = FD->ChptPos.IRec;
	CFile = new FileD(*FD);
	CFile->OrigFD = FD;
	CFile->TxtPosUDLI = 0;
}

// ze souboru .000 vycte data
void* RdFileD(std::string FileName, char FDTyp, std::string Ext)
{
	std::string JournalFlds = "A Upd,1;F RecNr,8.0;F User,4.0;D TimeStamp,'DD.MM.YYYY mm hh:ss'";
	FileD* FD = nullptr; XKey* K = nullptr;
	FieldDescr* F = nullptr; FieldDescr* F2 = nullptr;
	void* p = nullptr;
	ChkD* C = nullptr; LinkD* LDOld = nullptr;
	size_t n = 0, i = 0; bool isHlp = false;
	std::string Prefix, s;
	LiRoots* li = nullptr;
	CompInpD* ChPos = nullptr;

	ResetCompilePars();
	RdLex();
	issql = SEquUpcase(Ext, ".SQL");
	isHlp = SEquUpcase(Ext, ".HLP");
	if (IsKeyWord("JOURNALOF")) {
		FD = RdFileName();
		if (Lexem == ';') RdLex();
		SetMsgPar(FileName);
		if (FDTyp != '6') OldError(103);
		if (Lexem != 0x1A) Error(40);
#ifdef FandSQL
		if (issql || FD->typSQLFile) OldError(155);
#endif
		LDOld = LinkDRoot;
		CallRdFDSegment(FD);
		LinkDRoot = LDOld;
		F = CFile->FldD.front();
		CFile->Reset();
		CFile->Name = FileName;
		CFile->IsJournal = true;
		SetHCatTyp(FDTyp);
		if (PrevCompInp != nullptr) {
			CFile->ChptPos = OrigInp()->InpRdbPos;
		}
		SetInpStr(JournalFlds);
		RdLex();
		RdFieldDList(true);
		F2 = (FieldDescr*)LastInChain(CFile->FldD.front());
		while (F != nullptr) {
			if ((F->Flg & f_Stored) != 0) {
				F2->Chain = F;
				F2 = F;
				if (F->Typ == 'T') {
					/* !!! with F^ do!!! */
					F->FrmlTyp = 'R';
					F->Typ = 'F';
					F->L = 10;
					F->Flg = F->Flg & !f_Encryp;
				}
				F = (FieldDescr*)F->Chain;
			}
		}
		F2->Chain = nullptr;
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
		//CallRdFDSegment(FD);
		// misto nacitani objektu ze souboru budeme objekt kopirovat
		FakeRdFDSegment(FD);
		CFile->IsHlpFile = false;
		if (!(FDTyp == '6' || FDTyp == 'X') || !(CFile->Typ == '6' || CFile->Typ == 'X')) OldError(106);
		K = CFile->Keys;
		while (K != nullptr) {
			if (!K->Alias.empty()) {
				s = K->Alias;
				i = s.find('_');
				if (i != std::string::npos) s = s.substr(i + 1, 255);
				s = Prefix + "_" + s;
				K->Alias = s;
			}
			K = K->Chain;
		}
	}
	else {
		CFile = new FileD();
	}
	//if (InpArrLen == 6261 && CurrPos == 5052)
	//if (FileName == "MZDY")
	//{
	//	printf("RdFileD() r. 530\n");
	//}
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
		F2 = (FieldDescr*)F->Chain;
		if ((F->Typ != 'A') || (F2 == nullptr) || (F2->Typ != 'T') || (F2->Chain != nullptr)) OldError(128);
		CFile->IsHlpFile = true;
	}
label2:
	if ((Lexem == '#') && (ForwChar == 'C')) {
		RdLex();
		RdLex();
		RdFieldDList(false);
		goto label2;
	}
	if ((Lexem == '#') && (ForwChar == 'K')) {
		RdLex();
		RdKeyD();
		goto label2;
	}
	if (issql && (CFile->Keys != nullptr)) CFile->Typ = 'X';
	GetXFileD();
	CompileRecLen();
	SetLDIndexRoot(LinkDRoot, LDOld);
	if ((CFile->Typ == 'X') && (CFile->Keys == nullptr)) Error(107);
	if ((Lexem == '#') && (ForwChar == 'A'))
	{
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

	if (Ext == "$"/*compile from text at run time*/) {
		CFile->IsDynFile = true;
		CFile->ChptPos.R = CRdb;
		MarkStore(p);
		goto label1;
	}
	if (Lexem != 0x1A) {
		CFile->TxtPosUDLI = /*OrigInp()->*/CurrPos - 1;
	}
	if ((Lexem == '#') && (ForwChar == 'U'))
	{
		// nacteni uzivatelskych pohledu
		// nazev musi byt jedinecny v ramci cele ulohy
		// format: #U NazevPohledu (SeznamPristupovychPrav): DruhEditace;
		RdLex();
		TestUserView();
	}
	MarkStore(p);
	li = new LiRoots();
	CFile->LiOfs = uintptr_t(li) - uintptr_t(CFile);
	if ((Lexem == '#') && (ForwChar == 'D')) { RdLex(); TestDepend(); }
	if ((Lexem == '#') && (ForwChar == 'L')) { RdLex(); RdChkDChain(&li->Chks); }
	if ((Lexem == '#') && (ForwChar == 'I')) { RdLex(); RdImpl(&li->Impls); }
	// TODO: jak toto nahradit?
	//if (PtrRec(InpRdbPos.R).Seg == 0/*compiled from pstring*/) {
	//	CFile->LiOfs = 0; ReleaseStore(p);
	//}
	if (Lexem != ']' && Lexem != 0x1A) Error(66); // !!! pridana podminka ']' oproti originalu
label1:
	return p; // ma asi vracet HeapPtr
}

void RdKeyD()
{
	FieldDescr* F = nullptr; FieldDescr* F2 = nullptr;
	KeyFldD* KF = nullptr; KeyFldD* Arg = nullptr;
	FileD* FD = nullptr; LinkD* L = nullptr;
	XKey* K = nullptr; XKey* K1 = nullptr;
	pstring Name; WORD N = 0;
	RdLex();
	if (Lexem == '@')
	{
		if ((CFile->Keys != nullptr) || CFile->IsParFile) Error(26);
		RdLex();
		if (Lexem == '@') {
			RdLex();
			CFile->IsParFile = true;
		}
		else {
			Name = "";
		label1:
			K = new XKey();
			K1 = (XKey*)(&CFile->Keys);
			N = 1;
			while (K1->Chain != nullptr) { K1 = K1->Chain; N++; }
			K1->Chain = K;
			K->Alias = Name;
			K->Intervaltest = false;
			K->Duplic = false;
			if (Lexem == _le) {
				K->Intervaltest = true;
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
			if (K->IndexLen > MaxIndexLen) OldError(105);
		}
		goto label6;
	}
label2:
	TestIdentif();
	Name = LexWord;
	SkipBlank(false);
	if (ForwChar == '(')
	{
		RdLex(); RdLex();
		if (Lexem == '@')
		{
			CheckDuplAlias(Name);
			RdLex(); Accept(')');
			goto label1;
		}
		RdFileOrAlias(&FD, &K);
		Accept(')');
	}
	else RdFileOrAlias(&FD, &K);
	L = FindLD(Name);
	if (L != nullptr) OldError(26);
	L = new LinkD();
	//L->Args = new KeyFldD(); // pridano navic, aby to o par radku niz nepadalo
	L->Chain = LinkDRoot;
	LinkDRoot = L;
	//Move(&Name, &L->RoleName, Name.length() + 1);
	L->RoleName = Name;
	L->FromFD = CFile; L->ToFD = FD; L->ToKey = K;
	if (Lexem == '!') {
		if (CFile->Typ != 'X'
#ifdef FandSQL
			&& !CFile->typSQLFile
#endif
			) Error(108);
		if (K->Duplic) Error(153);
		RdLex();
		L->MemberRef = 1;
		if (Lexem == '!') {
			RdLex();
			L->MemberRef = 2;
		}
	}
	Arg = (KeyFldD*)&L->Args;
	KF = K->KFlds;
label3:
	F = RdFldName(CFile);
	if (F->Typ == 'T') OldError(84);
	Arg->Chain = new KeyFldD();
	Arg = (KeyFldD*)Arg->Chain;
	/* !!! with Arg^ do!!! */
	{ Arg->FldD = F; Arg->CompLex = KF->CompLex; Arg->Descend = KF->Descend; }
	F2 = KF->FldD;
	if ((F->Typ != F2->Typ) || (F->Typ != 'D') && (F->L != F2->L) ||
		(F->Typ == 'F') && (F->M != F2->M)) OldError(12);
	KF = (KeyFldD*)KF->Chain;
	if (KF != nullptr) {
		Accept(',');
		goto label3;
	}
label6:
	if (Lexem == ';')
	{
		RdLex();
		if (!(Lexem == '#' || Lexem == 0x1A)) goto label2;
	}
}

void CheckDuplAlias(pstring Name)
{
	FileDPtr F;
	if (CFile->Typ != 'X'
#ifdef FandSQL
		&& !CFile->typSQLFile
#endif
		) Error(108);
	LookForK(&Name, CFile);
	F = FileDRoot;
	while (F != nullptr) {
		LookForK(&Name, F);
		F = (FileD*)F->Chain;
	}
}

void LookForK(pstring* Name, FileD* F)
{
	XKey* K = nullptr;
	std::string name = *Name;
	if (SEquUpcase(F->Name, name)) Error(26);
	K = F->Keys;
	while (K != nullptr) {
		if (SEquUpcase(K->Alias, *Name)) Error(26);
		K = K->Chain;
	}
}

XKey* RdFileOrAlias1(FileD* F)
{
	XKey* k = F->Keys;
	std::string lw = LexWord;
	if (!EquUpcase(F->Name, lw))
		while (k != nullptr) {
			std::string lw = LexWord;
			if (EquUpcase(k->Alias, lw)) goto label1;
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
		f = r->FD;
		while (f != nullptr) {
			k = RdFileOrAlias1(f);
			if (k != nullptr) goto label1;
			f = (FileD*)f->Chain;
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

void SetLDIndexRoot(LinkD* L, LinkD* L2)
{
	XKey* K = nullptr;
	KeyFldD* Arg = nullptr;
	KeyFldD* KF = nullptr;
	bool cmptd = false;
	L = LinkDRoot;
	while (L != L2) {   /* find key with equal beginning */
		if (CFile->Typ == 'X') {
			K = CFile->Keys;
			while (K != nullptr) {
				KF = K->KFlds;
				Arg = L->Args;
				cmptd = false;
				while (Arg != nullptr) {
					if ((KF == nullptr) || (Arg->FldD != KF->FldD)
						|| (Arg->CompLex != KF->CompLex)
						|| (Arg->Descend != KF->Descend))
						goto label1;
					if ((Arg->FldD->Flg & f_Stored) == 0) cmptd = true;
					Arg = (KeyFldD*)Arg->Chain;
					KF = (KeyFldD*)KF->Chain;
				}
				L->IndexRoot = K->IndexRoot;
				goto label2;
			label1:
				K = K->Chain;
			}
		}
	label2:
		if ((L->MemberRef != 0) && ((L->IndexRoot == 0) || cmptd)) {
			SetMsgPar(L->RoleName);
			OldError(152);
		}
		L = L->Chain;
		CFile->nLDs++;
	}
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
	ZBool = RdBool();
	Accept(')');
label2:
	F = RdFldName(CFile);
	if ((F->Flg & f_Stored) == 0) OldError(14);
	Accept(_assign);
	Z = RdFrml(FTyp);
	if (F->FrmlTyp != FTyp) {
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
	ReleaseStore(p);
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
	Z = RdFrml(FTyp);
	if (FTyp != F->FrmlTyp) OldError(12);
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
			AD->Bool = RdBool();
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
				TestReal(AD->Field->FrmlTyp);
				AD->Frml = RdRealFrml();
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
			KeyFldDPtr KF = AD->LD->ToKey->KFlds;
			while (KF != nullptr) {
				if ((KF->FldD->Flg & f_Stored) == 0) OldError(148);
				KF = (KeyFldD*)KF->Chain;
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
	AD->Frml = RdFrml(FTyp);
	if (FTyp != AD->Field->FrmlTyp) OldError(12);
}

/// smaze CFile->Handle, nastavi typ na FDTyp a ziska CatIRec z GetCatIRec() - musi existovat CatFD
void SetHCatTyp(char FDTyp)
{
	/* !!! with CFile^ do!!! */
	CFile->Handle = nullptr;
	CFile->Typ = FDTyp;
	CFile->CatIRec = GetCatIRec(CFile->Name, CFile->Typ == '0'/*multilevel*/);
#ifdef FandSQL
	typSQLFile = issql;
	SetIsSQLFile();
#endif
}

void GetTFileD(char FDTyp)
{
	//if ((uintptr_t)CFile->TF == 0x534b007f)
	//{
	//	return;
	//}
	/* !!! with CFile^ do!!! */
	if (!HasTT && (CFile->TF == nullptr)) return;
	if (CFile->TF == nullptr) CFile->TF = new TFile();
	CFile->TF->Handle = nullptr;
	if (FDTyp == 'D') CFile->TF->Format = TFile::DbtFormat;
}

void GetXFileD()
{
	/* !!! with CFile^ do!!! */
	if (CFile->Typ != 'X') {
		if (CFile->XF != nullptr) {
			OldError(104);
		}
	}
	else {
		//if (CFile->XF == nullptr) CFile->XF = (XFile*)GetZStore(sizeof(XFile));
		if (CFile->XF == nullptr) {
			CFile->XF = new XFile();
		}
		CFile->XF->Handle = nullptr;
	}
}

void CallRdFDSegment(FileD* FD)
{
	RdbD* rdb = nullptr;
	void* cr = nullptr;
	FileD* cf = nullptr;
	bool b = false;
	WORD i = 0; longint pos = 0;
	if (Lexem != 0x1A) Accept(';');
	rdb = CRdb; cr = CRecPtr;
	RdbD* r = FD->ChptPos.R;
	if ((r == nullptr) || FD->IsDynFile) OldError(106);
	CRdb = r;
	i = FD->ChptPos.IRec;
	CFile = CRdb->FD;
	CRecPtr = CFile->RecPtr;
	ReadRec(CFile, i, CRecPtr);
	pos = _T(ChptOldTxt);
	if (pos <= 0) Error(25);
	b = RdFDSegment(i, pos);
	cf = CFile; CRdb = rdb;
	if (InpRdbPos.IRec != 0) {
		CFile = rdb->FD;
		ReadRec(CFile, InpRdbPos.IRec, CRecPtr);
		CFile = cf;
	}
	CRecPtr = cr;
	if (!b) Error(25);
	CFile->OrigFD = FD;
	CFile->TxtPosUDLI = 0;
}

CompInpD* OrigInp()
{
	if (PrevCompInp == nullptr) {
		return nullptr;
	}
	CompInpD* i = PrevCompInp;
	while (i->ChainBack != nullptr) {
		i = i->ChainBack;
	}
	return i;
}
