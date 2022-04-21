#include "RdProlog.h"

#include "Prolog.h"
#include "TMemory.h"
#include "../cppfand/base.h"
#include "../cppfand/Chained.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/FileD.h"
#include "../cppfand/pstring.h"
#include "../Indexes/XKey.h"
#include "../cppfand/compile.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/obaseww.h"
#include "../cppfand/runproc.h"
#include "../cppfand/runproj.h"
#include "../cppfand/models/Instr.h"
#include "../Editor/OldEditor.h"

//template <class T>
//void ChainLst(Chained<T>* Root, Chained<T>* NewOfs) // assembler
//{
//	// ES:DI = Root;
//	// ...
//}

//void* OOfs(void* p)
//{
//	//return ((PtrRec(p).Seg - _Sg) << 4) + PtrRec(p).Ofs;
//	return 0;
//}

//WORD GetZStor(WORD Sz)
//{
//	void* p = GetZStore(Sz);
//	return OOfs(p);
//}

//void* OPtr(WORD Sg, void* p)
//{
//	return ptr(Sg, ((PtrRec(p).Seg - Sg) << 4) + PtrRec(p).Ofs);
//}

std::vector<std::string> vPredicates;

std::string StorStr(std::string S) // assembler
{
	vPredicates.push_back(S);
	return S;
}

/*  L E X A N A L  =========================================================*/
bool IsCharUpper2(unsigned char C)
{
	return UpcCharTab[C] == C;
}

bool IsUpperIdentif()
{
	return (Lexem == _identifier) && IsCharUpper(LexWord[1]);
}

void RdLexP()
{
	WORD i = 0;
	OldErrPos = CurrPos;
	SkipBlank(false);
	ReadChar();
	Lexem = CurrChar;
	switch (CurrChar) {
	case '\'': {
		Lexem = _quotedstr;
		ReadChar();
		LexWord = "";
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
			LexWord[0] = (char)i;
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

void TestIdentifP()
{
	if (Lexem != _identifier) Error(29);
}

void AcceptP(char X)
{
	if (Lexem != X)
		if (X == _assign) Error(506);
		else {
			ExpChar = X;
			Error(1);
		}
	RdLexP();
}

bool TestKeyWordP(pstring s)
{
	return (Lexem == _identifier) && (LexWord == s);
}


bool IsKeyWordP(pstring s)
{
	if (TestKeyWordP(s)) { RdLexP(); return true; }
	return false;
}

void AcceptPKeyWord(pstring s)
{
	if (!IsKeyWordP(s)) { SetMsgPar(s); Error(33); }
}

integer RdIntegerP()
{
	integer i = 0, j = 0;
	if (Lexem != _number) Error(525);
	val(LexWord, i, j);
	RdLexP();
	return i;
}

/*  T D O M A I N  =========================================================*/
TFunDcl* GetFunDclByName(TDomain* D, BYTE& I)
{
	TDomain* fd = (TDomain*)D->FunDcl;
	I = 0;

	std::string sLexWord = LexWord;

	while (fd != nullptr && fd->Name != sLexWord)
	{
		fd = fd->pChain;
		I++;
	}
	if (fd == nullptr) return nullptr;
	else return (TFunDcl*)fd;
}

TDomain* GetOrigDomain(TDomain* D)
{
	if (D != nullptr)
		while (D->Typ == TDomainTyp::_RedefD) {
			D = D->OrigDom;
		}
	return D;
}

/*  T D A T A B A S E  =====================================================*/
TDatabase* FindDataBase(std::string S)
{
	TDatabase* db = nullptr;
	TDatabase* next = nullptr;

	if (Roots->Databases == nullptr) return nullptr;

	db = Roots->Databases;
	while (db->pChain != nullptr) {
		if (db->Name == S) goto label1;
		next = db->pChain;
	}
label1:
	return db->pChain;
}

/*  T P R O G R A M  =======================================================*/
TPTerm* FindConst(TDomain* D)
{
	TConst* p = Roots->Consts;
	TConst* next = (TConst*)p->pChain;
	while (next != nullptr) {
		if ((p->Dom == D) && (p->Name == LexWord)) { return p->Expr; }
		next = (TConst*)p->pChain;
	}
	return nullptr;
}

bool RdConst(TDomain* D, TPTerm* RT)
{
	TPTerm* tofs = nullptr;
	if (Lexem == _identifier) {
		tofs = FindConst(D);
		if (tofs != 0) {
			RdLexP();
			RT = tofs;
			return true;
		}
	}
	return false;
}

TPTerm* GetOp1(TDomain* D, char Op, TPTerm* E1); /*PPTerm*/ //forward;

TVarDcl* FindVarDcl()
{
	TVarDcl* v = VarDcls;
	while (v != nullptr) {
		if (v->Name == LexWord) goto label1;
		v = v->pChain;
	}
label1:
	return v;
}

TVarDcl* MakeVarDcl(TDomain* D, integer Idx)
{
	TVarDcl* v = (TVarDcl*)Mem1.Get(sizeof(TVarDcl) + LexWord.length() - 1);
	ChainLast<TVarDcl>(VarDcls, v);
	v->Dom = D;
	//Move(LexWord, v->Name, LexWord.length() + 1);
	v->Name = LexWord;
	if (Idx < 0) { v->Idx = VarCount; VarCount++; }
	else v->Idx = Idx;
	return v;
}

bool RdVar(TDomain* D, integer Kind, integer Idx, TPTerm* RT) /*PTerm || idx*/
{
	TVarDcl* v = nullptr;
	bool bnd; /* idx=-1 except solo variable in head */
	TPTerm* t = nullptr;
	//WORD tofs; // absolute t
	WORD t1Ofs;
	if (IsKeyWordP('_')) {
		if (!(Kind >= 1 && Kind <= 4)) OldError(508);
		UnbdVarsInTerm = true;
		WasUnbd = true;
		RT = UnderscoreTerm;
		return true;
	}
	RT = nullptr;
	if (!IsUpperIdentif() || (Kind == 6/*const dcl*/)) {
		if (Kind == 5) Error(523);
		return false;
	}
	v = FindVarDcl();
	if (v == nullptr) v = MakeVarDcl(D, Idx);
	else if ((v->Dom != D) &&
		!((v->Dom == StrDom) && (D == LongStrDom)) &&
		!((v->Dom == LongStrDom) && (D == StrDom)) &&
		!((v->Dom == IntDom) && (D == RealDom)) &&
		!((v->Dom == RealDom) && (D == IntDom))) {
		RdLexP();
	label1:
		SetMsgPar(v->Dom->Name, D->Name);
		OldError(507);
	}
	RdLexP();
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
		//RT = v->Idx;
		return true;
	}
	}
	if ((Idx == -1) || (v->Idx != Idx)) {
		//PtrRec(t).Seg = _Sg;
		//t->pChain = GetZStor(5);
		//t->Fun = _VarT; t->Idx = v->Idx; t->Bound = bnd;
		//if ((v->Dom != DOfs)) {
		//	if (!bnd) goto label1;
		//	tofs = GetOp1(DOfs, _conv, tofs);
		//}
		//RT = tofs;
	}
	return true;
}

TPTerm* RdTerm(TDomain* D, integer Kind); /*PPTerm*/ // forward;
TPTerm* RdAddExpr(TDomain* D, integer Kind); /*PPTerm*/ // forward;

WORD DomFun(TDomain* D)
{
	if (D == IntDom) return _IntT;
	else if (D == RealDom) return _RealT;
	else if (D == StrDom) return _StrT;
	else return _LongStrT;
}

WORD GetOpl(TDomain* D, char Op, TPTerm* E1)
{
	TPTerm* t = new TPTerm(); // ptr(_Sg, GetZStor(1 + 1 + 2));
	WORD tofs = 0; // absolute t
	t->Fun = DomFun(D);
	t->Op = Op;
	WasOp = true;
	t->E1 = E1;
	return tofs;
}

TPTerm* GetOp2(TDomain* D, char Op, TPTerm* E1, TPTerm* E2)
{
	TPTerm* t = new TPTerm(); // ptr(_Sg, GetZStor(1 + 1 + 2 * 2));
	TPTerm* tofs = nullptr; // absolute t
	t->Fun = DomFun(D);
	t->Op = Op;
	WasOp = true;
	t->E1 = E1;
	t->E2 = E2;
	return tofs;
}

TPTerm* GetFunOp(TDomain* D, TDomain* ResD, char Op, pstring ArgTyp, integer Kind)
{
	TPTerm* t = nullptr;
	WORD tofs = 0; // absolute t 
	WORD i = 0, l = 0;
	TPTerm* t1ofs = nullptr;
	if (D != ResD) OldError(510);
	l = ArgTyp.length();
	if (l > 0) AcceptP('(');
	t = new TPTerm(); // ptr(_Sg, GetZStor(1 + 1 + 2 * l));
	t->Fun = DomFun(D);
	t->Op = Op; WasOp = true;
	for (i = 1; i <= l; i++) {
		if (i > 1) AcceptP(',');
		switch (ArgTyp[i]) {
		case 'l': t1ofs = RdAddExpr(LongStrDom, Kind); break;
		case 's': t1ofs = RdAddExpr(StrDom, Kind); break;
		case 'i': t1ofs = RdAddExpr(IntDom, Kind); break;
		case 'c': {
			if ((Lexem != _quotedstr) || (LexWord.length() != 1)) Error(560);
			//t1ofs = LexWord[1];
			RdLexP();
			break;
		}
		}
		//t->E[i] = t1ofs;
	}
	if (l > 0) AcceptP(')');
	return (TPTerm*)tofs;
}

TPTerm* RdPrimExpr(TDomain* D, integer Kind)
{
	TPTerm* t = nullptr;
	TPTerm* tofs = nullptr; // absolute t tofs; 
	char op = '\0';
	bool minus = false; double r = 0.0;
	pstring s; integer i = 0; longint n = 0;

	// PtrRec(t).Seg = _Sg;
	switch (Lexem) {
	case '^': {
		if (D != IntDom) Error(510);
		op = Lexem;
		RdLexP();
		//tofs = GetOp1(D, op, RdPrimExpr(D, Kind));
		break;
	}
	case '(': {
		RdLexP();
		tofs = RdAddExpr(D, Kind);
		AcceptP(')');
		break;
	}
	case _quotedstr: {
		if ((D != StrDom) && (D != LongStrDom)) Error(510);
		//tofs = GetZStor(1 + 1 + 1 + LexWord.length());
		t->Fun = DomFun(D);
		t->Op = _const;
		t->SS = LexWord;
		RdLexP();
		break;
	}
	case '$': {
		if (D != IntDom) Error(510);
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
			if (i == 0) Error(504);
			s[0] = char(i);
			//n = HexStrToLong(s);
			RdLexP();
			goto label2;
			break;
		}
	}
	case '-': {
		RdLexP();
		if (Lexem != _number) Error(525);
		minus = true;
		goto label1;
		break;
	}
	case _number: {
		minus = false;
	label1:
		s = LexWord;
		RdLexP();
		if (D == IntDom) {
			//val(s, n, i);
			if (minus) n = -n;
		label2:
			//tofs = GetZStor(1 + 1 + sizeof(integer));
			t->Fun = _IntT;
			t->II = n;
		}
		else if (D == RealDom) {
			if ((Lexem == '.') && isdigit(ForwChar)) {
				RdLexP();
				s.Append('.');
				s += LexWord;
				RdLexP();
			}
			val(s, r, i);
			if (minus) r = -r;
			//tofs = GetZStor(1 + 1 + sizeof(double));
			t->Fun = _RealT;
			t->RR = r;
		}
		else OldError(510);
		t->Op = _const;
		break;
	}
	default: {
		if (!RdVar(D, Kind, -1, nullptr /*tofs*/) && !RdConst(D, tofs))
			if (IsKeyWordP("length")) tofs = GetFunOp(D, IntDom, _length, 's', Kind);
			else if (IsKeyWordP("pos")) tofs = GetFunOp(D, IntDom, _pos, "sl", Kind);
			else if (IsKeyWordP("min")) tofs = GetFunOp(D, IntDom, _min, "ii", Kind);
			else if (IsKeyWordP("max")) tofs = GetFunOp(D, IntDom, _max, "ii", Kind);
			else if (IsKeyWordP("val")) tofs = GetFunOp(D, IntDom, _val, 's', Kind);
			else if (IsKeyWordP("copy")) tofs = GetFunOp(D, StrDom, _copy, "sii", Kind);
			else if (IsKeyWordP("str")) tofs = GetFunOp(D, StrDom, _str, 'i', Kind);
			else if (IsKeyWordP("repeatstr")) tofs = GetFunOp(D, StrDom, _repeatstr, "si", Kind);
			else if (IsKeyWordP("leadchar")) tofs = GetFunOp(D, StrDom, _leadchar, "cs", Kind);
			else if (IsKeyWordP("trailchar")) tofs = GetFunOp(D, StrDom, _trailchar, "cs", Kind);
			else if (IsKeyWordP("maxrow")) tofs = GetFunOp(D, IntDom, _maxrow, "", Kind);
			else if (IsKeyWordP("maxcol")) tofs = GetFunOp(D, IntDom, _maxcol, "", Kind);
			else Error(511);
	}
	}
	return (TPTerm*)&tofs;
}

