#pragma once
#include "constants.h"
#include "pstring.h"
#include "rdrun.h"
#include "models/Instr.h"

void UserHeadLine(pstring UserHeader);

void ReportProc(RprtOpt* RO, bool save);
void PromptAutoRprt(RprtOpt* RO);
void AssignField(Instr* PD);
void AssignRecVar(LocVar* LV1, LocVar* LV2, AssignD* A);
void AssignRecFld(Instr* PD);
void SortProc(FileDPtr FD, KeyFldDPtr SK);
void MergeProc(Instr* PD);
void WritelnProc(Instr* PD);
void DisplayProc(RdbDPtr R, WORD IRec);
void ClrWwProc(Instr* PD);
void ExecPgm(Instr* PD);
void CallRdbProc(Instr* PD);
void IndexfileProc(FileDPtr FD, bool Compress);
void MountProc(WORD CatIRec, bool NoCancel);
void EditProc(Instr* PD);
void EditTxtProc(Instr* PD);
pstring* GetStr(FrmlPtr Z);

void PrintTxtProc(Instr* PD);
bool SrchXKey(KeyDPtr K, XString& X, longint& N);
void DeleteRecProc(Instr* PD);
void RecallRecProc(Instr* PD);
void AppendRecProc();
void UpdRec(void* CR, longint N, bool AdUpd);
void ReadWriteRecProc(bool IsRead, Instr* PD);
void LinkRecProc(Instr* PD);
void ForAllProc(Instr* PD);
void HeadLineProc(FrmlPtr Z);
void SetKeyBufProc(FrmlPtr Z);
void SetWwViewPort();
void WithWindowProc(Instr* PD);
void WithLockedProc(Instr* PD);
void UnLck(Instr* PD, LockD* Ld1, PInstrCode Op);

void HelpProc(Instr_menubox_menubar* PD);
FILE* OpenHForPutTxt(Instr* PD);
void PutTxt(Instr* PD);
void AssgnCatFld(Instr* PD);
void AssgnAccRight(Instr* PD);
void AssgnUserName(Instr* PD);
void ReleaseDriveProc(FrmlPtr Z);
void WithGraphicsProc(Instr* PD);

#ifdef FandGraph
void DrawProc(Instr* PD);
#endif

void ResetCatalog();

#ifdef FandSQL
void SQLProc(FrmlPtr Z);
void StartLogIn(FrmlPtr Nm, FrmlPtr Pw);
void SQLRdWrTxt(Instr* PD);
#endif

void PortOut(bool IsWord, WORD Port, WORD What); // ASM
void WaitProc();

#ifndef FandRunV
void MemDiagProc();
#endif

void RunInstr(Instr* PD);
void RunProcedure(void* PDRoot);
void CallProcedure(Instr* PD); // TODO: nìjaké ukazatele
void RunMainProc(RdbPos RP, bool NewWw);
