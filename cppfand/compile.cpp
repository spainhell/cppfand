#include "compile.h"
#include <map>
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "obaseww.h"
#include "rdfildcl.h"
#include "runfrml.h"
#include "TFile.h"
#include "wwmix.h"
#include "../Editor/OldEditor.h"
#include "../textfunc/textfunc.h"

const BYTE MaxLen = 9;
RdbPos ChptIPos; // used in LexAnal & ProjMgr

bool KeyArgFound;
FieldDescr* KeyArgFld;

pstring QQdiv = "div";
pstring QQmod = "mod";
pstring QQround = "round";

BYTE CurrChar; // { Compile }
BYTE ForwChar, ExpChar, Lexem;
pstring LexWord;

void Error(integer N)
{
	RdMsg(1000 + N);
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
			case _number: RdMsg(1004); break;
			case _identifier: RdMsg(1005); break;
			case _quotedstr: RdMsg(1013); break;
			}
			ErrMsg = ErrMsg + " " + MsgLine;
		}
	}
	CurrPos--;
	ClearKbdBuf();
	size_t l = InpArrLen;
	WORD i = CurrPos;
	if (IsTestRun && (!PrevCompInp.empty() && InpRdbPos.R != CRdb /* 0xinclude higher Rdb*/
		|| InpRdbPos.R == nullptr) /* TODO: ptr(0, 1)*/ /*LongStr + ShowErr*/
		&& StoreAvail() > l + TxtCols * TxtRows * 2 + 50)
	{
		bool upd;
		//MarkStore(p1);
		longint w = PushW(1, 1, TxtCols, TxtRows);
		TextAttr = screen.colors.tNorm;
		char* p = new char[l]; // GetStore(l);
		memcpy(p, InpArrPtr, l);
		if (!PrevCompInp.empty()) {
			RdMsg(63);
		}
		else {
			RdMsg(61);
		}
		std::string HdTxt = MsgLine;
		LongStr LS;
		LS.A = p;
		LS.LL = l;
		SimpleEditText('T', ErrMsg, HdTxt, &LS, 0xfff, i, upd);
		p = LS.A;
		l = LS.LL;
		PopW(w);
		//ReleaseStore(p1);
	}
	EdRecKey = ErrMsg;
	LastExitCode = i + 1;
	IsCompileErr = true;
	MsgLine = ErrMsg;
	GoExit();
}

void SetInpStr(std::string& s)
{
	InpArrLen = s.length();
	InpArrPtr = (BYTE*)s.data();
	if (InpArrLen == 0) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[0];
	CurrPos = 0;
	FillChar(&InpRdbPos, sizeof(InpRdbPos), 0);
}

void SetInpStdStr(std::string& s, bool ShowErr)
{
	InpArrLen = s.length();
	InpArrPtr = (BYTE*)s.c_str();
	if (InpArrLen == 0) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[0];
	CurrPos = 0;
	InpRdbPos.R = nullptr;
	if (ShowErr) InpRdbPos.R = nullptr; // TODO: tady bylo InpRdbPos.R:=ptr(0,1);
	InpRdbPos.IRec = 0;
}

void SetInpLongStr(LongStr* S, bool ShowErr)
{
	InpArrLen = S->LL;
	InpArrPtr = (BYTE*)&S->A[0];
	if (InpArrLen == 0) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[0];
	CurrPos = 0;
	InpRdbPos.R = nullptr;
	if (ShowErr) InpRdbPos.R = nullptr; // TODO: tady bylo InpRdbPos.R:=ptr(0,1);
	InpRdbPos.IRec = 0;
}

// vycte z CFile->TF blok dat
// nastavi InpArrPtr a InptArrLen - retezec pro zpracovani
void SetInpTTPos(longint Pos, bool Decode)
{
	LongStr* s = CFile->TF->Read(2, Pos);
	if (Decode) CodingLongStr(s);
	InpArrLen = s->LL;
	InpArrPtr = (BYTE*)&s->A[0];
	if (InpArrLen == 0) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[0];
	CurrPos = 0;
}

void SetInpTTPos(FileD* file, longint Pos, bool Decode)
{
	LongStr* s = file->TF->Read(2, Pos);
	if (Decode) CodingLongStr(s);
	InpArrLen = s->LL;
	InpArrPtr = (BYTE*)&s->A[0];
	if (InpArrLen == 0) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[0];
	CurrPos = 0;
}

void SetInpTT(RdbPos* RP, bool FromTxt)
{
	longint Pos = 0;
	FileD* CF = nullptr;
	void* CR = nullptr;
	LongStr* s = nullptr;

	if (RP->IRec == 0) {
		std::string inp = RunStdStr((FrmlElem*)RP->R);
		SetInpStdStr(inp, true);
		return;
	}
	InpRdbPos = *RP;
	CF = CFile;
	CR = CRecPtr;
	CFile = RP->R->FD;
	CRecPtr = new BYTE[RP->R->FD->RecLen];
	ReadRec(CFile, RP->IRec, CRecPtr);
	if (FromTxt) Pos = _T(ChptTxt, (unsigned char*)CRecPtr, CFile->Typ);
	else Pos = _T(ChptOldTxt, (unsigned char*)CRecPtr, CFile->Typ);
	SetInpTTPos(CFile, Pos, RP->R->Encrypted);
	ReleaseStore(CRecPtr);
	CFile = CF; CRecPtr = CR;
}

void SetInpTTxtPos(FileD* FD)
{
	SetInpTT(&FD->ChptPos, true);
	WORD pos = FD->TxtPosUDLI;
	RdbD* r = FD->ChptPos.R;
	if (pos > InpArrLen) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[pos];
	CurrPos = pos;
}

void ReadChar()
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

WORD RdDirective(bool& b)
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
		if (SEquUpcase(s, Dirs[i])) goto label1;
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
			for (WORD j = 1; j < Switches.length(); j++)
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
		if (/*(PtrRec(InpRdbPos.R).Seg*/ InpRdbPos.R != nullptr) CRdb = InpRdbPos.R;
		res = FindChpt('I', s, false, &ChptIPos);
		CRdb = r;
		if (!res) Error(37);
	}
	if (ForwChar != '}') Error(158);
	ReadChar();
	return i;
}

void RdForwName(pstring& s)
{
	s[0] = 0;
	while ((s.length() < 12) && (IsLetter(ForwChar) || isdigit(ForwChar)))
	{
		s[0]++; s[s.length()] = ForwChar;
		ReadChar();
	}
}

void SkipLevel(bool withElse)
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

void SkipBlank(bool toNextLine)
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

void OldError(integer N)
{
	CurrPos = OldErrPos;
	Error(N);
}

void RdBackSlashCode()
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

void RdLex()
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
	if (LexWord == "H052")
	{
		printf("RdLex() r. 437 - %s\n", LexWord.c_str());
	}
}

bool IsForwPoint()
{
	return (ForwChar == '.') && (InpArrPtr[CurrPos + 1] != '.');
}

void TestIdentif()
{
	if (Lexem != _identifier) Error(29);
}

