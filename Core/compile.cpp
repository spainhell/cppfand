#include "compile.h"
#include <map>
#include <memory>

#include "Coding.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "obaseww.h"
#include "rdfildcl.h"
#include "runfrml.h"
#include "../fandio/FandTFile.h"
#include "../fandio/XWKey.h"
#include "wwmix.h"
#include "../TextEditor/TextEditor.h"
#include "../Common/compare.h"
#include "../Common/textfunc.h"
#include "../Drivers/constants.h"
#include "../Core/DateTime.h"

const BYTE MaxLen = 9;
RdbPos ChptIPos; // used in LexAnal & ProjMgr
Compiler* compiler = new Compiler();

bool KeyArgFound;
FieldDescr* KeyArgFld;

pstring QQdiv = "div";
pstring QQmod = "mod";
pstring QQround = "round";

BYTE CurrChar; // { Compile }
BYTE ForwChar, ExpChar, Lexem;
pstring LexWord;

std::string Compiler::Error(short N)
{
	ReadMessage(1000 + N);
	std::string ErrMsg = MsgLine;

	if (N == 1) {
		if (ExpChar >= ' ') {
			ErrMsg = ErrMsg + " " + (char)ExpChar;
		}
		else {
			switch (ExpChar) {
			case _assign: MsgLine = ":="; break;
			case _addass: MsgLine = "+="; break;
			case _equ: MsgLine = "="; break;
			case _number: ReadMessage(1004); break;
			case _identifier: ReadMessage(1005); break;
			case _quotedstr: ReadMessage(1013); break;
			}
			ErrMsg = ErrMsg + " " + MsgLine;
		}
	}
	CurrPos--;
	ClearKbdBuf();
	size_t l = InpArrLen;
	WORD i = CurrPos;
	if (IsTestRun && (!PrevCompInp.empty() && InpRdbPos.rdb != CRdb /* 0xinclude higher Rdb*/
		|| InpRdbPos.rdb == nullptr) /* TODO: ptr(0, 1)*/ /*LongStr + ShowErr*/
		&& StoreAvail() > l + TxtCols * TxtRows * 2 + 50)
	{
		bool upd;
		//MarkStore(p1);
		int w = PushW(1, 1, TxtCols, TxtRows);
		TextAttr = screen.colors.tNorm;
		char* p = new char[l];
		memcpy(p, InpArrPtr, l);
		if (!PrevCompInp.empty()) {
			ReadMessage(63);
		}
		else {
			ReadMessage(61);
		}
		std::string HdTxt = MsgLine;
		LongStr LS;
		LS.A = p;
		LS.LL = l;
		std::unique_ptr<TextEditor> editor = std::make_unique<TextEditor>();
		editor->SimpleEditText('T', ErrMsg, HdTxt, &LS, 0xfff, i, upd);
		p = LS.A;
		l = LS.LL;
		PopW(w);
		delete[] p;
		p = nullptr;
		//ReleaseStore(p1);
	}
	EdRecKey = ErrMsg;
	LastExitCode = i + 1 + 1;
	IsCompileErr = true;
	MsgLine = ErrMsg;
	GoExit();
	return ErrMsg;
}

void Compiler::SetInpStr(std::string& s)
{
	InpArrLen = s.length();
	InpArrPtr = (BYTE*)s.data();
	if (InpArrLen == 0) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[0];
	CurrPos = 0;
	FillChar(&InpRdbPos, sizeof(InpRdbPos), 0);
}

void Compiler::SetInpStdStr(std::string& s, bool ShowErr)
{
	InpArrLen = s.length();
	InpArrPtr = (BYTE*)s.c_str();
	if (InpArrLen == 0) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[0];
	CurrPos = 0;
	InpRdbPos.rdb = nullptr;
	if (ShowErr) InpRdbPos.rdb = nullptr; // TODO: tady bylo InpRdbPos.rdb:=ptr(0,1);
	InpRdbPos.i_rec = 0;
}

void Compiler::SetInpLongStr(LongStr* S, bool ShowErr)
{
	InpArrLen = S->LL;
	InpArrPtr = (BYTE*)&S->A[0];
	if (InpArrLen == 0) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[0];
	CurrPos = 0;
	InpRdbPos.rdb = nullptr;
	if (ShowErr) InpRdbPos.rdb = nullptr; // TODO: tady bylo InpRdbPos.rdb:=ptr(0,1);
	InpRdbPos.i_rec = 0;
}

// vycte z CFile->TF blok dat
// nastavi InpArrPtr a InptArrLen - retezec pro zpracovani
void Compiler::SetInpTTPos(FileD* file_d, int Pos, bool Decode)
{
	LongStr* s = file_d->FF->TF->ReadLongStr(Pos);
	if (Decode) {
		Coding::CodingLongStr(file_d, s);
	}
	InpArrLen = s->LL;
	InpArrPtr = (BYTE*)&s->A[0];
	if (InpArrLen == 0) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[0];
	CurrPos = 0;
}

void Compiler::SetInpTT(RdbPos* rdb_pos, bool FromTxt)
{
	if (rdb_pos->i_rec == 0) {
		std::string run_str = RunStdStr(CFile, (FrmlElem*)rdb_pos->rdb, CRecPtr);
		// std::string cannot be used here!
		// it's deleted on the end of this method!
		LongStr* run_long_str = new LongStr(run_str.length());
		run_long_str->LL = run_str.length();
		memcpy(run_long_str->A, run_str.c_str(), run_long_str->LL);
		SetInpLongStr(run_long_str, true);
		return;
	}
	InpRdbPos = *rdb_pos;

	RdbD* rdb = rdb_pos->rdb;
	uint8_t* rec = rdb->rdb_file->GetRecSpace();

	rdb->rdb_file->ReadRec(rdb_pos->i_rec, rec);
	int pos;
	if (FromTxt) {
		pos = rdb->rdb_file->loadT(ChptTxt, rec);
	}
	else {
		pos = rdb->rdb_file->loadT(ChptOldTxt, rec);
	}
	SetInpTTPos(rdb->rdb_file, pos, rdb->Encrypted);

	delete[] rec; rec = nullptr;
}

void Compiler::SetInpTTxtPos(FileD* FD)
{
	SetInpTT(&FD->ChptPos, true);
	WORD pos = FD->TxtPosUDLI;
	RdbD* r = FD->ChptPos.rdb;
	if (pos > InpArrLen) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[pos];
	CurrPos = pos;
}

void Compiler::ReadChar()
{
	CurrChar = ForwChar;
	if (CurrPos < InpArrLen)
	{
		CurrPos++;
		if (CurrPos == InpArrLen) ForwChar = 0x1A;
		else ForwChar = InpArrPtr[CurrPos];
	}
	else if (CurrPos == InpArrLen) {
		CurrPos++; ForwChar = 0x1A; // CTRL+Z = 0x1A
	}
}

WORD Compiler::RdDirective(bool& b)
{
	const pstring Dirs[6] = {
		pstring(7) = "define",
		pstring(7) = "ifdef",
		pstring(7) = "ifndef",
		pstring(7) = "else",
		pstring(7) = "endif",
		pstring(7) = "include"
	};
	WORD i;
	pstring s(12);
	RdbD* r = nullptr;
	bool res = false;

	ReadChar();
	RdForwName(s);
	for (i = 0; i < 5; i++) {
		if (EquUpCase(s, Dirs[i])) goto label1;
	}
	Error(158);
label1:
	if (i <= 2) {
		while (ForwChar == ' ') { ReadChar(); }
		if (i == 0) {
			Switches[0] = 0;
			while ((Switches.length() < sizeof(Switches) - 1)
				&& (ForwChar >= 'A' && ForwChar <= 'Z'))
			{
				Switches[0]++;
				Switches[Switches.length()] = ForwChar;
				ReadChar();
			}
		}
		else {
			if (!(ForwChar >= 'A' && ForwChar <= 'Z')) Error(158);
			ReadChar();
			b = false;
			for (WORD j = 1; j <= Switches.length(); j++)
				if (Switches[j] == CurrChar) b = true;
			if (i == 2) b = !b;
			i = 1;
		}
	}
	else if (i == 5) {
		while (ForwChar == ' ') { ReadChar(); }
		RdForwName(s);
		r = CRdb;
		// TODO: co to je??? PtrRec ...
		if (/*(PtrRec(InpRdbPos.rdb).Seg*/ InpRdbPos.rdb != nullptr) CRdb = InpRdbPos.rdb;
		res = FindChpt('I', s, false, &ChptIPos);
		CRdb = r;
		if (!res) Error(37);
	}
	if (ForwChar != '}') Error(158);
	ReadChar();
	return i;
}

void Compiler::RdForwName(pstring& s)
{
	s[0] = 0;
	while ((s.length() < 12) && (IsLetter(ForwChar) || isdigit(ForwChar)))
	{
		s[0]++; s[s.length()] = ForwChar;
		ReadChar();
	}
}

void Compiler::SkipLevel(bool withElse)
{
	WORD n = 0;
	bool b = false;
	WORD begLevel = SwitchLevel;

label1:
	switch (ForwChar) {       /* skip to directive */
	case '\'': {
		do { ReadChar(); } while (!(ForwChar == '\'' || ForwChar == 0x1A));
		break;
	}
	case '{': {
		ReadChar();
		if (ForwChar == '$') goto label3;
		n = 1;
	label2:
		switch (ForwChar) {
		case '{': { n++; break; }
		case 0x1A: { Error(11); break; }
		case '}': { n--; if (n == 0) { ReadChar(); goto label1; } break; }
		}
		ReadChar();
		goto label2;
		break;
	}
	case 0x1A: { Error(11); break; }
	}
	ReadChar();
	goto label1;
label3:
	switch (RdDirective(b)) {
	case 1/*if*/: SwitchLevel++; break;
	case 3/*else*/: if (SwitchLevel == begLevel) if (withElse) return; else Error(159); break;
	case 4/*end*/: {
		if (SwitchLevel == 0) Error(159);
		SwitchLevel--;
		if (SwitchLevel < begLevel) return;
		break;
	}
	}
	goto label1;
}

