#pragma once

#include "../Core/access-structs.h"
#include "../Core/EditOpt.h"
#include "../Core/FieldDescr.h"
#include "../Core/rdrun.h"

struct EFldD;
struct StringListEl;
struct EditD;
// TODO - uz je deklarovane jinde
extern EditD* E;

void PushEdit();
void SToSL(StringListEl** SLRoot, pstring s);
void StoreRT(WORD Ln, StringList SL, WORD NFlds);
void RdEForm(FileD* ParFD, RdbPos FormPos);
EFldD* FindScanNr(WORD N);
void AutoDesign(std::vector<FieldDescr*>& FL);
void RdFormOrDesign(FileD* F, std::vector<FieldDescr*>& FL, RdbPos FormPos);
void NewEditD(FileD* ParFD, EditOpt* EO); // r158
EFldD* FindEFld_E(FieldDescr* F); // existuje -> *_E
void ZeroUsed();
EFldD* LstUsedFld();
void RdDepChkImpl();
void TestedFlagOff();
void SetFrmlFlags(FrmlElem* Z);
void SetFlag(FieldDescr* F);
void RdDep();
void RdCheck();
void RdImpl();
void RdUDLI();
void RdAllUDLIs(FileD* FD);
std::string StandardHead();
pstring GetStr_E(FrmlElem* Z); // existuje -> *_E
void NewChkKey();
