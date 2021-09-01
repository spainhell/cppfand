#pragma once
#include "constants.h"
#include "olongstr.h"
#include "pstring.h"
#include "rdrun.h"

extern RdbPos ChptIPos; // usen in LexAnal & ProjMgr

struct stSaveState
{
	BYTE CurrChar = 0;
	BYTE ForwChar = 0; BYTE ExpChar = 0; BYTE Lexem = 0;
	pstring LexWord;
	bool SpecFDNameAllowed = false, IdxLocVarAllowed = false, FDLocVarAllowed = false, IsCompileErr = false;
	CompInpD* PrevCompInp = nullptr;
	BYTE* InpArrPtr = nullptr; RdbPos InpRdbPos;
	WORD InpArrLen = 0, CurrPos = 0, OldErrPos = 0;
	std::vector<FrmlElemSum*> *FrmlSumEl = nullptr;
	bool FrstSumVar = false, FileVarsAllowed = false;
	FrmlElem* (*RdFldNameFrml)(char&) = nullptr; // ukazatel na funkci
	FrmlElem* (*RdFunction)(char&) = nullptr; // ukazatel na funkci
	void(*ChainSumEl)(); // {set by user}
};

// funkce dle COMPILE.PAS
bool EquUpcase(pstring& S1, pstring& S2); // r274 ASM
bool EquUpcase(std::string& S1, std::string& S2);
bool EquUpcase(const char* S);
void Error(integer N); // r1
void SetInpStr(std::string& S); //r31
void SetInpStdStr(std::string& s, bool ShowErr);
void SetInpLongStr(LongStr* S, bool ShowErr); // r37
void SetInpTTPos(longint Pos, bool Decode); // r46
void SetInpTTPos(FileD* file, longint Pos, bool Decode);
void SetInpTT(RdbPos* RP, bool FromTxt); // r53
void SetInpTTxtPos(FileD* FD);
void ReadChar(); // r73
WORD RdDirective(bool& b); // r81
void RdForwName(pstring& s); // r82
void SkipLevel(bool withElse); // r113
void SkipBlank(bool toNextLine); // r134
void OldError(integer N); // r167
void RdBackSlashCode(); // r170
void RdLex(); // r179
bool IsForwPoint(); // r233
void TestIdentif(); // r235
void TestLex(char X); // r237
void Accept(char X); // r239 ASM
integer RdInteger(); // r245
double RdRealConst(); // r250
bool IsKeyWord(pstring S); // r284 ASM
bool TestKeyWord(pstring S); // r282
bool IsOpt(pstring S); // r296 ASM
bool IsDigitOpt(pstring S, WORD& N); // r305
bool IsIdentifStr(std::string& S); //r323
pstring* RdStrConst(); // r314
char Rd1Char(); // r317
char RdQuotedChar(); // r320
void AcceptKeyWord(pstring S); // r293
void RdLocDcl(LocVarBlkD* LVB, bool IsParList, bool WithRecVar, char CTyp); // r1
bool FindLocVar(LocVarBlkD* LVB, LocVar** LV); // r84
bool FindLocVar(LocVar* LVRoot, LocVar** LV); // r84
bool FindChpt(char Typ, const pstring& name, bool local, RdbPos* RP);
void RdChptName(char C, RdbPos* Pos, bool TxtExpr); // r108
FieldListEl* AllFldsList(FileD* FD, bool OnlyStored); // r118
EditOpt* GetEditOpt(); // r129
RprtOpt* GetRprtOpt(); // r132
//FrmlPtr GetOp(BYTE Op, integer BytesAfter); // r1
FieldDescr* FindFldName(FileD* FD, std::string fieldName = ""); // r7
FieldDescr* RdFldName(FileD* FD); // r17
FileD* FindFileD(); // r22
FileD* RdFileName(); // r34
LinkDPtr FindLD(pstring RoleName); // r41
bool IsRoleName(bool Both, FileD** FD, LinkD** LD); // r49
FrmlElem* RdFAccess(FileD* FD, LinkD* LD, char& FTyp); // r58
FrmlPtr TryRdFldFrml(FileD* FD, char& FTyp); // r76
FrmlElem* RdFldNameFrmlF(char& FTyp); // r111
FrmlPtr FrmlContxt(FrmlPtr Z, FileD* FD, void* RP); // r68
FrmlPtr MakeFldFrml(FieldDescr* F, char& FTyp); // r72
void TestString(char FTyp);
void TestReal(char FTyp);
FrmlPtr RdFrml(char& FTyp);
FrmlElem* RdKeyInBool(KeyInD** KIRoot, bool NewMyBP, bool FromRdProc, bool& SQLFilter);
FrmlPtr RdBool();
FrmlPtr RdRealFrml();
FrmlPtr RdStrFrml();
KeyDPtr RdViewKey(); // r238
KeyFldD* RdKF(FileD* FD);
WORD RdKFList(KeyFldD** KFRoot, FileD* FD); // r298
bool IsKeyArg(FieldDescr* F, FileD* FD); // r278
void CompileRecLen();
stSaveState* SaveCompState(); // r104
void RestoreCompState(stSaveState* p); // 109
void CFileLikeFD(FileD* FD, WORD MsgNr);
std::string* RdHelpName(); // r144
FrmlPtr RdAttr(); // r152
void RdW(WRectFrml& W); // r161
void RdFrame(FrmlElem** Z, BYTE& WFlags); // r166
bool PromptSortKeys(FieldListEl* FL, KeyFldD* SKRoot);
void RdAssignFrml(char FTyp, bool& Add, FrmlElem** Z); // r193
bool FldTypIdentity(FieldDescr* F1, FieldDescr* F2);
void RdFldList(FieldListEl** FLRoot);
void RdNegFldList(bool& Neg, FieldListEl** FLRoot); // r214
void EditModeToFlags(pstring Mode, void* Flgs, bool Err); // r220

// z LEXANAL

//double ValofS(pstring& S); // r251
//void SrchF(FieldDPtr F);
//LocVar* RdVarName(LocVarBlkD* LVB, bool IsParList);
//void SrchZ(FrmlPtr Z);
//void TestBool(char FTyp);
//FrmlPtr RdPrim(char& FTyp);
//bool FindFuncD(FrmlPtr* ZZ);
//bool IsFun(void* XFun, BYTE N, void* XCode, char& FunCode); // ASM
//FrmlPtr RdMult(char& FTyp);
//FrmlPtr RdAdd(char& FTyp);
//FrmlPtr RdComp(char& FTyp);
//WORD RdPrecision();
//WORD RdTilde();
//void RdInConst(FrmlPtr Z, double& R, pstring* S, char& FTyp);
//void StoreConst(double& R, pstring* S, char& FTyp);
//FrmlPtr BOperation(char Typ, char Fun, FrmlPtr Frml);
//FrmlPtr RdBAnd(char& FTyp);
//FrmlPtr RdBOr(char& FTyp);
//FrmlPtr RdFormula(char& FTyp);
//FrmlPtr MyBPContext(FrmlPtr Z, bool NewMyBP);
//FrmlList RdFL(bool NewMyBP, FrmlList FL1);
//LinkDPtr FindOwnLD(FileDPtr FD, const pstring& RoleName); // 77

