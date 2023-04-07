#pragma once
#include "../cppfand/access-structs.h"
#include "../cppfand/base.h"
#include "../cppfand/rdrun.h"

struct InpD;
struct BlkD;
class KeyFldD;

struct TTD : public Chained<TTD>
{
    //TTD* pChain = nullptr;
    StringList SL = nullptr;
    WORD Col = 0, Width = 0, Ln = 0;
};

struct YRec
{
    const char* P = nullptr; // pchar
    WORD I = 0, Ln = 0, TLn = 0, Sz = 0;
    BlkD* Blk = nullptr;
    bool ChkPg = false;
    TTD* TD = nullptr;
};

extern WORD PrintDH;
extern YRec Y;
extern bool FrstBlk, NoFF, WasFF2, SetPage, WasOutput;
extern short LineLenLst, PageNo, PgeSize;
extern void* Store2Ptr;

//int NRecsAll;
extern int RecCount;
extern WORD NEof;
extern InpD* MinID;
extern bool FirstLines, WasDot;
extern int NLinesOutp;

void RunReport(RprtOpt* RO);
//void PrintPageHd; forward;
//void PrintPageFt; forward;
//void TruncLine; forward;
void ResetY();
void IncPage();
void NewLine(std::string& text);
void FormFeed(std::string& text);
void NewPage(std::string& text);
bool OutOfLineBound(BlkD* B);
std::string NewTxtCol(std::string S, WORD Col, WORD Width, bool Wrap);
std::string GetLine(std::string& S, WORD Width, bool Wrap, bool &paragraph);
void CheckPgeLimit(std::string& text);
void PendingTT(std::string& text);
void FinishTuple(std::string& text);
void RunAProc(std::vector<AssignD*> A);

void PrintBlock(BlkD* B, std::string& text, BlkD* DH);
void PrintTxt(BlkD* B, std::string& text, bool ChkPg);
void Print1NTupel(std::string& text, bool Skip);

void TruncLine(std::string& text);
void PrintBlkChn(BlkD* B, std::string& text, bool ChkPg, bool ChkLine);
void PrintPageFt(std::string& text);
void PrintPageHd(std::string& text);
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
