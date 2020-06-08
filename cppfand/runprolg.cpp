#include "runprolg.h"
#include "runproj.h"
#include "compile.h"

TVarDcl* VarDcls; integer VarCount;
WORD IntDom, RealDom, StrDom, LongStrDom, BoolDom, LexDom, LLexDom; /*PDomain*/
WORD MemPred, LenPred, InvPred, AddPred, DelPred;
WORD UnionPred, MinusPred, InterPred;
WORD TxtPred; /*PPredicate*/
WORD UnderscoreTerm; /*PPTerm*/
bool UnbdVarsInTerm, WasUnbd, WasOp;
TProgRoots* Roots;
char* PackedTermPtr;
WORD PTPMaxOfs;
WORD ClausePreds;

void ChainLst(void* Root, WORD NewOfs) // assembler
{
	// ES:DI = Root;
	// ...
}

WORD OOfs(void* p)
{
	return ((PtrRec(p).Seg - _Sg) << 4) + PtrRec(p).Ofs;
}

WORD GetZStor(WORD Sz)
{
	void* p = GetZStore(Sz);
	return OOfs(p);
}

void* OPtr(WORD Sg, void* p)
{
	return ptr(Sg, ((PtrRec(p).Seg - Sg) << 4) + PtrRec(p).Ofs);
}

WORD StorStr(pstring S) // assembler
{
	return 0;
}

/*  L E X A N A L  =========================================================*/
bool IsCharUpper2(char C)
{
	return UpcCharTab[C] == C;
}

bool IsUpperIdentif()
{
	return (Lexem == _identifier) && IsCharUpper(LexWord[1]);
}

void RdLex()
{
	WORD i = 0;
	OldErrPos = CurrPos;
	SkipBlank(false);
	ReadChar();
	Lexem = CurrChar;
	switch (CurrChar) {
	case '\'': {
		Lexem = _quotedstr; ReadChar(); LexWord = "";
		while ((CurrChar != '\'') || (ForwChar == '\'')) {
			if (CurrChar == 0x1A) Error(17);
			if (LexWord.length() == pred(sizeof(LexWord))) Error(6);
			if (CurrChar == '\'') ReadChar();
			else if (CurrChar == '\\') RdBackSlashCode();
			LexWord.Append(CurrChar);
			ReadChar();
		}
		break;
	}
	case ':': {
		if (ForwChar == '-') {
			ReadChar();
			Lexem = _assign;
		}
		break;
	}
	case '|': {
		if (ForwChar == '|') { ReadChar(); Lexem = _or; }
		break;
	}
	case '&': {
		if (ForwChar == '&') { ReadChar(); Lexem = _and; }
		break;
	}
	default:
		if (IsLetter(CurrChar)) {
			Lexem = _identifier; LexWord[1] = CurrChar; i = 1;
			while (IsLetter(ForwChar) || isdigit(ForwChar)) {
				i++;
				if (i > 32) Error(2);
				ReadChar();
				LexWord[i] = CurrChar;
			}
			LexWord[0] = char(i);
		}
		else if (isdigit(CurrChar)) {
		label1:
			Lexem = _number;
			LexWord[1] = CurrChar;
			i = 1;
			while (isdigit(ForwChar)) {
				i++;
				if (i > 9) Error(3);
				ReadChar();
				LexWord[i] = CurrChar;
			}
			LexWord[0] = char(i);
		}
	}
}

void TestIdentif()
{
	if (Lexem != _identifier) Error(29);
}

void Accept(char X)
{
	if (Lexem != X)
		if (X == _assign) Error(506);
		else {
			ExpChar = X;
			Error(1);
		}
	RdLex();
}

bool TestKeyWord(pstring s)
{
	return (Lexem == _identifier) && (LexWord == s);
}

bool IsKeyWord(pstring s)
{
	if (TestKeyWord(s)) { RdLex(); return true; }
	return false;
}

void AcceptKeyWord(pstring s)
{
	if (!IsKeyWord(s)) { SetMsgPar(s); Error(33); }
}

integer RdInteger()
{
	integer i = 0, j = 0;
	if (Lexem != _number) Error(525);
	val(LexWord, i, j);
	RdLex();
	return i;
}

/*  T D O M A I N  =========================================================*/
TFunDcl* GetFunDclByName(WORD D, BYTE& I)
{
	WORD fd = (TDomain*)(ptr(_Sg, D))->FunDcl;
	I = 0;

	while ((fd != 0) &&
		(*(pstring*)(ptr(_Sg, (TFunDcl*)(ptr(_Sg, fd))->Name)) != LexWord))
		do {
			fd = (TFunDcl*)(ptr(_Sg, fd))->Chain;
			I++;
		}

	if (fd == 0) return nullptr;
	else return ptr(_Sg, fd);
}

WORD GetOrigDomain(WORD D)
{
	if (D != 0)
		while ((TDomain*)(ptr(_Sg, D))->Typ = _RedefD)
			D = (TDomain*)(ptr(_Sg, D))->OrigDom;
	return D;
}

/*  T D A T A B A S E  =====================================================*/
WORD FindDataBase(pstring S)
{
	TDatabase* db = nullptr;
	WORD next = 0;

	db = ptr(_Sg, Roots->Databases);
	while (db->Chain != 0) {
		if (db->Name == S) goto label1;
		next = db->Chain;
	}
label1:
	return db->Chain;
}

/*  T P R O G R A M  =======================================================*/
WORD FindConst(WORD D)
{
	TConst* p = ptr(_Sg, Roots->Consts);
	WORD next = p->Chain;
	while (next != 0) {
		if ((p->Dom == D) && (p->Name == LexWord)) { return p->Expr; }
		next = p->Chain;
	}
	return 0;
}

bool RdConst(WORD D, WORD& RT)
{
	WORD tofs;
	if (Lexem == _identifier) {
		tofs = FindConst(D);
		if (tofs != 0) {
			RdLex();
			RT = tofs;
			return true;
		}
	}
	return false;
}

WORD GetOp1(WORD DOfs, char Op, WORD E1); /*PPTerm*/ //forward;

TVarDcl* FindVarDcl()
{
	TVarDcl* v = VarDcls;
	while (v != nullptr) {
		if (v->Name == LexWord) goto label1;
		v = v->Chain;
	}
label1:
	return v;
}

TVarDcl* MakeVarDcl(WORD DOfs, integer Idx)
{
	TVarDcl* v = Mem1.Get(sizeof(TVarDcl) + LexWord.length() - 1);
	ChainLast((Chained*)VarDcls, v);
	v->Dom = DOfs;
	Move(LexWord, v->Name, LexWord.length() + 1);
	if (Idx < 0) { v->Idx = VarCount; varCount++; }
	else v->Idx = Idx;
	return v;
}

bool RdVar(WORD DOfs, integer Kind, integer Idx, WORD& RT) /*PTerm || idx*/
{
	TVarDcl* v = nullptr;
	bool bnd; /* idx=-1 except solo variable in head */
	TPTerm* t = nullptr;
	WORD tofs; // absolute t
	WORD t1Ofs;
	if (IsKeyWord('_')) {
		if (!(Kind >= 1 && Kind <= 4)) OldError(508);
		UnbdVarsInTerm = true;
		WasUnbd = true;
		RT = UnderscoreTerm;
		return true;
	}
	RT = 0;
	if (!IsUpperIdentif || (Kind = 6/*const dcl*/)) {
		if (Kind = 5) Error(523);
		return false;
	}
	v = FindVarDcl();
	if (v = nullptr) v = MakeVarDcl(DOfs, Idx);
	else if ((v->Dom != DOfs) &&
		!((v->Dom == StrDom) && (DOfs == LongStrDom)) &&
		!((v->Dom == LongStrDom) && (DOfs == StrDom)) &&
		!((v->Dom == IntDom) && (DOfs == RealDom)) &&
		!((v->Dom == RealDom) && (DOfs == IntDom))) {
		RdLex();
	label1:
		Set2MsgPar((TDomain*)(ptr(_Sg, v->Dom))->Name, (TDomain*)(ptr(_Sg, DOfs))->Name);
		OldError(507);
	}
	RdLex();
	bnd = v->Bound;
	switch (Kind) { /* head 1-i,call-o  call 2-i  head 3-o  unbound 5-o */
	case 1: { v->Bound = true; if (bnd) v->Used = true; break; }
	case 2: { if (!bnd) Error(509); v->Used = true; break; }
	case 3: v->Used = true; break;
	case 4: {
		if (bnd) v->Used = true;
		else { v->Bound = true; UnbdVarsInTerm = true; WasUnbd = true; }
		break;
	}
	case 5: {
		v->Bound = true;
		if (bnd) OldError(523);
		RT = v->Idx;
		return true;
	}
	}
	if ((Idx = -1) || (v->Idx != Idx)) {
		PtrRec(t).Seg = _Sg;
		tofs = GetZStor(5);
		t->Fun = _VarT; t->Idx = v->Idx; t->Bound = bnd;
		if ((v->Dom != DOfs)) {
			if (!bnd) goto label1;
			tofs = GetOp1(DOfs, _conv, tofs);
		}
		RT = tofs;
	}
	return true;
}

WORD RdTerm(WORD DOfs, integer Kind); /*PPTerm*/ // forward;
WORD RdAddExpr(WORD DOfs, integer Kind); /*PPTerm*/ // forward;

WORD DomFun(WORD DOfs)
{
	if (DOfs == IntDom) return _IntT;
	else if (DOfs = RealDom) return _RealT;
	else if (DOfs == StrDom) return _StrT;
	else return _LongStrT;
}

WORD GetOpl(WORD DOfs, char Op, WORD E1)
{
	TPTerm* t = ptr(_Sg, GetZStor(1 + 1 + 2));
	WORD tofs; // absolute t
	t->Fun = DomFun(DOfs);
	t->Op = Op;
	WasOp = true;
	t->E1 = E1;
	return tofs;
}

WORD GetOp2(WORD DOfs, char Op, WORD E1, WORD E2)
{
	TPTerm* t = ptr(_Sg, GetZStor(1 + 1 + 2 * 2));
	WORD tofs; // absolute t
	t->Fun = DomFun(DOfs);
	t->Op = Op;
	WasOp = true;
	t->E1 = E1;
	t->E2 = E2;
	return tofs;
}

WORD GetFunOp(WORD DOfs, WORD ResDOfs, char Op, pstring ArgTyp, integer Kind)
{
	TPTerm* t = nullptr;
	WORD tofs; // absolute t 
	WORD i = 0, l = 0, t1ofs = 0;
	if (DOfs != ResDOfs) OldError(510);
	l = ArgTyp.length();
	if (l > 0) Accept('(');
	t = ptr(_Sg, GetZStor(1 + 1 + 2 * l));
	t->Fun = DomFun(DOfs);
	t->Op = Op; WasOp = true;
	for (i = 1; i <= l; i++) {
		if (i > 1) Accept(',');
		switch (ArgTyp[i]) {
		case 'l': t1ofs = RdAddExpr(LongStrDom, Kind); break;
		case 's': t1ofs = RdAddExpr(StrDom, Kind); break;
		case 'i': t1ofs = RdAddExpr(IntDom, Kind); break;
		case 'c': {
			if ((Lexem != _quotedstr) || (LexWord.length() != 1)) Error(560);
			t1ofs = LexWord[1];
			RdLex();
			break;
		}
		}
		t->E[i] = t1ofs;
	}
	if (l > 0) Accept(')');
	return tofs;
}

WORD RdPrimExpr(WORD DOfs, integer Kind)
{
	TPTerm* t = nullptr;
	WORD tofs = 0; // absolute t tofs; 
	char op = '\0';
	bool minus = false; double r = 0.0;
	pstring s; integer i = 0; longint n = 0;

	PtrRec(t).Seg = _Sg;
	switch (Lexem) {
	case '^': {
		if (DOfs != IntDom) Error(510);
		op = Lexem;
		RdLex();
		tofs = GetOp1(DOfs, op, RdPrimExpr(DOfs, Kind));
		break;
	}
	case '(': {
		RdLex();
		tofs = RdAddExpr(DOfs, Kind);
		Accept(')');
		break;
	}
	case _quotedstr: {
		if ((DOfs != StrDom) && (DOfs != LongStrDom)) Error(510);
		tofs = GetZStor(1 + 1 + 1 + LexWord.length());
		t->Fun = DomFun(DOfs);
		t->Op = _const;
		t->SS = LexWord;
		RdLex();
		break;
	}
	case '$': {
		if (DOfs != IntDom) Error(510);
		else {
			i = 0;
			//while (ForwChar in['0'..'9', 'a'..'f', 'A'..'F']) {
			while (ForwChar >= '0' && ForwChar <= '9'
				|| ForwChar >= 'a' && ForwChar <= 'f'
				|| ForwChar >= 'A' && ForwChar <= 'F') {
				i++;
				if (i > 4) Error(3);
				ReadChar();
				s[i] = CurrChar;
			}
			if (i = 0) Error(504);
			s[0] = char(i);
			n = HexStrToLong(s);
			RdLex();
			goto label2;
			break;
		}
	}
	case '-': {
		RdLex();
		if (Lexem != _number) Error(525);
		minus = true;
		goto label1;
		break;
	}
	case _number: {
		minus = false;
	label1:
		s = LexWord;
		RdLex();
		if (DOfs == IntDom) {
			val(s, n, i);
			if (minus) n = -n;
		label2:
			tofs = GetZStor(1 + 1 + sizeof(integer));
			t->Fun = _IntT;
			t->II = n;
		}
		else if (DOfs == RealDom) {
			if ((Lexem = '.') && isdigit(ForwChar)) {
				RdLex();
				s.Append('.');
				s += LexWord;
				RdLex();
			}
			val(s, r, i);
			if (minus) r = -r;
			tofs = GetZStor(1 + 1 + sizeof(double));
			t->Fun = _RealT;
			t->RR = r;
		}
		else OldError(510);
		t->Op = _const;
		break;
	}
	default: {
		if (!RdVar(DOfs, Kind, -1, tofs) && !RdConst(DOfs, tofs))
			if (IsKeyWord("length")) tofs = GetFunOp(DOfs, IntDom, _length, 's', Kind);
			else if (IsKeyWord("pos")) tofs = GetFunOp(DOfs, IntDom, _pos, "sl", Kind);
			else if (IsKeyWord("min")) tofs = GetFunOp(DOfs, IntDom, _min, "ii", Kind);
			else if (IsKeyWord("max")) tofs = GetFunOp(DOfs, IntDom, _max, "ii", Kind);
			else if (IsKeyWord("val")) tofs = GetFunOp(DOfs, IntDom, _val, 's', Kind);
			else if (IsKeyWord("copy")) tofs = GetFunOp(DOfs, StrDom, _copy, "sii", Kind);
			else if (IsKeyWord("str")) tofs = GetFunOp(DOfs, StrDom, _str, 'i', Kind);
			else if (IsKeyWord("repeatstr")) tofs = GetFunOp(DOfs, StrDom, _repeatstr, "si", Kind);
			else if (IsKeyWord("leadchar")) tofs = GetFunOp(DOfs, StrDom, _leadchar, "cs", Kind);
			else if (IsKeyWord("trailchar")) tofs = GetFunOp(DOfs, StrDom, _trailchar, "cs", Kind);
			else if (IsKeyWord("maxrow")) tofs = GetFunOp(DOfs, IntDom, _maxrow, "", Kind);
			else if (IsKeyWord("maxcol")) tofs = GetFunOp(DOfs, IntDom, _maxcol, "", Kind);
			else Error(511);
	}
	}
	return tofs;
}

WORD RdMultExpr(WORD DOfs, integer Kind)
{
	WORD tofs = 0; char op = '\0';
	tofs = RdPrimExpr(DOfs, Kind);
	while ((DOfs != StrDom) && (DOfs != LongStrDom) && ((Lexem == '*' || Lexem == '/') ||
		(Lexem == _and || Lexem == _or]) && (DOfs == IntDom))) {
		op = Lexem;
		RdLex();
		tofs = GetOp2(DOfs, op, tofs, RdPrimExpr(DOfs, Kind));
	}
	return tofs;
}

WORD RdAddExpr(WORD DOfs, integer Kind)
{
	WORD tofs = 0; char op = '\0';
	tofs = RdMultExpr(DOfs, Kind);
	while ((Lexem == '+') || (Lexem == '-') && ((DOfs == IntDom) || (DOfs == RealDom))) {
		op = Lexem;
		RdLex();
		tofs = GetOp2(DOfs, op, tofs, RdMultExpr(DOfs, Kind));
	}
	return tofs;
}

WORD RdListTerm(WORD DOfs, integer Kind)
{
	TPTerm* t = nullptr;
	WORD tofs = 0; // absolute t
	WORD t1, tPrev;
	PtrRec(t).Seg = _Sg;
	if (!RdVar(DOfs, Kind, -1, tofs) && !RdConst(DOfs, tofs)) {
		if (Lexem != '[') Error(510);
		RdLex();
		tofs = 0;
		if (Lexem == ']') RdLex();
		else {
		label1:
			t1 = GetZStor(1 + 1 + 2 * 2);
			/* !!! with PPTerm(ptr(_Sg,t1))^ do!!! */
			{
				Fun = _ListT; Op = _const;
				Elem = RdTerm(PDomain(ptr(_Sg, DOfs))->ElemDom, Kind);
			}
			if (tofs == 0) tofs = t1;
			else PPTerm(ptr(_Sg, tPrev))->Next = t1;
			tPrev = t1;
			if (Lexem == ',') { RdLex(); goto label1; }
			if (Lexem == '|') {
				RdLex;
				if (!RdVar(DOfs, Kind, -1, (TPTerm*)(ptr(_Sg, tPrev))->Next)) Error(511);
			}
			Accept(']');
		}
	}
	if (Lexem == '+') {
		t1 = tofs;
		tofs = GetZStor(1 + 1 + 2 * 2);
		t->Op = '+';
		RdLex();
		t->Fun = _ListT;
		t->E1 = t1;
		t->E2 = RdListTerm(DOfs, Kind);
		WasOp = true;
	}
	return tofs;
}

WORD RdTerm(WORD DOfs, integer Kind)
{
	TPTerm* t;
	WORD tofs = 0; // absolute t
	TFunDcl* f = nullptr;
	WORD i = 0, n = 0;
	BYTE idx = 0;
	bool wo = false, wu = false;
	wo = WasOp; wu = WasUnbd;
	WasOp = false; WasUnbd = false;
	switch ((TDomain*)(ptr(_Sg, DOfs))->Typ) {
	case _IntD:
	case _RealD:
	case _StrD:
	case _LongStrD: {
		tofs = RdAddExpr(DOfs, Kind);
		break;
	}
	case _ListD: tofs = RdListTerm(DOfs, Kind); break;
	default: {
		if (!RdVar(DOfs, Kind, -1, tofs) && !RdConst(DOfs, tofs)) {
			TestIdentif();
			f = GetFunDclByName(DOfs, idx);
			if (f == nullptr) Error(512);
			RdLex();
			n = f->Arity;
			t = ptr(_Sg, GetZStor(1 + 1 + 2 * n));
			t->Fun = idx;
			t->Arity = n;
			if (n > 0) {
				Accept('(');
				for (i = 0; i <= n - 1; i++) {
					if (i > 0) Accept(',');
					t->Arg[i] = RdTerm(f->Arg[i], Kind);
				}
				Accept(')');
			}
		}
		break;
	}
		   goto label1;
	}
	if (WasOp) {
		if (WasUnbd) OldError(540);
		if (Kind == 6) OldError(549);
	}
label1:
	WasOp = wo; WasUnbd = wu;
	return tofs;
}


