#pragma once
#include "FandFile.h"
#include "../Core/FileD.h"
#include "../Core/switches.h"
#ifdef FandSQL
#include "channel.h"
#endif

void RunErrorM(FileD* file_d, LockMode Md, WORD N);
bool TryLMode(FileD* fileD, LockMode Mode, LockMode& OldMode, WORD Kind);
void OldLMode(FileD* fileD, LockMode Mode);
LockMode NewLMode(FileD* fileD, LockMode Mode);
bool TryLockN(FandFile* fand_file, int N, WORD Kind);
void UnLockN(FandFile* fand_file, int N);
bool ChangeLMode(FileD* fileD, LockMode Mode, WORD Kind, bool RdPref);