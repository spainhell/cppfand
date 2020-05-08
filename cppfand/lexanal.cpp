#include "lexanal.h"

#include "access.h"
#include "compile.h"
#include "drivers.h"
#include "kbdww.h"
#include "legacy.h"
#include "rdmix.h"
#include "recacc.h"
#include "runfrml.h"

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
		SimpleEditText('T', ErrMsg, HdTxt, (CharArr*)p, 0xfff, l, i, upd);
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
	InpArrLen = S.length(); InpArrPtr = CharArrPtr(&S[1]);
	if (InpArrLen == 0) ForwChar = 0x1A; else ForwChar = *InpArrPtr[1];
	CurrPos = 1; FillChar(&InpRdbPos, sizeof(InpRdbPos), 0);
}

void SetInpLongStr(LongStr* S, bool ShowErr)
{
	InpArrLen = S->LL;
	InpArrPtr = CharArrPtr(*S->A);
	if (InpArrLen == 0) ForwChar = 0x1A; else ForwChar = *InpArrPtr[1];
	CurrPos = 1; InpRdbPos.R = nullptr;
	if (ShowErr) InpRdbPos.R = nullptr; // TODO: tady bylo InpRdbPos.R:=ptr(0,1);
	InpRdbPos.IRec = 0;
}

void SetInpTTPos(longint Pos, bool Decode)
{
	LongStrPtr s;
	s = CFile->TF->Read(2, Pos);
	if (Decode) CodingLongStr(s);
	InpArrLen = s->LL;
	InpArrPtr = CharArrPtr(s->A);
	if (InpArrLen == 0) ForwChar = 0x1A; else ForwChar = *InpArrPtr[1];
	CurrPos = 1;
}

void SetInpTT(RdbPos RP, bool FromTxt)
{
	longint Pos;  FileDPtr CF; void* CR; LongStrPtr s;

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
	if (pos > InpArrLen) ForwChar = 0x1A; else ForwChar = *InpArrPtr[pos];
	CurrPos = pos;
}