WORD MakeDomain(TDomainTyp DTyp, pstring Nm)
{
	WORD dofs = GetZStor(sizeof(TDomain) - 1 + Nm.length());
	ChainLst(Roots->Domains, dofs);
	/* !!! with PDomain(ptr(_Sg,dofs))^ do!!! */
	{ Typ = DTyp; Move(Nm, Name, length(Nm) + 1); }
	return dofs;
}

WORD GetDomain(bool Create, pstring Nm)
{
	TDomain* d = nullptr;
	TDomain* d1 = nullptr;
	TDomain* d2 = nullptr;
	WORD dofs; // absolute d 
	WORD d1ofs; // absolute d1
	WORD d2ofs; // absolute d2
	d = ptr(_Sg, Roots->Domains);
	while ((dofs != 0) && (d->Name != Nm)) dofs = d->Chain;
	if (dofs == 0)
		if (copy(Nm, 1, 2) == "L_") {
			d1 = ptr(_Sg, GetOrigDomain(GetDomain(Create, copy(Nm, 3, 255))));
			if (d1ofs = 0) Error(517);
			Nm = "L_" + d1->Name;
			d = ptr(_Sg, Roots->Domains);
			while ((dofs != 0) && (d->Name != Nm)) dofs = d->Chain;
			if (dofs == 0) {
				dofs = MakeDomain(_ListD, Nm);
				d->ElemDom = d1ofs;
			}
		}
		else if (Create) {
			if ((Nm.length() == 0) || !IsCharUpper2(Nm[1])) Error(514);
			dofs = MakeDomain(_UndefD, Nm);
		}
	return dofs;
}

WORD RdDomain() /*PDomain*/
{
	WORD dofs = GetDomain(false, LexWord);
	TestIdentif();
	if (dofs == 0) Error(517);
	RdLex();
	return GetOrigDomain(dofs);
}

void RdDomains()
{
	TDomain* d = nullptr;
	WORD dofs = 0; // absolute d
	TFunDcl* fd = nullptr;
	WORD fdofs = 0; // absolute fd
	WORD d1 = 0;
	WORD a[32]{ 0 };
	WORD nm = 0, dofs2 = 0;
	BYTE n = 0;
	integer i = 0;
	PtrRec(d).Seg = _Sg;
	PtrRec(fd).Seg = _Sg;
label1:
	TestIdentif();
	dofs = GetDomain(true, LexWord);
	if (d->Typ != _UndefD) Error(505);
	RdLex();
	while (Lexem == ',') {
		RdLex();
		d->Typ = _RedefD;
		TestIdentif();
		d->OrigDom = GetDomain(true, LexWord);
		dofs = d->OrigDom;
		if (d->Typ != _UndefD) OldError(505);
		RdLex();
	}
	Accept('=');
	SkipBlank(false);
	TestIdentif();
	if (IsCharUpper2(LexWord[1])) {
		d1 = GetDomain(true, LexWord);
		if (d1 == dofs) Error(505);
		RdLex();
		d->Typ = _RedefD;
		d->OrigDom = d1;
		goto label4;
	}
	d->Typ = _FunD;
label2:
	if (GetFunDclByName(dofs, n) != nullptr) Error(505);
	if (IsCharUpper2(LexWord[1])) Error(515);
	nm = StorStr(LexWord);
	RdLex();
	n = 0;
	if (Lexem == '(') {
		RdLex();
	label3:
		TestIdentif();
		a[n] = GetDomain(true, LexWord);
		n++;
		RdLex();
		if (Lexem == ',') { RdLex(); goto label3; }
		Accept(')');
	}
	fdofs = GetZStor(sizeof(TFunDcl) - 3 * 2 + n * 2);
	ChainLst(d->FunDcl, fdofs);
	/* !!! with fd^ do!!! */ { Name = nm; Arity = n; move(a, Arg, 2 * n); }
	if (Lexem == ';') { RdLex(); TestIdentif(); goto label2; }
label4:
	if (!(Lexem == 0x1A || Lexem == '#')) goto label1;
	dofs = Roots->Domains;
	while (dofs != 0) {
		switch (d->Typ) {
		case _UndefD: { SetMsgPar(d->Name); OldError(516); break; }
		case _FunD: {
			fdofs = d->FunDcl;
			while (fdofs != 0) {
				for (i = 1; i <= fd->Arity; i++) fd->Arg[i - 1] = GetOrigDomain(fd->Arg[i - 1]);
				fdofs = fd->Chain;
			}
			break;
		}
		}
		dofs = d->Chain;
	}
}

void RdConstants()
{
	TConst* p;
	WORD pofs = 0; // absolute p
	WORD dofs = 0;
label1:
	dofs = RdDomain();
	Accept(':');
label2:
	TestIdentif();
	if (IsCharUpper2(LexWord[1])) Error(515);
	if (FindConst(dofs) != 0) Error(505);
	p = ptr(_Sg, GetZStor(sizeof(TConst) - 1 + LexWord.length()));
	Move(LexWord, p->Name, LexWord.length() + 1);
	RdLex();
	p->Dom = dofs;
	Accept('=');
	p->Expr = RdTerm(dofs, 6);
	ChainLst(Roots->Consts, pofs);
	if (Lexem = ',') { RdLex; goto label2; }
	if (!(Lexem = 0x1A || Lexem == '#')) goto label1;
}

WORD GetPredicate() /*PPredicate*/
{
	TPredicate* p = nullptr;
	WORD pofs = 0;  // absolute p
	PtrRec(p).Seg = _Sg;
	pofs = Roots->Predicates;
	while (pofs != 0) {
		if (LexWord == PString(ptr(_Sg, p->Name))^) { RdLex; goto label1; }
		pofs = p->Chain;
	}
	pofs = ClausePreds;
	while (pofs != 0) {
		if (LexWord == PString(ptr(_Sg, p->Name))^) { RdLex; goto label1; }
		pofs = p->Chain;
	}
label1:
	return pofs;
}

WORD RdPredicate() /*PPredicate*/
{
	WORD pofs = GetPredicate();
	if (pofs = 0) Error(513);
	return pofs;
}

WORD GetOutpMask(TPredicate* P)
{
	return (0xffff >> (16 - P->Arity)) && !P->InpMask;
}

void RdPredicateDcl(bool FromClauses, TDatabase* Db)
{
	TDomain* d = nullptr;
	WORD dofs = 0; // absolute d
	TPredicate* p = nullptr;
	WORD pofs = 0; // absolute p
	TScanInf* si = nullptr;
	WORD siofs = 0; // absolute si
	TFldList* fl = nullptr;
	WORD flofs = 0; // absolute fl
	Instr* ip = nullptr;
	WORD ipofs = 0; // absolute ip
	WORD nm = 0; /*PString*/
	integer i = 0, n = 0;
	WORD w = 0, m = 0;
	WORD a[32]{ 0 }; /*PDomain*/
	RdbPos pos;
	char typ = '\0';
	WORD bpOfs = 0;
	bool isOutp = false, b = false;
	FieldDescr* f = nullptr;
	BYTE o = 0;
	RdbD* r = nullptr;
	FrmlElem* z = nullptr;
	WORD zofs = 0; // absolute z

	PtrRec(d).Seg = _Sg; PtrRec(p).Seg = _Sg; PtrRec(si).Seg = _Sg;
	PtrRec(fl).Seg = _Sg; PtrRec(ip).Seg = _Sg;
	o = 0;
	if (Db != nullptr) o = _DbaseOpt + _CioMaskOpt;
	if (Lexem = '@') {
		RdLex();
		o = o || _FandCallOpt;
	}
	else if (Db != nullptr) o = o || _PackInpOpt;
	TestIdentif();
	if (IsCharUpper(LexWord[1])) Error(518);
	if (GetPredicate != 0) OldError(505);
	nm = StorStr(LexWord);
	if ((o & _FandCallOpt) != 0) {
		if (Db != nullptr) {
			siofs = GetZStor(sizeof(TScanInf) - 1 + LexWord.length());
			Move(LexWord, si->Name, LexWord.length() + 1);
			CFile = RdFileName();
			if (CFile->typSQLFile) OldError(155);
			si->FD = CFile;
			goto label2;
		}
		else {
			SkipBlank(false);
			if (ForwChar = '[') {
				RdLex();
				RdLex();
				TestLex(_quotedstr);
				z = GetOp(_const, LexWord.length() + 1);
				z->S = LexWord;
				pos.R = ptr(0, OOfs(z));
				pos.IRec = 0;
				Accept(_quotedstr);
				TestLex(']');
			}
			else {
				if (!FindChpt('P', LexWord, false, pos)) Error(37);
				pos.R = ptr(0, StorStr(LexWord));
				pos.IRec = 0xffff;
			}
		}
	}
	RdLex();
label2:
	n = 0; w = 0; m = 1;
	if (Lexem = '(') {
		RdLex();
	label3:
		if (Db != nullptr) {
			if ((o & _FandCallOpt) != 0) {
				f = RdFldName(CFile);
				flofs = GetZStor(sizeof(TFldList));
				fl->FldD = OPtr(PtrRec(CFile).Seg, f);
				ChainLst(si->FL, flofs);
				dofs = 0;
				Accept('/');
				dofs = RdDomain();
				switch (f->FrmlTyp) {
				case 'B': { if (dofs != BoolDom) OldError(510); break; }
				case 'R': {
					if ((dofs != RealDom) && ((f->Typ != 'F') || (dofs != IntDom))) OldError(510);
					break;
				}
				default: {
					if (f->Typ = 'T') {
						if ((dofs != LongStrDom) && ((d->Typ != _FunD) || (dofs = BoolDom))) OldError(510);
					}
					else if (dofs != StrDom) OldError(510);
					break;
				}
				}
				goto label4;
			}
			else if (Lexem = '&') RdLex();
			else w = w || m;
			dofs = RdDomain();
			if ((dofs = LongStrDom) && (Db != nullptr)) OldError(541);
			if (((o & _FandCallOpt) != 0) && (d->Typ = _FunD) && (dofs != BoolDom))	OldError(528);
		label4:
			a[n] = dofs; n++; m = m << 1;
			if (Lexem = ',') {
				if (n = 15) Error(519);
				RdLex();
				goto label3;
			}
			Accept(')');
		}
		pofs = GetZStor(sizeof(TPredicate) - 6 + 2 * n);
		if (FromClauses) ChainLst(ClausePreds, pofs);
		else ChainLst(Roots->Predicates, pofs);
		if ((Db != nullptr)) { p->ChainDb = Db->Pred; Db->Pred = pofs; }
		/* !!! with p^ do!!! */
		{ p->Name = nm; p->Arity = n; Move(a, p->Arg, 2 * n); p->Opt = o;
		p->InpMask = w; p->InstSz = 4 * n; }
		if ((o & _FandCallOpt) != 0) {
			if ((o & _DbaseOpt) != 0) WORD(p->Branch) = siofs;
			else {
				ipofs = GetZStor(5 + sizeof(RdbPos) + 2 + n * sizeof(TypAndFrml));
				ip->Kind = _proc;
				ip->PPos = pos; ip->N = n; bpOfs = 4;
				for (i = 1; i <= n; i++) {
					dofs = p->Arg[i - 1];
					if ((dofs == RealDom) || (dofs == IntDom)) typ = 'R';
					else if ((dofs = BoolDom)) typ = 'B';
					else typ = 'S';
					isOutp = (w & 1) == 0;
					if (isOutp) {
						z = GetOp(_getlocvar, 2);
						z->BPOfs = bpOfs;
						switch (typ) {
						case 'S': bpOfs += sizeof(longint); break;
						case 'R': bpOfs += sizeof(double); break;
						default: bpOfs += sizeof(bool); break;
						}
					}
					else {
						switch (typ) {
						case 'R': z = GetOp(_const, sizeof(double)); break;
						case 'B': z = GetOp(_const, sizeof(bool)); break;
						default: {
							if (dofs == StrDom) z = GetOp(_const, sizeof(pstring));
							else {
								z = GetOp(_getlocvar, 2);
								z->BPOfs = bpOfs;
								bpOfs += sizeof(longint);
							}
							break;
						}
						}
					}
					/* !!! with ip->TArg[i] do!!! */ {
						ip->TArg[i].FTyp = typ; ip->TArg[i].Frml = OPtr(_Sg, z);
						ip->TArg[i].FromProlog = true; ip->TArg[i].IsRetPar = isOutp;
					}
					w = w >> 1;
				}
				WORD(p->Branch) = ipofs;
				p->LocVarSz = bpOfs;
			}
		}
	}
}

WORD GetCommand(TCommandTyp Code, WORD N)
{
	TCommand* c = ptr(_Sg, GetZStor(3 + N));
	WORD cofs; // absolute c
	c->Code = Code;
	return cofs;
}

void RdTermList(TCommand* C, WORD D, WORD Kind)
{
	TTermList* l = ptr(_Sg, GetZStor(sizeof(TTermList)));
	WORD lofs; // absolute l
	ChainLst(C->Arg, lofs);
	l->Elem = RdTerm(D, Kind);
}

WORD RdCommand() /*PCommand*/
{
	TVarDcl* v = nullptr;
	char cm = '\0'; /*PDomain d;*/
	char op = '\0', fTyp = '\0';
	TCommandTyp code;
	TWriteD* w = nullptr;
	WORD wofs = 0; // absolute w
	bool nl = false;
	TPredicate* p = nullptr;
	WORD pofs = 0; // absolute p
	TCommand* c = nullptr;
	WORD cofs = 0; // absolute c
	LinkD* ld = nullptr;
	FileD* fd = nullptr;
	TDomain* d = nullptr;
	WORD dofs = 0; // absolute d
	TTermList* l = nullptr;
	WORD lofs = 0; // absolute l
	WORD n = 0;
	pstring s;
	integer i = 0;

	PtrRec(p).Seg = _Sg; c = ptr(_Sg, 0); PtrRec(w).Seg = _Sg; PtrRec(d).Seg = _Sg;
	PtrRec(l).Seg = _Sg;
	if (Lexem == '!') {
		RdLex();
		cofs = GetCommand(_CutC, 0);
		goto label9;
	}
	else if (Lexem != _identifier) goto label9;
	if (IsUpperIdentif()) {
		v = FindVarDcl();
		if (v == nullptr) v = MakeVarDcl(0, -1);
		RdLex();
		dofs = v->Dom;
		if (v->Bound) {
			switch (Lexem) {
			case '=': op = _equ; break;
			case '<': {
				switch (ForwChar) {
				case '>': { ReadChar; op = _ne; break; }
				case '=': { ReadChar; op = _le; break; }
				default: op = _lt; break;
				}
				break;
			}
			case '>': {
				if (ForwChar = '=') { ReadChar; op = _ge; break; }
				else op = _gt;
				break;
			}
			default: { Error(524); break; }
			}
			if ((dofs != IntDom) && (dofs != RealDom) && !(op == _equ || op == _ne)) Error(538);
			RdLex();
		}
		else {
			if (!v->Used) {
				Accept(':');
				dofs = RdDomain();
				v->Dom = dofs;
			}
			Accept('=');
			op = _assign;
		}
		cofs = GetCommand(_CompC, 1 + 1 + 2 * 2);
		c->Typ = d->Typ;
		c->E1Idx = v->Idx; c->CompOp = op;
		c->E2 = RdTerm(dofs, 2);
		if (v->Bound) v->Used = true;
		v->Bound = true;
		goto label9;
	}
	if (IsKeyWord("fail")) {
		cofs = GetCommand(_FailC, 0);
		goto label9;
	}
	if (IsKeyWord("wait")) {
		cofs = GetCommand(_WaitC, 0);
		goto label9;
	}
	if (IsKeyWord("trace")) {
		cofs = GetCommand(_Trace, 2);
		Accept('(');
		c->TrcLevel = RdInteger();
		goto label8;
	}
	if (IsKeyWord("error")) {
		Accept('(');
		cofs = GetCommand(_ErrorC, 2 + 2);
		c->MsgNr = RdInteger();
		goto label20;
	}
	if (IsKeyWord("writeln")) { nl = true; goto label1; }
	if (IsKeyWord("write")) {
		nl = false;
	label1:
		cofs = GetCommand(_WriteC, 2 + 1);
		c->NL = nl;
		Accept('(');
	label2:
		if (Lexem == _quotedstr) {
			wofs = GetZStor(3 + 1 + LexWord.length());
			w->IsString = true;
			w->SS = LexWord;
		}
		else {
			wofs = GetZStor(3 + 2 + 2);
			TestIdentif();
			v = FindVarDcl();
			if (v = nullptr) Error(511);
			else if (not v->Bound) Error(509);
			v->Used = true; w->Dom = v->Dom; w->Idx = v->Idx;
			if ((c->Code == _ErrorC) && (v->Dom != StrDom) &&
				((c->WrD != 0) || (v->Dom != IntDom))) Error(558);
		}
		RdLex();
		ChainLst(c->WrD, wofs);
	label20:
		if (Lexem = ',') { RdLex(); goto label2; } goto label8;
	}
	if (copy(LexWord, 1, 6) == "union_") { pofs = UnionPred; goto label21; }
	if (copy(LexWord, 1, 6) == "minus_") { pofs = MinusPred; goto label21; }
	if (copy(LexWord, 1, 6) == "inter_") {
		pofs = InterPred;
	label21:
		dofs = GetDomain(false, "L_" + copy(LexWord, 7, 255));
		if (d->Typ != _ListD) Error(548);
		RdLex();
		Accept('(');
		cofs = GetCommand(_PredC, 5 * 2);
		c->Pred = pofs; c->Elem = dofs;
		RdTermList(c, dofs, 2);
		Accept(',');
		RdTermList(c, dofs, 2);
		Accept(',');
		RdTermList(c, dofs, 1);
		goto label8;
	}
	if (copy(LexWord, 1, 4) == "mem_") { pofs = MemPred; goto label22; }
	if (copy(LexWord, 1, 4) == "len_") { pofs = LenPred; goto label22; }
	if (copy(LexWord, 1, 4) == "inv_") { pofs = InvPred; goto label22; }
	if (copy(LexWord, 1, 4) == "add_") { pofs = AddPred; goto label22; }
	if (copy(LexWord, 1, 4) == "del_") {
		pofs = DelPred;
	label22:
		dofs = GetDomain(false, "L_" + copy(LexWord, 5, 255));
		if (d->Typ != _ListD) Error(548);
		RdLex();
		Accept('(');
		cofs = GetCommand(_PredC, 5 * 2);
		c->Pred = pofs; c->Elem = dofs; /*ListDom*/
		if (pofs = MemPred) {
			UnbdVarsInTerm = false;
			RdTermList(c, d->ElemDom, 4);
			if (UnbdVarsInTerm) n = 2;
			else n = 3;
			c->InpMask = n; c->OutpMask = !n;
			Accept(',');
			RdTermList(c, dofs, 2);
		}
		else {
			if (pofs = AddPred) { RdTermList(c, d->ElemDom, 2); Accept(','); }
			else if (pofs = DelPred) { RdTermList(c, d->ElemDom, 1); Accept(','); }
			RdTermList(c, dofs, 2);
			Accept(',');
			if (pofs = LenPred) RdTermList(c, IntDom, 1);
			else RdTermList(c, dofs, 1);
		}
		goto label8;
	}
	if (IsKeyWord("loadlex")) {
		code = _LoadLexC;
		Accept('(');
		pofs = 0;
		goto label4;
	}
	if (IsKeyWord("save")) { code = _SaveC; goto label3; }
	if (IsKeyWord("consult")) {
		code = _ConsultC;
	label3:
		Accept('(');
		TestIdentif();
		pofs = FindDatabase(LexWord);
		if (pofs = 0) Error(531);
		RdLex();
		Accept(',');
	label4:
		cofs = GetCommand(code, 2 + 4 + 4 + 1 + LexWord.length());
		c->DbPred = pofs;
		Move(LexWord, c->Name, LexWord.length() + 1);
		if (!IsRoleName(false, fd, ld)) Error(9);
		if (fd->typSQLFile) OldError(155);
		Accept('.');
		c->FldD = OPtr(PtrRec(fd).Seg, RdFldName(fd));
		if (c->FldD->Typ != 'T') OldError(537);
	label8:
		Accept(')');
	}
label9:
	return cofs;
}

