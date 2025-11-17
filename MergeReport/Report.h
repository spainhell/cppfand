#pragma once
#include "BlkD.h"
#include "MergeReportBase.h"
#include "../Core/Compiler.h"
#include "../Core/rdrun.h"

struct InpD;
class KeyFldD;

struct TTD
{
	std::vector<std::string> SL;
	WORD Col = 0, Width = 0, Ln = 0;
};

struct YRec
{
	const char* P = nullptr;
	WORD I = 0, Ln = 0, TLn = 0, Sz = 0;
	BlkD* Blk = nullptr;
	bool ChkPg = false;
	std::vector<TTD> TD;
};

class Report : public MergeReportBase
{
public:
	Report();
	~Report();
	void Read(RprtOpt* RO);
	void Run(RprtOpt* RO);
	FrmlElem* RdFldNameFrml(char& FTyp) override;
	void ChainSumEl() override;

private:
	WORD PrintDH = 0;
	YRec Y;
	bool FrstBlk = false;
	bool NoFF = false;
	bool WasFF2 = false;
	bool SetPage = false;
	bool WasOutput = false;
	short LineLenLst = 0, PageNo = 0, PgeSize = 0;
	//void* Store2Ptr;
	int RecCount = 0;
	WORD NEof = 0;
	InpD* MinID = nullptr;
	bool FirstLines = false, WasDot = false;
	int NLinesOutp = 0;
	BlkD* CBlk = nullptr;
	std::vector<FrmlElemSum*>* CZeroLst = nullptr;
	LvDescr* LvToRd = nullptr;           /*all used while translating frml*/
	BlkD* CBlkSave = nullptr;

	bool FindInLvBlk(LvDescr* L, BlkD** B, RFldD** RF);
	FrmlElem* FindIiandFldFrml(FileD** FD, char& FTyp);
	void RdDirFilVar(char& FTyp, FrmlElem** res, bool wasIiPrefix);
	bool OwnInBlock(char& FTyp, FrmlElem** res);
	void FindInRec(char& FTyp, FrmlElem** res, bool wasIiPrefix);
	void Rd_Oi();
	LvDescr* MakeOldMLvD();
	void RdAutoSortSK(InpD* ID);
	LvDescr* NewLvS(LvDescr* L, InpD* ID);
	void RdAssignBlk(std::vector<AssignD*>* ARoot);
	void RdBeginEnd(std::vector<AssignD*>* ARoot);
	void RdBlock(std::vector<BlkD*>& BB);
	void RdCh(short& LineLen);
	short NUnderscores(char C, short& LineLen);
	void EndString(BlkD* block, uint8_t* buffer, size_t LineLen, size_t NBytesStored);
	void TestSetRFTyp(char Typ, bool RepeatedGrp, RFldD* RF);
	void TestSetBlankOrWrap(bool RepeatedGrp, char UC, RFldD* RF);
	std::vector<AssignD*> RdAssign2();
	void RdCond();
	LvDescr* RdKeyName();

	void IncPage();
	void NewLine(std::string& text);
	void FinishTuple(std::string& text);
	void TruncLine(std::string& text);
	void ResetY();
	void FormFeed(std::string& text);
	void Print1NTupel(std::string& text, bool Skip);
	void RunAProc(std::vector<AssignD*> vAssign);
	void PrintTxt(BlkD* B, std::string& text, bool ChkPg);
	bool OutOfLineBound(BlkD* B);
	void PrintBlkChn(std::vector<BlkD*>& block, std::string& text, bool ChkPg, bool ChkLine);
	void PrintPageFt(std::string& text);
	void PrintPageHd(std::string& text);
	void NewPage(std::string& text);
	void WriteNBlks(std::string& text, short N);
	std::string GetLine(std::string& S, WORD Width, bool Wrap, bool& paragraph);
	std::string NewTxtCol(std::string S, WORD Col, WORD Width, bool Wrap);
	void CheckPgeLimit(std::string& text);
	void PendingTT(std::string& text);
	void PrintBlock(std::vector<BlkD*>& block, std::string& text, std::vector<BlkD*>& dh);
	void Footings(LvDescr* L, LvDescr* L2, std::string& text);
	void Headings(LvDescr* L, LvDescr* L2, std::string& text);
	void ReadInpFile(InpD* ID);
	void OpenInp();
	void CloseInp();
	WORD CompMFlds(std::vector<ConstListEl>& C, std::vector<KeyFldD*>& M, short& NLv);
	void GetMFlds(std::vector<ConstListEl>& C, std::vector<KeyFldD*>& M);
	void MoveMFlds(std::vector<ConstListEl>& C1, std::vector<ConstListEl>& C2);
	void PutMFlds(std::vector<KeyFldD*>& M);
	void GetMinKey();
	void ZeroCount();
	LvDescr* GetDifLevel();
	void MoveForwToRec(InpD* ID);
	void MoveFrstRecs();
	void MergeProc(std::string& text);
	bool RewriteRprt(RprtOpt* RO, WORD pageLimit, WORD& Times, bool& IsLPT1);
};
