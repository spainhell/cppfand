#include "KeyFldD.h"

KeyFldD::KeyFldD(const KeyFldD& orig, bool copyFlds)
{
	// TODO: if (orig.pChain != nullptr) pChain = new KeyFldD(*(KeyFldD*)orig.pChain);
	// v objektu FileD jsou asi ukazatele FldD a Keys->KFlds->FldD stejne
	if (copyFlds && orig.FldD != nullptr) FldD = new FieldDescr(*orig.FldD);
	CompLex = orig.CompLex;
	Descend = orig.Descend;
}

// Odstraneno: KeyFldD(uint8_t* inputStr) cetl z bufferu 4 bajty a delal z nich
// ukazatel FldD (rozvrzeni 32bitoveho Pascalu). Na x64 by z ukazatele zustala jen
// dolni polovina a dalsi polozky by se cetly o 4 bajty vedle. Konstruktor nikdo
// nevolal, vsechna mista pouzivaji KeyFldD(). Pokud by nekdy bylo potreba nacitat
// KeyFldD z bajtu, musi se ukladat identifikace pole, ne adresa.

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
