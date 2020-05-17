#pragma once

#include "constants.h"
#include "pstring.h"
#include "rdrun.h"
#include "wwmix.h"

bool IsCurrChpt();
void ReleaseFDLDAfterChpt();
bool RdFDSegment(WORD FromI, longint Pos);
bool ChptDel();
WORD ChptWriteCRec(); /* 0-O.K., 1-fail, 2-fail && undo*/
bool PromptHelpName(WORD& N);
void EditHelpOrCat(WORD cc, WORD kind, pstring txt);
void StoreChptTxt(FieldDPtr F, LongStr* S, bool Del);
bool EditExecRdb(pstring* Nm, pstring* ProcNm, Instr* ProcCall, wwmix* ww);
void InstallRdb(pstring n);



//// *** PROJMGR1.PAS ***
//char ExtToTyp(pstring Ext);
//bool NetFileTest(RdbRecVars* X);
//void GetSplitChptName(pstring* Name, pstring* Ext);
//void GetRdbRecVars(void* RecPtr, RdbRecVars* X);
//bool ChptDelFor(RdbRecVars* X);
//void RenameWithOldExt();
//bool IsDuplFileName(pstring name);
//void WrFDSegment(longint RecNr); // ASM
//void* O(void* p); // ASM
//void* OCF(void* p); // ASM
//void* OTb(pstring Nm);
//void* OLinkD(LinkD* Ld);
//void OFrml(FrmlPtr Z);
//void OKF(KeyFldDPtr kf);
//FileD* GetFD(void* p, bool WithSelf, WORD Sg);
//FuncD* GetFC(void* p, WORD Sg);
//LinkD* GetLinkD(void* P, WORD Sg);
//void SgFrml(FrmlPtr Z);
//void SgKF(KeyFldDPtr kf);
//WORD FindHelpRecNr(FileDPtr FD, pstring txt);
//
//// *** PROJMGR.PAS ***
//void SetChptFldDPtr();
//void CreateOpenChpt(pstring* Nm, bool create);
//void SetRdbDir(char Typ, pstring* Nm); // pùvodnì vracel pstring, ale nic z nìj nelezlo :-)
//void ResetRdOnly();
//void CloseChpt();
//void GoCompileErr(WORD IRec, WORD N);
//void ClearXFUpdLock(); // far? proè? je definována ...
//bool CompRunChptRec(WORD CC);
//void Diagnostics(void* MaxHp, longint Free, FileD* FD);
//FileD* FindFD();
//void RdUserId(bool Chk);
//bool CompileRdb(bool Displ, bool Run, bool FromCtrlF10);
//void* RdF(pstring* FileName);
//longint MakeDbfDcl(pstring Nm);
//bool MergeOldNew(bool Veriflongint, bool Pos);
//bool EquStoredF(FieldDPtr F1, FieldDPtr F2);
//bool EquKeys(KeyD* K1, KeyD* K2);
//void DeleteF();
//bool MergAndReplace(FileD* FDOld, FileD* FDNew);
//WORD CompileMsgOn(WORD* Buf, longint& w);
//void CompileMsgOff(WORD* Buf, longint& w);
//void GotoErrPos(WORD& Brk);
//void WrErrMsg630(pstring* Nm);
//void UpdateUTxt();
//void UpdateCat();