void Compiler::SkipBlank(bool toNextLine)
{
	WORD n = 0;
	bool b = false;
	CompInpD* ci = nullptr;
	char CC = CurrChar;

label1:
	switch (ForwChar)
	{
	case 0x1A: {
		if (!PrevCompInp.empty()) {
			PrevCompInp.pop_back();
			if (CurrPos <= InpArrLen) ForwChar = InpArrPtr[CurrPos];
			goto label1;
		}
		break;
	}
	case '{': {
		ReadChar();
		if (ForwChar == '$') {
			n = RdDirective(b);
			switch (n) {
			case 0: break;
			case 1: {
				SwitchLevel++;
				if (!b) SkipLevel(true);
				break;
			}
			case 5: {
				PrevCompInp.emplace_back(CompInpD());
				SetInpTT(&ChptIPos, true);
				break;
			}
			default: {
				if (SwitchLevel == 0) Error(159);
				if (n == 3) SkipLevel(false);
				else SwitchLevel--;
				break;
			}
			}
			goto label1;
		}
		else {
			n = 1;
		label2:
			switch (ForwChar) {
			case '{': {
				n++;
				break;
			}
			case 0x1A: {
				Error(11);
				break;
			}
			case '}': {
				n--;
				if (n == 0) {
					ReadChar();
					goto label1;
				}
				break;
			}
			default: break;
			}
			ReadChar();
			goto label2;
		}
		break;
	}
	default:
		if (ForwChar >= 0x00 && ForwChar <= 0x20) // ^@..' '
		{
			if (toNextLine && (ForwChar == 0x0D)) // ^M = CR = 013 = 0x0D
			{
				ReadChar();
				if (ForwChar == 0x0A) ReadChar(); // ^J = LF = 010 = 0x0A
			}
			else {
				ReadChar();
				goto label1;
			}
			break;
		}
		break;
	}
	CurrChar = CC;
}

void Compiler::OldError(short N)
{
	CurrPos = OldErrPos;
	Error(N);
}

void Compiler::RdBackSlashCode()
{
	WORD i, n;
	pstring Num;

	if (ForwChar == '\\') { ReadChar(); return; }
	Num = "";
	while ((std::isdigit(ForwChar)) && (Num.length() < 3)) {
		ReadChar();
		Num.Append(CurrChar);
	}
	if (Num == "") return;
	val(Num, n, i);
	if (n > 255) Error(7);
	CurrChar = (char)n;
}

void Compiler::RdLex()
{
	OldErrPos = CurrPos;
	SkipBlank(false);
	ReadChar();
	Lexem = CurrChar;
	if (IsLetter(CurrChar))
	{
		Lexem = _identifier;
		LexWord[1] = CurrChar;
		WORD i = 1;
		while (IsLetter(ForwChar) || isdigit(ForwChar))
		{
			i++;
			if (i > 32) Error(2);
			ReadChar();
			LexWord[i] = CurrChar;
		}
		LexWord[0] = (char)i;
		LexWord[i + 1] = '\0';
	}
	else if (isdigit(CurrChar))
	{
		Lexem = _number; LexWord[1] = CurrChar;
		WORD i = 1;
		while (isdigit(ForwChar))
		{
			i++;
			if (i > 15) Error(6);
			ReadChar();
			LexWord[i] = CurrChar;
		}
		LexWord[0] = (char)i;
	}
	else switch (CurrChar) {
	case '\'': {
		Lexem = _quotedstr;
		ReadChar(); LexWord = "";
		while (CurrChar != '\'' || ForwChar == '\'')
		{
			if (CurrChar == 0x1A) Error(17);
			if (LexWord.length() == LexWord.initLength() - 1) Error(6);
			if (CurrChar == '\'') ReadChar();
			else if (CurrChar == '\\') RdBackSlashCode();
			LexWord.Append(CurrChar); ReadChar();
		}
		break;
	}
	case ':':
		if (ForwChar == '=') { ReadChar(); Lexem = _assign; }
		break;
	case '.':
		if (ForwChar == '.') { ReadChar(); Lexem = _subrange; }
		break;
	case '=':
		if (ForwChar == '>') { ReadChar(); Lexem = _limpl; }
		else Lexem = _equ;
		break;
	case '+':
		if (ForwChar == '=') { ReadChar(); Lexem = _addass; }
		break;
	case '<': {
		switch (ForwChar) {
		case '>': { ReadChar(); Lexem = _ne; break; }
		case '=': { ReadChar();
			if (ForwChar == '>')
			{
				ReadChar(); Lexem = _lequ;
			}
			else Lexem = _le;
			break;
		}
		default: Lexem = _lt; break;
		}
		break;
	}
	case '>':
		if (ForwChar == '=') { ReadChar(); Lexem = _ge; }
		else Lexem = _gt;
		break;
	default: break;
	}
	//if (LexWord == "S333")
	//{
	//	printf("RdLex() r. 437 - %s\n", LexWord.c_str());
	//}
}

bool Compiler::IsForwPoint()
{
	return (ForwChar == '.') && (InpArrPtr[CurrPos + 1] != '.');
}

void Compiler::TestIdentif()
{
	if (Lexem != _identifier) Error(29);
}

void Compiler::TestLex(char X)
{
	if ((char)Lexem != X) { ExpChar = X; Error(1); }
}

void Compiler::Accept(char X)
{
	if (X == (char)Lexem) {
		RdLex();
	}
	else {
		ExpChar = X;
		Error(X);
	}
}

short Compiler::RdInteger()
{
	short I, J;
	val(LexWord, I, J);
	if (J != 0) Lexem = 0 /* != _number*/;
	Accept(_number);
	return I;
}

double Compiler::ValofS(pstring& S)
{
	short I = 0; double R = 0.0;

	val(S, R, I);
	if (I != 0) {
		R = ValDate(S, "DD.MM.YY");
		if (R == 0.0) {
			R = ValDate(S, "DD.MM.YYYY");
			if (R == 0.0) {
				R = ValDate(S, "mm:hh:ss.tt");
				if (R == 0.0) {
					Error(7);
				}
			}
		}
	}
	return R;
}

double Compiler::RdRealConst()
{
	pstring S;
	if (Lexem == '-') { S = '-'; RdLex(); }
	else S = "";
	TestLex(_number); S = S + LexWord;
label1:
	if ((ForwChar == '.' || ForwChar == ':')) {
		RdLex();
		if ((Lexem != _subrange) && (ForwChar >= '0' && ForwChar <= '9')) {
			S.Append(Lexem);
			RdLex();
			S = S + LexWord;
			goto label1;
		}
		return ValofS(S);
	}
	if ((ForwChar == 'e' || ForwChar == 'E')
		&& (InpArrPtr[CurrPos + 1] == '-' || (InpArrPtr[CurrPos + 1] >= 0 && InpArrPtr[CurrPos + 1] <= 9))) {
		S.Append('e'); ReadChar();
		if (ForwChar == '-') { ReadChar(); S.Append('-'); }
		RdLex(); TestLex(_number); S = S + LexWord;
	}
	RdLex();
	return ValofS(S);
}

//bool TestKeyWord(pstring S)
//{
//	return (Lexem == _identifier) && EquUpCase(S, LexWord);
//}

bool Compiler::TestKeyWord(const std::string& S)
{
	std::string lw = LexWord;
	return (Lexem == _identifier) && EquUpCase(S, lw);
}

bool Compiler::IsKeyWord(const std::string& S)
{
	if (Lexem != _identifier) return false;
	if (LexWord.length() != S.length()) return false;

	std::string sLexWord = LexWord;

	for (size_t i = 0; i < sLexWord.length(); i++) {
		BYTE lw = (BYTE)sLexWord[i];
		BYTE upcLw = UpcCharTab[lw]; // velke pismeno dle UpcCharTab
		if (upcLw != (BYTE)S[i]) return false;
	}
	RdLex();
	return true;
}

void Compiler::AcceptKeyWord(const std::string& S)
{
	if (TestKeyWord(S)) {
		RdLex();
	}
	else {
		SetMsgPar(S);
		Error(33);
	}
}

bool Compiler::IsOpt(pstring S)
{
	if (Lexem != _identifier) return false;
	if (S.length() != LexWord.length()) return false;

	if (S.length() != 0) {
		for (size_t i = 1; i <= S.length(); i++) {
			char cL = LexWord[i];
			char cU = UpcCharTab[cL];
			char cS = S[i];
			if (cU != cS) return false;
		}
	}

	RdLex();
	Accept(_equ);

	return true;
}

bool Compiler::IsDigitOpt(pstring S, WORD& N)
{
	char LastLexWord = LexWord[LexWord.length()];
	if ((Lexem == _identifier) && (LexWord.length() == S.length() + 1)
		&& EquUpCase(copy(LexWord, 1, S.length()), S)
		&& (LastLexWord >= '0' && LastLexWord <= '9'))
	{
		N = LastLexWord - '0';
		RdLex();
		Accept(_equ);
		return true;
	}
	return false;
}

pstring* Compiler::RdStrConst()
{
	pstring* S = new pstring(LexWord);
	Accept(_quotedstr);
	return S;
}

std::string Compiler::RdStringConst()
{
	std::string result = LexWord;
	Accept(_quotedstr);
	return result;
}

char Compiler::Rd1Char()
{
	if ((Lexem != _identifier) || (LexWord.length() != 1)) Error(124);
	char result = LexWord[1];
	RdLex();
	return result;
}

char Compiler::RdQuotedChar()
{
	if ((Lexem != _quotedstr) || (LexWord.length() != 1)) Error(15);
	char result = LexWord[1];
	RdLex();
	return result;
}

bool Compiler::IsIdentifStr(std::string& S)
{
	if ((S.length() == 0) || !IsLetter(S[0])) return false;
	for (size_t i = 1; i < S.length(); i++) {
		if (!(IsLetter(S[i]) || isdigit(S[i]))) return false;
	}
	return true;
}

stSaveState* Compiler::SaveCompState()
{
	stSaveState* state = new stSaveState();
	state->CurrChar = CurrChar;
	state->ForwChar = ForwChar;
	state->ExpChar = ExpChar;
	state->Lexem = Lexem;
	state->LexWord = LexWord;
	state->SpecFDNameAllowed = SpecFDNameAllowed;
	state->IdxLocVarAllowed = IdxLocVarAllowed;
	state->FDLocVarAllowed = FDLocVarAllowed;
	state->IsCompileErr = IsCompileErr;
	state->PrevCompInp = PrevCompInp;
	state->InpArrPtr = InpArrPtr;
	state->InpRdbPos = InpRdbPos;
	state->InpArrLen = InpArrLen;
	state->CurrPos = CurrPos;
	state->OldErrPos = OldErrPos;
	state->FrmlSumEl = FrmlSumEl;
	state->FrstSumVar = FrstSumVar;
	state->FileVarsAllowed = FileVarsAllowed;
	state->ptrRdFldNameFrml = ptrRdFldNameFrml;
	state->RdFunction = RdFunction;
	return state;
}

