#pragma once
#include "constants.h"
#include "pstring.h"
#include "rdrun.h"
#include "models/Instr.h"

void UserHeadLine(pstring UserHeader);

void ReportProc(RprtOpt* RO, bool save);
void PromptAutoRprt(RprtOpt* RO);
void AssignField(Instr_assign* PD);
void AssignRecVar(LocVar* LV1, LocVar* LV2, AssignD* A);
void AssignRecFld(Instr_assign* PD);
void SortProc(FileDPtr FD, KeyFldDPtr SK);
void MergeProc(Instr_proc* PD);
void WritelnProc(Instr_writeln* PD);
void DisplayProc(RdbDPtr R, WORD IRec);
void ClrWwProc(Instr* PD);
void ExecPgm(Instr_exec* PD);
void CallRdbProc(Instr_call* PD);
void IndexfileProc(FileDPtr FD, bool Compress);
void MountProc(WORD CatIRec, bool NoCancel);
void EditProc(Instr_edit* PD);
void EditTxtProc(Instr_edittxt* PD);
pstring* GetStr(FrmlPtr Z);

void PrintTxtProc(Instr_edittxt* PD);
bool SrchXKey(KeyDPtr K, XString& X, longint& N);
void DeleteRecProc(Instr_recs* PD);
void RecallRecProc(Instr_recs* PD);
void AppendRecProc();
void UpdRec(void* CR, longint N, bool AdUpd);
void ReadWriteRecProc(bool IsRead, Instr_recs* PD);
void LinkRecProc(Instr_assign* PD);
void ForAllProc(Instr* PD);
void HeadLineProc(FrmlPtr Z);
void SetKeyBufProc(FrmlPtr Z);
void SetWwViewPort();
void WithWindowProc(Instr* PD);
void WithLockedProc(Instr* PD);
void UnLck(Instr* PD, LockD* Ld1, PInstrCode Op);

void HelpProc(Instr_help* PD);
FILE* OpenHForPutTxt(Instr_puttxt* PD);
void PutTxt(Instr_puttxt* PD);
void AssgnCatFld(Instr_assign* PD);
void AssgnAccRight(Instr_assign* PD);
void AssgnUserName(Instr_assign* PD);
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
void CallProcedure(Instr_proc* PD); // TODO: nìjaké ukazatele
void RunMainProc(RdbPos RP, bool NewWw);
