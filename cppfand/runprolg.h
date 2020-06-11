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

struct TDomain : public Chained {
	//WORD Chain = 0;
	TDomainTyp Typ = _UndefD;
	// case byte of
	TDomain* ElemDom = nullptr; pstring Name; // 0
	TDomain* OrigDom = nullptr; // 1
	Chained* FunDcl = 0; // 2
};

struct TPTerm;

struct TConst : public Chained {
	//WORD Chain = 0;
	TDomain* Dom = nullptr;/*PDomain*/
	TPTerm* Expr = nullptr;/*PPTerm*/ pstring Name;
};

struct TFunDcl : public Chained {
	//WORD Chain = 0;
	pstring Name;
	BYTE Arity = 0;
	Chained* Arg[3]{ nullptr };
};

struct TPTerm {
	WORD Fun = 0;
	BYTE Arity = 0; TPTerm* Arg[1]{ nullptr }; /*PPTerm*/
	char Op = '\0';
	TPTerm* E1 = nullptr; TPTerm* E2 = nullptr; TPTerm* E3 = nullptr; /*PPTerm*/
	char Op0 = '\0'; WORD E[4]{ 0 }; // puvodne 1..3
	char Op1 = '\0'; integer II = 0;
	char Op2 = '\0'; double RR = 0.0;
	char Op3 = '\0'; pstring SS;
	char Op4 = '\0'; TPTerm* Elem = nullptr; TPTerm* Next = nullptr; /*PPTerm*/
	WORD Idx = 0; bool Bound = false;
};

struct TTermList : public Chained {
	//WORD Chain = 0; /*PTermList*/
	TPTerm* Elem = nullptr; /*PPTerm*/
};

struct TVarDcl : public Chained {
	//TVarDcl* Chain = nullptr;
	TDomain* Dom = nullptr;
	integer Idx = 0;
	bool Bound = false, Used = false;
	pstring Name;
};

struct TWriteD : public Chained {
	// WORD Chain = 0; /*PWriteD*/
	bool IsString = false;
	pstring SS;
	integer Idx = 0;
	TDomain* Dom = nullptr;
};

enum TCommandTyp {
	_PredC, _FailC, _CutC, _WriteC, _CompC, _AssertC, _RetractC,
	_SaveC, _ConsultC, _LoadLexC, _Trace, _SelfC,
	_AppPkC, _AppUnpkC,
	_ErrorC, _WaitC, _NotC, _AllC, _AutoC
};

struct TDatabase;
struct TPredicate;

struct TCommand : public Chained {
	// WORD Chain = 0; /*PCommand*/
	TCommandTyp Code;
	TTermList* Arg = nullptr; /*PTermList*/
	TPredicate* Pred = nullptr; /*PPredicate*/
	WORD InpMask = 0, OutpMask = 0; /*only _CioMaskOpt*/
	TDomain* Elem = nullptr; /*_MemP..:ListDom else PPTerm*/
	WORD Idx = 0;
	TPTerm* Idx2 = nullptr; /*_AllC*/
	WORD CompMask = 0; KeyD* KDOfs = nullptr; BYTE ArgI[1]{ 0 }; /*only FAND-file*/
	TWriteD* WrD = nullptr; /*PWriteD*/ bool NL = false;
	WORD WrD1 = 0; WORD MsgNr = 0;
	integer TrcLevel = 0;
	TDomainTyp Typ; char CompOp = 0; WORD E1Idx = 0; TPTerm* E2 = nullptr;/*PPTerm*/
	TDatabase* DbPred = nullptr;  /* !_LoadLexC */
	FileD* FD = nullptr; FieldDescr* FldD = nullptr; pstring Name;
	WORD apIdx = 0; TPTerm* apDom = nullptr; TPTerm* apTerm = nullptr;/*Unpk*/
	WORD Arg1 = 0; /*PTermList*/  /* like _PredC       autorecursion*/
	BYTE iWrk = 0, iOutp = 0, nPairs = 0;
	struct stPair { BYTE iInp = 0; BYTE iOutp = 0; } Pair[6];
};

struct TBranch : public Chained {
	//WORD Chain = 0; /*PBranch*/
	WORD HeadIMask = 0, HeadOMask = 0; 
	TTermList* Head = nullptr; /*PTermList*/ 
	TCommand* Cmd = nullptr; /*PCommand*/
};

struct TDbBranch : public Chained {
	WORD LL = 0; 
	BYTE A[2/*LL*/]{ 0 }; /*  for all Arguments */
};

struct TFldList : public Chained {
	//WORD Chain = 0; /*PFldList*/
	FieldDescr* FldD = nullptr;
};

struct TScanInf {
	FileD* FD = nullptr; 
	TFldList* FL = nullptr; /*PFldList*/
	pstring Name;
};

struct TPredicate : public Chained {
	// WORD Chain = 0;
	TPredicate* ChainDb = nullptr; /*PPredicate*/
	pstring Name; /*PString*/
	TBranch* Branch = nullptr; /*offset*/ /*InstrPtr|ofs PScanInf|PDbBranch*/
	WORD InstSz = 0, InpMask = 0, LocVarSz = 0; /*FAND-proc| _xxxP for buildIn*/
	BYTE Opt = 0; BYTE Arity = 0; TDomain* Arg[3]{ nullptr }; /*PDomain*/
};

struct TDatabase : public Chained {
	// WORD Chain = 0; /*PDatabase*/
	TPredicate* Pred = nullptr; /*PPredicate*/ 
	WORD SOfs = 0; /*LongStrPtr/saved/*/
	pstring Name;
};

struct TProgRoots {
	TDomain* Domains = nullptr;
	TConst* Consts = nullptr;
	TPredicate* Predicates = nullptr;
	TDatabase* Databases = nullptr;
};

extern WORD _Sg;

struct TMemBlkHd : public Chained
{
	// TMemBlkHd* Chain = nullptr;
	WORD Sz = 0;
};

class TMemory {
public:
	TMemory();
	void Init();
	WORD RestSz;
	void* CurLoc;
	TMemBlkHd* CurBlk;
	TMemBlkHd* FreeList;
	void* Get(WORD Sz);
	void* Mark();
	void Release(void* P);
	pstring StoreStr(pstring s);
	void* Alloc(WORD Sz);
	void Free(void* P, WORD Sz);
};

extern TMemBlkHd* FreeMemList;
extern TMemory Mem1;
extern TMemory Mem2;
extern TMemory Mem3;
extern WORD ProlgCallLevel;

LongStr* SaveDb(WORD DbOfs/*PDatabase*/, longint AA);
WORD ReadProlog(WORD RecNr);
void RunProlog(RdbPos* Pos, pstring* PredName);