void RestoreCompState(stSaveState* p)
{
	CurrChar = p->CurrChar;
	ForwChar = p->ForwChar;
	ExpChar = p->ExpChar;
	Lexem = p->Lexem;
	LexWord = p->LexWord;
	SpecFDNameAllowed = p->SpecFDNameAllowed;
	IdxLocVarAllowed = p->IdxLocVarAllowed;
	FDLocVarAllowed = p->FDLocVarAllowed;
	IsCompileErr = p->IsCompileErr;
	PrevCompInp = p->PrevCompInp;
	InpArrPtr = p->InpArrPtr;
	InpRdbPos = p->InpRdbPos;
	InpArrLen = p->InpArrLen;
	CurrPos = p->CurrPos;
	OldErrPos = p->OldErrPos;
	FrmlSumEl = p->FrmlSumEl;
	FrstSumVar = p->FrstSumVar;
	FileVarsAllowed = p->FileVarsAllowed;
	ptrRdFldNameFrml = p->ptrRdFldNameFrml;
	RdFunction = p->RdFunction;
	delete p;
}

LocVar* Compiler::RdVarName(LocVarBlkD* LVB, bool IsParList)
{
	TestIdentif();
	LocVar* lvar = LVB->FindByName(LexWord);
	if (lvar != nullptr) Error(26); // promenna uz existuje
	else
	{
		lvar = new LocVar(LexWord);
		if (IsParList) { lvar->IsPar = true; LVB->NParam++; }
		LVB->vLocVar.push_back(lvar);

	}
	RdLex();
	return lvar;
}

KeyFldD* Compiler::RdKF(FileD* FD)
{
	//KF = (KeyFldD*)GetZStore(sizeof(KeyFldD));
	KeyFldD* KF = new KeyFldD();
	if (Lexem == _gt) {
		RdLex();
		KF->Descend = true;
	}
	if (Lexem == '~') {
		RdLex();
		KF->CompLex = true;
	}
	FieldDescr* F = RdFldName(FD);
	KF->FldD = F;
	if (F == nullptr) {
		return nullptr;
	}
	if (F->field_type == FieldType::TEXT) {
		OldError(84);
	}
	if (KF->CompLex && (F->field_type != FieldType::ALFANUM)) {
		OldError(94);
	}
	return KF;
}

WORD Compiler::RdKFList(KeyFldD** KFRoot, FileD* FD)
{
	WORD n = 0; KeyFldD* KF = nullptr;
label1:
	if (*KFRoot == nullptr) *KFRoot = RdKF(FD);
	else ChainLast(*KFRoot, RdKF(FD));

	if (Lexem == ',') {
		RdLex();
		goto label1;
	}
	n = 0;
	KF = *KFRoot;   /*looping over all fields, !only the last read*/
	while (KF != nullptr) {
		if (KF->FldD != nullptr) n += KF->FldD->NBytes;
		KF = (KeyFldD*)KF->pChain;
	}
	if (n > 255) OldError(126);
	return n;
}

void Compiler::RdLocDcl(LocVarBlkD* LVB, bool IsParList, bool WithRecVar, char CTyp)
{
	FrmlElem* Z = nullptr;
	bool b = false;
	double r = 0;
	std::string s;
	char typ = '\0';
	BYTE lx = '\0', fc = '\0';
	size_t sz = 0, n = 0;
	FileD* cf = nullptr; FileD* fd = nullptr;
	void* cr = nullptr; stSaveState* p = nullptr;
	XWKey* k = nullptr; bool rp = false;
	KeyFldD* kf = nullptr; KeyFldD* kf1 = nullptr;
	FileType FDTyp = FileType::UNKNOWN;
	std::vector<LocVar*> newVars;
label1:
	rp = false;
	if (IsParList && IsKeyWord("VAR")) {
		if (CTyp == 'D') OldError(174);
		rp = true;
	}
	newVars.clear(); // zde se budou ukladat vsechny promenne stejneho typu oddelene carkami
	newVars.push_back(RdVarName(LVB, IsParList)); // ulozime novou promennou do vektoru pro jeji dalsi nastaveni
	if (!IsParList) while (Lexem == ',') {
		RdLex();
		newVars.push_back(RdVarName(LVB, IsParList)); // vsechny stejne promenne ulozime do vektoru
	}
	Accept(':');
	Z = nullptr;
	if (IsKeyWord("BOOLEAN")) {
		if ((Lexem == _equ) && !IsParList) {
			RdLex();
			if (IsKeyWord("TRUE")) {
				// Z = new FrmlElemBool(_const, 0, true); // GetOp(_const, sizeof(bool));
				newVars[0]->B = true;
			}
			else {
				if (!IsKeyWord("FALSE")) Error(42);
			}
		}
		typ = 'B';
		sz = sizeof(bool);
		goto label2;
	}
	else if (IsKeyWord("REAL")) {
		if ((Lexem == _equ) && !IsParList) {
			RdLex();
			r = RdRealConst();
			newVars[0]->R = r;
		}
		typ = 'R';
		sz = sizeof(double);
		goto label2;
	}
	else if (IsKeyWord("STRING")) {
		if ((Lexem == _equ) && !IsParList) {
			RdLex();
			s = LexWord;
			Accept(_quotedstr);
			newVars[0]->S = s;
		}
		typ = 'S';
		sz = sizeof(int);
	label2:
		for (LocVar* locvar : newVars) {
			locvar->FTyp = typ;
			locvar->Op = _getlocvar;
			locvar->IsRetPar = rp;
			locvar->Init = Z;
		}
	}
	else if (rp) Error(168);
	else if (WithRecVar)
		if (TestKeyWord("FILE")) {
			// budeme pracovat jen s 1. promennou ve vektoru
			auto lv = newVars[0];
			lv->FTyp = 'f';
			LexWord = lv->Name;
			if (LexWord.length() > 8) OldError(2);
			fd = FindFileD();
			RdLex();
			if (IsParList) {
				if (!WithRecVar) OldError(162);
				if (fd == nullptr) OldError(163);
				lv->FD = fd;
			}
			else {
				if (fd != nullptr) OldError(26);
				FDTyp = FileType::FAND16;
				if (Lexem == '.') {
					RdLex();
					TestIdentif();
					if (EquUpCase("X", LexWord)) FDTyp = FileType::INDEX;
					else if (EquUpCase("DBF", LexWord)) FDTyp = FileType::DBF;
					else Error(185);
					RdLex();
				}
				TestLex('[');
				p = SaveCompState();
				RdFileD(CFile, lv->Name, FDTyp, "$");
				TestLex(']');
				lv->FD = CFile;
				n = CurrPos;
				lx = Lexem;
				fc = ForwChar;
				RestoreCompState(p);
				CurrPos = n; Lexem = lx; ForwChar = fc;
				RdLex();
			}
		}
		else if (IsKeyWord("INDEX")) {
			typ = 'i';
			goto label3;
		}
		else if (IsKeyWord("RECORD")) {
			typ = 'r';
		label3:
			AcceptKeyWord("OF");
			cf = CFile; cr = CRecPtr;
			CFile = RdFileName();
			if (typ == 'i') {
				if (CFile->FF->file_type != FileType::INDEX) OldError(108);
				kf1 = nullptr;
				if (Lexem == '(') {
					RdLex();
					RdKFList(&kf1, CFile);
					Accept(')');
				}
			}
			for (LocVar* locvar : newVars) {
				locvar->FTyp = typ;
				locvar->FD = CFile;
				if (typ == 'r') locvar->record = nullptr; // ptr(0,1) ??? /* for RdProc nullptr-tests + no Run*/
				/* frueher bei IsParList K = nullptr; warum? */
				else {
					k = new XWKey(CFile);
					k->Duplic = true;
					k->InWork = true;
					k->KFlds = kf1;
					kf = kf1;
					while (kf != nullptr) {
						k->IndexLen += kf->FldD->NBytes;
						kf = kf->pChain;
					}
					locvar->record = reinterpret_cast<uint8_t*>(k);
				}
			}
			CFile = cf;
			CRecPtr = cr;
		}
		else Error(137);
	else Error(39);
	if (IsParList) {
		if (Lexem == ')') return;
		else {
			Accept(';');
			goto label1;
		}
	}
	Accept(';');
	if ((Lexem != '#') && (Lexem != '.') && !TestKeyWord("BEGIN"))
		goto label1;
}

bool Compiler::FindLocVar(LocVar* LVRoot, LocVar** LV)
{
	auto result = false;
	if (Lexem != _identifier) return result;
	*LV = LVRoot;
	while (*LV != nullptr) {
		pstring lvName = (*LV)->Name.c_str();
		if (EquUpCase(lvName, LexWord)) { return true; }
		*LV = (LocVar*)(*LV)->pChain;
	}
	return result;
}

bool Compiler::FindLocVar(LocVarBlkD* LVB, LocVar** LV)
{
	if (Lexem != _identifier) return false;
	*LV = LVB->FindByName(LexWord);
	if (*LV == nullptr) return false;
	return true;
}

bool Compiler::FindChpt(char Typ, const pstring& name, bool local, RdbPos* RP)
{
	FileD* CF = CFile;
	void* CR = CRecPtr;
	CFile = Chpt;
	CRecPtr = Chpt->GetRecSpace();
	RdbD* R = CRdb;
	auto result = false;
	while (R != nullptr) {
		CFile = R->rdb_file;
		for (WORD i = 1; i <= CFile->FF->NRecs; i++) {
			CFile->ReadRec(i, CRecPtr);
			std::string chapterType = CFile->loadS(ChptTyp, CRecPtr);
			std::string chapterName = CFile->loadS(ChptName, CRecPtr);
			chapterName = TrailChar(chapterName, ' ');

			if (chapterType.length() == 1
				&& chapterType[0] == Typ
				&& EquUpCase(chapterName, name)) {

				RP->rdb = R;
				RP->i_rec = i;
				result = true;
				goto label1;
			}
		}
		if (local) goto label1;
		R = R->ChainBack;
	}
label1:
	ReleaseStore(&CRecPtr);
	CFile = CF; CRecPtr = CR;
	return result;
}