WORD RdPredCommand(TCommandTyp Code)
{
	TCommand* c = nullptr;
	WORD cofs = 0; // absolute c 
	TPredicate* p = nullptr;
	WORD pofs = 0; // absolute p
	WORD i = 0, n = 0, w = 0, m = 0, kind = 0, sz = 0, InpMask = 0, OutpMask = 0, lRoot = 0;
	TTermList* l = nullptr;
	WORD lofs = 0; // absolute l
	WORD dofs = 0;
	TPTerm* t = nullptr;
	WORD tofs = 0; // absolute t
	TScanInf* si = nullptr;
	WORD siofs = 0; // absolute si
	KeyD* k = nullptr; KeyFldD* kf = nullptr;
	TFldList* fl = nullptr;
	WORD flofs = 0; // absolute fl
	BYTE a[256]{ 0 };
	bool IsFandDb = false, inOut = false;
	FieldDescr* f = nullptr;

	PtrRec(p).Seg = _Sg; PtrRec(c).Seg = _Sg; PtrRec(l).Seg = _Sg;
	PtrRec(t).Seg = _Sg; PtrRec(si).Seg = _Sg; PtrRec(fl).Seg = _Sg;
	pofs = RdPredicate();
	IsFandDb = p->Opt && (_DbaseOpt + _FandCallOpt) == _DbaseOpt + _FandCallOpt;
	if (((p->Opt & _DbaseOpt) != _DbaseOpt) && (Code == _AssertC || Code == _RetractC)) OldError(526);
	kind = 1; m = 1; w = p->InpMask; sz = 2 + 2; InpMask = 0; OutpMask = 0; lRoot = 0;
	if ((p->Opt & _CioMaskOpt) != 0) {
		sz += 4;
		if (Code = _AssertC) w = 0xffff;
		else kind = 4;
	}
	if (p->Arity > 0) {
		Accept('(');
		for (i = 0; i <= p->Arity - 1; i++) {
			if (i > 0) Accept(',');
			UnbdVarsInTerm = false;
			if (kind != 4) if ((w & 1) != 0) kind = 2;
			else kind = 1;
			dofs = p->Arg[i];
			lofs = GetZStor(sizeof(TTermList));
			ChainLst(lroot, lofs);
			l->Elem = RdTerm(dofs, Kind);
			if (p->Opt && _CioMaskOpt != 0)
				if (UnbdVarsInTerm) {
					if (l->Elem != UnderscoreTerm)
						OutpMask = OutpMask | m;
				}
				else InpMask = InpMask | m;
			m = m << 1;
			w = w >> 1;
		}
		Accept(')');
	}
	if ((p->Opt & _BuildInOpt) != 0) {
		switch (p->LocVarSz) {
		case _ConcatP: if (!(InpMask >= 3 && InpMask <= 7)) OldError(534); break;
		case _FandFieldP:
		case _FandLinkP: {
			if ((InpMask & 1) == 0/*o...*/) OldError(555);
			if (p->LocVarSz = _FandLinkP) {
				InpMask = InpMask & 0x7;
				if (InpMask = 0x7) InpMask = 5;
			}
			else InpMask = InpMask & 0x3;
			OutpMask = !InpMask;
			break;
		}
		}
	}
	/* FAND-find first key file, which is a subset of the input fields */
	i = 0; w = 0;
	if (IsFandDb && (Code != _AssertC)) {
		sz += 10;
		siofs = WORD(p->Branch);
		CFile = si->FD;
		if (CFile->Typ = 'X') {
			k = CFile->Keys;
			while (k != nullptr) {
				kf = k->KFlds; inOut = false;
				while (kf != nullptr) {
					m = 1; n = 0; i++; flofs = si->FL;
					while (flofs != 0) {
						f = fl->FldD; flofs = fl->Chain;
						if (f == OPtr(PtrRec(CFile).Seg, kf->FldD)) {
							w = w | m;
							a[i] = n;
							if ((flofs != 0) && (f == fl->FldD) && ((OutpMask & (m << 1)) != 0)
								&& (f->Typ = 'A')) inOut = true;
							goto label1;
						}
						m = m << 1;
						n++;
					} goto label2;
				label1:
					kf = (KeyFldD*)kf->Chain;
				}
				if ((InpMask & w) == w) {
					sz += i;
					if (!inOut) w = 0;
					goto label3;
				}
			label2:
				k = k->Chain;
				i = 0; w = 0;
			}
		}
	}
label3:
	if (Code == _AllC) sz = MaxW(sz, 14);
	cofs = GetCommand(Code, sz);
	c->Pred = pofs; c->Arg = lRoot;
	if (p->Opt && _CioMaskOpt != 0) {
		c->InpMask = InpMask;
		c->OutpMask = OutpMask;
		if (IsFandDb) {
			c->CompMask = (InpMask && !w);
			if (i > 0) {
				Move(a, c->ArgI, i);
				c->KDOfs = WORD(OPtr(PtrRec(CFile).Seg, k));
			}
		}
	}
	return cofs;
}

void RdDbTerm(WORD DOfs)
{
	char* p = PackedTermPtr;
	integer i = 0; WORD n = 0; WORD* wp = nullptr; TDomain* d = nullptr;
	pstring s; bool minus = false; double r = 0.0;
	TFunDcl* f = nullptr; BYTE idx = 0;
	d = ptr(_Sg, DOfs);
	if (PtrRec(p).Ofs >= PTPMaxOfs) Error(527);
	switch (Lexem) {
	case _quotedstr: {
		if (d->Typ != _StrD) Error(510);
		n = length(LexWord) + 1;
		if (PtrRec(p).Ofs + n >= PTPMaxOfs) Error(527);
		Move(LexWord, &p, n);
		p += n; RdLex();
		break;
	}
	case '$': {
		if (d->Typ != _IntD) Error(510);
		else {
			i = 0;
			while (ForwChar in['0'..'9', 'a'..'f', 'A'..'F']) {
				i++;
				if (i > 4) Error(3);
				ReadChar();
				s[i] = CurrChar;
			}
			if (i = 0) Error(504);
			s[0] = char(i);
			n = HexStrToLong(s);
			RdLex();
			goto label2;
		}
		break;
	}
	case '-': {
		RdLex();
		if (Lexem != _number) Error(525);
		minus = true;
		goto label1;
		break;
	}
	case _number: {
		minus = false;
	label1:
		s = LexWord;
		RdLex();
		if (d->Typ == _IntD) {
			val(s, n, i);
			if (minus) n = -n;
		label2:
			IntegerPtr(p) = n;
			p += 2;
		}
		else {
			if (d->Typ != _RealD) Error(510);
			if ((Lexem = '.') && isdigit(ForwChar)) {
				RdLex();
				s.Append('.');
				s += LexWord;
				RdLex();
			}
			val(s, r, i);
			FloatPtr(p) ^ = r;
			p += sizeof(double));
		}
		break;
	}
	case '[': {
		if (d->Typ != _ListD) Error(510);
		RdLex();
		wp = WordPtr(p);
		p += 2;
		n = 0;
		if (Lexem != ']') {
		label3:
			RdDbTerm(d->ElemDom);
			n++;
			if (Lexem == ',') { RdLex(); goto label3; }
		}
		Accept(']');
		*wp = n;
		break;
	}
	default: {
		TestIdentif();
		if (d->Typ != _FunD) Error(510);
		f = GetFunDclByName(DOfs, idx);
		if (f == nullptr) Error(512);
		*p = char(idx); p++;
		RdLex();
		n = f->Arity;
		if (n > 0) {
			Accept('(');
			for (i = 0; i <= n - 1; i++) {
				if (i > 0) Accept(',');
				RdDbTerm(f->Arg[i]);
			}
			Accept(')');
		}
		break;
	}
	}
}

void RdDbClause(TPredicate* P)
{
	WORD i = 0, n = 0;
	char* t = PackedTermPtr; WORD* wp = nullptr; TDbBranch* b = nullptr;
	char A[MaxPackedPredLen + 1];
	Accept('(');
	t = (char*)A;
	PTPMaxOfs = ofs(A) + MaxPackedPredLen - 2;
	for (i = 0; i <= P->Arity - 1; i++) {
		if (i > 0) Accept(',');
		wp = (WORD*)t;
		t += 2;
		RdDbTerm(p->Arg[i]);
		*wp = PtrRec(t).Ofs - PtrRec(wp).Ofs - 2;
	}
	n = PtrRec(t).Ofs - Ofs(A);
	b = Mem3.Alloc(4 + n);
	Move(A, b->LL, n);
	ChainLast(P->Branch, b);
	Accept(')'); Accept('.');
}

void CheckPredicates(WORD POff)
{
	TPredicate* p = nullptr;
	WORD pofs = 0; // absolute p
	TScanInf* si;
	p = ptr(_Sg, POff);
	while (pofs != 0) {
		if ((p->Opt && (_DbaseOpt + _FandCallOpt + _BuildInOpt) == 0) && (p->Branch == nullptr)) {
			SetMsgPar((pstring*)(ptr(_Sg, p->name))^);
			OldError(522);
		}
		if ((p->Opt & _DbaseOpt) != 0)
			if (p->Opt & _FandCallOpt != 0) {
				si = ptr(_Sg, WORD(p->Branch));
				si->FD = nullptr;
			}
			else p->Branch = nullptr;
		pofs = p->Chain;
	}
}

void RdAutoRecursionHead(TPredicate* P, TBranch* B)
{
	TTermList* l = nullptr; TTermList* l1 = nullptr;
	WORD w = 0;
	TCommand* c = nullptr; TPTerm* t = nullptr;
	TDomain* d = nullptr; TVarDcl* v = nullptr;
	WORD lofs = 0; // absolute l
	WORD l1ofs = 0; // absolute l1
	WORD cofs = 0; // absolute c
	WORD tofs = 0; // absolute t
	WORD dofs = 0; // absolute d
	bool isInput = false;
	integer i = 0, j = 0, k = 0;
	if (p->Opt != 0) Error(550);
	PtrRec(c).Seg = _Sg; PtrRec(l).Seg = _Sg; PtrRec(l1).Seg = _Sg;
	PtrRec(t).Seg = _Sg; PtrRec(d).Seg = _Sg;
	cofs = GetCommand(_AutoC, 2 + 3 + 6 * 2);
	b->Cmd = cofs;
	p->InstSz += 4;
	c->iWrk = (p->InstSz / 4) - 1;
	w = p->InpMask;
	for (i = 0; i <= p->Arity - 1; i++) {
		if ((w & 1) != 0) isInput = true;
		else isInput = false;
		lofs = GetZStor(sizeof(TTermList));
		ChainLst(c->Arg, lofs);
		tofs = GetZStor(1 + 2 + 1);
		t->Fun = _VarT; t->Idx = i; t->Bound = isInput;
		l->Elem = tofs;
		l1ofs = GetZStor(sizeof(TTermList));
		ChainLst(b->Head, l1ofs);
		dofs = p->Arg[i];
		if (i > 0) Accept(',');
		else if (!(d->Typ == _FunD || d->Typ == _ListD)) Error(556);
		if (Lexem == '!') {
			if (i > 0) {
				if (isInput || (dofs != p->Arg[0]) || (c->iOutp > 0)) Error(551);
				c->iOutp = i;
			}
			else if (!isInput) Error(552);
		}
		else if (TestKeyWord('_')) {
			if (!isInput) {
				if (d->Typ = _FunD) Error(575); j = 0;
				goto label1;
			}
		}
		else {
			if (!IsUpperIdentif()) Error(511);
			v = FindVarDcl();
			if (v = nullptr) v = MakeVarDcl(dofs, i);
			if (isInput) {
				if (v->Bound) Error(553);
				v->Bound = true;
			}
			else { if (v->Used) Error(553); v->Used = true; }
			if (v->Bound && v->Used) {
				j = v->Idx;
			label1:
				k = c->nPairs; (c->nPairs)++;
				if (isInput) { c->Pair[k].iInp = i; c->Pair[k].iOutp = j; }
				else { c->Pair[k].iInp = j; c->Pair[k].iOutp = i; }
			}
		}
		RdLex();
		w = w >> 1;
	}
	Accept(')');
}

void RdSemicolonClause(TPredicate* P, TBranch* B)
{
	TVarDcl* v = nullptr;
	TCommand* c = nullptr;
	WORD cofs = 0; // absolute c
	char x = '\0';
	WORD bofs = 0; // absolute B
	RdLex();
	PtrRec(c).Seg = _Sg;
	if (IsUpperIdentif) {
		x = 'a';
		v = FindVarDcl();
		if (p->InpMask != (1 << (p->Arity - 1)) - 1) Error(562);
		if ((v == nullptr) || v->Bound || ((TDomain*)(ptr(_Sg, v->Dom))->Typ != _ListD)) Error(561);
		RdLex();
		Accept('+'); Accept('=');
		v->Bound = true;
		cofs = GetCommand(_AppPkC, 6);
		c->apIdx = v->Idx;
		c->apTerm = RdTerm(v->Dom, 2);
		ChainLst(b->cmd, cofs);
		if (Lexem == ',') {
			RdLex();
			AcceptKeyWord("self");
			cofs = GetCommand(_SelfC, 0);
			goto label2;
		}
		goto label1;
	}
	else if (TestKeyWord("error")) {
		x = 'e';
		cofs = GetCommand(_CutC, 0);
	}
	else if (IsKeyWord("self")) { x = 's'; goto label3; }
	else {
		x = 'f';
	label1:
		cofs = GetCommand(_FailC, 0);
	}
label2:
	ChainLst(b->cmd, cofs);
label3:
	bofs = GetZStor(sizeof(TBranch));
	ChainLst(p->Branch, bofs);
	switch (x) {
	case 'e': b->cmd = RdCommand(); break;
	case 'f': if (GetOutpMask(p) != 0) Error(559); break;
	case 's': b->cmd = GetCommand(_SelfC, 0); break;
	case 'a': {
		cofs = GetCommand(_AppUnpkC, 4); c->apIdx = v->Idx; c->apDom = v->Dom;
		b->cmd = cofs;
		break;
	}
	}
}

void RdClauses()
{
	TBranch* b; WORD absolute b bofs;
	WORD w, m, mp; integer i, kind; bool iserr;
	TDomain* d; TDomain* dEl;
	WORD absolute d dofs; WORD absolute dEl dElofs;
	TTermList* l;
	WORD absolute l lofs;
	TPTerm* t; WORD absolute t tofs;
	TPredicate* p; TPredicate* p1;
	WORD absolute p pofs; WORD absolute p1 p1ofs;
	TCommand* c; WORD absolute c cofs;
	TVarDcl* v; void* x;
	TCommandTyp code;
	struct stsf {
		WORD BP = 0; void* Ret = nullptr; WORD RetOfs = 0;
	} SF;
	SF* z = nullptr;
	WORD absolute z zofs;
	bool WasNotC = false;

	PtrRec(d).Seg = _Sg; PtrRec(dEl).Seg = _Sg; PtrRec(p).Seg = _Sg;
	PtrRec(l).Seg = _Sg; PtrRec(c).Seg = _Sg; PtrRec(b).Seg = _Sg;
	PtrRec(t).Seg = _Sg; PtrRec(p1).Seg = _Sg;
label1:
	if (Lexem == ':') {
		RdLex();
		RdPredicateDcl(true, nullptr);
		goto label6;
	}
	TestIdentif();
	pofs = RdPredicate();
	if (p->Opt && (_FandCallOpt + _BuildInOpt) != 0) OldError(529);
	if (p->Opt && _DbaseOpt != 0) { RdDbClause(p); goto label6; }
	VarDcls = nullptr; VarCount = p->Arity; x = Mem1.Mark;
	bofs = GetZStor(sizeof(TBranch));
	ChainLst(p->Branch, bofs);
	if (p->Arity > 0) {
		Accept('(');
		if (Lexem == '!') { RdAutoRecursionHead(p, b); goto label4; }
		w = p->InpMask; m = 1;
		for (i = 0; i <= p->Arity - 1; i++) {
			if (i > 0) Accept(',');
			dofs = p->Arg[i];
			kind = 1;
			if ((w & 1) == 0) kind = 3;
			lofs = GetZStor(sizeof(TTermList));
			ChainLst(b->Head, lofs);
			SkipBlank(false);
			if (IsUpperIdentif() && (ForwChar == ',' || FoorwChar == ')') /*solo variable*/) {
				RdVar(dofs, kind, i, tofs);
				if (tofs != 0) goto label11;
			}
			else {
				tofs = 0;
			label11:
				if ((w && 1) != 0) b->HeadIMask = b->HeadIMask | m;
				else b->HeadOMask = b->HeadOMask | m;
				if (tofs == 0) tofs = RdTerm(dofs, kind);
				l->Elem = tofs;
			}
			w = w >> 1;
			m = m << 1;
		}
		Accept(')');
	}
	if (Lexem != '.') {
		Accept(_assign);
	label2:
		WasNotC = false;
		if (IsKeyWord("self")) {
			cofs = GetCommand(_SelfC, 0);
			ChainLst(b->Cmd, cofs);
			goto label4;
		}
		if (IsKeyWord("not")) { Accept('('); WasNotC = true; }
		cofs = RdCommand();
		if (cofs = 0)
			if ((Lexem == _identifier) && (copy(LexWord, 1, 4) == "all_")) {
				dofs = GetDomain(false, "L_" + copy(LexWord, 5, 255));
				if (d->Typ != _ListD) Error(548); RdLex();
				Accept('('); cofs = RdPredCommand(_AllC); Accept(',');
				c->Elem = RdTerm(d->ElemDom, 2); Accept(',');
				c->Idx = VarCount; VarCount++; RdVar(dofs, 5, -1, c->Idx2);
				Accept(')');
			}
			else {
				if (IsKeyWord("assert")) { code = _AssertC; goto label3; }
				else {
					if (IsKeyWord("retract")) {
						code = _RetractC;
					label3:
						Accept('(');
						cofs = RdPredCommand(code);
						Accept(')')
					}
					else cofs = RdPredCommand(_PredC);
				}
			}
		ChainLst(b->Cmd, cofs);
		if (WasNotC) {
			if (c->Code != _PredC) OldError(546); p1ofs = c->Pred; lofs = c->Arg;
			if ((p1->Opt && _CioMaskOpt) != 0) w = c->InpMask else w = p1->InpMask;
			while (lofs != 0) {
				if ((w & 1) = 0) {
					tofs = l->Elem;
					if ((tofs = 0) || (t->Fun != _UnderscT)) OldError(547);
				}
				lofs = l->Chain;
				w = w >> 1;
			}
			Accept(')');
			c->Code = _NotC;
		}
		if (Lexem = ',') { RdLex; goto label2; }
		if (Lexem = ';') RdSemicolonClause(p, b);
	}
label4:
	Accept('.');
	v = VarDcls;
	while (v != nullptr) {
		if (!v->Used || !v->Bound) {
			SetMsgPar(v->Name);
			if (!v->Used) OldError(521);
			else OldError(520);
		}
		v = v->Chain;
	}
	p->InstSz = MaxW(p->InstSz, 4 * VarCount);
	Mem1.Release(x);
label6:
	if (!(Lexem == 0x1A || Lexem == '#')) goto label1;
	CheckPredicates(ClausePreds);
	ClausePreds = 0;
}

