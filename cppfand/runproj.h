#pragma once

#include "compile.h"
#include "constants.h"
#include "pstring.h"
#include "rdrun.h"

//EditD* E = (EditD*)&EditDRoot;
longint UserW = 0;

// *** PROJMGR1.PAS ***

bool IsCurrChpt();
char ExtToTyp(pstring Ext);
void ReleaseFDLDAfterChpt();

struct RdbRecVars
{
	char Typ = 0; pstring Name = pstring(12); pstring Ext;
	longint Txt = 0; longint OldTxt = 0;
	char FTyp = 0; WORD CatIRec = 0; bool isSQL = false;
};

bool NetFileTest(RdbRecVars* X);
void GetSplitChptName(pstring* Name, pstring* Ext);
void GetRdbRecVars(void* RecPtr, RdbRecVars* X);
bool ChptDelFor(RdbRecVars* X);
bool ChptDel();
WORD ChptWriteCRec(); /* 0-O.K., 1-fail, 2-fail && undo*/
void RenameWithOldExt();
bool IsDuplFileName(pstring name);

FileD* CFileF;
longint sz; WORD nTb; void* Tb;

void WrFDSegment(longint RecNr); // ASM
void* O(void* p); // ASM
void* OCF(void* p); // ASM
void* OTb(pstring Nm);
void* OLinkD(LinkD* Ld);
void OFrml(FrmlPtr Z);
void OKF(KeyFldDPtr kf);

bool RdFDSegment(WORD FromI, longint Pos);
FileD* GetFD(void* p, bool WithSelf, WORD Sg);
FuncD* GetFC(void* p, WORD Sg);
LinkD* GetLinkD(void* P, WORD Sg);
void SgFrml(FrmlPtr Z);
void SgKF(KeyFldDPtr kf);
WORD FindHelpRecNr(FileDPtr FD, pstring txt);
bool PromptHelpName(WORD& N);
void EditHelpOrCat(WORD cc, WORD kind, pstring txt);
void StoreChptTxt(FieldDPtr F, LongStr* S, bool Del);

// *** PROJMGR.PAS ***
void SetChptFldDPtr();
void CreateOpenChpt(pstring* Nm, bool create);
void SetRdbDir(char Typ, pstring* Nm); // pùvodnì vracel pstring, ale nic z nìj nelezlo :-)
void ResetRdOnly();
void CloseChpt();
void GoCompileErr(WORD IRec, WORD N);
void ClearXFUpdLock(); // far? proè? je definována ...
bool CompRunChptRec(WORD CC);
void Diagnostics(void* MaxHp, longint Free, FileD* FD);
FileD* FindFD();
void RdUserId(bool Chk);
bool CompileRdb(bool Displ, bool Run, bool FromCtrlF10);
void* RdF(pstring* FileName);
longint MakeDbfDcl(pstring Nm);
bool MergeOldNew(bool Veriflongint, bool Pos);
bool EquStoredF(FieldDPtr F1, FieldDPtr F2);
bool EquKeys(KeyD* K1, KeyD* K2);
void DeleteF();
bool MergAndReplace(FileD* FDOld, FileD* FDNew);
WORD CompileMsgOn(WORD* Buf, longint& w);
void CompileMsgOff(WORD* Buf, longint& w);
void GotoErrPos(WORD& Brk);
bool EditExecRdb(pstring* Nm, pstring* ProcNm, Instr* ProcCall);
void WrErrMsg630(pstring* Nm);
void InstallRdb(pstring n);
void UpdateUTxt();
void UpdateCat();