void TestLex(char X)
{
	if ((char)Lexem != X) { ExpChar = X; Error(1); }
}

void Accept(char X)
{
	if (X == (char)Lexem) {
		RdLex();
	}
	else {
		ExpChar = X;
		Error(X);
	}
}

integer RdInteger()
{
	integer I, J;
	val(LexWord, I, J);
	if (J != 0) Lexem = 0 /* != _number*/;
	Accept(_number);
	return I;
}

double ValofS(pstring& S)
{
	integer I = 0; double R = 0.0;

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

double RdRealConst()
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

bool EquUpCase(pstring& S1, pstring& S2)
{
	if (S1.length() != S2.length()) return false;
	for (size_t i = 1; i <= S1.length(); i++) // Pascal. string -> index od 1
	{
		BYTE c1 = S1[i]; BYTE c2 = S2[i];
		BYTE upC1 = UpcCharTab[c1]; BYTE upC2 = UpcCharTab[c2];
		if (upC1 != upC2) return false;
	}
	return true;

	/*asm  lea si, LexWord; les di, S; cld;xor ch, ch; mov cl, [si]; cmpsb; jnz @3;
	jcxz @2;xor bh, bh;
	@1:  mov bl, [si]; mov al, BYTE PTR UpcCharTab[bx]; mov bl, es: [di] ;
	cmp al, BYTE PTR UpcCharTab[bx]; jnz @3; inc si; inc di; loop @1;
	@2:  mov ax, 1; jmp @4;
	@3: xor ax, ax;
	@4:  end;*/
	//return false;
}

bool EquUpCase(std::string& S1, std::string& S2)
{
	if (S1.length() != S2.length()) return false;
	for (size_t i = 0; i <= S1.length(); i++)
	{
		BYTE c1 = S1[i]; BYTE c2 = S2[i];
		BYTE upC1 = UpcCharTab[c1]; BYTE upC2 = UpcCharTab[c2];
		if (upC1 != upC2) return false;
	}
	return true;
}

bool EquUpCase(const char* S)
{
	pstring temp = S;
	return EquUpCase(temp, LexWord);
}

//bool TestKeyWord(pstring S)
//{
//	return (Lexem == _identifier) && EquUpCase(S, LexWord);
//}

bool TestKeyWord(std::string S)
{
	std::string lw = LexWord;
	return (Lexem == _identifier) && EquUpCase(S, lw);
}

bool IsKeyWord(std::string S)
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

void AcceptKeyWord(const std::string& S)
{
	if (TestKeyWord(S)) {
		RdLex();
	}
	else {
		SetMsgPar(S);
		Error(33);
	}
}

bool IsOpt(pstring S)
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

bool IsDigitOpt(pstring S, WORD& N)
{
	char LastLexWord = LexWord[LexWord.length()];
	if ((Lexem == _identifier) && (LexWord.length() == S.length() + 1)
		&& SEquUpcase(copy(LexWord, 1, S.length()), S)
		&& (LastLexWord >= '0' && LastLexWord <= '9'))
	{
		N = LastLexWord - '0';
		RdLex();
		Accept(_equ);
		return true;
	}
	return false;
}

pstring* RdStrConst()
{
	pstring* S = new pstring(LexWord);
	Accept(_quotedstr);
	return S;
}

std::string RdStringConst()
{
	std::string result = LexWord;
	Accept(_quotedstr);
	return result;
}

char Rd1Char()
{
	if ((Lexem != _identifier) || (LexWord.length() != 1)) Error(124);
	char result = LexWord[1];
	RdLex();
	return result;
}

char RdQuotedChar()
{
	if ((Lexem != _quotedstr) || (LexWord.length() != 1)) Error(15);
	char result = LexWord[1];
	RdLex();
	return result;
}

bool IsIdentifStr(std::string& S)
{
	if ((S.length() == 0) || !IsLetter(S[0])) return false;
	for (size_t i = 1; i < S.length(); i++) {
		if (!(IsLetter(S[i]) || isdigit(S[i]))) return false;
	}
	return true;
}

stSaveState* SaveCompState()
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
	state->RdFldNameFrml = RdFldNameFrml;
	state->RdFunction = RdFunction;
	state->ChainSumEl = ChainSumEl;
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
	RdFldNameFrml = p->RdFldNameFrml;
	RdFunction = p->RdFunction;
	ChainSumEl = p->ChainSumEl;
	delete p;
}

LocVar* RdVarName(LocVarBlkD* LVB, bool IsParList)
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

KeyFldD* RdKF(FileD* FD)
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
	if (F->Typ == 'T') {
		OldError(84);
	}
	if (KF->CompLex && (F->Typ != 'A')) {
		OldError(94);
	}
	return KF;
}

WORD RdKFList(KeyFldD** KFRoot, FileD* FD)
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
		KF = (KeyFldD*)KF->Chain;
	}
	if (n > 255) OldError(126);
	return n;
}

void RdLocDcl(LocVarBlkD* LVB, bool IsParList, bool WithRecVar, char CTyp)
{
	FrmlElem* Z = nullptr;
	bool b = false;
	double r = 0;
	std::string s;
	char typ = '\0', lx = '\0', fc = '\0';
	WORD sz = 0, n = 0;
	FileD* cf = nullptr; FileD* fd = nullptr;
	void* cr = nullptr; stSaveState* p = nullptr;
	XWKey* k = nullptr; bool rp = false;
	KeyFldD* kf = nullptr; KeyFldD* kf1 = nullptr;
	char FDTyp = '\0';
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
				// Z = new FrmlElem5(_const, 0, true); // GetOp(_const, sizeof(bool));
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
			//if (r != 0) {
			//	Z = new FrmlElem2(_const, 0, r); // GetOp(_const, sizeof(double));
			//}
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
			//if (s != "") {
			//	Z = new FrmlElem4(_const, 0, s); // GetOp(_const, s.length() + 1);
			//}
			newVars[0]->S = s;
		}
		typ = 'S';
		sz = sizeof(longint);
	label2:
		for (LocVar* locvar : newVars)
		{
			locvar->FTyp = typ;
			locvar->Op = _getlocvar;
			locvar->IsRetPar = rp;
			locvar->Init = Z;
			locvar->BPOfs = LVB->Size;
			LVB->Size += sz;
		}

		//while (lv != nullptr) {
		//	/* !!! with lv^ do!!! */
		//	lv->FTyp = typ;
		//	lv->Op = _getlocvar;
		//	lv->IsRetPar = rp;
		//	lv->Init = Z;
		//	lv->BPOfs = LVB->Size;
		//	LVB->Size += sz;
		//	lv = (LocVar*)lv->Chain;
		//}
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
				FDTyp = '6';
				if (Lexem == '.') {
					RdLex();
					TestIdentif();
					if (EquUpCase("X")) FDTyp = 'X';
					else if (EquUpCase("DBF")) FDTyp = 'D';
					else Error(185);
					RdLex();
				}
				TestLex('[');
				p = SaveCompState();
				RdFileD(lv->Name, FDTyp, "$");
				TestLex(']');
				lv->FD = CFile;
				n = CurrPos; lx = Lexem; fc = ForwChar;
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
				if (CFile->Typ != 'X') OldError(108);
				kf1 = nullptr;
				if (Lexem == '(') {
					RdLex();
					RdKFList(&kf1, CFile);
					Accept(')');
				}
			}
			//while (lv != nullptr) {
			for (LocVar* locvar : newVars)
			{
				locvar->FTyp = typ;
				locvar->FD = CFile;
				if (typ == 'r') locvar->RecPtr = nullptr; // ptr(0,1) ??? /* for RdProc nullptr-tests + no Run*/
				   /* frueher bei IsParList K = nullptr; warum? */
				else {
					k = new XWKey(); // (XWKey*)GetZStore(sizeof(*k));
					k->Duplic = true;
					k->InWork = true;
					k->KFlds = kf1;
					kf = kf1;
					while (kf != nullptr) {
						k->IndexLen += kf->FldD->NBytes;
						kf = (KeyFldD*)kf->Chain;
					}
					locvar->RecPtr = k;
				}
				//lv = (LocVar*)lv->Chain;
			}
			CFile = cf; CRecPtr = cr;
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

