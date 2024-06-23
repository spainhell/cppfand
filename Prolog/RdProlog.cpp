#include "RdProlog.h"

#include "Prolog.h"
#include "RunProlog.h"
#include "TMemory.h"
#include "../Core/base.h"
#include "../Core/Chained.h"
#include "../Core/FieldDescr.h"
#include "../Core/FileD.h"
#include "../Common/pstring.h"
#include "../Core/Compiler.h"
#include "../Core/GlobalVariables.h"
#include "../Core/KeyFldD.h"
#include "../Core/obaseww.h"
#include "../Core/runproj.h"
#include "../Core/models/Instr.h"
#include "..\TextEditor\TextEditor.h"
#include "../fandio/XKey.h"

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
	g_compiler->SkipBlank(false);
	g_compiler->ReadChar();
	Lexem = CurrChar;
	switch (CurrChar) {
	case '\'': {
		Lexem = _quotedstr;
		g_compiler->ReadChar();
		LexWord = "";
		while ((CurrChar != '\'') || (ForwChar == '\'')) {
			if (CurrChar == 0x1A) g_compiler->Error(17);
			// asi puvodni kontrola delky retezce:
			// if (LexWord.length() == pred(sizeof(LexWord))) Error(6);
			if (CurrChar == '\'') g_compiler->ReadChar();
			else if (CurrChar == '\\') g_compiler->RdBackSlashCode();
			LexWord.Append(CurrChar);
			g_compiler->ReadChar();
		}
		break;
	}
	case ':': {
		if (ForwChar == '-') {
			g_compiler->ReadChar();
			Lexem = _assign;
		}
		break;
	}
	case '|': {
		if (ForwChar == '|') { g_compiler->ReadChar(); Lexem = _or; }
		break;
	}
	case '&': {
		if (ForwChar == '&') { g_compiler->ReadChar(); Lexem = _and; }
		break;
	}
	default:
		if (IsLetter(CurrChar)) {
			Lexem = _identifier; LexWord[1] = CurrChar; i = 1;
			while (IsLetter(ForwChar) || isdigit(ForwChar)) {
				i++;
				if (i > 32) g_compiler->Error(2);
				g_compiler->ReadChar();
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
				if (i > 9) g_compiler->Error(3);
				g_compiler->ReadChar();
				LexWord[i] = CurrChar;
			}
			LexWord[0] = char(i);
		}
	}
}

void TestIdentifP()
{
	if (Lexem != _identifier) g_compiler->Error(29);
}

