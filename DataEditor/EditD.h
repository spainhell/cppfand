#pragma once
#include <memory>
#include <string>
#include <vector>

#include "DataEditorParams.h"
#include "../Common/FileD.h"
#include "../Core/base.h"

class Implicit;
class EditableField;
struct ERecTxtD;
struct EdExitD;
class LinkD;
class XWKey;
struct KeyInD;
class LocVar;
class FieldDescr;

class EditD
{
public:
	// EditD* PrevE; - toto bude pChain ...
	EditD(uint8_t cols, uint8_t rows);
	~EditD();
	std::vector<EditableField*>::iterator GetEFldIter(EditableField* e_fld);
	FileD* FD = nullptr;
	LockMode OldMd = NullMode;
	bool IsUserForm = false;
	std::vector<FieldDescr*> Flds;
	//Record* OldRec = nullptr;
	//Record* NewRec = nullptr;
	//uint8_t* OldRecPtr = nullptr;
	//uint8_t* NewRecPtr = nullptr;
	uint8_t FrstCol = 0, FrstRow = 0, LastCol = 0, LastRow = 0, Rows = 0;
	WRect V; uint8_t ShdwX = 0, ShdwY = 0;
	uint8_t Attr = 0, dNorm = 0, dHiLi = 0, dSubSet = 0;
	uint8_t dDel = 0, dTab = 0, dSelect = 0;
	std::string Top;
	uint8_t WFlags = 0;                          /* copied from options */
	std::vector<EdExitD*> ExD;                   /*          "          */
	FileD* Journal = nullptr;                    /*          "          */
	std::string ViewName;                        /*          "          */
	char OwnerTyp = '\0'; /* #0=CtrlF7 */        /*          "          */
	LinkD* DownLD = nullptr;                     /*          "          */
	LocVar* DownLV = nullptr;                    /*          "          */
	Record* DownRecord;							 /*          "          */
	Record* LvRec;								 /*          "          */
	std::vector<KeyInD*> KIRoot;                 /*          "          */
	bool SQLFilter = false;                      /*          "          */
	XWKey* SelKey = nullptr;                     /*          "          */
	std::vector<std::string> HdTxt;
	uint8_t NHdTxt = 0;
	unsigned int SaveAfter = 0, WatchDelay = 0, RefreshDelay = 0;
	uint8_t RecNrPos = 0, RecNrLen = 0;
	uint8_t NPages = 0;
	//std::vector<std::string> RecTxt;
	std::vector<ERecTxtD*> RecTxt;
	uint8_t NRecs = 0;     /*display*/
	std::vector<EditableField*> FirstFld;
	std::vector<EditableField*>::iterator CFld;
	EditableField* LastFld = nullptr;
	EditableField* StartFld = nullptr;
	EditableField* FirstEmptyFld = nullptr;					/* copied */
	XKey* VK = nullptr;								/*   "    */		
	XWKey* WK = nullptr;							/*   "    */
	int BaseRec = 0;								/*   "    */
	uint8_t IRec = 0;									/*   "    */
	bool IsNewRec = false;

	std::unique_ptr<DataEditorParams> params_ = std::make_unique<DataEditorParams>();

	bool DownSet = false, IsLocked = false, WwPart = false;
	XKey* DownKey = nullptr;
	int LockedRec = 0;
	FrmlElem* Cond = nullptr;
	FrmlElem* Bool = nullptr;
	std::string BoolTxt;
	std::string Head;
	std::string Last;
	std::string CtrlLast;
	std::string AltLast;
	std::string ShiftLast;
	uint16_t NFlds = 0, NTabsSet = 0, NDuplSet = 0, NEdSet = 0;
	bool EdUpdated = false;
	std::vector<Implicit*> Impl;
	int StartRecNo = 0;
	std::string StartRecKey;
	short StartIRec = 0;
	int OwnerRecNo = 0;
	LinkD* ShiftF7_link = nullptr;
	EditD* ShiftF7_caller = nullptr;
	uint8_t* AfterE = nullptr;
};
