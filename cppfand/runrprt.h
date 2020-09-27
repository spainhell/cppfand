#pragma once
#include "access.h"
#include "constants.h"
#include "rdrun.h"

struct TTD : public Chained
{
    //TTD* Chain = nullptr;
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
extern integer LineLenLst, PageNo, PgeSize;
extern void* Store2Ptr;

//longint NRecsAll;
extern longint RecCount;
extern WORD NEof;
extern InpD* MinID;
extern bool FirstLines, WasDot;
extern longint NLinesOutp;

void RunReport(RprtOpt* RO);
//void PrintPageHd; forward;
//void PrintPageFt; forward;
//void TruncLine; forward;
void ResetY();
void IncPage();
void NewLine(std::string& text);
void FormFeed();
void NewPage(std::string& text);
bool OutOfLineBound(BlkD* B);
void Zero(FloatPtrList Z);
std::string NewTxtCol(LongStr* S, WORD Col, WORD Width, bool Wrap);
pstring GetLine(char* TA, WORD& TLen, WORD Width, bool Wrap, bool &Absatz);
void CheckPgeLimit(std::string& text);
void PendingTT(std::string& text);
void FinishTuple(std::string& text);
void RunAProc(AssignD* A);

void PrintBlock(BlkD* B, std::string& text, BlkD* DH);
void PrintTxt(BlkD* B, std::string& text, bool ChkPg);
void Print1NTupel(std::string& text, bool Skip);

void TruncLine(std::string& text);
void PrintBlkChn(BlkD* B, std::string& text, bool ChkPg, bool ChkLine);
void PrintPageFt(std::string& text);
void PrintPageHd(std::string& text);
void SumUp(SumElem* S);
void Footings(LvDescr* L, LvDescr* L2, std::string& text);
void Headings(LvDescr* L, LvDescr* L2, std::string& text);
void ZeroSumFlds(LvDescr* L);
void ReadInpFile(InpD* ID);
void OpenInp();
void CloseInp();
WORD CompMFlds(ConstListEl* C, KeyFldD* M, integer& NLv);
void GetMFlds(ConstListEl* C, KeyFldD* M);
void MoveMFlds(ConstListEl* C1, ConstListEl* C2);
void PutMFlds(KeyFldDPtr M);
void GetMinKey();
void ZeroCount();
LvDescr* GetDifLevel();
void MoveForwToRec(InpD* ID);
void MoveFrstRecs();
void MergeProc(std::string& text);
bool RewriteRprt(RprtOpt* RO, WORD Pl, WORD& Times, bool& IsLPT1);
