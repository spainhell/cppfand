#pragma once
#include "constants.h"
#include "rdrun.h"
#include "wwmix.h"

bool IsCurrChpt();
void ReleaseFilesAndLinksAfterChapter();
bool RdFDSegment(WORD FromI, int Pos);
bool ChptDel();
WORD ChptWriteCRec(); /* 0-O.K., 1-fail, 2-fail && undo*/
bool PromptHelpName(WORD& N);
void EditHelpOrCat(WORD cc, WORD kind, std::string txt);
void StoreChptTxt(FieldDescr* F, LongStr* S, bool Del);
bool EditExecRdb(std::string Nm, std::string proc_name, Instr_proc* proc_call, wwmix* ww);
void InstallRdb(std::string n);


// for DLL
void CreateOpenChpt(std::string Nm, bool create);