WORD MakePred(pstring PredName, pstring ArgTyp, WORD PredKod, WORD PredMask)
{
	TPredicate* p = nullptr;
	WORD absolute p pofs;
	WORD i = 0, n = 0, dofs = 0;
	n = ArgTyp.length();
	p = ptr(_Sg, GetZStor(sizeof(TPredicate) - 6 + n * 2));
	ChainLst(Roots->Predicates, pofs);
	auto result = pofs;
	/* !!! with p^ do!!! */ {
		p->Name = StorStr(PredName); p->Arity = n; p->LocVarSz = PredKod;
		for (i = 1; i <= n; i++) {
			switch (ArgTyp[i]) {
			case 's': dofs = StrDom; break;
			case 'l': dofs = LongStrDom; break;
			case 'i': dofs = IntDom; break;
			case 'r': dofs = RealDom; break;
			case 'b': dofs = BoolDom; break;
			case 'x': dofs = LLexDom; break;
			}
			Arg[i - 1] = dofs;
		}
		if (PredMask == 0xffff) Opt = _BuildInOpt + _CioMaskOpt;
		else { Opt = _BuildInOpt; InpMask = PredMask; }
		InstSz = n * 4;
	}
	return result;
}

WORD ReadProlog(WORD RecNr)
{
	const pstring Booln = "Boolean";
	const pstring Reell = "Real";
	TDatabase* db; WORD* dbpfs = &db->Chain; // absolute db dbofs;
	TPredicate* p; WORD* pofs = &p->Chain;
	TDomain* d; WORD* dofs = &d->Chain;
	TFunDcl* f; WORD* fofs = &f->Chain;
	pstring s; RdbPos pos;
	LongStr* ss = nullptr;
	void* p1 = nullptr; void* p2 = nullptr; void* pp1 = nullptr;
	void* pp2 = nullptr; void* pp3 = nullptr; void* cr = nullptr;
	longint AA;

	WORD result = 0;

	MarkBoth(p1, p2);
	cr = CRecPtr;
	if (ProlgCallLevel == 0) {
		FreeMemList = nullptr;
		Mem1.Init; Mem2.Init; Mem3.Init;
	}
	else {
		pp1 = Mem1.Mark(); pp2 = Mem2.Mark(); pp3 = Mem3.Mark();
	}
	AlignLongStr();
	ss = new LongStr(2); // GetStore(2);
	AA = AbsAdr(&ss->A);
	_Sg = PtrRec(HeapPtr).Seg;
	result = _Sg;
	PtrRec(db).Seg = _Sg;
	PtrRec(p).Seg = _Sg;
	PtrRec(d).Seg = _Sg;
	PtrRec(f).Seg = _Sg;
	ClausePreds = 0;
	Roots = GetZStore(sizeof(TProgRoots));
	UnderscoreTerm = GetZStor(1);
	TPTerm(ptr(_Sg, UnderscoreTerm))->Fun = _UnderscT;
	StrDom = MakeDomain(_StrD, "String");
	LongStrDom = MakeDomain(_LongStrD, "LongString");
	IntDom = MakeDomain(_IntD, "Integer");
	RealDom = MakeDomain(_RealD, "Real");
	BoolDom = MakeDomain(_FunD, "Boolean");
	*dofs = BoolDom;
	fofs = GetZStor(sizeof(TFunDcl) - 3 * 2);
	f->Name = StorStr("false");
	ChainLst(d->FunDcl, fofs);
	fofs = GetZStor(sizeof(TFunDcl) - 3 * 2);
	f->Name = StorStr("true");
	ChainLst(d->FunDcl, fofs);
	pofs = GetZStor(sizeof(TPredicate) - 6);
	p->Name = StorStr("main");
	Roots->Predicates = *pofs;
	LexDom = MakeDomain(_FunD, "Lexem");
	*dofs = LexDom;
	fofs = GetZStor(sizeof(TFunDcl));
	f->Name = StorStr("lex");
	f->Arity = 3;
	f->Arg[0] = IntDom; f->Arg[1] = IntDom; f->Arg[2] = StrDom;
	ChainLst(d->FunDcl, fofs);
	LLexDom = MakeDomain(_ListD, "L_Lexem");
	*dofs = LLexDom; d->ElemDom = LexDom;
	MemPred = MakePred("mem_?", "ii", _MemP, 0xffff);
	MakePred("concat", "sss", _ConcatP, 0xffff);
	MakePred("call", "ss", _CallP, 3/*ii*/);
	LenPred = MakePred("len_?", "ii", _LenP, 1/*io*/);
	InvPred = MakePred("inv_?", "ii", _InvP, 1/*io*/);
	AddPred = MakePred("add_?", "iii", _AddP, 3/*iio*/);
	DelPred = MakePred("del_?", "iii", _DelP, 2/*oio*/);
	UnionPred = MakePred("union_?", "iii", _UnionP, 3/*iio*/);
	MinusPred = MakePred("minus_?", "iii", _MinusP, 3/*iio*/);
	InterPred = MakePred("inter_?", "iii", _InterP, 3/*iio*/);
	MakePred("abbrev", "ss", _AbbrevP, 1/*io*/);
	MakePred("fandfile", "ssss", _FandFileP, 0/*oooo*/);
	MakePred("fandfield", "sssiiis", _FandFieldP, 0xffff);
	MakePred("fandkey", "ssbb", _FandKeyP, 1/*iooo*/);
	MakePred("fandkeyfield", "sssbb", _FandKeyFieldP, 3/*iiooo*/);
	MakePred("fandlink", "sssssi", _FandLinkP, 0xffff);
	MakePred("fandlinkfield", "sss", _FandLinkFieldP, 3/*iio*/);
	MakePred("nextlex", "", _NextLexP, 0);
	MakePred("getlex", 'x', _GetLexP, 0/*o*/);
	ResetCompilePars(); RdLex();
	while (Lexem != 0x1A) {
		Accept('#');
		if (IsKeyWord("DOMAINS")) RdDomains;
		else if (IsKeyWord("CONSTANTS")) RdConstants;
		else if (IsKeyWord("DATABASE")) {
			s[0] = 0;
			if (Lexem = '-') {
				RdLex(); TestIdentif();
				s = LexWord; RdLex();
			}
			dbofs = FindDataBase(s);
			if (dbofs = 0) {
				dbofs = GetZStor(sizeof(TDatabase) - 1 + s.length());
				ChainLst(Roots->Databases, dbofs);
				//Move(s, db->Name, s.length() + 1);
				db->Name = s;
			}
			do {
				RdPredicateDcl(false, db);
			} while (Lexem != 0x1A && Lexem != '#');
		}
		else {
			if (IsKeyWord("PREDICATES")) {
				do {
					RdPredicateDcl(false, nullptr);
				} while (Lexem != 0x1A && Lexem != '#');
			}
			else { AcceptKeyWord("CLAUSES"); RdClauses(); }
		}
	}
	if (AbsAdr(HeapPtr) - AA > MaxLStrLen) OldError(544);
	dbofs = Roots->Databases;
	while (dbofs != 0) {
		db->SOfs = WORD(OPtr(_Sg, SaveDb(dbofs, AA)));
		dbofs = db->Chain;
	}
	CheckPredicates(Roots->Predicates);
	ss->LL = AbsAdr(HeapPtr) - AA;
	if (ProlgCallLevel == 0) ReleaseStore2(p2);
	else {
		Mem1.Release(pp1); Mem2.Release(pp2); Mem3.Release(pp3);
	}
	if (RecNr != 0) {
		CFile = Chpt; CRecPtr = cr;
		StoreChptTxt(ChptOldTxt, ss, true);
		WriteRec(RecNr);
		ReleaseStore(p1);
	}
	return result;
}


// ******************* odsud je obsah RUNPROLG.PAS ********************************************
const WORD MaxPackedPredLen = 4000;

struct TTerm {
	BYTE Fun = 0;
	BYTE Arity = 0;
	TTerm* Arg[3];
	integer II = 0;
	double RR = 0.0;
	pstring SS;
	longint Pos = 0;
	TTerm* Elem = nullptr; TTerm* Next = nullptr;
};

struct TInstance {
	WORD Pred = 0; /*PPredicate*/
	TInstance* PrevInst = nullptr; TInstance* RetInst = nullptr; WORD RetCmd = 0; /*PCommand*/
	TBranch* NextBranch = nullptr; /*PDbBranch,PFileScan*/
	WORD RetBranch/*PBranch*/; void* StkMark; longint WMark; WORD CallLevel;
	TTerm* Vars[7];
};

struct TFileScan {
	longint IRec = 0, Count = 0;
};

TInstance* CurrInst = nullptr;
char* PackedTermPtr = nullptr; WORD PTPMaxOfs = 0;
integer TrcLevel = 0, CallLevel = 0;
TTerm* LexemList = nullptr;

bool Trace()
{
	/*asm
	xor ax, ax; mov bx, TrcLevel; cmp bx, ax; je @1;
	cmp bx, CallLevel; jb @1; mov ax, 1; { if TrcLevel < >0 and >= CallLevel }
	@1: end;*/

	return false;
}

void SetCallLevel(WORD Lv)
{
	/*
	asm mov ax,Lv; mov CallLevel,ax; { CallLevel=lv  if =0 then TrcLevel=0 }
	cmp ax,0; jne @1; mov TrcLevel,ax;
	@1: end;
	*/
}

void WaitC()
{
	WORD c = ReadKey();
	if ((c == _ESC_) && PromptYN(21)) GoExit();
}

/*  T M E M O R Y  =========================================================*/
const WORD MemBlockSize = 8192;

TMemory::TMemory()
{
	CurBlk = nullptr;
	CurLoc = nullptr;
	RestSz = 0;
	FreeList = nullptr;
}

void* TMemory::Get(WORD Sz)
{
	void* p = nullptr;
	TMemBlkHd* p1 = nullptr; TMemBlkHd* p2 = nullptr;
	WORD n = 0;
	if (Sz > RestSz) {
		n = Sz + sizeof(TMemBlkHd);
		if (n < MemBlockSize) n = MemBlockSize;
		p2 = PMemBlkHd(@FreeMemList);
		p1 = FreeMemList;
		while (p1 != nullptr) {
			if (p1->Sz >= n) { p2->Chain = p1->Chain; goto label1; }
			p2 = p1; p1 = p1->Chain;
		}
		p1 = GetStore2(n);
		p1->Sz = n;
	label1:
		p1->Chain = CurBlk;
		CurBlk = p1;
		CurLoc = Ptr(Seg(p1^), Ofs(p1^) + sizeof(TMemBlkHd));
		RestSz = p1->Sz - sizeof(TMemBlkHd);
	}
	p = CurLoc;
	PtrRec(CurLoc).Ofs += Sz;
	RestSz -= Sz;
	/*asm les di, p; mov al, 0; mov cx, Sz; cld; rep stosb;*/
	return p;
}

void* TMemory::Mark()
{
	return CurLoc;
}

void TMemory::Release(void* P) /* only for pure stack */
{
	TMemBlkHd* p1; TMemBlkHd* p2;
	p1 = CurBlk;
	while (PtrRec(p1).Seg != PtrRec(p).Seg) {
		p2 = p1->Chain;
		p1->Chain = FreeMemList;
		FreeMemList = p1;
		p1 = p2;
	}
	CurBlk = p1;
	CurLoc = p;
	if (p = nullptr) RestSz = 0;
	else RestSz = p1->Sz - (PtrRec(p).Ofs - (PtrRec(p1).Ofs + sizeof(TMemBlkHd)));
}

pstring TMemory::StoreStr(pstring s)
{
	void* p;
	p = Get(s.length() + 1);
	Move(s, p^, s.length() + 1);
	return p;
}

void* TMemory::Alloc(WORD Sz) /* doesn't free once allocated blocks */
{
	TMemBlkHd* p; TMemBlkHd* p1; TMemBlkHd* p2;
	Sz = (Sz + 7) & 0xfff8;
	p1 = (TMemBlkHd*)(&FreeList);
	p = FreeList;
	while (p != nullptr) {
		if (p->Sz >= Sz) {
			if (p->Sz > Sz) {
				p2 = p;
				PtrRec(p2).Ofs += Sz;
				p1->Chain = p2;
				p2->Chain = p->Chain;
				p2->Sz = p->Sz - Sz;
			}
			else p1->Chain = p->Chain;
			/* asm les di, p; mov al, 0; mov cx, Sz; cld; rep stosb;*/
			return p;
		}
		p1 = p;
		p = p->Chain;
	}
	return Get(Sz);
}

void TMemory::Free(void* P, WORD Sz)
{
	TMemBlkHd* p1 = nullptr; TMemBlkHd* p2 = nullptr;
	TMemBlkHd* pp = (TMemBlkHd*)P;
	Sz = (Sz + 7) & 0xfff8;
	p1 = (TMemBlkHd*)FreeList;
	p2 = FreeList;
	while ((PtrRec(p2).Seg != 0) && (PtrRec(p2).Seg != PtrRec(P).Seg)) {
		p1 = p2; p2 = p2->Chain;
	}
	while ((PtrRec(p2).Seg = PtrRec(P).Seg) && (PtrRec(p2).Ofs < PtrRec(P).Ofs)) {
		p1 = p2; p2 = p2->Chain;
	}
	if ((PtrRec(P).Seg = PtrRec(p2).Seg) && (PtrRec(P).Ofs + Sz = PtrRec(p2).Ofs)) {
		pp->Sz = Sz + p2->Sz; pp->Chain = p2->Chain;
	}
	else { pp->Sz = Sz; pp->Chain = p2; }
	if ((PtrRec(p1).Seg = PtrRec(P).Seg) && (PtrRec(p1).Ofs + p1->Sz = PtrRec(P).Ofs)) {
		p1->Sz = p1->Sz + pp->Sz; p1->Chain = pp->Chain;
	}
	else p1->Chain = pp;
}

/*  T D O M A I N  =========================================================*/
TFunDcl* GetFunDcl(WORD D, BYTE I)
{
	TFunDcl* fd = nullptr;
	WORD fdofs = 0; // absolute fd
	PtrRec(fd).Seg = _Sg;
	fdofs = (TDomain*)(ptr(_Sg, D))->FunDcl;
	while (I > 0) { I--; fdofs = fd->Chain; }
	return fd;
}

/*  T T E R M  =============================================================*/
TTerm* GetIntTerm(integer I)
{
	TTerm* t = Mem1.Get(1 + sizeof(integer));
	/* !!! with t^ do!!! */
	{ t->Fun = _IntT; t->II = I; }
	return t;
}

TTerm* GetRealTerm(double R)
{
	TTerm* t = Mem1.Get(1 + sizeof(double));
	/* !!! with t^ do!!! */
	{ t->Fun = _RealT; t->RR = R; }
	return t;
}

TTerm* GetBoolTerm(bool B)
{
	TTerm* t = Mem1.Get(1 + 1);
	t->Fun = B;
	return t;
}

TTerm* GetStringTerm(pstring S)
{
	TTerm* t = Mem1.Get(1 + 1 + S.length());
	/* !!! with t^ do!!! */
	{ t->Fun = _StrT; Move(S, SS, S.length() + 1); }
	return t;
}

TTerm* GetLongStrTerm(longint N)
{
	TTerm* t = Mem1.Get(1 + 4);
	/* !!! with t^ do!!! */
	{ t->Fun = _LongStrT; t->Pos = N; }
	return t;
}

TTerm* GetListTerm(TTerm* aElem, TTerm* aNext)
{
	TTerm* t = Mem1.Get(1 + 2 * 4);
	/* !!! with t^ do!!! */
	{
		t->Fun = _ListT;
		t->Elem = aElem;
		t->Next = aNext;
	}
	return t;
}

TTerm* GetFunTerm(BYTE aFun, BYTE aArity)
{
	TTerm* t = Mem1.Get(1 + 1 + aArity * 4);
	/* !!! with t^ do!!! */
	{ t->Fun = aFun; t->Arity = aArity; }
	return t;
}

void ChainList(void* Frst, void* New)
{
	/*
	asm  push ds; lds si,Frst; sub si,TTerm.Next;
	@1:  cmp [si+2].TTerm.Next.word,0; je @2;
	 lds si,[si].TTerm.Next;
	 jmp @1;
	@2:  les di,New; mov [si].TTerm.Next.word,di; mov [si+2].TTerm.Next.word,es;
	 pop ds;
	end;
	*/
}

pstring XXS;
LongStr* RdLongStr(longint Pos)
{
	LongStrPtr p = GetStore(2);
	WORD l = 0;
	SeekH(WorkHandle, Pos);
	ReadH(WorkHandle, 2, l);
	p->LL = l;
	if (l > 0) { GetStore(l); ReadH(WorkHandle, l, p->A); }
	return p;
}

longint WrLongStrLP(WORD L, void* P)
{
	auto result = MaxWSize;
	SeekH(WorkHandle, MaxWSize);
	WriteH(WorkHandle, 2, &L);
	WriteH(WorkHandle, L, P);
	MaxWSize += L + 2;
	return result;
}

longint WrLongStr(LongStr* S)
{
	return WrLongStrLP(S->LL, S->A);
}

LongStr* RunLSExpr(WORD TOfs);
void RunSExpr(WORD TOfs, pstring* s);
double RunRExpr(WORD TOfs);

integer RunIExpr1(TPTerm* T)
{
	pstring s, s2;
	integer i = 0, err = 0, l = 0;
	LongStr* ss = nullptr;
	switch (T->Op) {
	case _length: { RunSExpr(t->E1, &s); i = s.length(); break; }
	case _val: { RunSExpr(t->E1, &s); val(s, i, err); break; }
	case _pos: {
		RunSExpr(t->E1, &s);
		ss = RunLSExpr(t->E2);
		l = ss->LL;
		i = FindText(s, "", @ss->A, l);
		if (i > 0) i = i - s.length();
		ReleaseStore(ss);
		break;
	}
	}
	return i;
}

integer RunIExpr(WORD TOfs/*PPTerm*/)
{
	TPTerm* t = ptr(_Sg, TOfs);
	if (t->Fun == _VarT) { return CurrInst->Vars[t->Idx]->II; }
	switch (t->Op) {
	case _const: return t->II;
	case '^': return !RunIExpr(t->E1);
	case _and: return RunIExpr(t->E1) && RunIExpr(t->E2);
	case _or: return RunIExpr(t->E1) || RunIExpr(t->E2);
	case '+': return RunIExpr(t->E1) + RunIExpr(t->E2);
	case '-': return RunIExpr(t->E1) - RunIExpr(t->E2);
	case '*': return RunIExpr(t->E1) * RunIExpr(t->E2);
	case '/': return RunIExpr(t->E1) / RunIExpr(t->E2);
	case _conv: return trunc(RunRExpr(t->E1));
	case _min: return MinI(RunIExpr(t->E1), RunIExpr(t->E2));
	case _max: return MaxI(RunIExpr(t->E1), RunIExpr(t->E2));
	case _maxcol: return TxtCols;
	case _maxrow: return TxtRows;
	default: return RunIExpr1(t);
	}
}

