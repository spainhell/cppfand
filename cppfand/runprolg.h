#pragma once
#include "constants.h"
#include "pstring.h"
#include "access.h"

const BYTE _IntT = 249; const BYTE _RealT = 250; const BYTE _StrT = 251;
const BYTE _LongStrT = 252; const BYTE _ListT = 253; const BYTE _VarT = 254;
const BYTE _UnderscT = 255; /*term fun*/
const BYTE _CioMaskOpt = 0x80; const BYTE _PackInpOpt = 0x40;
const BYTE _FandCallOpt = 0x20; const BYTE _BuildInOpt = 0x10;
const BYTE _DbaseOpt = 0x08;  /* predicate Opt */
const BYTE _ConcatP = 2; const BYTE _NextLexP = 4; const BYTE _GetLexP = 5;
const BYTE _FandFieldP = 6; const BYTE _FandFileP = 7; const BYTE _FandKeyP = 8;
const BYTE _FandKeyFieldP = 9;
const BYTE _FandLinkP = 10; const BYTE _FandLinkFieldP = 11;
const BYTE _MemP = 14; const BYTE _LenP = 15; const BYTE _InvP = 16;
const BYTE _AddP = 17; const BYTE _DelP = 18;
const BYTE _UnionP = 20; const BYTE _MinusP = 21; const BYTE _InterP = 22;
const BYTE _AbbrevP = 25;
const BYTE _CallP = 32;

enum TDomainTyp { _UndefD, _IntD, _RealD, _StrD, _LongStrD, _ListD, _FunD, _RedefD };

struct TDomain {
	WORD Chain = 0;
	TDomainTyp Typ = _UndefD;
	// case byte of
	WORD ElemDom = 0; pstring Name; // 0
	WORD OrigDom = 0; // 1
	WORD FunDcl = 0; // 2
};

struct TConst {
	WORD Chain = 0; WORD Dom = 0;/*PDomain*/
	WORD Expr = 0;/*PPTerm*/ pstring Name;
};

struct TFunDcl {
	WORD Chain = 0; WORD Name = 0; BYTE Arity = 0;
	WORD Arg[3]{ 0 };
};

struct TPTerm {
	BYTE Fun = 0;
	BYTE Arity = 0; WORD Arg[1]{ 0 }; /*PPTerm*/
	char Op = '\0'; WORD E1 = 0, E2 = 0, E3 = 0; /*PPTerm*/
	char Op0 = '\0'; WORD E[4]{ 0 }; // puvodne 1..3
	char Op1 = '\0'; integer II = 0;
	char Op2 = '\0'; double RR = 0.0;
	char Op3 = '\0'; pstring SS;
	char Op4 = '\0'; WORD Elem = 0, Next = 0; /*PPTerm*/
	WORD Idx = 0; bool Bound;
};

struct TTermList {
	WORD Chain = 0; /*PTermList*/ WORD Elem = 0; /*PPTerm*/
};

struct TVarDcl {
	TVarDcl* Chain = nullptr; WORD Dom = 0; integer Idx = 0;
	bool Bound = false, Used = false; pstring Name;
};

struct TWriteD {
	WORD Chain = 0; /*PWriteD*/
	bool IsString = false;
	pstring SS;
	integer Idx = 0; WORD Dom = 0;
};

enum TCommandTyp {
	_PredC, _FailC, _CutC, _WriteC, _CompC, _AssertC, _RetractC,
	_SaveC, _ConsultC, _LoadLexC, _Trace, _SelfC,
	_AppPkC, _AppUnpkC,
	_ErrorC, _WaitC, _NotC, _AllC, _AutoC
};

struct TCommand {
	WORD Chain = 0; /*PCommand*/
	TCommandTyp Code;
	WORD Arg = 0; /*PTermList*/
	WORD Pred = 0; /*PPredicate*/
	WORD InpMask = 0, OutpMask = 0; /*only _CioMaskOpt*/
	WORD Elem = 0; /*_MemP..:ListDom else PPTerm*/
	WORD Idx = 0, Idx2 = 0; /*_AllC*/
	WORD CompMask = 0, KDOfs = 0; BYTE ArgI[1]{ 0 }; /*only FAND-file*/
	WORD WrD = 0; /*PWriteD*/ bool NL = false;
	WORD WrD1 = 0; WORD MsgNr = 0;
	integer TrcLevel = 0;
	TDomainTyp Typ; char CompOp = 0; WORD E1Idx = 0, E2 = 0;/*PPTerm*/
	WORD DbPred = 0;  /* !_LoadLexC */
	FileD* FD = nullptr; FieldDescr* FldD = nullptr; pstring Name;
	WORD apIdx = 0, apDom = 0, /*Unpk*/ apTerm = 0;
	WORD Arg1 = 0; /*PTermList*/  /* like _PredC       autorecursion*/
	BYTE iWrk = 0, iOutp = 0, nPairs = 0;
	struct stPair { BYTE iInp = 0; BYTE iOutp = 0; } Pair[6];
};

struct TBranch {
	WORD Chain = 0; /*PBranch*/
	WORD HeadIMask = 0, HeadOMask = 0; 
	WORD Head = 0; /*PTermList*/ 
	WORD Cmd = 0; /*PCommand*/
};

struct TDbBranch {
	WORD LL; 
	BYTE A[2/*LL*/]{ 0 }; /*  for all Arguments */
};

struct TFldList {
	WORD Chain = 0; /*PFldList*/
	FieldDescr* FldD = nullptr;
};

struct TScanInf {
	FileD* FD = nullptr; 
	WORD FL = 0; /*PFldList*/
	pstring Name;
};

struct TPredicate {
	WORD Chain = 0, ChainDb = 0; /*PPredicate*/
	WORD Name = 0; /*PString*/
	TBranch* Branch; /*offset*/ /*InstrPtr|ofs PScanInf|PDbBranch*/
	WORD InstSz = 0, InpMask = 0, LocVarSz = 0; /*FAND-proc| _xxxP for buildIn*/
	BYTE Opt = 0; BYTE Arity = 0; WORD Arg[3]{ 0 }; /*PDomain*/
};

struct TDatabase {
	WORD Chain = 0; /*PDatabase*/
	WORD Pred = 0; /*PPredicate*/ 
	WORD SOfs = 0; /*LongStrPtr/saved/*/
	pstring Name;
};

struct TProgRoots {
	WORD Domains = 0, Consts = 0, Predicates = 0, Databases = 0;
};

extern WORD _Sg;

struct TMemBlkHd
{
	TMemBlkHd* Chain = nullptr;
	WORD Sz = 0;
};

class TMemory {
public:
	TMemory();
	WORD RestSz = 0;
	void* CurLoc = nullptr;
	TMemBlkHd* CurBlk = nullptr;
	TMemBlkHd* FreeList = nullptr;
	void* Get(WORD Sz);
	void* Mark();
	void Release(void* P);
	pstring StoreStr(pstring s);
	void* Alloc(WORD Sz);
	void Free(void* P, WORD Sz);
};

extern TMemBlkHd* FreeMemList;
extern TMemory Mem1; extern TMemory Mem2; extern TMemory Mem3;
const WORD ProlgCallLevel = 0;

void RunProlog(RdbPos* Pos, pstring* PredName);
LongStr* SaveDb(WORD DbOfs/*PDatabase*/, longint AA);

WORD ReadProlog(WORD RecNr);

