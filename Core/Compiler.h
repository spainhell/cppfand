#pragma once

#include "../Common/pstring.h"
#include "rdrun.h"
#include "../MergeReport/MergeReportBase.h"
#include "../MergeReport/RprtOpt.h"

class Compiler;
extern RdbPos ChptIPos; // usen in LexAnal & ProjMgr
extern Compiler* gc; // global g_compiler instance
extern bool IsRdUserFunc;

enum class FieldNameType { none, F, P, T };
enum class ReadFuncType { none, P };

struct stSaveState
{
	BYTE CurrChar = 0;
	BYTE ForwChar = 0; BYTE ExpChar = 0; BYTE Lexem = 0;
	std::string lex_word;
	bool SpecFDNameAllowed = false, IdxLocVarAllowed = false, FDLocVarAllowed = false, IsCompileErr = false;
	std::deque<CompInpD> PrevCompInp;
	std::string InputString;
	RdbPos InpRdbPos;
	size_t CurrPos = 0;
	size_t OldErrPos = 0;
	std::vector<FrmlElemSum*> *FrmlSumEl = nullptr;
	bool FrstSumVar = false, FileVarsAllowed = false;
	//FrmlElem* (*ptrRdFldNameFrml)(char&, MergeReportBase*) = nullptr; // ukazatel na funkci
	FieldNameType RdFldNameType = FieldNameType::none;
	ReadFuncType RdFuncType = ReadFuncType::none;
	FileD* processed_file = nullptr;
};

class Compiler {
public:
	Compiler();
	Compiler(std::string& input);
	Compiler(FileD* file_d);
	Compiler(FileD* file_d, std::string& input);
	~Compiler();
	std::string Error(short N);
	void SetInpStr(std::string& s);
	void SetInpStdStr(std::string& s, bool ShowErr);
	void SetInpTTPos(FileD* file_d, int Pos, bool Decode);
	void SetInpTT(RdbPos* rdb_pos, bool FromTxt);
	void SetInpTTxtPos(FileD* file_d);
	void ResetCompilePars();
	void ReadChar();
	WORD RdDirective(bool& b);
	void RdForwName(pstring& s);
	void SkipLevel(bool withElse);
	void SkipBlank(bool toNextLine);
	void OldError(short N);
	void RdBackSlashCode();
	void RdLex();
	bool IsForwPoint();
	void TestIdentif();
	void TestLex(char X);
	void Accept(char X);
	short RdInteger();
	double RdRealConst();
	bool IsKeyWord(const std::string& S);
	bool TestKeyWord(const std::string& S);
	bool IsOpt(pstring S);
	bool IsDigitOpt(pstring S, WORD& N);
	bool IsIdentifStr(std::string& S);
	std::string RdStringConst();
	char Rd1Char();
	char RdQuotedChar();
	void AcceptKeyWord(const std::string& S);
	void RdLocDcl(LocVarBlock* LVB, bool IsParList, bool WithRecVar, char CTyp);
	bool FindLocVar(LocVarBlock* LVB, LocVar** LV);
	//LocVar* FindLocVar(std::vector<LocVar*>& LVRoot);
	bool FindChpt(char Typ, const pstring& name, bool local, RdbPos* RP);
	std::string RdChptName(char C, RdbPos* Pos, bool TxtExpr);
	std::vector<FieldDescr*> AllFldsList(FileD* FD, bool OnlyStored);
	RprtOpt* GetRprtOpt();
	FieldDescr* FindFldName(FileD* FD, std::string fieldName = "");
	FieldDescr* RdFldName(FileD* FD);
	FileD* FindFileD();
	FileD* RdFileName();
	LinkD* FindLD(FileD* file_d, std::string RoleName);
	bool IsRoleName(bool both, FileD* file_d, FileD** up_file_d, LinkD** link);
	FrmlElem* RdFAccess(FileD* FD, LinkD* LD, char& FTyp);
	FrmlElem* TryRdFldFrml(FileD* FD, char& FTyp, MergeReportBase* caller);
	FrmlElem* RdFldNameFrmlF(char& FTyp, MergeReportBase* caller);
	FrmlElem* RdFldNameFrmlT(char& FTyp, MergeReportBase* caller);
	FrmlElem* FrmlContxt(FrmlElem* Z, FileD* FD, void* RP);
	FrmlElem* MakeFldFrml(FieldDescr* F, char& FTyp);
	void TestString(char FTyp);
	void TestReal(char FTyp);
	FrmlElem* RdFrml(char& FTyp, MergeReportBase* caller);
	FrmlElem* RdKeyInBool(std::vector<KeyInD*>& KIRoot, bool NewMyBP, bool FromRdProc, bool& SQLFilter, MergeReportBase* caller);
	FrmlElem* RdBool(MergeReportBase* caller);
	FrmlElem* RdRealFrml(MergeReportBase* caller);
	FrmlElem* RdStrFrml(MergeReportBase* caller);
	XKey* RdViewKey(FileD* file_d);
	KeyFldD* RdKF(FileD* FD);
	WORD RdKFList(std::vector<KeyFldD*>& KFRoot, FileD* FD);
	bool IsKeyArg(FieldDescr* F, FileD* FD);
	stSaveState* SaveCompState();
	void RestoreCompState(stSaveState* p);
	void CFileLikeFD(FileD* FD, WORD MsgNr);
	std::string RdHelpName();
	FrmlElem* RdAttr();
	void RdW(WRectFrml& W);
	void RdFrame(FrmlElem** Z, BYTE& WFlags);
	bool PromptSortKeys(FileD* file_d, std::vector<FieldDescr*>& FL, std::vector<KeyFldD*>& SKRoot);
	void RdAssignFrml(char FTyp, bool& Add, FrmlElem** Z, MergeReportBase* caller);
	bool FldTypIdentity(FieldDescr* F1, FieldDescr* F2);
	void RdFldList(std::vector<FieldDescr*>& vFields);
	void RdNegFldList(bool& neg, std::vector<FieldDescr*>& vFields);
	void GoCompileErr(int i_rec, WORD n);

