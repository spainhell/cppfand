#pragma once

#include "access-structs.h"
#include "Rdb.h"
#include "rdrun.h"

class EditOpt
{
public:
	EditOpt() = default;
	~EditOpt();
	RdbPos FormPos;
	bool UserSelFlds = false, SetOnlyView = false, NegDupl = false;
	bool NegTab = false, NegNoEd = false, SyntxChk = false;
	//FieldListEl* Flds = nullptr;
	std::vector<FieldDescr*> Flds;
	std::vector<FieldDescr*> Dupl;
	std::vector<FieldDescr*> Tab;
	std::vector<FieldDescr*> NoEd;
	FrmlElem* Cond = nullptr;
	FrmlElem* Head = nullptr;
	FrmlElem* Last = nullptr;
	FrmlElem* CtrlLast = nullptr;
	FrmlElem* AltLast = nullptr;
	FrmlElem* ShiftLast = nullptr;
	FrmlElem* Mode = nullptr;
	FrmlElem* StartRecNoZ = nullptr;
	FrmlElem* StartRecKeyZ = nullptr;
	FrmlElem* StartIRecZ = nullptr;
	FrmlElem* StartFieldZ = nullptr;
	FrmlElem* SaveAfterZ = nullptr;
	FrmlElem* WatchDelayZ = nullptr;
	FrmlElem* RefreshDelayZ = nullptr;
	WRectFrml W;
	FrmlElem* ZAttr = nullptr;
	FrmlElem* ZdNorm = nullptr;
	FrmlElem* ZdHiLi = nullptr;
	FrmlElem* ZdSubset = nullptr;
	FrmlElem* ZdDel = nullptr;
	FrmlElem* ZdTab = nullptr;
	FrmlElem* ZdSelect = nullptr;
	FrmlElem* Top = nullptr;
	uint8_t WFlags = 0;
	std::vector<EdExitD*> ExD;
	FileD* Journal = nullptr;
	std::string ViewName;
	char OwnerTyp = '\0';
	LinkD* DownLD = nullptr;
	LocVar* DownLV = nullptr;
	uint8_t* DownRecPtr = nullptr;
	void* LVRecPtr = nullptr;
	std::vector<KeyInD*> KIRoot;
	bool SQLFilter = false;
	XKey* SelKey = nullptr;
	XKey* ViewKey = nullptr;
};