double RunRExpr(WORD TOfs/*PPTerm*/)
{
	TPTerm* t = ptr(_Sg, TOfs);
	if (t->Fun == _VarT) { return CurrInst->Vars[t->Idx]->RR; }
	switch (t->Op) {
	case _const: return t->RR;
	case '+': return RunRExpr(t->E1) + RunRExpr(t->E2);
	case '-': return RunRExpr(t->E1) - RunRExpr(t->E2);
	case '*': return RunRExpr(t->E1) * RunRExpr(t->E2);
	case '/': return RunRExpr(t->E1) / RunRExpr(t->E2);
	case _conv: return RunIExpr(t->E1);
	}
}

void RunSExpr1(TPTerm* T, pstring* s)
{
	pstring s2;
	WORD tofs = 0; // absolute T
	WORD t1ofs = 0, l = 0, l2 = 0;
	bool b = false;
	do {
		b = (t->Fun == _StrT) && (t->Op == '+');
		if (b) t1ofs = t->E1;
		else t1ofs = tofs;
		RunSExpr(t1ofs, s2);
		l2 = MinW(s2.length(), 255 - l);
		Move(s2[1], s[l + 1], l2);
		l += l2;
		if (b) tofs = t->E2;
	} while (b);
	s[0] = char(l);
}

void RunSExpr(WORD TOfs, pstring* s)
{
	TPTerm* t = nullptr;
	WORD i = 0, n = 0, l = 0;
	LongStr* p = nullptr;
	pstring* q = nullptr;
	t = ptr(_Sg, TOfs);
	if (t->Fun = _VarT) { q = CurrInst->Vars[t->Idx]->SS; goto label1; }
	else {
		switch (t->Op) {
		case _const: {
			q = &t->SS;
		label1:
			Move(q^, s, q->length() + 1);
			break;
		}
		case '+': RunSExpr1(t, s); break;
		case _conv: {
			p = RunLSExpr(t->E1);
			l = MinW(p->LL, 255);
			s[0] = char(l);
			Move(p->A, &s[1], l);
			ReleaseStore(p);
			break;
		}
		case _copy: {
			RunSExpr(t->E1, s);
			i = RunIExpr(t->E2);
			n = RunIExpr(t->E3);
			l = s->length();
			i = MaxW(1, MinW(i, l + 1));
		label2:
			n = MinW(n, l + 1 - i);
			s[0] = char(n);
			Move(&s[i], &s[1], n);
			break;
		}
		case _leadchar: {
			RunSExpr(t->E2, s);
			i = 1;
			l = s->length();
			n = l;
			while ((i <= l) && (s[i] == char(t->E1))) i++;
			goto label2;
			break;
		}
		case _trailchar: {
			RunSExpr(t->E2, s);
			l = s->length();
			while ((l > 0) && (s[l] == char(t->E1))) l--;
			s[0] = char(l);
			break;
		}
		case _repeatstr: {
			RunSExpr(t->E1, s);
			n = RunIExpr(t->E2);
			l = s->length();
			i = 0;
			while ((n > 0) && (i + l <= 255)) {
				n--;
				Move(&s[1], &s[i + 1], l);
				i += l;
			}
			s[0] = char(i);
			break;
		}
		case _str: str(RunIExpr(t->E1), s); break;
		}
	}
}

LongStr* RunLSExpr(WORD TOfs)
{
	TPTerm* t = ptr(_Sg, TOfs);
	LongStr* p = nullptr; LongStr* p2 = nullptr;
	WORD l = 0;
	pstring* s = nullptr;

	if (t->Fun == _VarT) p = RdLongStr(CurrInst->Vars[t->Idx]->Pos);
	else {
		switch (t->Op) {
		case _const: {
			l = t->SS.length();
			p = GetStore(l + 2);
			p->LL = l;
			Move(&t->SS[1], &p->A, l);
			break;
		}
		case '+': {
			p = RunLSExpr(t->E1);
			p2 = RunLSExpr(t->E2);
			l = p2->LL;
			p->LL = p->LL + l;
			Move(&p2->A, &p2->LL, l);
			ReleaseAfterLongStr(p);
			break;
		}
		case _conv: {
			p = GetStore(257);
			s = ptr(PtrRec(p).Seg, PtrRec(p).Ofs + 1);
			RunSExpr(t->E1, s);
			p->LL = s->length();
			ReleaseAfterLongStr(p);
			break;
		}
		}
	}
	return p;
}

bool UnifyTermsCC(TTerm* T1, TTerm* T2)
{
	integer i = 0;
	LongStr* p1 = nullptr; LongStr* p2 = nullptr;
	auto result = true;
	if (T2 == nullptr) { if (T1 != nullptr) return false; }
	if ((T1 == nullptr) || (T1->Fun != T2->Fun)) return false;
	else switch (T2->Fun) {
	case _IntT: return T1->II == T2->II;
	case _RealT: return T1->RR == T2->RR;
	case _StrT: return T1->SS == T2->SS;
	case _LongStrT: {
		p1 = RdLongStr(T1->Pos);
		p2 = RdLongStr(T2->Pos);
		result = EquLongStr(p1, p2);
		ReleaseStore(p1);
		return result;
	}
	case _ListT: return UnifyTermsCC(T1->Elem, T2->Elem) && UnifyTermsCC(T1->Next, T2->Next);
	default: {
		for (i = 0; i <= (integer)T1->Arity - 1; i++)
			if (!UnifyTermsCC(T1->Arg[i], T2->Arg[i])) {
				return false;
			}
	}
	}
	return result;
}

bool UnifyTermsCV(TTerm* T1, WORD T2Ofs/*PPTerm*/)
{
	integer i = 0;
	TPTerm* T2 = nullptr;
	LongStr* p = nullptr; LongStr* p2 = nullptr;
	auto result = true;
	if (T2Ofs == 0) { if (T1 != nullptr) return false; }
	T2 = ptr(_Sg, T2Ofs);
	switch (T2->Fun) {
	case _VarT: {
		if (T2->Bound) result = UnifyTermsCC(T1, CurrInst->Vars[T2->Idx]);
		else CurrInst->Vars[T2->Idx] = T1;
		break;
	}
	case _UnderscT: break;
	default: {
		if ((T1 == nullptr) || (T1->Fun != T2->Fun)) goto label1;
		else switch (T2->Fun) {
		case _IntT: result = T1->II == RunIExpr(T2Ofs); break;
		case _RealT: result = T1->RR == RunRExpr(T2Ofs); break;
		case _StrT: {
			if (T2->Op == _const) result = T1->SS == T2->SS;
			else {
				RunSExpr(T2Ofs, XXS);
				result = T1->SS == XXS;
			}
			break;
		}
		case _LongStrT: {
			p = RdLongStr(T1->Pos);
			p2 = RunLSExpr(T2Ofs);
			result = EquLongStr(p, p2);
			ReleaseStore(p);
			break;
		}
		case _ListT: {
			while (T2->Op = '+') {
				if (!UnifyVList(T1, ptr(_Sg, T2->E1))) goto label1;
				T2Ofs = T2->E2;
			}
			if (!UnifyVList(T1, T2) || (T1 != nullptr)) goto label1;
			break;
		}
		default: {
			for (i = 0; i <= integer(T1->Arity) - 1; i++)
				if (!UnifyTermsCV(T1->Arg[i], T2->Arg[i])) {
				label1:
					return false;
				}
			break;
		}
		}
	}
	}
}

bool UnifyVList(TTerm* TT1, TPTerm* T2)
{
	WORD t2ofs = 0; // absolute T2
	TTerm* t; TTerm* t1;
	t1 = TT1;
	auto result = false;
	while (t2ofs != 0)
		switch (T2->Fun) {
		case _VarT: {
			if (T2->Bound) {
				t = CurrInst->Vars[T2->Idx];
				while (t != nullptr) {
					if ((t1 = nullptr) || !UnifyTermsCC(t1->Elem, t->Elem)) return result;
					t = t->Next; t1 = t1->Next;
				}
			}
			else {
				CurrInst->Vars[T2->Idx] = t1;
				t1 = nullptr;
			}
			goto label1;
			break;
		}
		case _UnderscT: { t1 = nullptr; goto label1; break; }
		default: {
			if ((t1 = nullptr) || !UnifyTermsCV(t1->Elem, T2->Elem)) return result;
			t1 = t1->Next; t2ofs = T2->Next;
			break;
		}
		}
label1:
	result = true;
	TT1 = t1;
	return result;
}

bool FindInCList(TTerm* tEl, TTerm* t)
{
	auto result = true;
	while (t != nullptr) {
		if (UnifyTermsCC(tEl, t->Elem)) return result;
		t = t->Next;
	}
	return false;
}

TTerm* CopyCList(TTerm* T)
{
	TTerm* root = nullptr;
	TTerm* t1 = nullptr; TTerm* prev = nullptr;
	if (T == nullptr) { return nullptr; }
	root = nullptr;
	do {
		t1 = Mem1.Get(1 + 2 * 4);
		t1->Fun = _ListT;
		t1->Elem = T->Elem;
		if (root == nullptr) root = t1;
		else prev->Next = t1;
		prev = t1;
		T = T->Next
	} while (T != nullptr);
		return root;
}

TTerm* CopyTerm(WORD TOff/*PPTerm*/)
{
	TPTerm* t = nullptr; PTerm* t1 = nullptr;
	WORD tofs = 0; // absolute t
	TTerm* t2 = nullptr;
	integer i = 0;
	LongStr* p = nullptr;
	if (TOff == 0) { return nullptr; }
	t = ptr(_Sg, TOff);
	switch (t->Fun) {
	case _IntT: return GetIntTerm(RunIExpr(TOfs)); break;
	case _RealT: return GetRealTerm(RunRExpr(TOfs)); break;
	case _StrT: {
		if (t->Op == _const) return GetStringTerm(t->SS);
		else {
			RunSExpr(TOff, XXS);
			return GetStringTerm(XXS);
		}
		break;
	}
	case _LongStrT: {
		p = RunLSExpr(TOff);
		auto result = GetLongStrTerm(WrLongStr(p));
		ReleaseStore(p);
		return result;
		break;
	}
	case _VarT: return CurrInst->Vars[T->Idx]; break;
	case _ListT: {
		t2 = nullptr;
		while (t->Op = '+') {
			ChainList(t2, CopyVList(ptr(_Sg, t->E1), true));
			tofs = t->E2;
		}
		ChainList(t2, CopyVList(t, false));
		return t2;
		break;
	}
	default: {
		t2 = GetFunTerm(T->Fun, T->Arity);
		for (i = 0; i <= integer(T->Arity) - 1; i++) t2->Arg[i] = CopyTerm(T->Arg[i]);
		return t2;
		break;
	}
	}
}

TTerm* CopyVList(TPTerm* T, bool Cpy)
{
	WORD tofs = 0; // absolute T
	TTerm* root = nullptr; TTerm* t1 = nullptr; TTerm* prev = nullptr;
	if (tofs == 0) { return nullptr; }
	root = nullptr;
label1:
	if (T->Fun == _VarT) {
		t1 = CurrInst->Vars[T->Idx];
		if (Cpy) t1 = CopyCList(t1);
	}
	else {
		t1 = Mem1.Get(1 + 2 * 4);
		t1->Fun = _ListT;
		t1->Elem = CopyTerm(T->Elem);
	}
	if (root == nullptr) root = t1;
	else prev->Next = t1;
	if ((T->Fun != _VarT)) {
		prev = t1;
		tofs = T->Next;
		if (tofs != 0) goto label1;
	}
	return root;
}

void PackTermC(TTerm* T)
{
	char* p = PackedTermPtr;
	integer i = 0;
	WORD n = 0;
	WORD* wp = nullptr;

	if (PtrRec(p).Ofs >= PTPMaxOfs) RunError(1527);
label1:
	if (T == nullptr /* [] */) { *(WORD*)p = 0; p += 2; }
	else {
		switch (T->Fun) {
		case _IntT: { *(integer*)p = T->II; p += 2; break; }
		case _RealT: { *(double*)p = T->RR; p += sizeof(double)); break; }
		case _StrT: {
			n = T->SS.length() + 1;
			if (PtrRec(p).Ofs + n >= PTPMaxOfs) RunError(1527);
			Move(t->SS, p, n);
			p += n;
			break;
		}
		case _LongStrT: RunError(1543); break;
		case _ListT: {
			wp = (WORD*)p;
			p += 2;
			n = 0;
			while (T != nullptr) { PackTermC(T->Elem); T = T->Next; n++; }
			*wp = n;
			break;
		}
		default: {
			*p = char(T->Fun);
			p++;
			for (i = 0; i <= integer(T->Arity) - 1; i++) PackTermC(T->Arg[i]);
		}
		}
	}
}

void PackTermV(WORD TOff/*PPTerm*/);

WORD PackVList(TPTerm* T)
{
	WORD tofs = 0; // absolute T
	WORD n = 0;
	TTerm* t1 = nullptr;
	do {
		if (T->Fun = _VarT) {
			t1 = CurrInst->Vars[T->Idx];
			while (t1 != nullptr) {
				PackTermC(t1->Elem);
				t1 = t1->Next;
				n++;
			}
			goto label1;
		}
		PackTermV(T->Elem);
		n++;
		tofs = T->Next;
	} while (tofs != 0);
label1:
	return n;
}

void PackTermV(WORD TOff/*PPTerm*/)
{
	char* p = PackedTermPtr;
	integer i = 0; WORD n = 0; WORD* wp = nullptr;
	TPTerm* T = nullptr;
	WORD tofs = 0; // absolute t
	TTerm* t1 = nullptr;
	if (PtrRec(p).Ofs >= PTPMaxOfs) RunError(1527);
	T = ptr(_Sg, TOff);
label1:
	if (tofs == 0 /* [] */) { *(WORD*)p = 0; p += 2; }
	else {
		switch (T->Fun) {
		case _VarT: PackTermC(CurrInst->Vars[T->Idx]); break;
		case _IntT: { *(integer*)p = RunIExpr(tofs); p += 2; break; }
		case _RealT: { *(double*)p = RunRExpr(tofs); p += sizeof(double)); break; }
		case _StrT: {
			RunSExpr(tofs, XXS);
			n = XXS.length() + 1;
			if (PtrRec(p).Ofs + n >= PTPMaxOfs) RunError(1527);
			Move(XXS, p, n);
			p += n;
			break;
		}
		case _LongStrT: RunError(1543); break;
		case _ListT: {
			wp = (WORD*)p;
			p += 2;
			*wp = 0;
			while (t->Op == '+') {
				*wp += PackVList(ptr(_Sg, t->E1)));
				tofs = t->E2;
			}
			*wp += PackVList(t));
			break;
		}
		default: {
			*p = char(t->Fun);
			p++;
			for (i = 0; i <= integer(t->Arity) - 1; i++) PackTermV(t->Arg[i]);
		}
		}
	}
}

LongStr* GetPackedTerm(TTerm* T)
{
	char* pt = PackedTermPtr;
	char A[MaxPackedPredLen + 1]{ '\0' };
	LongStr* s = nullptr; WORD n = 0;
	pt = (char*)A;
	PTPMaxOfs = ofs(A) + MaxPackedPredLen - 2;
	PackTermC(T);
	n = PtrRec(pt).Ofs - ofs(A);
	s = GetStore(2 + n);
	s->LL = n;
	Move(A, s->A, n);
	return s;
}

TTerm* UnpackTerm(WORD D)
{
	char* p = PackedTermPtr;
	integer i = 0; WORD n = 0; char* q = nullptr;
	TTerm* t = nullptr; TTerm* tPrev = nullptr;
	TTerm* t1 = nullptr; TFunDcl* f = nullptr;
	switch ((TDomain*)(ptr(_Sg, D))->Typ) {
	case _IntD: { t = GetIntTerm(*(integer*)p); p += 2; break; }
	case _RealD: { t = GetRealTerm(*(double*)p); p += sizeof(double); break; }
	case _StrD: { t = GetStringTerm(*(pstring*)p); p += *p + 1; break; }
	case _LongStrD: RunError(1543); break;
	case _ListD: {
		n = *(WORD*)p;
		p += 2;
		t = nullptr;
		while (n > 0) {
			t1 = GetListTerm(UnpackTerm((TDomain*)(ptr(_Sg, D))->ElemDom), nullptr);
			if (t == nullptr) t = t1;
			else tPrev->Next = t1;
			tPrev = t1;
			n--;
		}
		break;
	}
	default: {
		n = *p; p++;
		f = GetFunDcl(D, n);
		t = GetFunTerm(n, f->Arity);
		for (i = 0; i <= integer(f->Arity) - 1; i++) t->Arg[i] = UnpackTerm(f->Arg[i]);
		break;
	}
	}
	return t;
}
char* PrintPackedTerm(char* p, WORD D)
{
	integer i = 0, n = 0;
	TFunDcl* f = nullptr;
	switch (PDomain(ptr(_Sg, D))->Typ) {
	case _IntD: { printf(*(integer*)p); p += 2; break; }
	case _RealD: { printf(*(double*)p); p += sizeof(double); break; }
	case _StrD: { printf("'", *(pstring*)p->c_str(), "'"); p += *p + 1; break; }
	case _ListD: {
		printf("[");
		n = *(WORD*)p;
		p += 2;
		for (i = 1; i <= n; i++) {
			if (i > 1) printf(",");
			p = PrintPackedTerm(p, PDomain(ptr(_Sg, D))->ElemDom);
		}
		printf("]");
		break;
	}
	default: {
		f = GetFunDcl(D, p);
		p++;
		printf(*(pstring*)(ptr(_Sg, f->Name)));
		if (f->Arity > 0) {
			printf("(");
			for (i = 1; i <= f->Arity; i++) {
				if (i > 1) printf(",");
				p = PrintPackedTerm(p, f->Arg[i - 1]);
			}
			printf(")");
		}
	}
	}
	return p;
}

void PrintPackedPred(char* Q, WORD POfs/*PPredicate*/)
{
	integer i = 0, n = 0;
	TPredicate* p = ptr(_Sg, POfs);
	printf("CALL assert(", (pstring*)(ptr(_Sg, p->Name))->c_str());
	n = p->Arity;
	if (n > 0) {
		printf("(");
		for (i = 1; i <= n; i++) {
			if (i > 1) printf(",");
			Q += 2;
			Q = PrintPackedTerm(Q, p->Arg[i - 1]);
		}
		printf(")");
	}
	printf(")");
	WaitC();
}

void PrintTerm(TTerm* T, WORD DOfs)
{
	TFunDcl* fd = nullptr;
	WORD i = 0; LongStr* p = nullptr;
	TDomain* d = nullptr;

	if (T == nullptr) printf("[]");
	else {
		switch (T->Fun) {
		case _IntT: printf(T->II); break;
		case _RealT: printf(T->RR); break;
		case _StrT: printf("'", T->SS, "'"); break;
		case _LongStrT: {
			p = RdLongStr(T->Pos);
			printf("'");
			for (i = 1; i <= p->LL; i++) printf(p->A[i]);
			printf("'");
			ReleaseStore(p);
			break;
		}
		case _ListT: {
			printf("[");
			d = ptr(_Sg, DOfs);
			i = 0;
		label1:
			PrintTerm(T->Elem, d->ElemDom);
			T = T->Next;
			if (T != nullptr) {
				printf(","); i++;
				if ((i == 3) && (d->Name == "L_Lexem")) printf("...");
				else goto label1;
			}
			printf("]");
			break;
		}
		default: {
			fd = GetFunDcl(DOfs, T->Fun);
			printf((pstring*)(ptr(_Sg, fd->Name))->c_str());
			if (T->Arity == 0) return;
			break;
		}
		}
	}
	printf("(");
	for (i = 0; i <= T->Arity - 1; i++) {
		if (i > 0) printf(",");
		PrintTerm(T->Arg[i], fd->Arg[i]);
	}
	printf(")");
}

