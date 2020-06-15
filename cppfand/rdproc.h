#pragma once
#include "access.h"
#include "constants.h"
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
void RdSelectStr(FrmlPtr Z);
Instr* GetPInstr(PInstrCode Kind, WORD Size);

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
Instr* RdForAll();
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
Instr* RdEditCall();
void RdEditOpt(EditOpt* EO);
void RdReportCall();
void RdRprtOpt(RprtOpt* RO, bool HasFrst);
void RdRDBCall();
void RdExec();
void RdCopyFile();
CpOption RdCOpt();
bool RdX(FileD* FD);
bool TestFixVar(CpOption Opt, FileD* FD1, FileD* FD2);
bool RdList(pstring* S);
void RdPrintTxt();
void RdEditTxt();
void RdPutTxt();
void RdTurnCat();
void RdWriteln(BYTE OpKind, Instr_writeln** pinstr);
void RdReleaseDrive();
void RdIndexfile();
void RdGetIndex();
void RdGotoXY();
void RdClrWw();
void RdMount();
void RdDisplay();
void RdGraphP();
Instr_recs* RdMixRecAcc(PInstrCode Op);
void RdLinkRec();
void RdBackup(char MTyp, bool IsBackup);
void RdSetEditTxt();
#ifndef FandSQL
void RdSqlRdWrTxt(bool Rd);
#endif
#ifdef FandProlog
void RdCallLProc();
#endif
FrmlPtr AdjustComma(FrmlPtr Z1, FieldDPtr F, char Op);
AssignD* MakeImplAssign(FileD* FD1, FileD* FD2);
Instr_assign* RdAssign();
Instr* RdWith();
Instr_assign* RdUserFuncAssign();
void ReadProcHead();
Instr* ReadProcBody();
void ReadDeclChpt();
FrmlPtr GetEvalFrml(FrmlPtr X);
