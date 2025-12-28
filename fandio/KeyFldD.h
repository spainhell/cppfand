#pragma once
#include <cstdint>
#include <vector>
#include "FieldDescr.h"


class FieldDescr;

class KeyFldD
{
public:
	KeyFldD() {}
	KeyFldD(const KeyFldD& orig, bool copyFlds);
	KeyFldD(uint8_t* inputStr);
	FieldDescr* FldD = nullptr;
	bool CompLex = false;
	bool Descend = false;
	bool static EquKFlds(std::vector<KeyFldD*>& KF1, std::vector<KeyFldD*>& KF2);
};
