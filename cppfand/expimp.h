#pragma once
#include "constants.h"
#include "rdrun.h"

class Instr_backup;

bool OldToNewCat(longint& FilSz);

inline void XEncode(LongStr* S1, LongStr* S2)
{
}

inline void CopyFileE(CopyD* CD)
{
}

inline void Backup(bool IsBackup, bool NoCompress, WORD Ir, bool NoCancel)
{
}

inline void BackupM(Instr_backup* PD)
{
}

void CheckFile(FileD* FD);

inline void CodingCRdb(bool Rotate)
{
}

inline bool PromptCodeRdb()
{
	return false;
}
