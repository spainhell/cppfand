#pragma once
#include "access.h"
#include "Compiler.h"

#include "models/FrmlElem.h"
#include "models/Instr.h"
#include "../Common/pstring.h"
#include "rdrun.h"
#include "switches.h"

class MergeReportBase;

void TestCatError(Compiler* compiler, int i, const std::string& name, bool old);
bool IsRecVar(Compiler* compiler, LocVar** LV);
LocVar* RdRecVar(Compiler* compiler);
LocVar* RdIdxVar(Compiler* compiler);
FrmlElem* RdRecVarFldFrml(Compiler* compiler, LocVar* LV, char& FTyp);
char RdOwner(Compiler* compiler, FileD* file_d, LinkD** LLD, LocVar** LLV); // 'r','i','F'
FrmlElem* RdFldNameFrmlP(Compiler* compiler, char& FTyp, MergeReportBase*);
FileD* RdPath(Compiler* compiler, bool NoFD, std::string& Path, WORD& CatIRec);
FrmlElem* RdFunctionP(Compiler* compiler, char& FFTyp);
XKey* RdViewKeyImpl(Compiler* compiler, FileD* FD);
void RdSelectStr(Compiler* compiler, FrmlElemFunction* Z);

std::vector<Instr*> RdPInstr(Compiler* compiler);

void RdChoices(Compiler* compiler, Instr* PD);
void RdMenuAttr(Compiler* compiler, Instr* PD);
Instr* RdMenuBox(Compiler* compiler, bool Loop);
Instr* RdMenuBar(Compiler* compiler);
Instr_loops* RdIfThenElse(Compiler* compiler);
Instr_loops* RdWhileDo(Compiler* compiler);
std::vector<Instr*> RdFor(Compiler* compiler);
Instr* RdCase(Compiler* compiler);
Instr_loops* RdRepeatUntil(Compiler* compiler);
Instr_forall* RdForAll(Compiler* compiler);
std::vector<Instr*> RdBeginEnd(Compiler* compiler);
Instr_proc* RdProcArg(Compiler* compiler, char Caller);
void RdKeyCode(Compiler* compiler, EdExitD* X);

bool RdHeadLast(Compiler* compiler, EditOpt* EO);
bool RdHeadLast(Compiler* compiler, Instr_edittxt* IE);
void RdKeyList(Compiler* compiler, EdExitD* X);
void RdProcCall(Compiler* compiler, Instr** pinstr); // muze upravit pinstr z hlavni funkce

std::vector<FieldDescr*> RdFlds(Compiler* compiler);
std::vector<FieldDescr*> RdSubFldList(Compiler* compiler, const std::vector<FieldDescr*>& v_fields, char Opt);
Instr_sort* RdSortCall(Compiler* compiler);
Instr_edit* RdEditCall(Compiler* compiler);

bool RdViewOpt(Compiler* compiler, EditOpt* EO, FileD* file_d);
void RdEditOpt(Compiler* compiler, EditOpt* EO, FileD* file_d);
void RdRprtOpt(Compiler* compiler, RprtOpt* RO, bool has_first);

Instr* RdReportCall(Compiler* compiler);
Instr* RdRDBCall(Compiler* compiler);
Instr* RdExec(Compiler* compiler);
Instr* RdCopyFile(Compiler* compiler);
CpOption RdCOpt(Compiler* compiler);
bool RdX(Compiler* compiler, FileD* FD);
bool TestFixVar(Compiler* compiler, CpOption Opt, FileD* FD1, FileD* FD2);
bool RdList(Compiler* compiler, pstring* S);
Instr* RdPrintTxt(Compiler* compiler);
Instr* RdEditTxt(Compiler* compiler);
Instr* RdPutTxt(Compiler* compiler);
Instr* RdTurnCat(Compiler* compiler);
void RdWriteln(Compiler* compiler, WriteType OpKind, Instr_writeln** pinstr);
Instr* RdReleaseDrive(Compiler* compiler);
Instr* RdIndexfile(Compiler* compiler);
Instr* RdGetIndex(Compiler* compiler);
Instr* RdGotoXY(Compiler* compiler);
Instr* RdClrWw(Compiler* compiler);
Instr* RdMount(Compiler* compiler);
Instr* RdDisplay(Compiler* compiler);

Instr_recs* RdMixRecAcc(Compiler* compiler, PInstrCode Op);
Instr* RdLinkRec(Compiler* compiler);
Instr* RdBackup(Compiler* compiler, char MTyp, bool IsBackup);
Instr* RdSetEditTxt(Compiler* compiler);
FrmlElem* AdjustComma(FrmlElem* Z1, FieldDescr* F, instr_type Op);
std::vector<AssignD*> MakeImplAssign(Compiler* compiler, FileD* FD1, FileD* FD2);
Instr_assign* RdAssign(Compiler* compiler);
Instr* RdWith(Compiler* compiler);
Instr_assign* RdUserFuncAssign(Compiler* compiler);
void ReadProcHead(Compiler* compiler, const std::string& name);
std::vector<Instr*> ReadProcBody(Compiler* compiler);
void ReadDeclChpt(Compiler* compiler);
FrmlElem* GetEvalFrml(FileD* file_d, FrmlElem21* X, void* record);
Instr* RdCallLProc(Compiler* compiler);

#ifdef FandGraph
Instr_graph* RdGraphP(Compiler* compiler);
#endif

#ifndef FandSQL
void RdSqlRdWrTxt(bool Rd);
#endif