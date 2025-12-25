#pragma once
//#include "../Core/Compiler.h"
#include "../Core/rdrun.h"

class Compiler;

class MergeReportBase {
public:
	bool ChainSum;
	virtual FrmlElem* RdFldNameFrml(char& FTyp) = 0;
	virtual void ChainSumEl() = 0;
	void SetInput(std::string& input);
	void SetInput(std::string& s, bool ShowErr);
	void SetInput(RdbPos* rdb_pos, bool FromTxt);
	void SetInput(std::string& s, int32_t license_nr, bool decode);

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
	void SumUp(FileD* file_d, std::vector<FrmlElemSum*>* S, Record* record);

	WORD Ii, Oi, SumIi;
	char WhatToRd; /*i=Oi output FDs;O=O outp.FDs*/
	int NRecsAll;

	InpD* IDA[30];
	short MaxIi;

	std::vector<ConstListEl> OldMFlds;
	std::vector<ConstListEl> NewMFlds;

	Compiler* base_compiler;
};
