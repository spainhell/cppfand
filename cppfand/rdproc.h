#pragma once
#include "access.h"
#include "constants.h"
#include "pstring.h"
#include "rdrun.h"

static bool IsRdUserFunc;

void TestCatError(WORD I, pstring Nm, bool Old);
bool IsRecVar(LocVar* LV);
LocVar* RdRecVar();
LocVar* RdIdxVar();
FrmlPtr RdRecVarFldFrml(LocVar* LV, char& FTyp);
char RdOwner(LinkD* LLD, LocVar* LLV); // 'r','i','F'
FrmlPtr RdFldNameFrmlP(char& FTyp);
FileD* RdPath(bool NoFD, pstring* Path, WORD& CatIRec);
FrmlPtr RdFunctionP(char& FFTyp);
KeyD* RdViewKeyImpl(FileD* FD);
void RdSelectStr(FrmlPtr Z);
Instr* GetPInstr(PInstrCode Kind, WORD Size);
// Instr* RdPInstr(); je níže
void RdPInstrAndChain(Instr* PD);
void RdChoices(Instr* PD);
void RdMenuAttr(Instr* PD);
Instr* RdMenuBox(bool Loop);
Instr* RdMenuBar();
Instr* RdIfThenElse();
Instr* RdWhileDo();
Instr* RdFor();
Instr* RdCase();
Instr* RdRepeatUntil();
Instr* RdForAll();
Instr* RdBeginEnd();
Instr* RdProcArg(char Caller);
void RdKeyCode(EdExitD* X);
bool NotCode(pstring Nm, WORD CodeBase, WORD BrkBase, EdExKeyD* E);

const BYTE NKeyNames = 21;
struct kNames { pstring Nm; BYTE Brk; WORD Code; };
static kNames KeyNames[NKeyNames] = {
	{"HOME", 51, _Home_},
	{"UP", 52, _up_},
	{"PGUP", 53, _PgUp_},
	{"LEFT", 55, _left_},
	{"RIGHT", 57, _right_},
	{"END", 59, _End_},
	{"DOWN", 60, _down_},
	{"PGDN", 61, _PgDn_},
	{"INS", 62, _Ins_},
	{"CTRLLEFT", 71, _CtrlLeft_},
	{"CTRLRIGHT", 72, _CtrlRight_},
	{"CTRLEND", 73, _CtrlEnd_},
	{"CTRLPGDN", 74, _CtrlPgDn_},
	{"CTRLHOME", 75, _CtrlHome_},
	{"CTRLPGUP", 76, _CtrlPgUp_},
	{"TAB", 77, _Tab_},
	{"SHIFTTAB", 78, _ShiftTab_},
	{"CTRLN", 79, _N_},
	{"CTRLY", 80, _Y_},
	{"ESC", 81, _ESC_},
	{"CTRLP", 82, _P_} };

bool RdHeadLast(void* AA);
bool RdViewOpt(EditOpt* EO);
void RdKeyList(EdExitD* X);
static Instr* RdPInstr; // toto bude ukazatel na pozdìji pøiøazenou funkci
Instr* GetPD(PInstrCode Kind, WORD Size);
void RdProcCall(); // hlavní funkce v souboru
FieldList RdFlds();
FieldList RdSubFldList(FieldList InFL, char Opt);
void RdSortCall();
void RdEditCall();
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
void RdWriteln(BYTE OpKind);
void RdReleaseDrive();
void RdIndexfile();
void RdGetIndex();
void RdGotoXY();
void RdClrWw();
void RdMount();
void RdDisplay();
void RdGraphP();
void RdMixRecAcc(PInstrCode Op);
void RdLinkRec();
void RdBackup(char MTyp, bool IsBackup);
void RdSetEditTxt();
#ifndef FandSQL
void RdSqlRdWrTxt(bool Rd);
#endif
#ifndef FandProlog
void RdCallLProc();
#endif
FrmlPtr AdjustComma(FrmlPtr Z1, FieldDPtr F, char Op);
AssignD* MakeImplAssign(FileD* FD1, FileD* FD2);
void RdAssign();
Instr* RdWith();
Instr* RdUserFuncAssign();
void ReadProcHead();
Instr* ReadProcBody();
void ReadDeclChpt();
FrmlPtr GetEvalFrml(FrmlPtr X);
