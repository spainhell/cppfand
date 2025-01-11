#include "GlobalVariables.h"

class FileD;

WORD randIndex = 0;

FileD* CFile;
//std::vector<FileD*> FileDRoot;	// only current RDB
std::deque<LinkD*> LinkDRoot;	// for all RDBs
std::deque<FuncD*> FuncDRoot;
void* CRecPtr;
XKey* CViewKey;
std::string TopRdbDir, TopDataDir;
std::string CatFDName;
RdbD* CRdb;
RdbD* TopRdb;
Catalog* catalog = nullptr;
FileD* HelpFD;
//size_t InpArrLen, CurrPos, OldErrPos;

std::string LockModeTxt[9] = { "NULL", "NOEXCL", "NODEL", "NOCR", "RD", "WR", "CR", "DEL", "EXCL" };

structXPath XPath[20];
WORD XPathN;

FandXFile XWork(nullptr);
FandTFile TWork(nullptr);
int ClpBdPos = 0;
bool IsTestRun = false;
bool IsInstallRun = false;
FileD* Chpt = nullptr; // absolute FileDRoot;
FandTFile* ChptTF;
FieldDescr* ChptTxtPos;
FieldDescr* ChptVerif; // { updated record }
FieldDescr* ChptOldTxt; // { ChptTyp = 'F' : -1 = new unchecked record, else = old declaration }
FieldDescr* ChptTyp, *ChptName, *ChptTxt;
bool EscPrompt = false;
//pstring UserName = pstring(20);
//pstring UserPassWORD = pstring(20);
//pstring AccRight;
std::unique_ptr<Users> user = std::make_unique<Users>();
bool EdUpdated = false;
int EdRecNo = 0;
pstring EdRecKey = "";
pstring EdKey = pstring(32);
bool EdOk = false;
pstring EdField = pstring(32);
int32_t LastTxtPos = 0;
int TxtXY = 0;
// { consecutive WORD - sized / for formula access / }
WORD RprtLine = 0; WORD RprtPage = 0; WORD PgeLimit = 0; // {report}
WORD EdBreak = 0;
WORD EdIRec = 1; // {common - alphabetical order}
WORD MenuX = 1; WORD MenuY = 1;
//WORD UserCode = 0;
// WORD* WordVarArr = &RprtLine;
std::string MountedVol[FloppyDrives];
pstring SQLDateMask = "DD.MM.YYYY hh:mm:ss";
double Power10[21] = { 1E0, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7, 1E8, 1E9, 1E10,
	1E11, 1E12, 1E13, 1E14, 1E15, 1E16, 1E17, 1E18, 1E19, 1E20 };
bool SpecFDNameAllowed, IdxLocVarAllowed, FDLocVarAllowed, IsCompileErr;
//CompInpD* PrevCompInp = nullptr;			// { saved at "include" }
std::deque<CompInpD> PrevCompInp;			// { saved at "include" }
//BYTE* InpArrPtr;		// { "  "  }
RdbPos InpRdbPos;		// { "  "  }
std::vector<FrmlElemSum*> *FrmlSumEl;				//{ set while reading sum / count argument }
bool FrstSumVar, FileVarsAllowed;

//FrmlElem* (*ptrRdFldNameFrml)(char&, MergeReportBase*) = nullptr; // ukazatel na funkci
//FrmlElem* (*RdFunction)(void*, char&) = nullptr; // ukazatel na funkci
//void (*ptrChainSumEl)(); // {set by user}
BYTE LstCompileVar; // { boundary }

pstring Switches = "";
WORD SwitchLevel = 0;

char Version[] = { '4', '.', '2', '0', '\0' };
