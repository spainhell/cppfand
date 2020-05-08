#include "rdfrml.h"

#include "lexanal.h"
#include "rdfildcl.h"

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

FrmlPtr RdPrim(char& FTyp)
{
	const BYTE R0FunN = 16; // { alphabetic ordered lower case names }
	pstring R0Fun[R0FunN] = { "cprinter","currtime", "edrecno","exitcode","getmaxx","getmaxy",
		"maxcol","maxrow","memavail","mousex","mousey", "pi","random","today","txtpos","txtxy" };
	char R0Code[R0FunN] = { _cprinter,_currtime, _edrecno,_exitcode,_getmaxx,_getmaxy,
		_maxcol,_maxrow,_memavail,_mousex,_mousey, _pi,_random,_today,_txtpos,_txtxy };
	const BYTE RCFunN = 5;
	pstring RCFun[RCFunN] = { "edbreak","edirec","menux","menuy","usercode" };
	char RCCode[RCFunN] = { 3, 4, 5, 6, 7 };
	const BYTE S0FunN = 12;
	pstring S0Fun[S0FunN] = { "accright","clipbd","edbool","edfield","edfile","edkey",
		"edreckey","keybuf","passWORD","readkey","username","version" };
	char S0Code[S0FunN] = { _accright,_clipbd,_edbool,_edfield,	_edfile,_edkey,_edreckey,
		_keybuf,_passWORD,_readkey,_username,_version };
	const BYTE B0FunN = 2;
	pstring B0Fun[B0FunN] = { "isnewrec","testmode" };
	char B0Code[B0FunN] = { _isnewrec,_testmode };
	const BYTE S1FunN = 5;
	pstring S1Fun[S1FunN] = { "char","getenv", "lowcase","nodiakr","upcase" };
	char S1Code[S1FunN] = { _char,_getenv,_lowcase,_nodiakr,_upcase };
	const BYTE R1FunN = 12;
	pstring R1Fun[R1FunN] = { "abs","arctan","color","cos","exp","frac","int",
		"ln","sin","sqr","sqrt","typeday" };
	char R1Code[R1FunN] = { _abs,_arctan,_color,_cos,_exp,_frac,_int,_ln,_sin,_sqr,_sqrt,_typeday };
	const BYTE R2FunN = 4;
	pstring R2Fun[R2FunN] = { "addmonth","addwdays","difmonth","difwdays" };
	char R2Code[R2FunN] = { _addmonth,_addwdays,_difmonth,_difwdays };
	const BYTE RS1FunN = 5;
	pstring RS1Fun[RS1FunN] = { "diskfree","length","linecnt","ord","val" };
	char RS1Code[RS1FunN] = { _diskfree,_length,_linecnt,_ord,_val };
	const BYTE S3FunN = 5;
	pstring S3Fun[S3FunN] = { "copy","str","text" };
	char S3Code[S3FunN] = { _copy,_str,_str };

	char FunCode;
	FrmlPtr Z = nullptr, Z1 = nullptr, Z2 = nullptr, Z3 = nullptr;
	char Typ;
	integer I, N; BYTE* B;
	pstring Options(5);

	switch (Lexem) {
	case _identifier: { SkipBlank(false);
		if (IsFun(R0Fun, R0FunN, R0Code, FunCode))
		{
			Z = GetOp(FunCode, 0); FTyp = 'R';
		}
		else if (IsFun(RCFun, RCFunN, RCCode, FunCode))
		{
			Z = GetOp(_getWORDvar, 1); Z->N01 = FunCode; FTyp = 'R';
		}
		else if (IsFun(S0Fun, S0FunN, S0Code, FunCode))
		{
			Z = GetOp(FunCode, 0); FTyp = 'S';
		}
		else if (IsFun(B0Fun, B0FunN, B0Code, FunCode))
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
			else if (IsFun(S3Fun, S3FunN, S3Code, FunCode))
			{
				RdLex(); Z = GetOp(FunCode, 0); Z->P1 = RdAdd(FTyp);
				if (FunCode == _copy) TestString(FTyp);
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
				if (not IsKeyWord("ELSE"))
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
				Z = GetOp(_strdate, LexWord.length() + 1); Z->P1 = Z1;
				Z->Mask = LexWord; RdLex();
				Accept(')'); FTyp = 'S';
			}
			else if (IsKeyWord("DATE"))
			{
				RdLex(); Z = GetOp(_valdate, 9); Z->Mask = "DD.MM.YY";
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
			else if (IsFun(RS1Fun, RS1FunN, RS1Code, FunCode))
			{
				RdLex(); Z = GetOp(FunCode, 0);
			label3:
				Z->P1 = RdAdd(Typ); TestString(Typ); goto label4;
			}
			else if (IsFun(R1Fun, R1FunN, R1Code, FunCode))
			{
				RdLex(); Z = GetOp(FunCode, 0); Z->P1 = RdAdd(Typ); TestReal(Typ);
			label4:
				FTyp = 'R'; Accept(')');
			}
			else if (IsFun(R2Fun, R2FunN, R2Code, FunCode))
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
			else if (IsFun(S1Fun, S1FunN, S1Code, FunCode))
			{
				Z = GetOp(FunCode, 0); RdLex(); Z->P1 = RdAdd(FTyp);
				if (FunCode == _char) TestReal(FTyp); else TestString(FTyp);
			label6:
				FTyp = 'S'; Accept(')');
			}
			else if (IsKeyWord("TRUST")) {
				Z = GetOp(_trust, 0); RdByteListInStore; FTyp = 'B';
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
			Z = RdFldNameFrml(FTyp);
			if ((Z->Op != _access) || (Z->LD != nullptr)) FrstSumVar = false;
		}
		break;
	}
	case '^': { RdLex(); Z = GetOp(_lneg, 0); Z->P1 = RdPrim(FTyp); TestBool(FTyp); break; }
	case '(': { RdLex(); Z = RdFormula(FTyp); Accept(')'); break; }
	case '-': {
		RdLex(); if (Lexem == '-') Error(7); Z = GetOp(_unminus, 0);
		Z->P1 = RdPrim(FTyp); TestReal(FTyp);
		break;
	}
	case '+': {
		RdLex(); if (Lexem == '+') Error(7); Z = RdPrim(FTyp);
		TestReal(FTyp);
		break;
	}
	case _quotedstr: {
		Z = GetOp(_const, LexWord.length() + 1);
		FTyp = 'S'; Z->S = LexWord; RdLex();
		break;
	}
	default: FTyp = 'R'; Z = GetOp(_const, sizeof(double));
		Z->R = RdRealConst();
	}
	return Z;
}

bool FindFuncD(FrmlPtr* ZZ)
{
	char typ = '\0';
	FuncDPtr fc = FuncDRoot;
	while (fc != nullptr) {
		if (EquUpcase(fc->Name)) {
			RdLex(); RdLex(); FrmlPtr z = GetOp(_userfunc, 8); z->FC = fc;
			LocVar* lv = fc->LVB.Root;
			WORD n = fc->LVB.NParam;
			for (WORD i = 1; i < n; i++) {
				FrmlList fl = (FrmlList)GetStore(sizeof(*fl));
				ChainLast(z->FrmlL, fl);
				fl->Frml = RdFormula(typ); if (typ != lv->FTyp) OldError(12);
				lv = lv->Chain; if (i < n) Accept(',');
			}
			Accept(')');
			ZZ = &z;
			return true;
		}
		fc = fc->Chain;
	}
	return false;
}

bool IsFun(void* XFun, BYTE N, void* XCode, char& FunCode)
{
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
	return false;
}

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
		if (EquUpcase(QQdiv)) { Z = GetOp(_div, 0); goto label2; }
		else if (EquUpcase(QQmod)) { Z = GetOp(_mod, 0); goto label2; }
		else if (EquUpcase(QQround)) {
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
	FrmlPtr Z, Z1;
	Z = RdMult(FTyp);
label1:
	switch (Lexem) {
	case '+': { Z1 = Z;
		if (FTyp == 'R') { Z = GetOp(_plus, 0); goto label2; }
		else {
			Z = GetOp(_concat, 0); TestString(FTyp); RdLex(); Z->P1 = Z1;
			Z->P2 = RdMult(FTyp); TestString(FTyp); goto label1;
		}
		break;
		}
	case '-': { Z1 = Z; Z = GetOp(_minus, 0); TestReal(FTyp);
	label2:
		RdLex(); Z->P1 = Z1; Z->P2 = RdMult(FTyp); TestReal(FTyp); goto label1;
		break; }
	}
	return Z;
}

FrmlPtr RdComp(char& FTyp)
{
	FrmlPtr Z;
	double R;
	pstring S;
	BYTE* B = new BYTE;
	integer N;
	FrmlPtr Z1;
	Z = RdAdd(FTyp);
	Z1 = Z;
	if (Lexem >= _equ && Lexem <= _ne)
		if (FTyp == 'R')
		{
			Z = GetOp(_compreal, 2); Z->P1 = Z1;
			Z->N21 = Lexem; RdLex(); Z->N22 = RdPrecision();
			Z->P2 = RdAdd(FTyp); TestReal(FTyp); FTyp = 'B';
		}
		else {
			TestString(FTyp); Z = GetOp(_compstr, 2); Z->P1 = Z1;
			Z->N21 = Lexem; RdLex(); Z->N22 = RdTilde();
			Z->P2 = RdAdd(FTyp); TestString(FTyp); FTyp = 'B';
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
			B = (BYTE*)GetStore(sizeof(*B)); *B = 0xFF; StoreConst(R, &S, FTyp);
			RdLex(); RdInConst(Z, R, &S, FTyp); StoreConst(R, &S, FTyp);
		}
		else {
			if (N == 0) B = (BYTE*)GetStore(sizeof(*B));
			N++; StoreConst(R, &S, FTyp);
		}
		if (Lexem != ']') { Accept(','); goto label1; }
		RdLex();
		if (N != 0) *B = N; *B = (BYTE)GetStore(sizeof(*B)); *B = 0;
		FTyp = 'B';
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
	case 'R': { RPtr = (double*)GetStore(sizeof(*RPtr)); *RPtr = R; break; }
	}
}

