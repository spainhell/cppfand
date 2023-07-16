#pragma once
#include "access.h"

#include "models/FrmlElem.h"
#include "models/Instr.h"
#include "../Common/pstring.h"
#include "rdrun.h"
#include "switches.h"

class MergeReportBase;
extern bool IsRdUserFunc;

void TestCatError(int i, const std::string& name, bool old);
bool IsRecVar(LocVar** LV);
LocVar* RdRecVar();
LocVar* RdIdxVar();
FrmlElem* RdRecVarFldFrml(LocVar* LV, char& FTyp);
char RdOwner(LinkD** LLD, LocVar** LLV); // 'r','i','F'
FrmlElem* RdFldNameFrmlP(char& FTyp, MergeReportBase*);
FileD* RdPath(bool NoFD, std::string& Path, WORD& CatIRec);
FrmlElem* RdFunctionP(char& FFTyp);
XKey* RdViewKeyImpl(FileD* FD);
void RdSelectStr(FrmlElemFunction* Z);

Instr* RdPInstr();

void RdPInstrAndChain(Instr** PD);
void RdChoices(Instr* PD);
void RdMenuAttr(Instr* PD);
Instr* RdMenuBox(bool Loop);
Instr* RdMenuBar();
Instr_loops* RdIfThenElse();
Instr_loops* RdWhileDo();
Instr* RdFor();
Instr* RdCase();
Instr_loops* RdRepeatUntil();
Instr_forall* RdForAll();
Instr* RdBeginEnd();
Instr_proc* RdProcArg(char Caller);
void RdKeyCode(EdExitD* X);

const BYTE NKeyNames = 21;
struct kNames { pstring Nm; BYTE Brk; unsigned __int32 Code; };
extern kNames KeyNames[NKeyNames];
bool RdHeadLast(EditOpt* EO);
bool RdHeadLast(Instr_edittxt* IE);
bool RdViewOpt(EditOpt* EO);
void RdKeyList(EdExitD* X);
void RdProcCall(Instr** pinstr); // muze upravit pinstr z hlavni funkce

std::vector<FieldDescr*> RdFlds();
std::vector<FieldDescr*> RdSubFldList(std::vector<FieldDescr*>& InFL, char Opt);
Instr_sort* RdSortCall();
Instr_edit* RdEditCall();
void RdEditOpt(EditOpt* EO);
Instr* RdReportCall();
void RdRprtOpt(RprtOpt* RO, bool has_first);
Instr* RdRDBCall();
Instr* RdExec();
Instr* RdCopyFile();
CpOption RdCOpt();
bool RdX(FileD* FD);
bool TestFixVar(CpOption Opt, FileD* FD1, FileD* FD2);
bool RdList(pstring* S);
Instr* RdPrintTxt();
Instr* RdEditTxt();
Instr* RdPutTxt();
Instr* RdTurnCat();
void RdWriteln(WriteType OpKind, Instr_writeln** pinstr);
Instr* RdReleaseDrive();
Instr* RdIndexfile();
Instr* RdGetIndex();
Instr* RdGotoXY();
Instr* RdClrWw();
Instr* RdMount();
Instr* RdDisplay();

Instr_recs* RdMixRecAcc(PInstrCode Op);
Instr* RdLinkRec();
Instr* RdBackup(char MTyp, bool IsBackup);
Instr* RdSetEditTxt();
FrmlElem* AdjustComma(FrmlElem* Z1, FieldDescr* F, instr_type Op);
AssignD* MakeImplAssign(FileD* FD1, FileD* FD2);
Instr_assign* RdAssign();
Instr* RdWith();
Instr_assign* RdUserFuncAssign();
void ReadProcHead(const std::string& name);
Instr* ReadProcBody();
void ReadDeclChpt();
FrmlElem* GetEvalFrml(FileD* file_d, FrmlElem21* X, void* record);
Instr* RdCallLProc();

#ifdef FandGraph
Instr_graph* RdGraphP();
#endif

#ifndef FandSQL
void RdSqlRdWrTxt(bool Rd);
#endif