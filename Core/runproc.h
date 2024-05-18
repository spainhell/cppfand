#pragma once

#include "rdrun.h"
#include "switches.h"
#include "models/Instr.h"

class KeyFldD;

void UserHeadLine(std::string UserHeader);

void ReportProc(RprtOpt* RO, bool save);
void PromptAutoRprt(RprtOpt* RO);
void AssignField(Instr_assign* PD);
void AssignRecVar(LocVar* LV1, LocVar* LV2, AssignD* A);
void AssignRecFld(Instr_assign* PD);
void SortProc(FileD* FD, KeyFldD* SK);
void MergeProc(Instr_merge_display* PD);
void WritelnProc(Instr_writeln* PD);
void DisplayProc(RdbD* R, WORD IRec);
void ClrWwProc(Instr_clrww* PD);
void ExecPgm(Instr_exec* PD);
void CallRdbProc(Instr_call* PD);
void MountProc(WORD CatIRec, bool NoCancel);
void EditProc(Instr_edit* PD);
void EditTxtProc(Instr_edittxt* PD);

void PrintTxtProc(Instr_edittxt* PD);
bool SrchXKey(XKey* K, XString& X, int& N);
void DeleteRecProc(Instr_recs* PD);
void RecallRecProc(Instr_recs* PD);
void AppendRecProc(FileD* file_d);
void UpdRec(FileD* file_d, int rec_nr, bool ad_upd, void* record);
void ReadWriteRecProc(bool IsRead, Instr_recs* PD);
void LinkRecProc(Instr_assign* PD);
void ForAllProc(Instr_forall* PD);
void HeadLineProc(FrmlElem* Z);
void SetKeyBufProc(FrmlElem* Z);
void SetWwViewPort();
void WithWindowProc(Instr_window* PD);
void WithLockedProc(Instr_withshared* PD);
void UnLck(Instr_withshared* PD, LockD* Ld1, PInstrCode Op);

void HelpProc(Instr_help* PD);
HANDLE OpenHForPutTxt(Instr_puttxt* PD);
void PutTxt(Instr_puttxt* PD);
void AssgnCatFld(Instr_assign* PD, void* record);
void AssgnAccRight(Instr_assign* PD);
void AssgnUserName(Instr_assign* PD);
void ReleaseDriveProc(FrmlElem* Z);
void WithGraphicsProc(std::vector<Instr*>& PD);

#ifdef FandGraph
void DrawProc(Instr_graph* PD);
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

void RunInstr(const std::vector<Instr*>& instructions);
void RunProcedure(std::vector<Instr*>& PDRoot);
void CallProcedure(Instr_proc* PD);
void RunMainProc(RdbPos RP, bool NewWw);