void Compiler::RdChptName(char C, RdbPos* Pos, bool TxtExpr)
{
	//if (InpArrLen == 17623 && CurrPos > 4700) {
	//	printf("");
	//}

	if (TxtExpr && (Lexem == '[')) {
		RdLex();
		Pos->rdb = (RdbD*)RdStrFrml(nullptr);
		Pos->i_rec = 0;
		Accept(']');
	}
	else {
		TestLex(_identifier);
		if (!FindChpt(C, LexWord, false, Pos)) Error(37);
		RdLex();
	}
}

std::vector<FieldDescr*> Compiler::AllFldsList(FileD* FD, bool OnlyStored)
{
	std::vector<FieldDescr*> FLRoot;
	for (FieldDescr* F : FD->FldD) {
		if (F->isStored() || !OnlyStored) {
			FLRoot.push_back(F);
		}
	}
	return FLRoot;
}

RprtOpt* Compiler::GetRprtOpt()
{
	auto RO = new RprtOpt();
	RO->Mode = _ALstg;
	RO->Style = '?';
	RO->Width = spec.AutoRprtWidth;
	return RO;
}

void Compiler::CFileLikeFD(FileD* FD, WORD MsgNr)
{
	FileD* FD1;
	if (!CFile->IsJournal && ((CFile == FD) || (CFile->OrigFD == FD))) return;
	SetMsgPar(CFile->Name, FD->Name);
	RunError(MsgNr);
}

std::string Compiler::RdHelpName()
{
	std::string s;
	if (CRdb->help_file == nullptr) Error(132);
	if (Lexem != _identifier) TestLex(_quotedstr);
	s = LexWord;
	RdLex();
	return s;
}

FrmlElem* Compiler::RdAttr()
{
	if (Lexem == '^') {
		BYTE n;
		RdLex();
		const char c = (char)(toupper(Rd1Char()) - 64);
		if (!screen.SetStyleAttr(c, n)) OldError(120);
		FrmlElem* z = new FrmlElemNumber(_const, 0, n);
		return z;
	}
	return RdRealFrml(nullptr);
}

void Compiler::RdW(WRectFrml& W)
{
	W.C1 = RdRealFrml(nullptr); Accept(',');
	W.R1 = RdRealFrml(nullptr); Accept(',');
	W.C2 = RdRealFrml(nullptr); Accept(',');
	W.R2 = RdRealFrml(nullptr);
}

void Compiler::RdFrame(FrmlElem** Z, BYTE& WFlags)
{
	if (Lexem != ',') return;
	RdLex();
	if (Lexem == '@') { WFlags = WFlags | WNoClrScr; RdLex(); }
	if (Lexem == '*') { WFlags = WFlags | WPushPixel; RdLex(); }
	if (!(Lexem == ',' || Lexem == ')' || Lexem == '!')) {
		WFlags = WFlags | WHasFrame;
		if (Lexem == _equ) {
			RdLex(); WFlags = WFlags | WDoubleFrame;
		}
		*Z = RdStrFrml(nullptr);
	}
	if (Lexem == '!') { WFlags = WFlags | WShadow; RdLex(); }
}

bool Compiler::PromptSortKeys(FieldListEl* FL, KeyFldD* SKRoot)
{
	wwmix ww;

	KeyFldD* SK;
	auto result = true;
	SKRoot = nullptr;
	while (FL != nullptr) {
		/* !!! with FL->FldD^ do!!! */
		if (FL->FldD->field_type != FieldType::TEXT) ww.PutSelect(FL->FldD->Name);
		FL = FL->pChain;
	}
	if (ss.Empty) return result;
	ss.AscDesc = true;
	ss.Subset = true;
	ww.SelectStr(0, 0, 25, "");
	if (Event.Pressed.KeyCombination() == __ESC) { return false; }
label1:
	LexWord = ww.GetSelect();
	if (LexWord != "") {
		SK = new KeyFldD();
		ChainLast(SKRoot, SK);
		SK->FldD = FindFldName(CFile);
		if (ss.Tag == '>') SK->Descend = true;
		if (SK->FldD->field_type == FieldType::ALFANUM) SK->CompLex = true;
		goto label1;
	}
	return result;
}

bool Compiler::PromptSortKeys(std::vector<FieldDescr*>& FL, KeyFldD* SKRoot)
{
	wwmix ww;

	KeyFldD* SK;
	auto result = true;
	SKRoot = nullptr;
	//while (FL != nullptr) {
	for (auto& fld : FL) {
		if (fld->field_type != FieldType::TEXT) ww.PutSelect(fld->Name);
	}
	if (ss.Empty) return result;
	ss.AscDesc = true;
	ss.Subset = true;
	ww.SelectStr(0, 0, 25, "");
	if (Event.Pressed.KeyCombination() == __ESC) { return false; }
label1:
	LexWord = ww.GetSelect();
	if (LexWord != "") {
		SK = new KeyFldD();
		ChainLast(SKRoot, SK);
		SK->FldD = FindFldName(CFile);
		if (ss.Tag == '>') SK->Descend = true;
		if (SK->FldD->field_type == FieldType::ALFANUM) SK->CompLex = true;
		goto label1;
	}
	return result;
}

void Compiler::RdAssignFrml(char FTyp, bool& Add, FrmlElem** Z, MergeReportBase* caller)
{
	char Typ = '\0';
	if (Lexem == _addass) { RdLex(); Add = true; }
	else Accept(_assign);
	*Z = RdFrml(Typ, caller);
	if ((FTyp != Typ) || Add && (Typ != 'R')) OldError(12);
}

bool Compiler::FldTypIdentity(FieldDescr* F1, FieldDescr* F2)
{
	bool result = false;
	if (F1->field_type != F2->field_type) return result;
	if ((F1->field_type == FieldType::FIXED) && (F1->M != F2->M)) return result;
	if ((F1->field_type == FieldType::NUMERIC || F1->field_type == FieldType::ALFANUM || F1->field_type == FieldType::FIXED) 
		&& (F1->L != F2->L)) return result;
	return true;
}

void Compiler::RdFldList(FieldListEl** FLRoot)
{
	FieldDescr* F = nullptr;
	FieldListEl* FL = nullptr;
label1:
	F = RdFldName(CFile);
	FL = new FieldListEl();
	FL->FldD = F;
	if (*FLRoot == nullptr) *FLRoot = FL;
	else ChainLast(*FLRoot, FL);
	if (Lexem == ',') { RdLex(); goto label1; }
}

void Compiler::RdFldList(std::vector<FieldDescr*>& vFields)
{
	while (true) {
		FieldDescr* F = RdFldName(CFile);
		vFields.push_back(F);
		if (Lexem == ',') {
			RdLex();
			continue;
		}
		break;
	}
}

void Compiler::RdFldList(std::vector<FieldDescr*>* vFields)
{
	while (true) {
		FieldDescr* F = RdFldName(CFile);
		vFields->push_back(F);
		if (Lexem == ',') {
			RdLex();
			continue;
		}
		break;
	}
}

void Compiler::RdNegFldList(bool& Neg, std::vector<FieldDescr*>& vFields)
{
	if (Lexem == '^') { RdLex(); Neg = true; }
	Accept('(');
	if (Lexem == ')') Neg = true;
	else RdFldList(vFields);
	Accept(')');
}

void Compiler::RdNegFldList(bool& Neg, std::vector<FieldDescr*>* vFields)
{
	if (Lexem == '^') { RdLex(); Neg = true; }
	Accept('(');
	if (Lexem == ')') Neg = true;
	else RdFldList(vFields);
	Accept(')');
}

XKey* Compiler::RdViewKey()
{
	XKey* lastK = nullptr;
	LocVar* lv = nullptr;
	std::string s;
	XKey* result = nullptr;
	if (Lexem != '/') return result;
	RdLex();
	size_t i = 0;

	if (Lexem == '@') {
		lastK = CFile->Keys.empty() ? nullptr : CFile->Keys[0];
		goto label1;
	}
	TestIdentif();

	for (auto& k : CFile->Keys) {
		std::string lw = LexWord;
		if (EquUpCase(k->Alias, lw)) {
			lastK = k;
			goto label1;
		}
	}
	s = LexWord;
	i = s.find('_');
	if (i != std::string::npos) s = copy(s, i + 2, 255);
	s = CFile->Name + "_" + s;

	for (auto& k : CFile->Keys) {
		std::string kAl = k->Alias;
		if (EquUpCase(s, kAl)) {
			lastK = k;
			goto label1;
		}
	}

	if (IdxLocVarAllowed && FindLocVar(&LVBD, &lv) && (lv->FTyp == 'i'))
	{
		if (lv->FD != CFile) Error(164);
		lastK = (XKey*)(lv->record);
		goto label1;
	}
	Error(109);
label1:
	if (CFile->FF->file_type != FileType::INDEX)
#ifdef FandSQL
		if (CFile->typSQLFile) Error(24); else
#endif
			Error(108);
	RdLex();
	result = lastK;
	return result;
}

void Compiler::SrchF(FieldDescr* F)
{
	if (F == KeyArgFld) {
		KeyArgFound = true;
		return;
	}
	if (!F->isStored()) {
		SrchZ(F->Frml);
	}
}

void Compiler::SrchZ(FrmlElem* Z)
{
	KeyFldD* KF = nullptr;
	FrmlListEl* fl = nullptr;
	if (Z == nullptr) return;

	auto iZ0 = (FrmlElemFunction*)Z;
	auto iZ7 = (FrmlElem7*)Z;
	auto iZ19 = (FrmlElem19*)Z;

	switch (Z->Op) {
	case _field: {
		SrchF(iZ7->Field);
		break;
	}
	case _access: {
		if (iZ7->LD != nullptr) {
			//KF = iZ7->LD->Args;
			//while (KF != nullptr) {
			for (auto& KF : iZ7->LD->Args) {
				SrchF(KF->FldD);
				//KF = KF->pChain;
			}
		}
		break;
	}
	case _userfunc: {
		fl = iZ19->FrmlL;
		while (fl != nullptr) {
			SrchZ(fl->Frml);
			fl = fl->pChain;
		}
		break;
	}
	default: {
		if (Z->Op >= 0x60 && Z->Op <= 0xAF) SrchZ(iZ0->P1); /*1-ary*/
		else if (Z->Op >= 0xB0 && Z->Op <= 0xEF) /*2-ary*/
		{
			SrchZ(iZ0->P1);
			SrchZ(iZ0->P2);
		}
		else if (Z->Op >= 0xB0 && Z->Op <= 0xEF) /*3-ary*/
		{
			SrchZ(iZ0->P1);
			SrchZ(iZ0->P2);
			SrchZ(iZ0->P3);
		}
		break;
	}
	}
}