	FieldNameType rdFldNameType = FieldNameType::none;
	ReadFuncType rdFuncType = ReadFuncType::none;
	FrmlElem* RdFldNameFrml(char& FTyp, MergeReportBase* caller);

	FileD* processing_F = nullptr; // actually compiled file
	static std::deque<LocVarBlock> ProcStack;

	std::string input_string;
	size_t input_pos = 0;
	size_t input_old_err_pos = 0;

	BYTE CurrChar; // { Compile }
	BYTE ForwChar, ExpChar, Lexem;
	pstring LexWord;

private:
	double ValofS(pstring& S);
	bool SrchF(FieldDescr* F1, FieldDescr* F);
	bool SrchZ(FieldDescr* F1, FrmlElem* Z);
	bool IsFun(std::map<std::string, int>& strs, std::string input, instr_type& FunCode);
	void TestBool(char FTyp);
	LocVar* RdVarName(LocVarBlock* LVB, bool IsParList);
	FrmlElem* BOperation(char Typ, instr_type Fun, FrmlElem* Frml);
	FrmlElem* RdMult(char& FTyp, MergeReportBase* caller);
	FrmlElem* RdAdd(char& FTyp, MergeReportBase* caller);
	WORD RdTilde();
	void RdInConst(FrmlElemIn* Z, char& FTyp, std::string& str, double& R);
	FrmlElem* RdComp(char& FTyp, MergeReportBase* caller);
	FrmlElem* RdBAnd(char& FTyp, MergeReportBase* caller);
	FrmlElem* RdBOr(char& FTyp, MergeReportBase* caller);
	FrmlElem* RdFormula(char& FTyp, MergeReportBase* caller);
	bool FindFuncD(FrmlElem** ZZ, MergeReportBase* caller);
	FrmlElem* RdPrim(char& FTyp, MergeReportBase* caller);
	WORD RdPrecision();
	std::vector<FrmlElem*> RdFL(bool NewMyBP);
	std::vector<FrmlElem*> RdFL(bool NewMyBP, std::vector<FrmlElem*>& left_side_items);
	LinkD* FindOwnLD(FileD* FD, std::string RoleName);
	void SetLocVars(FrmlElem* Z, char typ, bool return_param, std::vector<LocVar*>& newVars);
	void RdIndexOrRecordDecl(char typ, std::vector<KeyFldD*> kf1, std::vector<LocVar*> newVars);
};
