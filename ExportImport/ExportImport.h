#pragma once
#include "../cppfand/constants.h"
#include "../cppfand/rdrun.h"
#include "../cppfand/models/Instr.h"


bool OldToNewCat(longint& FilSz);

void FileCopy(CopyD* CD);
void MakeMerge(CopyD* CD);

void Backup(bool IsBackup, bool NoCompress, WORD Ir, bool NoCancel);
void BackupM(Instr_backup* PD);

void CheckFile(FileD* FD);

inline void CodingCRdb(bool Rotate)
{
}

bool PromptCodeRdb();

inline void XEncode(LongStr* S1, LongStr* S2)
{
}