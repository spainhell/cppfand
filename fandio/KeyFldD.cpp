#include "KeyFldD.h"
//#include "access.h"

KeyFldD::KeyFldD(const KeyFldD& orig, bool copyFlds)
{
	// TODO: if (orig.pChain != nullptr) pChain = new KeyFldD(*(KeyFldD*)orig.pChain);
	// v objektu FileD jsou asi ukazatele FldD a Keys->KFlds->FldD stejne
	if (copyFlds && orig.FldD != nullptr) FldD = new FieldDescr(*orig.FldD);
	CompLex = orig.CompLex;
	Descend = orig.Descend;
}

KeyFldD::KeyFldD(uint8_t* inputStr)
{
	size_t index = 0;
	// TODO: pChain = reinterpret_cast<KeyFldD*>(*(unsigned int*)&inputStr[index]); index += 4;
	FldD = reinterpret_cast<FieldDescr*>(*(unsigned int*)&inputStr[index]); index += 4;
	CompLex = *(bool*)&inputStr[index]; index++;
	Descend = *(bool*)&inputStr[index]; index++;
}

bool KeyFldD::EquKFlds(std::vector<KeyFldD*>& KF1, std::vector<KeyFldD*>& KF2)
{
	bool result = true;
	if (KF1.size() != KF2.size()) {
		result = false;
	}
	else {
		for (size_t i = 0; i < KF1.size(); i++) {
			if ((KF1[i]->CompLex != KF2[i]->CompLex) 
				|| (KF1[i]->Descend != KF2[i]->Descend)
				|| (KF1[i]->FldD->Name != KF2[i]->FldD->Name)) {
				result = false;
				break;
			}
		}
	}
	return result;
}
