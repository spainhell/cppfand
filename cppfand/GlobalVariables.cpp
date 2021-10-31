#include "GlobalVariables.h"

class FileD;

WORD randIndex = 0;

FileD* CFile;
FieldDescr* CatRdbName, * CatFileName, * CatArchiv, * CatPathName, * CatVolume;
FileD* FileDRoot; // { only current RDB }
std::vector<LinkD*> LinkDRoot; // { for all RDBs     }
FuncD* FuncDRoot;
void* CRecPtr;
XKey* CViewKey;
std::string TopRdbDir, TopDataDir;
pstring CatFDName;
RdbD* CRdb, TopRdb;
FileD* CatFD, *HelpFD;
size_t InpArrLen, CurrPos, OldErrPos;

pstring LockModeTxt[9] = { "NULL", "NOEXCL", "NODEL", "NOCR", "RD", "WR", "CR", "DEL", "EXCL" };

structXPath XPath[10];
WORD XPathN;

XWFile XWork;
TFile TWork;
longint ClpBdPos = 0;
bool IsTestRun = false;
bool IsInstallRun = false;
FileD* Chpt = FileDRoot; // absolute FileDRoot;
TFile* ChptTF;
FieldDescr* ChptTxtPos;
FieldDescr* ChptVerif; // { updated record }
FieldDescr* ChptOldTxt; // { ChptTyp = 'F' : -1 = new unchecked record, else = old declaration }
FieldDescr* ChptTyp, *ChptName, *ChptTxt;
bool EscPrompt = false;
pstring UserName = pstring(20);
pstring UserPassWORD = pstring(20);
pstring AccRight;
bool EdUpdated = false;
longint EdRecNo = 0;
pstring EdRecKey = "";
pstring EdKey = pstring(32);
bool EdOk = false;
pstring EdField = pstring(32);
longint LastTxtPos = 0;
longint TxtXY = 0;
// { consecutive WORD - sized / for formula access / }
WORD RprtLine = 0; WORD RprtPage = 0; WORD PgeLimit = 0; // {report}
WORD EdBreak = 0;
WORD EdIRec = 1; // {common - alphabetical order}
WORD MenuX = 1; WORD MenuY = 1;
WORD UserCode = 0;
// WORD* WordVarArr = &RprtLine;
std::string MountedVol[FloppyDrives];
pstring SQLDateMask = "DD.MM.YYYY hh:mm:ss";
double Power10[21] = { 1E0, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7, 1E8, 1E9, 1E10,
	1E11, 1E12, 1E13, 1E14, 1E15, 1E16, 1E17, 1E18, 1E19, 1E20 };
bool SpecFDNameAllowed, IdxLocVarAllowed, FDLocVarAllowed, IsCompileErr;
CompInpD* PrevCompInp = nullptr;			// { saved at "include" }
BYTE* InpArrPtr; RdbPos InpRdbPos;		// { "  "  }
std::vector<FrmlElemSum*> *FrmlSumEl;				//{ set while reading sum / count argument }
bool FrstSumVar, FileVarsAllowed;

FrmlElem* (*RdFldNameFrml)(char&) = nullptr; // ukazatel na funkci
FrmlElem* (*RdFunction)(char&) = nullptr; // ukazatel na funkci
void(*ChainSumEl)(); // {set by user}
BYTE LstCompileVar; // { boundary }

pstring Switches = "";
WORD SwitchLevel = 0;

char Version[] = { '4', '.', '2', '0', '\0' };
