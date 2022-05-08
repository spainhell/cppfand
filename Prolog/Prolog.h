#pragma once
#include "../cppfand/Chained.h"
#include "../cppfand/constants.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/FileD.h"
#include "../cppfand/pstring.h"

const BYTE _IntT = 249;
const BYTE _RealT = 250;
const BYTE _StrT = 251;
const BYTE _LongStrT = 252;
const BYTE _ListT = 253;
const BYTE _VarT = 254;
const BYTE _UnderscT = 255; /*term fun*/
const BYTE _CioMaskOpt = 0x80;
const BYTE _PackInpOpt = 0x40;
const BYTE _FandCallOpt = 0x20;
const BYTE _BuildInOpt = 0x10;
const BYTE _DbaseOpt = 0x08;  /* predicate Opt */

enum proc_type
{
	_undefined = 0, _ConcatP = 2, _NextLexP = 4, _GetLexP = 5,
	_FandFieldP = 6, _FandFileP = 7, _FandKeyP = 8,
	_FandKeyFieldP = 9, _FandLinkP = 10, _FandLinkFieldP = 11,
	_MemP = 14,	_LenP = 15, _InvP = 16, _AddP = 17, _DelP = 18,
	_UnionP = 20, _MinusP = 21,	_InterP = 22, _AbbrevP = 25, _CallP = 32
};

enum TDomainTyp { _UndefD, _IntD, _RealD, _StrD, _LongStrD, _ListD, _FunD, _RedefD };

struct TFunDcl;
struct TDomain : public Chained<TDomain> {
	//WORD pChain = 0;
	TDomainTyp Typ = _UndefD;
	// case byte of
	TDomain* ElemDom = nullptr;
	std::string Name; // 0
	TDomain* OrigDom = nullptr; // 1
	TFunDcl* FunDcl = nullptr; // 2
};

struct TPTerm;
struct TConst : public Chained<TConst> {
	//WORD pChain = 0;
	TDomain* Dom = nullptr;/*PDomain*/
	TPTerm* Expr = nullptr;/*PPTerm*/
	std::string Name;
};

struct TFunDcl : public Chained<TFunDcl> {
	//WORD pChain = 0;
	std::string Name;
	BYTE Arity = 0;
	std::vector<TDomain*> Arg;
};

struct TPTerm {
	WORD Fun = 0;
	BYTE Arity = 0; TPTerm* Arg[1]{ nullptr }; /*PPTerm*/
	instr_type Op = _notdefined;
	TPTerm* E1 = nullptr; TPTerm* E2 = nullptr; TPTerm* E3 = nullptr; /*PPTerm*/
	instr_type Op0 = _notdefined;
	TPTerm* E[4]{ nullptr }; // puvodne 1..3
	instr_type Op1 = _notdefined; integer II = 0;
	instr_type Op2 = _notdefined; double RR = 0.0;
	instr_type Op3 = _notdefined; std::string SS;
	instr_type Op4 = _notdefined; TPTerm* Elem = nullptr; TPTerm* Next = nullptr; /*PPTerm*/
	WORD Idx = 0; bool Bound = false;
};

struct TTermList : public Chained<TTermList> {
	//WORD pChain = 0; /*PTermList*/
	TPTerm* Elem = nullptr; /*PPTerm*/
};

class TVarDcl : public Chained<TVarDcl> {
public:
	//TVarDcl* pChain = nullptr;
	TDomain* Dom = nullptr;
	integer Idx = 0;
	bool Bound = false, Used = false;
	pstring Name;
};

struct TWriteD : public Chained<TWriteD> {
	// WORD pChain = 0; /*PWriteD*/
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

struct TCommand : public Chained<TCommand> {
	// WORD pChain = 0; /*PCommand*/
	TCommandTyp Code;
	TTermList* Arg = nullptr; /*PTermList*/
	TPredicate* Pred = nullptr; /*PPredicate*/
	WORD InpMask = 0, OutpMask = 0; /*only _CioMaskOpt*/
	TDomain* Elem = nullptr; /*_MemP..:ListDom else PPTerm*/
	WORD Idx = 0;
	WORD Idx2 = 0; /*_AllC*/
	WORD CompMask = 0; XKey* KDOfs = nullptr; BYTE ArgI[1]{ 0 }; /*only FAND-file*/
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

struct TBranch : public Chained<TBranch> {
	//WORD pChain = 0; /*PBranch*/
	WORD HeadIMask = 0, HeadOMask = 0;
	TTermList* Head = nullptr; /*PTermList*/
	TCommand* Cmd = nullptr; /*PCommand*/
};

struct TDbBranch : public Chained<TDbBranch> {
	WORD LL = 0;
	BYTE A[2/*LL*/]{ 0 }; /*  for all Arguments */
};

struct TFldList : public Chained<TFldList> {
	//WORD pChain = 0; /*PFldList*/
	FieldDescr* FldD = nullptr;
};

struct TScanInf {
	FileD* FD = nullptr;
	TFldList* FL = nullptr; /*PFldList*/
	std::string Name;
};

struct TPredicate : public Chained<TPredicate> {
	TPredicate* ChainDb = nullptr; /*PPredicate*/
	std::string Name; /*PString*/
	TBranch* Branch = nullptr; /*offset*/ /*InstrPtr|ofs PScanInf|PDbBranch*/
	WORD InstSz = 0, InpMask = 0;
	proc_type LocVarSz = _undefined; /*FAND-proc | _xxxP for buildIn*/
	BYTE Opt = 0;
	BYTE Arity = 0;
	std::vector<TDomain*> Arg; /*PDomain*/
};

struct TDatabase : public Chained<TDatabase> {
	// WORD pChain = 0; /*PDatabase*/
	TPredicate* Pred = nullptr; /*PPredicate*/
	std::string SOfs; /*LongStrPtr/saved/*/
	std::string Name;
};

struct TProgRoots {
	TDomain* Domains = nullptr;
	TConst* Consts = nullptr;
	TPredicate* Predicates = nullptr;
	TDatabase* Databases = nullptr;
};

extern TVarDcl* VarDcls;
extern integer VarCount;
extern TDomain* IntDom;
extern TDomain* RealDom;
extern TDomain* StrDom;
extern TDomain* LongStrDom; /*PDomain*/
extern TDomain* BoolDom;
extern TDomain* LexDom;
extern TDomain* LLexDom; /*PDomain*/
extern TPredicate* MemPred;
extern TPredicate* LenPred;
extern TPredicate* InvPred;
extern TPredicate* AddPred;
extern TPredicate* DelPred;
extern TPredicate* UnionPred;
extern TPredicate* MinusPred;
extern TPredicate* InterPred;
extern TPredicate* TxtPred; /*PPredicate*/
extern TPTerm* UnderscoreTerm; /*PPTerm*/
extern bool UnbdVarsInTerm, WasUnbd, WasOp;
extern TProgRoots* Roots;
extern char* PackedTermPtr;
extern WORD PTPMaxOfs;
extern TPredicate* ClausePreds;
