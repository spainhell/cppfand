#pragma once
#include "../cppfand/base.h"
#include "../cppfand/rdrun.h"

class FieldDescr;

struct PFldD //: public Chained
{
	//PFldD* pChain = nullptr; 
	FieldDescr* FldD = nullptr;
	short ColTxt = 0, ColItem = 0;
	bool IsCtrl = false, IsSum = false, NxtLine = false;
	BYTE Level = 0;
};

extern std::vector<PFldD> PFldDs;
extern bool KpLetter;
extern short MaxCol, MaxColOld, MaxColUsed, NLines, NLevels;
extern AutoRprtMode ARMode;

void Design(RprtOpt* RO);
//void WrChar(char C); // existuje i v EDEVPROC!
//void WrBlks(short N);
//void WrStr(pstring S);
//void WrLevel(short Level);
std::string GenAutoRprt(RprtOpt* RO, bool WithNRecs);
void RunAutoReport(RprtOpt* RO); // r223
bool SelForAutoRprt(RprtOpt* RO); // r232
std::string SelGenRprt(pstring RprtName);
