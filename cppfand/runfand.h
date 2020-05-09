#pragma once
#include "pstring.h"
#include "constants.h"
#include "switches.h"

void ScrGraphMode(bool Redraw, WORD OldScrSeg);
WORD ScrTextMode(bool Redraw, bool Switch);

void InitRunFand(); // !!! spuštìní - vstupní procedura
//bool IsAT();
//void OpenXMS();
//void OpenCache();
//void DetectVideoCard();
//void InitDrivers();
//void InitAccess();
//void RdCFG();
//void RdColors(FILE* CfgHandle);
//void RdPrinter(FILE* CfgHandle);
//void RdWDaysTab(FILE* CfgHandle);
//void CompileHelpCatDcl();
//
//bool SetTopDir(pstring& path, pstring& name);
//void RunRdb(pstring path);
//void SelectRunRdb(bool OnFace);
//void CallInstallRdb();
//void CallEditTxt();
//void SelectEditTxt(pstring E, bool OnFace);
