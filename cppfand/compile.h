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
	std::deque<CompInpD> PrevCompInp;
	BYTE* InpArrPtr = nullptr; RdbPos InpRdbPos;
	size_t InpArrLen = 0;
	size_t CurrPos = 0;
	size_t OldErrPos = 0;
	std::vector<FrmlElemSum*> *FrmlSumEl = nullptr;
	bool FrstSumVar = false, FileVarsAllowed = false;
	FrmlElem* (*RdFldNameFrml)(char&) = nullptr; // ukazatel na funkci
	FrmlElem* (*RdFunction)(char&) = nullptr; // ukazatel na funkci
	void(*ChainSumEl)() = nullptr; // {set by user}
};

// funkce dle COMPILE.PAS
bool EquUpCase(pstring& S1, pstring& S2); // r274 ASM
bool EquUpCase(std::string S1, std::string S2);
bool EquUpCase(const char* S);
void Error(integer N); // r1
void SetInpStr(std::string& s); //r31
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
bool IsKeyWord(std::string S); // r284 ASM
//bool TestKeyWord(pstring S); // r282
bool TestKeyWord(std::string S); // r282
bool IsOpt(pstring S); // r296 ASM
bool IsDigitOpt(pstring S, WORD& N); // r305
bool IsIdentifStr(std::string& S); //r323
pstring* RdStrConst(); // r314
std::string RdStringConst(); // r314
char Rd1Char(); // r317
char RdQuotedChar(); // r320
void AcceptKeyWord(const std::string& S); // r293
void RdLocDcl(LocVarBlkD* LVB, bool IsParList, bool WithRecVar, char CTyp); // r1
bool FindLocVar(LocVarBlkD* LVB, LocVar** LV); // r84
bool FindLocVar(LocVar* LVRoot, LocVar** LV); // r84
bool FindChpt(char Typ, const pstring& name, bool local, RdbPos* RP);
void RdChptName(char C, RdbPos* Pos, bool TxtExpr); // r108
std::vector<FieldDescr*> AllFldsList(FileD* FD, bool OnlyStored); // r118
RprtOpt* GetRprtOpt(); // r132
//FrmlPtr GetOp(BYTE Op, integer BytesAfter); // r1
FieldDescr* FindFldName(FileD* FD, std::string fieldName = ""); // r7
FieldDescr* RdFldName(FileD* FD); // r17
FileD* FindFileD(); // r22
FileD* RdFileName(); // r34
LinkD* FindLD(std::string RoleName); // r41
bool IsRoleName(bool Both, FileD** FD, LinkD** LD); // r49
FrmlElem* RdFAccess(FileD* FD, LinkD* LD, char& FTyp); // r58
FrmlElem* TryRdFldFrml(FileD* FD, char& FTyp); // r76
FrmlElem* RdFldNameFrmlF(char& FTyp); // r111
FrmlElem* FrmlContxt(FrmlElem* Z, FileD* FD, void* RP); // r68
FrmlElem* MakeFldFrml(FieldDescr* F, char& FTyp); // r72
void TestString(char FTyp);
void TestReal(char FTyp);
FrmlElem* RdFrml(char& FTyp);
FrmlElem* RdKeyInBool(KeyInD** KIRoot, bool NewMyBP, bool FromRdProc, bool& SQLFilter);
FrmlElem* RdBool();
FrmlElem* RdRealFrml();
FrmlElem* RdStrFrml();
XKey* RdViewKey(); // r238
KeyFldD* RdKF(FileD* FD);
WORD RdKFList(KeyFldD** KFRoot, FileD* FD); // r298
bool IsKeyArg(FieldDescr* F, FileD* FD); // r278
void CompileRecLen();
stSaveState* SaveCompState(); // r104
void RestoreCompState(stSaveState* p); // 109
void CFileLikeFD(FileD* FD, WORD MsgNr);
std::string RdHelpName(); // r144
FrmlElem* RdAttr(); // r152
void RdW(WRectFrml& W); // r161
void RdFrame(FrmlElem** Z, BYTE& WFlags); // r166
bool PromptSortKeys(FieldListEl* FL, KeyFldD* SKRoot);
bool PromptSortKeys(std::vector<FieldDescr*>& FL, KeyFldD* SKRoot);
void RdAssignFrml(char FTyp, bool& Add, FrmlElem** Z); // r193
bool FldTypIdentity(FieldDescr* F1, FieldDescr* F2);
void RdFldList(FieldListEl** FLRoot);
void RdFldList(std::vector<FieldDescr*>& vFields);
void RdFldList(std::vector<FieldDescr*>* vFields);
void RdNegFldList(bool& Neg, FieldListEl** FLRoot); // r214
void RdNegFldList(bool& Neg, std::vector<FieldDescr*>& vFields);
void RdNegFldList(bool& Neg, std::vector<FieldDescr*>* vFields);
void EditModeToFlags(pstring Mode, void* Flgs, bool Err); // r220
