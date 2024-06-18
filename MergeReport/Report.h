#pragma once
#include "MergeReportBase.h"
#include "../Core/access-structs.h"
#include "../Core/Compiler.h"
#include "../Core/rdrun.h"

struct InpD;
struct BlkD;
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
	WORD PrintDH;
	YRec Y;
	bool FrstBlk, NoFF, WasFF2, SetPage, WasOutput;
	short LineLenLst, PageNo, PgeSize;
	//void* Store2Ptr;
	int RecCount;
	WORD NEof;
	InpD* MinID;
	bool FirstLines, WasDot;
	int NLinesOutp;
	BlkD* CBlk;
	std::vector<FrmlElemSum*>* CZeroLst;
	LvDescr* LvToRd;           /*all used while translating frml*/
	BlkD* CBlkSave;

	bool FindInLvBlk(LvDescr* L, BlkD** B, RFldD** RF);
	FrmlElem* FindIiandFldFrml(FileD** FD, char& FTyp);
	void RdDirFilVar(char& FTyp, FrmlElem** res, bool wasIiPrefix);
	bool OwnInBlock(char& FTyp, FrmlElem** res);
	void FindInRec(char& FTyp, FrmlElem** res, bool wasIiPrefix);
	void Rd_Oi();
	LvDescr* MakeOldMLvD();
	void RdAutoSortSK(InpD* ID, std::unique_ptr<Compiler>& compiler);
	LvDescr* NewLvS(LvDescr* L, InpD* ID);
	void RdAssignBlk(std::vector<AssignD*>* ARoot);
	void RdBeginEnd(std::vector<AssignD*>* ARoot);
	void RdBlock(BlkD** BB);
	void RdCh(short& LineLen);
	short NUnderscores(char C, short& LineLen);
	void EndString(BlkD* block, BYTE* buffer, size_t LineLen, size_t NBytesStored);
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
	void PrintBlkChn(BlkD* B, std::string& text, bool ChkPg, bool ChkLine);
	void PrintPageFt(std::string& text);
	void PrintPageHd(std::string& text);
	void NewPage(std::string& text);
	void WriteNBlks(std::string& text, short N);
	std::string GetLine(std::string& S, WORD Width, bool Wrap, bool& paragraph);
	std::string NewTxtCol(std::string S, WORD Col, WORD Width, bool Wrap);
	void CheckPgeLimit(std::string& text);
	void PendingTT(std::string& text);
	void PrintBlock(BlkD* B, std::string& text, BlkD* DH);
	void Footings(LvDescr* L, LvDescr* L2, std::string& text);
	void Headings(LvDescr* L, LvDescr* L2, std::string& text);
	void ReadInpFile(InpD* ID);
	void OpenInp();
	void CloseInp();
	WORD CompMFlds(std::vector<ConstListEl>& C, KeyFldD* M, short& NLv);
	void GetMFlds(std::vector<ConstListEl>& C, KeyFldD* M);
	void MoveMFlds(std::vector<ConstListEl>& C1, std::vector<ConstListEl>& C2);
	void PutMFlds(KeyFldD* M);
	void GetMinKey();
	void ZeroCount();
	LvDescr* GetDifLevel();
	void MoveForwToRec(InpD* ID);
	void MoveFrstRecs();
	void MergeProc(std::string& text);
	bool RewriteRprt(RprtOpt* RO, WORD pageLimit, WORD& Times, bool& IsLPT1);
};
