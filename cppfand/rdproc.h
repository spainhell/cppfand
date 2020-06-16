#pragma once
#include "access.h"
#include "constants.h"
#include "models/FrmlElem.h"
#include "models/Instr.h"
#include "pstring.h"
#include "rdrun.h"

extern bool IsRdUserFunc;

void TestCatError(WORD I, pstring Nm, bool Old);
bool IsRecVar(LocVar** LV);
LocVar* RdRecVar();
LocVar* RdIdxVar();
FrmlPtr RdRecVarFldFrml(LocVar* LV, char& FTyp);
char RdOwner(LinkD* LLD, LocVar* LLV); // 'r','i','F'
FrmlPtr RdFldNameFrmlP(char& FTyp);
FileD* RdPath(bool NoFD, pstring** Path, WORD& CatIRec);
FrmlPtr RdFunctionP(char& FFTyp);
KeyD* RdViewKeyImpl(FileD* FD);
void RdSelectStr(FrmlElem0* Z);
//Instr* GetPInstr(PInstrCode Kind, WORD Size);

Instr* RdPInstr(); // hlavní funkce

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
bool NotCode(pstring Nm, WORD CodeBase, WORD BrkBase, EdExKeyD* E);

const BYTE NKeyNames = 21;
struct kNames { pstring Nm; BYTE Brk; WORD Code; };
extern kNames KeyNames[NKeyNames];
bool RdHeadLast(void* AA);
bool RdViewOpt(EditOpt* EO);
void RdKeyList(EdExitD* X);
//extern Instr* RdPInstr; // toto bude ukazatel na pozdìji pøiøazenou funkci
//Instr* GetPD(PInstrCode Kind, WORD Size);
void RdProcCall(Instr** pinstr); // mùže upravit pinstr z hlavní funkce
FieldList RdFlds();
FieldList RdSubFldList(FieldList InFL, char Opt);
Instr_sort* RdSortCall();
Instr_edit* RdEditCall();
void RdEditOpt(EditOpt* EO);
Instr* RdReportCall();
void RdRprtOpt(RprtOpt* RO, bool HasFrst);
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
void RdWriteln(BYTE OpKind, Instr_writeln** pinstr);
Instr* RdReleaseDrive();
Instr* RdIndexfile();
Instr* RdGetIndex();
Instr* RdGotoXY();
Instr* RdClrWw();
Instr* RdMount();
Instr* RdDisplay();
void RdGraphP();
Instr_recs* RdMixRecAcc(PInstrCode Op);
Instr* RdLinkRec();
Instr* RdBackup(char MTyp, bool IsBackup);
Instr* RdSetEditTxt();
#ifndef FandSQL
void RdSqlRdWrTxt(bool Rd);
#endif
#ifdef FandProlog
Instr* RdCallLProc();
#endif
FrmlElem* AdjustComma(FrmlElem* Z1, FieldDescr* F, char Op);
AssignD* MakeImplAssign(FileD* FD1, FileD* FD2);
Instr_assign* RdAssign();
Instr* RdWith();
Instr_assign* RdUserFuncAssign();
void ReadProcHead();
Instr* ReadProcBody();
void ReadDeclChpt();
FrmlElem* GetEvalFrml(FrmlElem21* X);
