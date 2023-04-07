#pragma once
#include "../cppfand/rdrun.h"

struct LvDescr;
struct BlkD;
extern BlkD* CBlk;
extern std::vector<FrmlElemSum*>* CZeroLst;
extern LvDescr* LvToRd;           /*all used while translating frml*/
//WORD Ii, Oi, SumIi;
//char WhatToRd; /*'i'=#XXi 'O'=#XX */
//extern bool WasIiPrefix;
extern BlkD* CBlkSave;


FileD* InpFD(WORD I);
bool FindInLvBlk(LvDescr* L, BlkD** B, RFldD** RF);
FrmlElem* RdFldNameFrmlR(char& FTyp);
FrmlElem* FindIiandFldFrml(FileD** FD, char& FTyp);
void RdDirFilVar(char& FTyp, FrmlElem** res, bool wasIiPrefix);
bool OwnInBlock(char& FTyp, FrmlElem** res);
void FindInRec(char& FTyp, FrmlElem** res, bool wasIiPrefix);
void ChainSumElR();
void ReadReport(RprtOpt* RO); 
LvDescr* MakeOldMLvD();
void RdAutoSortSK(InpD* ID);
LvDescr* NewLvS(LvDescr* L, InpD* ID);
void RdBlock(BlkD** BB);
void RdCh(short& LineLen);
short NUnderscores(char C, short& LineLen);
void EndString(BlkD* block, BYTE* buffer, size_t LineLen, size_t NBytesStored);
void TestSetRFTyp(char Typ, bool RepeatedGrp, RFldD* RF);
void TestSetBlankOrWrap(bool RepeatedGrp, char UC, RFldD* RF);
std::vector<AssignD*> RdAssign2();
void RdCond();
LvDescr* RdKeyName();
