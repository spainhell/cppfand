#pragma once

#include "rdrun.h"
#include "wwmix.h"

bool IsCurrChpt();
void ReleaseFilesAndLinksAfterChapter(EditD* edit);
bool RdFDSegment(WORD FromI, int Pos);
bool ChptDel(EditD* edit);
WORD ChptWriteCRec(EditD* edit); /* 0-O.K., 1-fail, 2-fail && undo*/
bool PromptHelpName(WORD& N);
void EditHelpOrCat(WORD cc, WORD kind, std::string txt);
void StoreChptTxt(FieldDescr* F, std::string text, bool Del);
bool EditExecRdb(std::string Nm, std::string proc_name, Instr_proc* proc_call, wwmix* ww);
void InstallRdb(std::string n);
void CreateOpenChpt(std::string Nm, bool create);