FrmlPtr BOperation(char Typ, char Fun, FrmlPtr Frml)
{
	FrmlPtr Z;
	TestBool(Typ); Z = GetOp(Fun, 0);
	RdLex(); Z->P1 = Frml; return Z;
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
		Z = BOperation(FTyp, _or, Z); Z->P2 = RdBAnd(FTyp); TestBool(FTyp);
	}
	return Z;
}

FrmlPtr RdFormula(char& FTyp)
{
	FrmlPtr Z = RdBOr(FTyp);
	while ((BYTE)Lexem == _limpl || (BYTE)Lexem == _lequ)
	{
		Z = BOperation(FTyp, Lexem, Z); Z->P2 = RdBOr(FTyp); TestBool(FTyp);
	}
	return Z;
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
	KeyFldDPtr KF2 = KF->Chain;
	bool FVA = FileVarsAllowed;
	FileVarsAllowed = false;
	bool b = FL1 != nullptr;
	if (KF2 != nullptr) Accept('(');
label1:
	FrmlList FL = (FrmlListEl*)GetStore(sizeof(*FL)); ChainLast(FLRoot, FL);
	FL->Frml = MyBPContext(RdFrml(FTyp), NewMyBP);
	if (FTyp != KF->FldD->FrmlTyp) OldError(12); KF = KF->Chain;
	if (b) {
		FL1 = FL1->Chain; if (FL1 != nullptr) { Accept(','); goto label1; }
	}
	else if ((KF != nullptr) && (Lexem == ',')) { RdLex(); goto label1; }
	if (KF2 != nullptr) Accept(')'); auto result = FLRoot;
	FileVarsAllowed = FVA;
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





