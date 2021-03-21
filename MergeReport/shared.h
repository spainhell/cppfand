#pragma once
#include "../cppfand/KeyFldD.h"
#include "../cppfand/rdrun.h"

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
void SumUp(std::vector<FrmlElemSum*>* S);
