#pragma once
#include "access.h"
#include "constants.h"
#include "rdrun.h"

struct TTD
{
    TTD* Chain = nullptr;
    StringList SL = nullptr;
    WORD Col = 0, Width = 0, Ln = 0;
};

struct YRec
{
    char* P = nullptr; // pchar
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
void NewLine();
void FormFeed();
void NewPage();
bool OutOfLineBound(BlkD* B);
void Zero(FloatPtrList Z);
void WriteNBlks(integer N);
void NewTxtCol(LongStrPtr S, WORD Col, WORD Width, bool Wrap);
pstring GetLine(CharArr* TA, WORD& TLen, WORD Width, bool Wrap, bool Absatz);
void CheckPgeLimit();
void PendingTT();
void Print1NTupel(bool Skip);
void FinishTuple();
void RunAProc(AssignD* A);
void PrintTxt(BlkD* B, bool ChkPg);
void TruncLine();
void PrintBlkChn(BlkD* B, bool ChkPg, bool ChkLine);
void PrintPageFt();
void PrintPageHd();
void SumUp(SumElem* S);
void PrintBlock(BlkD* B, BlkD* DH);
void Footings(LvDescr* L, LvDescr* L2);
void Headings(LvDescr* L, LvDescr* L2);
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
void MergeProc();
bool RewriteRprt(RprtOpt* RO, WORD Pl, WORD& Times, bool& IsLPT1);
