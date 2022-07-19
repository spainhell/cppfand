#pragma once
#include <map>

#include "../cppfand/Chained.h"
#include "../cppfand/constants.h"
#include "../cppfand/models/Instr.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/FileD.h"
#include "../cppfand/pstring.h"

/*term fun*/
const BYTE _CioMaskOpt = 0x80;
const BYTE _PackInpOpt = 0x40;
const BYTE _FandCallOpt = 0x20;
const BYTE _BuildInOpt = 0x10;
const BYTE _DbaseOpt = 0x08;  /* predicate Opt */


enum class prolog_func
{
	_undefined = 0, _IntT = 249, _RealT = 250, _StrT = 251,
	_LongStrT = 252, _ListT = 253, _VarT = 254, _UnderscT = 255
};

enum class proc_type
{
	_undefined = 0, _ConcatP = 2, _NextLexP = 4, _GetLexP = 5,
	_FandFieldP = 6, _FandFileP = 7, _FandKeyP = 8,
	_FandKeyFieldP = 9, _FandLinkP = 10, _FandLinkFieldP = 11,
	_MemP = 14, _LenP = 15, _InvP = 16, _AddP = 17, _DelP = 18,
	_UnionP = 20, _MinusP = 21, _InterP = 22, _AbbrevP = 25, _CallP = 32
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

struct TFunDcl : public Chained<TFunDcl> {
	//WORD pChain = 0;
	std::string Name;
	BYTE Arity = 0;
	std::vector<TDomain*> Arg;
};

struct TTerm {
	prolog_func Fun = prolog_func::_undefined;
	BYTE FunIdx = 0;
	BYTE Arity = 0;
	longint Pos = 0;
	TTerm* Arg[3]{ nullptr }; /*PPTerm*/
	instr_type Op = _notdefined;
	TTerm* E1 = nullptr;
	TTerm* E2 = nullptr;
	TTerm* E3 = nullptr; /*PPTerm*/
	instr_type Op0 = _notdefined;
	TTerm* E[4]{ nullptr }; // puvodne 1..3
	bool BB = false;
	instr_type Op1 = _notdefined;
	integer II = 0;
	instr_type Op2 = _notdefined;
	double RR = 0.0;
	instr_type Op3 = _notdefined;
	std::string SS;
	instr_type Op4 = _notdefined;
	TTerm* Elem = nullptr;
	TTerm* Next = nullptr; /*PPTerm*/
	WORD Idx = 0;
	bool Bound = false;
};

struct TConst : public Chained<TConst> {
	//WORD pChain = 0;
	std::string Name;
	TDomain* Dom = nullptr;/*PDomain*/
	TTerm* Expr = nullptr;/*PPTerm*/
};

class TVarDcl {
public:
	std::string Name;
	TDomain* Dom = nullptr;
	TTerm* term = nullptr;
	integer Idx = 0;
	bool Bound = false;
	bool Used = false;
};

struct TWriteD : public Chained<TWriteD> {
	// WORD pChain = 0; /*PWriteD*/
	bool IsString = false;
	std::string SS;
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
	std::string Name;
	TCommandTyp Code;
	std::map<int, TTerm*> Arg; /*PTermList*/
	TPredicate* Pred = nullptr; /*PPredicate*/
	WORD InpMask = 0, OutpMask = 0; /*only _CioMaskOpt*/
	TDomain* ElemDomain = nullptr; /*_MemP..:ListDom else PPTerm*/
	TTerm* ElemTerm = nullptr;
	WORD Idx = 0; WORD Idx2 = 0; /*_AllC*/
	WORD CompMask = 0; XKey* KDOfs = nullptr; BYTE ArgI[1]{ 0 }; /*only FAND-file*/
	TWriteD* WrD = nullptr; /*PWriteD*/ bool NL = false;
	WORD WrD1 = 0; WORD MsgNr = 0;
	integer TrcLevel = 0;
	TDomainTyp Typ; char CompOp = 0; WORD E1Idx = 0; TTerm* E2 = nullptr;/*PPTerm*/
	TDatabase* DbPred = nullptr;  /* !_LoadLexC */
	FileD* FD = nullptr; FieldDescr* FldD = nullptr;
	WORD apIdx = 0; TTerm* apDom = nullptr; TTerm* apTerm = nullptr;/*Unpk*/
	WORD Arg1 = 0; /*PTermList*/  /* like _PredC       autorecursion*/
	BYTE iWrk = 0, iOutp = 0, nPairs = 0;
	struct stPair { BYTE iInp = 0; BYTE iOutp = 0; } Pair[6];
};

struct TBranch : public Chained<TBranch> {
	//WORD pChain = 0; /*PBranch*/
	WORD HeadIMask = 0;
	WORD HeadOMask = 0;
	std::map<int, TTerm*> Head; /*PTermList*/
	std::vector<TCommand*> Cmd; /*PCommand*/
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
	std::string Name;
	FileD* FD = nullptr;
	TFldList* FL = nullptr; /*PFldList*/
};

struct TPredicate : public Chained<TPredicate> {
	TPredicate* ChainDb = nullptr; /*PPredicate*/
	std::string Name; /*PString*/
	TBranch* branch = nullptr;
	TDbBranch* dbBranch = nullptr;
	TScanInf* scanInf = nullptr;
	Instr_proc* instr = nullptr;
	WORD InstSz = 0, InpMask = 0;
	proc_type LocVarSz = proc_type::_undefined; /*FAND-proc | _xxxP for buildIn*/
	BYTE Opt = 0;
	BYTE Arity = 0;
	std::vector<TDomain*> Arg; /*PDomain*/
	std::map<int, TVarDcl*> VarsCheck;
};

struct TDatabase : public Chained<TDatabase> {
	// WORD pChain = 0; /*PDatabase*/
	std::string Name;
	TPredicate* Pred = nullptr; /*PPredicate*/
	std::string SOfs; /*LongStrPtr/saved/*/
};

struct TProgRoots {
	std::map<std::string, TDomain*> Domains;
	std::vector<TConst*> Consts;
	std::vector<TPredicate*> Predicates;
	std::map<std::string, TDatabase*> Databases;
};

//extern TVarDcl* VarDcls;
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
extern TTerm* UnderscoreTerm; /*PPTerm*/
extern bool UnbdVarsInTerm, WasUnbd, WasOp;
extern TProgRoots* Roots;
extern char* PackedTermPtr;
extern WORD PTPMaxOfs;
extern TPredicate* ClausePreds;