WORD LenDbEntry(LongStr* S, integer Arity)
{
	WORD n = 0; integer i = 0;
	for (i = 1; i <= Arity; i++) {
		n += S->LL + 2;
		PtrRec(S).Ofs += S->LL + 2;
	}
	return n;
}

LongStr* SaveDb(WORD DbOfs,/*PDatabase*/ longint AA)
{
	WORD n = 0, arity = 0; longint l = 0;
	LongStr* s = nullptr; char* q = nullptr; bool x = false;
	TPredicate* p = nullptr;
	WORD pofs = 0; // absolute p
	TDbBranch* b = nullptr; TDatabase* db = nullptr;
	s = GetStore(2);
	db = ptr(_Sg, DbOfs);
	p = ptr(_Sg, Db->Pred);
	x = AA != 0;
	while (pofs != 0) {
		if ((p->Opt & _FandCallOpt) == 0) {
			arity = p->Arity;
			if (!x) {
				StoreStr((pstring)(ptr(_Sg, p->Name)));
				*(BYTE*)(GetStore(1)) = arity;
			}
			b = (TDbBranch*)p->Branch;
			while (b != nullptr) {
				n = LenDbEntry((LongStr*)b->LL, arity);
				q = GetStore(n + 1);
				if (x && (AbsAdr(HeapPtr) - AA > MaxLStrLen)) OldError(544);
				q[0] = 1;
				Move(&b->LL, &q[1], n);
				b = b->Chain;
			}
			*(BYTE*)(GetStore(1)) = 0;
		}
		pofs = p->ChainDb;
	}
	l = AbsAdr(HeapPtr) - AbsAdr(s) - 2;
	s->LL = l;
	if (l > MaxLStrLen) {
		SetMsgPar(Db->Name);
		RunError(1532);
	}
	return s;
}

void ConsultDb(LongStr* S, WORD DbOfs/*PDatabase*/)
{
	WORD n = 0; char* q = nullptr;
	TPredicate* p = nullptr;
	WORD pofs = 0; // absolute p
	TDbBranch* b = nullptr; TDatabase* db = nullptr;
	q = (char*)(S->A);
	db = ptr(_Sg, DbOfs);
	p = ptr(_Sg, Db->Pred);
	while (pofs != 0) {
		if ((p->Opt & _FandCallOpt) == 0) {
			if (PtrRec(S).Seg != _Sg) {
				if (*(pstring*)q != *(pstring*)(ptr(_Sg, p->Name))) goto label1;
				q += *q + 1;
				if (*q != p->Arity) goto label1;
				q++;
			}
			while (q[0] == 1) {
				q++;
				n = LenDbEntry(LongStrPtr(q), p->Arity);
				b = Mem3.Alloc(n + 4);
				ChainLast(p->Branch, b);
				Move(q[0], b->LL, n);
				q += n;
			}
			if (q[0] != 0) goto label1;
			q++;
		}
		pofs = p->ChainDb;
	}
	if (S->LL != AbsAdr(q) - AbsAdr(S) - 2) {
	label1:
		SetMsgPar(Db->Name);
		RunError(1533);
	}
}

bool Vokal(char C)
{
	std::set<char> charset;
	char kam = CurrToKamen(C);
	Vokal = CurrToKamen(C) in charset;
}

bool IsUpper(char C)
{
	return (C != '.') && (C == UpcCharTab[C]);
}

pstring Abbrev(pstring S)
{
	pstring t; integer i = 0, j = 0;
	pstring oldT;
	t[0] = 0;
	i = S.length();
	while (i > 0) {
		if (S[i] == ' ') {
			while (S[i] == ' ') { i--; if (i == 0) goto label9; }
			if (t.length() > 0) {
				oldT = t;
				t = " ";
				t += oldT;
			}
		}
		j = i;
		if (IsUpper(S[i])) goto label1;
		while (Vokal(S[i])) {
			i--;
			if ((i == 0) || (S[i] == '.' || S[i] == ' ')) goto label2;
		}
		while (!Vokal(S[i])) {
			i--;
			if ((i == 0) || (S[i] == '.' || S[i] == ' ')) goto label2;
		}
		while (Vokal(S[i])) {
			i--;
			if ((i == 0) || (S[i] == '.' || S[i] == ' ')) goto label2;
		}
		j = i;
		oldT = t;
		t = ".";
		t += oldT;
	label1:
		while ((i > 0) && !(S[i] == '.' || S[i] == ' ')) i--;
	label2:
		t = copy(S, i + 1, j - i) + t;
	}
	if (t.length() == S.length()) {
		i = 1;
		while (i < t.length() - 1) {
			if ((t[i] == '.') && (t[i + 1] == ' ')) t.Delete(i + 1, 1);
			else i++;
		}
	}
label9:
	return t;
}

FileD* NextFD(FileD* FD)
{
	RdbDPtr r;
	if (FD == nullptr) { r = CRdb; FD = r->FD; }
	else r = FD->ChptPos.R; /*not .RDB*/
label1:
	FD = FD->Chain;
	if (FD == nullptr) {
		r = r->ChainBack;
		if (r != nullptr) { FD = r->FD; goto label1; }
	}
	else if ((FD->Typ == '0') || (FD->ChptPos.R == nullptr)) goto label1;
	return FD;
}

FileD* FindFD(pstring FDName)
{
	FileD* fd = nullptr;
	do {
		fd = NextFD(fd);
	} while (!((fd == nullptr) || SEquUpcase(fd->Name, FDName)));
	return fd;
}

pstring Pound(pstring s)
{
	if (s == "") return '@';
	return s;
}

bool RunBuildIn()
{
	WORD l = 0, l1 = 0, l2 = 0, l3 = 0, n = 0, m = 0, w = 0;
	pstring s;
	LongStr* p = nullptr; LongStr* p1 = nullptr; LongStr* p2 = nullptr; LongStr* p3 = nullptr;
	RdbD* r = nullptr; FileD* fd = nullptr; FieldDescr* f = nullptr;
	KeyD* k = nullptr; KeyFldD* kf = nullptr;
	TTerm* t1 = nullptr; TTerm* t2 = nullptr; TTerm* tprev = nullptr;
	TTerm* t = nullptr; TTerm* root = nullptr;
	LinkD* ld = nullptr; pstring* mask = nullptr;
	integer i = 0, err = 0;
	TCommand* c = nullptr; bool b = false;
	TInstance* q = nullptr; RdbPos pos;

	/* !!! with CurrInst^ do!!! */
	{
		c = ptr(_Sg, RetCmd);
		w = c->InpMask;
		switch ((TPredicate*)(ptr(_Sg, Pred))->LocVarSz) {
		case _NextLexP: if (LexemList != nullptr) LexemList = LexemList->Next; break;
		case _GetLexP: Vars[0] = LexemList; break;
		case _ConcatP: {
			switch (w) {
			case 7/*iii*/: { if (Vars[0]->SS + Vars[1]->SS != Vars[2]->SS) goto label1; break; }
			case 3/*iio*/: { Vars[2] = GetStringTerm(Vars[0]->SS + Vars[1]->SS); break; }
			case 5/*ioi*/: {
				l = Vars[0]->SS.length();
				if (Vars[0]->SS != copy(Vars[2]->SS, 1, l)) goto label1;
				Vars[1] = GetStringTerm(copy(Vars[2]->SS, l + 1, 255));
				break;
			}
			case 6/*oii*/: {
				l = Vars[1]->SS.length();
				l2 = Vars[2]->SS.length();
				if (Vars[1]->SS != copy(Vars[2]->SS, l2 - l + 1, l)) {
				label1:
					NextBranch = nullptr;
					return false;
				}
				Vars[0] = GetStringTerm(copy(Vars[2]->SS, 1, l2 - l));
				break;
			}
			case 4/*ooi*/: {
				n = WORD(NextBranch);
				s = Vars[2]->SS;
				if (n == 0) n = s.length();
				if (n == 0) goto label1;
				n--;
				Vars[0] = GetStringTerm(copy(s, 1, s.length() - n));
				Vars[1] = GetStringTerm(copy(s, s.length() - n + 1, n));
				WORD(NextBranch) = n;
				break;
			}
			}
			break;
		}
		case _MemP: {
			switch (w) {
			case 3/*ii*/: {
				t1 = Vars[0]; t2 = Vars[1];
				while (t2 != nullptr) {
					if (UnifyTermsCC(t1, t2->Elem)) goto label3;
					t2 = t2->Next;
				}
				goto label1;
				break;
			}
			case 2/*oi*/: {
				t2 = PTerm(NextBranch);
				if (t2 = nullptr) {
					t2 = Vars[1];
					if (t2 = nullptr) goto label1;
				}
				Vars[0] = t2->Elem;
				NextBranch = PBranch(t2->Next);
				break;
			}
			}
		}
		case _FandFileP: {
			fd = (FileD*)NextBranch;
			if (fd == nullptr) {
				fd = NextFD(nullptr);
				if (fd == nullptr) goto label1;
			}
			Vars[0] = GetStringTerm(fd->Name);
			s[0] = 0;
			switch (fd->Typ) {
			case '6': if (fd->IsSQLFile) s = "SQL"; break;
			case 'X': s = 'X'; break;
			case 'D': s = "DBF"; break;
			case '8': s = "DTA"; break;
			}
			Vars[1] = GetStringTerm(s);
			Vars[2] = GetStringTerm(fd->ChptPos.R->FD->Name);
			CFile = fd;
			SetCPathVol();
			Vars[3] = GetStringTerm(CPath);
			NextBranch = PBranch(NextFD(fd));
			break;
		}
		case _FandFieldP: {
			f = (FieldDescr*)NextBranch;
			if (f == nullptr) {
				fd = FindFD(Vars[0]->SS);
				if (fd == nullptr) goto label1;
				f = fd->FldD;
			}
			if (w == 3/*ii*/) {
				while ((f != nullptr) && !SEquUpcase(f->Name, Vars[1]->SS)) f = f->Chain;
				if (f == nullptr) goto label1;
			}
			else Vars[1] = GetStringTerm(f->Name);
			Vars[2] = GetStringTerm(f->Typ);
			m = 0; l = f->L;
			if (f->Typ == 'F') {
				m = f->M;
				l--;
				if (m > 0) l -= (m + 1);
			}
			Vars[3] = GetIntTerm(l);
			Vars[4] = GetIntTerm(m);
			m = f->Flg;
			if ((f->Typ == 'N' || f->Typ == 'A')) m = m | (f->M << 4);
			Vars[5] = GetIntTerm(m);
			if ((f->Flg & f_Mask != 0)) Mask = FieldDMask(f);
			else Mask = StringPtr(@EmptyStr);
			Vars[6] = GetStringTerm(Mask^);
			if (w == 3) NextBranch = nullptr;
			else NextBranch = PBranch(f->Chain);
			break;
		}
		case _FandKeyP: {
			k = KeyDPtr(NextBranch);
			if (k == nullptr) {
				fd = FindFD(Vars[0]->SS);
				if (fd == nullptr) goto label1;
				k = fd->Keys;
				if (k == nullptr) goto label1;
			}
			Vars[1] = GetStringTerm(Pound(k->Alias));
			Vars[2] = GetBoolTerm(k->Intervaltest);
			Vars[3] = GetBoolTerm(k->Duplic);
			NextBranch = (TBranch*)(k->Chain);
			break;
		}
		case _FandLinkP: {
			ld = LinkDPtr(NextBranch);
			if (ld == nullptr) {
				ld = LinkDRoot;
				while ((ld != nullptr) && !SEquUpcase(ld->FromFD->Name, Vars[0]->SS)) {
					ld = ld->Chain;
				}
				if (ld == nullptr) goto label1;
			}
			fd = ld->FromFD;
			if (w == 3/*ii*/) {
				while ((ld != nullptr) && ((fd != ld->FromFD) ||
					!SEquUpcase(ld->RoleName, Vars[1]->SS))) ld = ld->Chain;
				if (ld = nullptr) goto label1;
				Vars[2] = GetStringTerm(ld->ToFD->Name);
			}
			else if (w == 5/*ioi*/) {
				while ((ld != nullptr) && ((fd != ld->FromFD) ||
					!SEquUpcase(ld->ToFD->Name, Vars[2]->SS))) ld = ld->Chain;
				if (ld == nullptr) goto label1;
				Vars[1] = GetStringTerm(ld->RoleName);
			}
			else {
				Vars[1] = GetStringTerm(ld->RoleName);
				Vars[2] = GetStringTerm(ld->ToFD->Name);
			}
			Vars[3] = GetStringTerm(Pound(ld->ToKey->Alias^));
			s[0] = 0;
			k = fd->Keys;
			while (k != nullptr) {
				if (k->IndexRoot == ld->IndexRoot) s = k->Alias*;
				k = k->Chain;
			}
			Vars[4] = GetStringTerm(Pound(s));
			n = ld->MemberRef;
			if (ld->IndexRoot != 0) n += 4;
			Vars[5] = GetIntTerm(n);
			if (w = 3) ld = nullptr;
			else
				do {
					ld = ld->Chain;
				} while (!((ld == nullptr) || (ld->FromFD == fd)));
				NextBranch = PBranch(ld);
				break;
		}
		case _FandKeyFieldP: {
			kf = KeyFldDPtr(NextBranch);
			if (kf = nullptr) {
				fd = FindFD(Vars[0]->SS);
				if (fd = nullptr) goto label1;
				k = fd->Keys;
				while ((k != nullptr) && !SEquUpcase(Pound(k->Alias^), Vars[1]->SS)) k = k->Chain;
				if (k = nullptr) goto label1; kf = k->KFlds;
			}
			Vars[2] = GetStringTerm(kf->FldD->Name);
			Vars[3] = GetBoolTerm(kf->CompLex);
			Vars[4] = GetBoolTerm(kf->Descend);
			NextBranch = PBranch(kf->Chain);
			break;
		}
		case _FandLinkFieldP: {
			kf = KeyFldDPtr(NextBranch);
			if (kf = nullptr) {
				ld = LinkDRoot;
				while ((ld != nullptr) && (
					!SEquUpcase(ld->FromFD->Name, Vars[0]->SS) ||
					!SEquUpcase(ld->RoleName, Vars[1]->SS))) ld = ld->Chain;
				if (ld == nullptr) goto label1;
				kf = ld->Args;
			}
			Vars[2] = GetStringTerm(kf->FldD->Name); NextBranch = PBranch(kf->Chain);
			break;
		}
		case _LenP: {
			t1 = Vars[0]; n = 0;
			while (t1 != nullptr) { n++; t1 = t1->Next; }
			Vars[1] = GetIntTerm(n);
			break;
		}
		case _InvP: {
			t1 = Vars[0]; t2 = nullptr;
			while (t1 != nullptr) {
				t2 = GetListTerm(t1->Elem, t2);
				t1 = t1->Next;
			}
			Vars[1] = t2;
			break;
		}
		case _AddP: {
			t1 = Vars[0]; t2 = Vars[1]; Vars[2] = t2;
			while (t2 != nullptr) {
				if (UnifyTermsCC(t1, t2->Elem)) goto label3;
				t2 = t2->Next;
			}
			Vars[2] = GetListTerm(t1, Vars[1]);
			break;
		}
		case _DelP: {
			q = CurrInst;
			t1 = PTerm(NextBranch);
			if (t1 == nullptr) t1 = Vars[1];
		label2:
			if (t1 == nullptr) goto label1;
			Vars[0] = t1->Elem;
			CurrInst = RetInst;
			b = UnifyTermsCV(t1->Elem, PTermList(ptr(_Sg, c->Arg))->Elem);
			CurrInst = q;
			if (b) {
				root = nullptr;
				t = Vars[1];
				while (t != t1) {
					t2 = GetListTerm(t->Elem, nullptr);
					if (root == nullptr) root = t2;
					else tprev->Next = t2;
					tprev = t2;
					t = t->Next;
				}
				t1 = t1->Next;
				if (root == nullptr) root = t1;
				else tprev->Next = t1;
				Vars[2] = root;
				NextBranch = PBranch(t1);
				goto label3;
			}
			Mem1.Release(q->StkMark); t1 = t1->Next; goto label2;
			break;
		}
		case _UnionP: {
			t1 = CopyCList(Vars[0]);
			root = nullptr;
			t2 = Vars[1];
			while (t2 != nullptr) {
				if (!FindInCList(t2->Elem, t1)) {
					t = GetListTerm(t2->Elem, nullptr);
					if (root == nullptr) root = t;
					else tprev->Next = t;
					tprev = t;
				}
				t2 = t2->next;
			}
			ChainList(t1, root);
			Vars[2] = t1;
			break;
		}
		case _MinusP: {
			root = nullptr; t1 = Vars[0]; t2 = Vars[1];
			while (t1 != nullptr) {
				if (!FindInCList(t1->Elem, t2)) {
					t = GetListTerm(t1->Elem, nullptr);
					if (root = nullptr) root = t;
					else tprev->Next = t;
					tprev = t;
				}
				t1 = t1->next;
			}
			Vars[2] = root;
			break;
		}
		case _InterP: {
			root = nullptr;
			t1 = Vars[0]; t2 = Vars[1];
			while (t1 != nullptr) {
				if (FindInCList(t1->Elem, t2)) {
					t = GetListTerm(t1->Elem, nullptr);
					if (root = nullptr) root = t;
					else tprev->Next = t;
					tprev = t;
				}
				t1 = t1->next;
			}
			Vars[2] = root;
			break;
		}
		case _AbbrevP: { Vars[1] = GetStringTerm(Abbrev(Vars[0]->SS)); break; }
		case _CallP: {
			if (!FindChpt('L', Vars[0]->SS, false, pos)) {
				SetMsgPar(Vars[0]->SS);
				RunError(1554);
			}
			RunProlog(pos, @Vars[1]->SS);
			if (EdBreak != 0) goto label1;
			break;
		}
		}
	label3:
		return true;
	}
}

void SyntxError(WORD N, integer Ex)
{
	RdMsg(3000 + N);
	EdRecKey = MsgLine;
	LastExitCode = Ex;
	GoExit();
}

void AppendLex(TTerm* tPrev, integer Pos, integer Typ, pstring s)
{
	TTerm* t = GetFunTerm(0, 3);
	t->Arg[2] = GetStringTerm(s);
	t->Arg[0] = GetIntTerm(Pos);
	t->Arg[1] = GetIntTerm(Typ);
	t = GetListTerm(t, nullptr);
	if (LexemList == nullptr) LexemList = t;
	else tPrev->Next = t;
	tPrev = t;
}

