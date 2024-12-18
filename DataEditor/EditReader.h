#pragma once

#include "../Core/EditOpt.h"
#include "../Core/FieldDescr.h"
#include "../Core/rdrun.h"

struct EFldD;
class EditD;

//extern std::vector<EditD*> v_edits;

//void PushEdit();


class EditReader
{
public:
	EditReader();
	void NewEditD(FileD* file_d, EditOpt* EO, uint8_t* rec); // r158
	void RdFormOrDesign(std::vector<FieldDescr*>& FL, RdbPos FormPos);
	EditD* GetEditD();

private:
	EditD* edit_ = nullptr;
	FileD* file_d_ = nullptr;

	void StoreRT(WORD Ln, std::vector<std::string>& SL, WORD NFlds);
	void RdEForm(EditD* edit, RdbPos FormPos);
	EFldD* FindScanNr(WORD N);
	void AutoDesign(std::vector<FieldDescr*>& FL);
	EFldD* FindEFld_E(FieldDescr* F);
	void ZeroUsed();
	EFldD* LstUsedFld();
	void SetFlag(FieldDescr* F);
	void SetFrmlFlags(FrmlElem* Z);
	void ReadDependencies(FileD* file_d);
	void RdCheck();
	void RdImpl(FileD* file_d);
	void RdDepChkImpl(EditD* edit);
	void RdUDLI(FileD* file_d);
	void RdAllUDLIs(FileD* FD);
	void NewChkKey(FileD* file_d);
	std::string StandardHead(EditD* edit);
	//void SToSL(StringListEl** SLRoot, pstring s);
	void TestedFlagOff();
	std::string GetStr_E(FrmlElem* Z, uint8_t* record);
};