bool Compiler::IsFun(std::map<std::string, int>& strs, std::string input, instr_type& FunCode)
{
	// prevedeme vse ze vstupu na mala pismena
	for (auto&& c : input)
	{
		c = tolower(c);
	}
	auto it = strs.find(input);
	if (it != strs.end()) {
		FunCode = (instr_type)it->second;
		RdLex();
		return true;
	}
	return false;
}

bool Compiler::IsKeyArg(FieldDescr* F, FileD* FD)
{
	KeyArgFound = false;
	for (auto& k : FD->Keys) {
		KeyArgFld = F;
		KeyFldD* kf = k->KFlds;
		while (kf != nullptr) {
			SrchF(kf->FldD);
			if (KeyArgFound) { return true; }
			kf = kf->pChain;
		}
	}
	return false;
}

void Compiler::CompileRecLen(FileD* file_d)
{
	WORD l = 0;
	WORD n = 0;
	if ((file_d->FF->file_type == FileType::INDEX || file_d->FF->file_type == FileType::DBF)) {
		l = 1;
	}
	for (auto& F : file_d->FldD) {
		switch (file_d->FF->file_type) {
		case FileType::FAND8: {
			if (F->field_type == FieldType::DATE) F->NBytes = 2;
			break;
		}
		case FileType::DBF: {
			switch (F->field_type) {
			case FieldType::FIXED: {
				F->NBytes = F->L - 1;
				break;
			}
			case FieldType::DATE: {
				F->NBytes = 8;
				break;
			}
			case FieldType::TEXT: {
				F->NBytes = 10;
				break;
			}
			}
			break;
		}
		}
		if (F->isStored()) {
			F->Displ = l;
			l += F->NBytes;
			n++;
		}
	}
	file_d->FF->RecLen = l;
	switch (file_d->FF->file_type) {
	case FileType::FAND8: {
		file_d->FF->FirstRecPos = 4;
		break;
	}
	case FileType::DBF: {
		file_d->FF->FirstRecPos = (n + 1) * 32 + 1;
		break;
	}
	default: {
		file_d->FF->FirstRecPos = 6;
		break;
	}
	}
}

void Compiler::TestBool(char FTyp)
{
	if (FTyp != 'B') OldError(18);
}

void Compiler::TestString(char FTyp)
{
	if (FTyp != 'S') OldError(19);
}

void Compiler::TestReal(char FTyp)
{
	if (FTyp != 'R') OldError(20);
}

FrmlElem* Compiler::BOperation(char Typ, instr_type Fun, FrmlElem* Frml)
{
	TestBool(Typ);
	FrmlElemFunction* Z = new FrmlElemFunction(Fun, 0); // GetOp(Fun, 0);
	RdLex();
	Z->P1 = Frml;
	return Z;
}

FrmlElem* Compiler::RdMult(char& FTyp, MergeReportBase* caller)
{
	WORD N = 0;
	FrmlElemFunction* Z = (FrmlElemFunction*)RdPrim(FTyp, caller);
label1:
	FrmlElemFunction* Z1 = Z;
	switch (Lexem) {
	case '*': {
		Z = new FrmlElemFunction(_times, 0); // GetOp(_times, 0);
		goto label2;
		break;
	}
	case  '/': {
		Z = new FrmlElemFunction(_divide, 0); // GetOp(_divide, 0);
	label2:
		TestReal(FTyp);
		RdLex();
		Z->P1 = Z1;
		Z->P2 = RdPrim(FTyp, caller);
		TestReal(FTyp);
		goto label1;
		break;
	}
	case _identifier: {
		if (EquUpCase(QQdiv, LexWord)) {
			Z = new FrmlElemFunction(_div, 0); /*GetOp(_div, 0);*/
			goto label2;
		}
		else if (EquUpCase(QQmod, LexWord)) {
			Z = new FrmlElemFunction(_mod, 0); /*GetOp(_mod, 0);*/
			goto label2;
		}
		else if (EquUpCase(QQround, LexWord)) {
			TestReal(FTyp);
			Z = new FrmlElemFunction(_round, 0); /*GetOp(_round, 0);*/
			RdLex();
			Z->P1 = Z1;
			Z->P2 = RdPrim(FTyp, caller);
			TestReal(FTyp);
		}
		break;
	}
	}
	return Z;
}

FrmlElem* Compiler::RdAdd(char& FTyp, MergeReportBase* caller)
{
	FrmlElemFunction* Z = (FrmlElemFunction*)RdMult(FTyp, caller);
	FrmlElemFunction* Z1 = nullptr;
label1:
	switch (Lexem) {
	case '+': {
		Z1 = Z;
		if (FTyp == 'R') {
			Z = new FrmlElemFunction(_plus, 0); /*GetOp(_plus, 0);*/
			goto label2;
		}
		else {
			Z = new FrmlElemFunction(_concat, 0); // GetOp(_concat, 0);
			TestString(FTyp);
			RdLex();
			Z->P1 = Z1;
			Z->P2 = RdMult(FTyp, caller);
			TestString(FTyp);
			goto label1;
		}
		break;
	}
	case '-': {
		Z1 = Z;
		Z = new FrmlElemFunction(_minus, 0); // GetOp(_minus, 0);
		TestReal(FTyp);
	label2:
		RdLex();
		Z->P1 = Z1;
		Z->P2 = RdMult(FTyp, caller);
		TestReal(FTyp);
		goto label1;
		break;
	}
	}
	return Z;
}

WORD Compiler::RdTilde()
{
	if (Lexem == '~') {
		RdLex();
		return 1;
	}
	return 0;
}

void Compiler::RdInConst(FrmlElemIn* Z, char& FTyp, std::string& str, double& R)
{
	if (FTyp == 'S') {
		if (Z->param1 == 1/*tilde*/) str = OldTrailChar(' ', LexWord);
		else str = LexWord;
		Accept(_quotedstr);
	}
	else {
		R = RdRealConst();
	}
}

FrmlElem* Compiler::RdComp(char& FTyp, MergeReportBase* caller)
{
	pstring S;
	BYTE* B = nullptr;
	short N = 0;
	FrmlElem* Z1 = nullptr;
	FrmlElem* Z = RdAdd(FTyp, caller);
	Z1 = Z;
	if (Lexem >= _equ && Lexem <= _ne)
		if (FTyp == 'R') {
			Z = new FrmlElemFunction(_compreal, 2); // GetOp(_compreal, 2);
			auto iZ0 = (FrmlElemFunction*)Z;
			iZ0->P1 = Z1;
			iZ0->N21 = Lexem;
			RdLex();
			iZ0->N22 = RdPrecision();
			iZ0->P2 = RdAdd(FTyp, caller);
			TestReal(FTyp);
			FTyp = 'B';
		}
		else {
			TestString(FTyp);
			Z = new FrmlElemFunction(_compstr, 2); // GetOp(_compstr, 2);
			auto iZ0 = (FrmlElemFunction*)Z;
			iZ0->P1 = Z1;
			iZ0->N21 = Lexem;
			RdLex();
			iZ0->N22 = RdTilde();
			iZ0->P2 = RdAdd(FTyp, caller);
			TestString(FTyp);
			FTyp = 'B';
		}
	else if ((Lexem == _identifier) && IsKeyWord("IN"))
	{
		FrmlElemIn* zIn = nullptr;
		if (FTyp == 'R') {
			zIn = new FrmlElemIn(_inreal); // GetOp(_inreal, 1);
			zIn->param1 = RdPrecision();
		}
		else {
			TestString(FTyp);
			zIn = new FrmlElemIn(_instr); // GetOp(_instr, 1);
			zIn->param1 = RdTilde();
		}
		zIn->P1 = Z1;
		Accept('[');
	label1:
		std::string str;
		double R = 0.0;
		RdInConst(zIn, FTyp, str, R);
		if (Lexem != _subrange) {
			// jde o samostatnou hodnotu
			if (FTyp == 'S') zIn->strings.push_back(str);
			else zIn->reals.push_back(R);
		}
		else {
			std::string f_str;
			double f_double;
			// jde o rozsah
			if (FTyp == 'S') { f_str = str; }
			else { f_double = R; }
			RdLex();
			RdInConst(zIn, FTyp, str, R);
			if (FTyp == 'S') {
				zIn->strings_range.push_back(std::pair<std::string, std::string>(f_str, str));
			}
			else {
				zIn->reals_range.push_back(std::pair<double, double>(f_double, R));
			}
		}
		if (Lexem != ']') { Accept(','); goto label1; }
		RdLex();
		FTyp = 'B';
		Z = zIn;
	}
	return Z;
}


FrmlElem* Compiler::RdBAnd(char& FTyp, MergeReportBase* caller)
{
	FrmlElem* Z = RdComp(FTyp, caller);
	while (Lexem == '&') {
		Z = BOperation(FTyp, _and, Z);
		((FrmlElemFunction*)Z)->P2 = RdComp(FTyp, caller);
		TestBool(FTyp);
	}
	return Z;
}

FrmlElem* Compiler::RdBOr(char& FTyp, MergeReportBase* caller)
{
	FrmlElem* Z = RdBAnd(FTyp, caller);
	while (Lexem == '|')
	{
		Z = BOperation(FTyp, _or, Z);
		((FrmlElemFunction*)Z)->P2 = RdBAnd(FTyp, caller);
		TestBool(FTyp);
	}
	return Z;
}

FrmlElem* Compiler::RdFormula(char& FTyp, MergeReportBase* caller)
{
	FrmlElem* Z = RdBOr(FTyp, caller);
	while ((BYTE)Lexem == _limpl || (BYTE)Lexem == _lequ) {
		Z = BOperation(FTyp, (instr_type)Lexem, Z);
		((FrmlElemFunction*)Z)->P2 = RdBOr(FTyp, caller);
		TestBool(FTyp);
	}
	return Z;
}

