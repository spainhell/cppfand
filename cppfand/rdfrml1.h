#pragma once
#include "runfrml.h"

FrmlPtr GetOp(char Op, integer BytesAfter); // r1
FieldDPtr FindFldName(FileDPtr FD); // r7
bool IsRoleName(bool Both, FileDPtr& FD, LinkDPtr& LD); // r49
FrmlPtr RdFAccess(FileDPtr FD, LinkD* LD, char& FTyp); // r58
FrmlPtr MakeFldFrml(FieldDPtr F, char& FTyp); // r72
FrmlPtr TryRdFldFrml(FileDPtr FD, char& FTyp); // r76
LinkDPtr FindOwnLD(FileDPtr FD, const pstring& RoleName); // 77

FrmlElem* RdFldNameFrmlF(char& FTyp); // r111
