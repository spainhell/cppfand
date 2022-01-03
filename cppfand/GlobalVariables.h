#pragma once
#include "access.h"
#include "constants.h"
#include "pstring.h"
#include "TFile.h"

extern pstring LockModeTxt[9];

// r474
extern FileD* FileDRoot; // { only current RDB }
extern std::deque<LinkD*> LinkDRoot; // { for all RDBs     }
extern FuncD* FuncDRoot;
extern FileD* CFile;
extern void* CRecPtr;
extern XKey* CViewKey;
extern std::string TopRdbDir, TopDataDir;
extern pstring CatFDName;
extern RdbD* CRdb, *TopRdb;
extern FileD* CatFD, *HelpFD;

// r483
extern structXPath XPath[10];
extern WORD XPathN;
extern XWFile XWork;
extern TFile TWork;
extern longint ClpBdPos;
extern bool IsTestRun;
extern bool IsInstallRun;

extern FileD* Chpt; // absolute FileDRoot;
extern TFile* ChptTF;
extern FieldDescr* ChptTxtPos;
extern FieldDescr* ChptVerif; // { updated record }
extern FieldDescr* ChptOldTxt; // { ChptTyp = 'F' : -1 = new unchecked record, else = old declaration }
extern FieldDescr* ChptTyp, *ChptName, *ChptTxt;


// ********** konstanty ********** // r496
extern bool EscPrompt;
extern pstring UserName;
extern pstring UserPassWORD;
extern pstring AccRight;
extern bool EdUpdated;
extern longint EdRecNo;
extern pstring EdRecKey;
extern pstring EdKey;
extern bool EdOk;
extern pstring EdField;
extern longint LastTxtPos;
extern longint TxtXY;
// { consecutive WORD - sized / for formula access / }
extern WORD RprtLine; extern WORD RprtPage; extern WORD PgeLimit; // {report}
extern WORD EdBreak;
extern WORD EdIRec; // {common - alphabetical order}
extern WORD MenuX; extern WORD MenuY;
extern WORD UserCode;
// **********

// extern WORD* WordVarArr;
extern FieldDescr* CatRdbName, *CatFileName, *CatArchiv, *CatPathName, *CatVolume;
extern std::string MountedVol[FloppyDrives];

extern pstring SQLDateMask;

// ********** COMPARE FUNCTIONS **********
extern double Power10[21];

extern BYTE CurrChar; // { Compile }
extern BYTE ForwChar, ExpChar, Lexem;
extern pstring LexWord;

extern bool SpecFDNameAllowed, IdxLocVarAllowed, FDLocVarAllowed, IsCompileErr;
//extern CompInpD* PrevCompInp;						// { saved at "include" }
extern std::deque<CompInpD> PrevCompInp;						// { saved at "include" }
extern BYTE* InpArrPtr; extern RdbPos InpRdbPos;		// { "  "  }
extern size_t InpArrLen;
extern size_t CurrPos;
extern size_t OldErrPos;			// { "  "  }
extern std::vector<FrmlElemSum*> *FrmlSumEl;				//{ set while reading sum / count argument }
extern bool FrstSumVar, FileVarsAllowed;
extern FrmlElem* (*RdFldNameFrml)(char&); // ukazatel na funkci
extern FrmlElem* (*RdFunction)(char&); // ukazatel na funkci
extern void(*ChainSumEl)(); // {set by user}
extern BYTE LstCompileVar; // { boundary }

extern pstring Switches;
extern WORD SwitchLevel;

extern pstring LockModeTxt[9];

// ***** BASE.H *****
extern char Version[5];
extern WORD CachePageSize;
extern void* AfterCatFD; // r108
extern ExitRecord ExitBuf; // r202 - r210
extern ProcStkD* MyBP;
extern ProcStkD* ProcMyBP;
extern WORD BPBound; // r212
extern bool ExitP, BreakP;
extern longint LastExitCode; // r215
extern WORD HandleError; // r229
extern pstring OldDir;
extern pstring FandDir;
extern std::string WrkDir;
extern pstring FandOvrName;
extern pstring FandResName;
extern pstring FandWorkName;
extern pstring FandWorkXName;
extern pstring FandWorkTName;
extern std::string CPath;
extern std::string CDir;
extern std::string CName;
extern std::string CExt;
extern std::string CVol;
extern bool WasLPTCancel;
extern FILE* WorkHandle;
extern longint MaxWSize; // {currently occupied in FANDWORK.$$$}
// *** MESSAGES ***
extern WORD F10SpecKey; // ø. 293
extern BYTE ProcAttr;
// extern bool SetStyleAttr(char c, BYTE& a); // je v KBDWW
extern std::string MsgLine;
extern std::string MsgPar[4];
// ********** DML **********
extern void* FandInt3f; // ø. 311
extern FILE* OvrHandle;
extern WORD Fand_ss, Fand_sp, Fand_bp, DML_ss, DML_sp, DML_bp;
extern longint _CallDMLAddr; // {passed to FANDDML by setting "DMLADDR="in env.}

extern wdaystt WDaysTabType;
extern WORD NWDaysTab;
extern double WDaysFirst;
extern double WDaysLast;
extern wdaystt* WDaysTab;

extern char AbbrYes;
extern char AbbrNo;

extern char CharOrdTab[256]; // after Colors /FANDDML/ // ø. 370
extern char UpcCharTab[256]; // TODO: v obou øádcích bylo 'array[char] of char;' - WTF?
extern WORD TxtCols, TxtRows;

extern integer prCurr, prMax;
extern Printer printer[10];

extern TPrTimeOut OldPrTimeOut;
extern TPrTimeOut PrTimeOut;  // absolute 0:$478;
extern bool WasInitDrivers;
extern bool WasInitPgm;
extern WORD LANNode; // ø. 431
extern void (*CallOpenFandFiles)(); // r453
extern void (*CallCloseFandFiles)(); // r454

extern double userToday;

//TMsgIdxItem TMsgIdx[100];
extern TResFile ResFile;
extern TMsgIdxItem* MsgIdx;// = TMsgIdx;
extern WORD MsgIdxN;
extern longint FrstMsgPos;