TPTerm* RdMultExpr(TDomain* D, integer Kind)
{
	TPTerm* tofs = nullptr;
	char op = '\0';
	//tofs = RdPrimExpr(D, Kind);
	while ((D != StrDom) && (D != LongStrDom) && ((Lexem == '*' || Lexem == '/') ||
		(Lexem == _and || Lexem == _or) && (D == IntDom))) {
		op = Lexem;
		RdLexP();
		tofs = GetOp2(D, op, nullptr/*tofs*/, RdPrimExpr(D, Kind));
	}
	return tofs;
}

TPTerm* RdAddExpr(TDomain* D, integer Kind)
{
	TPTerm* tofs = 0; char op = '\0';
	//tofs = RdMultExpr(D, Kind);
	while ((Lexem == '+') || (Lexem == '-') && ((D == IntDom) || (D == RealDom))) {
		op = Lexem;
		RdLexP();
		tofs = GetOp2(D, op, nullptr /*tofs*/, RdMultExpr(D, Kind));
	}
	return tofs;
}

TPTerm* RdListTerm(TDomain* D, integer Kind)
{
	TPTerm* t = nullptr;
	TPTerm* tofs = nullptr; // absolute t
	TPTerm* t1 = nullptr;
	TPTerm* tPrev = nullptr;
	//PtrRec(t).Seg = _Sg;
	if (!RdVar(D, Kind, -1, tofs) && !RdConst(D, tofs)) {
		if (Lexem != '[') Error(510);
		RdLexP();
		tofs = nullptr;
		if (Lexem == ']') RdLexP();
		else {
		label1:
			t1 = new TPTerm(); // GetZStor(1 + 1 + 2 * 2);
			/* !!! with PPTerm(ptr(_Sg,t1))^ do!!! */
			{
				t1->Fun = _ListT; t1->Op = _const;
				t1->Elem = RdTerm(D->ElemDom, Kind);
			}
			if (tofs == nullptr) tofs = t1;
			else ((TPTerm*)tPrev)->Next = (TPTerm*)t1;
			tPrev = t1;
			if (Lexem == ',') { RdLexP(); goto label1; }
			if (Lexem == '|') {
				RdLexP();
				if (!RdVar(D, Kind, -1, ((TPTerm*)tPrev)->Next)) Error(511);
			}
			AcceptP(']');
		}
	}
	if (Lexem == '+') {
		t1 = tofs;
		//tofs = GetZStor(1 + 1 + 2 * 2);
		t->Op = '+';
		RdLexP();
		t->Fun = _ListT;
		t->E1 = t1;
		t->E2 = RdListTerm(D, Kind);
		WasOp = true;
	}
	return tofs;
}

