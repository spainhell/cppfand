#pragma once
#include "constants.h"
#include "pstring.h"
#include "rdrun.h"


void RdLocDcl(LocVarBlkD* LVB, bool IsParList, bool WithRecVar, char CTyp); // r1
LocVar* RdVarName(LocVarBlkD* LVB, bool IsParList);
bool FindLocVar(LocVar* LVRoot, LocVar* LV); // r84
bool FindChpt(char Typ, const pstring& name, bool local, RdbPos* RP); // r92
void RdChptName(char C, RdbPos* Pos, bool TxtExpr); // r108
FieldList AllFldsList(FileDPtr FD, bool OnlyStored); // r118
EditOpt* GetEditOpt(); // r129
RprtOpt* GetRprtOpt(); // r132
void CFileLikeFD(FileD* FD, WORD MsgNr);
pstring* RdHelpName(); // r144
FrmlPtr RdAttr(); // r152
void RdW(WRectFrml& W); // r161
void RdFrame(FrmlPtr Z, BYTE& WFlags); // r166
bool PromptSortKeys(FieldList FL, KeyFldD* SKRoot);
void RdAssignFrml(char FTyp, bool& Add, FrmlPtr Z); // r193
bool FldTypIdentity(FieldDescr* F1, FieldDescr* F2);
void RdFldList(FieldListEl* FLRoot);
void RdNegFldList(bool& Neg, FieldList FLRoot); // r214
void EditModeToFlags(pstring Mode, void* Flgs, bool Err); // r220
KeyDPtr RdViewKey(); // r238

bool KeyArgFound;
FieldDPtr KeyArgFld;

void SrchF(FieldDPtr F);
void SrchZ(FrmlPtr Z);
bool IsKeyArg(FieldDPtr F, FileDPtr FD); // r278
KeyFldD* RdKF(FileDPtr FD);
WORD RdKFList(KeyFldDPtr KFRoot, FileDPtr FD); // r298
void CompileRecLen();
