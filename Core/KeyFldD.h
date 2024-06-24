#pragma once
#include "base.h"

class FieldDescr;

class KeyFldD
{
public:
	KeyFldD() {}
	KeyFldD(const KeyFldD& orig, bool copyFlds);
	KeyFldD(BYTE* inputStr);
	FieldDescr* FldD = nullptr;
	bool CompLex = false;
	bool Descend = false;
	bool static EquKFlds(std::vector<KeyFldD*>& KF1, std::vector<KeyFldD*>& KF2);
};
