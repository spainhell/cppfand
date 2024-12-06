#pragma once
#include "../Core/rdrun.h"

class MergeReportBase {
public:
	bool ChainSum;
	virtual FrmlElem* RdFldNameFrml(char& FTyp) = 0;
	virtual void ChainSumEl() = 0;

protected:
	MergeReportBase();
	~MergeReportBase();
	FileD* InpFD(WORD I);
	void TestNotSum();
	void Err(char source, bool wasIiPrefix);
	void SetIi_Merge(bool wasIiPrefix);
	void SetIi_Report(bool wasIiPrefix);
	bool RdIiPrefix();
	void CopyPrevMFlds();
	void CheckMFlds(std::vector<KeyFldD*>& M1, std::vector<KeyFldD*>& M2);
	void TestSetSumIi();
	void ZeroSumFlds(LvDescr* L);
	void ZeroSumFlds(std::vector<FrmlElemSum*>* sum);
	void SumUp(FileD* file_d, std::vector<FrmlElemSum*>* S, void* record);

	WORD Ii, Oi, SumIi;
	char WhatToRd; /*i=Oi output FDs;O=O outp.FDs*/
	int NRecsAll;

	InpD* IDA[30];
	short MaxIi;
};
