#pragma once
#include "../Core/rdrun.h"
#include "../Core/models/Instr.h"

void FileCopy(CopyD* CD);
void MakeMerge(CopyD* CD);
void BackUp(bool IsBackup, bool NoCompress, WORD Ir, bool NoCancel);
void BackupM(Instr_backup* PD);
void CheckFile(FileD* FD);
void CodingCRdb(EditD* edit, bool rotate);
bool PromptCodeRdb(EditD* edit);