void ReadChar()
{
	CurrChar = ForwChar;
	if (CurrPos < InpArrLen)
	{
		CurrPos++; ForwChar = *InpArrPtr[CurrPos];
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
	while ((s.length() < 12) && (isalpha(ForwChar) || isdigit(ForwChar)))
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
			if (CurrPos <= InpArrLen) ForwChar = *InpArrPtr[CurrPos];
			goto label1;
		}
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
	}
	default:
		if (ForwChar >= 0x00 && ForwChar <= 0x20) // ^@..' '
		{
			if (toNextLine && (ForwChar == 0x0D)) // ^M = CR = 013 = 0x0D
			{
				ReadChar(); if (ForwChar == 0x0A) ReadChar(); // ^J = LF = 010 = 0x0A
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
	CurrPos = OldErrPos; Error(N);
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
	WORD i;
	OldErrPos = CurrPos; SkipBlank(false); ReadChar(); Lexem = CurrChar;
	if (std::isalpha(CurrChar))
	{
		Lexem = _identifier; LexWord[1] = CurrChar; i = 1;
		while (isalpha(ForwChar) || isdigit(ForwChar))
		{
			i++; if (i > 32) Error(2); ReadChar(); LexWord[i] = CurrChar;
		}
		LexWord[0] = char(i);
	}
	else if (isdigit(CurrChar))
	{
		Lexem = _number; LexWord[1] = CurrChar; i = 1;
		while (isdigit(ForwChar))
		{
			i++; if (i > 15) Error(6); ReadChar(); LexWord[i] = CurrChar;
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
			if (LexWord.length() == sizeof(LexWord) - 1) Error(6);
			if (CurrChar == '\'') ReadChar();
			else if (CurrChar == '\\') RdBackSlashCode();
			LexWord = LexWord + CurrChar; ReadChar();
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
	return (ForwChar == '.') && (*InpArrPtr[CurrPos + 1] != '.');
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
	/*asm  mov al, X; cmp al, Lexem; je @1;
	mov ExpChar, al; mov ax, 1; push ax; call Error;
	@1:  call RdLex;*/
}

integer RdInteger()
{
	integer I, J;
	val(LexWord, I, J); if (J != 0) Lexem = 0 /* != _number*/;
	Accept(_number);
	return I;
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
		if ((Lexem != _subrange) && (ForwChar >= 0 && ForwChar <= 9)) {
			S = S + Lexem; RdLex(); S = S + LexWord;
			goto label1;
		}
		return ValofS(S);
	}
	if ((ForwChar == 'e' || ForwChar == 'E')
		&& (*InpArrPtr[CurrPos + 1] == '-' || (ForwChar >= 0 && ForwChar <= 9))) {
		S = S + "e"; ReadChar(); if (ForwChar == '-') { ReadChar(); S = S + "-"; }
		RdLex(); TestLex(_number); S = S + LexWord;
	}
	RdLex();
	return ValofS(S);
}

double ValofS(pstring& S)
{
	integer I; double R;

	val(S, R, I); if (I != 0) {
		R = ValDate(S, "DD.MM.YY"); if (R == 0) {
			R = ValDate(S, "DD.MM.YYYY"); if (R == 0) {
				R = ValDate(S, "mm hh:ss.tt"); if (R == 0) Error(7);
			}
		}
	}
	return R;
}

bool EquUpcase(pstring& S)
{
	// TODO
	/*asm  lea si, LexWord; les di, S; cld;xor ch, ch; mov cl, [si]; cmpsb; jnz @3;
	jcxz @2;xor bh, bh;
	@1:  mov bl, [si]; mov al, BYTE PTR UpcCharTab[bx]; mov bl, es: [di] ;
	cmp al, BYTE PTR UpcCharTab[bx]; jnz @3; inc si; inc di; loop @1;
	@2:  mov ax, 1; jmp @4;
	@3: xor ax, ax;
	@4:  end;*/
	return false;
}

bool EquUpcase(const char* S)
{
	pstring temp = S;
	return EquUpcase(temp);
}

bool TestKeyWord(pstring S)
{
	return (Lexem == _identifier) && EquUpcase(S);
}

bool IsKeyWord(pstring S)
{
	//TODO
	/*asm  cmp Lexem, _identifier; jne @3;
	lea si, LexWord; les di, S; cld;xor ch, ch; mov cl, [si]; cmpsb; jnz @3;
	jcxz @2;xor bh, bh;
	@1:  mov bl, [si]; mov al, BYTE PTR UpcCharTab[bx]; cmp al, es: [di] ; jnz @3;
	inc si; inc di; loop @1;
	@2:  call RdLex; mov ax, 1; jmp @4;
	@3: xor ax, ax;
	@4:  end;*/
	return false;
}

void AcceptKeyWord(pstring S)
{
	if (TestKeyWord(S)) RdLex();
	else { SetMsgPar(S); Error(33); }
}

bool IsOpt(pstring S)
{
	//// TODO
	//asm  cmp Lexem, _identifier; jne @3;
	//lea si, LexWord; les di, S; cld;xor ch, ch; mov cl, [si]; cmpsb; jnz @3;
	//jcxz @2;xor bh, bh;
	//@1:  mov bl, [si]; mov al, BYTE PTR UpcCharTab[bx]; cmp al, es: [di] ; jnz @3;
	//inc si; inc di; loop @1;
	//@2:  call RdLex; mov ax, _equ; push ax; call Accept; mov ax, 1; jmp @4;
	//@3: xor ax, ax;
	//@4:;
	return false;
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
	pstring* S = nullptr;
	S = StoreStr(LexWord);
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
	if ((S.length() == 0) || !isalpha(S[1])) return false;
	for (i = 2; i < S.length(); i++) {
		if (!isalpha(S[i]) || isdigit(S[i])) return false;
	}
	return true;
}













