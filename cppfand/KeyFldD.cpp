#include "KeyFldD.h"
#include "access.h"

KeyFldD::KeyFldD(const KeyFldD& orig, bool copyFlds)
{
	if (orig.Chain != nullptr) Chain = new KeyFldD(*(KeyFldD*)orig.Chain);
	// v objektu FileD jsou asi ukazatele FldD a Keys->KFlds->FldD stejne
	if (copyFlds && orig.FldD != nullptr) FldD = new FieldDescr(*orig.FldD);
	CompLex = orig.CompLex;
	Descend = orig.Descend;
}

KeyFldD::KeyFldD(BYTE* inputStr)
{
	size_t index = 0;
	Chain = reinterpret_cast<KeyFldD*>(*(unsigned int*)&inputStr[index]); index += 4;
	FldD = reinterpret_cast<FieldDescr*>(*(unsigned int*)&inputStr[index]); index += 4;
	CompLex = *(bool*)&inputStr[index]; index++;
	Descend = *(bool*)&inputStr[index]; index++;
}
