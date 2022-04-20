#include "RunProlog.h"

#include <set>
#include "Prolog.h"
#include "RdProlog.h"
#include "TMemory.h"
#include "../cppfand/compile.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"
#include "../cppfand/runproc.h"
#include "../cppfand/runproj.h"
#include "../cppfand/models/Instr.h"
#include "../Editor/OldEditor.h"

const WORD MaxPackedPredLen = 4000;

struct TTerm {
	BYTE Fun = 0;
	BYTE Arity = 0;
	TTerm* Arg[3];
	integer II = 0;
	double RR = 0.0;
	std::string SS;
	longint Pos = 0;
	TTerm* Elem = nullptr;
	TTerm* Next = nullptr;
};

struct TInstance {
	TPredicate* Pred = nullptr; /*PPredicate*/
	TInstance* PrevInst = nullptr;
	TInstance* RetInst = nullptr;
	TCommand* RetCmd = nullptr; /*PCommand*/
	TBranch* NextBranch = nullptr; /*PDbBranch, PFileScan*/
	TBranch* RetBranch = nullptr; /*PBranch*/
	void* StkMark = nullptr;
	longint WMark = 0;
	WORD CallLevel = 0;
	TTerm* Vars[7]{ nullptr };
};

struct TFileScan {
	longint IRec = 0, Count = 0;
};

TInstance* CurrInst = nullptr;
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

/*  T D O M A I N  =========================================================*/
TFunDcl* GetFunDcl(TDomain* D, BYTE I)
{
	TFunDcl* fd = nullptr;
	//WORD fdofs = 0; // absolute fd
	//PtrRec(fd).Seg = _Sg;
	//fdofs = (TDomain*)(ptr(_Sg, D))->FunDcl;
	//while (I > 0) { I--; fdofs = fd->pChain; }
	return fd;
}

/*  T T E R M  =============================================================*/
TTerm* GetIntTerm(integer I)
{
	TTerm* t = (TTerm*)Mem1.Get(1 + sizeof(integer));
	/* !!! with t^ do!!! */
	{ t->Fun = _IntT; t->II = I; }
	return t;
}

TTerm* GetRealTerm(double R)
{
	TTerm* t = (TTerm*)Mem1.Get(1 + sizeof(double));
	/* !!! with t^ do!!! */
	{ t->Fun = _RealT; t->RR = R; }
	return t;
}

TTerm* GetBoolTerm(bool B)
{
	TTerm* t = (TTerm*)Mem1.Get(1 + 1);
	t->Fun = B;
	return t;
}

TTerm* GetStringTerm(pstring S)
{
	TTerm* t = (TTerm*)Mem1.Get(1 + 1 + S.length());
	/* !!! with t^ do!!! */
	{ t->Fun = _StrT; t->SS = S; }
	return t;
}

TTerm* GetLongStrTerm(longint N)
{
	TTerm* t = (TTerm*)Mem1.Get(1 + 4);
	/* !!! with t^ do!!! */
	{ t->Fun = _LongStrT; t->Pos = N; }
	return t;
}