bool Compiler::FindFuncD(FrmlElem** ZZ, MergeReportBase* caller)
{
	char typ = '\0';
	FuncD* fc = FuncDRoot;
	while (fc != nullptr) {
		if (EquUpCase(fc->Name, LexWord)) {
			RdLex(); RdLex();
			FrmlElem19* z = new FrmlElem19(_userfunc, 8); // GetOp(_userfunc, 8);
			z->FC = fc;
			WORD n = fc->LVB.NParam;
			auto itr = fc->LVB.vLocVar.begin();
			for (WORD i = 1; i <= n; i++) {
				FrmlListEl* fl = new FrmlListEl();
				if (z->FrmlL == nullptr) z->FrmlL = fl;
				else ChainLast(z->FrmlL, fl);
				fl->Frml = RdFormula(typ, caller);
				if (typ != (*itr++)->FTyp) OldError(12);
				if (i < n) Accept(',');
			}
			Accept(')');
			*ZZ = z;
			return true;
		}
		fc = fc->Chain;
	}
	return false;
}

std::map<std::string, int> R0Fun = {
	std::pair<std::string, int> {"cprinter", _cprinter},
	std::pair<std::string, int> {"currtime", _currtime},
	std::pair<std::string, int> {"edrecno", _edrecno},
	std::pair<std::string, int> {"exitcode", _exitcode},
	std::pair<std::string, int> {"getmaxx", _getmaxx},
	std::pair<std::string, int> {"getmaxy", _getmaxy},
	std::pair<std::string, int> {"maxcol", _maxcol},
	std::pair<std::string, int> {"maxrow", _maxrow},
	std::pair<std::string, int> {"memavail", _memavail},
	std::pair<std::string, int> {"mousex", _mousex},
	std::pair<std::string, int> {"mousey", _mousey},
	std::pair<std::string, int> {"pi", _pi},
	std::pair<std::string, int> {"random", _random},
	std::pair<std::string, int> {"today", _today},
	std::pair<std::string, int> {"txtpos", _txtpos},
	std::pair<std::string, int> {"txtxy", _txtxy},
};

std::map<std::string, int> RCFun = {
	std::pair<std::string, int> {"edbreak", 3},
	std::pair<std::string, int> {"edirec", 4},
	std::pair<std::string, int> {"menux", 5},
	std::pair<std::string, int> {"menuy", 6},
	std::pair<std::string, int> {"usercode", 7},
};

std::map<std::string, int> S0Fun = {
	std::pair<std::string, int> {"accright", _accright},
	std::pair<std::string, int> {"clipbd", _clipbd},
	std::pair<std::string, int> {"edbool", _edbool},
	std::pair<std::string, int> {"edfield", _edfield},
	std::pair<std::string, int> {"edfile", _edfile},
	std::pair<std::string, int> {"edkey", _edkey},
	std::pair<std::string, int> {"edreckey", _edreckey},
	std::pair<std::string, int> {"keybuf", _keybuf},
	std::pair<std::string, int> {"password", _password},
	std::pair<std::string, int> {"readkey", _readkey},
	std::pair<std::string, int> {"username", _username},
	std::pair<std::string, int> {"version", _version},
};

std::map<std::string, int> B0Fun = {
	std::pair<std::string, int> {"isnewrec", _isnewrec},
	std::pair<std::string, int> {"testmode", _testmode},
};

std::map<std::string, int> S1Fun = {
	std::pair<std::string, int> {"char", _char},
	std::pair<std::string, int> {"getenv", _getenv},
	std::pair<std::string, int> {"lowcase", _lowcase},
	std::pair<std::string, int> {"nodiakr", _nodiakr},
	std::pair<std::string, int> {"upcase", _upcase},
};

std::map<std::string, int> R1Fun = {
	std::pair<std::string, int> {"abs", _abs},
	std::pair<std::string, int> {"arctan", _arctan},
	std::pair<std::string, int> {"color", _color},
	std::pair<std::string, int> {"cos", _cos},
	std::pair<std::string, int> {"exp", _exp},
	std::pair<std::string, int> {"frac", _frac},
	std::pair<std::string, int> {"int", _int},
	std::pair<std::string, int> {"ln", _ln},
	std::pair<std::string, int> {"sin", _sin},
	std::pair<std::string, int> {"sqr", _sqr},
	std::pair<std::string, int> {"sqrt", _sqrt},
	std::pair<std::string, int> {"typeday", _typeday},
};

std::map<std::string, int> R2Fun = {
	std::pair<std::string, int> {"addmonth", _addmonth},
	std::pair<std::string, int> {"addwdays", _addwdays},
	std::pair<std::string, int> {"difmonth", _difmonth},
	std::pair<std::string, int> {"difwdays", _difwdays},
};

std::map<std::string, int> RS1Fun = {
	std::pair<std::string, int> {"diskfree", _diskfree},
	std::pair<std::string, int> {"length", _length},
	std::pair<std::string, int> {"linecnt", _linecnt},
	std::pair<std::string, int> {"ord", _ord},
	std::pair<std::string, int> {"val", _val},
};

std::map<std::string, int> S3Fun = {
	std::pair<std::string, int> {"copy", _copy},
	std::pair<std::string, int> {"str", _str},
	std::pair<std::string, int> {"text", _str},
};

