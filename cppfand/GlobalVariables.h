#pragma once
#include "access.h"
#include "constants.h"
#include "pstring.h"

struct FuncD;
struct LinkD;
class FileD;

// r474
extern FileD* FileDRoot; // { only current RDB }
extern LinkD* LinkDRoot; // { for all RDBs     }
extern FuncD* FuncDRoot;
extern FileD* CFile;
extern void* CRecPtr;
extern KeyD* CViewKey;
extern std::string TopRdbDir, TopDataDir;
extern pstring CatFDName;
extern RdbD* CRdb, TopRdb;
extern FileD* CatFD, HelpFD;

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
extern FieldDPtr ChptTxtPos;
extern FieldDPtr ChptVerif; // { updated record }
extern FieldDPtr ChptOldTxt; // { ChptTyp = 'F' : -1 = new unchecked record, else = old declaration }
extern FieldDPtr ChptTyp, ChptName, ChptTxt;


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
extern WORD EdBreak; extern WORD EdIRec; // {common - alphabetical order}
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
extern CompInpD* PrevCompInp;						// { saved at "include" }
extern BYTE* InpArrPtr; extern RdbPos InpRdbPos;		// { "  "  }
extern size_t InpArrLen;
extern size_t CurrPos;
extern size_t OldErrPos;			// { "  "  }
extern SumElPtr FrmlSumEl;				//{ set while reading sum / count argument }
extern bool FrstSumVar, FileVarsAllowed;
extern FrmlElem* (*RdFldNameFrml)(char&); // ukazatel na funkci
extern FrmlElem* (*RdFunction)(char&); // ukazatel na funkci
extern void(*ChainSumEl)(); // {set by user}
extern BYTE LstCompileVar; // { boundary }

extern pstring Switches;
extern WORD SwitchLevel;

extern pstring LockModeTxt[9];