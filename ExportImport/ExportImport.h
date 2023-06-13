#pragma once
#include "CodingRdb.h"
#include "../CppFand/constants.h"
#include "../CppFand/rdrun.h"
#include "../CppFand/models/Instr.h"


void FileCopy(CopyD* CD);
void MakeMerge(CopyD* CD);

void Backup(bool IsBackup, bool NoCompress, WORD Ir, bool NoCancel);
void BackupM(Instr_backup* PD);

void CheckFile(FileD* FD);

void CodingCRdb(bool Rotate);

bool PromptCodeRdb();

void XEncode(LongStr* S1, LongStr* S2);