FrmlElem* Compiler::RdPrim(char& FTyp, MergeReportBase* caller)
{
	instr_type FunCode = _notdefined;
	FrmlElem* Z = nullptr; FrmlElem* Z1 = nullptr; FrmlElem* Z2 = nullptr; FrmlElem* Z3 = nullptr;
	char Typ = '\0';
	short I = 0, N = 0; BYTE* B = nullptr;
	//pstring Options;

	switch (Lexem) {
	case _identifier: {
		SkipBlank(false);
		if (IsFun(R0Fun, LexWord, FunCode)) {
			Z = new FrmlElemFunction(FunCode, 0); // GetOp(FunCode, 0);
			FTyp = 'R';
		}
		else if (IsFun(RCFun, LexWord, FunCode)) {
			Z = new FrmlElem1(_getWORDvar, 1); // GetOp(_getWORDvar, 1);
			((FrmlElem1*)Z)->N01 = FunCode;
			FTyp = 'R';
		}
		else if (IsFun(S0Fun, LexWord, FunCode)) {
			Z = new FrmlElemFunction(FunCode, 0); // GetOp(FunCode, 0);
			FTyp = 'S';
		}
		else if (IsFun(B0Fun, LexWord, FunCode)) {
			Z = new FrmlElemFunction(FunCode, 0); // GetOp(FunCode, 0);
			FTyp = 'B';
		}
		else if (IsKeyWord("TRUE")) {
			Z = new FrmlElemBool(_const, 1, true); // GetOp(_const, 1);
			//Z->B = true;
			FTyp = 'B';
		}
		else if (IsKeyWord("FALSE")) {
			Z = new FrmlElemBool(_const, 1, false); // GetOp(_const, 1);
			FTyp = 'B';
		}
		else if (!EquUpCase("OWNED", LexWord) && (ForwChar == '(')) {
			if (FindFuncD(&Z, caller)) FTyp = ((FrmlElem19*)Z)->FC->FTyp;
			else if (IsFun(S3Fun, LexWord, FunCode)) {
				RdLex();
				Z = new FrmlElemFunction(FunCode, 0); // GetOp(FunCode, 0);
				((FrmlElemFunction*)Z)->P1 = RdAdd(FTyp, caller);
				if ((BYTE)FunCode == _copy) TestString(FTyp);
				else {
					TestReal(FTyp);
					FTyp = 'S';
				}
				Accept(',');
				((FrmlElemFunction*)Z)->P2 = RdAdd(Typ, caller);
				if (((BYTE)FunCode == _str) && (Typ == 'S')) goto label0;
				TestReal(Typ);
				Accept(',');
				((FrmlElemFunction*)Z)->P3 = RdAdd(Typ, caller);
				TestReal(Typ);
			label0:
				Accept(')');
			}
			else if (IsKeyWord("COND"))
			{
				RdLex();
				Z2 = nullptr;
			label1:
				Z = new FrmlElemFunction(_cond, 0); // GetOp(_cond, 0);
				if (!IsKeyWord("ELSE"))
				{
					((FrmlElemFunction*)Z)->P1 = RdFormula(Typ, caller);
					TestBool(Typ);
				}
				Accept(':');
				((FrmlElemFunction*)Z)->P2 = RdAdd(Typ, caller);
				if (Z2 == nullptr)
				{
					Z1 = Z;
					FTyp = Typ;
					if (Typ == 'B') OldError(70);
				}
				else {
					((FrmlElemFunction*)Z2)->P3 = Z;
					if (FTyp == 'S') TestString(Typ);
					else TestReal(Typ);
				}
				if ((((FrmlElemFunction*)Z)->P1 != nullptr) && (Lexem == ','))
				{
					RdLex();
					Z2 = Z;
					goto label1;
				}
				Accept(')');
				Z = Z1;
			}
			else if (IsKeyWord("MODULO"))	{
				RdLex();
				Z = new FrmlElemFunction(_modulo, 2); // GetOp(_modulo, 2);
				((FrmlElemFunction*)Z)->P1 = RdAdd(Typ, caller);
				TestString(Typ);
				N = 0;
				do {
					Accept(',');
					((FrmlElemFunction*)Z)->vValues.push_back(RdInteger());
					N++;
				} while (Lexem == ',');
				Accept(')');
				//((FrmlElemFunction*)Z)->W11 = N;
				FTyp = 'B';
			}
			else if (IsKeyWord("SUM")) {
				RdLex();
				if (FrmlSumEl != nullptr) OldError(74);
				if (!caller->ChainSum) Error(28);
				FrmlSumEl = new std::vector<FrmlElemSum*>();
				FrstSumVar = true;
				FrmlElemSum* f = new FrmlElemSum(_const, 0.0, RdAdd(FTyp, caller));
				FrmlSumEl->push_back(f);
				TestReal(FTyp);
				Accept(')');
				Z = (FrmlElem*)f;
				caller->ChainSumEl();
				FrmlSumEl = nullptr; // TODO: toto by se melo smazat, ale vyuziva ho v urcitych pripadech ChainSumEl() -> nutno upravit
			}
			else if (IsKeyWord("DTEXT"))
			{
				RdLex();
				Z1 = RdAdd(Typ, caller);
				TestReal(Typ);
				Accept(',');
				TestLex(_identifier);
				for (I = 1; I <= LexWord.length(); I++) {
					LexWord[I] = toupper(LexWord[I]);
				}
				goto label2;
			}
			else if (IsKeyWord("STRDATE"))
			{
				RdLex();
				Z1 = RdAdd(Typ, caller);
				TestReal(Typ);
				Accept(',');
				TestLex(_quotedstr);
			label2:
				Z = new FrmlElem6(_strdate1, 0); // GetOp(_strdate1, LexWord.length() + 1);
				auto iZ = (FrmlElem6*)Z;
				iZ->P1 = Z1;
				iZ->Mask = LexWord;
				RdLex();
				Accept(')');
				FTyp = 'S';
			}
			else if (IsKeyWord("DATE"))
			{
				RdLex();
				Z = new FrmlElem6(_valdate, 9); // GetOp(_valdate, 9);
				auto iZ = (FrmlElem6*)Z;
				iZ->Mask = "DD.MM.YY";
				goto label3;
			}
			else if (IsKeyWord("VALDATE"))
			{
				RdLex();
				Z1 = RdAdd(Typ, caller);
				TestString(Typ);
				Accept(',');
				TestLex(_quotedstr);
				Z = new FrmlElem6(_valdate, 0); // GetOp(_valdate, LexWord.length() + 1);
				auto iZ = (FrmlElem6*)Z;
				iZ->P1 = Z1;
				iZ->Mask = LexWord;
				RdLex();
				goto label4;
			}
			else if (IsKeyWord("REPLACE")) {
				RdLex();
				Z1 = RdAdd(Typ, caller);
				TestString(Typ);
				Accept(',');
				Z2 = RdAdd(Typ, caller);
				TestString(Typ);
				Accept(',');
				Z3 = RdAdd(FTyp, caller);
				TestString(FTyp);
				FunCode = _replace;
				goto label8;
			}
			else if (IsKeyWord("POS")) {
				RdLex();
				Z1 = RdAdd(Typ, caller);
				TestString(Typ);
				Accept(',');
				Z2 = RdAdd(Typ, caller);
				TestString(Typ);
				Z3 = nullptr;
				FunCode = _pos;
				FTyp = 'R';
			label8:
				//Options = "";
				Z = new FrmlElemPosReplace(FunCode, 0);
				FrmlElemPosReplace* iZ = (FrmlElemPosReplace*)Z;
				if (Lexem == ',') {
					RdLex();
					if (Lexem != ',') {
						TestLex(_quotedstr);
						iZ->Options = LexWord;
						RdLex();
					}
					if (((BYTE)FunCode == _pos) && (Lexem == ',')) {
						// which occurrence - count (kolikaty vyskyt)
						RdLex();
						Z3 = RdAdd(Typ, caller);
						TestReal(Typ);
					}
				}
				iZ->P1 = Z1; iZ->P2 = Z2; iZ->P3 = Z3;
				Accept(')');
			}
			else if (IsFun(RS1Fun, LexWord, FunCode)) {
				RdLex();
				Z = new FrmlElemFunction(FunCode, 0); // GetOp(FunCode, 0);
			label3:
				((FrmlElemFunction*)Z)->P1 = RdAdd(Typ, caller);
				TestString(Typ);
				goto label4;
			}
			else if (IsFun(R1Fun, LexWord, FunCode))
			{
				RdLex();
				Z = new FrmlElemFunction(FunCode, 0); // GetOp(FunCode, 0);
				((FrmlElemFunction*)Z)->P1 = RdAdd(Typ, caller);
				TestReal(Typ);
			label4:
				FTyp = 'R';
				Accept(')');
			}
			else if (IsFun(R2Fun, LexWord, FunCode))
			{
				RdLex();
				Z = new FrmlElemFunction(FunCode, 1); // GetOp(FunCode, 1);
				((FrmlElemFunction*)Z)->P1 = RdAdd(Typ, caller);
				TestReal(Typ);
				Accept(',');
				((FrmlElemFunction*)Z)->P2 = RdAdd(Typ, caller);
				TestReal(Typ);
				if ((Z->Op == _addwdays || Z->Op == _difwdays) && (Lexem == ','))
				{
					RdLex();
					((FrmlElemFunction*)Z)->N21 = RdInteger();
					if (((FrmlElemFunction*)Z)->N21 > 3) OldError(136);
				}
				goto label4;
			}
			else if (IsKeyWord("LEADCHAR"))
			{
				Z = new FrmlElemFunction(_leadchar, 2); // GetOp(_leadchar, 2);
				goto label5;
			}
			else if (IsKeyWord("TRAILCHAR"))
			{
				Z = new FrmlElemFunction(_trailchar, 2); // GetOp(_trailchar, 2);
			label5:
				RdLex();
				((FrmlElemFunction*)Z)->N11 = RdQuotedChar();
				Accept(',');
				((FrmlElemFunction*)Z)->P1 = RdAdd(Typ, caller);
				TestString(Typ);
				if (Lexem == ',') {
					RdLex();
					((FrmlElemFunction*)Z)->N12 = RdQuotedChar();
				}
				goto label6;
			}
			else if (IsKeyWord("COPYLINE"))
			{
				Z = new FrmlElemFunction(_copyline, 0); // GetOp(_copyline, 0);
				goto label7;
			}
			else if (IsKeyWord("REPEATSTR"))
			{
				Z = new FrmlElemFunction(_repeatstr, 0); // GetOp(_repeatstr, 0);
			label7:
				RdLex();
				((FrmlElemFunction*)Z)->P1 = RdAdd(Typ, caller);
				TestString(Typ);
				Accept(',');
				((FrmlElemFunction*)Z)->P2 = RdAdd(Typ, caller);
				TestReal(Typ);
				if ((Lexem == ',') && (Z->Op == _copyline)) {
					RdLex();
					((FrmlElemFunction*)Z)->P3 = RdAdd(Typ, caller);
					TestReal(Typ);
				}
				goto label6;
			}
			else if (IsFun(S1Fun, LexWord, FunCode))
			{
				Z = new FrmlElemFunction(FunCode, 0); // GetOp(FunCode, 0);
				RdLex();
				((FrmlElemFunction*)Z)->P1 = RdAdd(FTyp, caller);
				if (FunCode == _char) TestReal(FTyp);
				else TestString(FTyp);
			label6:
				FTyp = 'S';
				Accept(')');
			}
			else if (IsKeyWord("TRUST")) {
				Z = new FrmlElemFunction(_trust, 0); // GetOp(_trust, 0);
				RdByteListInStore();
				FTyp = 'B';
			}
			else if (IsKeyWord("EQUMASK")) {
				Z = new FrmlElemFunction(_equmask, 0); // GetOp(_equmask, 0);
				FTyp = 'B';
				RdLex();
				((FrmlElemFunction*)Z)->P1 = RdAdd(Typ, caller);
				TestString(Typ);
				Accept(',');
				((FrmlElemFunction*)Z)->P2 = RdAdd(Typ, caller);
				TestString(Typ);
				Accept(')');
			}
			else if (RdFunction != nullptr) Z = RdFunction(FTyp);
			else Error(75);
		}
		else {
			if (ptrRdFldNameFrml == nullptr && caller == nullptr) Error(110);
			else {
				if (ptrRdFldNameFrml != nullptr) {
					Z = ptrRdFldNameFrml(FTyp, caller); // volani ukazatele na funkci
				}
				else {
					Z = caller->RdFldNameFrml(FTyp); // volani ukazatele na funkci
				}
				if (Z == nullptr) return nullptr;
				if ((Z->Op != _access) || (((FrmlElem7*)Z)->LD != nullptr)) FrstSumVar = false;
			}
		}
		break;
	}
	case '^': {
		RdLex();
		Z = new FrmlElemFunction(_lneg, 0); // GetOp(_lneg, 0);
		((FrmlElemFunction*)Z)->P1 = RdPrim(FTyp, caller);
		TestBool(FTyp);
		break;
	}
	case '(': {
		RdLex();
		Z = RdFormula(FTyp, caller);
		Accept(')');
		break;
	}
	case '-': {
		RdLex();
		if (Lexem == '-') Error(7);
		Z = new FrmlElemFunction(_unminus, 0); // GetOp(_unminus, 0);
		((FrmlElemFunction*)Z)->P1 = RdPrim(FTyp, caller);
		TestReal(FTyp);
		break;
	}
	case '+': {
		RdLex();
		if (Lexem == '+') Error(7);
		Z = RdPrim(FTyp, caller);
		TestReal(FTyp);
		break;
	}
	case _quotedstr: {
		Z = new FrmlElemString(_const, 0); // GetOp(_const, LexWord.length() + 1);
		FTyp = 'S';
		((FrmlElemString*)Z)->S = LexWord;
		RdLex();
		break;
	}
	default: {
		FTyp = 'R';
		Z = new FrmlElemNumber(_const, 0); // GetOp(_const, sizeof(double));
		((FrmlElemNumber*)Z)->R = RdRealConst();
		break;
	}
	}
	return Z;
}

WORD Compiler::RdPrecision()
{
	WORD n = 5;
	if ((Lexem == '.') && (ForwChar >= '0' && ForwChar <= '9'))
	{
		RdLex(); n = RdInteger();
		if (n > 10) OldError(21);
	}
	return n;
}

FrmlListEl* Compiler::RdFL(bool NewMyBP, FrmlList FL1)
{
	char FTyp = '\0';
	KeyFldD* KF = CViewKey->KFlds;
	FrmlListEl* FLRoot = nullptr;
	KeyFldD* KF2 = KF->pChain;
	bool FVA = FileVarsAllowed;
	FileVarsAllowed = false;
	bool b = FL1 != nullptr;
	if (KF2 != nullptr) Accept('(');
label1:
	FrmlList FL = new FrmlListEl(); // (FrmlListEl*)GetStore(sizeof(*FL));
	if (FLRoot == nullptr) FLRoot = FL;
	else ChainLast(FLRoot, FL);
	FL->Frml = RdFrml(FTyp, nullptr); // MyBPContext(RdFrml(FTyp), NewMyBP);
	if (FTyp != KF->FldD->frml_type) OldError(12);
	KF = KF->pChain;
	if (b) {
		FL1 = FL1->pChain;
		if (FL1 != nullptr) { Accept(','); goto label1; }
	}
	else if ((KF != nullptr) && (Lexem == ',')) { RdLex(); goto label1; }
	if (KF2 != nullptr) Accept(')');
	auto result = FLRoot;
	FileVarsAllowed = FVA;
	return result;
}