TPTerm* RdTerm(TDomain* D, integer Kind)
{
	TPTerm* t = nullptr;
	TPTerm* tofs = nullptr; // absolute t
	TFunDcl* f = nullptr;
	WORD i = 0, n = 0;
	BYTE idx = 0;
	bool wo = false, wu = false;
	wo = WasOp; wu = WasUnbd;
	WasOp = false; WasUnbd = false;
	switch (D->Typ) {
	case _IntD:
	case _RealD:
	case _StrD:
	case _LongStrD: {
		tofs = RdAddExpr(D, Kind);
		break;
	}
	case _ListD: tofs = RdListTerm(D, Kind); break;
	default: {
		if (!RdVar(D, Kind, -1, tofs) && !RdConst(D, tofs)) {
			TestIdentifP();
			f = GetFunDclByName(D, idx);
			if (f == nullptr) Error(512);
			RdLexP();
			n = f->Arity;
			//t = ptr(_Sg, GetZStor(1 + 1 + 2 * n));
			t->Fun = idx;
			t->Arity = n;
			if (n > 0) {
				AcceptP('(');
				for (i = 0; i <= n - 1; i++) {
					if (i > 0) AcceptP(',');
					t->Arg[i] = RdTerm((TDomain*)f->Arg[i], Kind);
				}
				AcceptP(')');
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

TDomain* MakeDomain(TDomainTyp DTyp, std::string Nm)
{
	TDomain* d = new TDomain();
	if (Roots->Domains == nullptr) Roots->Domains = d;
	else ChainLast(Roots->Domains, d);
	d->Typ = DTyp;
	d->Name = Nm;
	return d;
}

TDomain* GetDomain(bool Create, std::string Nm)
{
	TDomain* d = Roots->Domains;

	TDomain* d1 = nullptr;
	TDomain* d2 = nullptr;

	while ((d != nullptr) && (d->Name != Nm)) {
		d = d->pChain;
	}

	if (d == nullptr) {
		if (Nm.substr(0, 2) == "L_") {
			d1 = GetOrigDomain(GetDomain(Create, Nm.substr(2)));
			if (d1 == nullptr) Error(517);
			Nm = "L_";
			Nm += d1->Name;
			d = Roots->Domains;
			while ((d != nullptr) && (d->Name != Nm)) {
				d = d->pChain;
			}
			if (d == nullptr) {
				d = MakeDomain(_ListD, Nm);
				d->ElemDom = d1;
			}
		}
		else if (Create) {
			if ((Nm.length() == 0) || !IsCharUpper2(Nm[1])) Error(514);
			d = MakeDomain(_UndefD, Nm);
		}
	}
	return d;
}

TDomain* RdDomain() /*PDomain*/
{
	TDomain* D = GetDomain(false, LexWord);
	TestIdentifP();
	if (D == nullptr) Error(517);
	RdLexP();
	return GetOrigDomain(D);
}

void RdDomains()
{
	TDomain* d = nullptr;
	TDomain* dofs = nullptr; // absolute d
	TFunDcl* fd = nullptr;
	TDomain* fdofs = nullptr; // absolute fd
	TDomain* d1 = nullptr;
	TDomain* a[32]{ nullptr };
	std::string nm;
	TDomain* dofs2 = nullptr;
	BYTE n = 0;
	integer i = 0;
	//PtrRec(d).Seg = _Sg;
	//PtrRec(fd).Seg = _Sg;
label1:
	TestIdentifP();
	dofs = GetDomain(true, LexWord);
	if (d->Typ != _UndefD) Error(505);
	RdLexP();
	while (Lexem == ',') {
		RdLexP();
		d->Typ = _RedefD;
		TestIdentifP();
		d->OrigDom = GetDomain(true, LexWord);
		dofs = d->OrigDom;
		if (d->Typ != _UndefD) OldError(505);
		RdLexP();
	}
	AcceptP('=');
	SkipBlank(false);
	TestIdentifP();
	if (IsCharUpper2(LexWord[1])) {
		d1 = GetDomain(true, LexWord);
		if (d1 == dofs) Error(505);
		RdLexP();
		d->Typ = _RedefD;
		d->OrigDom = d1;
		goto label4;
	}
	d->Typ = _FunD;
label2:
	if (GetFunDclByName(dofs, n) != nullptr) Error(505);
	if (IsCharUpper2(LexWord[1])) Error(515);
	nm = StorStr(LexWord);
	RdLexP();
	n = 0;
	if (Lexem == '(') {
		RdLexP();
	label3:
		TestIdentifP();
		a[n] = GetDomain(true, LexWord);
		n++;
		RdLexP();
		if (Lexem == ',') { RdLexP(); goto label3; }
		AcceptP(')');
	}
	fdofs = new TDomain(); // GetZStor(sizeof(TFunDcl) - 3 * 2 + n * 2);
	ChainLast((Chained<TDomain>*)d->FunDcl, fdofs);
	/* !!! with fd^ do!!! */ { fd->Name = nm; fd->Arity = n; Move(a, fd->Arg, 2 * n); }
	if (Lexem == ';') { RdLexP(); TestIdentifP(); goto label2; }
label4:
	if (!(Lexem == 0x1A || Lexem == '#')) goto label1;
	dofs = Roots->Domains;
	while (dofs != 0) {
		switch (d->Typ) {
		case _UndefD: { SetMsgPar(d->Name); OldError(516); break; }
		case _FunD: {
			fdofs = (TDomain*)d->FunDcl;
			while (fdofs != 0) {
				for (i = 1; i <= fd->Arity; i++) fd->Arg[i - 1] = GetOrigDomain((TDomain*)fd->Arg[i - 1]);
				fdofs = (TDomain*)fd->pChain;
			}
			break;
		}
		}
		dofs = (TDomain*)d->pChain;
	}
}

void RdConstants()
{
	TConst* p = nullptr;
	//pofs = 0; // absolute p
	TDomain* dofs = nullptr;
label1:
	dofs = RdDomain();
	AcceptP(':');
label2:
	TestIdentifP();
	if (IsCharUpper2(LexWord[1])) Error(515);
	if (FindConst(dofs) != 0) Error(505);
	p = new TConst(); // ptr(_Sg, GetZStor(sizeof(TConst) - 1 + LexWord.length()));
	p->pChain = new TConst();
	//Move(LexWord, p->Name, LexWord.length() + 1);
	p->Name = LexWord;
	RdLexP();
	p->Dom = dofs;
	AcceptP('=');
	p->Expr = RdTerm(dofs, 6);
	ChainLast(Roots->Consts, p->pChain);
	if (Lexem == ',') { RdLexP(); goto label2; }
	if (!(Lexem == 0x1A || Lexem == '#')) goto label1;
}

TPredicate* GetPredicate() /*PPredicate*/
{
	std::string sLexWord = LexWord;

	TPredicate* p = Roots->Predicates;
	while (p != nullptr) {
		if (sLexWord == p->Name) {
			RdLexP();
			return p;
		}
		p = p->pChain;
	}

	p = ClausePreds;
	while (p != nullptr) {
		if (sLexWord == p->Name) {
			RdLexP();
			return p;
		}
		p = p->pChain;
	}

	return p;
}

TPredicate* RdPredicate() /*PPredicate*/
{
	TPredicate* pofs = GetPredicate();
	if (pofs == nullptr) Error(513);
	return pofs;
}

WORD GetOutpMask(TPredicate* P)
{
	return (0xffff >> (16 - P->Arity)) && !P->InpMask;
}

void RdPredicateDcl(bool FromClauses, TDatabase* Db)
{
	TDomain* d = nullptr;
	TPredicate* p = nullptr;
	//TPredicate* pofs = 0; // absolute p
	TScanInf* si = nullptr;
	//TScanInf* siofs = nullptr; // absolute si
	TFldList* fl = nullptr;
	//TFldList* flofs = nullptr; // absolute fl
	Instr_proc* ip = nullptr;
	//WORD ipofs = 0; // absolute ip
	std::string nm; /*PString*/
	integer n = 0;
	WORD w = 0, m = 0;
	TDomain* a[32]{ nullptr }; /*PDomain*/
	RdbPos pos;
	char typ = '\0';
	WORD bpOfs = 0;
	bool isOutp = false, b = false;
	FieldDescr* f = nullptr;
	BYTE o = 0;
	RdbD* r = nullptr;
	FrmlElem* z = nullptr;
	WORD zofs = 0; // absolute z

	//PtrRec(d).Seg = _Sg; PtrRec(p).Seg = _Sg; PtrRec(si).Seg = _Sg;
	//PtrRec(fl).Seg = _Sg; PtrRec(ip).Seg = _Sg;
	o = 0;
	if (Db != nullptr) o = _DbaseOpt + _CioMaskOpt;
	if (Lexem == '@') {
		RdLexP();
		o = o | _FandCallOpt;
	}
	else if (Db != nullptr) o = o | _PackInpOpt;
	TestIdentifP();
	if (IsCharUpper2(LexWord[1])) Error(518);
	if (GetPredicate() != nullptr) OldError(505);
	nm = StorStr(LexWord);
	if ((o & _FandCallOpt) != 0) {
		if (Db != nullptr) {
			si = new TScanInf();
			si->Name = LexWord;
			CFile = RdFileName();
			if (CFile->typSQLFile) OldError(155);
			si->FD = CFile;
			goto label2;
		}
		else {
			SkipBlank(false);
			if (ForwChar == '[') {
				RdLexP();
				RdLexP();
				TestLex(_quotedstr);
				z = new FrmlElem4(_const, 0); // GetOp(_const, LexWord.length() + 1);
				((FrmlElem4*)z)->S = LexWord;
				//pos.R = ptr(0, OOfs(z));
				pos.IRec = 0;
				AcceptP(_quotedstr);
				TestLex(']');
			}
			else {
				if (!FindChpt('P', LexWord, false, &pos)) Error(37);
				//pos.R = ptr(0, StorStr(LexWord));
				pos.IRec = 0xffff;
			}
		}
	}
	RdLexP();
label2:
	n = 0; w = 0; m = 1;
	if (Lexem == '(') {
		RdLexP();
	label3:
		if (Db != nullptr) {
			if ((o & _FandCallOpt) != 0) {
				f = RdFldName(CFile);
				fl = new TFldList(); // GetZStor(sizeof(TFldList));
				fl->FldD = CFile->FldD.front(); // OPtr(PtrRec(CFile).Seg, f);

				if (si->FL == nullptr) si->FL = fl;
				else ChainLast(si->FL, fl);

				d = nullptr;
				AcceptP('/');
				d = RdDomain();
				switch (f->FrmlTyp) {
				case 'B': {
					if (d != BoolDom) OldError(510);
					break;
				}
				case 'R': {
					if ((d != RealDom) && ((f->Typ != 'F') || (d != IntDom))) OldError(510);
					break;
				}
				default: {
					if (f->Typ == 'T') {
						if ((d != LongStrDom) && ((d->Typ != _FunD) || (d == BoolDom))) OldError(510);
					}
					else if (d != StrDom) OldError(510);
					break;
				}
				}
				goto label4;
			}
		}
		else {
			if (Lexem == '&') RdLexP();
			else w = w | m;
		}
		d = RdDomain();
		if ((d == LongStrDom) && (Db != nullptr)) OldError(541);
		if (((o & _FandCallOpt) != 0) && (d->Typ == _FunD) && (d != BoolDom)) OldError(528);
	label4:
		a[n] = d;
		n++;
		m = m << 1;
		if (Lexem == ',') {
			if (n == 15) Error(519);
			RdLexP();
			goto label3;
		}
		AcceptP(')');
	}
	p = new TPredicate(); // GetZStor(sizeof(TPredicate) - 6 + 2 * n);

	if (FromClauses) {
		if (ClausePreds == nullptr) ClausePreds = p;
		else ChainLast(ClausePreds, p);
	}
	else {
		ChainLast(Roots->Predicates, p);
	}

	if (Db != nullptr) {
		p->ChainDb = Db->Pred;
		Db->Pred = p;
	}

	p->Name = nm;
	p->Arity = n;
	memcpy(p->Arg, a, 2 * n);
	p->Opt = o;
	p->InpMask = w;
	p->InstSz = 4 * n;

	if ((o & _FandCallOpt) != 0) {
		if ((o & _DbaseOpt) != 0) p->Branch = (TBranch*)si;
		else {
			//ipofs = GetZStor(5 + sizeof(RdbPos) + 2 + n * sizeof(TypAndFrml));
			ip = new Instr_proc(0);
			ip->Kind = _proc;
			ip->PPos = pos;
			ip->N = n;
			bpOfs = 4;
			for (size_t i = 0; i < n; i++) {
				d = p->Arg[i];

				if ((d == RealDom) || (d == IntDom)) typ = 'R';
				else if ((d = BoolDom)) typ = 'B';
				else typ = 'S';

				isOutp = (w & 1) == 0;
				if (isOutp) {
					z = new FrmlElem18(_getlocvar, 2); // GetOp(_getlocvar, 2);
					//((FrmlElem18*)z)->BPOfs = bpOfs;
					switch (typ) {
					case 'S': bpOfs += sizeof(longint); break;
					case 'R': bpOfs += sizeof(double); break;
					default: bpOfs += sizeof(bool); break;
					}
				}
				else {
					switch (typ) {
					case 'R': z = new FrmlElem2(_const, 0); /* GetOp(_const, sizeof(double));*/ break;
					case 'B': z = new FrmlElem5(_const, 0); /* GetOp(_const, sizeof(bool));*/ break;
					default: {
						if (d == StrDom) z = new FrmlElem4(_const, 0); // GetOp(_const, sizeof(pstring));
						else {
							z = new FrmlElem18(_getlocvar, 2); // GetOp(_getlocvar, 2);
							//((FrmlElem18*)z)->BPOfs = bpOfs;
							bpOfs += sizeof(longint);
						}
						break;
					}
					}
				}
				TypAndFrml tf;
				tf.FTyp = typ;
				tf.Frml = z;
				tf.FromProlog = true;
				tf.IsRetPar = isOutp;
				ip->TArg.push_back(tf);
				w = w >> 1;
			}
			p->Branch = (TBranch*)ip;
			p->LocVarSz = bpOfs;
		}
	}
}

TCommand* GetCommand(TCommandTyp Code, WORD N)
{
	TCommand* c = new TCommand(); // ptr(_Sg, GetZStor(3 + N));
	//WORD cofs; // absolute c
	c->Code = Code;
	return c; // ofs;
}

void RdTermList(TCommand* C, TDomain* D, WORD Kind)
{
	TTermList* l = new TTermList(); // ptr(_Sg, GetZStor(sizeof(TTermList)));
	//WORD lofs; // absolute l
	ChainLast(C->Arg, l);
	l->Elem = RdTerm(D, Kind);
}

TCommand* RdCommand() /*PCommand*/
{
	TVarDcl* v = nullptr;
	char cm = '\0'; /*PDomain d;*/
	char op = '\0', fTyp = '\0';
	TCommandTyp code;
	TWriteD* w = nullptr;
	WORD wofs = 0; // absolute w
	bool nl = false;
	TPredicate* p = nullptr;
	TDatabase* pofs = 0; // absolute p
	TCommand* c = nullptr;
	TCommand* cofs = nullptr; // absolute c
	LinkD* ld = nullptr;
	FileD* fd = nullptr;
	TDomain* d = nullptr;
	TDomain* dofs = nullptr; // absolute d
	TTermList* l = nullptr;
	WORD lofs = 0; // absolute l
	WORD n = 0;
	pstring s;
	integer i = 0;

	/*PtrRec(p).Seg = _Sg; c = ptr(_Sg, 0); PtrRec(w).Seg = _Sg; PtrRec(d).Seg = _Sg;
	PtrRec(l).Seg = _Sg;*/
	if (Lexem == '!') {
		RdLexP();
		cofs = GetCommand(_CutC, 0);
		goto label9;
	}
	else if (Lexem != _identifier) goto label9;
	if (IsUpperIdentif()) {
		v = FindVarDcl();
		if (v == nullptr) v = MakeVarDcl(0, -1);
		RdLexP();
		dofs = v->Dom;
		if (v->Bound) {
			switch (Lexem) {
			case '=': op = _equ; break;
			case '<': {
				switch (ForwChar) {
				case '>': { ReadChar(); op = _ne; break; }
				case '=': { ReadChar(); op = _le; break; }
				default: op = _lt; break;
				}
				break;
			}
			case '>': {
				if (ForwChar == '=') { ReadChar(); op = _ge; break; }
				else op = _gt;
				break;
			}
			default: { Error(524); break; }
			}
			if ((dofs != IntDom) && (dofs != RealDom) && !(op == _equ || op == _ne)) Error(538);
			RdLexP();
		}
		else {
			if (!v->Used) {
				AcceptP(':');
				dofs = RdDomain();
				v->Dom = dofs;
			}
			AcceptP('=');
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
	if (IsKeyWordP("fail")) {
		cofs = GetCommand(_FailC, 0);
		goto label9;
	}
	if (IsKeyWordP("wait")) {
		cofs = GetCommand(_WaitC, 0);
		goto label9;
	}
	if (IsKeyWordP("trace")) {
		cofs = GetCommand(_Trace, 2);
		AcceptP('(');
		c->TrcLevel = RdIntegerP();
		goto label8;
	}
	if (IsKeyWordP("error")) {
		AcceptP('(');
		cofs = GetCommand(_ErrorC, 2 + 2);
		c->MsgNr = RdIntegerP();
		goto label20;
	}
	if (IsKeyWordP("writeln")) { nl = true; goto label1; }
	if (IsKeyWordP("write")) {
		nl = false;
	label1:
		cofs = GetCommand(_WriteC, 2 + 1);
		c->NL = nl;
		AcceptP('(');
	label2:
		if (Lexem == _quotedstr) {
			//wofs = GetZStor(3 + 1 + LexWord.length());
			w->IsString = true;
			w->SS = LexWord;
		}
		else {
			//wofs = GetZStor(3 + 2 + 2);
			TestIdentifP();
			v = FindVarDcl();
			if (v == nullptr) Error(511);
			else if (!v->Bound) Error(509);
			v->Used = true; w->Dom = v->Dom; w->Idx = v->Idx;
			if ((c->Code == _ErrorC) && (v->Dom != StrDom) &&
				((c->WrD != 0) || (v->Dom != IntDom))) Error(558);
		}
		RdLexP();
		//ChainLst(c->WrD, wofs);
	label20:
		if (Lexem == ',') { RdLexP(); goto label2; } goto label8;
	}
	if (copy(LexWord, 1, 6) == "union_") { pofs = (TDatabase*)&UnionPred; goto label21; }
	if (copy(LexWord, 1, 6) == "minus_") { pofs = (TDatabase*)&MinusPred; goto label21; }
	if (copy(LexWord, 1, 6) == "inter_") {
		pofs = (TDatabase*)&InterPred;
	label21:
		pstring str = "L_";
		str += copy(LexWord, 7, 255);
		dofs = GetDomain(false, str);
		if (d->Typ != _ListD) Error(548);
		RdLexP();
		AcceptP('(');
		cofs = GetCommand(_PredC, 5 * 2);
		c->Pred = (TPredicate*)pofs;
		c->Elem = dofs;
		RdTermList(c, dofs, 2);
		AcceptP(',');
		RdTermList(c, dofs, 2);
		AcceptP(',');
		RdTermList(c, dofs, 1);
		goto label8;
	}
	if (copy(LexWord, 1, 4) == "mem_") { pofs = (TDatabase*)&MemPred; goto label22; }
	if (copy(LexWord, 1, 4) == "len_") { pofs = (TDatabase*)&LenPred; goto label22; }
	if (copy(LexWord, 1, 4) == "inv_") { pofs = (TDatabase*)&InvPred; goto label22; }
	if (copy(LexWord, 1, 4) == "add_") { pofs = (TDatabase*)&AddPred; goto label22; }
	if (copy(LexWord, 1, 4) == "del_") {
		pofs = (TDatabase*)&DelPred;
	label22:
		pstring str = "L_";
		str += copy(LexWord, 5, 255);
		dofs = GetDomain(false, str);
		if (d->Typ != _ListD) Error(548);
		RdLexP();
		AcceptP('(');
		cofs = GetCommand(_PredC, 5 * 2);
		c->Pred = (TPredicate*)pofs;
		c->Elem = dofs; /*ListDom*/
		if (pofs == (TDatabase*)&MemPred) {
			UnbdVarsInTerm = false;
			RdTermList(c, d->ElemDom, 4);
			if (UnbdVarsInTerm) n = 2;
			else n = 3;
			c->InpMask = n; c->OutpMask = !n;
			AcceptP(',');
			RdTermList(c, dofs, 2);
		}
		else {
			if (pofs == (TDatabase*)&AddPred) { RdTermList(c, d->ElemDom, 2); AcceptP(','); }
			else if (pofs == (TDatabase*)&DelPred) { RdTermList(c, d->ElemDom, 1); AcceptP(','); }
			RdTermList(c, dofs, 2);
			AcceptP(',');
			if (pofs == (TDatabase*)&LenPred) RdTermList(c, IntDom, 1);
			else RdTermList(c, dofs, 1);
		}
		goto label8;
	}
	if (IsKeyWordP("loadlex")) {
		code = _LoadLexC;
		AcceptP('(');
		pofs = 0;
		goto label4;
	}
	if (IsKeyWordP("save")) { code = _SaveC; goto label3; }
	if (IsKeyWordP("consult")) {
		code = _ConsultC;
	label3:
		AcceptP('(');
		TestIdentifP();
		pofs = FindDataBase(LexWord);
		if (pofs == 0) Error(531);
		RdLexP();
		AcceptP(',');
	label4:
		cofs = GetCommand(code, 2 + 4 + 4 + 1 + LexWord.length());
		c->DbPred = pofs;
		//Move(LexWord, c->Name, LexWord.length() + 1);
		c->Name = LexWord;
		if (!IsRoleName(false, &fd, &ld)) Error(9);
		if (fd->typSQLFile) OldError(155);
		AcceptP('.');
		c->FldD = RdFldName(fd);
		if (c->FldD->Typ != 'T') OldError(537);
	label8:
		AcceptP(')');
	}
label9:
	return cofs;
}

TCommand* RdPredCommand(TCommandTyp Code)
{
	TCommand* c = nullptr;
	TCommand* cofs = 0; // absolute c 
	TPredicate* p = nullptr;
	TPredicate* pofs = 0; // absolute p
	WORD i = 0, n = 0, w = 0, m = 0, kind = 0, sz = 0, InpMask = 0, OutpMask = 0;
	TTermList* lRoot = nullptr;
	TTermList* l = nullptr;
	TTermList* lofs = nullptr; // absolute l
	TDomain* dofs = nullptr;
	TPTerm* t = nullptr;
	WORD tofs = 0; // absolute t
	TScanInf* si = nullptr;
	WORD siofs = 0; // absolute si
	XKey* k = nullptr; KeyFldD* kf = nullptr;
	TFldList* fl = nullptr;
	TFldList* flofs = nullptr; // absolute fl
	BYTE a[256]{ 0 };
	bool IsFandDb = false, inOut = false;
	FieldDescr* f = nullptr;

	/*PtrRec(p).Seg = _Sg; PtrRec(c).Seg = _Sg; PtrRec(l).Seg = _Sg;
	PtrRec(t).Seg = _Sg; PtrRec(si).Seg = _Sg; PtrRec(fl).Seg = _Sg;*/
	pofs = RdPredicate();
	IsFandDb = (p->Opt & (_DbaseOpt + _FandCallOpt)) == _DbaseOpt + _FandCallOpt;
	if (((p->Opt & _DbaseOpt) != _DbaseOpt) && (Code == _AssertC || Code == _RetractC)) OldError(526);
	kind = 1; m = 1; w = p->InpMask; sz = 2 + 2; InpMask = 0; OutpMask = 0; lRoot = 0;
	if ((p->Opt & _CioMaskOpt) != 0) {
		sz += 4;
		if (Code == _AssertC) w = 0xffff;
		else kind = 4;
	}
	if (p->Arity > 0) {
		AcceptP('(');
		for (i = 0; i <= p->Arity - 1; i++) {
			if (i > 0) AcceptP(',');
			UnbdVarsInTerm = false;
			if (kind != 4) if ((w & 1) != 0) kind = 2;
			else kind = 1;
			dofs = p->Arg[i];
			lofs = new TTermList(); // GetZStor(sizeof(TTermList));
			ChainLast(lRoot, lofs);
			l->Elem = RdTerm(dofs, kind);
			if (p->Opt && _CioMaskOpt != 0)
				if (UnbdVarsInTerm) {
					if (l->Elem != UnderscoreTerm)
						OutpMask = OutpMask | m;
				}
				else InpMask = InpMask | m;
			m = m << 1;
			w = w >> 1;
		}
		AcceptP(')');
	}
	if ((p->Opt & _BuildInOpt) != 0) {
		switch (p->LocVarSz) {
		case _ConcatP: if (!(InpMask >= 3 && InpMask <= 7)) OldError(534); break;
		case _FandFieldP:
		case _FandLinkP: {
			if ((InpMask & 1) == 0/*o...*/) OldError(555);
			if (p->LocVarSz == _FandLinkP) {
				InpMask = InpMask & 0x7;
				if (InpMask == 0x7) InpMask = 5;
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
		if (CFile->Typ == 'X') {
			//k = CFile->Keys;
			while (k != nullptr) {
				kf = k->KFlds; inOut = false;
				while (kf != nullptr) {
					m = 1; n = 0; i++; flofs = si->FL;
					while (flofs != nullptr) {
						f = fl->FldD; flofs = (TFldList*)fl->pChain;
						if (f == kf->FldD) {
							w = w | m;
							a[i] = n;
							if ((flofs != nullptr) && (f == fl->FldD) && ((OutpMask & (m << 1)) != 0)
								&& (f->Typ == 'A')) inOut = true;
							goto label1;
						}
						m = m << 1;
						n++;
					} goto label2;
				label1:
					kf = (KeyFldD*)kf->pChain;
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
				//Move(a, c->ArgI, i);

				c->ArgI[0] = a[0];
				c->KDOfs = k;
			}
		}
	}
	return cofs;
}

void RdDbTerm(TDomain* D)
{
	char* p = PackedTermPtr;
	WORD i = 0; WORD n = 0; WORD* wp = nullptr; TDomain* d = nullptr;
	pstring s; bool minus = false; double r = 0.0;
	TFunDcl* f = nullptr; BYTE idx = 0;
	d = D;
	//if (PtrRec(p).Ofs >= PTPMaxOfs) Error(527);
	switch (Lexem) {
	case _quotedstr: {
		if (d->Typ != _StrD) Error(510);
		n = LexWord.length() + 1;
		//if (PtrRec(p).Ofs + n >= PTPMaxOfs) Error(527);
		Move(&LexWord[0], &p, n);
		p += n;
		RdLexP();
		break;
	}
	case '$': {
		if (d->Typ != _IntD) Error(510);
		else {
			i = 0;
			while ((ForwChar >= '0' && ForwChar <= '9') || (ForwChar >= 'a' && ForwChar <= 'f') || (ForwChar >= 'A' && ForwChar <= 'F')) {
				i++;
				if (i > 4) Error(3);
				ReadChar();
				s[i] = CurrChar;
			}
			if (i == 0) Error(504);
			s[0] = char(i);
			//n = HexStrToLong(s);
			unsigned int n = std::stoul(s.c_str(), nullptr, 16);
			RdLexP();
			goto label2;
		}
		break;
	}
	case '-': {
		RdLexP();
		if (Lexem != _number) Error(525);
		minus = true;
		goto label1;
		break;
	}
	case _number: {
		minus = false;
	label1:
		s = LexWord;
		RdLexP();
		if (d->Typ == _IntD) {
			val(s, n, i);
			if (minus) n = -n;
		label2:
			*(integer*)p = n;
			p += 2;
		}
		else {
			if (d->Typ != _RealD) Error(510);
			if ((Lexem = '.') && isdigit(ForwChar)) {
				RdLexP();
				s.Append('.');
				s += LexWord;
				RdLexP();
			}
			val(s, r, i);
			*(double*)p = r;
			p += sizeof(double);
		}
		break;
	}
	case '[': {
		if (d->Typ != _ListD) Error(510);
		RdLexP();
		wp = (WORD*)p;
		p += 2;
		n = 0;
		if (Lexem != ']') {
		label3:
			RdDbTerm(d->ElemDom);
			n++;
			if (Lexem == ',') { RdLexP(); goto label3; }
		}
		AcceptP(']');
		*wp = n;
		break;
	}
	default: {
		TestIdentifP();
		if (d->Typ != _FunD) Error(510);
		f = GetFunDclByName(D, idx);
		if (f == nullptr) Error(512);
		*p = char(idx); p++;
		RdLexP();
		n = f->Arity;
		if (n > 0) {
			AcceptP('(');
			for (i = 0; i <= n - 1; i++) {
				if (i > 0) AcceptP(',');
				RdDbTerm((TDomain*)f->Arg[i]);
			}
			AcceptP(')');
		}
		break;
	}
	}
}

void RdDbClause(TPredicate* P)
{
	WORD i = 0, n = 0;
	char* t = PackedTermPtr; WORD* wp = nullptr; TDbBranch* b = nullptr;
	char A[4000 + 1];
	AcceptP('(');
	t = (char*)A;
	//PTPMaxOfs = ofs(A) + MaxPackedPredLen - 2;
	for (i = 0; i <= P->Arity - 1; i++) {
		if (i > 0) AcceptP(',');
		wp = (WORD*)t;
		t += 2;
		//RdDbTerm(p->Arg[i]);
		//*wp = PtrRec(t).Ofs - PtrRec(wp).Ofs - 2;
	}
	//n = PtrRec(t).Ofs - Ofs(A);
	//b = Mem3.Alloc(4 + n);
	//Move(A, b->LL, n);
	//ChainLast(P->Branch, b);
	AcceptP(')');
	AcceptP('.');
}

void CheckPredicates(TPredicate* POff)
{
	TPredicate* p = nullptr;
	TPredicate* pofs = 0; // absolute p
	TScanInf* si = nullptr;
	p = POff;
	while (pofs != nullptr) {
		if (((p->Opt & (_DbaseOpt + _FandCallOpt + _BuildInOpt)) == 0) && (p->Branch == nullptr)) {
			SetMsgPar(p->Name);
			OldError(522);
		}
		if ((p->Opt & _DbaseOpt) != 0)
			if ((p->Opt & _FandCallOpt) != 0) {
				si = (TScanInf*)p->Branch;
				si->FD = nullptr;
			}
			else p->Branch = nullptr;
		pofs = (TPredicate*)p->pChain;
	}
}

void RdAutoRecursionHead(TPredicate* p, TBranch* b)
{
	TTermList* l = nullptr;
	TTermList* l1 = nullptr;
	WORD w = 0;
	TCommand* c = nullptr;
	TPTerm* t = nullptr;
	TDomain* d = nullptr;
	TVarDcl* v = nullptr;
	TTermList* lofs = nullptr; // absolute l
	TTermList* l1ofs = nullptr; // absolute l1
	TCommand* cofs = nullptr; // absolute c
	TPTerm* tofs = nullptr; // absolute t
	TDomain* dofs = nullptr; // absolute d
	bool isInput = false;
	integer i = 0, j = 0, k = 0;
	if (p->Opt != 0) Error(550);
	//PtrRec(c).Seg = _Sg; PtrRec(l).Seg = _Sg; PtrRec(l1).Seg = _Sg;
	//PtrRec(t).Seg = _Sg; PtrRec(d).Seg = _Sg;
	cofs = GetCommand(_AutoC, 2 + 3 + 6 * 2);
	b->Cmd = cofs;
	p->InstSz += 4;
	c->iWrk = (p->InstSz / 4) - 1;
	w = p->InpMask;
	for (i = 0; i <= p->Arity - 1; i++) {
		if ((w & 1) != 0) isInput = true;
		else isInput = false;
		lofs = new TTermList(); // GetZStor(sizeof(TTermList));
		ChainLast(c->Arg, lofs);
		tofs = new TPTerm(); // GetZStor(1 + 2 + 1);
		t->Fun = _VarT; t->Idx = i; t->Bound = isInput;
		l->Elem = tofs;
		l1ofs = new TTermList(); // GetZStor(sizeof(TTermList));
		ChainLast(b->Head, l1ofs);
		dofs = p->Arg[i];
		if (i > 0) AcceptP(',');
		else if (!(d->Typ == _FunD || d->Typ == _ListD)) Error(556);
		if (Lexem == '!') {
			if (i > 0) {
				if (isInput || (dofs != p->Arg[0]) || (c->iOutp > 0)) Error(551);
				c->iOutp = i;
			}
			else if (!isInput) Error(552);
		}
		else if (TestKeyWordP('_')) {
			if (!isInput) {
				if (d->Typ == _FunD) Error(575); j = 0;
				goto label1;
			}
		}
		else {
			if (!IsUpperIdentif()) Error(511);
			v = FindVarDcl();
			if (v == nullptr) v = MakeVarDcl(dofs, i);
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
		RdLexP();
		w = w >> 1;
	}
	AcceptP(')');
}

void RdSemicolonClause(TPredicate* p, TBranch* b)
{
	TVarDcl* v = nullptr;
	TCommand* c = nullptr;
	TCommand* cofs = nullptr; // absolute c
	char x = '\0';
	TBranch* bofs = nullptr; // absolute B
	RdLexP();
	//PtrRec(c).Seg = _Sg;
	if (IsUpperIdentif()) {
		x = 'a';
		v = FindVarDcl();
		if (p->InpMask != (1 << (p->Arity - 1)) - 1) Error(562);
		if ((v == nullptr) || v->Bound || (v->Dom->Typ != _ListD)) Error(561);
		RdLexP();
		AcceptP('+'); AcceptP('=');
		v->Bound = true;
		cofs = GetCommand(_AppPkC, 6);
		c->apIdx = v->Idx;
		c->apTerm = RdTerm(v->Dom, 2);
		ChainLast(b->Cmd, cofs);
		if (Lexem == ',') {
			RdLexP();
			AcceptPKeyWord("self");
			cofs = GetCommand(_SelfC, 0);
			goto label2;
		}
		goto label1;
	}
	else if (TestKeyWordP("error")) {
		x = 'e';
		cofs = GetCommand(_CutC, 0);
	}
	else if (IsKeyWordP("self")) { x = 's'; goto label3; }
	else {
		x = 'f';
	label1:
		cofs = GetCommand(_FailC, 0);
	}
label2:
	ChainLast(b->Cmd, cofs);
label3:
	bofs = new TBranch(); // GetZStor(sizeof(TBranch));
	ChainLast(p->Branch, bofs);
	switch (x) {
	case 'e': b->Cmd = RdCommand(); break;
	case 'f': if (GetOutpMask(p) != 0) Error(559); break;
	case 's': b->Cmd = GetCommand(_SelfC, 0); break;
	case 'a': {
		cofs = GetCommand(_AppUnpkC, 4); c->apIdx = v->Idx; c->apDom = (TPTerm*)v->Dom;
		b->Cmd = cofs;
		break;
	}
	}
}

void RdClauses()
{
	TBranch* b = nullptr;
	WORD w = 0, m = 0, mp = 0; integer i = 0, kind = 0; bool iserr = false;
	TDomain* d = nullptr;
	TDomain* dEl = nullptr;
	TTermList* l = nullptr;
	TPTerm* t = nullptr;
	TPredicate* p = nullptr;
	TPredicate* p1 = nullptr;
	TCommand* c = nullptr;
	TVarDcl* v = nullptr;
	void* x = nullptr;
	TCommandTyp code;
	struct SF {
		WORD BP = 0; void* Ret = nullptr; WORD RetOfs = 0;
	};
	SF* z = nullptr;
	WORD zofs; // absolute z
	bool WasNotC = false;

	/*PtrRec(d).Seg = _Sg; PtrRec(dEl).Seg = _Sg; PtrRec(p).Seg = _Sg;
	PtrRec(l).Seg = _Sg; PtrRec(c).Seg = _Sg; PtrRec(b).Seg = _Sg;
	PtrRec(t).Seg = _Sg; PtrRec(p1).Seg = _Sg;*/
label1:
	if (Lexem == ':') {
		RdLexP();
		RdPredicateDcl(true, nullptr);
		goto label6;
	}
	TestIdentifP();
	p = RdPredicate();
	if (p->Opt && (_FandCallOpt + _BuildInOpt) != 0) OldError(529);
	if (p->Opt && _DbaseOpt != 0) { RdDbClause(p); goto label6; }
	VarDcls = nullptr;
	VarCount = p->Arity;
	x = Mem1.Mark();
	b = new TBranch(); // GetZStor(sizeof(TBranch));
	ChainLast(p->Branch, b);
	if (p->Arity > 0) {
		AcceptP('(');
		if (Lexem == '!') { RdAutoRecursionHead(p, b); goto label4; }
		w = p->InpMask; m = 1;
		for (i = 0; i <= p->Arity - 1; i++) {
			if (i > 0) AcceptP(',');
			d = p->Arg[i];
			kind = 1;
			if ((w & 1) == 0) kind = 3;
			l = new TTermList(); // GetZStor(sizeof(TTermList));
			ChainLast(b->Head, l);
			SkipBlank(false);
			if (IsUpperIdentif() && (ForwChar == ',' || ForwChar == ')') /*solo variable*/) {
				RdVar(d, kind, i, t);
				if (t != nullptr) goto label11;
			}
			else {
				t = nullptr;
			label11:
				if ((w & 1) != 0) b->HeadIMask = b->HeadIMask | m;
				else b->HeadOMask = b->HeadOMask | m;
				if (t == nullptr) t = RdTerm(d, kind);
				l->Elem = t;
			}
			w = w >> 1;
			m = m << 1;
		}
		AcceptP(')');
	}
	if (Lexem != '.') {
		AcceptP(_assign);
	label2:
		WasNotC = false;
		if (IsKeyWordP("self")) {
			c = GetCommand(_SelfC, 0);
			ChainLast(b->Cmd, c);
			goto label4;
		}
		if (IsKeyWordP("not")) { AcceptP('('); WasNotC = true; }
		c = RdCommand();
		if (c == nullptr)
			if ((Lexem == _identifier) && (copy(LexWord, 1, 4) == "all_")) {
				pstring s1 = "L_";
				s1 += copy(LexWord, 5, 255);
				d = GetDomain(false, s1);
				if (d->Typ != _ListD) Error(548);
				RdLexP();
				AcceptP('(');
				c = RdPredCommand(_AllC);
				AcceptP(',');
				c->Elem = (TDomain*)RdTerm(d->ElemDom, 2);
				AcceptP(',');
				c->Idx = VarCount;
				VarCount++;
				RdVar(d, 5, -1, (TPTerm*)&c->Idx2);
				AcceptP(')');
			}
			else {
				if (IsKeyWordP("assert")) { code = _AssertC; goto label3; }
				else {
					if (IsKeyWordP("retract")) {
						code = _RetractC;
					label3:
						AcceptP('(');
						c = RdPredCommand(code);
						AcceptP(')');
					}
					else c = RdPredCommand(_PredC);
				}
			}
		ChainLast(b->Cmd, c);
		if (WasNotC) {
			if (c->Code != _PredC) OldError(546);
			p1 = c->Pred;
			l = c->Arg;
			if ((p1->Opt & _CioMaskOpt) != 0) w = c->InpMask;
			else w = p1->InpMask;
			while (l != nullptr) {
				if ((w & 1) == 0) {
					t = l->Elem;
					if ((t == nullptr) || (t->Fun != _UnderscT)) OldError(547);
				}
				l = l->pChain;
				w = w >> 1;
			}
			AcceptP(')');
			c->Code = _NotC;
		}
		if (Lexem == ',') { RdLexP(); goto label2; }
		if (Lexem == ';') RdSemicolonClause(p, b);
	}
label4:
	AcceptP('.');
	v = VarDcls;
	while (v != nullptr) {
		if (!v->Used || !v->Bound) {
			SetMsgPar(v->Name);
			if (!v->Used) OldError(521);
			else OldError(520);
		}
		v = v->pChain;
	}
	p->InstSz = MaxW(p->InstSz, 4 * VarCount);
	Mem1.Release(x);
label6:
	if (!(Lexem == 0x1A || Lexem == '#')) goto label1;
	CheckPredicates(ClausePreds);
	ClausePreds = nullptr;
}

TPredicate* MakePred(std::string PredName, std::string ArgTyp, WORD PredKod, WORD PredMask)
{
	TDomain* d = nullptr;
	TPredicate* p = new TPredicate();
	if (Roots->Predicates == nullptr) Roots->Predicates = p->pChain;
	else ChainLast(Roots->Predicates, p->pChain);

	TPredicate* result = p->pChain;
	p->Name = StorStr(PredName);
	p->Arity = ArgTyp.length();
	p->LocVarSz = PredKod;
	for (size_t i = 0; i < ArgTyp.length(); i++) {
		switch (ArgTyp[i]) {
		case 's': d = StrDom; break;
		case 'l': d = LongStrDom; break;
		case 'i': d = IntDom; break;
		case 'r': d = RealDom; break;
		case 'b': d = BoolDom; break;
		case 'x': d = LLexDom; break;
		}
		p->Arg[i] = d;
		if (PredMask == 0xffff) {
			p->Opt = _BuildInOpt + _CioMaskOpt;
		}
		else {
			p->Opt = _BuildInOpt;
			p->InpMask = PredMask;
		}
		p->InstSz = ArgTyp.length() * 4;
	}
	return result;
}

WORD ReadProlog(WORD RecNr)
{
	pstring Booln = "Boolean";
	pstring Reell = "Real";
	TDatabase* db = nullptr;
	//WORD* dbofs = &db->pChain; // absolute db dbofs;
	TPredicate* p = nullptr;
	//WORD* pofs = &p->pChain;
	TDomain* d = nullptr;
	//WORD* dofs = &d->pChain;
	TFunDcl* f = nullptr;
	//WORD* fofs = &f->pChain;
	pstring s;
	RdbPos pos;
	LongStr* ss = nullptr;
	void* p1 = nullptr; void* p2 = nullptr; void* pp1 = nullptr;
	void* pp2 = nullptr; void* pp3 = nullptr; void* cr = nullptr;
	longint AA = 0;
	WORD result = 0;

	MarkBoth(p1, p2);
	cr = CRecPtr;
	if (ProlgCallLevel == 0) {
		FreeMemList = nullptr;
		Mem1.Init(); Mem2.Init(); Mem3.Init();
	}
	else {
		pp1 = Mem1.Mark(); pp2 = Mem2.Mark(); pp3 = Mem3.Mark();
	}
	AlignLongStr();
	ss = new LongStr(2); // GetStore(2);
	//AA = AbsAdr(&ss->A);
	//_Sg = PtrRec(HeapPtr).Seg;
	//result = _Sg;
	//PtrRec(db).Seg = _Sg;
	db = new TDatabase();
	//PtrRec(p).Seg = _Sg;
	p = new TPredicate();
	//PtrRec(d).Seg = _Sg;
	d = new TDomain();
	//PtrRec(f).Seg = _Sg;
	f = new TFunDcl();
	ClausePreds = 0;
	Roots = new TProgRoots(); // GetZStore(sizeof(TProgRoots));
	UnderscoreTerm = new TPTerm(); // GetZStor(1);
	UnderscoreTerm->Fun = _UnderscT;
	StrDom = MakeDomain(_StrD, "String");
	LongStrDom = MakeDomain(_LongStrD, "LongString");
	IntDom = MakeDomain(_IntD, "Integer");
	RealDom = MakeDomain(_RealD, "Real");
	BoolDom = MakeDomain(_FunD, "Boolean");

	d->pChain = BoolDom;

	f->pChain = new TFunDcl();
	f->Name = StorStr("false");
	if (d->FunDcl == nullptr) d->FunDcl = f->pChain;
	else ChainLast(d->FunDcl, f->pChain);

	f->pChain = new TFunDcl();
	f->Name = StorStr("true");
	if (d->FunDcl == nullptr) d->FunDcl = f->pChain;
	else ChainLast(d->FunDcl, f->pChain);

	p->pChain = new TPredicate();
	p->Name = StorStr("main");
	Roots->Predicates = p->pChain;
	LexDom = MakeDomain(_FunD, "Lexem");
	d->pChain = LexDom;

	f->pChain = new TFunDcl();
	f->Name = StorStr("lex");
	f->Arity = 3;
	f->Arg[0] = IntDom;
	f->Arg[1] = IntDom;
	f->Arg[2] = StrDom;
	if (d->FunDcl == nullptr) d->FunDcl = f->pChain;
	else ChainLast(d->FunDcl, f->pChain);

	LLexDom = MakeDomain(_ListD, "L_Lexem");
	d->pChain = LLexDom;
	d->ElemDom = LexDom;
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
	MakePred("getlex", "x", _GetLexP, 0/*o*/);
	ResetCompilePars();
	RdLexP();
	while (Lexem != 0x1A) {
		AcceptP('#');
		if (IsKeyWordP("DOMAINS")) {
			RdDomains();
		}
		else if (IsKeyWordP("CONSTANTS")) {
			RdConstants();
		}
		else if (IsKeyWordP("DATABASE")) {
			s[0] = 0;
			if (Lexem == '-') {
				RdLexP();
				TestIdentifP();
				s = LexWord;
				RdLexP();
			}
			db->pChain = FindDataBase(s);
			if (db->pChain == nullptr) {
				db->pChain = new TDatabase();
				if (Roots->Databases == nullptr) Roots->Databases = db->pChain;
				else ChainLast(Roots->Databases, db->pChain);
				db->Name = s;
			}
			do {
				RdPredicateDcl(false, db);
			} while (Lexem != 0x1A && Lexem != '#');
		}
		else {
			if (IsKeyWordP("PREDICATES")) {
				do {
					RdPredicateDcl(false, nullptr);
				} while (Lexem != 0x1A && Lexem != '#');
			}
			else {
				AcceptPKeyWord("CLAUSES");
				RdClauses();
			}
		}
	}
	//if (AbsAdr(HeapPtr) - AA > MaxLStrLen) OldError(544);
	db->pChain = Roots->Databases;
	while (db->pChain != nullptr) {
		//db->SOfs = WORD(OPtr(_Sg, SaveDb(dbofs, AA)));
		db = db->pChain;
	}
	CheckPredicates(Roots->Predicates);
	//ss->LL = AbsAdr(HeapPtr) - AA;
	if (ProlgCallLevel == 0) ReleaseStore2(p2);
	else {
		Mem1.Release(pp1); Mem2.Release(pp2); Mem3.Release(pp3);
	}
	if (RecNr != 0) {
		CFile = Chpt; CRecPtr = cr;
		StoreChptTxt(ChptOldTxt, ss, true);
		WriteRec(CFile, RecNr, CRecPtr);
		ReleaseStore(p1);
	}
	return result;
}