bool FindLocVar(LocVar* LVRoot, LocVar** LV)
{
	auto result = false;
	if (Lexem != _identifier) return result;
	*LV = LVRoot;
	while (*LV != nullptr) {
		pstring lvName = (*LV)->Name.c_str();
		if (EquUpCase(lvName, LexWord)) { return true; }
		*LV = (LocVar*)(*LV)->Chain;
	}
	return result;
}

bool FindLocVar(LocVarBlkD* LVB, LocVar** LV)
{
	//auto result = false;
	if (Lexem != _identifier) return false;
	*LV = LVB->FindByName(LexWord);
	if (*LV == nullptr) return false;
	return true;
}

bool FindChpt(char Typ, const pstring& name, bool local, RdbPos* RP)
{
	FileD* CF = CFile;
	void* CR = CRecPtr;
	CFile = Chpt;
	CRecPtr = GetRecSpace();
	RdbD* R = CRdb;
	auto result = false;
	while (R != nullptr) {
		CFile = R->FD;
		for (WORD i = 1; i <= CFile->NRecs; i++) {
			ReadRec(CFile, i, CRecPtr);
			pstring chapterType = _ShortS(ChptTyp);
			pstring chapterName = OldTrailChar(' ', _ShortS(ChptName));

			//if ((_ShortS(ChptTyp) == Typ)
				//&& SEquUpcase(TrailChar(' ', _ShortS(ChptName)), name))
			if (chapterType.length() == 1 && chapterType[1] == Typ && SEquUpcase(chapterName, name))
			{
				RP->R = R;
				RP->IRec = i;
				result = true;
				goto label1;
			}
		}
		if (local) goto label1;
		R = R->ChainBack;
	}
label1:
	ReleaseStore(CRecPtr); CFile = CF; CRecPtr = CR;
	return result;
}

void RdChptName(char C, RdbPos* Pos, bool TxtExpr)
{
	if (TxtExpr && (Lexem == '[')) {
		RdLex();
		Pos->R = (RdbD*)RdStrFrml();
		Pos->IRec = 0;
		Accept(']');
	}
	else {
		TestLex(_identifier);
		if (!FindChpt(C, LexWord, false, Pos)) Error(37);
		RdLex();
	}
}

FieldListEl* AllFldsList(FileD* FD, bool OnlyStored)
{
	FieldListEl* FLRoot = nullptr;
	for (auto& F : FD->FldD) {
		if (((F->Flg & f_Stored) != 0) || !OnlyStored) {
			auto* const FL = new FieldListEl();
			FL->FldD = F;
			if (FLRoot == nullptr) { FLRoot = FL; FL->Chain = nullptr; }
			else ChainLast(FLRoot, FL);
		}
	}
	return FLRoot;
}

RprtOpt* GetRprtOpt()
{
	auto RO = new RprtOpt();
	RO->Mode = _ALstg;
	RO->Style = '?';
	RO->Width = spec.AutoRprtWidth;
	return RO;
}

void CFileLikeFD(FileD* FD, WORD MsgNr)
{
	FileD* FD1;
	if (!CFile->IsJournal && ((CFile == FD) || (CFile->OrigFD == FD))) return;
	SetMsgPar(CFile->Name, FD->Name);
	RunError(MsgNr);
}

std::string RdHelpName()
{
	std::string s;
	if (CRdb->HelpFD == nullptr) Error(132);
	if (Lexem != _identifier) TestLex(_quotedstr);
	s = LexWord;
	RdLex();
	return s;
}

FrmlElem* RdAttr()
{
	if (Lexem == '^') {
		BYTE n;
		RdLex();
		const char c = (char)(toupper(Rd1Char()) - 64);
		if (!screen.SetStyleAttr(c, n)) OldError(120);
		FrmlElem* z = new FrmlElem2(_const, 0, n);
		return z;
	}
	return RdRealFrml();
}

void RdW(WRectFrml& W)
{
	W.C1 = RdRealFrml(); Accept(',');
	W.R1 = RdRealFrml(); Accept(',');
	W.C2 = RdRealFrml(); Accept(',');
	W.R2 = RdRealFrml();
}

void RdFrame(FrmlElem** Z, BYTE& WFlags)
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
		*Z = RdStrFrml();
	}
	if (Lexem == '!') { WFlags = WFlags | WShadow; RdLex(); }
}

bool PromptSortKeys(FieldList FL, KeyFldD* SKRoot)
{
	wwmix ww;

	KeyFldD* SK;
	auto result = true;
	SKRoot = nullptr;
	while (FL != nullptr) {
		/* !!! with FL->FldD^ do!!! */
		if (FL->FldD->Typ != 'T') ww.PutSelect(FL->FldD->Name);
		FL = (FieldList)FL->Chain;
	}
	if (ss.Empty) return result;
	ss.AscDesc = true;
	ss.Subset = true;
	ww.SelectStr(0, 0, 25, "");
	if (Event.Pressed.KeyCombination() == __ESC) { return false; }
label1:
	LexWord = ww.GetSelect();
	if (LexWord != "") {
		SK = (KeyFldD*)GetZStore(sizeof(*SK));
		ChainLast(SKRoot, SK);
		SK->FldD = FindFldName(CFile);
		if (ss.Tag == '>') SK->Descend = true;
		if (SK->FldD->Typ == 'A') SK->CompLex = true;
		goto label1;
	}
	return result;
}

void RdAssignFrml(char FTyp, bool& Add, FrmlElem** Z)
{
	char Typ = '\0';
	if (Lexem == _addass) { RdLex(); Add = true; }
	else Accept(_assign);
	*Z = RdFrml(Typ);
	if ((FTyp != Typ) || Add && (Typ != 'R')) OldError(12);
}

bool FldTypIdentity(FieldDescr* F1, FieldDescr* F2)
{
	auto result = false;
	if (F1->Typ != F2->Typ) return result;
	if ((F1->Typ == 'F') && (F1->M != F2->M)) return result;
	if ((F1->Typ == 'N' || F1->Typ == 'A' || F1->Typ == 'F') && (F1->L != F2->L)) return result;
	return true;
}

