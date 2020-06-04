#include "compile.h"

#include <map>

#include "editor.h"
#include "obaseww.h"
#include "rdfildcl.h"
#include "runfrml.h"
#include "wwmix.h"

const BYTE MaxLen = 9;
RdbPos ChptIPos; // usen in LexAnal & ProjMgr

bool KeyArgFound;
FieldDPtr KeyArgFld;

pstring QQdiv = "div";
pstring QQmod = "mod";
pstring QQround = "round";

BYTE CurrChar; // { Compile }
BYTE ForwChar, ExpChar, Lexem;
pstring LexWord;

void Error(integer N)
{
	pstring ErrMsg;
	pstring HdTxt(40);
	void* p = nullptr; void* p1 = nullptr;
	bool upd;
	WORD l, i;
	longint w;

	RdMsg(1000 + N);
	ErrMsg = MsgLine;
	if (N == 1) {
		if (ExpChar >= ' ') ErrMsg = ErrMsg + " " + ExpChar;
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
	ClearKbdBuf(); l = InpArrLen; i = CurrPos;
	if (IsTestRun && (PrevCompInp != nullptr && InpRdbPos.R != CRdb /* 0xinclude higher Rdb*/
		|| InpRdbPos.R == nullptr) /* TODO: ptr(0, 1)*/ /*LongStr + ShowErr*/
		&& StoreAvail() > l + TxtCols * TxtRows * 2 + 50)
	{
		MarkStore(p1);
		w = PushW(1, 1, TxtCols, TxtRows);
		TextAttr = colors.tNorm;
		p = GetStore(l);
		Move(InpArrPtr, p, l);
		if (PrevCompInp != nullptr) RdMsg(63); else RdMsg(61);
		HdTxt = MsgLine;
		SimpleEditText('T', ErrMsg, HdTxt, (char*)p, 0xfff, l, i, upd);
		PopW(w);
		ReleaseStore(p1);
	}
	EdRecKey = ErrMsg;
	LastExitCode = i + 1;
	IsCompileErr = true;
	MsgLine = ErrMsg;
	GoExit();
}

void SetInpStr(pstring& S)
{
	InpArrLen = S.length();
	InpArrPtr = &S[1];
	if (InpArrLen == 0) ForwChar = 0x1A;
	else ForwChar = InpArrPtr[0];
	CurrPos = 0;
	FillChar(&InpRdbPos, sizeof(InpRdbPos), 0);
}

void SetInpLongStr(LongStr* S, bool ShowErr)
{
	InpArrLen = S->LL;
	InpArrPtr = (BYTE*)S->A[0];
	if (InpArrLen == 0) ForwChar = 0x1A; else ForwChar = InpArrPtr[0];
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

void SetInpTT(RdbPos RP, bool FromTxt)
{
	longint Pos = 0;
	FileD* CF = nullptr;
	void* CR = nullptr;
	LongStr* s = nullptr;

	if (RP.IRec == 0) {
		SetInpLongStr(RunLongStr(FrmlPtr(RP.R)), true);
		return;
	}
	InpRdbPos = RP;
	CF = CFile;
	CR = CRecPtr;
	CFile = RP.R->FD;
	CRecPtr = GetRecSpace();
	ReadRec(RP.IRec);
	if (FromTxt) Pos = _T(ChptTxt);
	else Pos = _T(ChptOldTxt);
	SetInpTTPos(Pos, RP.R->Encrypted);
	ReleaseStore(CRecPtr); CFile = CF; CRecPtr = CR;
}

void SetInpTTxtPos(FileDPtr FD)
{
	WORD pos; RdbDPtr r;
	SetInpTT(FD->ChptPos, true);
	pos = FD->TxtPosUDLI;
	r = FD->ChptPos.R;
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
	else if (CurrPos == InpArrLen) { CurrPos++; ForwChar = 0x1A; } // CTRL+Z = 0x1A
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
	WORD i, j;
	pstring s(12);
	RdbDPtr r;
	bool res;

	ReadChar(); RdForwName(s);
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
			for (j = 1; j < Switches.length(); j++)
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
		s[0]++; s[s.length()] = ForwChar; ReadChar();
	}
}

void SkipLevel(bool withElse)
{
	WORD begLevel, n;
	bool b;
	begLevel = SwitchLevel;

label1:
	switch (ForwChar) {       /* skip to directive */
	case '\'': { do { ReadChar(); } while (!(ForwChar == '\'' || ForwChar == 0x1A)); break; }
	case '{': {
		ReadChar(); if (ForwChar == '$') goto label3;
		n = 1;
	label2:
		switch (ForwChar) {
		case '{': { n++; break; }
		case 0x1A: { Error(11); break; }
		case '}': { n--; if (n == 0) { ReadChar(); goto label1; } break; }
		}
		ReadChar();
		goto label2;
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
	char CC;
	WORD n;
	bool b;
	CompInpD* ci;
	CC = CurrChar;

label1:
	switch (ForwChar)
	{
	case 0x1A: {
		if (PrevCompInp != nullptr) {
			ci = PrevCompInp; Move(ci->ChainBack, PrevCompInp, sizeof(CompInpD));
			if (CurrPos <= InpArrLen) ForwChar = InpArrPtr[CurrPos];
			goto label1;
		}
		break;
	}
	case '{':
	{
		ReadChar();
		if (ForwChar == '$') {
			n = RdDirective(b);
			switch (n)
			{
			case 0: break;
			case 1: { SwitchLevel++; if (!b) SkipLevel(true); break; }
			case 5: {
				ci = (CompInpD*)GetStore2(sizeof(CompInpD));
				Move(PrevCompInp, ci, sizeof(CompInpD));
				PrevCompInp = ci; SetInpTT(ChptIPos, true); break; }
			default:
			{
				if (SwitchLevel == 0) Error(159);
				if (n == 3) SkipLevel(false); else SwitchLevel--;
				break;
			}
			}
			goto label1;
		}
		else {
			n = 1;
		label2:
			switch (ForwChar) {
			case '{': n++; // TODO: /*^Error z(11);*/
			case '}': {
				n--;
				if (n == 0) { ReadChar(); goto label1; break; }
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
			else { ReadChar(); goto label1; }
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
	pstring Num(3);

	if (ForwChar == '\\') { ReadChar(); return; }
	Num = "";
	while ((std::isdigit(ForwChar)) && (Num.length() < 3)) {
		ReadChar();
		Num = Num + CurrChar;
	}
	if (Num == "") return;
	val(Num, n, i);
	if (n > 255) Error(7);
	CurrChar = char(n);
}

void RdLex()
{
	OldErrPos = CurrPos;
	SkipBlank(false);
	ReadChar();
	Lexem = CurrChar;
	if (IsLetter(CurrChar))
	{
		Lexem = _identifier; LexWord[1] = CurrChar; 
		WORD i = 1;
		while (IsLetter(ForwChar) || isdigit(ForwChar))
		{
			i++; if (i > 32) Error(2); 
			ReadChar(); 
			LexWord[i] = CurrChar;
		}
		LexWord[0] = char(i);
		LexWord[i + 1] = '\0';
	}
	else if (isdigit(CurrChar))
	{
		Lexem = _number; LexWord[1] = CurrChar; 
		WORD i = 1;
		while (isdigit(ForwChar))
		{
			i++; if (i > 15) Error(6); 
			ReadChar(); 
			LexWord[i] = CurrChar;
		}
		LexWord[0] = char(i);
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
	if (Lexem != X) { ExpChar = X; Error(1); };
}

void Accept(char X)
{
	/*asm
	mov al, X;
	cmp al, Lexem;
	je @1;
	mov ExpChar, al;
	mov ax, 1;
	push ax;
	call Error;
	@1:  call RdLex;*/

	if (X == Lexem)
	{
		RdLex();
	}
	else
	{
		ExpChar = X;
		Error(X);
	}
}

integer RdInteger()
{
	integer I, J;
	val(LexWord, I, J); if (J != 0) Lexem = 0 /* != _number*/;
	Accept(_number);
	return I;
}

double ValofS(pstring& S)
{
	integer I = 0; double R = 0.0;

	val(S, R, I);
	if (I != 0) {
		R = ValDate(S, "DD.MM.YY");
		if (R == 0) {
			R = ValDate(S, "DD.MM.YYYY");
			if (R == 0) {
				R = ValDate(S, "mm:hh:ss.tt");
				if (R == 0) {
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

bool EquUpcase(pstring& S1, pstring& S2)
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

bool EquUpcase(const char* S)
{
	pstring temp = S;
	return EquUpcase(temp, LexWord);
}

bool TestKeyWord(pstring S)
{
	return (Lexem == _identifier) && EquUpcase(S, LexWord);
}

bool IsKeyWord(pstring S)
{
	if (Lexem != _identifier) return false;
	if (LexWord.length() != S.length()) return false;
	for (size_t i = 1; i <= LexWord.length(); i++) // procházíme Pascalovský string, tedy od indexu 1
	{
		BYTE lw = LexWord[i];
		BYTE upcLw = UpcCharTab[lw]; // velke pismeno dle UpcCharTab
		if (upcLw != S[i]) return false;
	}
	RdLex();
	return true;
	
	//if (Lexem == _identifier)
	//{
	//	size_t count = LexWord.length();
	//	if (S.length() == count)
	//	{
	//	label1:
	//		size_t index = 1;
	//		
	//		if (UpcCharTab[LexWord[index]] != S[index]) return false;
	//		index++;
	//		if (count - index > 0) goto label1;
	//		RdLex();
	//		return true;
	//	}
	//}
	//return false;
}

void AcceptKeyWord(pstring S)
{
	if (TestKeyWord(S)) RdLex();
	else { SetMsgPar(S); Error(33); }
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

bool IsIdentifStr(pstring& S)
{
	WORD i;
	if ((S.length() == 0) || !IsLetter(S[1])) return false;
	for (i = 2; i < S.length(); i++) {
		if (!(IsLetter(S[i]) || isdigit(S[i]))) return false;
	}
	return true;
}

void* SaveCompState()
{
	return nullptr;
}

void RestoreCompState(void* p)
{
}

LocVar* RdVarName(LocVarBlkD* LVB, bool IsParList)
{
	TestIdentif();
	LocVar* lv = LVB->Root;
	while (lv != nullptr) {
		if (EquUpcase(lv->Name, LexWord)) Error(26);
		lv = (LocVar*)lv->Chain;
	}
	//lv = (LocVar*)GetZStore(sizeof(*lv) - 1 + LexWord.length());
	lv = new LocVar();
	if (LVB->Root == nullptr) LVB->Root = lv;
	else ChainLast(LVB->Root, lv);
	//Move(&LexWord, &lv->Name, LexWord.length() + 1); 
	lv->Name = LexWord;
	RdLex();
	auto result = lv;
	if (IsParList) { lv->IsPar = true; LVB->NParam++; }
	return lv;
}

KeyFldD* RdKF(FileD* FD)
{
	//KF = (KeyFldD*)GetZStore(sizeof(KeyFldD));
	KeyFldD* KF = new KeyFldD();
	KeyFldD* result = KF;
	if (Lexem == _gt) { RdLex(); KF->Descend = true; }
	if (Lexem == '~') { RdLex(); KF->CompLex = true; }
	FieldDescr* F = RdFldName(FD); 
	KF->FldD = F;
	if (F->Typ == 'T') OldError(84);
	if (KF->CompLex && (F->Typ != 'A')) OldError(94);
	return result;
}

WORD RdKFList(KeyFldD** KFRoot, FileD* FD)
{
	WORD n = 0; KeyFldD* KF = nullptr;
label1:
	if (*KFRoot == nullptr) *KFRoot = RdKF(FD);
	else ChainLast(*KFRoot, RdKF(FD));
	if (Lexem == ',') { RdLex(); goto label1; }
	n = 0; 
	KF = *KFRoot;   /*looping over all fields, !only the last read*/
	while (KF != nullptr) 
	{ 
		if (KF->FldD != nullptr) n += KF->FldD->NBytes;
		KF = (KeyFldD*)KF->Chain; }
	if (n > 255) OldError(126);
	return n;
}

void RdLocDcl(LocVarBlkD* LVB, bool IsParList, bool WithRecVar, char CTyp)
{
	LocVar* lv = nullptr; FrmlElem* Z = nullptr; 
	pstring s; double r = 0; char typ = '\0', lx = '\0', fc = '\0'; 
	WORD sz = 0, n = 0;
	FileD* cf = nullptr; FileD* fd = nullptr;
	void* cr = nullptr; void* p = nullptr; XWKey* k = nullptr; bool rp = false;
	KeyFldD* kf = nullptr; KeyFldD* kf1 = nullptr;
	char FDTyp = '\0';
label1:
	rp = false;
	if (IsParList && IsKeyWord("VAR")) {
		if (CTyp == 'D') OldError(174); rp = true;
	}
	lv = RdVarName(LVB, IsParList);
	if (!IsParList) while (Lexem == ',') { RdLex(); RdVarName(LVB, IsParList); }
	Accept(':'); Z = nullptr;
	if (IsKeyWord("BOOLEAN")) {
		if ((Lexem == _equ) && !IsParList) {
			RdLex();
			if (IsKeyWord("TRUE")) {
				Z = GetOp(_const, sizeof(bool)); Z->B = true;
			}
			else { if (!IsKeyWord("FALSE")) Error(42); }
		}
		typ = 'B'; sz = sizeof(bool); goto label2;
	}
	else if (IsKeyWord("REAL")) {
		if ((Lexem == _equ) && !IsParList) {
			RdLex();
			r = RdRealConst();
			if (r != 0) {
				Z = GetOp(_const, sizeof(double));
				Z->R = r;
			}
		}
		typ = 'R'; sz = sizeof(double); goto label2;
	}
	else if (IsKeyWord("STRING")) {
		if ((Lexem == _equ) && !IsParList) {
			RdLex();
			s = LexWord; Accept(_quotedstr);
			if (s != "") {
				Z = GetOp(_const, s.length() + 1); Z->S = s;
			}
		}
		typ = 'S'; sz = sizeof(longint);
	label2:
		while (lv != nullptr) {
			/* !!! with lv^ do!!! */
			lv->FTyp = typ; lv->Op = _getlocvar; lv->IsRetPar = rp;
			lv->Init = Z; lv->BPOfs = LVB->Size;
			LVB->Size += sz; lv = (LocVar*)lv->Chain;
		}
	}
	else if (rp) Error(168);
	else if (WithRecVar)
		if (TestKeyWord("FILE")) {
			lv->FTyp = 'f';
			LexWord = lv->Name;
			if (LexWord.length() > 8) OldError(2);
			fd = FindFileD(); RdLex();
			if (IsParList) {
				if (!WithRecVar) OldError(162);
				if (fd == nullptr) OldError(163); lv->FD = fd;
			}
			else {
				if (fd != nullptr) OldError(26); FDTyp = '6';
				if (Lexem == '.') {
					RdLex(); TestIdentif();
					if (EquUpcase("X")) FDTyp = 'X';
					else if (EquUpcase("DBF")) FDTyp = 'D';
					else Error(185); RdLex();
				}
				TestLex('[');
				p = SaveCompState();
				RdFileD(lv->Name, FDTyp, '$');
				TestLex(']');
				lv->FD = CFile; n = CurrPos; lx = Lexem; fc = ForwChar;
				RestoreCompState(p);
				CurrPos = n; Lexem = lx; ForwChar = fc;
				RdLex();
			}
		}
		else if (IsKeyWord("INDEX")) { typ = 'i'; goto label3; }
		else if (IsKeyWord("RECORD")) {
			typ = 'r';
		label3:
			AcceptKeyWord("OF"); cf = CFile; cr = CRecPtr; CFile = RdFileName();
			if (typ == 'i') {
				if (CFile->Typ != 'X') OldError(108); kf1 = nullptr;
				if (Lexem == '(') { RdLex(); RdKFList(&kf1, CFile); Accept(')'); }
			}
			while (lv != nullptr) {
				lv->FTyp = typ; lv->FD = CFile;
				if (typ == 'r') lv->RecPtr = nullptr; // ptr(0,1) ??? /* for RdProc nullptr-tests + no Run*/
				   /* frueher bei IsParList K = nullptr; warum? */
				else {
					k = (XWKey*)GetZStore(sizeof(*k));
					k->Duplic = true; k->InWork = true;
					k->KFlds = kf1; kf = kf1;
					while (kf != nullptr) {
						k->IndexLen += kf->FldD->NBytes; kf = (KeyFldD*)kf->Chain;
					}
					lv->RecPtr = k;
				}
				lv = (LocVar*)lv->Chain;
			}
			CFile = cf; CRecPtr = cr;
		}
		else Error(137);
	else Error(39);
	if (IsParList) {
		if (Lexem == ')') return;
		else { Accept(';'); goto label1; };
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
		if (EquUpcase((*LV)->Name, LexWord)) { return true; }
		*LV = (LocVar*)(*LV)->Chain;
	}
	return result;
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
			ReadRec(i);
			pstring chapterType = _ShortS(ChptTyp);
			pstring chapterName = TrailChar(' ', _ShortS(ChptName));
			
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
		RdLex(); Pos->R = RdbDPtr(RdStrFrml); Pos->IRec = 0; Accept(']');
	}
	else {
		TestLex(_identifier);
		if (!FindChpt(C, LexWord, false, Pos)) Error(37);
		RdLex();
	}
}

FieldList AllFldsList(FileDPtr FD, bool OnlyStored)
{
	FieldList FLRoot, FL; FieldDPtr F;
	F = FD->FldD; FLRoot = nullptr;
	while (F != nullptr) {
		if ((F->Flg && f_Stored != 0) || !OnlyStored)
		{
			FL = (FieldListEl*)GetStore(sizeof(*FL));
			FL->FldD = F;
			ChainLast(FLRoot, FL);
		}
		F = (FieldDescr*)F->Chain;
	}
	return FLRoot;
}

EditOpt* GetEditOpt()
{
	EditOpt* EO = new EditOpt();
	//EO = (EditOpt*)GetZStore(sizeof(*EO));
	auto result = EO;
	EO->UserSelFlds = true;
	return result;
}

RprtOpt* GetRprtOpt()
{
	RprtOpt* RO;
	RO = (RprtOpt*)GetZStore(sizeof(*RO));
	auto result = RO;
	RO->Mode = _ALstg; RO->Style = '?'; RO->Width = spec.AutoRprtWidth;
	return RO;
}

void CFileLikeFD(FileD* FD, WORD MsgNr)
{
	FileD* FD1;
	if (!CFile->IsJournal && ((CFile == FD) || (CFile->OrigFD == FD))) return;
	Set2MsgPar(CFile->Name, FD->Name);
	RunError(MsgNr);
}

pstring* RdHelpName()
{
	pstring* s;
	if (CRdb->HelpFD == nullptr) Error(132);
	if (Lexem != _identifier) TestLex(_quotedstr);
	s = StoreStr(LexWord);
	RdLex();
	return s;
}

FrmlPtr RdAttr()
{
	char c; BYTE n; FrmlPtr z;
	if (Lexem == '^') {
		RdLex();
		c = (char)toupper(Rd1Char()) - 64;
		if (!SetStyleAttr(c, n)) OldError(120);
		z = GetOp(_const, sizeof(double));
		z->R = n; return z;
	}
	return RdRealFrml();
}

void RdW(WRectFrml& W)
{
	W.C1 = RdRealFrml(); Accept(','); W.R1 = RdRealFrml();
	Accept(',');
	W.C2 = RdRealFrml(); Accept(','); W.R2 = RdRealFrml();
}

void RdFrame(FrmlPtr Z, BYTE& WFlags)
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
		Z = RdStrFrml();
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
		if (FL->FldD->Typ != 'T') ww.PutSelect(FL->FldD->Name); FL = (FieldList)FL->Chain;
	}
	if (ss.Empty) return result;
	ss.AscDesc = true;
	ss.Subset = true;
	ww.SelectStr(0, 0, 25, "");
	if (KbdChar == _ESC_) { return false; }
label1:
	LexWord = ww.GetSelect(); if (LexWord != "") {
		SK = (KeyFldD*)GetZStore(sizeof(*SK)); 
		ChainLast(SKRoot, SK);
		SK->FldD = FindFldName(CFile);
		if (ss.Tag == '>') SK->Descend = true;
		if (SK->FldD->Typ == 'A') SK->CompLex = true;
		goto label1;
	}
	return result;
}

void RdAssignFrml(char FTyp, bool& Add, FrmlPtr Z)
{
	char Typ;
	if (Lexem == _addass) { RdLex(); Add = true; }
	else Accept(_assign);
	Z = RdFrml(Typ);
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

void RdFldList(FieldListEl* FLRoot)
{
	FieldDescr* F = nullptr; FieldListEl* FL = nullptr;
label1:
	F = RdFldName(CFile);
	//FL = (FieldListEl*)GetStore(sizeof(*FL));
	FL = new FieldListEl();
	FL->FldD = F;
	if (FLRoot == nullptr) FLRoot = FL;
	else ChainLast(FLRoot, FL);
	if (Lexem == ',') { RdLex(); goto label1; };
}

void RdNegFldList(bool& Neg, FieldList FLRoot)
{
	if (Lexem == '^') { RdLex(); Neg = true; }
	Accept('(');
	if (Lexem == ')') Neg = true;
	else RdFldList(FLRoot);
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

KeyDPtr RdViewKey()
{
	KeyDPtr k = nullptr; LocVar* lv = nullptr; pstring s; integer i = 0;
	KeyDPtr result = nullptr;
	if (Lexem != '/') return result;
	RdLex();
	k = CFile->Keys;
	if (Lexem == '@') goto label1;
	TestIdentif();
	while (k != nullptr) {
		if (EquUpcase(*k->Alias, LexWord)) goto label1;
		k = k->Chain;
	}
	s = LexWord;
	i = s.first('_');
	if (i != 0) s = copy(s, i + 1, 255);
	s = CFile->Name + "_" + s; 
	k = CFile->Keys;
	while (k != nullptr) {
		if (SEquUpcase(s, *k->Alias)) goto label1;
		k = k->Chain;
	}
	if (IdxLocVarAllowed && FindLocVar(LVBD.Root, &lv) && (lv->FTyp == 'i'))
	{
		if (lv->FD != CFile) Error(164);
		k = KeyDPtr(lv->RecPtr);
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

void SrchZ(FrmlPtr Z);

void SrchF(FieldDPtr F)
{
	if (F == KeyArgFld) { KeyArgFound = true; return; }
	if (F->Flg && f_Stored == 0) SrchZ(F->Frml);
}

void SrchZ(FrmlPtr Z)
{
	KeyFldDPtr KF; FrmlList fl;
	if (Z == nullptr) return;
	switch (Z->Op) {
	case _field: SrchF(Z->Field); break;
	case _access: {
		if (Z->LD != nullptr) {
			KF = Z->LD->Args;
			while (KF != nullptr) {
				SrchF(KF->FldD); KF = (KeyFldD*)KF->Chain;
			}
		}
		break;
	}
	case _userfunc: {
		fl = Z->FrmlL;
		while (fl != nullptr) {
			SrchZ(fl->Frml); fl = (FrmlListEl*)fl->Chain;
		}
		break;
	}
	default: {
		if (Z->Op >= 0x60 && Z->Op <= 0xAF)SrchZ(Z->P1); /*1-ary*/
		else if (Z->Op >= 0xB0 && Z->Op <= 0xEF) /*2-ary*/
		{
			SrchZ(Z->P1); SrchZ(Z->P2);
		}
		else if (Z->Op >= 0xB0 && Z->Op <= 0xEF) /*3-ary*/
		{
			SrchZ(Z->P1); SrchZ(Z->P2); SrchZ(Z->P3);
		}
		break;
	}
	}
}

bool IsFun(std::map<std::string, int>& strs, std::string input, char& FunCode)
{
	// prevedeme vse ze vstupu na mala pismena
	for (auto && c : input)
	{
		c = tolower(c);
	}
	auto it = strs.find(input);
	if (it != strs.end())
	{
		FunCode = (char)it->second;
		RdLex();
		return true;
	}
	return false;

	/*
	 asm  les bx,XFun; lea dx,LexWord[1]; mov ch,LexWord.byte; xor cl,cl; cld;
@1:  mov ah,es:[bx]; cmp ah,ch; jne @4; mov si,dx; mov di,bx; inc di;
@2:  lodsb; cmp al,41H; jb @3; cmp al,5AH; ja @3; add al,20H; { lowercase }
@3:  cmp al,es:[di]; jb @5; ja @4; inc di; dec ah; jnz @2; jmp @6;
@4:  add bx,MaxLen+1; inc cl; cmp cl,N; jb @1;              { next string }
@5:  mov ax,0; jmp @7;                                      { not found }
@6:  xor ch,ch; mov bx,cx; les di,XCode; mov al,es:[di+bx];  { found }
	 les di,FunCode; mov es:[di],al;
	 call RdLex; mov ax,1;
@7:
end;
	 */
}

bool IsKeyArg(FieldDPtr F, FileDPtr FD)
{
	KeyDPtr k; KeyFldDPtr kf;
	k = FD->Keys;
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
	FieldDPtr F = CFile->FldD;
	WORD l = 0;
	WORD n = 0;
	if ((CFile->Typ == 'X' || CFile->Typ == 'D')) l = 1;
	while (F != nullptr) {
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
		F = (FieldDescr*)F->Chain;
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

FrmlPtr BOperation(char Typ, char Fun, FrmlPtr Frml)
{
	FrmlPtr Z;
	TestBool(Typ); Z = GetOp(Fun, 0);
	RdLex(); Z->P1 = Frml; return Z;
}

FrmlPtr RdPrim(char& FTyp);

FrmlPtr RdMult(char& FTyp)
{
	WORD N;
	FrmlPtr Z = RdPrim(FTyp);
label1:
	FrmlPtr Z1 = Z;
	switch (Lexem) {
	case '*': {
		Z = GetOp(_times, 0); goto label2;
		break;
	}
	case  '/': {
		Z = GetOp(_divide, 0);
	label2:
		TestReal(FTyp); RdLex(); Z->P1 = Z1; Z->P2 = RdPrim(FTyp);
		TestReal(FTyp); goto label1;
		break;
	}
	case _identifier: {
		if (EquUpcase(QQdiv, LexWord)) { Z = GetOp(_div, 0); goto label2; }
		else if (EquUpcase(QQmod, LexWord)) { Z = GetOp(_mod, 0); goto label2; }
		else if (EquUpcase(QQround, LexWord)) {
			TestReal(FTyp); Z = GetOp(_round, 0); RdLex();
			Z->P1 = Z1;
			Z->P2 = RdPrim(FTyp);
			TestReal(FTyp);
		}
		break;
	}
	}
	return Z;
}

FrmlPtr RdAdd(char& FTyp)
{
	FrmlElem* Z = nullptr;
	FrmlElem* Z1 = nullptr;
	Z = RdMult(FTyp);
label1:
	switch (Lexem) {
	case '+': { Z1 = Z;
		if (FTyp == 'R') { Z = GetOp(_plus, 0); goto label2; }
		else {
			Z = GetOp(_concat, 0); 
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
		Z = GetOp(_minus, 0); 
		TestReal(FTyp);
	label2:
		RdLex();
		Z->P1 = Z1; 
		Z->P2 = RdMult(FTyp); 
		TestReal(FTyp); 
		goto label1;
		break; }
	}
	return Z;
}

WORD RdPrecision();

WORD RdTilde()
{
	if (Lexem == '~') { RdLex(); return 1; }
	return 0;
}

void RdInConst(FrmlPtr Z, double& R, pstring* S, char& FTyp)
{
	if (FTyp == 'S')
	{
		if (Z->N11 == 1/*tilde*/) *S = TrailChar(' ', LexWord);
		else *S = LexWord;
		Accept(_quotedstr);
	}
	else R = RdRealConst();
}

void StoreConst(double& R, pstring* S, char& FTyp)
{
	double* RPtr;
	switch (FTyp) {
	case 'S': StoreStr(*S); break;
	case 'R': { 
		//RPtr = (double*)GetStore(sizeof(*RPtr)); 
		RPtr = new double();
		*RPtr = R; 
		break; 
	}
	}
}

FrmlPtr RdComp(char& FTyp)
{
	FrmlElem* Z = nullptr;
	double R = 0.0;
	pstring S;
	BYTE* B = nullptr;
	integer N = 0;
	FrmlElem* Z1 = nullptr;
	Z = RdAdd(FTyp);
	Z1 = Z;
	if (Lexem >= _equ && Lexem <= _ne)
		if (FTyp == 'R')
		{
			Z = GetOp(_compreal, 2); 
			Z->P1 = Z1;
			Z->N21 = Lexem; 
			RdLex(); 
			Z->N22 = RdPrecision();
			Z->P2 = RdAdd(FTyp); 
			TestReal(FTyp); 
			FTyp = 'B';
		}
		else {
			TestString(FTyp); 
			Z = GetOp(_compstr, 2); 
			Z->P1 = Z1;
			Z->N21 = Lexem; 
			RdLex(); 
			Z->N22 = RdTilde();
			Z->P2 = RdAdd(FTyp); 
			TestString(FTyp); 
			FTyp = 'B';
		}
	else if ((Lexem == _identifier) && IsKeyWord("IN"))
	{
		if (FTyp == 'R')
		{
			Z = GetOp(_inreal, 1); Z->N11 = RdPrecision();
		}
		else { TestString(FTyp); Z = GetOp(_instr, 1); Z->N11 = RdTilde(); }
		Z->P1 = Z1; Accept('['); N = 0;
	label1:
		RdInConst(Z, R, &S, FTyp);
		if (Lexem == _subrange)
		{
			if (N != 0) { *B = N; N = 0; }
			//B = (BYTE*)GetStore(sizeof(*B)); 
			B = new BYTE();
			*B = 0xFF; 
			StoreConst(R, &S, FTyp);
			RdLex(); 
			RdInConst(Z, R, &S, FTyp); 
			StoreConst(R, &S, FTyp);
		}
		else {
			//if (N == 0) B = (BYTE*)GetStore(sizeof(*B));
			if (N == 0) B = new BYTE();
			N++; 
			StoreConst(R, &S, FTyp);
		}
		if (Lexem != ']') { Accept(','); goto label1; }
		RdLex();
		if (N != 0) *B = N; 
		//*B = (BYTE)GetStore(sizeof(*B)); 
		B = new BYTE();
		*B = 0;
		FTyp = 'B';
	}
	return Z;
}


FrmlPtr RdBAnd(char& FTyp)
{
	FrmlPtr Z = RdComp(FTyp);
	while (Lexem == '&')
	{
		Z = BOperation(FTyp, _and, Z);
		Z->P2 = RdComp(FTyp);
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
		Z->P2 = RdBAnd(FTyp); 
		TestBool(FTyp);
	}
	return Z;
}

FrmlPtr RdFormula(char& FTyp)
{
	FrmlPtr Z = RdBOr(FTyp);
	while ((BYTE)Lexem == _limpl || (BYTE)Lexem == _lequ)
	{
		Z = BOperation(FTyp, Lexem, Z); 
		Z->P2 = RdBOr(FTyp); 
		TestBool(FTyp);
	}
	return Z;
}

bool FindFuncD(FrmlPtr* ZZ)
{
	char typ = '\0';
	FuncDPtr fc = FuncDRoot;
	while (fc != nullptr) {
		if (EquUpcase(fc->Name, LexWord)) {
			RdLex(); RdLex(); 
			FrmlElem* z = GetOp(_userfunc, 8); 
			z->FC = fc;
			LocVar* lv = fc->LVB.Root;
			WORD n = fc->LVB.NParam;
			for (WORD i = 1; i <= n; i++) {
				//FrmlList fl = (FrmlList)GetStore(sizeof(*fl));
				FrmlListEl* fl = new FrmlListEl();
				if (z->FrmlL == nullptr) z->FrmlL = fl;
				else ChainLast(z->FrmlL, fl);
				fl->Frml = RdFormula(typ);
				if (typ != lv->FTyp) OldError(12);
				lv = (LocVar*)lv->Chain;
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

FrmlPtr RdPrim(char& FTyp)
{
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

	char FunCode = '\0';;
	FrmlElem* Z = nullptr; FrmlElem* Z1 = nullptr; FrmlElem* Z2 = nullptr; FrmlElem* Z3 = nullptr;
	char Typ = '\0';
	integer I = 0, N = 0; BYTE* B = nullptr;
	pstring Options;

	switch (Lexem) {
	case _identifier: { SkipBlank(false);
		if (IsFun(R0Fun, LexWord, FunCode))
		{
			Z = GetOp(FunCode, 0); FTyp = 'R';
		}
		else if (IsFun(RCFun, LexWord, FunCode))
		{
			Z = GetOp(_getWORDvar, 1);
			Z->N01 = FunCode; FTyp = 'R';
		}
		else if (IsFun(S0Fun, LexWord, FunCode))
		{
			Z = GetOp(FunCode, 0); FTyp = 'S';
		}
		else if (IsFun(B0Fun, LexWord, FunCode))
		{
			Z = GetOp(FunCode, 0); FTyp = 'B';
		}
		else if (IsKeyWord("TRUE"))
		{
			Z = GetOp(_const, 1); Z->B = true; FTyp = 'B';
		}
		else if (IsKeyWord("FALSE"))
		{
			Z = GetOp(_const, 1); FTyp = 'B';
		}
		else if (!EquUpcase("OWNED") && (ForwChar == '('))
			if (FindFuncD(&Z)) FTyp = Z->FC->FTyp;
			else if (IsFun(S3Fun, LexWord, FunCode))
			{
				RdLex(); Z = GetOp(FunCode, 0); Z->P1 = RdAdd(FTyp);
				if ((BYTE)FunCode == _copy) TestString(FTyp);
				else { TestReal(FTyp); FTyp = 'S'; }
				Accept(','); Z->P2 = RdAdd(Typ);
				if (((BYTE)FunCode == _str) && (Typ == 'S')) goto label0;
				TestReal(Typ); Accept(',');
				Z->P3 = RdAdd(Typ); TestReal(Typ);
			label0:
				Accept(')');
			}
			else if (IsKeyWord("COND"))
			{
				RdLex(); Z2 = nullptr;
			label1:
				Z = GetOp(_cond, 0);
				if (!IsKeyWord("ELSE"))
				{
					Z->P1 = RdFormula(Typ); TestBool(Typ);
				}
				Accept(':');
				Z->P2 = RdAdd(Typ);
				if (Z2 == nullptr)
				{
					Z1 = Z; FTyp = Typ; if (Typ == 'B') OldError(70);
				}
				else {
					Z2->P3 = Z;
					if (FTyp == 'S') TestString(Typ); else TestReal(Typ);
				}
				if ((Z->P1 != nullptr) && (Lexem == ','))
				{
					RdLex(); Z2 = Z; goto label1;
				}
				Accept(')'); Z = Z1;
			}
			else if (IsKeyWord("MODULO"))
			{
				RdLex(); Z1 = RdAdd(Typ); TestString(Typ);
				Z = GetOp(_modulo, 2); Z->P1 = Z1; N = 0;
				do {
					Accept(',');
					B = (BYTE*)GetStore(2);
					*B = (BYTE)RdInteger;
					N++;
				} while (Lexem == ',');
				Accept(')'); Z->W11 = N; FTyp = 'B';
			}
			else if (IsKeyWord("SUM"))
			{
				RdLex(); if (FrmlSumEl != nullptr) OldError(74);
				if (ChainSumEl == nullptr) Error(28);
				FrmlSumEl = (SumElem*)GetStore(sizeof(SumElem));
				FrstSumVar = true; FrmlSumEl->Op = _const; FrmlSumEl->R = 0;
				FrmlSumEl->Frml = RdAdd(FTyp); TestReal(FTyp);
				Accept(')'); Z = FrmlPtr(&FrmlSumEl->Op);
				ChainSumEl(); FrmlSumEl = nullptr;
			}
			else if (IsKeyWord("DTEXT"))
			{
				RdLex(); Z1 = RdAdd(Typ); TestReal(Typ);
				Accept(','); TestLex(_identifier);
				for (I = 1; I < LexWord.length(); I++) LexWord[I] = toupper(LexWord[I]);
				goto label2;
			}
			else if (IsKeyWord("STRDATE"))
			{
				RdLex(); Z1 = RdAdd(Typ); TestReal(Typ);
				Accept(','); TestLex(_quotedstr);
			label2:
				Z = GetOp(_strdate1, LexWord.length() + 1); Z->P1 = Z1;
				Z->Mask = LexWord; RdLex();
				Accept(')'); FTyp = 'S';
			}
			else if (IsKeyWord("DATE"))
			{
				RdLex(); 
				Z = GetOp(_valdate, 9); 
				Z->Mask = "DD.MM.YY";
				goto label3;
			}
			else if (IsKeyWord("VALDATE"))
			{
				RdLex(); Z1 = RdAdd(Typ); TestString(Typ);
				Accept(','); TestLex(_quotedstr);
				Z = GetOp(_valdate, LexWord.length() + 1); Z->P1 = Z1;
				Z->Mask = LexWord; RdLex(); goto label4;
			}
			else if (IsKeyWord("REPLACE")) {
				RdLex(); Z1 = RdAdd(Typ); TestString(Typ);
				Accept(','); Z2 = RdAdd(Typ); TestString(Typ); Accept(',');
				Z3 = RdAdd(FTyp); TestString(FTyp); FunCode = _replace; goto label8;
			}
			else if (IsKeyWord("POS")) {
				RdLex(); Z1 = RdAdd(Typ); TestString(Typ); Accept(',');
				Z2 = RdAdd(Typ); TestString(Typ);
				Z3 = nullptr; FunCode = _pos; FTyp = 'R';
			label8:
				Options = ""; if (Lexem == ',') {
					RdLex();
					if (Lexem != ',')
					{
						TestLex(_quotedstr); Options = LexWord; RdLex();
					}
					if (((BYTE)FunCode == _pos) && (Lexem == ','))
					{
						RdLex(); Z3 = RdAdd(Typ); TestReal(Typ);
					}
				}
				Z = GetOp(FunCode, Options.length() + 1);
				Z->P1 = Z1; Z->P2 = Z2; Z->P3 = Z3; Z->Options = Options;
				Accept(')');
			}
			else if (IsFun(RS1Fun, LexWord, FunCode))
			{
				RdLex(); Z = GetOp(FunCode, 0);
			label3:
				Z->P1 = RdAdd(Typ); TestString(Typ); goto label4;
			}
			else if (IsFun(R1Fun, LexWord, FunCode))
			{
				RdLex(); Z = GetOp(FunCode, 0); Z->P1 = RdAdd(Typ); TestReal(Typ);
			label4:
				FTyp = 'R'; Accept(')');
			}
			else if (IsFun(R2Fun, LexWord, FunCode))
			{
				RdLex(); Z = GetOp(FunCode, 1); Z->P1 = RdAdd(Typ); TestReal(Typ);
				Accept(','); Z->P2 = RdAdd(Typ); TestReal(Typ);
				if ((Z->Op == _addwdays || Z->Op == _difwdays) && (Lexem == ','))
				{
					RdLex(); Z->N21 = RdInteger(); if (Z->N21 > 3) OldError(136);
				}
				goto label4;
			}
			else if (IsKeyWord("LEADCHAR"))
			{
				Z = GetOp(_leadchar, 2); goto label5;
			}
			else if (IsKeyWord("TRAILCHAR"))
			{
				Z = GetOp(_trailchar, 2);
			label5:
				RdLex(); Z->N11 = RdQuotedChar(); Accept(',');
				Z->P1 = RdAdd(Typ); TestString(Typ);
				if (Lexem == ',') {
					RdLex(); Z->N12 = RdQuotedChar();
				}
				goto label6;
			}
			else if (IsKeyWord("COPYLINE"))
			{
				Z = GetOp(_copyline, 0); goto label7;
			}
			else if (IsKeyWord("REPEATSTR"))
			{
				Z = GetOp(_repeatstr, 0);
			label7:
				RdLex(); Z->P1 = RdAdd(Typ); TestString(Typ); Accept(',');
				Z->P2 = RdAdd(Typ); TestReal(Typ);
				if ((Lexem == ',') && (Z->Op == _copyline)) {
					RdLex(); Z->P3 = RdAdd(Typ); TestReal(Typ);
				}
				goto label6;
			}
			else if (IsFun(S1Fun, LexWord, FunCode))
			{
				Z = GetOp(FunCode, 0); RdLex(); Z->P1 = RdAdd(FTyp);
				if (FunCode == _char) TestReal(FTyp); else TestString(FTyp);
			label6:
				FTyp = 'S'; Accept(')');
			}
			else if (IsKeyWord("TRUST")) {
				Z = GetOp(_trust, 0); RdByteListInStore(); FTyp = 'B';
			}
			else if (IsKeyWord("EQUMASK")) {
				Z = GetOp(_equmask, 0); FTyp = 'B'; RdLex();
				Z->P1 = RdAdd(Typ); TestString(Typ); Accept(',');
				Z->P2 = RdAdd(Typ); TestString(Typ); Accept(')');
			}
			else if (RdFunction != nullptr) Z = RdFunction(FTyp);
			else Error(75);
		else {
			if (RdFldNameFrml == nullptr) Error(110);
			else {
				Z = RdFldNameFrml(FTyp); // volani ukazatele na funkci
				if ((Z->Op != _access) || (Z->LD != nullptr)) FrstSumVar = false;
			}
		}
		break;
	}
	case '^': { 
		RdLex(); 
		Z = GetOp(_lneg, 0);
		Z->P1 = RdPrim(FTyp); 
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
		RdLex(); if (Lexem == '-') Error(7); 
		Z = GetOp(_unminus, 0);
		Z->P1 = RdPrim(FTyp); 
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
		Z = GetOp(_const, LexWord.length() + 1);
		FTyp = 'S';
		Z->S = LexWord; 
		RdLex();
		break;
	}
	default: {
		FTyp = 'R';
		Z = GetOp(_const, sizeof(double));
		Z->R = RdRealConst();
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

FrmlPtr MyBPContext(FrmlPtr Z, bool NewMyBP)
{
	FrmlPtr Z1;
	if (NewMyBP) {
		Z1 = GetOp(_setmybp, 0); Z1->P1 = Z; Z = Z1;
	}
	return Z;
}

FrmlList RdFL(bool NewMyBP, FrmlList FL1)
{
	char FTyp;
	KeyFldDPtr KF = CViewKey->KFlds;
	FrmlList FLRoot = nullptr;
	KeyFldDPtr KF2 = (KeyFldD*)KF->Chain;
	bool FVA = FileVarsAllowed;
	FileVarsAllowed = false;
	bool b = FL1 != nullptr;
	if (KF2 != nullptr) Accept('(');
label1:
	FrmlList FL = (FrmlListEl*)GetStore(sizeof(*FL)); 
	ChainLast(FLRoot, FL);
	FL->Frml = MyBPContext(RdFrml(FTyp), NewMyBP);
	if (FTyp != KF->FldD->FrmlTyp) OldError(12); KF = (KeyFldD*)KF->Chain;
	if (b) {
		FL1 = (FrmlListEl*)FL1->Chain; 
		if (FL1 != nullptr) { Accept(','); goto label1; }
	}
	else if ((KF != nullptr) && (Lexem == ',')) { RdLex(); goto label1; }
	if (KF2 != nullptr) Accept(')'); auto result = FLRoot;
	FileVarsAllowed = FVA;
	return result;
}

FrmlPtr RdKeyInBool(KeyInD* KIRoot, bool NewMyBP, bool FromRdProc, bool& SQLFilter)
{
	KeyInD* KI; WORD l; char FTyp; FrmlPtr Z; bool FVA;
	FrmlPtr result = nullptr; KIRoot = nullptr; SQLFilter = false;
	if (FromRdProc) {
		FVA = FileVarsAllowed; FileVarsAllowed = true;
		if ((Lexem == _identifier) && (ForwChar == '(') and
			(EquUpcase("EVALB") || EquUpcase("EVALS") || EquUpcase("EVALR")))
			FileVarsAllowed = false;
	}
	if (IsKeyWord("KEY")) {
		AcceptKeyWord("IN");
		if ((CFile->Typ != 'X') || (CViewKey == nullptr)) OldError(118);
		if (CViewKey->KFlds == nullptr) OldError(176);
		Accept('['); l = CViewKey->IndexLen + 1;
	label1:
		KI = (KeyInD*)GetZStore(sizeof(KeyInD));
		ChainLast(KIRoot, KI);
		KI->X1 = (pstring*)GetZStore(l);
		KI->X2 = (pstring*)GetZStore(l + 1);
		KI->FL1 = RdFL(NewMyBP, nullptr);
		if (Lexem == _subrange) {
			RdLex(); KI->FL2 = RdFL(NewMyBP, KI->FL1);
		}
		if (Lexem == ',') { RdLex(); goto label1; }
		Accept(']');
		if (Lexem == '&') { RdLex(); goto label2; }
	}
	else {
	label2:
		FrmlSumEl = nullptr; Z = RdFormula(FTyp);
		if (CFile->typSQLFile && (FTyp == 'S')) SQLFilter = true;
		else {
			TestBool(FTyp);
			if (Z->Op == _eval) Z->EvalFD = CFile;
		}
		result = MyBPContext(Z, NewMyBP && ((BYTE)Z->Op != _eval));
	}
	if (FromRdProc) FileVarsAllowed = FVA;
	return result;
}

FrmlPtr RdFrml(char& FTyp)
{
	FrmlSumEl = nullptr; return RdFormula(FTyp);
}

FrmlPtr RdBool()
{
	char FTyp;
	FrmlSumEl = nullptr; auto result = RdFormula(FTyp); TestBool(FTyp);
	return result;
}

FrmlPtr RdRealFrml()
{
	char FTyp;
	FrmlSumEl = nullptr; auto result = RdAdd(FTyp); TestReal(FTyp);
	return result;
}

FrmlPtr RdStrFrml()
{
	char FTyp;
	FrmlSumEl = nullptr; auto result = RdAdd(FTyp); TestString(FTyp);
	return result;
}

FrmlPtr GetOp(BYTE Op, integer BytesAfter)
{
	WORD l;
	if (Op < 0x60) l = 1;
	else if (Op < 0xb0) l = 5;
	else if (Op < 0xf0) l = 9;
	else l = 13;
	//Z = (FrmlPtr)GetZStore(l + BytesAfter);
	FrmlElem* Z = new FrmlElem();
	Z->Op = Op;
	return Z;
}

FieldDPtr FindFldName(FileDPtr FD)
{
	FieldDPtr F = FD->FldD;
	while (F != nullptr) {
		{
			if (EquUpcase(F->Name, LexWord)) goto label1;
			F = (FieldDescr*)F->Chain;
		}
	}
label1:
	return F;
}

FieldDPtr RdFldName(FileDPtr FD)
{
	FieldDPtr F;
	TestIdentif(); F = FindFldName(FD);
	if (F == nullptr) { Set2MsgPar(LexWord, FD->Name); Error(87); }
	RdLex(); return F;
}

FileDPtr FindFileD()
{
	FileD* FD = nullptr; RdbD* R = nullptr; LocVar* LV = nullptr;
	if (FDLocVarAllowed && FindLocVar(LVBD.Root, &LV) && (LV->FTyp == 'f'))
	{
		return LV->FD;
	}
	R = CRdb;
	while (R != nullptr) {
		FD = R->FD;
		while (FD != nullptr) {
			if (EquUpcase(FD->Name, LexWord)) { return FD; }
			FD = (FileD*)FD->Chain;
		}
		R = R->ChainBack;
	}
	if (EquUpcase("CATALOG")) return CatFD; 
	return nullptr;
}

FileD* RdFileName()
{
	FileDPtr FD;
	if (SpecFDNameAllowed && (Lexem == '@'))
	{
		LexWord = '@'; Lexem = _identifier;
	}
	TestIdentif(); FD = FindFileD();
	if ((FD == nullptr) || (FD == CRdb->FD) && !SpecFDNameAllowed) Error(9);
	RdLex(); return FD;
}

LinkDPtr FindLD(pstring RoleName)
{
	LinkDPtr L;
	L = LinkDRoot;
	while (L != nullptr) {
		if ((L->FromFD == CFile) && SEquUpcase(L->RoleName, RoleName)) {
			return L;
		}
		L = L->Chain;
	}
	return nullptr;
}

bool IsRoleName(bool Both, FileDPtr& FD, LinkDPtr& LD)
{
	TestIdentif();
	FD = FindFileD(); auto result = true;
	if ((FD != nullptr) && FD->IsParFile) { RdLex(); LD = nullptr; return result; }
	if (Both)
	{
		LD = FindLD(LexWord);
		if (LD != nullptr) { RdLex(); FD = LD->ToFD; return result; }
	}
	result = false;
	return result;
}

FrmlPtr RdFAccess(FileDPtr FD, LinkD* LD, char& FTyp)
{
	FrmlPtr Z; FileDPtr cf; bool fa;
	TestIdentif();
	Z = GetOp(_access, 12);
	Z->File2 = FD;
	Z->LD = LD;
	if ((LD != nullptr) && EquUpcase("EXIST")) { RdLex(); FTyp = 'B'; }
	else {
		cf = CFile;
		CFile = FD;
		fa = FileVarsAllowed;
		FileVarsAllowed = true;
		Z->P1 = RdFldNameFrmlF(FTyp);
		CFile = cf;
		FileVarsAllowed = fa;
	}
	return Z;
}

FrmlPtr FrmlContxt(FrmlPtr Z, FileDPtr FD, void* RP)
{
	FrmlElem* Z1 = GetOp(_newfile, 8); 
	Z1->Frml = Z;
	Z1->NewFile = FD; 
	Z1->NewRP = RP; 
	return Z1;
}

FrmlPtr MakeFldFrml(FieldDPtr F, char& FTyp)
{
	FrmlElem* Z = GetOp(_field, 4);
	Z->Field = F; 
	FTyp = F->FrmlTyp; 
	return Z;
}

LinkDPtr FindOwnLD(FileDPtr FD, const pstring& RoleName)
{
	LinkDPtr ld;
	LinkDPtr result = nullptr;
	ld = LinkDRoot;
	while (ld != nullptr) {
		if ((ld->ToFD == FD) && EquUpcase(ld->FromFD->Name, LexWord) &&
			(ld->IndexRoot != 0) && SEquUpcase(ld->RoleName, RoleName)) goto label1;
		ld = ld->Chain;
	}
label1:
	RdLex();
	return ld;
}

FrmlPtr TryRdFldFrml(FileDPtr FD, char& FTyp)
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
		z = GetOp(_owned, 12);
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
		z->ownLD = ld;
		cf = CFile;
		CFile = ld->FromFD;
		if (Lexem == '.') {
			RdLex();
			z->ownSum = RdFldNameFrmlF(FTyp);
			if (FTyp != 'R') OldError(20);
		}
		if (Lexem == ':') {
			RdLex();
			z->ownBool = RdFormula(typ);
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
	LinkDPtr ld;
	FileDPtr fd;
	FrmlPtr z;

	if (IsForwPoint())
	{
		if (!IsRoleName(FileVarsAllowed, fd, ld)) Error(9);
		RdLex();
		return RdFAccess(fd, ld, FTyp);
	}
	if (!FileVarsAllowed) Error(110);
	z = TryRdFldFrml(CFile, FTyp);
	if (z == nullptr) Error(8);
	return z;
}
