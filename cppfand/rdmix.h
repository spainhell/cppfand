#pragma once
#include "constants.h"
#include "pstring.h"
#include "rdrun.h"

struct RdbPos;
void RdLocDcl(LocVarBlkD* LVB, bool IsParList, bool WithRecVar, char CTyp); // r1
bool FindLocVar(LocVar* LVRoot, LocVar* LV); // r84
bool FindChpt(char Typ, const pstring& name, bool local, RdbPos* RP); // r92
void RdChptName(char C, RdbPos* Pos, bool TxtExpr); // r108
FieldList AllFldsList(FileDPtr FD, bool OnlyStored); // r118
EditOpt* GetEditOpt(); // r129
RprtOpt* GetRprtOpt(); // r132
pstring* RdHelpName(); // r144
FrmlPtr RdAttr(); // r152
void RdW(WRectFrml W); // r161
void RdFrame(FrmlPtr Z, BYTE& WFlags); // r166
void RdAssignFrml(char FTyp, bool& Add, FrmlPtr Z); // r193
void RdNegFldList(bool& Neg, FieldList FLRoot); // r214
void EditModeToFlags(pstring Mode, void* Flgs, bool Err); // r220
KeyDPtr RdViewKey(); // r238
bool IsKeyArg(FieldDPtr F, FileDPtr FD); // r278
WORD RdKFList(KeyFldDPtr KFRoot, FileDPtr FD); // r298