FrmlElem* Compiler::RdKeyInBool(KeyInD** KIRoot, bool NewMyBP, bool FromRdProc, bool& SQLFilter, MergeReportBase* caller)
{
	KeyInD* KI = nullptr; WORD l = 0; char FTyp = '\0';
	FrmlElem* Z = nullptr; bool FVA = false;
	FrmlElem* result = nullptr;
	*KIRoot = nullptr; SQLFilter = false;

	if (FromRdProc) {
		FVA = FileVarsAllowed;
		FileVarsAllowed = true;
		if ((Lexem == _identifier)
			&& (ForwChar == '(')
			&& (EquUpCase("EVALB", LexWord) || EquUpCase("EVALS", LexWord) || EquUpCase("EVALR", LexWord)))
			FileVarsAllowed = false;
	}
	if (IsKeyWord("KEY")) {
		AcceptKeyWord("IN");
		if ((CFile->FF->file_type != FileType::INDEX) || (CViewKey == nullptr)) OldError(118);
		if (CViewKey->KFlds == nullptr) OldError(176);
		Accept('[');
		l = CViewKey->IndexLen + 1;
	label1:
		KI = new KeyInD();
		if (*KIRoot == nullptr) *KIRoot = KI;
		else ChainLast(*KIRoot, KI);
		KI->FL1 = RdFL(NewMyBP, nullptr);
		if (Lexem == _subrange) {
			RdLex();
			KI->FL2 = RdFL(NewMyBP, KI->FL1);
		}
		if (Lexem == ',') { RdLex(); goto label1; }
		Accept(']');
		if (Lexem == '&') { RdLex(); goto label2; }
	}
	else {
	label2:
		FrmlSumEl = nullptr;
		Z = RdFormula(FTyp, caller);
		if (CFile->typSQLFile && (FTyp == 'S')) SQLFilter = true;
		else {
			TestBool(FTyp);
			if (Z->Op == _eval) ((FrmlElem21*)Z)->EvalFD = CFile;
		}
		result = Z; // MyBPContext(Z, NewMyBP && (Z->Op != _eval));
	}
	if (FromRdProc) FileVarsAllowed = FVA;
	return result;
}

FrmlElem* Compiler::RdFrml(char& FTyp, MergeReportBase* caller)
{
	FrmlSumEl = nullptr;
	return RdFormula(FTyp, caller);
}

FrmlElem* Compiler::RdBool(MergeReportBase* caller)
{
	char FTyp = 0;
	FrmlSumEl = nullptr;
	auto result = RdFormula(FTyp, caller);
	TestBool(FTyp);
	return result;
}

FrmlElem* Compiler::RdRealFrml(MergeReportBase* caller)
{
	char FTyp = 0;
	FrmlSumEl = nullptr;
	auto result = RdAdd(FTyp, caller);
	TestReal(FTyp);
	return result;
}

FrmlElem* Compiler::RdStrFrml(MergeReportBase* caller)
{
	char FTyp = 0;
	FrmlSumEl = nullptr;
	auto result = RdAdd(FTyp, caller);
	TestString(FTyp);
	return result;
}

FieldDescr* Compiler::FindFldName(FileD* FD, std::string fieldName)
{
	if (fieldName.empty()) fieldName = LexWord;
	if (FD != nullptr) {
		for (auto& i : FD->FldD) {
			if (EquUpCase(i->Name, fieldName)) return i;
		}
	}
	return nullptr;
}

FieldDescr* Compiler::RdFldName(FileD* FD)
{
	FieldDescr* F = FindFldName(FD);
	TestIdentif();
	if (F == nullptr) {
		SetMsgPar(LexWord, FD->Name);
		Error(87);
	}
	RdLex();
	return F;
}

FileD* Compiler::FindFileD()
{
	FileD* FD = nullptr;
	RdbD* R = nullptr;
	LocVar* LV = nullptr;
	if (FDLocVarAllowed && FindLocVar(&LVBD, &LV) && (LV->FTyp == 'f')) {
		return LV->FD;
	}
	R = CRdb;
	while (R != nullptr) {
		FD = R->rdb_file;
		while (FD != nullptr) {
			std::string lw = LexWord;
			if (EquUpCase(FD->Name, lw)) {
				return FD;
			}
			FD = FD->pChain;
		}
		R = R->ChainBack;
	}
	if (EquUpCase("CATALOG", LexWord)) {
		return CatFD->GetCatalogFile();
	}
	return nullptr;
}

FileD* Compiler::RdFileName()
{
	FileD* FD = nullptr;
	if (SpecFDNameAllowed && (Lexem == '@')) {
		LexWord = "@";
		Lexem = _identifier;
	}
	TestIdentif();
	FD = FindFileD();
	if ((FD == nullptr) || (FD == CRdb->rdb_file) && !SpecFDNameAllowed) Error(9);
	RdLex();
	return FD;
}

LinkD* Compiler::FindLD(std::string RoleName)
{
	FileD* F = CFile;

	// pro soubory 'LIKE' neexistuje zaznam v LinkDRoot, budeme tedy prochazet i predky (OrigFD)
	while (F != nullptr) {
		for (auto& L : LinkDRoot) {
			if ((L->FromFD == F) && EquUpCase(L->RoleName, RoleName)) {
				return L;
			}
		}
		F = F->OrigFD;
	}

	return nullptr;
}

bool Compiler::IsRoleName(bool Both, FileD** FD, LinkD** LD)
{
	TestIdentif();
	*FD = FindFileD();
	auto result = true;
	if ((*FD != nullptr) && (*FD)->IsParFile) {
		RdLex();
		*LD = nullptr;
		return result;
	}
	if (Both) {
		*LD = FindLD(LexWord);
		if (*LD != nullptr) {
			RdLex();
			*FD = (*LD)->ToFD;
			return result;
		}
	}
	result = false;
	return result;
}

FrmlElem* Compiler::RdFAccess(FileD* FD, LinkD* LD, char& FTyp)
{
	TestIdentif();
	auto Z = new FrmlElem7(_access, 12);
	Z->File2 = FD;
	Z->LD = LD;
	if ((LD != nullptr) && EquUpCase("EXIST", LexWord)) {
		RdLex();
		FTyp = 'B';
	}
	else {
		FileD* cf = CFile;
		CFile = FD;
		bool fa = FileVarsAllowed;
		FileVarsAllowed = true;
		Z->P011 = RdFldNameFrmlF(FTyp, nullptr);
		CFile = cf;
		FileVarsAllowed = fa;
	}
	return Z;
}

FrmlElem* Compiler::FrmlContxt(FrmlElem* Z, FileD* FD, void* RP)
{
	auto Z1 = new FrmlElem8(_newfile, 8);
	Z1->Frml = Z;
	Z1->NewFile = FD;
	Z1->NewRP = RP;
	return Z1;
}

FrmlElem* Compiler::MakeFldFrml(FieldDescr* F, char& FTyp)
{
	auto Z = new FrmlElem7(_field, 4);
	Z->Field = F;
	FTyp = F->frml_type;
	return Z;
}

LinkD* Compiler::FindOwnLD(FileD* FD, std::string RoleName)
{
	std::string lw = LexWord;
	LinkD* result = nullptr;
	for (auto& ld : LinkDRoot) {
		if ((ld->ToFD == FD)
			&& EquUpCase(ld->FromFD->Name, lw)
			&& (ld->IndexRoot != 0)
			&& EquUpCase(ld->RoleName, RoleName))
		{
			result = ld;
			break;
		}
	}
	RdLex();
	return result;
}

FrmlElem* Compiler::TryRdFldFrml(FileD* FD, char& FTyp, MergeReportBase* caller)
{
	FileD* cf = nullptr; FieldDescr* f = nullptr;
	LinkD* ld = nullptr; FrmlElem* z = nullptr;
	pstring roleNm;
	FrmlElem* (*rff)(char&, MergeReportBase*);
	char typ = '\0';

	if (IsKeyWord("OWNED")) {
		rff = ptrRdFldNameFrml;
		//TODO: !!! ptrRdFldNameFrml = RdFldNameFrmlF;
		Accept('(');
		z = new FrmlElem23(_owned, 12); // GetOp(_owned, 12);
		TestIdentif();
		SkipBlank(false);
		if (ForwChar == '(') {
			roleNm = LexWord;
			RdLex();
			RdLex();
			ld = FindOwnLD(FD, roleNm);
			Accept(')');
		}
		else {
			ld = FindOwnLD(FD, FD->Name);
		}
		if (ld == nullptr) OldError(182);
		((FrmlElem23*)z)->ownLD = ld;
		cf = CFile;
		CFile = ld->FromFD;
		if (Lexem == '.') {
			RdLex();
			((FrmlElem23*)z)->ownSum = RdFldNameFrmlF(FTyp, caller);
			if (FTyp != 'R') OldError(20);
		}
		if (Lexem == ':') {
			RdLex();
			((FrmlElem23*)z)->ownBool = RdFormula(typ, caller);
			TestBool(typ);
		}
		Accept(')');
		CFile = cf;
		FTyp = 'R';
		ptrRdFldNameFrml = rff;
	}
	else {
		f = FindFldName(FD);
		if (f == nullptr) z = nullptr;
		else {
			RdLex();
			z = MakeFldFrml(f, FTyp);
		}
	}
	return z;
}

FrmlElem* Compiler::RdFldNameFrmlF(char& FTyp, MergeReportBase* caller)
{
	LinkD* ld = nullptr;
	FileD* fd = nullptr;
	FrmlElem* z = nullptr;

	if (IsForwPoint()) {
		const bool isRoleName = IsRoleName(FileVarsAllowed, &fd, &ld);
		if (!isRoleName) {
			Error(9);
		}
		RdLex();
		return RdFAccess(fd, ld, FTyp);
	}
	if (!FileVarsAllowed) Error(110);
	z = TryRdFldFrml(CFile, FTyp, caller);
	if (z == nullptr) Error(8);
	return z;
}