void RdFldList(FieldListEl** FLRoot)
{
	FieldDescr* F = nullptr;
	FieldListEl* FL = nullptr;
label1:
	F = RdFldName(CFile);
	//FL = (FieldListEl*)GetStore(sizeof(*FL));
	FL = new FieldListEl();
	FL->FldD = F;
	if (*FLRoot == nullptr) *FLRoot = FL;
	else ChainLast(*FLRoot, FL);
	if (Lexem == ',') { RdLex(); goto label1; }
}

void RdFldList(std::vector<FieldDescr*>& vFields)
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

void RdFldList(std::vector<FieldDescr*>* vFields)
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

void RdNegFldList(bool& Neg, FieldListEl** FLRoot)
{
	if (Lexem == '^') { RdLex(); Neg = true; }
	Accept('(');
	if (Lexem == ')') Neg = true;
	else RdFldList(FLRoot);
	Accept(')');
}

void RdNegFldList(bool& Neg, std::vector<FieldDescr*>& vFields)
{
	if (Lexem == '^') { RdLex(); Neg = true; }
	Accept('(');
	if (Lexem == ')') Neg = true;
	else RdFldList(vFields);
	Accept(')');
}

void RdNegFldList(bool& Neg, std::vector<FieldDescr*>* vFields)
{
	if (Lexem == '^') { RdLex(); Neg = true; }
	Accept('(');
	if (Lexem == ')') Neg = true;
	else RdFldList(vFields);
	Accept(')');
}

void EditModeToFlags(pstring Mode, void* Flgs, bool Err)
{
	pstring FlgTxt[24] = { "^Y","?Y","^N","F1","F2","F3","01",
	"!!","??","?E","?N","<=","R2","24","CO","LI",
	"->","^M","EX","WX","S7","#A","#L","SL" };
	pstring s = "xx";
	bool* Flags = (bool*)Flgs;
	WORD i = 1;
	while (i < Mode.length()) {
		s[1] = toupper(Mode[i]);
		s[2] = toupper(Mode[i + 1]);
		i += 2;
		for (WORD j = 0; j < 24; j++)
			if (s == FlgTxt[j]) { Flags[j] = true; goto label1; }
		goto label2;
	label1:
		{}
	}
	if (i == Mode.length())
		label2:
	if (Err) Error(92);
}

XKey* RdViewKey()
{
	XKey* k = nullptr; LocVar* lv = nullptr;
	std::string s;
	XKey* result = nullptr;
	if (Lexem != '/') return result;
	RdLex();
	size_t i = 0;
	k = CFile->Keys;
	if (Lexem == '@') goto label1;
	TestIdentif();
	while (k != nullptr) {
		std::string lw = LexWord;
		if (EquUpCase(k->Alias, lw)) goto label1;
		k = k->Chain;
	}
	s = LexWord;
	i = s.find('_');
	if (i != std::string::npos) s = copy(s, i + 1, 255);
	s = CFile->Name + "_" + s;
	k = CFile->Keys;
	while (k != nullptr) {
		std::string kAl = k->Alias;
		if (SEquUpcase(s, kAl)) goto label1;
		k = k->Chain;
	}
	if (IdxLocVarAllowed && FindLocVar(&LVBD, &lv) && (lv->FTyp == 'i'))
	{
		if (lv->FD != CFile) Error(164);
		k = (XKey*)(lv->RecPtr);
		goto label1;
	}
	Error(109);
label1:
	if (CFile->Typ != 'X')
#ifdef FandSQL
		if (CFile->typSQLFile) Error(24); else
#endif
			Error(108);
	RdLex();
	result = k;
	return result;
}

void SrchZ(FrmlElem* Z);

void SrchF(FieldDPtr F)
{
	if (F == KeyArgFld) { KeyArgFound = true; return; }
	if ((F->Flg & f_Stored) == 0) SrchZ(F->Frml);
}

