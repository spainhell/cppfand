#pragma once
#include "../cppfand/rdrun.h"

extern WORD Ii, Oi, SumIi;
extern char WhatToRd; /*i=Oi output FDs;O=O outp.FDs*/
extern int NRecsAll;

FileD* InpFD(WORD I);
void TestNotSum();
void Err(char source, bool wasIiPrefix);
void SetIi_Merge(bool wasIiPrefix);
void SetIi_Report(bool wasIiPrefix);
bool RdIiPrefix();
void CopyPrevMFlds();
void CheckMFlds(KeyFldD* M1, KeyFldD* M2);
void TestSetSumIi();
void ZeroSumFlds(LvDescr* L);
void ZeroSumFlds(std::vector<FrmlElemSum*>* sum);
void SumUp(FileD* file_d, std::vector<FrmlElemSum*>* S, void* record);
