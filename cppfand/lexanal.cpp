#include "lexanal.h"

#include "access.h"
#include "common.h"
#include "drivers.h"
#include "kbdww.h"
#include "legacy.h"
#include "memory.h"
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
	Drivers::ClearKbdBuf(); l = InpArrLen; i = CurrPos;
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
		SimpleEditText("T", ErrMsg, HdTxt, p, 0xfff, l, i, upd);
		PopW(w);
		ReleaseStore(p1);
	}
	EdRecKey = ErrMsg;
	LastExitCode = i + 1;
	IsCompileErr = true;
	MsgLine = ErrMsg;
	GoExit();
}

void SetInpLongStr(LongStrPtr S, bool ShowErr)
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
	InpArrPtr = CharArrPtr(@s->A);
	if (InpArrLen == 0) ForwChar = 0x1A; else ForwChar = *InpArrPtr[1];
	CurrPos = 1;
}

void SetInpTT(RdbPos RP, bool FromTxt)
{
	longint Pos;  FileDPtr CF; void* CR; LongStrPtr s;

	if (RP.IRec == 0) {
		SetInpLongStr(runfrml::RunLongStr(FrmlPtr(RP.R)), true);
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
		res = FindChpt("I", s, false, ChptIPos);
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
			ci = PrevCompInp; move(ci->ChainBack, PrevCompInp, sizeof(CompInpD));
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
				move(PrevCompInp, ci, sizeof(CompInpD));
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
