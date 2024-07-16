#pragma once
#include <string>
#include <vector>
#include "RprtFDListEl.h"
#include "../Core/Rdb.h"

class FieldDescr;
class KeyFldD;
class FrmlElem;

enum AutoRprtMode { _ALstg, _ARprt, _ATotal, _AErrRecs };

struct RprtOpt
{
	std::vector<RprtFDListEl*> FDL;
	std::string Path;
	uint16_t CatIRec = 0;
	bool UserSelFlds = false, UserCondQuest = false, FromStr = false, SyntxChk = false;
	FrmlElem* Times = nullptr;
	AutoRprtMode Mode = _ALstg;
	RdbPos RprtPos;
	std::vector<FieldDescr*> Flds;  // !empty => autoreport
	std::vector<FieldDescr*> Ctrl;
	std::vector<FieldDescr*> Sum;
	std::vector<KeyFldD*> SK;
	FrmlElem* WidthFrml = nullptr, * Head = nullptr;
	uint16_t Width = 0;
	std::string CondTxt;
	std::string HeadTxt;
	char Style = '\0';
	bool Edit = false, PrintCtrl = false;
};