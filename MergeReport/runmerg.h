#pragma once
#include "../cppfand/access.h"
#include "../cppfand/constants.h"
#include "../cppfand/rdrun.h"

extern longint NRecsAll;
void RunMerge();
WORD CompMFlds(KeyFldD* M);
void SetOldMFlds(KeyFldD* M);
void SetMFlds(KeyFldD* M);
void ReadInpFileM(InpD* ID); // stejná implementace v runrprt.cpp -> pøidáno M na konci
void RunAssign(AssignD* A);
void WriteOutp(OutpRD* RD);
void OpenInpM(); // JINÁ implementace v runrprt.cpp -> pøidáno M na konci
void OpenOutp();
void CloseInpOutp();
void MoveForwToRecM(InpD* ID); // implementace v runrprt.cpp->pøidáno M na konci
void MergeProcM(); // implementace v runrprt.cpp -> pøidáno M na konci
void JoinProc(WORD Ii, bool& EmptyGroup);