void LoadLex(LongStrPtr S)
{
	WORD l, i, n;
	char* p;
	integer typ; pstring x;
	TTerm* t; TTerm* tPrev;
	LexemList = nullptr;
	l = S->LL;
	p = (char*)(S->A);
label1:
	if (l == 0) { AppendLex(tPrev, S->LL, 0, ""); return; }
	if (*p <= ' ') { p++; l--; goto label1; }
	if (*p == '{') {
		n = 1;
	label11:
		p++; l--;
		if (n == 0) goto label1;
		if (l == 0) SyntxError(503, S->LL);
		if (*p == '{') n++;
		else if (*p == '}') n--;
		goto label11;
	}
	i = 0;
	if (IsLetter(*p)) {
		typ = 1;
	label2:
		i++;
		x[i] = *p;
		p++; l--;
		if ((l > 0) && (i < 255))
			if (IsLetter(*p)) goto label2;
			else if (isdigit(*p) || (*p == '_')) { typ = 2; goto label2; }
	}
	else if (isdigit(*p)) {
		typ = 3;
	label3:
		i++;
		x[i] = *p;
		p++; l--;
		if ((l > 0) && (i < 255) && isdigit(*p)) goto label3;
	}
	else { x[1] = *p; p++; l--; typ = 0; i = 1; }
	x[0] = char(i);
	AppendLex(tPrev, S->LL - l, typ, x);
	goto label1;
}

void SetCFile(const pstring Name)
{
	RdbDPtr r = CRdb;
	while (r != nullptr) {
		CFile = r->FD;
		while (CFile != nullptr) {
			if (SEquUpcase(Name, CFile->Name)) return;
			CFile = (FileD*)CFile->Chain;
		}
		r = r->ChainBack;
	}
	if (SEquUpcase(Name, "CATALOG")) CFile = CatFD;
	else RunError(1539);
}

void RetractDbEntry(TInstance* Q, WORD POfs/*PPredicate*/, TDbBranch* B)
{
	TDbBranch* b1 = nullptr; TPredicate* p = nullptr;
	p = ptr(_Sg, POfs);
	b1 = (TDbBranch*)(&p->Branch);
label1:
	if (b1->Chain != nullptr)
		if (b1->Chain = B) {
			b1->Chain = B->Chain;
			while ((Q != nullptr)) {
				if (Q->NextBranch == (void*)B) Q->NextBranch = (void*)B->Chain;
				Q = Q->PrevInst;
			}
		}
		else { b1 = b1->Chain; goto label1; }
	Mem3.Free(B, LenDbEntry((LongStr*)&B->LL, P->Arity) + 4);
}

void AppendPackedTerm(TCommand* C)
{
	char* pt = PackedTermPtr;
	TDbBranch* b = nullptr;
	char A[MaxPackedPredLen + 1]{ '\0' };
	WORD n = 0; // absolute A
	pt = &A[3];
	PTPMaxOfs = ofs(A) + MaxPackedPredLen - 2;
	PackTermV(C->apTerm);
	n = PtrRec(pt).Ofs - ofs(A);
	b = Mem3.Alloc(4 + n);
	Move(A, b->LL, n);
	ChainLast(CurrInst->Vars[C->apIdx], b);
}

void UnpackAppendedTerms(TCommand* C)
{
	TDbBranch* b = nullptr; TDbBranch* b1 = nullptr;
	TTerm* t = nullptr; TTerm* pt = nullptr;
	pt = CurrInst->Vars[C->apIdx];
	b = (TDbBranch*)(pt);
	pt = nullptr;
	while (b != nullptr) {
		PackedTermPtr = &b->A;
		t = UnpackTerm(c->apDom);
		ChainList(pt, t);
		b1 = b; b = b->Chain;
		Mem3.Free(b1, b1->LL + 4);
	}
}

bool RunCommand(WORD COff/*PCommand*/)
{
	integer i1 = 0, i2 = 0;
	double r1 = 0.0, r2 = 0.0; char res = '\0';
	WORD i = 0;
	TTerm* t = nullptr;
	TWriteD* w = nullptr;
	WORD wofs = 0; // absolute w
	TCommand* c = nullptr;
	WORD cofs = 0; // absolute c
	void* p1 = nullptr;
	longint n = 0; LongStr* s = nullptr;
	LockMode md;

	c = ptr(_Sg, COff);
	switch (c->Code) {
	case _WriteC: {
		w = ptr(_Sg, c->WrD);
		while (wofs != 0) {
			if (w->IsString) printf(w->SS);
			else PrintTerm(CurrInst->Vars[w->Idx], w->Dom);
			wofs = w->Chain;
		}
		if (c->NL) printf("\n");
		break;
	}
	case _CompC: {
		i = c->E1Idx;
		if (c->CompOp == _assign) CurrInst->Vars[i] = CopyTerm(c->E2);
		else {
			res = _equ;
			switch (c->Typ) {
			case _IntD: {
				i1 = CurrInst->Vars[i]->II;
				i2 = RunIExpr(c->E2);
				if (i1 < i2) res = _lt;
				else if (i1 > i2) res = _gt;
				break;
			}
			case _RealD: {
				r1 = CurrInst->Vars[i]->RR;
				r2 = RunRExpr(C->E2);
				if (r1 < r2) res = _lt;
				else if (r1 > r2) res = _gt;
				break;
			}
			default: {
				if (!UnifyTermsCV(CurrInst->Vars[i], c->E2)) res = _gt;
				break;
			}
			}
			if ((res & c->CompOp) == 0) { return false; }
		}
		break;
	}
	case _SaveC:
	case _ConsultC:
	case _LoadLexC: {
		MarkStore(p1);
		CFile = c->FD;
		if (CFile == nullptr) {
			SetCFile(c->Name);
			c->FD = CFile;
			PtrRec(c->FldD).Seg = PtrRec(CFile).Seg;
		}
		if (c->Code == _SaveC) {
			md = NewLMode(WrMode);
			if (!LinkLastRec(CFile, n, true)) IncNRecs(1);
			DelTFld(c->FldD);
			s = SaveDb(c->DbPred, 0);
			LongS_(c->FldD, s);
			WriteRec(CFile->NRecs);
		}
		else {
			md = NewLMode(RdMode);
			LinkLastRec(CFile, n, true);
			s = _LongS(c->FldD);
			if (c->Code == _ConsultC) ConsultDb(s, c->DbPred);
			else LoadLex(s);
		}
		OldLMode(md);
		ReleaseStore(p1);
		break;
	}
	case _ErrorC: {
		i1 = -1; i = 1; w = ptr(_Sg, c->WrD);
		while (wofs != 0) {
			if (w->IsString) { MsgPar[i] = w->SS; i++; }
			else {
				t = CurrInst->Vars[w->Idx];
				if ((TDomain*)(ptr(_Sg, w->Dom))->Typ = _IntD) i1 = t->II;
				else { MsgPar[i] = t->SS; i++; };
			}
			wofs = w->Chain;
		}
		if (i1 == -1) {
			i1 = 0;
			if (LexemList != nullptr) i1 = LexemList->Elem->Arg[0]->II;
		}
		SyntxError(c->MsgNr, i1);
		break;
	}
	case _WaitC: WaitC(); break;
	case _AppPkC: AppendPackedTerm(c); break;
	case _AppUnpkC: UnpackAppendedTerms(c); break;
	}
	return true;
}

void CallFandProc()
{
	TPredicate* p = nullptr;
	Instr* pd = nullptr;
	ProcStkD* oldBP = nullptr;
	ProcStkD* ps = nullptr;
	WORD i = 0, n = 0, w = 0;
	TTerm* t = nullptr;
	char* pp = (char*)ps;
	LongStrPtr s = nullptr;
	TDomain* d = nullptr;
	WORD dofs = 0; // absolute d
	void* pt = PackedTermPtr;
	pstring* ss = nullptr;

	p = ptr(_Sg, CurrInst->Pred);
	PtrRec(d).Seg = _Sg;
	pd = ptr(_Sg, WORD(p->Branch));
	ps = GetZStore(p->LocVarSz);
	w = p->InpMask;
	if (PtrRec(pd->Pos.R).Seg == 0) {
		PtrRec(pd->Pos.R).Seg = _Sg;
		if (pd->Pos.IRec = 0xffff)
			if (!FindChpt('P', (pstring*)(pd->Pos.R), false, pd->Pos)) RunError(1037);
	}
	for (i = 1; i <= p->Arity; i++) /* !!! with pd->TArg[i] do!!! */
	{
		PtrRec(Frml).Seg = _Sg;
		dofs = p->Arg[i - 1];
		t = CurrInst->Vars[i - 1];
		if ((w && 1) != 0) {
			switch (FTyp) {
			case 'R': if (t->Fun = _IntT) Frml->R = t->II; else Frml->R = t->RR; break;
			case 'B':Frml->B = bool(t->Fun); break;
			default: {
				if (Frml->Op = _const) Frml->S = t->SS;
				else {
					if (d->Typ == _LongStrD) s = RdLongStr(t->Pos);
					else s = GetPackedTerm(t);
					LongintPtr(pp + Frml->BPOfs) = TWork.Store(s);
					ReleaseStore(s);
				}
				break;
			}
			}
		}
		w = w >> 1;
	}
	ps->ChainBack = MyBP;
	oldBP = MyBP;
	SetMyBP(ps);
	CallProcedure(pd);
	w = p->InpMask;
	for (i = 1; i <= p->Arity; i++) {
		/* !!! with pd->TArg[i] do with CurrInst^ do  do!!! */
		fs = p->Arg[i - 1];
		if (Frml->Op = _getlocvar) {
			switch (FTyp) {
			case 'S': {
				if ((w & 1) == 0) {
					if (d->Typ = _StrD) Vars[i - 1] = GetStringTerm(RunShortStr(Frml));
					else {
						s = RunLongStr(Frml);
						if (d->Typ = _LongStrD) Vars[i - 1] = GetLongStrTerm(WrLongStr(s));
						else { PackedTermPtr = @s->A; Vars[i - 1] = UnpackTerm(dofs); }
						ReleaseStore(s);
					}
				}
				TWork.Delete(LongintPtr(Ptr(Seg(MyBP^), Ofs(MyBP^) + Frml->BPOfs))^);
				break;
			}
			case 'R': {
				if (d->Typ = _IntD) Vars[i - 1] = GetIntTerm(RunInt(Frml));
				else Vars[i - 1] = GetRealTerm(RunReal(Frml));
				break;
			}
			default: {
				Vars[i - 1] = GetBoolTerm(RunBool(Frml));
				break;
			}
			}
		}
		w = w >> 1;
	}
	SetMyBP(oldBP);
	ReleaseStore(ps);
}

TScanInf* SiCFile(WORD SiOfs)
{
	TScanInf* si = nullptr; TFldList* fl = nullptr;
	WORD flofs = 0; // absolute fl
	si = ptr(_sg, SiOfs);
	auto result = si;
	CFile = si->FD;
	if (CFile != nullptr) return;
	SetCFile(si->Name);
	si->FD = CFile;
	fl = ptr(_sg, si->FL);
	while (flofs != 0) {
		PtrRec(fl->FldD).Seg = PtrRec(CFile).Seg;
		flofs = fl->Chain;
	}
	return result;
}

void AssertFand(TPredicate* P, TCommand* C)
{
	TFldList* fl = nullptr;
	WORD flofs = 0; // absolute fl 
	TTermList* l = nullptr;
	WORD lofs = 0; // absolute l
	TDomain* d = nullptr;
	WORD dofs = 0; // absolute d
	FieldDescr* f = nullptr;
	LockMode md;
	TTerm* t = nullptr;
	TScanInf* si = nullptr;
	WORD i = 0;
	LongStr* s = nullptr;

	si = SiCFile(WORD(P->Branch));
	md = NewLMode(CrMode);
	CRecPtr = GetRecSpace();
	ZeroAllFlds();
	PtrRec(d).Seg = _Sg;
	fl = ptr(_Sg, si->FL);
	l = ptr(_Sg, c->Arg);
	i = 0;
	if (trace) printf("CALL assert(", PString(ptr(_Sg, P->Name))^, '(');
	while (flofs != 0) {
		f = fl->FldD;
		if ((f->Flg & f_Stored) != 0) {
			t = CopyTerm(l->Elem);
			dofs = p->Arg[i];
			if (trace) {
				if (i > 0) printf(",");
				PrintTerm(t, dofs);
			}
			switch (f->FrmlTyp) {
			case 'B': B_(f, bool(t->Fun)); break;
			case 'R': {
				if (t->Fun = _IntT) R_(f, t->II);
				else R_(f, t->RR);
				break;
			}
			default: {
				if (f->Typ = 'T') {
					if (d->Typ == _LongStrD) s = RdLongStr(t->Pos);
					else s = GetPackedTerm(t);
					LongS_(f, s);
					ReleaseStore(s);
				}
				else S_(f, t->SS);
				break;
			}
			}
			flofs = fl->Chain;
			lofs = l->Chain;
			i++;
		}
	}
#ifdef FandSQL
	if (trace) { writeln("))"); waitC; }
	if (CFile->IsSQLFile) Strm1->InsertRec(false, true); else
#endif
	{
		TestXFExist();
		IncNRecs(1);
		if (CFile->Typ == 'X') RecallRec(CFile->NRecs);
		else WriteRec(CFile->NRecs);
	}
	OldLMode(md);
	ReleaseStore(CRecPtr);
}

TFileScan* GetScan(WORD SIOfs, TCommand* C, TInstance* Q)
{
	TFldList* fl = nullptr;
	WORD flofs = 0; // absolute fl
	KeyD* k = nullptr;
	WORD kofs = 0; // absolute k
	KeyFldD* kf = nullptr; FieldDescr* f = nullptr;
	XString xx; WORD i = 0;
	TTerm* t = nullptr; double r = 0.0; LongStr* s = nullptr; pstring* ss = nullptr;
	LockMode md; longint n = 0; bool b = false;

	TScanInf* si = SiCFile(SIOfs);
	TFileScan* fs = Mem1.Get(sizeof(TFileScan));
	md = NewLMode(RdMode);
	k = nullptr;
	if (c->KDOfs != 0) k = ptr(PtrRec(CFile).Seg, c->KDOfs);
	if (kofs == 0) {
		fs->IRec = 1;
		fs->Count = CFile->NRecs;
		goto label1;
	}
	TestXFExist();
	xx.Clear();
	i = 0;
	kf = k->KFlds;
	while (kf != nullptr) {
		f = kf->FldD;
		t = Q->Vars[c->ArgI[i]];
		switch (f->FrmlTyp) {
		case 'R': {
			if (t->Fun == _IntT) r = t->II;
			else r = t->RR;
			xx.StoreReal(r, kf);
			break;
		}
		case 'B': xx.StoreBool(bool(t->Fun), kf); break;
		default: {
			if (t->Fun = _StrT) xx.StoreStr(t->SS, kf);
			else {
				if (t->Fun = _LongStrT) s = RdLongStr(t->Pos);
				else s = GetPackedTerm(t);
				ss = ptr(PtrRec(s).Seg, PtrRec(s).Ofs + 1);
				ss[0] = char(MinW(s->LL, 255));
				xx.StoreStr(ss, kf);
				ReleaseStore(s);
			}
			break;
		}
		}
		kf = (KeyFldD*)kf->Chain;
		i++;
	}
	k->FindNr(xx, fs->IRec);
	if ((f->Typ != 'A') || (xx.S[xx.S.length()] != 0x1f)) xx.S[0]++;
	xx.S[xx.S.length()] = 0xFF;
	b = k->FindNr(xx, n);
	fs->Count = 0;
	if (n >= fs->IRec) fs->Count = n - fs->IRec + b;
label1:
	OldLMode(md);
	if (fs->Count == 0) fs = nullptr;
	return fs;
}

pstring _MyS(FieldDescr* F)
{
	if (F->Typ == 'A') {
		if (F->M == LeftJust) return TrailChar(' ', _ShortS(F));
		else return LeadChar(' ', _ShortS(F));
	}
	return _ShortS(F);
}

bool ScanFile(TInstance* Q)
{
	TPredicate* p = nullptr;
	WORD pofs = 0; // absolute p
	TCommand* c = nullptr;
	WORD cofs = 0; // absolute c
	TFldList* fl = nullptr;
	WORD flofs = 0; // absolute fl
	FieldDescr* f = nullptr; TScanInf* si = nullptr;
	WORD w = 0; integer i = 0;
	TTerm* t = nullptr; double r = 0.0;
	LongStr* s = nullptr;
	pstring ss; bool b = false;
	TDomain* d = nullptr;
	WORD dofs = 0; // absolute d
	void* pt = PackedTermPtr;
	KeyDPtr k = nullptr;
	WORD kofs = 0; // absolute k
	LockMode md, md1;
	TFileScan* fs = nullptr; TFileScan* fs1 = nullptr;
	longint RecNr = 0; XString xx;

	auto result = false;
	fs = (TFileScan*)CurrInst->NextBranch;
	if (fs == nullptr) return;
	p = ptr(_Sg, CurrInst->Pred);
	c = ptr(_Sg, CurrInst->RetCmd);
	PtrRec(d).Seg = _Sg;
	si = ptr(_Sg, WORD(p->Branch));
	PtrRec(fl).Seg = _Sg;
	CFile = si->FD;
	CRecPtr = GetRecSpace;
	md = NewLMode(RdMode);
	k = nullptr;
	if (c->KDOfs != 0) k = ptr(PtrRec(CFile).Seg, c->KDOfs);
label1:
	if (k == nullptr) {
		do {
			RecNr = fs->IRec;
			if (RecNr > CFile->NRecs) {
				CurrInst->NextBranch = nullptr;
				goto label2;
			}
			ReadRec(RecNr);
			(fs->IRec)++;
		} while (DeletedFlag);
		if (fs->IRec > CFile->NRecs) CurrInst->NextBranch = nullptr;
	}
	else {
		if ((fs->Count == 0) || (fs->IRec > k->NRecs())) {
			CurrInst->NextBranch = nullptr;
			goto label2;
		}
		RecNr = k->NrToRecNr(fs->IRec);
		ReadRec(RecNr);
		fs->IRec++;
		fs->Count--;
		if ((fs->Count == 0) || (fs->IRec > k->NRecs())) CurrInst->NextBranch = nullptr;
	}
	flofs = si->FL;
	w = c->CompMask;
	for (i = 0; i <= integer(p->Arity) - 1; i++)
	{ /* compare with inp. parameters */
		if ((w & 1) != 0) {
			t = CurrInst->Vars[i];
			f = fl->FldD;
			switch (f->FrmlTyp) {
			case 'B': if (_B(f) != t->Fun) goto label1; break;
			case 'R': {
				r = _R(f);
				if (t->Fun = _IntT) { if (r != t->II) goto label1; }
				else if (r != t->RR) goto label1;
				break;
			}
			default: {
				if (f->Typ == 'T') {
					dofs = p->Arg[i];
					if (d->Typ = _LongStrD) s = RdLongStr(t->Pos);
					else s = GetPackedTerm(t);
					b = EquLongStr(s, _LongS(f));
					ReleaseStore(s);
					if (!b) goto label1;
					break;
				}
			}
			}
		}
		else if (t->SS != _MyS(f)) goto label1;
		flofs = fl->Chain;
		w = w >> 1;
	}
	flofs = si->FL;
	w = c->OutpMask;
	for (i = 0; i <= integer(p->Arity) - 1; i++)
	{ /* create outp. parameters */
		if ((w & 1) != 0) {
			f = fl->FldD;
			dofs = p->Arg[i];
			switch (f->FrmlTyp) {
			case 'B': CurrInst->Vars[i] = GetBoolTerm(_B(f)); break;
			case 'R': {
				if (d->Typ == _RealD) CurrInst->Vars[i] = GetRealTerm(_R(f));
				else CurrInst->Vars[i] = GetIntTerm(trunc(_R(f)));
				break;
			}
			default: {
				if (f->Typ == 'T') {
					s = _LongS(f);
					if (d->Typ = _LongStrD) CurrInst->Vars[i] = GetLongStrTerm(WrLongStr(s));
					else { pt = s->A; CurrInst->Vars[i] = UnpackTerm(dofs); }
					ReleaseStore(s);
				}
				else CurrInst->Vars[i] = GetStringTerm(_MyS(f));
				break;
			}
			}
			flofs = fl->Chain;
			w = w >> 1;
		}
		result = true;
		if (c->Code == _RetractC) {
			md1 = NewLMode(DelMode);
			while ((Q != nullptr)) {
				fs1 = (TFileScan*)Q->NextBranch;
				if ((Q->Pred == pofs) && (fs1 != nullptr))
					if (CFile->Typ == 'X') {
						cofs = Q->RetCmd;
						kofs = c->KDOfs;
						if (kofs != 0) {
							xx.PackKF(k->KFlds);
							k->RecNrToPath(xx, RecNr);
							if (k->PathToNr() <= fs1->IRec) fs1->IRec--;
						}
					}
					else if (RecNr <= fs1->IRec) fs1->IRec--;
				Q = Q->PrevInst;
			}
			if (CFile->Typ = 'X') { DeleteXRec(RecNr, true); fs->IRec--; }
			else DeleteRec(RecNr);
			OldLMode(md1);
		}
	label2:
		OldLMode(md);
		ReleaseStore(CRecPtr);
	}
}

