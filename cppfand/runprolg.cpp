#include "runprolg.h"
#include "runproj.h"

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

void ChainLst(void* Root, WORD NewOfs); // assembler
WORD OOfs(void* p);
WORD GetZStor(WORD Sz);
void* OPtr(WORD Sg, void* p);
WORD StorStr(pstring S); // assembler

/*  L E X A N A L  =========================================================*/
bool IsCharUpper2(char C);
bool IsUpperIdentif();
void RdLex();
void TestIdentif();
void Accept(char X);
bool TestKeyWord(pstring s);
bool IsKeyWord(pstring s);
void AcceptKeyWord(pstring s);
integer RdInteger();

/*  T D O M A I N  =========================================================*/
TFunDcl* GetFunDclByName(WORD D, BYTE& I);
WORD GetOrigDomain(WORD D);

/*  T D A T A B A S E  =====================================================*/
WORD FindDataBase(const pstring S);

/*  T P R O G R A M  =======================================================*/
WORD FindConst(WORD D);
bool RdConst(WORD D, WORD& RT);
WORD GetOp1(WORD DOfs, char Op, WORD E1); /*PPTerm*/ //forward;
TVarDcl* FindVarDcl();
TVarDcl* MakeVarDcl(WORD DOfs, integer Idx);
bool RdVar(WORD DOfs, integer Kind, integer Idx, WORD& RT); /*PTerm || idx*/
WORD RdTerm(WORD DOfs, integer Kind); /*PPTerm*/ // forward;
WORD RdAddExpr(WORD DOfs, integer Kind); /*PPTerm*/ // forward;
WORD DomFun(WORD DOfs);
WORD GetOpl(WORD DOfs, char Op, WORD E1);
WORD GetOp2(WORD DOfs, char Op, WORD E1, WORD E2);
WORD GetFunOp(WORD DOfs, WORD ResDOfs, char Op, pstring ArgTyp, integer Kind);
WORD RdPrimExpr(WORD DOfs, integer Kind);
WORD RdMultExpr(WORD DOfs, integer Kind);
WORD RdAddExpr(WORD DOfs, integer Kind);
WORD RdListTerm(WORD DOfs, integer Kind);
WORD RdTerm(WORD DOfs, integer Kind);
WORD MakeDomain(TDomainTyp DTyp, const pstring Nm);
WORD GetDomain(bool Create, pstring Nm);
WORD RdDomain(); /*PDomain*/
void RdDomains();
void RdConstants();
WORD GetPredicate(); /*PPredicate*/
WORD RdPredicate(); /*PPredicate*/
WORD GetOutpMask(TPredicate* P);
void RdPredicateDcl(bool FromClauses, TDatabase* Db);
WORD GetCommand(TCommandTyp Code, WORD N);
void RdTermList(TCommand* C, WORD D, WORD Kind);
WORD RdCommand(); /*PCommand*/
WORD RdPredCommand(TCommandTyp Code);
void RdDbTerm(WORD DOfs);
void RdDbClause(TPredicate* P);
void CheckPredicates(WORD POff);
void RdAutoRecursionHead(TPredicate* P, TBranch* B);
void RdSemicolonClause(TPredicate* P, TBranch* B);
void RdClauses();
WORD MakePred(pstring PredName, pstring ArgTyp, WORD PredKod, WORD PredMask);

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
				Move(s, db->Name, length(s) + 1);
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
	if (ProlgCallLevel = 0) ReleaseStore2(p2); 
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



// odsud je obsah RUNPROLG.PAS
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
