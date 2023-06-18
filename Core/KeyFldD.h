#pragma once
#include "base.h"
#include "Chained.h"

class FieldDescr;

class KeyFldD : public Chained<KeyFldD> // ø. 108
{
public:
	KeyFldD() {}
	KeyFldD(const KeyFldD& orig, bool copyFlds);
	KeyFldD(BYTE* inputStr);
	FieldDescr* FldD = nullptr;
	bool CompLex = false, Descend = false;
	bool static EquKFlds(KeyFldD* KF1, KeyFldD* KF2);
};