void SrchZ(FrmlElem* Z)
{
	KeyFldD* KF = nullptr;
	FrmlListEl* fl = nullptr;
	if (Z == nullptr) return;

	auto iZ0 = (FrmlElem0*)Z;
	auto iZ7 = (FrmlElem7*)Z;
	auto iZ19 = (FrmlElem19*)Z;

	switch (Z->Op) {
	case _field: {
		SrchF(iZ7->Field);
		break;
	}
	case _access: {
		if (iZ7->LD != nullptr) {
			KF = iZ7->LD->Args;
			while (KF != nullptr) {
				SrchF(KF->FldD);
				KF = (KeyFldD*)KF->Chain;
			}
		}
		break;
	}
	case _userfunc: {
		fl = iZ19->FrmlL;
		while (fl != nullptr) {
			SrchZ(fl->Frml);
			fl = (FrmlListEl*)fl->Chain;
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

bool IsFun(std::map<std::string, int>& strs, std::string input, instr_type& FunCode)
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

bool IsKeyArg(FieldDescr* F, FileD* FD)
{
	KeyArgFound = false;
	KeyFldD* kf = nullptr;
	XKey* k = FD->Keys;
	while (k != nullptr) {
		KeyArgFld = F;
		kf = k->KFlds;
		while (kf != nullptr) {
			SrchF(kf->FldD);
			if (KeyArgFound) { return true; }
			kf = (KeyFldD*)kf->Chain;
		}
		k = k->Chain;
	}
	return false;
}

void CompileRecLen()
{
	/* !!! with CFile^ do!!! */
	WORD l = 0;
	WORD n = 0;
	if ((CFile->Typ == 'X' || CFile->Typ == 'D')) l = 1;
	for (auto& F : CFile->FldD) {
		switch (CFile->Typ) {
		case '8': if (F->Typ == 'D') F->NBytes = 2; break;
		case 'D': {
			switch (F->Typ) {
			case 'F': F->NBytes = F->L - 1; break;
			case 'D': F->NBytes = 8; break;
			case 'T': F->NBytes = 10; break;
			}
			break;
		}
		}
		if ((F->Flg & f_Stored) != 0) { F->Displ = l; l += F->NBytes; n++; }
	}
	CFile->RecLen = l;
	switch (CFile->Typ) {
	case '8': CFile->FrstDispl = 4; break;
	case 'D': CFile->FrstDispl = (n + 1) * 32 + 1; break;
	default:  CFile->FrstDispl = 6; break;
	}
}

void TestBool(char FTyp)
{
	if (FTyp != 'B') OldError(18);
}

void TestString(char FTyp)
{
	if (FTyp != 'S') OldError(19);
}

void TestReal(char FTyp)
{
	if (FTyp != 'R') OldError(20);
}

FrmlElem* BOperation(char Typ, instr_type Fun, FrmlElem* Frml)
{
	TestBool(Typ);
	FrmlElem0* Z = new FrmlElem0(Fun, 0); // GetOp(Fun, 0);
	RdLex();
	Z->P1 = Frml;
	return Z;
}

FrmlElem* RdPrim(char& FTyp);

FrmlElem* RdMult(char& FTyp)
{
	WORD N = 0;
	FrmlElem0* Z = (FrmlElem0*)RdPrim(FTyp);
label1:
	FrmlElem0* Z1 = Z;
	switch (Lexem) {
	case '*': {
		Z = new FrmlElem0(_times, 0); // GetOp(_times, 0);
		goto label2;
		break;
	}
	case  '/': {
		Z = new FrmlElem0(_divide, 0); // GetOp(_divide, 0);
	label2:
		TestReal(FTyp); RdLex();
		Z->P1 = Z1; Z->P2 = RdPrim(FTyp);
		TestReal(FTyp); goto label1;
		break;
	}
	case _identifier: {
		if (EquUpCase(QQdiv, LexWord)) { Z = new FrmlElem0(_div, 0); /*GetOp(_div, 0);*/ goto label2; }
		else if (EquUpCase(QQmod, LexWord)) { Z = new FrmlElem0(_mod, 0); /*GetOp(_mod, 0);*/ goto label2; }
		else if (EquUpCase(QQround, LexWord)) {
			TestReal(FTyp);
			Z = new FrmlElem0(_round, 0); /*GetOp(_round, 0);*/ RdLex();
			Z->P1 = Z1;
			Z->P2 = RdPrim(FTyp);
			TestReal(FTyp);
		}
		break;
	}
	}
	return Z;
}

FrmlElem* RdAdd(char& FTyp)
{
	FrmlElem0* Z = (FrmlElem0*)RdMult(FTyp);
	FrmlElem0* Z1 = nullptr;
label1:
	switch (Lexem) {
	case '+': {
		Z1 = Z;
		if (FTyp == 'R') {
			Z = new FrmlElem0(_plus, 0); /*GetOp(_plus, 0);*/
			goto label2;
		}
		else {
			Z = new FrmlElem0(_concat, 0); // GetOp(_concat, 0);
			TestString(FTyp);
			RdLex();
			Z->P1 = Z1;
			Z->P2 = RdMult(FTyp);
			TestString(FTyp);
			goto label1;
		}
		break;
	}
	case '-': {
		Z1 = Z;
		Z = new FrmlElem0(_minus, 0); // GetOp(_minus, 0);
		TestReal(FTyp);
	label2:
		RdLex();
		Z->P1 = Z1;
		Z->P2 = RdMult(FTyp);
		TestReal(FTyp);
		goto label1;
		break;
	}
	}
	return Z;
}

WORD RdPrecision();

WORD RdTilde()
{
	if (Lexem == '~') { RdLex(); return 1; }
	return 0;
}

void RdInConst(FrmlElemIn* Z, char& FTyp, std::string& str, double& R)
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

FrmlElem* RdComp(char& FTyp)
{
	pstring S;
	BYTE* B = nullptr;
	integer N = 0;
	FrmlElem* Z1 = nullptr;
	FrmlElem* Z = RdAdd(FTyp);
	Z1 = Z;
	if (Lexem >= _equ && Lexem <= _ne)
		if (FTyp == 'R') {
			Z = new FrmlElem0(_compreal, 2); // GetOp(_compreal, 2);
			auto iZ0 = (FrmlElem0*)Z;
			iZ0->P1 = Z1;
			iZ0->N21 = Lexem;
			RdLex();
			iZ0->N22 = RdPrecision();
			iZ0->P2 = RdAdd(FTyp);
			TestReal(FTyp);
			FTyp = 'B';
		}
		else {
			TestString(FTyp);
			Z = new FrmlElem0(_compstr, 2); // GetOp(_compstr, 2);
			auto iZ0 = (FrmlElem0*)Z;
			iZ0->P1 = Z1;
			iZ0->N21 = Lexem;
			RdLex();
			iZ0->N22 = RdTilde();
			iZ0->P2 = RdAdd(FTyp);
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


FrmlPtr RdBAnd(char& FTyp)
{
	FrmlPtr Z = RdComp(FTyp);
	while (Lexem == '&') {
		Z = BOperation(FTyp, _and, Z);
		((FrmlElem0*)Z)->P2 = RdComp(FTyp);
		TestBool(FTyp);
	}
	return Z;
}

FrmlPtr RdBOr(char& FTyp)
{
	FrmlPtr Z = RdBAnd(FTyp);
	while (Lexem == '|')
	{
		Z = BOperation(FTyp, _or, Z);
		((FrmlElem0*)Z)->P2 = RdBAnd(FTyp);
		TestBool(FTyp);
	}
	return Z;
}

FrmlElem* RdFormula(char& FTyp)
{
	FrmlElem* Z = RdBOr(FTyp);
	while ((BYTE)Lexem == _limpl || (BYTE)Lexem == _lequ) {
		Z = BOperation(FTyp, (instr_type)Lexem, Z);
		((FrmlElem0*)Z)->P2 = RdBOr(FTyp);
		TestBool(FTyp);
	}
	return Z;
}

bool FindFuncD(FrmlElem** ZZ)
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
				fl->Frml = RdFormula(typ);
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

FrmlElem* RdPrim(char& FTyp)
{
	instr_type FunCode = _notdefined;
	FrmlElem* Z = nullptr; FrmlElem* Z1 = nullptr; FrmlElem* Z2 = nullptr; FrmlElem* Z3 = nullptr;
	char Typ = '\0';
	integer I = 0, N = 0; BYTE* B = nullptr;
	pstring Options;

	switch (Lexem) {
	case _identifier: {
		if (InpArrLen == 705 && CurrPos >= 210)
		{
			//printf("compile.cpp 1728; ");
		}
		SkipBlank(false);
		if (IsFun(R0Fun, LexWord, FunCode))
		{
			Z = new FrmlElem0(FunCode, 0); // GetOp(FunCode, 0);
			FTyp = 'R';
		}
		else if (IsFun(RCFun, LexWord, FunCode))
		{
			Z = new FrmlElem1(_getWORDvar, 1); // GetOp(_getWORDvar, 1);
			((FrmlElem1*)Z)->N01 = FunCode;
			FTyp = 'R';
		}
		else if (IsFun(S0Fun, LexWord, FunCode))
		{
			Z = new FrmlElem0(FunCode, 0); // GetOp(FunCode, 0);
			FTyp = 'S';
		}
		else if (IsFun(B0Fun, LexWord, FunCode))
		{
			Z = new FrmlElem0(FunCode, 0); // GetOp(FunCode, 0);
			FTyp = 'B';
		}
		else if (IsKeyWord("TRUE"))
		{
			Z = new FrmlElem5(_const, 1, true); // GetOp(_const, 1);
			//Z->B = true;
			FTyp = 'B';
		}
		else if (IsKeyWord("FALSE"))
		{
			Z = new FrmlElem5(_const, 1, false); // GetOp(_const, 1);
			FTyp = 'B';
		}
		else if (!EquUpCase("OWNED") && (ForwChar == '(')) {
			if (FindFuncD(&Z)) FTyp = ((FrmlElem19*)Z)->FC->FTyp;
			else if (IsFun(S3Fun, LexWord, FunCode))
			{
				RdLex();
				Z = new FrmlElem0(FunCode, 0); // GetOp(FunCode, 0);
				((FrmlElem0*)Z)->P1 = RdAdd(FTyp);
				if ((BYTE)FunCode == _copy) TestString(FTyp);
				else {
					TestReal(FTyp);
					FTyp = 'S';
				}
				Accept(',');
				((FrmlElem0*)Z)->P2 = RdAdd(Typ);
				if (((BYTE)FunCode == _str) && (Typ == 'S')) goto label0;
				TestReal(Typ);
				Accept(',');
				((FrmlElem0*)Z)->P3 = RdAdd(Typ);
				TestReal(Typ);
			label0:
				Accept(')');
			}
			else if (IsKeyWord("COND"))
			{
				RdLex();
				Z2 = nullptr;
			label1:
				Z = new FrmlElem0(_cond, 0); // GetOp(_cond, 0);
				if (!IsKeyWord("ELSE"))
				{
					((FrmlElem0*)Z)->P1 = RdFormula(Typ);
					TestBool(Typ);
				}
				Accept(':');
				((FrmlElem0*)Z)->P2 = RdAdd(Typ);
				if (Z2 == nullptr)
				{
					Z1 = Z;
					FTyp = Typ;
					if (Typ == 'B') OldError(70);
				}
				else {
					((FrmlElem0*)Z2)->P3 = Z;
					if (FTyp == 'S') TestString(Typ);
					else TestReal(Typ);
				}
				if ((((FrmlElem0*)Z)->P1 != nullptr) && (Lexem == ','))
				{
					RdLex();
					Z2 = Z;
					goto label1;
				}
				Accept(')');
				Z = Z1;
			}
			else if (IsKeyWord("MODULO"))
			{
				RdLex();
				Z1 = RdAdd(Typ);
				TestString(Typ);
				Z = new FrmlElem0(_modulo, 2); // GetOp(_modulo, 2);
				((FrmlElem0*)Z)->P1 = Z1;
				N = 0;
				do {
					Accept(',');
					//B = (BYTE*)GetStore(2);
					B = new BYTE();
					*B = (BYTE)RdInteger();
					N++;
				} while (Lexem == ',');
				Accept(')');
				((FrmlElem0*)Z)->W11 = N;
				FTyp = 'B';
			}
			else if (IsKeyWord("SUM")) {
				RdLex();
				if (FrmlSumEl != nullptr) OldError(74);
				if (ChainSumEl == nullptr) Error(28);
				//FrmlSumEl = (SumElem*)GetStore(sizeof(SumElem));
				FrmlSumEl = new std::vector<FrmlElemSum*>();
				FrstSumVar = true;
				auto f = new FrmlElemSum(_const, 0.0, RdAdd(FTyp));
				FrmlSumEl->push_back(f);
				TestReal(FTyp);
				Accept(')');
				Z = (FrmlElem*)f;
				ChainSumEl();
				FrmlSumEl = nullptr; // TODO: toto by se melo smazat, ale vyuziva ho v urcitych pripadech ChainSumEl() -> nutno upravit
			}
			else if (IsKeyWord("DTEXT"))
			{
				RdLex();
				Z1 = RdAdd(Typ);
				TestReal(Typ);
				Accept(',');
				TestLex(_identifier);
				for (I = 1; I < LexWord.length(); I++) LexWord[I] = toupper(LexWord[I]);
				goto label2;
			}
			else if (IsKeyWord("STRDATE"))
			{
				RdLex();
				Z1 = RdAdd(Typ);
				TestReal(Typ);
				Accept(',');
				TestLex(_quotedstr);
			label2:
				Z = new FrmlElem6(_strdate1, 0); // GetOp(_strdate1, LexWord.length() + 1);
				auto iZ = (FrmlElem6*)Z;
				iZ->PP1 = Z1;
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
				Z1 = RdAdd(Typ);
				TestString(Typ);
				Accept(',');
				TestLex(_quotedstr);
				Z = new FrmlElem6(_valdate, 0); // GetOp(_valdate, LexWord.length() + 1);
				auto iZ = (FrmlElem6*)Z;
				iZ->PP1 = Z1;
				iZ->Mask = LexWord;
				RdLex();
				goto label4;
			}
			else if (IsKeyWord("REPLACE")) {
				RdLex();
				Z1 = RdAdd(Typ);
				TestString(Typ);
				Accept(',');
				Z2 = RdAdd(Typ);
				TestString(Typ);
				Accept(',');
				Z3 = RdAdd(FTyp);
				TestString(FTyp);
				FunCode = _replace;
				goto label8;
			}
			else if (IsKeyWord("POS")) {
				RdLex();
				Z1 = RdAdd(Typ);
				TestString(Typ);
				Accept(',');
				Z2 = RdAdd(Typ);
				TestString(Typ);
				Z3 = nullptr;
				FunCode = _pos;
				FTyp = 'R';
			label8:
				Options = "";
				if (Lexem == ',') {
					RdLex();
					if (Lexem != ',')
					{
						TestLex(_quotedstr);
						Options = LexWord;
						RdLex();
					}
					if (((BYTE)FunCode == _pos) && (Lexem == ','))
					{
						RdLex();
						Z3 = RdAdd(Typ);
						TestReal(Typ);
					}
				}
				Z = new FrmlElem12(FunCode, Options.length() + 1); // GetOp(FunCode, Options.length() + 1);
				auto iZ = (FrmlElem12*)Z;
				iZ->PPPP1 = Z1; iZ->PPP2 = Z2; iZ->PP3 = Z3;
				iZ->Options = Options;
				Accept(')');
			}
			else if (IsFun(RS1Fun, LexWord, FunCode))
			{
				RdLex();
				Z = new FrmlElem0(FunCode, 0); // GetOp(FunCode, 0);
			label3:
				((FrmlElem0*)Z)->P1 = RdAdd(Typ);
				TestString(Typ);
				goto label4;
			}
			else if (IsFun(R1Fun, LexWord, FunCode))
			{
				RdLex();
				Z = new FrmlElem0(FunCode, 0); // GetOp(FunCode, 0);
				((FrmlElem0*)Z)->P1 = RdAdd(Typ);
				TestReal(Typ);
			label4:
				FTyp = 'R';
				Accept(')');
			}
			else if (IsFun(R2Fun, LexWord, FunCode))
			{
				RdLex();
				Z = new FrmlElem0(FunCode, 1); // GetOp(FunCode, 1);
				((FrmlElem0*)Z)->P1 = RdAdd(Typ);
				TestReal(Typ);
				Accept(',');
				((FrmlElem0*)Z)->P2 = RdAdd(Typ);
				TestReal(Typ);
				if ((Z->Op == _addwdays || Z->Op == _difwdays) && (Lexem == ','))
				{
					RdLex();
					((FrmlElem0*)Z)->N21 = RdInteger();
					if (((FrmlElem0*)Z)->N21 > 3) OldError(136);
				}
				goto label4;
			}
			else if (IsKeyWord("LEADCHAR"))
			{
				Z = new FrmlElem0(_leadchar, 2); // GetOp(_leadchar, 2);
				goto label5;
			}
			else if (IsKeyWord("TRAILCHAR"))
			{
				Z = new FrmlElem0(_trailchar, 2); // GetOp(_trailchar, 2);
			label5:
				RdLex();
				((FrmlElem0*)Z)->N11 = RdQuotedChar();
				Accept(',');
				((FrmlElem0*)Z)->P1 = RdAdd(Typ);
				TestString(Typ);
				if (Lexem == ',') {
					RdLex();
					((FrmlElem0*)Z)->N12 = RdQuotedChar();
				}
				goto label6;
			}
			else if (IsKeyWord("COPYLINE"))
			{
				Z = new FrmlElem0(_copyline, 0); // GetOp(_copyline, 0);
				goto label7;
			}
			else if (IsKeyWord("REPEATSTR"))
			{
				Z = new FrmlElem0(_repeatstr, 0); // GetOp(_repeatstr, 0);
			label7:
				RdLex();
				((FrmlElem0*)Z)->P1 = RdAdd(Typ);
				TestString(Typ);
				Accept(',');
				((FrmlElem0*)Z)->P2 = RdAdd(Typ);
				TestReal(Typ);
				if ((Lexem == ',') && (Z->Op == _copyline)) {
					RdLex();
					((FrmlElem0*)Z)->P3 = RdAdd(Typ);
					TestReal(Typ);
				}
				goto label6;
			}
			else if (IsFun(S1Fun, LexWord, FunCode))
			{
				Z = new FrmlElem0(FunCode, 0); // GetOp(FunCode, 0);
				RdLex();
				((FrmlElem0*)Z)->P1 = RdAdd(FTyp);
				if (FunCode == _char) TestReal(FTyp);
				else TestString(FTyp);
			label6:
				FTyp = 'S';
				Accept(')');
			}
			else if (IsKeyWord("TRUST")) {
				Z = new FrmlElem0(_trust, 0); // GetOp(_trust, 0);
				RdByteListInStore();
				FTyp = 'B';
			}
			else if (IsKeyWord("EQUMASK")) {
				Z = new FrmlElem0(_equmask, 0); // GetOp(_equmask, 0);
				FTyp = 'B';
				RdLex();
				((FrmlElem0*)Z)->P1 = RdAdd(Typ);
				TestString(Typ);
				Accept(',');
				((FrmlElem0*)Z)->P2 = RdAdd(Typ);
				TestString(Typ);
				Accept(')');
			}
			else if (RdFunction != nullptr) Z = RdFunction(FTyp);
			else Error(75);
		}
		else {
			if (RdFldNameFrml == nullptr) Error(110);
			else {
				Z = RdFldNameFrml(FTyp); // volani ukazatele na funkci
				if (Z == nullptr) return nullptr;
				if ((Z->Op != _access) || (((FrmlElem7*)Z)->LD != nullptr)) FrstSumVar = false;
			}
		}
		break;
	}
	case '^': {
		RdLex();
		Z = new FrmlElem0(_lneg, 0); // GetOp(_lneg, 0);
		((FrmlElem0*)Z)->P1 = RdPrim(FTyp);
		TestBool(FTyp);
		break;
	}
	case '(': {
		RdLex();
		Z = RdFormula(FTyp);
		Accept(')');
		break;
	}
	case '-': {
		RdLex();
		if (Lexem == '-') Error(7);
		Z = new FrmlElem0(_unminus, 0); // GetOp(_unminus, 0);
		((FrmlElem0*)Z)->P1 = RdPrim(FTyp);
		TestReal(FTyp);
		break;
	}
	case '+': {
		RdLex();
		if (Lexem == '+') Error(7);
		Z = RdPrim(FTyp);
		TestReal(FTyp);
		break;
	}
	case _quotedstr: {
		Z = new FrmlElem4(_const, 0); // GetOp(_const, LexWord.length() + 1);
		FTyp = 'S';
		((FrmlElem4*)Z)->S = LexWord;
		RdLex();
		break;
	}
	default: {
		FTyp = 'R';
		Z = new FrmlElem2(_const, 0); // GetOp(_const, sizeof(double));
		((FrmlElem2*)Z)->R = RdRealConst();
		break;
	}
	}
	return Z;
}

WORD RdPrecision()
{
	WORD n = 5;
	if ((Lexem == '.') && (ForwChar >= '0' && ForwChar <= '9'))
	{
		RdLex(); n = RdInteger();
		if (n > 10) OldError(21);
	}
	return n;
}

//FrmlElem* MyBPContext(FrmlElem* Z, bool NewMyBP)
//{
//	FrmlElem* Z1;
//	if (NewMyBP) {
//		Z1 = new FrmlElem0(_setmybp, 0); // GetOp(_setmybp, 0);
//		((FrmlElem0*)Z1)->P1 = Z;
//		Z = Z1;
//	}
//	return Z;
//}

FrmlListEl* RdFL(bool NewMyBP, FrmlList FL1)
{
	char FTyp = '\0';
	KeyFldD* KF = CViewKey->KFlds;
	FrmlListEl* FLRoot = nullptr;
	KeyFldD* KF2 = (KeyFldD*)KF->Chain;
	bool FVA = FileVarsAllowed;
	FileVarsAllowed = false;
	bool b = FL1 != nullptr;
	if (KF2 != nullptr) Accept('(');
label1:
	FrmlList FL = new FrmlListEl(); // (FrmlListEl*)GetStore(sizeof(*FL));
	if (FLRoot == nullptr) FLRoot = FL;
	else ChainLast(FLRoot, FL);
	FL->Frml = RdFrml(FTyp); // MyBPContext(RdFrml(FTyp), NewMyBP);
	if (FTyp != KF->FldD->FrmlTyp) OldError(12);
	KF = (KeyFldD*)KF->Chain;
	if (b) {
		FL1 = (FrmlListEl*)FL1->Chain;
		if (FL1 != nullptr) { Accept(','); goto label1; }
	}
	else if ((KF != nullptr) && (Lexem == ',')) { RdLex(); goto label1; }
	if (KF2 != nullptr) Accept(')');
	auto result = FLRoot;
	FileVarsAllowed = FVA;
	return result;
}

FrmlElem* RdKeyInBool(KeyInD** KIRoot, bool NewMyBP, bool FromRdProc, bool& SQLFilter)
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
			&& (EquUpCase("EVALB") || EquUpCase("EVALS") || EquUpCase("EVALR")))
			FileVarsAllowed = false;
	}
	if (IsKeyWord("KEY")) {
		AcceptKeyWord("IN");
		if ((CFile->Typ != 'X') || (CViewKey == nullptr)) OldError(118);
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
		Z = RdFormula(FTyp);
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

FrmlElem* RdFrml(char& FTyp)
{
	FrmlSumEl = nullptr;
	return RdFormula(FTyp);
}

FrmlElem* RdBool()
{
	char FTyp = 0;
	FrmlSumEl = nullptr;
	auto result = RdFormula(FTyp);
	TestBool(FTyp);
	return result;
}

FrmlElem* RdRealFrml()
{
	char FTyp = 0;
	FrmlSumEl = nullptr;
	auto result = RdAdd(FTyp);
	TestReal(FTyp);
	return result;
}

FrmlElem* RdStrFrml()
{
	char FTyp = 0;
	FrmlSumEl = nullptr;
	auto result = RdAdd(FTyp);
	TestString(FTyp);
	return result;
}

//int ElemCount = 0;
//int ElemTotalSize = 0;

//FrmlPtr GetOp(BYTE Op, integer BytesAfter)
//{
//	WORD l = 0;
//	if (Op < 0x60) l = 1;	// bude uložen jen "Op"
//	else if (Op < 0xb0) l = 5;   // bude uložen "Op" a 1 ukazatel
//	else if (Op < 0xf0) l = 9;   // bude uložen "Op" a 2 ukazatele
//	else l = 13; // bude uložen "Op" a 3 ukazatele
//	//Z = (FrmlPtr)GetZStore(l + BytesAfter);
//	FrmlElem* Z = new FrmlElem(BytesAfter);
//	//ElemCount++;
//	//ElemTotalSize += l + BytesAfter;
//	//printf("FrmlElem: count %i, total size %i;", ElemCount, ElemTotalSize);
//	//printf("SizeOf FrmlElem* = %i; ", sizeof(FrmlElem));
//	Z->Op = Op;
//	//if (BytesAfter == 0) {
//	//	printf("GetOp() BytesAfter: %i", BytesAfter);
//	//}
//	return Z;
//}

FieldDescr* FindFldName(FileD* FD, std::string fieldName)
{
	if (fieldName.empty()) fieldName = LexWord;
	if (FD != nullptr) {
		for (auto& i : FD->FldD) {
			if (EquUpCase(i->Name, fieldName)) return i;
		}
	}
	return nullptr;
}

FieldDescr* RdFldName(FileD* FD)
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

FileD* FindFileD()
{
	FileD* FD = nullptr;
	RdbD* R = nullptr;
	LocVar* LV = nullptr;
	if (FDLocVarAllowed && FindLocVar(&LVBD, &LV) && (LV->FTyp == 'f')) {
		return LV->FD;
	}
	R = CRdb;
	while (R != nullptr) {
		FD = R->FD;
		while (FD != nullptr) {
			std::string lw = LexWord;
			if (EquUpCase(FD->Name, lw)) { return FD; }
			FD = (FileD*)FD->Chain;
		}
		R = R->ChainBack;
	}
	if (EquUpCase("CATALOG")) return CatFD;
	return nullptr;
}

FileD* RdFileName()
{
	FileD* FD = nullptr;
	if (SpecFDNameAllowed && (Lexem == '@')) {
		LexWord = "@";
		Lexem = _identifier;
	}
	TestIdentif();
	FD = FindFileD();
	if ((FD == nullptr) || (FD == CRdb->FD) && !SpecFDNameAllowed) Error(9);
	RdLex();
	return FD;
}

LinkD* FindLD(pstring RoleName)
{
	FileD* F = CFile;

	// pro soubory 'LIKE' neexistuje zaznam v LinkDRoot, budeme tedy prochazet i predky (OrigFD)
	while (F != nullptr) {
		LinkD* L = LinkDRoot;
		while (L != nullptr) {
			if ((L->FromFD == F) && SEquUpcase(L->RoleName, RoleName)) {
				return L;
			}
			L = L->Chain;
		}
		F = F->OrigFD;
	}

	return nullptr;
}

bool IsRoleName(bool Both, FileD** FD, LinkD** LD)
{
	TestIdentif();
	*FD = FindFileD();
	auto result = true;
	if ((*FD != nullptr) && (*FD)->IsParFile) {
		RdLex();
		*LD = nullptr;
		return result;
	}
	if (Both)
	{
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

FrmlElem* RdFAccess(FileD* FD, LinkD* LD, char& FTyp)
{
	TestIdentif();
	auto Z = new FrmlElem7(_access, 12); // GetOp(_access, 12);
	Z->File2 = FD;
	Z->LD = LD;
	if ((LD != nullptr) && EquUpCase("EXIST")) { RdLex(); FTyp = 'B'; }
	else {
		FileD* cf = CFile;
		CFile = FD;
		bool fa = FileVarsAllowed;
		FileVarsAllowed = true;
		Z->P011 = RdFldNameFrmlF(FTyp);
		CFile = cf;
		FileVarsAllowed = fa;
	}
	return Z;
}

FrmlElem* FrmlContxt(FrmlElem* Z, FileD* FD, void* RP)
{
	auto Z1 = new FrmlElem8(_newfile, 8); // GetOp(_newfile, 8);
	Z1->Frml = Z;
	Z1->NewFile = FD;
	Z1->NewRP = RP;
	return Z1;
}

FrmlElem* MakeFldFrml(FieldDescr* F, char& FTyp)
{
	auto Z = new FrmlElem7(_field, 4); // GetOp(_field, 4);
	Z->Field = F;
	FTyp = F->FrmlTyp;
	return Z;
}

LinkD* FindOwnLD(FileD* FD, pstring RoleName)
{
	LinkD* ld = LinkDRoot;
	LinkD* result = nullptr;
	while (ld != nullptr) {
		std::string lw = LexWord;
		if ((ld->ToFD == FD)
			&& EquUpCase(ld->FromFD->Name, lw)
			&& (ld->IndexRoot != 0)
			&& SEquUpcase(ld->RoleName, RoleName))
			goto label1;
		ld = ld->Chain;
	}
label1:
	RdLex();
	return ld;
}

FrmlElem* TryRdFldFrml(FileD* FD, char& FTyp)
{
	FileD* cf = nullptr; FieldDescr* f = nullptr;
	LinkD* ld = nullptr; FrmlElem* z = nullptr;
	pstring roleNm;
	FrmlElem* (*rff)(char&);
	char typ = '\0';

	if (IsKeyWord("OWNED")) {
		rff = RdFldNameFrml;
		RdFldNameFrml = RdFldNameFrmlF;
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
		else ld = FindOwnLD(FD, FD->Name);
		if (ld == nullptr) OldError(182);
		((FrmlElem23*)z)->ownLD = ld;
		cf = CFile;
		CFile = ld->FromFD;
		if (Lexem == '.') {
			RdLex();
			((FrmlElem23*)z)->ownSum = RdFldNameFrmlF(FTyp);
			if (FTyp != 'R') OldError(20);
		}
		if (Lexem == ':') {
			RdLex();
			((FrmlElem23*)z)->ownBool = RdFormula(typ);
			TestBool(typ);
		}
		Accept(')');
		CFile = cf;
		FTyp = 'R';
		RdFldNameFrml = rff;
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

FrmlElem* RdFldNameFrmlF(char& FTyp)
{
	//if (InpArrLen == 3360) {
	//	printf("RdFldNameFrmlF() %i\n", CurrPos);
	//}
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
	z = TryRdFldFrml(CFile, FTyp);
	if (z == nullptr) Error(8);
	return z;
}
