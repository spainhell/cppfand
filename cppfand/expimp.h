#pragma once
#include "constants.h"
#include "rdrun.h"


bool OldToNewCat(longint& FilSz);
void XEncode(LongStr* S1, LongStr* S2);
void CopyFile(CopyD* CD);

void Backup(bool IsBackup, bool NoCompress, WORD Ir, bool NoCancel);
void BackupM(Instr* PD);
void CheckFile(FileD* FD);