void AcceptP(char X)
{
	if (Lexem != X)
		if (X == _assign) g_compiler->Error(506);
		else {
			ExpChar = X;
			g_compiler->Error(1);
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

void AcceptPKeyWord(std::string s)
{
	if (!IsKeyWordP(s)) { SetMsgPar(s); g_compiler->Error(33); }
}

short RdIntegerP()
{
	short i = 0, j = 0;
	if (Lexem != _number) g_compiler->Error(525);
	val(LexWord, i, j);
	RdLexP();
	return i;
}

/*  T D O M A I N  =========================================================*/
TFunDcl* GetFunDclByName(TDomain* D, BYTE& I)
{
	TFunDcl* result = nullptr;
	std::string sLexWord = LexWord;
	for (I = 0; I < D->FunDcl.size(); I++) {
		if (D->FunDcl[I]->Name == sLexWord) {
			result = D->FunDcl[I];
			break;
		}
	}
	return result;
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
TDatabase* FindDataBase(std::string db)
{
	const auto it = Roots->Databases.find(db);
	if (it == Roots->Databases.end()) return nullptr;
	else return it->second;
}

/*  T P rdb O G rdb A M  =======================================================*/
TTerm* FindConst(std::string name /*LexWord*/, TDomain* domain)
{
	for (auto& c : Roots->Consts) {
		if (c->Dom == domain && c->Name == name) return c->Expr;
	}
	return nullptr;
}

bool RdConst(TDomain* D, TTerm** RT)
{
	if (Lexem == _identifier) {
		TTerm* t = FindConst(LexWord, D);
		if (t != nullptr) {
			RdLexP();
			*RT = t;
			return true;
		}
	}
	return false;
}

TVarDcl* FindVarDcl(const std::string name, std::vector<TVarDcl*>& Vars)
{
	TVarDcl* var = nullptr;
	for (TVarDcl* v : Vars) {
		if (v->Name == name) {
			var = v;
			break;
		}
	}
	return var;
}

TVarDcl* MakeVarDcl(TDomain* D, short Idx, std::vector<TVarDcl*>& Vars)
{
	TVarDcl* v = new TVarDcl();
	v->Dom = D;
	v->Name = LexWord;

	if (Idx < 0) {
		v->Idx = Vars.size();
	}
	else {
		v->Idx = Idx;
	}
	Vars.push_back(v);
	return v;
}

TTerm* GetOp1(TDomain* D, instr_type Op, TTerm* E1);

bool RdVar(TDomain* D, short Kind, short Idx, TTerm** RT, WORD* kind5idx, std::vector<TVarDcl*>& Vars) /*PTerm || idx*/
{
	if (IsKeyWordP("_")) {
		if (!(Kind >= 1 && Kind <= 4)) g_compiler->OldError(508);
		UnbdVarsInTerm = true;
		WasUnbd = true;
		*RT = UnderscoreTerm;
		return true;
	}
	if (!IsUpperIdentif() || (Kind == 6/*const dcl*/)) {
		*RT = nullptr;
		if (Kind == 5) g_compiler->Error(523);
		return false;
	}

	TVarDcl* var_dcl = FindVarDcl(LexWord, Vars);
	if (var_dcl == nullptr) {
		// 1. vyskyt promenne, jeste neexistuje -> bude vytvorena
		var_dcl = MakeVarDcl(D, Idx, Vars);
	}
	else if ((var_dcl->Dom != D) &&
		!((var_dcl->Dom == StrDom) && (D == LongStrDom)) &&
		!((var_dcl->Dom == LongStrDom) && (D == StrDom)) &&
		!((var_dcl->Dom == IntDom) && (D == RealDom)) &&
		!((var_dcl->Dom == RealDom) && (D == IntDom))) {
		RdLexP();
	label1:
		SetMsgPar(var_dcl->Dom->Name, D->Name);
		g_compiler->OldError(507);
	}

	RdLexP();
	bool bnd = var_dcl->Bound; /* idx=-1 except solo variable in head */

	switch (Kind) { /* head 1-i,call-o  call 2-i  head 3-o  unbound 5-o */
	case 1: {
		var_dcl->Bound = true;
		if (bnd) {
			var_dcl->Used = true;
		}
		break;
	}
	case 2: {
		if (!bnd) g_compiler->Error(509);
		var_dcl->Used = true;
		break;
	}
	case 3: {
		var_dcl->Used = true;
		break;
	}
	case 4: {
		if (bnd) {
			var_dcl->Used = true;
		}
		else {
			var_dcl->Bound = true;
			UnbdVarsInTerm = true;
			WasUnbd = true;
		}
		break;
	}
	case 5: {
		var_dcl->Bound = true;
		if (bnd) g_compiler->OldError(523);
		*kind5idx = var_dcl->Idx;
		return true;
	}
	}

	*RT = nullptr;
	if ((Idx == -1) || (var_dcl->Idx != Idx)) {
		// k promenne bude vytvoreny "nosic informace (hodnoty)"
		TTerm* t = new TTerm();
		t->Name = LexWord;
		t->Fun = prolog_func::_VarT;
		t->Idx = var_dcl->Idx;
		t->Bound = bnd;
		if ((var_dcl->Dom != D)) {
			if (!bnd) goto label1;
			t = GetOp1(D, _conv, t);
		}
		var_dcl->term = t;
		*RT = t;
	}
	return true;
}

TTerm* RdTerm(TDomain* D, short Kind, std::vector<TVarDcl*>& Vars); /*PPTerm*/ // forward;
TTerm* RdAddExpr(TDomain* D, short Kind, std::vector<TVarDcl*>& Vars); /*PPTerm*/ // forward;

prolog_func DomFun(TDomain* D)
{
	if (D == IntDom) return prolog_func::_IntT;
	else if (D == RealDom) return prolog_func::_RealT;
	else if (D == StrDom) return prolog_func::_StrT;
	else return prolog_func::_LongStrT;
}

TTerm* GetOp1(TDomain* D, instr_type Op, TTerm* E1)
{
	TTerm* t = new TTerm();
	t->Fun = DomFun(D);
	t->Op = Op;
	WasOp = true;
	t->E1 = E1;
	return t;
}

TTerm* GetOp2(TDomain* D, instr_type Op, TTerm* E1, TTerm* E2)
{
	TTerm* t = new TTerm();
	t->Fun = DomFun(D);
	t->Op = Op;
	WasOp = true;
	t->E1 = E1;
	t->E2 = E2;
	return t;
}

TTerm* GetFunOp(TDomain* D, TDomain* ResD, instr_type Op, std::string ArgTyp, short Kind, std::vector<TVarDcl*>& Vars)
{
	TTerm* t = nullptr;
	TTerm* t1 = nullptr;
	if (D != ResD) g_compiler->OldError(510);
	size_t l = ArgTyp.length();
	if (l > 0) AcceptP('(');
	t = new TTerm(); // ptr(_Sg, GetZStor(1 + 1 + 2 * l));
	t->Fun = DomFun(D);
	t->Op = Op;
	WasOp = true;
	for (size_t i = 0; i < l; i++) {
		if (i > 0) AcceptP(',');
		switch (ArgTyp[i]) {
		case 'l': t1 = RdAddExpr(LongStrDom, Kind, Vars); break;
		case 's': t1 = RdAddExpr(StrDom, Kind, Vars); break;
		case 'i': t1 = RdAddExpr(IntDom, Kind, Vars); break;
		case 'c': {
			if ((Lexem != _quotedstr) || (LexWord.length() != 1)) g_compiler->Error(560);
			//t1ofs = LexWord[1];
			RdLexP();
			break;
		}
		}
		t->E[i] = t1;
	}
	if (l > 0) AcceptP(')');
	return t;
}

TTerm* RdPrimExpr(TDomain* D, short Kind, std::vector<TVarDcl*>& Vars)
{
	TTerm* t = nullptr;
	instr_type op = _notdefined;
	bool minus = false; double r = 0.0;
	pstring s; short i = 0; int n = 0;

	// PtrRec(t).Seg = _Sg;
	switch (Lexem) {
	case '^': {
		if (D != IntDom) g_compiler->Error(510);
		op = (instr_type)Lexem;
		RdLexP();
		t = GetOp1(D, op, RdPrimExpr(D, Kind, Vars));
		break;
	}
	case '(': {
		RdLexP();
		t = RdAddExpr(D, Kind, Vars);
		AcceptP(')');
		break;
	}
	case _quotedstr: {
		if ((D != StrDom) && (D != LongStrDom)) g_compiler->Error(510);
		//tofs = GetZStor(1 + 1 + 1 + LexWord.length());
		t = new TTerm();
		t->Fun = DomFun(D);
		t->Op = _const;
		t->SS = LexWord;
		RdLexP();
		break;
	}
	case '$': {
		if (D != IntDom) g_compiler->Error(510);
		else {
			i = 0;
			//while (ForwChar in['0'..'9', 'a'..'f', 'A'..'F']) {
			while (ForwChar >= '0' && ForwChar <= '9'
				|| ForwChar >= 'a' && ForwChar <= 'f'
				|| ForwChar >= 'A' && ForwChar <= 'F') {
				i++;
				if (i > 4) g_compiler->Error(3);
				g_compiler->ReadChar();
				s[i] = CurrChar;
			}
			if (i == 0) g_compiler->Error(504);
			s[0] = (char)i;
			char* p;
			n = (int)strtoul(s.c_str(), &p, 16);
			RdLexP();
			goto label2;
		}
		break;
	}
	case '-': {
		RdLexP();
		if (Lexem != _number) g_compiler->Error(525);
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
			val(s, n, i);
			if (minus) n = -n;
		label2:
			t = new TTerm();
			//tofs = GetZStor(1 + 1 + sizeof(short));
			t->Fun = prolog_func::_IntT;
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
			t = new TTerm();
			t->Fun = prolog_func::_RealT;
			t->RR = r;
		}
		else g_compiler->OldError(510);
		t->Op = _const;
		break;
	}
	default: {
		if (!RdVar(D, Kind, -1, &t, nullptr, Vars) && !RdConst(D, &t)) {
			if (IsKeyWordP("length")) t = GetFunOp(D, IntDom, _length, "s", Kind, Vars);
			else if (IsKeyWordP("pos")) t = GetFunOp(D, IntDom, _pos, "sl", Kind, Vars);
			else if (IsKeyWordP("min")) t = GetFunOp(D, IntDom, _min, "ii", Kind, Vars);
			else if (IsKeyWordP("max")) t = GetFunOp(D, IntDom, _max, "ii", Kind, Vars);
			else if (IsKeyWordP("val")) t = GetFunOp(D, IntDom, _val, "s", Kind, Vars);
			else if (IsKeyWordP("copy")) t = GetFunOp(D, StrDom, _copy, "sii", Kind, Vars);
			else if (IsKeyWordP("str")) t = GetFunOp(D, StrDom, _str, "i", Kind, Vars);
			else if (IsKeyWordP("repeatstr")) t = GetFunOp(D, StrDom, _repeatstr, "si", Kind, Vars);
			else if (IsKeyWordP("leadchar")) t = GetFunOp(D, StrDom, _leadchar, "cs", Kind, Vars);
			else if (IsKeyWordP("trailchar")) t = GetFunOp(D, StrDom, _trailchar, "cs", Kind, Vars);
			else if (IsKeyWordP("maxrow")) t = GetFunOp(D, IntDom, _maxrow, "", Kind, Vars);
			else if (IsKeyWordP("maxcol")) t = GetFunOp(D, IntDom, _maxcol, "", Kind, Vars);
			else g_compiler->Error(511);
		}
	}
	}
	return t;
}

TTerm* RdMultExpr(TDomain* D, short Kind, std::vector<TVarDcl*>& Vars)
{
	TTerm* t = RdPrimExpr(D, Kind, Vars);
	while ((D != StrDom) && (D != LongStrDom) && ((Lexem == '*' || Lexem == '/') ||
		(Lexem == _and || Lexem == _or) && (D == IntDom))) {
		BYTE lx = (BYTE)Lexem;
		instr_type op = (instr_type)lx;
		RdLexP();
		t = GetOp2(D, op, t, RdPrimExpr(D, Kind, Vars));
	}
	return t;
}

TTerm* RdAddExpr(TDomain* D, short Kind, std::vector<TVarDcl*>& Vars)
{
	TTerm* t = RdMultExpr(D, Kind, Vars);
	while ((Lexem == '+') || (Lexem == '-') && ((D == IntDom) || (D == RealDom))) {
		BYTE lx = (BYTE)Lexem;
		instr_type op = (instr_type)lx;
		RdLexP();
		t = GetOp2(D, op, t, RdMultExpr(D, Kind, Vars));
	}
	return t;
}

TTerm* RdListTerm(TDomain* D, short Kind, std::vector<TVarDcl*>& Vars)
{
	TTerm* t = nullptr;
	TTerm* t1 = nullptr;
	TTerm* tPrev = nullptr;
	//PtrRec(t).Seg = _Sg;
	if (!RdVar(D, Kind, -1, &t, nullptr, Vars) && !RdConst(D, &t)) {
		if (Lexem != '[') g_compiler->Error(510);
		RdLexP();
		t = nullptr;
		if (Lexem == ']') RdLexP();
		else {
		label1:
			t1 = new TTerm(); // GetZStor(1 + 1 + 2 * 2);
			t1->Fun = prolog_func::_ListT;
			t1->Op = _const;
			t1->Elem = RdTerm(D->ElemDom, Kind, Vars);

			if (t == nullptr) t = t1;
			else tPrev->Next = t1;

			tPrev = t1;
			if (Lexem == ',') { RdLexP(); goto label1; }
			if (Lexem == '|') {
				RdLexP();
				if (!RdVar(D, Kind, -1, &tPrev->Next, nullptr, Vars)) g_compiler->Error(511);
			}
			AcceptP(']');
		}
	}
	if (Lexem == '+') {
		t1 = t;
		t = new TTerm(); // GetZStor(1 + 1 + 2 * 2);
		t->Op = (instr_type)'+';
		RdLexP();
		t->Fun = prolog_func::_ListT;
		t->E1 = t1;
		t->E2 = RdListTerm(D, Kind, Vars);
		WasOp = true;
	}
	return t;
}

TTerm* RdTerm(TDomain* D, short Kind, std::vector<TVarDcl*>& Vars)
{
	TTerm* t = nullptr;
	TFunDcl* f = nullptr;
	WORD i = 0, n = 0;
	BYTE idx = 0;
	bool wo = WasOp; bool wu = WasUnbd;
	WasOp = false; WasUnbd = false;
	switch (D->Typ) {
	case _IntD:
	case _RealD:
	case _StrD:
	case _LongStrD: {
		t = RdAddExpr(D, Kind, Vars);
		break;
	}
	case _ListD:
		t = RdListTerm(D, Kind, Vars);
		break;
	default: {
		if (!RdVar(D, Kind, -1, &t, nullptr, Vars) && !RdConst(D, &t)) {
			TestIdentifP();
			f = GetFunDclByName(D, idx);
			if (f == nullptr) g_compiler->Error(512);
			RdLexP();
			n = f->Arity;
			t = new TTerm(); // ptr(_Sg, GetZStor(1 + 1 + 2 * n));
			t->FunIdx = idx;
			t->Arity = n;
			if (n > 0) {
				AcceptP('(');
				for (i = 0; i <= n - 1; i++) {
					if (i > 0) AcceptP(',');
					t->Arg[i] = RdTerm(f->Arg[i], Kind, Vars);
				}
				AcceptP(')');
			}
		}
		else {
			goto label1;
		}
		break;
	}
	}
	if (WasOp) {
		if (WasUnbd) g_compiler->OldError(540);
		if (Kind == 6) g_compiler->OldError(549);
	}
label1:
	WasOp = wo;
	WasUnbd = wu;
	return t;
}

TDomain* MakeDomain(TDomainTyp domain_type, std::string domain_name)
{
	TDomain* d = new TDomain();
	d->Typ = domain_type;
	d->Name = domain_name;

	Roots->Domains.insert(std::pair(domain_name, d));

	return d;
}

TDomain* GetDomain(bool Create, std::string Nm)
{
	TDomain* d;
	auto it = Roots->Domains.find(Nm);
	if (it == Roots->Domains.end()) d = nullptr;
	else d = it->second;

	if (d == nullptr) {
		if (Nm.substr(0, 2) == "L_") {
			TDomain* d1 = GetOrigDomain(GetDomain(Create, Nm.substr(2)));
			if (d1 == nullptr) {
				g_compiler->Error(517);
				std::string msg = "Exception in Prolog GetDomain " + Nm;
				throw std::exception(msg.c_str());
			}
			Nm = "L_";
			Nm += d1->Name;

			// try to find domain again (maybe it was created few lines ago)
			it = Roots->Domains.find(Nm);
			if (it == Roots->Domains.end()) d = nullptr;
			else d = it->second;

			if (d == nullptr) {
				d = MakeDomain(_ListD, Nm);
				d->ElemDom = d1;
			}
		}
		else if (Create) {
			if ((Nm.length() == 0) || !IsCharUpper2(Nm[0])) g_compiler->Error(514);
			d = MakeDomain(_UndefD, Nm);
		}
	}
	return d;
}

TDomain* RdDomain() /*PDomain*/
{
	TDomain* D = GetDomain(false, LexWord);
	TestIdentifP();
	if (D == nullptr) g_compiler->Error(517);
	RdLexP();
	return GetOrigDomain(D);
}

void RdDomains()
{
	TDomain* d = nullptr;
	TFunDcl* fd = nullptr;
	TDomain* d1 = nullptr;
	std::vector<TDomain*> a;
	std::string nm;
	BYTE n = 0;
	short i = 0;
	//PtrRec(d).Seg = _Sg;
	//PtrRec(fd).Seg = _Sg;
label1:
	TestIdentifP();
	d = GetDomain(true, LexWord);
	if (d->Typ != _UndefD) g_compiler->Error(505);
	RdLexP();
	while (Lexem == ',') {
		RdLexP();
		d->Typ = _RedefD;
		TestIdentifP();
		d->OrigDom = GetDomain(true, LexWord);
		d = d->OrigDom;
		if (d->Typ != _UndefD) g_compiler->OldError(505);
		RdLexP();
	}
	AcceptP('=');
	g_compiler->SkipBlank(false);
	TestIdentifP();
	if (IsCharUpper2(LexWord[1])) {
		d1 = GetDomain(true, LexWord);
		if (d1 == d) g_compiler->Error(505);
		RdLexP();
		d->Typ = _RedefD;
		d->OrigDom = d1;
		goto label4;
	}
	d->Typ = _FunD;
label2:
	if (GetFunDclByName(d, n) != nullptr) g_compiler->Error(505);
	if (IsCharUpper2(LexWord[1])) g_compiler->Error(515);
	nm = LexWord;
	RdLexP();
	n = 0;
	if (Lexem == '(') {
		RdLexP();
	label3:
		TestIdentifP();
		TDomain* newA = GetDomain(true, LexWord);
		a.push_back(newA);
		n++;
		RdLexP();
		if (Lexem == ',') { RdLexP(); goto label3; }
		AcceptP(')');
	}

	fd = new TFunDcl();
	d->FunDcl.push_back(fd);
	
	fd->Name = nm;
	fd->Arity = n;
	fd->Arg = a; // Move(a, fd->Arg, 2 * n);
	if (Lexem == ';') {
		RdLexP();
		TestIdentifP();
		goto label2;
	}
label4:
	if (!(Lexem == 0x1A || Lexem == '#')) goto label1;

	for (std::pair<const std::string, TDomain*>& d : Roots->Domains) {
		switch (d.second->Typ) {
		case _UndefD: {
			SetMsgPar(d.second->Name);
			g_compiler->OldError(516);
			break;
		}
		case _FunD: {
			//fd = d.second->FunDcl;
			//while (fd != nullptr) {
			for (size_t ii = 0; ii < d.second->FunDcl.size(); ii++) {
				fd = d.second->FunDcl[ii];
				for (i = 0; i < fd->Arity; i++) {
					fd->Arg[i] = GetOrigDomain(fd->Arg[i]);
				}
				//fd = fd->pChain;
			}
			break;
		}
		default: {
			// process for other types probably not needed?
		}
		}
		//d = d->pChain;
	}
}

void RdConstants(std::vector<TVarDcl*>& Vars)
{
	// read all variables
	while (true) {
		TDomain* d = RdDomain();
		AcceptP(':');
		// read all values (enums, ...)
		while (true) {
			TestIdentifP();
			if (IsCharUpper2(LexWord[1])) g_compiler->Error(515);
			if (FindConst(LexWord, d) != nullptr) g_compiler->Error(505);
			TConst* p = new TConst();
			p->Name = LexWord;
			RdLexP();
			p->Dom = d;
			AcceptP('=');
			p->Expr = RdTerm(d, 6, Vars);
			p->Expr->Name = p->Name;

			/*if (Roots->Consts == nullptr) Roots->Consts = p;
			else ChainLast(Roots->Consts, p);*/
			Roots->Consts.push_back(p);

			if (Lexem == ',') {
				RdLexP();
				continue;
			}
			break;
		}
		if (!(Lexem == 0x1A || Lexem == '#')) {
			continue;
		}
		break;
	}
}

TPredicate* GetPredicate(const std::string predicate_name)
{
	for (TPredicate* p : Roots->Predicates) {
		if (p->Name == predicate_name) {
			RdLexP();
			return p;
		}
	}

	for (TPredicate* p : ClausePreds) {
		if (p->Name == predicate_name) {
			RdLexP();
			return p;
		}
	}

	return nullptr;
}

TPredicate* RdPredicate(const std::string name) /*PPredicate*/
{
	TPredicate* p = GetPredicate(name);
	if (p == nullptr) g_compiler->Error(513);
	return p;
}

WORD GetOutpMask(TPredicate* P)
{
	return (0xffff >> (16 - P->Arity)) && !P->InpMask;
}

void RdPredicateDcl(bool FromClauses, TDatabase* Db)
{
	TDomain* d = nullptr;
	TPredicate* p = nullptr;
	TScanInf* si = nullptr;
	TFldList* fl = nullptr;
	Instr_proc* ip = nullptr;
	std::string nm; /*PString*/
	short n = 0;
	WORD w = 0, m = 0;
	std::vector<TDomain*> a; /*PDomain*/
	RdbPos pos;
	char typ = '\0';
	WORD bpOfs = 0;
	bool isOutp = false, b = false;
	FieldDescr* f = nullptr;
	RdbD* r = nullptr;
	FrmlElem* z = nullptr;

	BYTE o = 0;
	if (Db != nullptr) o = _DbaseOpt + _CioMaskOpt;
	if (Lexem == '@') {
		RdLexP();
		o = o | _FandCallOpt;
	}
	else if (Db != nullptr) o = o | _PackInpOpt;
	TestIdentifP();
	if (IsCharUpper2(LexWord[1])) g_compiler->Error(518);
	if (GetPredicate(LexWord) != nullptr) g_compiler->OldError(505);
	nm = LexWord;
	if ((o & _FandCallOpt) != 0) {
		if (Db != nullptr) {
			si = new TScanInf();
			si->Name = LexWord;
			CFile = g_compiler->RdFileName();
			if (CFile->typSQLFile) g_compiler->OldError(155);
			si->FD = CFile;
			goto label2;
		}
		else {
			g_compiler->SkipBlank(false);
			if (ForwChar == '[') {
				RdLexP();
				RdLexP();
				g_compiler->TestLex(_quotedstr);
				z = new FrmlElemString(_const, 0); // GetOp(_const, LexWord.length() + 1);
				((FrmlElemString*)z)->S = LexWord;
				pos.rdb = (RdbD*)z;
				pos.i_rec = 0;
				AcceptP(_quotedstr);
				g_compiler->TestLex(']');
			}
			else {
				if (!g_compiler->FindChpt('P', LexWord, false, &pos)) g_compiler->Error(37);
				//pos.rdb = ptr(0, StorStr(LexWord));
				std::string* sLexWord = new std::string(LexWord);
				pos.rdb = (RdbD*)sLexWord;
				pos.i_rec = 0xffff;
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
				f = g_compiler->RdFldName(CFile);
				fl = new TFldList(); // GetZStor(sizeof(TFldList));
				fl->FldD = f; // OPtr(PtrRec(CFile).Seg, f);

				if (si->FL == nullptr) si->FL = fl;
				else ChainLast(si->FL, fl);

				d = nullptr;
				AcceptP('/');
				d = RdDomain();
				switch (f->frml_type) {
				case 'B': {
					if (d != BoolDom) g_compiler->OldError(510);
					break;
				}
				case 'R': {
					if ((d != RealDom) && ((f->field_type != FieldType::FIXED) || (d != IntDom))) g_compiler->OldError(510);
					break;
				}
				default: {
					if (f->field_type == FieldType::TEXT) {
						if ((d != LongStrDom) && ((d->Typ != _FunD) || (d == BoolDom))) g_compiler->OldError(510);
					}
					else if (d != StrDom) g_compiler->OldError(510);
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
		if ((d == LongStrDom) && (Db != nullptr)) g_compiler->OldError(541);
		if (((o & _FandCallOpt) != 0) && (d->Typ == _FunD) && (d != BoolDom)) g_compiler->OldError(528);
	label4:
		a.push_back(d); // a[n] = d;
		n++;
		m = m << 1;
		if (Lexem == ',') {
			if (n == 15) g_compiler->Error(519);
			RdLexP();
			goto label3;
		}
		AcceptP(')');
	}
	p = new TPredicate();

	if (FromClauses) {
		ClausePreds.push_back(p);
	}
	else {
		Roots->Predicates.push_back(p);
	}

	if (Db != nullptr) {
		p->ChainDb = Db->Pred;
		Db->Pred = p;
	}

	p->Name = nm;
	p->Arity = n;
	p->ArgDomains = a; //memcpy(p->Arg, a, 2 * n);
	p->Opt = o;
	p->InpMask = w;
	p->InstSz = 4 * n;

	if ((o & _FandCallOpt) != 0) {
		if ((o & _DbaseOpt) != 0) p->scanInf = si;
		else {
			//ipofs = GetZStor(5 + sizeof(RdbPos) + 2 + n * sizeof(TypAndFrml));
			ip = new Instr_proc(0);
			ip->Kind = PInstrCode::_proc;
			ip->PPos = pos;
			ip->N = n;
			bpOfs = 4;
			for (size_t i = 0; i < n; i++) {
				d = p->ArgDomains[i];

				if (d == RealDom || d == IntDom) typ = 'R';
				else if (d == BoolDom) typ = 'B';
				else typ = 'S';

				isOutp = (w & 1) == 0;
				if (isOutp) {
					z = new FrmlElemLocVar(_getlocvar, 2); // GetOp(_getlocvar, 2);
					//((FrmlElemLocVar*)z)->BPOfs = bpOfs;
					switch (typ) {
					case 'S': bpOfs += sizeof(int); break;
					case 'R': bpOfs += sizeof(double); break;
					default: bpOfs += sizeof(bool); break;
					}
				}
				else {
					switch (typ) {
					case 'R': z = new FrmlElemNumber(_const, 0); /* GetOp(_const, sizeof(double));*/ break;
					case 'B': z = new FrmlElemBool(_const, 0); /* GetOp(_const, sizeof(bool));*/ break;
					default: {
						// StrDom or LongStrDom
						z = new FrmlElemString(_const, 0);
						//if (d == StrDom) z = new FrmlElemString(_const, 0); // GetOp(_const, sizeof(pstring));
						//else {
						//	z = new FrmlElemLocVar(_getlocvar, 2); // GetOp(_getlocvar, 2);
						//	//((FrmlElemLocVar*)z)->BPOfs = bpOfs;
						//	bpOfs += sizeof(int);
						//}
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
			p->instr = ip;
			p->LocVarSz = proc_type::_undefined; // bpOfs;
		}
	}
}

TCommand* GetCommand(TCommandTyp Code, WORD N)
{
	TCommand* c = new TCommand();
	c->Code = Code;
	return c;
}

void RdTermList(TCommand* C, TDomain* D, WORD Kind, std::vector<TVarDcl*>& Vars)
{
	TTerm* t = RdTerm(D, Kind, Vars);
	C->Arg.push_back(t);
}

TCommand* RdCommand(std::vector<TVarDcl*>& Vars) /*PCommand*/
{
	TVarDcl* v = nullptr;
	char cm = '\0'; /*PDomain d;*/
	char op = '\0', fTyp = '\0';
	TCommandTyp code;
	TWriteD* w = nullptr;
	bool nl = false;
	TPredicate* p = nullptr;
	TCommand* c = nullptr;
	LinkD* ld = nullptr;
	FileD* fd = nullptr;
	TDomain* d = nullptr;
	WORD n = 0;
	pstring s;
	short i = 0;

	/*PtrRec(p).Seg = _Sg; c = ptr(_Sg, 0); PtrRec(w).Seg = _Sg; PtrRec(d).Seg = _Sg;
	PtrRec(l).Seg = _Sg;*/
	if (Lexem == '!') {
		RdLexP();
		c = GetCommand(_CutC, 0);
		goto label9;
	}
	else if (Lexem != _identifier) goto label9;
	if (IsUpperIdentif()) {
		v = FindVarDcl(LexWord, Vars);
		if (v == nullptr) v = MakeVarDcl(0, -1, Vars);
		RdLexP();
		d = v->Dom;
		if (v->Bound) {
			switch (Lexem) {
			case '=': op = _equ; break;
			case '<': {
				switch (ForwChar) {
				case '>': { g_compiler->ReadChar(); op = _ne; break; }
				case '=': { g_compiler->ReadChar(); op = _le; break; }
				default: op = _lt; break;
				}
				break;
			}
			case '>': {
				if (ForwChar == '=') { g_compiler->ReadChar(); op = _ge; break; }
				else op = _gt;
				break;
			}
			default: { g_compiler->Error(524); break; }
			}
			if ((d != IntDom) && (d != RealDom) && !(op == _equ || op == _ne)) g_compiler->Error(538);
			RdLexP();
		}
		else {
			if (!v->Used) {
				AcceptP(':');
				d = RdDomain();
				v->Dom = d;
			}
			AcceptP('=');
			op = _assign;
		}
		c = GetCommand(_CompC, 1 + 1 + 2 * 2);
		c->Typ = d->Typ;
		c->E1Idx = v->Idx;
		c->CompOp = op;
		c->E2 = RdTerm(d, 2, Vars);
		if (v->Bound) v->Used = true;
		v->Bound = true;
		goto label9;
	}
	if (IsKeyWordP("fail")) {
		c = GetCommand(_FailC, 0);
		goto label9;
	}
	if (IsKeyWordP("wait")) {
		c = GetCommand(_WaitC, 0);
		goto label9;
	}
	if (IsKeyWordP("trace")) {
		c = GetCommand(_Trace, 2);
		AcceptP('(');
		c->TrcLevel = RdIntegerP();
		goto label8;
	}
	if (IsKeyWordP("error")) {
		AcceptP('(');
		c = GetCommand(_ErrorC, 2 + 2);
		c->MsgNr = RdIntegerP();
		goto label20;
	}
	if (IsKeyWordP("writeln")) { nl = true; goto label1; }
	if (IsKeyWordP("write")) {
		nl = false;
	label1:
		c = GetCommand(_WriteC, 2 + 1);
		c->NL = nl;
		AcceptP('(');
	label2:
		if (Lexem == _quotedstr) {
			w = new TWriteD(); //  GetZStor(3 + 1 + LexWord.length());
			w->IsString = true;
			w->SS = LexWord;
		}
		else {
			w = new TWriteD(); //wofs = GetZStor(3 + 2 + 2);
			TestIdentifP();
			v = FindVarDcl(LexWord, Vars);
			if (v == nullptr) g_compiler->Error(511);
			else if (!v->Bound) g_compiler->Error(509);
			v->Used = true;
			w->Dom = v->Dom;
			w->Idx = v->Idx;
			if ((c->Code == _ErrorC) && (v->Dom != StrDom) &&
				((c->WrD != 0) || (v->Dom != IntDom))) g_compiler->Error(558);
		}
		RdLexP();

		if (c->WrD == nullptr) c->WrD = w;
		else ChainLast(c->WrD, w);

	label20:
		if (Lexem == ',') { RdLexP(); goto label2; }
		goto label8;
	}
	if (copy(LexWord, 1, 6) == "union_") { p = UnionPred; goto label21; }
	if (copy(LexWord, 1, 6) == "minus_") { p = MinusPred; goto label21; }
	if (copy(LexWord, 1, 6) == "inter_") {
		p = InterPred;
	label21:
		pstring str = "L_";
		str += copy(LexWord, 7, 255);
		d = GetDomain(false, str);
		if (d->Typ != _ListD) g_compiler->Error(548);
		RdLexP();
		AcceptP('(');
		c = GetCommand(_PredC, 5 * 2);
		c->Pred = p;
		c->ElemDomain = d;
		RdTermList(c, d, 2, Vars);
		AcceptP(',');
		RdTermList(c, d, 2, Vars);
		AcceptP(',');
		RdTermList(c, d, 1, Vars);
		goto label8;
	}
	if (copy(LexWord, 1, 4) == "mem_") { p = MemPred; goto label22; }
	if (copy(LexWord, 1, 4) == "len_") { p = LenPred; goto label22; }
	if (copy(LexWord, 1, 4) == "inv_") { p = InvPred; goto label22; }
	if (copy(LexWord, 1, 4) == "add_") { p = AddPred; goto label22; }
	if (copy(LexWord, 1, 4) == "del_") {
		p = DelPred;
	label22:
		pstring str = "L_";
		str += copy(LexWord, 5, 255);
		d = GetDomain(false, str);
		if (d->Typ != _ListD) g_compiler->Error(548);
		RdLexP();
		AcceptP('(');
		c = GetCommand(_PredC, 5 * 2);
		c->Pred = p;
		c->ElemDomain = d; /*ListDom*/
		if (p == MemPred) {
			UnbdVarsInTerm = false;
			RdTermList(c, d->ElemDom, 4, Vars);
			if (UnbdVarsInTerm) n = 2;
			else n = 3;
			c->InpMask = n;
			c->OutpMask = ~n;
			AcceptP(',');
			RdTermList(c, d, 2, Vars);
		}
		else {
			if (p == AddPred) {
				RdTermList(c, d->ElemDom, 2, Vars);
				AcceptP(',');
			}
			else if (p == DelPred) {
				RdTermList(c, d->ElemDom, 1, Vars);
				AcceptP(',');
			}
			RdTermList(c, d, 2, Vars);
			AcceptP(',');
			if (p == LenPred) {
				RdTermList(c, IntDom, 1, Vars);
			}
			else {
				RdTermList(c, d, 1, Vars);
			}
		}
		goto label8;
	}
	if (IsKeyWordP("loadlex")) {
		code = _LoadLexC;
		AcceptP('(');
		p = nullptr;
		goto label4;
	}
	if (IsKeyWordP("save")) { code = _SaveC; goto label3; }
	if (IsKeyWordP("consult")) {
		code = _ConsultC;
	label3:
		AcceptP('(');
		TestIdentifP();
		p = (TPredicate*)FindDataBase(LexWord);
		if (p == 0) g_compiler->Error(531);
		RdLexP();
		AcceptP(',');
	label4:
		c = GetCommand(code, 2 + 4 + 4 + 1 + LexWord.length());
		c->DbPred = (TDatabase*)p;
		//Move(LexWord, c->Name, LexWord.length() + 1);
		c->Name = LexWord;
		if (!g_compiler->IsRoleName(false, nullptr, &fd, &ld)) g_compiler->Error(9);
		if (fd->typSQLFile) g_compiler->OldError(155);
		AcceptP('.');
		c->FldD = g_compiler->RdFldName(fd);
		if (c->FldD->field_type != FieldType::TEXT) g_compiler->OldError(537);
	label8:
		AcceptP(')');
	}
label9:
	return c;
}

TCommand* RdPredCommand(TCommandTyp Code, TPredicate* predicate)
{
	TCommand* c = nullptr;
	WORD i = 0, n = 0, w = 0, m = 0, kind = 0, sz = 0, InpMask = 0, OutpMask = 0;
	std::vector<TTerm*> lRoot;
	TDomain* d = nullptr;
	TTerm* t = nullptr;
	TScanInf* si = nullptr;
	XKey* k = nullptr;
	KeyFldD* kf = nullptr;
	TFldList* fl = nullptr;
	BYTE a[256]{ 0 };
	bool IsFandDb = false, inOut = false;
	FieldDescr* f = nullptr;

	TPredicate* p = RdPredicate(LexWord);
	IsFandDb = (p->Opt & (_DbaseOpt + _FandCallOpt)) == _DbaseOpt + _FandCallOpt;
	if (((p->Opt & _DbaseOpt) != _DbaseOpt) && (Code == _AssertC || Code == _RetractC)) g_compiler->OldError(526);
	kind = 1;
	m = 1;
	w = p->InpMask;
	sz = 2 + 2;
	InpMask = 0; OutpMask = 0;
	lRoot.clear();
	if ((p->Opt & _CioMaskOpt) != 0) {
		sz += 4;
		if (Code == _AssertC) w = 0xffff;
		else kind = 4;
	}
	if (p->Arity > 0) {
		AcceptP('(');
		for (i = 0; i < p->Arity; i++) {
			if (i > 0) AcceptP(',');
			UnbdVarsInTerm = false;
			if (kind != 4) {
				if ((w & 1) != 0) kind = 2;
				else kind = 1;
			}
			d = p->ArgDomains[i];

			auto l = RdTerm(d, kind, predicate->VarsCheck);
			lRoot.push_back(l);

			if ((p->Opt & _CioMaskOpt) != 0) {
				if (UnbdVarsInTerm) {
					if (l->Elem != UnderscoreTerm) OutpMask = OutpMask | m;
				}
				else InpMask = InpMask | m;
			}
			m = m << 1;
			w = w >> 1;
		}
		AcceptP(')');
	}
	if ((p->Opt & _BuildInOpt) != 0) {
		switch (p->LocVarSz) {
		case proc_type::_ConcatP: {
			if (!(InpMask >= 3 && InpMask <= 7)) g_compiler->OldError(534);
			break;
		}
		case proc_type::_FandFieldP: {
			if ((InpMask & 1) == 0/*o...*/) g_compiler->OldError(555);
			InpMask = InpMask & 0x3;
			OutpMask = ~InpMask;
			break;
		}
		case proc_type::_FandLinkP: {
			if ((InpMask & 1) == 0/*o...*/) g_compiler->OldError(555);
			InpMask = InpMask & 0x7;
			if (InpMask == 0x7) InpMask = 0x5;
			OutpMask = ~InpMask;
			break;
		}
		}
	}
	/* FAND-find first key file, which is a subset of the input fields */
	i = 0; w = 0;
	if (IsFandDb && (Code != _AssertC)) {
		sz += 10;
		si = p->scanInf;
		CFile = si->FD;
		if (CFile->FF->file_type == FileType::INDEX) {
			k = CFile->Keys[0];
			while (k != nullptr) {
				//kf = k->KFlds; inOut = false;
				while (kf != nullptr) {
					m = 1; n = 0; i++; fl = si->FL;
					while (fl != nullptr) {
						f = fl->FldD;
						fl = fl->pChain;
						if (f == kf->FldD) {
							w = w | m;
							a[i] = n;
							if ((fl != nullptr) && (f == fl->FldD) && ((OutpMask & (m << 1)) != 0)
								&& (f->field_type == FieldType::ALFANUM)) inOut = true;
							goto label1;
						}
						m = m << 1;
						n++;
					} goto label2;
				label1:
					//kf = kf->pChain;
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
	c = GetCommand(Code, sz);
	c->Pred = p;
	c->Arg = lRoot;
	if ((p->Opt & _CioMaskOpt) != 0) {
		c->InpMask = InpMask;
		c->OutpMask = OutpMask;
		if (IsFandDb) {
			c->CompMask = InpMask & ~w;
			if (i > 0) {
				//Move(a, c->ArgI, i);

				c->ArgI[0] = a[0];
				c->KDOfs = k;
			}
		}
	}
	return c;
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
		if (d->Typ != _StrD) g_compiler->Error(510);
		n = LexWord.length() + 1;
		//if (PtrRec(p).Ofs + n >= PTPMaxOfs) Error(527);
		Move(&LexWord[0], &p, n);
		p += n;
		RdLexP();
		break;
	}
	case '$': {
		if (d->Typ != _IntD) g_compiler->Error(510);
		else {
			i = 0;
			while ((ForwChar >= '0' && ForwChar <= '9') || (ForwChar >= 'a' && ForwChar <= 'f') || (ForwChar >= 'A' && ForwChar <= 'F')) {
				i++;
				if (i > 4) g_compiler->Error(3);
				g_compiler->ReadChar();
				s[i] = CurrChar;
			}
			if (i == 0) g_compiler->Error(504);
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
		if (Lexem != _number) g_compiler->Error(525);
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
			*(short*)p = n;
			p += 2;
		}
		else {
			if (d->Typ != _RealD) g_compiler->Error(510);
			if ((Lexem == '.') && isdigit(ForwChar)) {
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
		if (d->Typ != _ListD) g_compiler->Error(510);
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
		if (d->Typ != _FunD) g_compiler->Error(510);
		f = GetFunDclByName(D, idx);
		if (f == nullptr) g_compiler->Error(512);
		*p = (char)idx;
		p++;
		RdLexP();
		n = f->Arity;
		if (n > 0) {
			AcceptP('(');
			for (i = 0; i <= n - 1; i++) {
				if (i > 0) AcceptP(',');
				RdDbTerm(f->Arg[i]);
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
	char A[4000 + 1] {0};
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

void CheckPredicates(std::vector<TPredicate*>& P)
{
	TScanInf* si = nullptr;
	//TPredicate* p = P;
	//while (p != nullptr) {
	for (TPredicate* p : P) {
		if (((p->Opt & (_DbaseOpt + _FandCallOpt + _BuildInOpt)) == 0)
			&& (p->branch.empty() && p->dbBranch == nullptr && p->scanInf == nullptr && p->instr == nullptr)) {
			SetMsgPar(p->Name);
			g_compiler->OldError(522);
		}
		if ((p->Opt & _DbaseOpt) != 0) {
			if ((p->Opt & _FandCallOpt) != 0) {
				si = p->scanInf;
				// not needed -> v_files is actual (not loaded from RDB file, but actually compiled)
				//si->v_files = nullptr;
			}
			else {
				// not needed -> branches and instructions are actual (not loaded from RDB file, but actually compiled)
				//p->dbBranch = nullptr;
				//p->instr = nullptr;
			}
		}
		//p = p->pChain;
	}
}

void RdAutoRecursionHead(TPredicate* p, TBranch* b)
{
	WORD w = 0;
	TTerm* t = nullptr;
	TDomain* d = nullptr;
	TVarDcl* v = nullptr;
	bool isInput = false;
	short i = 0, j = 0, k = 0;
	if (p->Opt != 0) g_compiler->Error(550);
	//PtrRec(c).Seg = _Sg; PtrRec(l).Seg = _Sg; PtrRec(l1).Seg = _Sg;
	//PtrRec(t).Seg = _Sg; PtrRec(d).Seg = _Sg;
	TCommand* c = GetCommand(_AutoC, 2 + 3 + 6 * 2);
	b->Commands.push_back(c);
	p->InstSz += 4;
	c->iWrk = (p->InstSz / 4) - 1;
	w = p->InpMask;
	for (i = 0; i < p->Arity; i++) {
		if ((w & 1) != 0) isInput = true;
		else isInput = false;

		t = new TTerm();
		c->Arg.push_back(t);
		t->Fun = prolog_func::_VarT;
		t->Idx = i;
		t->Bound = isInput;

		// TODO: proc je prvni zaznam "prazdny"?
		TTerm* empty_term = new TTerm();
		b->Heads.push_back(empty_term);
		d = p->ArgDomains[i];
		if (i > 0) AcceptP(',');
		else if (!(d->Typ == _FunD || d->Typ == _ListD)) g_compiler->Error(556);
		if (Lexem == '!') {
			if (i > 0) {
				if (isInput || (d != p->ArgDomains[0]) || (c->iOutp > 0)) g_compiler->Error(551);
				c->iOutp = i;
			}
			else if (!isInput) g_compiler->Error(552);
		}
		else if (TestKeyWordP('_')) {
			if (!isInput) {
				if (d->Typ == _FunD) g_compiler->Error(575); j = 0;
				goto label1;
			}
		}
		else {
			if (!IsUpperIdentif()) g_compiler->Error(511);
			v = FindVarDcl(LexWord, p->VarsCheck);
			if (v == nullptr) v = MakeVarDcl(d, i, p->VarsCheck);
			if (isInput) {
				if (v->Bound) g_compiler->Error(553);
				v->Bound = true;
			}
			else { if (v->Used) g_compiler->Error(553); v->Used = true; }
			if (v->Bound && v->Used) {
				j = v->Idx;
			label1:
				k = c->nPairs;
				(c->nPairs)++;
				if (isInput) {
					c->Pair[k].iInp = i;
					c->Pair[k].iOutp = j;
				}
				else {
					c->Pair[k].iInp = j;
					c->Pair[k].iOutp = i;
				}
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
	char x = '\0';
	RdLexP();
	//PtrRec(c).Seg = _Sg;
	if (IsUpperIdentif()) {
		x = 'a';
		v = FindVarDcl(LexWord, p->VarsCheck);
		if (p->InpMask != (1 << (p->Arity - 1)) - 1) { g_compiler->Error(562); }
		if ((v == nullptr) || v->Bound || (v->Dom->Typ != _ListD)) { g_compiler->Error(561); }
		RdLexP();
		AcceptP('+');
		AcceptP('=');
		v->Bound = true;
		c = GetCommand(_AppPkC, 6);
		c->apIdx = v->Idx;
		c->apTerm = RdTerm(v->Dom, 2, p->VarsCheck);
		b->Commands.push_back(c);
		if (Lexem == ',') {
			RdLexP();
			AcceptPKeyWord("self");
			c = GetCommand(_SelfC, 0);
			goto label2;
		}
		goto label1;
	}
	else if (TestKeyWordP("error")) {
		x = 'e';
		c = GetCommand(_CutC, 0);
	}
	else if (IsKeyWordP("self")) {
		x = 's';
		goto label3;
	}
	else {
		x = 'f';
	label1:
		c = GetCommand(_FailC, 0);
	}
label2:
	b->Commands.push_back(c);
label3:
	b = new TBranch();
	p->branch.push_back(b);
	switch (x) {
	case 'e': {
		b->Commands.push_back(RdCommand(p->VarsCheck));
		break;
	}
	case 'f': {
		if (GetOutpMask(p) != 0) g_compiler->Error(559);
		break;
	}
	case 's': {
		b->Commands.push_back(GetCommand(_SelfC, 0));
		break;
	}
	case 'a': {
		c = GetCommand(_AppUnpkC, 4);
		c->apIdx = v->Idx;
		c->apDom = (TTerm*)v->Dom;
		b->Commands.push_back(c);
		break;
	}
	default: break;
	}
}

void RdClauses()
{
	TBranch* b = nullptr;
	WORD w = 0, m = 0, mp = 0; short i = 0, kind = 0;
	bool iserr = false;
	TDomain* d = nullptr;
	TDomain* dEl = nullptr;
	TTerm* t = nullptr;
	TCommand* c = nullptr;
	//TVarDcl* v = nullptr;
	void* x = nullptr;
	TCommandTyp code;
	bool WasNotC = false;

	while (true) {
		if (Lexem == ':') {
			RdLexP();
			RdPredicateDcl(true, nullptr);
			if (!(Lexem == 0x1A || Lexem == '#')) continue;
			break;
		}

		TestIdentifP();
		TPredicate* p = RdPredicate(LexWord);
		if ((p->Opt & (_FandCallOpt | _BuildInOpt)) != 0) {
			g_compiler->OldError(529);
		}
		if ((p->Opt & _DbaseOpt) != 0) {
			RdDbClause(p);
			if (!(Lexem == 0x1A || Lexem == '#')) continue;
			break;
		}

		//VarDcls = nullptr;
		VarCount = 0; // p->Arity;
		x = Mem1.Mark();
		b = new TBranch();
		p->branch.push_back(b);

		if (p->Arity > 0) {
			AcceptP('(');
			if (Lexem == '!') {
				RdAutoRecursionHead(p, b);
				AcceptP('.');

				//v = VarDcls;
				//while (v != nullptr) {
				for (TVarDcl* v : p->VarsCheck) {
					if (!v->Used || !v->Bound) {
						SetMsgPar(v->Name);
						if (!v->Used) g_compiler->OldError(521);
						else g_compiler->OldError(520);
					}
					//v = v->pChain;
				}

				p->InstSz = MaxW(p->InstSz, 4 * VarCount);
				Mem1.Release(x);

				if (!(Lexem == 0x1A || Lexem == '#')) continue;
				break;

				//goto label4;
			}
			w = p->InpMask; m = 1;
			for (i = 0; i < p->Arity; i++) {
				if (i > 0) AcceptP(',');
				d = p->ArgDomains[i];
				kind = 1;
				if ((w & 1) == 0) kind = 3;

				g_compiler->SkipBlank(false);
				if (IsUpperIdentif() && (ForwChar == ',' || ForwChar == ')') /*solo variable*/) {
					RdVar(d, kind, i, &t, nullptr, p->VarsCheck);
					if (t != nullptr) {
						goto label11;
					}
				}
				else {
					t = nullptr;
				label11:
					if ((w & 1) != 0) b->HeadIMask = b->HeadIMask | m;
					else b->HeadOMask = b->HeadOMask | m;
					if (t == nullptr) {
						t = RdTerm(d, kind, p->VarsCheck);
					}
					if (t != nullptr) {
						b->Heads.push_back(t);
					}
				}
				w = w >> 1;
				m = m << 1;
			}
			AcceptP(')');
		}
		if (Lexem != '.') {
			AcceptP(_assign);
			//label2:
			while (true) {
				WasNotC = false;
				if (IsKeyWordP("self")) {
					c = GetCommand(_SelfC, 0);
					b->Commands.push_back(c);
					break; // goto label4;
				}
				if (IsKeyWordP("not")) { AcceptP('('); WasNotC = true; }
				c = RdCommand(p->VarsCheck);
				if (c == nullptr) {
					if ((Lexem == _identifier) && (copy(LexWord, 1, 4) == "all_")) {
						pstring s1 = "L_";
						s1 += copy(LexWord, 5, 255);
						d = GetDomain(false, s1);
						if (d->Typ != _ListD) g_compiler->Error(548);
						RdLexP();
						AcceptP('(');
						c = RdPredCommand(_AllC, p);
						AcceptP(',');
						c->ElemTerm = RdTerm(d->ElemDom, 2, p->VarsCheck);
						AcceptP(',');
						c->Idx = VarCount;
						VarCount++;
						//TODO: Idx je WORD - RdVar(d, 5, -1, &c->Idx2);
						RdVar(d, 5, -1, nullptr, &c->Idx2, p->VarsCheck);
						AcceptP(')');
					}
					else {
						if (IsKeyWordP("assert")) { code = _AssertC; goto label3; }
						else {
							if (IsKeyWordP("retract")) {
								code = _RetractC;
							label3:
								AcceptP('(');
								c = RdPredCommand(code, p);
								AcceptP(')');
							}
							else c = RdPredCommand(_PredC, p);
						}
					}
				}

				b->Commands.push_back(c);

				if (WasNotC) {
					if (c->Code != _PredC) g_compiler->OldError(546);
					TPredicate* p1 = c->Pred;

					if ((p1->Opt & _CioMaskOpt) != 0) w = c->InpMask;
					else w = p1->InpMask;

					for (TTerm* term : c->Arg) {
						if ((w & 1) == 0) {
							if (t->Fun != prolog_func::_UnderscT) g_compiler->OldError(547);
						}
						w = w >> 1;
					}
					AcceptP(')');
					c->Code = _NotC;
				}
				if (Lexem == ',') {
					RdLexP();
					continue;
				}
				if (Lexem == ';') {
					RdSemicolonClause(p, b);
				}
				break;
			}
		}
		//label4:
		AcceptP('.');

		for (TVarDcl* v : p->VarsCheck) {
			if (!v->Used || !v->Bound) {
				SetMsgPar(v->Name);
				if (!v->Used) g_compiler->OldError(521);
				else g_compiler->OldError(520);
			}
		}

		p->InstSz = MaxW(p->InstSz, 4 * VarCount);
		Mem1.Release(x);

		if (!(Lexem == 0x1A || Lexem == '#')) continue;
		break;
	}

	CheckPredicates(ClausePreds);
	ClausePreds.clear();
}

TPredicate* MakePred(std::string PredName, std::string ArgTyp, proc_type PredKod, WORD PredMask)
{
	TDomain* d = nullptr;
	TPredicate* p = new TPredicate();
	/*if (Roots->Predicates == nullptr) Roots->Predicates = p;
	else ChainLast(Roots->Predicates, p);*/

	p->Name = PredName;
	p->Arity = ArgTyp.length();
	p->LocVarSz = PredKod;

	Roots->Predicates.push_back(p);

	for (size_t i = 0; i < ArgTyp.length(); i++) {
		switch (ArgTyp[i]) {
		case 's': d = StrDom; break;
		case 'l': d = LongStrDom; break;
		case 'i': d = IntDom; break;
		case 'r': d = RealDom; break;
		case 'b': d = BoolDom; break;
		case 'x': d = LLexDom; break;
		default: break;
		}
		p->ArgDomains.push_back(d); // p->Arg[i] = d;
	}

	if (PredMask == 0xffff) {
		p->Opt = _BuildInOpt + _CioMaskOpt;
	}
	else {
		p->Opt = _BuildInOpt;
		p->InpMask = PredMask;
	}
	p->InstSz = ArgTyp.length() * 4;

	return p;
}

void AlignLongStr()
{
}

TProgRoots* ReadProlog(WORD RecNr)
{
	pstring Booln = "Boolean";
	pstring Reell = "Real";
	TPredicate* p = nullptr;
	pstring s;
	RdbPos pos;
	void* p1 = nullptr; void* p2 = nullptr; void* pp1 = nullptr;
	void* pp2 = nullptr; void* pp3 = nullptr;
	int AA = 0;
	WORD result = 0;

	MarkBoth(p1, p2);
	void* cr = CRecPtr;
	if (ProlgCallLevel == 0) {
		FreeMemList = nullptr;
		Mem1.Init(); Mem2.Init(); Mem3.Init();
	}
	else {
		pp1 = Mem1.Mark(); pp2 = Mem2.Mark(); pp3 = Mem3.Mark();
	}
	AlignLongStr();
	ClausePreds.clear();
	Roots = new TProgRoots();
	UnderscoreTerm = new TTerm();
	UnderscoreTerm->Fun = prolog_func::_UnderscT;
	StrDom = MakeDomain(_StrD, "String");
	LongStrDom = MakeDomain(_LongStrD, "LongString");
	IntDom = MakeDomain(_IntD, "Integer");
	RealDom = MakeDomain(_RealD, "Real");
	BoolDom = MakeDomain(_FunD, "Boolean");

	TDomain* d = BoolDom;

	TFunDcl* f = new TFunDcl();
	f->Name = "false";
	d->FunDcl.push_back(f);

	f = new TFunDcl();
	f->Name = "true";
	d->FunDcl.push_back(f);

	p = new TPredicate();
	p->Name = "main";
	Roots->Predicates.push_back(p);
	LexDom = MakeDomain(_FunD, "Lexem");

	//d->pChain = LexDom;

	f = new TFunDcl();
	f->Name = "lex";
	f->Arity = 3;
	f->Arg.push_back(IntDom);
	f->Arg.push_back(IntDom);
	f->Arg.push_back(StrDom);
	d->FunDcl.push_back(f);

	LLexDom = MakeDomain(_ListD, "L_Lexem");
	//d->pChain = LLexDom;
	d->ElemDom = LexDom;
	MemPred = MakePred("mem_?", "ii", proc_type::_MemP, 0xffff);
	MakePred("concat", "sss", proc_type::_ConcatP, 0xffff);
	MakePred("call", "ss", proc_type::_CallP, 3/*ii*/);
	LenPred = MakePred("len_?", "ii", proc_type::_LenP, 1/*io*/);
	InvPred = MakePred("inv_?", "ii", proc_type::_InvP, 1/*io*/);
	AddPred = MakePred("add_?", "iii", proc_type::_AddP, 3/*iio*/);
	DelPred = MakePred("del_?", "iii", proc_type::_DelP, 2/*oio*/);
	UnionPred = MakePred("union_?", "iii", proc_type::_UnionP, 3/*iio*/);
	MinusPred = MakePred("minus_?", "iii", proc_type::_MinusP, 3/*iio*/);
	InterPred = MakePred("inter_?", "iii", proc_type::_InterP, 3/*iio*/);
	MakePred("abbrev", "ss", proc_type::_AbbrevP, 1/*io*/);
	MakePred("fandfile", "ssss", proc_type::_FandFileP, 0/*oooo*/);
	MakePred("fandfield", "sssiiis", proc_type::_FandFieldP, 0xffff);
	MakePred("fandkey", "ssbb", proc_type::_FandKeyP, 1/*iooo*/);
	MakePred("fandkeyfield", "sssbb", proc_type::_FandKeyFieldP, 3/*iiooo*/);
	MakePred("fandlink", "sssssi", proc_type::_FandLinkP, 0xffff);
	MakePred("fandlinkfield", "sss", proc_type::_FandLinkFieldP, 3/*iio*/);
	MakePred("nextlex", "", proc_type::_NextLexP, 0);
	MakePred("getlex", "x", proc_type::_GetLexP, 0/*o*/);
	ResetCompilePars();
	RdLexP();
	while (Lexem != 0x1A) {
		AcceptP('#');
		if (IsKeyWordP("DOMAINS")) {
			RdDomains();
		}
		else if (IsKeyWordP("CONSTANTS")) {
			RdConstants(p->VarsCheck);
		}
		else if (IsKeyWordP("DATABASE")) {
			s[0] = 0;
			if (Lexem == '-') {
				RdLexP();
				TestIdentifP();
				s = LexWord;
				RdLexP();
			}
			TDatabase* db = FindDataBase(s);
			if (db == nullptr) {
				db = new TDatabase();
				db->Name = s;
				Roots->Databases.insert(std::pair(db->Name, db));
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
	//db = Roots->Databases;
	//while (db != nullptr) {
	//	db->SOfs = SaveDb(db, AA);
	//	db = db->pChain;
	//}

	for (const auto& db : Roots->Databases) {
		db.second->SOfs = SaveDb(db.second, AA);
	}

	CheckPredicates(Roots->Predicates);
	//ss->LL = AbsAdr(HeapPtr) - AA;
	if (ProlgCallLevel == 0) ReleaseStore(&p2);
	else {
		Mem1.Release(pp1);
		Mem2.Release(pp2);
		Mem3.Release(pp3);
	}
	// TODO: ulozeni cele pameti (kompilace) do RDB (TTT)
	//if (RecNr != 0) {
	//	CFile = Chpt; CRecPtr = cr;
	//	StoreChptTxt(ChptOldTxt, ss, true);
	//	CFile->WriteRec(RecNr, CRecPtr);
	//	ReleaseStore(p1);
	//}
	return Roots;
}
