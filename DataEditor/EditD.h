#pragma once
#include <memory>
#include <string>
#include <vector>

#include "DataEditorParams.h"
#include "../Core/FileD.h"

class Implicit;
struct EFldD;
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
	std::vector<EFldD*>::iterator GetEFldIter(EFldD* e_fld);
	FileD* FD = nullptr;
	LockMode OldMd = NullMode;
	bool IsUserForm = false;
	std::vector<FieldDescr*> Flds;
	uint8_t* OldRecPtr = nullptr;
	uint8_t* NewRecPtr = nullptr;
	BYTE FrstCol = 0, FrstRow = 0, LastCol = 0, LastRow = 0, Rows = 0;
	WRect V; BYTE ShdwX = 0, ShdwY = 0;
	BYTE Attr = 0, dNorm = 0, dHiLi = 0, dSubSet = 0;
	BYTE dDel = 0, dTab = 0, dSelect = 0;
	std::string Top;
	BYTE WFlags = 0;                             /* copied from options */
	std::vector<EdExitD*> ExD;                   /*          "          */
	FileD* Journal = nullptr;                    /*          "          */
	std::string ViewName;                        /*          "          */
	char OwnerTyp = '\0'; /* #0=CtrlF7 */        /*          "          */
	LinkD* DownLD = nullptr;                     /*          "          */
	LocVar* DownLV = nullptr;                    /*          "          */
	uint8_t* DownRecPtr; void* LVRecPtr;         /*          "          */
	std::vector<KeyInD*> KIRoot;                 /*          "          */
	bool SQLFilter = false;                      /*          "          */
	XWKey* SelKey = nullptr;                     /*          "          */
	std::vector<std::string> HdTxt;
	BYTE NHdTxt = 0;
	unsigned int SaveAfter = 0, WatchDelay = 0, RefreshDelay = 0;
	BYTE RecNrPos = 0, RecNrLen = 0;
	BYTE NPages = 0;
	//std::vector<std::string> RecTxt;
	std::vector<ERecTxtD*> RecTxt;
	BYTE NRecs = 0;     /*display*/
	std::vector<EFldD*> FirstFld;
	std::vector<EFldD*>::iterator CFld;
	EFldD* LastFld = nullptr;
	EFldD* StartFld = nullptr;
	EFldD* FirstEmptyFld = nullptr;					/* copied */
	XKey* VK = nullptr;								/*   "    */		
	XWKey* WK = nullptr;							/*   "    */
	int BaseRec = 0;								/*   "    */
	BYTE IRec = 0;									/*   "    */
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
	WORD NFlds = 0, NTabsSet = 0, NDuplSet = 0, NEdSet = 0;
	bool EdUpdated = false;
	std::vector<Implicit*> Impl;
	int StartRecNo = 0;
	std::string StartRecKey;
	short StartIRec = 0;
	int OwnerRecNo = 0;
	LinkD* ShiftF7_link = nullptr;
	EditD* ShiftF7_caller = nullptr;
	void* AfterE = nullptr;
};
