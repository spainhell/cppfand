#pragma once

#include "../Core/access-structs.h"
#include "../Core/EditOpt.h"
#include "../Core/FieldDescr.h"
#include "../Core/rdrun.h"

struct EFldD;
struct StringListEl;
class EditD;

//extern std::vector<EditD*> v_edits;

//void PushEdit();


class EditReader
{
public:
	EditReader();
	void NewEditD(FileD* file_d, EditOpt* EO); // r158
	void RdFormOrDesign(std::vector<FieldDescr*>& FL, RdbPos FormPos);
	EditD* GetEditD();

private:
	EditD* edit_ = nullptr;

	void StoreRT(WORD Ln, StringList SL, WORD NFlds);
	void RdEForm(EditD* edit, RdbPos FormPos);
	EFldD* FindScanNr(WORD N);
	void AutoDesign(FieldListEl* FL);
	void AutoDesign(std::vector<FieldDescr*>& FL);
	EFldD* FindEFld_E(FieldDescr* F);
	void ZeroUsed();
	EFldD* LstUsedFld();
	void SetFlag(FieldDescr* F);
	void SetFrmlFlags(FrmlElem* Z);
	void RdDep(FileD* file_d);
	void RdCheck();
	void RdImpl(FileD* file_d);
	void RdDepChkImpl(EditD* edit);
	void RdUDLI(FileD* file_d);
	void RdAllUDLIs(FileD* FD);
	void NewChkKey(FileD* file_d);
	std::string StandardHead(EditD* edit);
	void SToSL(StringListEl** SLRoot, pstring s);
	void TestedFlagOff();
};