void SaveLMode();
void SetOldLMode();

void TraceCall(TInstance* Q, BYTE X)
{
	WORD i, w; PPredicate p; PCommand c; PDomain d; WORD absolute d dofs;
	p = ptr(_Sg, Q->Pred); c = ptr(_Sg, Q->RetCmd); PtrRec(d).Seg = _Sg;

	if (X == 1) printf("CALL ");
	else
		if (c->Code == _AllC) printf("MEMBER ");
		else printf("RETURN ");
	switch (c->Code) {
	case _RetractC: printf("retract(");  break;
	case _NotC: printf("not("); break;
	case _AllC: printf("all_XX("); break;
	}
	printf(*(pstring*)(ptr(_Sg, p->Name)));
	if (p->Arity > 0) {
		printf("(");
		if ((p->Opt && _CioMaskOpt) != 0) w = c->InpMask;
		else w = p->InpMask;
		for (i = 0; i <= p->Arity - 1; i++) {
			if (i > 0) printf(",");
			dofs = p->Arg[i];
			if ((w & 1) == X) {
				if ((X = 1) && ((p->Opt && _PackInpOpt) != 0))
					PrintPackedTerm((char*)(Q->Vars[i]) + 2, dofs);
				else {
					if (p->Opt && _BuildInOpt != 0)
					{
						switch (p->LocVarSz) {
						case _MemP:
						case _AddP:
						case _DelP: {
							dofs = c->Elem;
							if (i == 0) dofs = d->ElemDom;
							break;
						}
						case _LenP: if (i == 0) dofs = c->Elem; break;
						case _InvP:
						case _UnionP:
						case _MinusP:
						case _InterP: dofs = c->Elem; break;
						}
					}
					PrintTerm(Q->Vars[i], dofs);
				}
			}
			else printf("_");
			w = w >> 1;
		}
		printf(")");
	}
	if (c->Code == _RetractC || c->Code == _NotC || c->Code == _AllC) printf(")");
	printf("\n");
	WaitC();
}

struct TAutoR
{
	BYTE i = 0; bool wasCall = false; TTerm* t = nullptr; TTerm* Arg[1];
};

bool AutoRecursion(TInstance* q, TPredicate* p, TCommand* c)
{
	integer i, j, i2, iOutp, sz, arity;
	TAutoR* w; TTerm* t; TTerm* t1; TFunDcl* f;
	TDomain* d;
	WORD dofs; // absolute d

	d = ptr(_Sg, p->Arg[0]);
	iOutp = c->iOutp;
	t1 = q->Vars[c->iWrk];

	if (d->Typ == _ListD)
		if (t1 == nullptr) {
			t = q->Vars[0];
			if (t == nullptr) {
				if (iOutp > 0) q->Vars[iOutp] = t;
				goto label2;
			}
			q->Vars[c->iWrk] = t; q->Vars[0] = t->Next;
			goto label1;
		}
		else {
			if (iOutp > 0) {
				t = q->Vars[iOutp];
				if (t != t1->Next) t1 = GetListTerm(t1->Elem, t);
				q->Vars[iOutp] = t1;
			}
			goto label3;
		}

	w = (TAutoR*)t1;
	if (w == nullptr) {
		t = q->Vars[0];
		f = GetFunDcl(dofs, t->Fun);
		w = Mem2.Get(sizeof(TAutoR) + 4 * f->Arity);
		w->t = t; i = 0;
		q->Vars[c->iWrk] = (TTerm*)w;
	}
	else {
		t = w->t;
		f = GetFunDcl(dofs, t->Fun);
		i = w->i;
		if (iOutp > 0) w->Arg[i] = q->Vars[iOutp];
		i++;
	}
	while (i < f->Arity)
		if (f->Arg[i] == dofs) {
			if (w->wasCall) for (j = 0; j <= c->nPairs - 1; j++)
				q->Vars[c->Pair[j].iInp] = q->Vars[c->Pair[j].iOutp];
			q->Vars[0] = t->Arg[i];
			w->i = i;
			w->wasCall = true;
		label1:
			return true;
		}
		else { if (iOutp > 0) w->Arg[i] = t->Arg[i]; i++; }
	if (iOutp > 0) {
		sz = 4 * f->Arity;
		if (!EquArea(@t->Arg, @w->Arg, sz)) {
			t1 = GetFunTerm(t->Fun, f->Arity);
			Move(w->Arg, t1->Arg, sz);
			q->Vars[iOutp] = t1;
		}
		else q->Vars[iOutp] = t;
	}
	if (!w->wasCall)
		label2:
	for (j = 0; j <= c->nPairs - 1; j++) {
		i = c->Pair[j].iInp;
		i2 = c->Pair[j].iOutp;
		if (i > 0) t = q->Vars[i];
		else {
			dofs = p->Arg[i2];
			switch (d->Typ) {
			case _ListD: t = nullptr; break;
			case _StrD: t = GetStringTerm(""); break;
			case _LongStrD: t = GetLongStrTerm(WrLongStrLP(0, nullptr)); break;
			case _IntD: t = GetIntTerm(0); break;
			case _RealD: t = GetRealTerm(0); break;
			}
		}
		q->Vars[i2] = t;
	}
label3:
	return false;
}

void RunProlog(RdbPos Pos, pstring* PredName)
{
	PInstance q, q1, TopInst; PDbBranch b1;
	WORD w, n; integer i; LongStrPtr s; Pchar absolute PackedTermPtr pt;
	char A[MaxPackedPredLen + 1]; WordPtr wp;
	void* pp, pp1, pp2, pp3, pm1, pm2; LongStrPtr ss; longint WMark;
	PPredicate p, p1; WORD absolute p pofs; PCommand c; WORD absolute c cofs;
	PBranch b; WORD absolute b bofs; PDbBranch absolute b bd;
	PTermList l; WORD absolute l lofs;  PDatabase db; WORD absolute db dbofs;
	PTerm t; PProgRoots Roots; RdbDPtr ChptLRdb;
	WORD oldSg; PInstance oldCurrInst; WORD tl, cl; ExitRecord er;

	inc(ProlgCallLevel); NewExit(Ovr, er); goto label7; LastExitCode = 1;
	oldSg = _Sg; oldCurrInst = CurrInst; ForallFDs(SaveLMode); WMark = MaxWSize;
	if (ProlgCallLevel = 1) {
		MarkBoth(pm1, pm2); FreeMemList = nullptr; Mem1.Init; Mem2.Init; Mem3.Init;
		TrcLevel = 0; CallLevel = 0
	}
	else {
		MarkStore(pp); pp1 = Mem1.Mark; pp2 = Mem2.Mark; pp3 = Mem3.Mark;
		tl = TrcLevel; cl = CallLevel;
	}
	if (Pos.IRec = 0) {
		SetInpLongStr(RunLongStr(FrmlPtr(Pos.R)), true); _Sg = ReadProlog(0);
		ChptLRdb = CRdb
	}
	else {
		ChptLRdb = Pos.R; CFile = ChptLRdb->FD; CRecPtr = GetRecSpace;
		ReadRec(Pos.IRec); AlignLongStr;
		_Sg = PtrRec(HeapPtr).Seg + 1; ss = _longs(ChptOldTxt);
		if (ChptLRdb->Encrypted) CodingLongStr(ss);
	}
	Roots = ptr(_Sg, 0);

	db = ptr(_Sg, Roots->Databases);
	while (dbofs != 0) {
		ConsultDb(ptr(_Sg, db->SOfs), dbofs);
		dbofs = db->Chain;
	}
	TopInst = nullptr; CurrInst = nullptr;
	c = ptr(_Sg, 0); b = ptr(_Sg, 0); l = ptr(_Sg, 0);
	p = ptr(_Sg, Roots->Predicates); /* main */
	if (PredName != nullptr) {
		while ((pofs != 0) && (StringPtr(ptr(_Sg, p->Name)) ^ != PredName^)) pofs = p->Chain;
		if ((pofs = 0) || (p->Arity != 0)) {
			Set2MsgPar(Pos.R->FD->Name, PredName^);
			RunError(1545);
		}
	}
label1:
	/*    new instance        remember prev. inst,branch,cmd */
	q = Mem2.Get(p->InstSz + (sizeof(TInstance) - 7 * 4));
	q->Pred = pofs;
	q->PrevInst = TopInst;
	TopInst = q;
	q->RetInst = CurrInst;
	q->RetBranch = bofs;
	q->RetCmd = cofs;
	if (TrcLevel != 0) {
		CallLevel = CurrInst->CallLevel + 1;
		q->CallLevel = CallLevel;
	}
	/*          copy input parameters  */
	b = p->Branch; i = 0;
	if ((p->Opt & _CioMaskOpt) != 0) w = c->InpMask;
	else w = p->InpMask;
	while (lofs != 0) {
		if ((w & 1) != 0) if ((p->Opt && _PackInpOpt) != 0) {
			pt = Pchar(@A);
			PTPMaxOfs = ofs(A) + MaxPackedPredLen - 2;
			PackTermV(l->Elem);
			n = PtrRec(pt).Ofs - ofs(A);
			s = Mem1.Get(2 + n);
			q->Vars[i] = PTerm(s);
			s->LL = n;
			Move(A, s->A, n);
		}
		else q->Vars[i] = CopyTerm(l->Elem);
		i++;
		lofs = l->Chain;
		w = w >> 1;
	}
	if ((p->Opt && (_FandCallOpt + _DbaseOpt)) == _FandCallOpt + _DbaseOpt)
		q->NextBranch = PBranch(GetScan(bofs, c, q));
	if (Trace) TraceCall(q, 1);
	q->StkMark = Mem1.Mark;
	q->WMark = MaxWSize;
	CurrInst = q;
	if ((p->Opt && (_FandCallOpt + _DbaseOpt)) == _FandCallOpt) {
		CallFandProc();
		goto label4;
	}
label2:
	/*        branch       / redo /        */
	if ((p->Opt && _BuildInOpt) != 0)             /* build-in predicates */
		if (RunBuildIn) goto label4;
		else goto label5;
	if ((p->Opt & _DbaseOpt) != 0) {         /* database predicates */
		if ((p->Opt & _FandCallOpt) != 0)
			if (ScanFile(TopInst)) goto label4;
			else goto label5;
		if (bd == nullptr) goto label5;
		cofs = q->RetCmd;
	label21:
		s = LongStrPtr(@bd->LL); w = c->InpMask;
		for (i = 0 to integer(p->Arity) - 1) {
			if (((w && 1) != 0) && !EquLongStr(LongStrPtr(q->Vars[i]), s)) {
				bd = bd->Chain; if (bd = nullptr) { q->NextBranch = nullptr; goto label5; }
				goto label21;
			}
			inc(PtrRec(s).Ofs, s->LL + 2); w = w shr 1;
		}
	label22:
		q->NextBranch = PBranch(bd->Chain);
		s = LongStrPtr(@bd->LL);
		w = c->OutpMask;
		for (i = 0 to integer(p->Arity) - 1)
		{ /* unpack db outp.parameters */
			if ((w && 1) != 0) { pt = Pchar(@s->A); q->Vars[i] = UnpackTerm(p->Arg[i]); }
			inc(PtrRec(s).Ofs, s->LL + 2);
			w = w >> 1;
		}
		if (c->Code = _RetractC) RetractDbEntry(TopInst, pofs, bd);
		goto label4;
	}
	PtrRec(b).Seg = _Sg;
label23:
	/* normal unify branch head * predicates/
WORD(q->NextBranch) = b->Chain;
i = 0; lofs = b->Head; w = b->HeadIMask; while (lofs != 0) {
  if (((w && 1) != 0) && !UnifyTermsCV(q->Vars[i],l->Elem)) {
	bofs = b->Chain; if (bofs=0) goto label5; goto label23;}
  inc(i); lofs = l->Chain; w = w shr 1 ;}
			  /*           execute all commands       */
	cofs = b->Cmd;
	while (cofs != 0) {
		switch (c->Code) {
			_PredC, _RetractC, _NotC: {
				pofs  24 = c->Pred; lofs = c->Arg; goto label1; }
		_AllC: { q->vars[c->Idx] = nullptr; goto label24; }
			_  CutC{ q->NextBranch = nullptr; while (TopInst != q) {
					  q1 = TopInst->PrevInst; Mem2.Release(TopInst); TopInst = q1
	  }; }
			_goto label5 FailC;
			_  Trace{ TrcLevel = c->TrcLevel;
					if (TrcLevel != 0) {
					  inc(TrcLevel); CallLevel = 1; q->CallLevel = 1
	  }
else { CallLevel = 0; q->CallLevel = 0 }; }
			_  AssertC{ p1 = ptr(_Sg,c->Pred);
					if (p1->Opt && _FandCallOpt != 0) AssertFand(p1,c) else {
					lofs = c->Arg; pt = Pchar(@A); PTPMaxOfs = ofs(A) + MaxPackedPredLen - 2;
					while (lofs != 0) {
					  wp = WordPtr(pt); inc(pt,2); PackTermV(l->Elem);
					  wp^ = PtrRec(pt).Ofs - PtrRec(wp).Ofs - 2; lofs = l->Chain;
	  }
n = PtrRec(pt).Ofs - Ofs(A); b1 = Mem3.Alloc(4 + n); move(A,b1->LL,n);
ChainLast(p1->Branch,b1);
if (Trace) PrintPackedPred(Pchar(@A),c->Pred)
}; }
			_25 AutoC : if (AutoRecursion(q, p, c)) { lofs = c->Arg; goto label1; }
			_  SelfC{
					if (TopInst != q) {
	   q1 = TopInst;
while (q1->PrevInst != q) q1 = q1->PrevInst;
Mem2.Release(q1); TopInst = q;
}
Mem1.Release(q->StkMark); MaxWSize = q->WMark; pofs = q->Pred;
b = p->Branch; goto label6; }
			else if (not RunCommand(cofs)) goto label5;
		}
		/*       resume command   */
	label3:
		cofs = c->Chain;
	}
	/*           copy output parameters */
	i = 0; lofs = b->Head; w = b->HeadOMask; while (lofs != 0) {
		if ((w && 1) != 0) q->Vars[i] = CopyTerm(l->Elem);
		inc(i); lofs = l->Chain; w = w shr 1;
	}
	/*       called predicate finished   */
label4:
	cofs = q->RetCmd;
	if (c->Code = _NotC) { TopInst = q->PrevInst; goto label5; }
label41:
	if (Trace)TraceCall(q, 0);
	/*      unify output with caller terms */
	b = ptr(_Sg, q->RetBranch);
	pofs = q->Pred;
	q1 = q;
	q = q->RetInst;
	if (q == nullptr) { EdBreak = 0; LastExitCode = 0; goto label8; }
	CurrInst = q;
	if ((p->Opt & _CioMaskOpt) != 0) w = c->OutpMask;
	else w = !p->InpMask;
	i = 0;
	lofs = c->Arg;
	while (lofs != 0) {
		if (((w && 1) = 1) && !UnifyTermsCV(q1->Vars[i], l->Elem)) goto label5;
		inc(i); lofs = l->Chain; w = w shr 1;
	}
	/*            return to caller;       */
	if (c->Code = _AllC) {
		ChainList(q->Vars[c->Idx], GetListTerm(CopyTerm(c->Elem), nullptr));
		q1 = TopInst; while (q1 != q) {
			q1->StkMark = Mem1.Mark; q1->WMark = MaxWSize; q1 = q1->PrevInst;
		}
		goto label5;
	}
	if ((q1->NextBranch = nullptr) && (q1 = TopInst)) {
		TopInst = q1->PrevInst; Mem2.Release(q1);
	}
	SetCallLevel(q->CallLevel);
	if (c->Code = _AutoC) goto label25;
	else goto label3;

	/*---------------------------------  backtracking  ---------------------------*/
label5:
	q1 = nullptr; q = TopInst;
	while (q != nullptr) && (q->NextBranch = nullptr) and
		!(PCommand(ptr(_Sg, q->RetCmd))->Code in[_NotC, _AllC]) do
	{
		q1 = q; q = q->PrevInst;
	}
	if (q = nullptr) {
		if (Trace) { writeln("FAIL"); waitC; }
		EdBreak = 1; LastExitCode = 0; goto label8;
	}
	Mem1.Release(q->StkMark); MaxWSize = q->WMark;
	if (q->NextBranch = nullptr) {
		q1 = q; q = q1->RetInst; b = ptr(_Sg, q1->RetBranch); cofs = q1->RetCmd;
		CurrInst = q; TopInst = q1->PrevInst; Mem2.Release(q1);
		if (c->Code = _NotC) {
			if (Trace) { writeln("FAIL" ^ m ^ j"RETURN not()"); waitC }
		}
		else {
			q->Vars[c->Idx2] = q->Vars[c->Idx]; q->Vars[c->Idx] = nullptr;
			if (Trace) { writeln("RETURN all_()"); waitC };
		}
		SetCallLevel(q->CallLevel); pofs = q->Pred;
		goto label3;
	}
	if (Trace) { writeln("FAIL"); waitC; }
	TopInst = q; CurrInst = TopInst; b = q->NextBranch; pofs = q->Pred;
	SetCallLevel(q->CallLevel);
	if (q1 != nullptr) Mem2.Release(q1);
label6:
	if (Trace) { writeln("REDO ", PString(ptr(_Sg, p->Name))^); waitC; }
	goto label2;

	/*--------------------------  } of program  ------------------------------*/
label7:
	EdBreak = 2;
label8:
	RestoreExit(er);
	/*writeln(AbsAdr(HeapPtr)-AbsAdr(pm1),'/',AbsAdr(pm2)-AbsAdr(Stack2Ptr)); */
	_Sg = oldSg; CurrInst = oldCurrInst; ForallFDs(SetOldLMode); MaxWSize = WMark;
	if (ProlgCallLevel = 1) ReleaseBoth(pm1, pm2) else {
		ReleaseStore(pp); Mem1.Release(pp1); Mem2.Release(pp2); Mem3.Release(pp3);
		TrcLevel = tl; CallLevel = cl;
	}
	dec(ProlgCallLevel);
}
