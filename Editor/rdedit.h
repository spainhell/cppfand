#pragma once

#include "../cppfand/access-structs.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/rdrun.h"

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
void AutoDesign(FieldList FL);
void RdFormOrDesign(FileD* F, FieldList FL, RdbPos FormPos);
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
pstring StandardHead();
pstring GetStr_E(FrmlPtr Z); // existuje -> *_E
void NewChkKey();
