#include "runprolg.h"

const WORD MaxPackedPredLen = 4000;

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
}

WORD OOfs(void* p);
WORD GetZStor(WORD Sz);
void* OPtr(WORD Sg, void* p);
WORD StorStr(pstring S); // assembler

/*  L E X A N A L  =========================================================*/
bool IsCharUpper(char C);
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
WORD RdDomain();/*PDomain*/
void RdDomains();
void RdConstants();
WORD GetPredicate(); /*PPredicate*/
WORD RdPredicate(); /*PPredicate*/
WORD GetOutpMask(TPredicate* P);
void RdPredicateDcl(bool FromClauses, TDatabase* Db);
WORD GetCommand(TCommandTyp Code, WORD N);
void RdTermList(TCommand* C, WORD D, WORD Kind);
WORD RdCommand()/*PCommand*/;
WORD RdPredCommand(TCommandTyp Code);
void RdDbTerm(WORD DOfs);
void RdDbClause(TPredicate* P);
void CheckPredicates(WORD POff);
void RdAutoRecursionHead(TPredicate* P, TBranch* B);
void RdSemicolonClause(TPredicate* P, TBranch* B);
void RdClauses();
WORD MakePred(pstring PredName, pstring ArgTyp, WORD PredKod, WORD PredMask);
WORD ReadProlog(WORD RecNr);




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

	return WORD();
}
