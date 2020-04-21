#pragma once
#include "runfrml.h"

FrmlPtr GetOp(char Op, integer BytesAfter); // r1
bool IsRoleName(bool Both, FileDPtr& FD, LinkDPtr& LD); // r49
FrmlPtr RdFAccess(FileDPtr FD, LinkD* LD, char& FTyp); // r58
FrmlPtr TryRdFldFrml(FileDPtr FD, char& FTyp); // r71
LinkDPtr FindOwnLD(FileDPtr FD, const pstring& RoleName); // 72

FrmlElem* RdFldNameFrmlF(char& FTyp); // r111
