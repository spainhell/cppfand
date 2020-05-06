#pragma once
#include "rdrun.h"

struct PFldD
{
	PFldD* Chain = nullptr; FieldDescr* FldD = nullptr;
	integer ColTxt = 0, ColItem = 0;
	bool IsCtrl = false, IsSum = false, NxtLine = false;
	BYTE Level = 0;
};

PFldD* PFldDs;
bool KpLetter;
integer MaxCol, MaxColOld, MaxColUsed, NLines, NLevels;
AutoRprtMode ARMode;
LongStrPtr Txt;

void SubstChar(pstring S, char C1, char C2);
void Design(RprtOpt* RO);
void WrChar(char C); // existuje i v EDEVPROC!
void WrBlks(integer N);
void WrStr(pstring S);
void WrLevel(integer Level);
LongStr* GenAutoRprt(RprtOpt* RO, bool WithNRecs);
void RunAutoReport(RprtOpt* RO); // r223
bool SelForAutoRprt(RprtOpt* RO); // r232
LongStr* SelGenRprt(pstring RprtName);
