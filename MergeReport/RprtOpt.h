#pragma once
#include <string>
#include <vector>
#include "RprtFDListEl.h"
#include "../Core/Rdb.h"

class FieldDescr;
class KeyFldD;
class FrmlElem;

enum AutoRprtMode { _ALstg, _ARprt, _ATotal, _AErrRecs };

class RprtOpt
{
public:
	RprtOpt();
	~RprtOpt();
	void CopyTo(RprtOpt* dst);
	std::vector<RprtFDListEl*> FDL;
	std::string Path;
	uint16_t CatIRec = 0;
	bool UserSelFlds = false;
	bool UserCondQuest = false;
	bool FromStr = false;
	bool SyntxChk = false;
	FrmlElem* Times = nullptr;
	AutoRprtMode Mode = _ALstg;
	RdbPos RprtPos;
	std::vector<FieldDescr*> Flds;  // !empty => autoreport
	std::vector<FieldDescr*> Ctrl;
	std::vector<FieldDescr*> Sum;
	std::vector<KeyFldD*> SK;
	FrmlElem* WidthFrml = nullptr;
	FrmlElem* Head = nullptr;
	uint16_t Width = 0;
	std::string CondTxt;
	std::string HeadTxt;
	char Style = '\0';
	bool Edit = false;
	bool PrintCtrl = false;
};