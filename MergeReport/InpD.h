#pragma once
#include <string>
#include <vector>

#include "OutpRD.h"

struct LvDescr;
class XScan;
class FrmlElem;
class FrmlElemString;
class KeyFldD;
class FrmlElemSum;
class LogicControl;

struct InpD
{
	XScan* Scan = nullptr;
	bool AutoSort = false;
	std::vector<KeyFldD*> SK;
	LockMode Md = NullMode;
	int IRec = 0;
	Record* RecPtr = nullptr;
	Record* ForwRecPtr = nullptr;
	FrmlElem* Bool = nullptr;
	bool SQLFilter = false;
	std::vector<KeyFldD*> MFld;
	std::vector<FrmlElemSum*>* Sum = nullptr;
	bool Exist = false;
	char Op = '\0';
	double Count = 0.0;
	std::vector<LogicControl*> Chk;
	char OpErr = '\0';
	bool Error = false;
	char OpWarn = '\0';
	bool Warning = false;
	FrmlElemString* ErrTxtFrml = nullptr;
	std::vector<KeyFldD*> SFld;                /* only Report */
	std::vector<ConstListEl> OldSFlds;
	LvDescr* FrstLvS = nullptr;
	LvDescr* LstLvS = nullptr;		/* FrstLvS->Ft=DE */
	bool IsInplace = false;              /* only Merge */
	std::vector<OutpRD*> RD;
};