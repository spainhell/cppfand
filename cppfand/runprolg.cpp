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

bool Trace();
void SetCallLevel(WORD Lv);
void WaitC();

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
	return nullptr;
}

void* TMemory::Mark()
{
	return nullptr;
}

void TMemory::Release(void* P)
{
}

pstring TMemory::StoreStr(pstring s)
{
	return pstring();
}

void* TMemory::Alloc(WORD Sz)
{
	return nullptr;
}

void TMemory::Free(void* P, WORD Sz)
{
}

/*  T D O M A I N  =========================================================*/
TFunDcl* GetFunDcl(WORD D, BYTE I);

/*  T T E R M  =============================================================*/
TTerm* GetIntTerm(integer I);
TTerm* GetRealTerm(double R);
TTerm* GetBoolTerm(bool B);

TTerm* GetStringTerm(pstring S);
TTerm* GetLongStrTerm(longint N);
TTerm* GetListTerm(TTerm* aElem, TTerm* aNext);
TTerm* GetFunTerm(BYTE aFun, BYTE aArity);
void ChainList(void* Frst, void* New);

pstring XXS;
LongStr* RdLongStr(longint Pos);
longint WrLongStrLP(WORD L, void* P);
longint WrLongStr(LongStr* S);
LongStr* RunLSExpr(WORD TOfs);
void RunSExpr(WORD TOfs, pstring* s);
double RunRExpr(WORD TOfs);
integer RunIExpr1(TPTerm* T);
double RunRExpr(WORD TOfs/*PPTerm*/);
void RunSExpr1(TPTerm* T, pstring* s);
void RunSExpr(WORD TOfs, pstring* s);
LongStr* RunLSExpr(WORD TOfs);
bool UnifyTermsCC(TTerm* T1, TTerm* T2);
bool UnifyTermsCV(TTerm* T1, WORD T2Ofs/*PPTerm*/);
bool UnifyVList(TTerm* TT1, TPTerm* T2);
bool FindInCList(TTerm* tEl, TTerm* t);
TTerm* CopyCList(TTerm* T);
TTerm* CopyTerm(WORD TOff/*PPTerm*/);
TTerm* CopyVList(TPTerm* T, bool Cpy);
void PackTermC(TTerm* T);
void PackTermV(WORD TOff/*PPTerm*/);
WORD PackVList(TPTerm* T);
void PackTermV(WORD TOff/*PPTerm*/);
LongStr* GetPackedTerm(TTerm* T);
TTerm* UnpackTerm(WORD D);
char* PrintPackedTerm(char* P, WORD D);
void PrintPackedPred(char* Q, WORD POfs/*PPredicate*/);
void PrintTerm(TTerm* T, WORD DOfs);
WORD LenDbEntry(LongStr* S, integer Arity);
LongStr* SaveDb(WORD DbOfs,/*PDatabase*/ longint AA);
void ConsultDb(LongStr* S, WORD DbOfs/*PDatabase*/);
bool Vokal(char C);
bool IsUpper(char C);
pstring Abbrev(pstring S);
FileD* NextFD(FileD* FD);
FileD* FindFD(pstring FDName);
pstring Pound(pstring s);
bool RunBuildIn();
void SyntxError(WORD N, integer Ex);
void AppendLex(TTerm* tPrev, integer Pos, integer Typ, pstring s);
void LoadLex(LongStrPtr S);
void SetCFile(const pstring Name);
void RetractDbEntry(TInstance* Q, WORD POfs/*PPredicate*/, TDbBranch* B);
void AppendPackedTerm(TCommand* C);
void UnpackAppendedTerms(TCommand* C);
bool RunCommand(WORD COff/*PCommand*/);
void CallFandProc();
TScanInf* SiCFile(WORD SiOfs);
void AssertFand(TPredicate* P, TCommand* C);
TFileScan* GetScan(WORD SIOfs, TCommand* C, TInstance* Q);
pstring _MyS(FieldDPtr F);
bool ScanFile(TInstance* Q);
void SaveLMode();
void SetOldLMode();
void TraceCall(TInstance* Q, BYTE X);
bool AutoRecursion(TInstance* q, TPredicate* p, TCommand* c);
void RunProlog(RdbPos Pos, pstring* PredName);
