#include "Prolog.h"

integer VarCount = 0;
TDomain* IntDom = nullptr;
TDomain* RealDom = nullptr;
TDomain* StrDom = nullptr;
TDomain* LongStrDom = nullptr; /*PDomain*/
TDomain* BoolDom = nullptr;
TDomain* LexDom = nullptr;
TDomain* LLexDom = nullptr; /*PDomain*/
TPredicate* MemPred = nullptr;
TPredicate* LenPred = nullptr;
TPredicate* InvPred = nullptr;
TPredicate* AddPred = nullptr;
TPredicate* DelPred = nullptr;
TPredicate* UnionPred = nullptr;
TPredicate* MinusPred = nullptr;
TPredicate* InterPred = nullptr;
TPredicate* TxtPred = nullptr; /*PPredicate*/
TTerm* UnderscoreTerm = nullptr; /*PPTerm*/
bool UnbdVarsInTerm = false, WasUnbd = false, WasOp = false;
TProgRoots* Roots = nullptr;
char* PackedTermPtr = nullptr;
WORD PTPMaxOfs = 0;
std::vector<TPredicate*> ClausePreds;