TTerm* GetListTerm(TTerm* aElem, TTerm* aNext)
{
	TTerm* t = (TTerm*)Mem1.Get(1 + 2 * 4);
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
	TTerm* t = (TTerm*)Mem1.Get(1 + 1 + aArity * 4);
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

std::string XXS;
LongStr* RdLongStr(longint Pos)
{
	LongStr* p = new LongStr(2); // GetStore(2);
	WORD l = 0;
	SeekH(WorkHandle, Pos);
	ReadH(WorkHandle, 2, &l);
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

LongStr* RunLSExpr(TPTerm* TOfs);
void RunSExpr(TPTerm* t, std::string* s);
double RunRExpr(TPTerm* TOfs);

integer RunIExpr1(TPTerm* t)
{
	std::string s, s2;
	integer i = 0, err = 0, l = 0;
	LongStr* ss = nullptr;
	switch (t->Op) {
	case _length: { RunSExpr(t->E1, &s); i = s.length(); break; }
	case _val: { RunSExpr(t->E1, &s); val(s, i, err); break; }
	case _pos: {
		RunSExpr(t->E1, &s);
		ss = RunLSExpr(t->E2);
		l = ss->LL;
		i = FindTextE(s, "", ss->A, l);
		if (i > 0) i = i - s.length();
		ReleaseStore(ss);
		break;
	}
	}
	return i;
}

integer RunIExpr(TPTerm* t/*PPTerm*/)
{
	//TPTerm* t = ptr(_Sg, TOfs);
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

double RunRExpr(TPTerm* t/*PPTerm*/)
{
	//TPTerm* t = ptr(_Sg, TOfs);
	if (t->Fun == _VarT) { return CurrInst->Vars[t->Idx]->RR; }
	switch (t->Op) {
	case _const: return t->RR;
	case '+': return RunRExpr(t->E1) + RunRExpr(t->E2);
	case '-': return RunRExpr(t->E1) - RunRExpr(t->E2);
	case '*': return RunRExpr(t->E1) * RunRExpr(t->E2);
	case '/': return RunRExpr(t->E1) / RunRExpr(t->E2);
	case _conv: return RunIExpr(t->E1);
	}
	return 0.0;
}

void RunSExpr1(TPTerm* t, std::string* s)
{
	std::string s2;
	TPTerm* tofs = nullptr; // absolute T
	TPTerm* t1ofs = nullptr;
	WORD l = 0, l2 = 0;
	bool b = false;
	do {
		b = (t->Fun == _StrT) && (t->Op == '+');
		if (b) t1ofs = t->E1;
		else t1ofs = tofs;
		RunSExpr(t1ofs, &s2);
		l2 = MinW(s2.length(), 255 - l);
		Move(&s2[1], &s[l + 1], l2);
		l += l2;
		if (b) tofs = t->E2;
	} while (b);
	s[0] = (char)l;
}

void RunSExpr(TPTerm* t, std::string* s)
{
	//TPTerm* t = nullptr;
	WORD i = 0, n = 0, l = 0;
	LongStr* p = nullptr;
	std::string* q = nullptr;
	//t = ptr(_Sg, TOfs);
	if (t->Fun == _VarT) { q = &CurrInst->Vars[t->Idx]->SS; goto label1; }
	else {
		switch (t->Op) {
		case _const: {
			q = &t->SS;
		label1:
			Move(q, s, q->length() + 1);
			break;
		}
		case '+': RunSExpr1(t, s); break;
		case _conv: {
			p = RunLSExpr(t->E1);
			l = MinW(p->LL, 255);
			s[0] = (char)l;
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
			while ((i <= l) && ((*s)[i] == (char)(t->E1))) i++;
			goto label2;
			break;
		}
		case _trailchar: {
			RunSExpr(t->E2, s);
			l = s->length();
			while ((l > 0) && ((*s)[l] == (char)(t->E1))) l--;
			s[0] = (char)l;
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
		case _str: *s = std::to_string(RunIExpr(t->E1)); break;
		}
	}
}

LongStr* RunLSExpr(TPTerm* t)
{
	//TPTerm* t = ptr(_Sg, TOfs);
	LongStr* p = nullptr;
	LongStr* p2 = nullptr;
	WORD l = 0;
	std::string* s = nullptr;

	if (t->Fun == _VarT) p = RdLongStr(CurrInst->Vars[t->Idx]->Pos);
	else {
		switch (t->Op) {
		case _const: {
			l = t->SS.length();
			p = new LongStr(l + 2); // GetStore(l + 2);
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
			//p = GetStore(257);
			//s = ptr(PtrRec(p).Seg, PtrRec(p).Ofs + 1);
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

bool UnifyVList(TTerm* TT1, TPTerm* T2);

bool UnifyTermsCV(TTerm* T1, TPTerm* T2/*PPTerm*/)
{
	integer i = 0;
	LongStr* p = nullptr;
	LongStr* p2 = nullptr;
	auto result = true;
	if (T2 == nullptr) { if (T1 != nullptr) return false; }
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
		case _IntT: result = T1->II == RunIExpr(T2); break;
		case _RealT: result = T1->RR == RunRExpr(T2); break;
		case _StrT: {
			if (T2->Op == _const) result = T1->SS == T2->SS;
			else {
				RunSExpr(T2, &XXS);
				result = T1->SS == XXS;
			}
			break;
		}
		case _LongStrT: {
			p = RdLongStr(T1->Pos);
			p2 = RunLSExpr(T2);
			result = EquLongStr(p, p2);
			ReleaseStore(p);
			break;
		}
		case _ListT: {
			while (T2->Op == '+') {
				if (!UnifyVList(T1, T2->E1)) goto label1;
				T2 = T2->E2;
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
	return result;
}

bool UnifyVList(TTerm* TT1, TPTerm* T2)
{
	WORD* t2ofs = &T2->Fun; // absolute T2
	TTerm* t = nullptr; TTerm* t1 = nullptr;
	t1 = TT1;
	auto result = false;
	while (t2ofs != nullptr)
		switch (T2->Fun) {
		case _VarT: {
			if (T2->Bound) {
				t = CurrInst->Vars[T2->Idx];
				while (t != nullptr) {
					if ((t1 == nullptr) || !UnifyTermsCC(t1->Elem, t->Elem)) return result;
					t = t->Next;
					t1 = t1->Next;
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
			if ((t1 == nullptr) || !UnifyTermsCV(t1->Elem, T2->Elem)) return result;
			t1 = t1->Next;
			t2ofs = (WORD*)&T2->Next;
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
		t1 = (TTerm*)Mem1.Get(1 + 2 * 4);
		t1->Fun = _ListT;
		t1->Elem = T->Elem;
		if (root == nullptr) root = t1;
		else prev->Next = t1;
		prev = t1;
		T = T->Next;
	} while (T != nullptr);
	return root;
}

TTerm* CopyVList(TPTerm* T, bool Cpy);

TTerm* CopyTerm(TPTerm* t/*PPTerm*/)
{
	TTerm* t1 = nullptr;
	WORD* tofs = (WORD*)t; // absolute t
	TTerm* t2 = nullptr;
	integer i = 0;
	LongStr* p = nullptr;
	if (t == nullptr) { return nullptr; }
	//t = ptr(_Sg, TOff);
	switch (t->Fun) {
	case _IntT: return GetIntTerm(RunIExpr((TPTerm*)tofs)); break;
	case _RealT: return GetRealTerm(RunRExpr((TPTerm*)tofs)); break;
	case _StrT: {
		if (t->Op == _const) return GetStringTerm(t->SS);
		else {
			RunSExpr(t, &XXS);
			return GetStringTerm(XXS);
		}
		break;
	}
	case _LongStrT: {
		p = RunLSExpr(t);
		auto result = GetLongStrTerm(WrLongStr(p));
		ReleaseStore(p);
		return result;
		break;
	}
	case _VarT: return CurrInst->Vars[t->Idx]; break;
	case _ListT: {
		t2 = nullptr;
		while (t->Op == '+') {
			ChainList(t2, CopyVList(t->E1, true));
			tofs = (WORD*)&t->E2;
		}
		ChainList(t2, CopyVList(t, false));
		return t2;
		break;
	}
	default: {
		t2 = GetFunTerm(t->Fun, t->Arity);
		for (i = 0; i <= integer(t->Arity) - 1; i++)
			t2->Arg[i] = CopyTerm(t->Arg[i]);
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
		t1 = (TTerm*)Mem1.Get(1 + 2 * 4);
		t1->Fun = _ListT;
		t1->Elem = CopyTerm(T->Elem);
	}
	if (root == nullptr) root = t1;
	else prev->Next = t1;
	if ((T->Fun != _VarT)) {
		prev = t1;
		tofs = *(WORD*)&T->Next;
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

	//if (PtrRec(p).Ofs >= PTPMaxOfs) RunError(1527);
label1:
	if (T == nullptr /* [] */) { *(WORD*)p = 0; p += 2; }
	else {
		switch (T->Fun) {
		case _IntT: { *(integer*)p = T->II; p += 2; break; }
		case _RealT: { *(double*)p = T->RR; p += sizeof(double); break; }
		case _StrT: {
			n = T->SS.length() + 1;
			//if (PtrRec(p).Ofs + n >= PTPMaxOfs) RunError(1527);
			//Move(t->SS, p, n);
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

void PackTermV(TPTerm* T/*PPTerm*/);

WORD PackVList(TPTerm* T)
{
	TPTerm* tofs = nullptr; // absolute T
	WORD n = 0;
	TTerm* t1 = nullptr;
	do {
		if (T->Fun == _VarT) {
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
	} while (tofs != nullptr);
label1:
	return n;
}

void PackTermV(TPTerm* T/*PPTerm*/)
{
	char* p = PackedTermPtr;
	integer i = 0; WORD n = 0; WORD* wp = nullptr;
	//TPTerm* T = nullptr;
	TPTerm* tofs = nullptr; // absolute t
	TTerm* t1 = nullptr;
	//if (PtrRec(p).Ofs >= PTPMaxOfs) RunError(1527);
	//T = ptr(_Sg, TOff);
label1:
	if (tofs == 0 /* [] */) { *(WORD*)p = 0; p += 2; }
	else {
		switch (T->Fun) {
		case _VarT: PackTermC(CurrInst->Vars[T->Idx]); break;
		case _IntT: { *(integer*)p = RunIExpr((TPTerm*)tofs); p += 2; break; }
		case _RealT: { *(double*)p = RunRExpr((TPTerm*)tofs); p += sizeof(double); break; }
		case _StrT: {
			RunSExpr((TPTerm*)tofs, &XXS);
			n = XXS.length() + 1;
			//if (PtrRec(p).Ofs + n >= PTPMaxOfs) RunError(1527);
			Move(&XXS, p, n);
			p += n;
			break;
		}
		case _LongStrT: RunError(1543); break;
		case _ListT: {
			wp = (WORD*)p;
			p += 2;
			*wp = 0;
			while (T->Op == '+') {
				*wp += PackVList(T->E1);
				tofs = T->E2;
			}
			*wp += PackVList(T);
			break;
		}
		default: {
			*p = char(T->Fun);
			p++;
			for (i = 0; i <= integer(T->Arity) - 1; i++) PackTermV(T->Arg[i]);
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
	//PTPMaxOfs = ofs(A) + MaxPackedPredLen - 2;
	PackTermC(T);
	//n = PtrRec(pt).Ofs - ofs(A);
	//s = GetStore(2 + n);
	s->LL = n;
	Move(A, s->A, n);
	return s;
}

TTerm* UnpackTerm(TDomain* D)
{
	char* p = PackedTermPtr;
	integer i = 0; WORD n = 0; char* q = nullptr;
	TTerm* t = nullptr; TTerm* tPrev = nullptr;
	TTerm* t1 = nullptr; TFunDcl* f = nullptr;
	switch (D->Typ) {
	case _IntD: { t = GetIntTerm(*(integer*)p); p += 2; break; }
	case _RealD: { t = GetRealTerm(*(double*)p); p += sizeof(double); break; }
	case _StrD: { t = GetStringTerm(*(pstring*)p); p += *p + 1; break; }
	case _LongStrD: RunError(1543); break;
	case _ListD: {
		n = *(WORD*)p;
		p += 2;
		t = nullptr;
		while (n > 0) {
			t1 = GetListTerm(UnpackTerm(D->ElemDom), nullptr);
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
		for (i = 0; i <= integer(f->Arity) - 1; i++) t->Arg[i] = UnpackTerm((TDomain*)f->Arg[i]);
		break;
	}
	}
	return t;
}
char* PrintPackedTerm(char* p, TDomain* D)
{
	integer i = 0, n = 0;
	TFunDcl* f = nullptr;
	switch (D->Typ) {
	case _IntD: { printf("%i", *(integer*)p); p += 2; break; }
	case _RealD: { printf("%f", *(double*)p); p += sizeof(double); break; }
	case _StrD: { printf("'%s'", ((pstring*)p)->c_str()); p += *p + 1; break; }
	case _ListD: {
		printf("[");
		n = *(WORD*)p;
		p += 2;
		for (i = 1; i <= n; i++) {
			if (i > 1) printf(",");
			p = PrintPackedTerm(p, D->ElemDom);
		}
		printf("]");
		break;
	}
	default: {
		f = GetFunDcl(D, *p);
		p++;
		printf("%s", f->Name.c_str());
		if (f->Arity > 0) {
			printf("(");
			for (i = 1; i <= f->Arity; i++) {
				if (i > 1) printf(",");
				p = PrintPackedTerm(p, (TDomain*)f->Arg[i - 1]);
			}
			printf(")");
		}
	}
	}
	return p;
}

void PrintPackedPred(char* Q, TPredicate* POfs/*PPredicate*/)
{
	integer i = 0, n = 0;
	TPredicate* p = POfs;
	printf("CALL assert(%s", p->Name.c_str());
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

void PrintTerm(TTerm* T, TDomain* DOfs)
{
	TFunDcl* fd = nullptr;
	WORD i = 0; LongStr* p = nullptr;
	TDomain* d = nullptr;

	if (T == nullptr) printf("[]");
	else {
		switch (T->Fun) {
		case _IntT: printf("%i", T->II); break;
		case _RealT: printf("%f", T->RR); break;
		case _StrT: printf("'%s'", T->SS.c_str()); break;
		case _LongStrT: {
			p = RdLongStr(T->Pos);
			printf("'");
			for (i = 1; i <= p->LL; i++) printf("%c", p->A[i]);
			printf("'");
			ReleaseStore(p);
			break;
		}
		case _ListT: {
			printf("[");
			d = DOfs;
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
			printf("%s", fd->Name.c_str());
			if (T->Arity == 0) return;
			break;
		}
		}
	}
	printf("(");
	for (i = 0; i <= T->Arity - 1; i++) {
		if (i > 0) printf(",");
		PrintTerm(T->Arg[i], (TDomain*)fd->Arg[i]);
	}
	printf(")");
}

WORD LenDbEntry(LongStr* S, integer Arity)
{
	WORD n = 0; integer i = 0;
	for (i = 1; i <= Arity; i++) {
		n += S->LL + 2;
		//PtrRec(S).Ofs += S->LL + 2;
	}
	return n;
}

LongStr* SaveDb(TDatabase* DbOfs,/*PDatabase*/ longint AA)
{
	WORD n = 0, arity = 0; longint l = 0;
	LongStr* s = nullptr;
	char* q = nullptr;
	bool x = false;
	TPredicate* p = nullptr;
	TPredicate* pofs = nullptr; // absolute p
	TDbBranch* b = nullptr;
	TDatabase* db = nullptr;
	//s = GetStore(2);
	db = DbOfs;
	p = db->Pred;
	x = AA != 0;
	while (pofs != nullptr) {
		if ((p->Opt & _FandCallOpt) == 0) {
			arity = p->Arity;
			if (!x) {
				StoreStr(p->Name);
				*(BYTE*)(GetStore(1)) = arity;
			}
			b = (TDbBranch*)p->Branch;
			while (b != nullptr) {
				n = LenDbEntry((LongStr*)b->LL, arity);
				//q = GetStore(n + 1);
				//if (x && (AbsAdr(HeapPtr) - AA > MaxLStrLen)) OldError(544);
				q[0] = 1;
				Move(&b->LL, &q[1], n);
				b = (TDbBranch*)b->pChain;
			}
			*(BYTE*)(GetStore(1)) = 0;
		}
		pofs = p->ChainDb;
	}
	//l = AbsAdr(HeapPtr) - AbsAdr(s) - 2;
	s->LL = l;
	if (l > MaxLStrLen) {
		SetMsgPar(db->Name);
		RunError(1532);
	}
	return s;
}

void ConsultDb(LongStr* S, TDatabase* DbOfs/*PDatabase*/)
{
	WORD n = 0; char* q = nullptr;
	TPredicate* p = nullptr;
	TPredicate* pofs = nullptr; // absolute p
	TDbBranch* b = nullptr; TDatabase* db = nullptr;
	q = (char*)(S->A);
	db = DbOfs;
	p = db->Pred;
	while (pofs != nullptr) {
		if ((p->Opt & _FandCallOpt) == 0) {
			//if (PtrRec(S).Seg != _Sg) {
			//	if (*(pstring*)q != *(pstring*)(ptr(_Sg, p->Name))) goto label1;
			//	q += *q + 1;
			//	if (*q != p->Arity) goto label1;
			//	q++;
			//}
			while (q[0] == 1) {
				q++;
				n = LenDbEntry((LongStr*)q, p->Arity);
				//b = Mem3.Alloc(n + 4);
				//ChainLast<TBranch>(p->Branch, b);
				//Move(q[0], b->LL, n);
				q += n;
			}
			if (q[0] != 0) goto label1;
			q++;
		}
		pofs = p->ChainDb;
	}
	if (S->LL != AbsAdr(q) - AbsAdr(S) - 2) {
	label1:
		SetMsgPar(db->Name);
		RunError(1533);
	}
}

bool Vokal(char C)
{
	std::set<BYTE> charset = { 0x61, 0xA0, 0x84, 0x65, 0x82, 0x88, 0x69, 0xA1, 0x6F, 0xA2, 0x93, 0x94, 0x75, 0xA3, 0x96, 0x81, 0x79, 0x98 };
	char kam = CurrToKamen(C);
	return (charset.count(kam) > 0);
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
	RdbD* r;
	if (FD == nullptr) { r = CRdb; FD = r->FD; }
	else r = FD->ChptPos.R; /*not .RDB*/
label1:
	FD = (FileD*)FD->pChain;
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
	} while (!((fd == nullptr) || EquUpCase(fd->Name, FDName)));
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
	XKey* k = nullptr; KeyFldD* kf = nullptr;
	TTerm* t1 = nullptr; TTerm* t2 = nullptr; TTerm* tprev = nullptr;
	TTerm* t = nullptr; TTerm* root = nullptr;
	LinkD* ld = nullptr; pstring mask;
	integer i = 0, err = 0;
	TCommand* c = nullptr; bool b = false;
	TInstance* q = nullptr; RdbPos pos;

	/* !!! with CurrInst^ do!!! */
	{
		c = CurrInst->RetCmd;
		w = c->InpMask;
		switch (((TPredicate*)&CurrInst->Pred)->LocVarSz) {
		case _NextLexP: if (LexemList != nullptr) LexemList = LexemList->Next; break;
		case _GetLexP: CurrInst->Vars[0] = LexemList; break;
		case _ConcatP: {
			switch (w) {
			case 7/*iii*/: { if (CurrInst->Vars[0]->SS + CurrInst->Vars[1]->SS != CurrInst->Vars[2]->SS) goto label1; break; }
			case 3/*iio*/: { CurrInst->Vars[2] = GetStringTerm(CurrInst->Vars[0]->SS + CurrInst->Vars[1]->SS); break; }
			case 5/*ioi*/: {
				l = CurrInst->Vars[0]->SS.length();
				if (CurrInst->Vars[0]->SS != CurrInst->Vars[2]->SS.substr(1, l)) goto label1;
				CurrInst->Vars[1] = GetStringTerm(copy(CurrInst->Vars[2]->SS, l + 1, 255));
				break;
			}
			case 6/*oii*/: {
				l = CurrInst->Vars[1]->SS.length();
				l2 = CurrInst->Vars[2]->SS.length();
				if (CurrInst->Vars[1]->SS != CurrInst->Vars[2]->SS.substr(l2 - l + 1, l)) {
				label1:
					CurrInst->NextBranch = nullptr;
					return false;
				}
				CurrInst->Vars[0] = GetStringTerm(copy(CurrInst->Vars[2]->SS, 1, l2 - l));
				break;
			}
			case 4/*ooi*/: {
				n = WORD(CurrInst->NextBranch);
				s = CurrInst->Vars[2]->SS;
				if (n == 0) n = s.length();
				if (n == 0) goto label1;
				n--;
				CurrInst->Vars[0] = GetStringTerm(copy(s, 1, s.length() - n));
				CurrInst->Vars[1] = GetStringTerm(copy(s, s.length() - n + 1, n));
				WORD(NextBranch) = n;
				break;
			}
			}
			break;
		}
		case _MemP: {
			switch (w) {
			case 3/*ii*/: {
				t1 = CurrInst->Vars[0]; t2 = CurrInst->Vars[1];
				while (t2 != nullptr) {
					if (UnifyTermsCC(t1, t2->Elem)) goto label3;
					t2 = t2->Next;
				}
				goto label1;
				break;
			}
			case 2/*oi*/: {
				t2 = (TTerm*)CurrInst->NextBranch;
				if (t2 == nullptr) {
					t2 = CurrInst->Vars[1];
					if (t2 == nullptr) goto label1;
				}
				CurrInst->Vars[0] = t2->Elem;
				CurrInst->NextBranch = (TBranch*)t2->Next;
				break;
			}
			}
		}
		case _FandFileP: {
			fd = (FileD*)CurrInst->NextBranch;
			if (fd == nullptr) {
				fd = NextFD(nullptr);
				if (fd == nullptr) goto label1;
			}
			CurrInst->Vars[0] = GetStringTerm(fd->Name);
			s[0] = 0;
			switch (fd->Typ) {
			case '6': if (fd->IsSQLFile) s = "SQL"; break;
			case 'X': s = 'X'; break;
			case 'D': s = "DBF"; break;
			case '8': s = "DTA"; break;
			}
			CurrInst->Vars[1] = GetStringTerm(s);
			CurrInst->Vars[2] = GetStringTerm(fd->ChptPos.R->FD->Name);
			CFile = fd;
			SetCPathVol();
			CurrInst->Vars[3] = GetStringTerm(CPath);
			CurrInst->NextBranch = (TBranch*)NextFD(fd);
			break;
		}
		case _FandFieldP: {
			f = (FieldDescr*)CurrInst->NextBranch;
			if (f == nullptr) {
				fd = FindFD(CurrInst->Vars[0]->SS);
				if (fd == nullptr) goto label1;
				f = fd->FldD.front();
			}
			if (w == 3/*ii*/) {
				std::string tmpSS = CurrInst->Vars[1]->SS;
				while ((f != nullptr) && !EquUpCase(f->Name, tmpSS)) f = (FieldDescr*)f->pChain;
				if (f == nullptr) goto label1;
			}
			else CurrInst->Vars[1] = GetStringTerm(f->Name);
			CurrInst->Vars[2] = GetStringTerm(f->Typ);
			m = 0; l = f->L;
			if (f->Typ == 'F') {
				m = f->M;
				l--;
				if (m > 0) l -= (m + 1);
			}
			CurrInst->Vars[3] = GetIntTerm(l);
			CurrInst->Vars[4] = GetIntTerm(m);
			m = f->Flg;
			if ((f->Typ == 'N' || f->Typ == 'A')) m = m | (f->M << 4);
			CurrInst->Vars[5] = GetIntTerm(m);
			//if ((f->Flg & f_Mask) != 0) mask = FieldDMask(f);
			if ((f->Flg & f_Mask) != 0) mask = f->Mask;
			else mask = "";
			CurrInst->Vars[6] = GetStringTerm(mask);
			if (w == 3) CurrInst->NextBranch = nullptr;
			else CurrInst->NextBranch = (TBranch*)f->pChain;
			break;
		}
		case _FandKeyP: {
			k = (XKey*)CurrInst->NextBranch;
			if (k == nullptr) {
				fd = FindFD(CurrInst->Vars[0]->SS);
				if (fd == nullptr) goto label1;
				//k = fd->Keys;
				if (k == nullptr) goto label1;
			}
			CurrInst->Vars[1] = GetStringTerm(Pound(k->Alias));
			CurrInst->Vars[2] = GetBoolTerm(k->Intervaltest);
			CurrInst->Vars[3] = GetBoolTerm(k->Duplic);
			CurrInst->NextBranch = (TBranch*)(k->Chain);
			break;
		}
		case _FandLinkP: {
			//ld = (LinkD*)CurrInst->NextBranch;
			//if (ld == nullptr) {
			//	ld = LinkDRoot;
			//	while ((ld != nullptr) && !EquUpCase(ld->FromFD->Name, CurrInst->Vars[0]->SS)) {
			//		ld = ld->pChain;
			//	}
			//	if (ld == nullptr) goto label1;
			//}
			//fd = ld->FromFD;
			//if (w == 3/*ii*/) {
			//	while ((ld != nullptr) && ((fd != ld->FromFD) ||
			//		!EquUpCase(ld->RoleName, CurrInst->Vars[1]->SS))) ld = ld->pChain;
			//	if (ld == nullptr) goto label1;
			//	CurrInst->Vars[2] = GetStringTerm(ld->ToFD->Name);
			//}
			//else if (w == 5/*ioi*/) {
			//	while ((ld != nullptr) && ((fd != ld->FromFD) ||
			//		!EquUpCase(ld->ToFD->Name, CurrInst->Vars[2]->SS))) ld = ld->pChain;
			//	if (ld == nullptr) goto label1;
			//	CurrInst->Vars[1] = GetStringTerm(ld->RoleName);
			//}
			//else {
			//	CurrInst->Vars[1] = GetStringTerm(ld->RoleName);
			//	CurrInst->Vars[2] = GetStringTerm(ld->ToFD->Name);
			//}
			//CurrInst->Vars[3] = GetStringTerm(Pound(ld->ToKey->Alias));
			//s[0] = 0;
			//k = fd->Keys;
			//while (k != nullptr) {
			//	if (k->IndexRoot == ld->IndexRoot) s = k->Alias;
			//	k = k->pChain;
			//}
			//CurrInst->Vars[4] = GetStringTerm(Pound(s));
			//n = ld->MemberRef;
			//if (ld->IndexRoot != 0) n += 4;
			//CurrInst->Vars[5] = GetIntTerm(n);
			//if (w == 3) ld = nullptr;
			//else
			//	do {
			//		ld = ld->pChain;
			//	} while (!((ld == nullptr) || (ld->FromFD == fd)));
			//	CurrInst->NextBranch = (TBranch*)ld;
				break;
		}
		case _FandKeyFieldP: {
			kf = KeyFldDPtr(CurrInst->NextBranch);
			if (kf == nullptr) {
				fd = FindFD(CurrInst->Vars[0]->SS);
				if (fd == nullptr) goto label1;
				//k = fd->Keys;
				while ((k != nullptr) && !EquUpCase(Pound(k->Alias), CurrInst->Vars[1]->SS)) k = k->Chain;
				if (k == nullptr) goto label1; kf = k->KFlds;
			}
			CurrInst->Vars[2] = GetStringTerm(kf->FldD->Name);
			CurrInst->Vars[3] = GetBoolTerm(kf->CompLex);
			CurrInst->Vars[4] = GetBoolTerm(kf->Descend);
			CurrInst->NextBranch = (TBranch*)kf->pChain;
			break;
		}
		case _FandLinkFieldP: {
			/*kf = KeyFldDPtr(CurrInst->NextBranch);
			if (kf == nullptr) {
				ld = LinkDRoot;
				while ((ld != nullptr) && (
					!EquUpCase(ld->FromFD->Name, CurrInst->Vars[0]->SS) ||
					!EquUpCase(ld->RoleName, CurrInst->Vars[1]->SS))) ld = ld->pChain;
				if (ld == nullptr) goto label1;
				kf = ld->Args;
			}
			CurrInst->Vars[2] = GetStringTerm(kf->FldD->Name);
			CurrInst->NextBranch = (TBranch*)kf->pChain;*/
			break;
		}
		case _LenP: {
			t1 = CurrInst->Vars[0]; n = 0;
			while (t1 != nullptr) { n++; t1 = t1->Next; }
			CurrInst->Vars[1] = GetIntTerm(n);
			break;
		}
		case _InvP: {
			t1 = CurrInst->Vars[0]; t2 = nullptr;
			while (t1 != nullptr) {
				t2 = GetListTerm(t1->Elem, t2);
				t1 = t1->Next;
			}
			CurrInst->Vars[1] = t2;
			break;
		}
		case _AddP: {
			t1 = CurrInst->Vars[0]; t2 = CurrInst->Vars[1]; CurrInst->Vars[2] = t2;
			while (t2 != nullptr) {
				if (UnifyTermsCC(t1, t2->Elem)) goto label3;
				t2 = t2->Next;
			}
			CurrInst->Vars[2] = GetListTerm(t1, CurrInst->Vars[1]);
			break;
		}
		case _DelP: {
			q = CurrInst;
			t1 = (TTerm*)CurrInst->NextBranch;
			if (t1 == nullptr) t1 = CurrInst->Vars[1];
		label2:
			if (t1 == nullptr) goto label1;
			CurrInst->Vars[0] = t1->Elem;
			CurrInst = CurrInst->RetInst;
			b = UnifyTermsCV(t1->Elem, c->Arg->Elem);
			CurrInst = q;
			if (b) {
				root = nullptr;
				t = CurrInst->Vars[1];
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
				CurrInst->Vars[2] = root;
				CurrInst->NextBranch = (TBranch*)t1;
				goto label3;
			}
			Mem1.Release(q->StkMark); t1 = t1->Next; goto label2;
			break;
		}
		case _UnionP: {
			t1 = CopyCList(CurrInst->Vars[0]);
			root = nullptr;
			t2 = CurrInst->Vars[1];
			while (t2 != nullptr) {
				if (!FindInCList(t2->Elem, t1)) {
					t = GetListTerm(t2->Elem, nullptr);
					if (root == nullptr) root = t;
					else tprev->Next = t;
					tprev = t;
				}
				t2 = t2->Next;
			}
			ChainList(t1, root);
			CurrInst->Vars[2] = t1;
			break;
		}
		case _MinusP: {
			root = nullptr; t1 = CurrInst->Vars[0]; t2 = CurrInst->Vars[1];
			while (t1 != nullptr) {
				if (!FindInCList(t1->Elem, t2)) {
					t = GetListTerm(t1->Elem, nullptr);
					if (root == nullptr) root = t;
					else tprev->Next = t;
					tprev = t;
				}
				t1 = t1->Next;
			}
			CurrInst->Vars[2] = root;
			break;
		}
		case _InterP: {
			root = nullptr;
			t1 = CurrInst->Vars[0]; t2 = CurrInst->Vars[1];
			while (t1 != nullptr) {
				if (FindInCList(t1->Elem, t2)) {
					t = GetListTerm(t1->Elem, nullptr);
					if (root == nullptr) root = t;
					else tprev->Next = t;
					tprev = t;
				}
				t1 = t1->Next;
			}
			CurrInst->Vars[2] = root;
			break;
		}
		case _AbbrevP: { CurrInst->Vars[1] = GetStringTerm(Abbrev(CurrInst->Vars[0]->SS)); break; }
		case _CallP: {
			if (!FindChpt('L', CurrInst->Vars[0]->SS, false, &pos)) {
				SetMsgPar(CurrInst->Vars[0]->SS);
				RunError(1554);
			}
			RunProlog(&pos, &CurrInst->Vars[1]->SS);
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

void LoadLex(LongStr* S)
{
	WORD l = 0, i = 0, n = 0;
	char* p = nullptr;
	integer typ = 0; pstring x;
	TTerm* t = nullptr;
	TTerm* tPrev = nullptr;
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
	RdbD* r = CRdb;
	while (r != nullptr) {
		CFile = r->FD;
		while (CFile != nullptr) {
			if (EquUpCase(Name, CFile->Name)) return;
			CFile = (FileD*)CFile->pChain;
		}
		r = r->ChainBack;
	}
	if (EquUpCase(Name, "CATALOG")) CFile = CatFD;
	else RunError(1539);
}

void RetractDbEntry(TInstance* Q, TPredicate* POfs/*PPredicate*/, TDbBranch* B)
{
	TDbBranch* b1 = nullptr;
	TPredicate* p = nullptr;
	p = POfs;
	b1 = (TDbBranch*)(&p->Branch);
label1:
	if (b1->pChain != nullptr)
		if (b1->pChain = B) {
			b1->pChain = B->pChain;
			while ((Q != nullptr)) {
				if (Q->NextBranch == (void*)B) Q->NextBranch = (TBranch*)B->pChain;
				Q = Q->PrevInst;
			}
		}
		else { b1 = (TDbBranch*)b1->pChain; goto label1; }
	Mem3.Free(B, LenDbEntry((LongStr*)&B->LL, p->Arity) + 4);
}

void AppendPackedTerm(TCommand* C)
{
	char* pt = PackedTermPtr;
	TDbBranch* b = nullptr;
	char A[MaxPackedPredLen + 1]{ '\0' };
	WORD n = 0; // absolute A
	pt = &A[3];
	//PTPMaxOfs = ofs(A) + MaxPackedPredLen - 2;
	PackTermV(C->apTerm);
	//n = PtrRec(pt).Ofs - ofs(A);
	//b = Mem3.Alloc(4 + n);
	Move(A, &b->LL, n);
	//ChainLast(CurrInst->Vars[C->apIdx], b);
}

void UnpackAppendedTerms(TCommand* C)
{
	TDbBranch* b = nullptr; TDbBranch* b1 = nullptr;
	TTerm* t = nullptr; TTerm* pt = nullptr;
	pt = CurrInst->Vars[C->apIdx];
	b = (TDbBranch*)(pt);
	pt = nullptr;
	while (b != nullptr) {
		PackedTermPtr = (char*)b->A;
		t = UnpackTerm((TDomain*)C->apDom);
		ChainList(pt, t);
		b1 = b; b = (TDbBranch*)b->pChain;
		Mem3.Free(b1, b1->LL + 4);
	}
}

bool RunCommand(TCommand* COff/*PCommand*/)
{
	integer i1 = 0, i2 = 0;
	double r1 = 0.0, r2 = 0.0; char res = '\0';
	WORD i = 0;
	TTerm* t = nullptr;
	TWriteD* w = nullptr;
	TWriteD* wofs = nullptr; // absolute w
	TCommand* c = nullptr;
	WORD cofs = 0; // absolute c
	void* p1 = nullptr;
	longint n = 0; LongStr* s = nullptr;
	LockMode md;

	c = COff;
	switch (c->Code) {
	case _WriteC: {
		w = c->WrD;
		while (wofs != nullptr) {
			if (w->IsString) printf("%s", w->SS.c_str());
			else PrintTerm(CurrInst->Vars[w->Idx], w->Dom);
			wofs = (TWriteD*)w->pChain;
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
				r2 = RunRExpr(c->E2);
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
			//PtrRec(c->FldD).Seg = PtrRec(CFile).Seg;
		}
		if (c->Code == _SaveC) {
			md = NewLMode(WrMode);
			if (!LinkLastRec(CFile, n, true)) IncNRecs(1);
			DelTFld(c->FldD);
			s = SaveDb(c->DbPred, 0);
			LongS_(c->FldD, s);
			WriteRec(CFile, CFile->NRecs, CRecPtr);
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
		i1 = -1; i = 1; w = c->WrD;
		while (wofs != 0) {
			if (w->IsString) { MsgPar[i] = w->SS; i++; }
			else {
				t = CurrInst->Vars[w->Idx];
				if (w->Dom->Typ == _IntD) i1 = t->II;
				else { MsgPar[i] = t->SS; i++; };
			}
			wofs = (TWriteD*)w->pChain;
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
	Instr_proc* pd = nullptr;
	ProcStkD* oldBP = nullptr;
	ProcStkD* ps = nullptr;
	WORD i = 0, n = 0, w = 0;
	TTerm* t = nullptr;
	char* pp = (char*)ps;
	LongStr* s = nullptr;
	TDomain* d = nullptr;
	TDomain* dofs = nullptr; // absolute d
	void* pt = PackedTermPtr;
	pstring* ss = nullptr;

	p = CurrInst->Pred;
	//PtrRec(d).Seg = _Sg;
	pd = (Instr_proc*)p->Branch;
	//ps = GetZStore(p->LocVarSz);
	w = p->InpMask;
	//if (PtrRec(pd->Pos.R).Seg == 0) {
	//	PtrRec(pd->Pos.R).Seg = _Sg;
	//	if (pd->Pos.IRec = 0xffff)
	//		if (!FindChpt('P', (pstring*)(pd->Pos.R), false, pd->Pos)) RunError(1037);
	//}
	for (i = 1; i <= p->Arity; i++) /* !!! with pd->TArg[i] do!!! */
	{
		auto ta = &pd->TArg[i];
		//PtrRec(Frml).Seg = _Sg;
		dofs = p->Arg[i - 1];
		t = CurrInst->Vars[i - 1];
		if ((w & 1) != 0) {
			switch (ta->FTyp) {
			case 'R': {
				if (t->Fun == _IntT) ((FrmlElem2*)ta->Frml)->R = t->II;
				else ((FrmlElem2*)ta->Frml)->R = t->RR;
				break;
			}
			case 'B': {
				((FrmlElem5*)ta->Frml)->B = bool(t->Fun);
				break;
			}
			default: {
				if (ta->Frml->Op == _const) ((FrmlElem4*)ta->Frml)->S = t->SS;
				else {
					if (d->Typ == _LongStrD) s = RdLongStr(t->Pos);
					else s = GetPackedTerm(t);
					//*(longint*)(pp + ((FrmlElem18*)ta->Frml)->BPOfs) = TWork.Store(s);
					ReleaseStore(s);
				}
				break;
			}
			}
		}
		w = w >> 1;
	}
	//ps->ChainBack = MyBP;
	//oldBP = MyBP;
	//SetMyBP(ps);
	CallProcedure(pd);
	w = p->InpMask;
	for (i = 1; i <= p->Arity; i++) {
		/* !!! with pd->TArg[i] do with CurrInst^ do  do!!! */
		auto ta = &pd->TArg[i];
		//fs = p->Arg[i - 1];
		if (ta->Frml->Op == _getlocvar) {
			switch (ta->FTyp) {
			case 'S': {
				if ((w & 1) == 0) {
					if (d->Typ == _StrD) CurrInst->Vars[i - 1] = GetStringTerm(RunShortStr(ta->Frml));
					else {
						s = RunLongStr(ta->Frml);
						if (d->Typ == _LongStrD) CurrInst->Vars[i - 1] = GetLongStrTerm(WrLongStr(s));
						else { PackedTermPtr = s->A; CurrInst->Vars[i - 1] = UnpackTerm(dofs); }
						ReleaseStore(s);
					}
				}
				//TWork.Delete(LongintPtr(Ptr(Seg(MyBP), Ofs(MyBP) + ta->Frml->BPOfs)));
				break;
			}
			case 'R': {
				if (d->Typ == _IntD) CurrInst->Vars[i - 1] = GetIntTerm(RunInt(ta->Frml));
				else CurrInst->Vars[i - 1] = GetRealTerm(RunReal(ta->Frml));
				break;
			}
			default: {
				CurrInst->Vars[i - 1] = GetBoolTerm(RunBool(ta->Frml));
				break;
			}
			}
		}
		w = w >> 1;
	}
	//SetMyBP(oldBP);
	ReleaseStore(ps);
}

TScanInf* SiCFile(TScanInf* SiOfs)
{
	TScanInf* si = nullptr;
	TFldList* fl = nullptr;
	TFldList* flofs = 0; // absolute fl
	si = SiOfs;
	auto result = si;
	CFile = si->FD;
	if (CFile != nullptr) return result;
	SetCFile(si->Name);
	si->FD = CFile;
	fl = si->FL;
	while (flofs != nullptr) {
		fl->FldD = (FieldDescr*)CFile;
		flofs = (TFldList*)fl->pChain;
	}
	return result;
}

void AssertFand(TPredicate* P, TCommand* C)
{
	TFldList* fl = nullptr;
	TFldList* flofs = nullptr; // absolute fl 
	TTermList* l = nullptr;
	TTermList* lofs = nullptr; // absolute l
	TDomain* d = nullptr;
	TDomain* dofs = nullptr; // absolute d
	FieldDescr* f = nullptr;
	LockMode md;
	TTerm* t = nullptr;
	TScanInf* si = nullptr;
	WORD i = 0;
	LongStr* s = nullptr;

	si = SiCFile((TScanInf*)P->Branch);
	md = NewLMode(CrMode);
	CRecPtr = GetRecSpace();
	ZeroAllFlds();
	//PtrRec(d).Seg = _Sg;
	fl = si->FL;
	l = C->Arg;
	i = 0;
	if (Trace()) printf("CALL assert(%s(", P->Name.c_str());
	while (flofs != nullptr) {
		f = fl->FldD;
		if ((f->Flg & f_Stored) != 0) {
			t = CopyTerm(l->Elem);
			dofs = P->Arg[i];
			if (Trace()) {
				if (i > 0) printf(",");
				PrintTerm(t, dofs);
			}
			switch (f->FrmlTyp) {
			case 'B': B_(f, bool(t->Fun)); break;
			case 'R': {
				if (t->Fun == _IntT) R_(f, t->II);
				else R_(f, t->RR);
				break;
			}
			default: {
				if (f->Typ == 'T') {
					if (d->Typ == _LongStrD) s = RdLongStr(t->Pos);
					else s = GetPackedTerm(t);
					LongS_(f, s);
					ReleaseStore(s);
				}
				else S_(f, t->SS);
				break;
			}
			}
			flofs = (TFldList*)fl->pChain;
			lofs = (TTermList*)l->pChain;
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
		else WriteRec(CFile, CFile->NRecs, CRecPtr);
	}
	OldLMode(md);
	ReleaseStore(CRecPtr);
}

TFileScan* GetScan(TScanInf* SIOfs, TCommand* C, TInstance* Q)
{
	TFldList* fl = nullptr;
	WORD flofs = 0; // absolute fl
	XKey* k = nullptr;
	WORD kofs = 0; // absolute k
	KeyFldD* kf = nullptr; FieldDescr* f = nullptr;
	XString xx; WORD i = 0;
	TTerm* t = nullptr; double r = 0.0; LongStr* s = nullptr; pstring* ss = nullptr;
	LockMode md; longint n = 0; bool b = false;

	TScanInf* si = SiCFile(SIOfs);
	TFileScan* fs = (TFileScan*)Mem1.Get(sizeof(TFileScan));
	md = NewLMode(RdMode);
	k = nullptr;
	if (C->KDOfs != nullptr) k = C->KDOfs;
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
		t = Q->Vars[C->ArgI[i]];
		switch (f->FrmlTyp) {
		case 'R': {
			if (t->Fun == _IntT) r = t->II;
			else r = t->RR;
			xx.StoreReal(r, kf);
			break;
		}
		case 'B': xx.StoreBool(bool(t->Fun), kf); break;
		default: {
			if (t->Fun == _StrT) xx.StoreStr(t->SS, kf);
			else {
				if (t->Fun == _LongStrT) s = RdLongStr(t->Pos);
				else s = GetPackedTerm(t);
				//ss = ptr(PtrRec(s).Seg, PtrRec(s).Ofs + 1);
				ss[0] = char(MinW(s->LL, 255));
				xx.StoreStr(*ss, kf);
				ReleaseStore(s);
			}
			break;
		}
		}
		kf = (KeyFldD*)kf->pChain;
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

std::string _MyS(FieldDescr* F)
{
	if (F->Typ == 'A') {
		if (F->M == LeftJust) return OldTrailChar(' ', _ShortS(F));
		else return LeadChar(' ', _ShortS(F));
	}
	return _ShortS(F);
}

bool ScanFile(TInstance* Q)
{
	TPredicate* p = nullptr;
	TPredicate* pofs = nullptr; // absolute p
	TCommand* c = nullptr;
	TCommand* cofs = nullptr; // absolute c
	TFldList* fl = nullptr;
	TFldList* flofs = nullptr; // absolute fl
	FieldDescr* f = nullptr; TScanInf* si = nullptr;
	WORD w = 0; integer i = 0;
	TTerm* t = nullptr; double r = 0.0;
	LongStr* s = nullptr;
	pstring ss; bool b = false;
	TDomain* d = nullptr;
	TDomain* dofs = nullptr; // absolute d
	void* pt = PackedTermPtr;
	XKey* k = nullptr;
	XKey* kofs = nullptr; // absolute k
	LockMode md, md1;
	TFileScan* fs = nullptr; TFileScan* fs1 = nullptr;
	longint RecNr = 0; XString xx;

	auto result = false;
	fs = (TFileScan*)CurrInst->NextBranch;
	if (fs == nullptr) return result;
	p = CurrInst->Pred;
	c = CurrInst->RetCmd;
	//PtrRec(d).Seg = _Sg;
	si = (TScanInf*)p->Branch;
	//PtrRec(fl).Seg = _Sg;
	CFile = si->FD;
	CRecPtr = GetRecSpace();
	md = NewLMode(RdMode);
	k = nullptr;
	if (c->KDOfs != nullptr) k = c->KDOfs;
label1:
	if (k == nullptr) {
		do {
			RecNr = fs->IRec;
			if (RecNr > CFile->NRecs) {
				CurrInst->NextBranch = nullptr;
				goto label2;
			}
			ReadRec(CFile, RecNr, CRecPtr);
			(fs->IRec)++;
		} while (DeletedFlag());
		if (fs->IRec > CFile->NRecs) CurrInst->NextBranch = nullptr;
	}
	else {
		if ((fs->Count == 0) || (fs->IRec > k->NRecs())) {
			CurrInst->NextBranch = nullptr;
			goto label2;
		}
		RecNr = k->NrToRecNr(fs->IRec);
		ReadRec(CFile, RecNr, CRecPtr);
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
				if (t->Fun == _IntT) { if (r != t->II) goto label1; }
				else if (r != t->RR) goto label1;
				break;
			}
			default: {
				if (f->Typ == 'T') {
					dofs = p->Arg[i];
					if (d->Typ == _LongStrD) s = RdLongStr(t->Pos);
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
		flofs = (TFldList*)fl->pChain;
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
			flofs = (TFldList*)fl->pChain;
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
			if (CFile->Typ == 'X') { DeleteXRec(RecNr, true); fs->IRec--; }
			else DeleteRec(RecNr);
			OldLMode(md1);
		}
	label2:
		OldLMode(md);
		ReleaseStore(CRecPtr);
	}
	return result;
}

void SaveLMode()
{
	CFile->ExLMode = CFile->LMode;
}

void SetOldLMode()
{
	OldLMode(CFile->ExLMode);
}

void TraceCall(TInstance* Q, BYTE X)
{
	WORD i = 0, w = 0;
	TPredicate* p = nullptr;
	TCommand* c = nullptr;
	TDomain* d = nullptr;
	TDomain* dofs = nullptr; // absolute d

	p = Q->Pred;
	c = Q->RetCmd;
	//PtrRec(d).Seg = _Sg;

	if (X == 1) printf("CALL ");
	else
		if (c->Code == _AllC) printf("MEMBER ");
		else printf("RETURN ");
	switch (c->Code) {
	case _RetractC: printf("retract(");  break;
	case _NotC: printf("not("); break;
	case _AllC: printf("all_XX("); break;
	}
	printf("%s", p->Name.c_str());
	if (p->Arity > 0) {
		printf("(");
		if ((p->Opt & _CioMaskOpt) != 0) w = c->InpMask;
		else w = p->InpMask;
		for (i = 0; i <= p->Arity - 1; i++) {
			if (i > 0) printf(",");
			dofs = p->Arg[i];
			if ((w & 1) == X) {
				if ((X == 1) && ((p->Opt & _PackInpOpt) != 0))
					PrintPackedTerm((char*)(Q->Vars[i]) + 2, dofs);
				else {
					if ((p->Opt & _BuildInOpt) != 0)
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
	BYTE i = 0; bool wasCall = false; TTerm* t = nullptr; TTerm* Arg[1]{ nullptr };
};

bool AutoRecursion(TInstance* q, TPredicate* p, TCommand* c)
{
	integer i = 0, j = 0, i2 = 0, iOutp = 0, sz = 0, arity = 0;
	TAutoR* w = nullptr; TTerm* t = nullptr; TTerm* t1 = nullptr; TFunDcl* f = nullptr;
	TDomain* d = nullptr;
	TDomain* dofs = nullptr; // absolute d

	d = p->Arg[0];
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
		w = (TAutoR*)Mem2.Get(sizeof(TAutoR) + 4 * f->Arity);
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
		if (!EquArea(t->Arg, w->Arg, sz)) {
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

void RunProlog(RdbPos* Pos, std::string* PredName)
{
	TInstance* q = nullptr; TInstance* q1 = nullptr; TInstance* TopInst = nullptr;
	TDbBranch* b1 = nullptr;
	WORD w = 0, n = 0; integer i = 0;
	LongStr* s = nullptr; char* pt = PackedTermPtr;
	char A[MaxPackedPredLen + 1]{ '\0' };
	WORD* wp = nullptr;
	void* pp = nullptr; void* pp1 = nullptr; void* pp2 = nullptr;
	void* pp3 = nullptr; void* pm1 = nullptr; void* pm2 = nullptr;
	LongStr* ss = nullptr; longint WMark = 0;
	TPredicate* p = nullptr; TPredicate* p1 = nullptr; TPredicate* pofs = nullptr; // absolute p
	TCommand* c = nullptr; TCommand* cofs = nullptr; // absolute c
	TBranch* b = nullptr; TBranch* bofs = nullptr; // absolute b
	TDbBranch* bd = nullptr; // absolute b
	TTermList* l = nullptr; TTermList* lofs = nullptr; // absolute l
	TDatabase* db = nullptr;
	TDatabase* dbofs = nullptr; // absolute db
	TTerm* t = nullptr; TProgRoots* Roots = nullptr;
	RdbD* ChptLRdb = nullptr;
	WORD oldSg = 0; TInstance* oldCurrInst = nullptr;
	WORD tl = 0, cl = 0;
	ExitRecord er;

	ProlgCallLevel++;
	//NewExit(Ovr, er); 
	//goto label7; 
	LastExitCode = 1;
	//oldSg = _Sg;
	oldCurrInst = CurrInst;
	ForAllFDs(SaveLMode);
	WMark = MaxWSize;
	if (ProlgCallLevel == 1) {
		MarkBoth(pm1, pm2);
		FreeMemList = nullptr;
		Mem1.Init(); Mem2.Init(); Mem3.Init();
		TrcLevel = 0;
		CallLevel = 0;
	}
	else {
		MarkStore(pp);
		pp1 = Mem1.Mark(); pp2 = Mem2.Mark(); pp3 = Mem3.Mark();
		tl = TrcLevel;
		cl = CallLevel;
	}
	if (Pos->IRec == 0) {
		SetInpLongStr(RunLongStr((FrmlElem*)Pos->R), true);
		WORD segment = ReadProlog(0);
		ChptLRdb = CRdb;
	}
	else {
		ChptLRdb = Pos->R;
		CFile = ChptLRdb->FD;
		CRecPtr = GetRecSpace();
		ReadRec(CFile, Pos->IRec, CRecPtr);
		AlignLongStr();
		//_Sg = PtrRec(HeapPtr).Seg + 1;
		ss = _LongS(ChptOldTxt);
		if (ChptLRdb->Encrypted) CodingLongStr(ss);
		std::string code = std::string(ss->A, ss->LL);
		printf("");
	}
	//Roots = ptr(_Sg, 0);
//
//	db = Roots->Databases;
//	while (dbofs != nullptr) {
//		ConsultDb(db->SOfs, dbofs);
//		dbofs = (TDatabase*)db->pChain;
//	}
//	TopInst = nullptr;
//	CurrInst = nullptr;
//	//c = ptr(_Sg, 0);
//	//b = ptr(_Sg, 0);
//	//l = ptr(_Sg, 0);
//	p = Roots->Predicates; /* main */
//	if (PredName != nullptr) {
//		while ((pofs != nullptr) && (p->Name != *PredName)) pofs = (TPredicate*)p->pChain;
//		if ((pofs == nullptr) || (p->Arity != 0)) {
//			SetMsgPar(Pos->R->FD->Name, *PredName);
//			RunError(1545);
//		}
//	}
//label1:
//	/* new instance remember prev. inst,branch,cmd */
//	q = (TInstance*)Mem2.Get(p->InstSz + (sizeof(TInstance) - 7 * 4));
//	q->Pred = pofs;
//	q->PrevInst = TopInst;
//	TopInst = q;
//	q->RetInst = CurrInst;
//	q->RetBranch = bofs;
//	q->RetCmd = cofs;
//	if (TrcLevel != 0) {
//		CallLevel = CurrInst->CallLevel + 1;
//		q->CallLevel = CallLevel;
//	}
//	/* copy input parameters */
//	b = p->Branch; i = 0;
//	if ((p->Opt & _CioMaskOpt) != 0) w = c->InpMask;
//	else w = p->InpMask;
//	while (lofs != 0) {
//		if ((w & 1) != 0) {
//			if ((p->Opt & _PackInpOpt) != 0) {
//				pt = (char*)A;
//				//PTPMaxOfs = ofs(A) + MaxPackedPredLen - 2;
//				PackTermV(l->Elem);
//				//n = PtrRec(pt).Ofs - ofs(A);
//				s = (LongStr*)Mem1.Get(2 + n);
//				q->Vars[i] = (TTerm*)s;
//				s->LL = n;
//				Move(A, s->A, n);
//			}
//			else q->Vars[i] = CopyTerm(l->Elem);
//		}
//		i++;
//		lofs = (TTermList*)l->pChain;
//		w = w >> 1;
//	}
//	if ((p->Opt & (_FandCallOpt + _DbaseOpt)) == _FandCallOpt + _DbaseOpt)
//		q->NextBranch = (TBranch*)GetScan((TScanInf*)bofs, c, q);
//	if (Trace()) TraceCall(q, 1);
//	q->StkMark = Mem1.Mark();
//	q->WMark = MaxWSize;
//	CurrInst = q;
//	if ((p->Opt & (_FandCallOpt + _DbaseOpt)) == _FandCallOpt) {
//		CallFandProc();
//		goto label4;
//	}
//label2:
//	/*        branch       / redo /        */
//	if ((p->Opt & _BuildInOpt) != 0)             /* build-in predicates */
//		if (RunBuildIn()) goto label4;
//		else goto label5;
//	if ((p->Opt & _DbaseOpt) != 0) {         /* database predicates */
//		if ((p->Opt & _FandCallOpt) != 0)
//			if (ScanFile(TopInst)) goto label4;
//			else goto label5;
//		if (bd == nullptr) goto label5;
//		cofs = q->RetCmd;
//	label21:
//		s = (LongStr*)bd->LL;
//		w = c->InpMask;
//		for (i = 0; i <= integer(p->Arity) - 1; i++) {
//			if (((w & 1) != 0) && !EquLongStr(LongStrPtr(q->Vars[i]), s)) {
//				bd = (TDbBranch*)bd->pChain;
//				if (bd == nullptr) { q->NextBranch = nullptr; goto label5; }
//				goto label21;
//			}
//			//inc(PtrRec(s).Ofs, s->LL + 2);
//			w = w >> 1;
//		}
//	label22:
//		q->NextBranch = (TBranch*)bd->pChain;
//		s = (LongStr*)bd->LL;
//		w = c->OutpMask;
//		for (i = 0; i <= integer(p->Arity) - 1; i++)
//		{ /* unpack db outp.parameters */
//			if ((w & 1) != 0) { pt = (char*)s->A; q->Vars[i] = UnpackTerm(p->Arg[i]); }
//			//PtrRec(s).Ofs += s->LL + 2;
//			w = w >> 1;
//		}
//		if (c->Code == _RetractC) RetractDbEntry(TopInst, pofs, bd);
//		goto label4;
//	}
//	//PtrRec(b).Seg = _Sg;
//label23:
//	/* normal unify branch head predicates */
//	q->NextBranch = (TBranch*)b->pChain;
//	i = 0; lofs = b->Head; w = b->HeadIMask;
//	while (lofs != nullptr) {
//		if (((w & 1) != 0) && !UnifyTermsCV(q->Vars[i], l->Elem)) {
//			bofs = (TBranch*)b->pChain;
//			if (bofs == nullptr) goto label5;
//			goto label23;
//		}
//		i++;
//		lofs = (TTermList*)l->pChain;
//		w = w >> 1;
//	}
//	/* execute all commands */
//	cofs = b->Cmd;
//	while (cofs != 0) {
//		switch (c->Code) {
//		case _PredC:
//		case _RetractC:
//		case _NotC: {
//		label24:
//			pofs = c->Pred; lofs = c->Arg; goto label1;
//			break;
//		}
//		case _AllC: { q->Vars[c->Idx] = nullptr; goto label24; break; }
//		case _CutC: {
//			q->NextBranch = nullptr;
//			while (TopInst != q) {
//				q1 = TopInst->PrevInst;
//				Mem2.Release(TopInst);
//				TopInst = q1;
//			}
//			break;
//		}
//		case _FailC: goto label5; break;
//		case _Trace: {
//			TrcLevel = c->TrcLevel;
//			if (TrcLevel != 0) {
//				TrcLevel++; CallLevel = 1; q->CallLevel = 1;
//			}
//			else { CallLevel = 0; q->CallLevel = 0; }
//			break;
//		}
//		case _AssertC: {
//			p1 = c->Pred;
//			if ((p1->Opt & _FandCallOpt) != 0) AssertFand(p1, c);
//			else {
//				lofs = c->Arg;
//				pt = (char*)A;
//				//PTPMaxOfs = ofs(A) + MaxPackedPredLen - 2;
//				while (lofs != nullptr) {
//					wp = (WORD*)pt;
//					pt += 2;
//					PackTermV(l->Elem);
//					//*wp = PtrRec(pt).Ofs - PtrRec(wp).Ofs - 2;
//					lofs = (TTermList*)l->pChain;
//				}
//				//n = PtrRec(pt).Ofs - Ofs(A);
//				b1 = (TDbBranch*)Mem3.Alloc(4 + n);
//				Move(A, &b1->LL, n);
//				ChainLast(p1->Branch, b1);
//				if (Trace()) PrintPackedPred(A, c->Pred);
//			}
//			break;
//		}
//		case _AutoC:
//		label25:
//			if (AutoRecursion(q, p, c)) { lofs = c->Arg; goto label1; }
//			break;
//		case _SelfC:
//		{
//			if (TopInst != q) {
//				q1 = TopInst;
//				while (q1->PrevInst != q) q1 = q1->PrevInst;
//				Mem2.Release(q1);
//				TopInst = q;
//			}
//			Mem1.Release(q->StkMark);
//			MaxWSize = q->WMark;
//			pofs = q->Pred;
//			b = p->Branch;
//			goto label6;
//			break;
//		}
//		default: if (!RunCommand(cofs)) goto label5; break;
//		}
//		/*       resume command   */
//	label3:
//		cofs = (TCommand*)c->pChain;
//	}
//	/*           copy output parameters */
//	i = 0; lofs = b->Head; w = b->HeadOMask;
//	while (lofs != nullptr) {
//		if ((w & 1) != 0) q->Vars[i] = CopyTerm(l->Elem);
//		i++;
//		lofs = (TTermList*)l->pChain;
//		w = w >> 1;
//	}
//	/*       called predicate finished   */
//label4:
//	cofs = q->RetCmd;
//	if (c->Code == _NotC) { TopInst = q->PrevInst; goto label5; }
//label41:
//	if (Trace()) TraceCall(q, 0);
//	/*      unify output with caller terms */
//	b = q->RetBranch;
//	pofs = q->Pred;
//	q1 = q;
//	q = q->RetInst;
//	if (q == nullptr) { EdBreak = 0; LastExitCode = 0; goto label8; }
//	CurrInst = q;
//	if ((p->Opt & _CioMaskOpt) != 0) w = c->OutpMask;
//	else w = !p->InpMask;
//	i = 0;
//	lofs = c->Arg;
//	while (lofs != nullptr) {
//		if (((w & 1) == 1) && !UnifyTermsCV(q1->Vars[i], l->Elem)) goto label5;
//		i++;
//		lofs = (TTermList*)l->pChain;
//		w = w >> 1;
//	}
//	/*  return to caller;  */
//	if (c->Code == _AllC) {
//		ChainList(q->Vars[c->Idx], GetListTerm(CopyTerm((TPTerm*)c->Elem), nullptr));
//		q1 = TopInst;
//		while (q1 != q) {
//			q1->StkMark = Mem1.Mark();
//			q1->WMark = MaxWSize;
//			q1 = q1->PrevInst;
//		}
//		goto label5;
//	}
//	if ((q1->NextBranch == nullptr) && (q1 == TopInst)) {
//		TopInst = q1->PrevInst;
//		Mem2.Release(q1);
//	}
//	SetCallLevel(q->CallLevel);
//	if (c->Code == _AutoC) goto label25;
//	else goto label3;
//
//	/*---------------------------------  backtracking  ---------------------------*/
//label5:
//	q1 = nullptr; q = TopInst;
//	while ((q != nullptr) && (q->NextBranch == nullptr) &&
//		!(q->RetCmd->Code == _NotC || q->RetCmd->Code == _AllC))
//	{
//		q1 = q;
//		q = q->PrevInst;
//	}
//	if (q == nullptr) {
//		if (Trace()) { printf("FAIL"); WaitC(); }
//		EdBreak = 1; LastExitCode = 0; goto label8;
//	}
//	Mem1.Release(q->StkMark);
//	MaxWSize = q->WMark;
//	if (q->NextBranch == nullptr) {
//		q1 = q;
//		q = q1->RetInst;
//		b = q1->RetBranch;
//		cofs = q1->RetCmd;
//		CurrInst = q;
//		TopInst = q1->PrevInst;
//		Mem2.Release(q1);
//		if (c->Code == _NotC) {
//			if (Trace()) { printf("FAIL%c%cRETURN not()", 0x0D, 0x0A); WaitC(); }
//		}
//		else {
//			q->Vars[c->Idx2] = q->Vars[c->Idx];
//			q->Vars[c->Idx] = nullptr;
//			if (Trace()) {
//				printf("RETURN all_()"); WaitC();
//			}
//		}
//		SetCallLevel(q->CallLevel);
//		pofs = q->Pred;
//		goto label3;
//	}
//	if (Trace()) { printf("FAIL"); WaitC(); }
//	TopInst = q;
//	CurrInst = TopInst;
//	b = q->NextBranch;
//	pofs = q->Pred;
//	SetCallLevel(q->CallLevel);
//	if (q1 != nullptr) Mem2.Release(q1);
//label6:
//	if (Trace()) { printf("REDO %s", p->Name.c_str()); WaitC(); }
//	goto label2;
//
//	/*--------------------------  } of program  ------------------------------*/
//label7:
//	EdBreak = 2;
//label8:
//	RestoreExit(er);
//	/*writeln(AbsAdr(HeapPtr)-AbsAdr(pm1),'/',AbsAdr(pm2)-AbsAdr(Stack2Ptr)); */
//	//_Sg = oldSg;
//	CurrInst = oldCurrInst;
//	ForAllFDs(SetOldLMode);
//	MaxWSize = WMark;
//	if (ProlgCallLevel == 1) ReleaseBoth(pm1, pm2);
//	else {
//		ReleaseStore(pp);
//		Mem1.Release(pp1);
//		Mem2.Release(pp2);
//		Mem3.Release(pp3);
//		TrcLevel = tl;
//		CallLevel = cl;
//	}
//	ProlgCallLevel--;
}
