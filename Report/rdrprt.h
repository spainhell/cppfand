#pragma once
#include "../cppfand/access-structs.h"
#include "../cppfand/rdrun.h"

struct LvDescr;
struct BlkD;
extern BlkD* CBlk;
extern std::vector<double*> CZeroLst;
extern LvDescr* LvToRd;           /*all used while translating frml*/
//WORD Ii, Oi, SumIi;
//char WhatToRd; /*'i'=#XXi 'O'=#XX */
extern bool WasIiPrefix;
extern BlkD* CBlkSave;


FileD* InpFD(WORD I);
bool FindInLvBlk(LvDescr* L, BlkD* B, RFldD* RF);
FrmlPtr RdFldNameFrmlR(char& FTyp);
bool RdIiPrefix();
void TestSetSumIi();
FrmlPtr FindIiandFldFrml(FileD** FD, char& FTyp);
void RdDirFilVar(char& FTyp, FrmlElem** res);
bool OwnInBlock(char& FTyp, FrmlElem** res);
void FindInRec(char& FTyp, FrmlElem** res);
void SetIi();
void TestNotSum();
void Err();
void ChainSumElR();
void ReadReport(RprtOpt* RO); 
void CopyPrevMFlds();
void CheckMFlds(KeyFldD* M1, KeyFldD* M2);
LvDescr* MakeOldMLvD();
void RdAutoSortSK(InpD* ID);
LvDescr* NewLvS(LvDescr* L, InpD* ID);
void RdBlock(BlkD** BB);
void RdCh(integer& LineLen);
//void StoreCh(BYTE C, integer& NBytesStored);
integer NUnderscores(char C, integer& LineLen);
void EndString(BlkD* block, BYTE* buffer, integer LineLen, integer NBytesStored, BYTE* LnL, WORD* StrL);
void TestSetRFTyp(char Typ, bool RepeatedGrp, RFldD* RF);
void TestSetBlankOrWrap(bool RepeatedGrp, char UC, RFldD* RF);
//void RdBeginEnd(AssignD* ARoot);
AssignD* RdAssign2();
//void RdAssignBlk(AssignD* ARoot);
void RdCond();
LvDescr* RdKeyName();
//void Rd_Oi();
