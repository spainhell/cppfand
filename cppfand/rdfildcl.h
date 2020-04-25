#pragma once
#include "editor.h"

FieldDPtr RdFldDescr(pstring Name, bool Stored); // r25
void RdBegViewDcl(EditOpt* EO); // r110
bool RdUserView(pstring ViewName, EditOpt* EO);
