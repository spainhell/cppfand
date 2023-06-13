#pragma once
#include "../CppFand/base.h"
#include "../CppFand/rdrun.h"

class FieldDescr;

struct PFldD
{
	FieldDescr* FldD = nullptr;
	short ColTxt = 0, ColItem = 0;
	bool IsCtrl = false, IsSum = false, NxtLine = false;
	BYTE Level = 0;
};

class ReportGenerator
{
public:
	ReportGenerator();
	~ReportGenerator();
	std::vector<PFldD> PFldDs;
	bool KpLetter;
	short MaxCol, MaxColOld, MaxColUsed, NLines, NLevels;
	AutoRprtMode ARMode;

	void RunAutoReport(RprtOpt* RO);
	bool SelForAutoRprt(RprtOpt* RO);
	std::string SelGenRprt(pstring RprtName);

private:
	void Design(RprtOpt* RO);
	std::string GenAutoRprt(RprtOpt* RO, bool WithNRecs);
	void WrChar(std::string& report, char C);
	void WrBlks(std::string& report, int N);
	void WrStr(std::string& report, std::string& S);
	void WrStr(std::string& report, const char* s);
	void WrLevel(std::string& report, int Level);
};
