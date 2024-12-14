#pragma once
#include <memory>

#include "access.h"
#include "Catalog.h"

#include "ResFile.h"
#include "Users.h"
#include "../fandio/FandTFile.h"

class MergeReportBase;
const double FirstDate = 6.97248E+5;

extern std::string LockModeTxt[9];

// r474
//extern std::vector<FileD*> FileDRoot; // { only current RDB }
extern std::deque<LinkD*> LinkDRoot;  // { for all RDBs     }
extern std::deque<FuncD*> FuncDRoot;
extern FileD* CFile;
extern void* CRecPtr;
extern XKey* CViewKey;
extern std::string TopRdbDir, TopDataDir;
extern std::string CatFDName;
extern RdbD* CRdb, *TopRdb;
extern Catalog* catalog;
extern FileD* HelpFD;

// r483
extern structXPath XPath[20];
extern WORD XPathN;
extern XWFile XWork;
extern FandTFile TWork;
extern int ClpBdPos;
extern bool IsTestRun;
extern bool IsInstallRun;

extern FileD* Chpt; // absolute FileDRoot;
extern FandTFile* ChptTF;
extern FieldDescr* ChptTxtPos;
extern FieldDescr* ChptVerif; // { updated record }
extern FieldDescr* ChptOldTxt; // { ChptTyp = 'F' : -1 = new unchecked record, else = old declaration }
extern FieldDescr* ChptTyp, *ChptName, *ChptTxt;


// ********** konstanty ********** // r496
extern bool EscPrompt;
//extern pstring UserName;
//extern pstring UserPassWORD;
//extern pstring AccRight;
extern std::unique_ptr<Users> user;
extern bool EdUpdated;
extern int EdRecNo;
extern pstring EdRecKey;
extern pstring EdKey;
extern bool EdOk;
extern pstring EdField;
extern int LastTxtPos;
extern int TxtXY;
// { consecutive WORD - sized / for formula access / }
extern WORD RprtLine; extern WORD RprtPage; extern WORD PgeLimit; // {report}
extern WORD EdBreak;
extern WORD EdIRec; // {common - alphabetical order}
extern WORD MenuX; extern WORD MenuY;
//extern WORD UserCode;
// **********

// extern WORD* WordVarArr;
extern std::string MountedVol[FloppyDrives];

extern pstring SQLDateMask;

// ********** COMPARE FUNCTIONS **********
extern double Power10[21];

extern BYTE CurrChar; // { Compile }
extern BYTE ForwChar, ExpChar, Lexem;
extern pstring LexWord;

extern bool SpecFDNameAllowed, IdxLocVarAllowed, FDLocVarAllowed, IsCompileErr;
//extern CompInpD* PrevCompInp;						    // { saved at "include" }
extern std::deque<CompInpD> PrevCompInp;				// { saved at "include" }
extern BYTE* InpArrPtr; extern RdbPos InpRdbPos;		// { "  "  }
extern size_t InpArrLen;
extern size_t CurrPos;
extern size_t OldErrPos;			// { "  "  }
extern std::vector<FrmlElemSum*> *FrmlSumEl;				//{ set while reading sum / count argument }
extern bool FrstSumVar, FileVarsAllowed;
//extern FrmlElem* (*ptrRdFldNameFrml)(char&, MergeReportBase*); // ukazatel na funkci
extern FrmlElem* (*RdFunction)(char&); // ukazatel na funkci
//extern void (*ptrChainSumEl)(); // {set by user}
extern BYTE LstCompileVar; // { boundary }

extern pstring Switches;
extern WORD SwitchLevel;


// ***** BASE.H *****
extern char Version[5];
extern void* AfterCatFD; // r108
//extern WORD BPBound; // r212
extern bool ExitP, BreakP;
extern int LastExitCode; // r215
extern unsigned long HandleError; // r229
extern std::string OldDir;
extern std::string FandDir;
extern std::string WrkDir;
extern std::string FandResName;
extern std::string FandWorkName;
extern std::string FandWorkXName;
extern std::string FandWorkTName;
extern std::string CPath;
extern std::string CDir;
extern std::string CName;
extern std::string CExt;
extern std::string CVol;
extern bool WasLPTCancel;
extern HANDLE WorkHandle;
extern int MaxWSize; // {currently occupied in FANDWORK.$$$}
// *** MESSAGES ***
extern WORD F10SpecKey; // r. 293
extern BYTE ProcAttr;
// extern bool SetStyleAttr(char c, BYTE& a); // je v KBDWW
extern std::string MsgLine;
extern std::string MsgPar[4];

extern wdaystt WDaysTabType;
extern WORD NWDaysTab;
extern double WDaysFirst;
extern double WDaysLast;
extern wdaystt* WDaysTab;

extern char AbbrYes;
extern char AbbrNo;

extern WORD TxtCols, TxtRows;

extern short prCurr, prMax;
extern Printer printer[10];

extern TPrTimeOut OldPrTimeOut;
extern TPrTimeOut PrTimeOut;  // absolute 0:$478;
extern bool WasInitDrivers;
extern bool WasInitPgm;
extern WORD LANNode;
extern void (*CallOpenFandFiles)(); // r453
extern void (*CallCloseFandFiles)(); // r454

extern double userToday;
extern __int32 UserLicNr;

extern ResFile resFile;
