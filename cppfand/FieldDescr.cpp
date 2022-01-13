#include "FieldDescr.h"
#include "models/FrmlElem.h"

FieldDescr::FieldDescr()
{
}

FieldDescr::FieldDescr(BYTE* inputStr)
{
	size_t index = 0;
	pChain = reinterpret_cast<FieldDescr*>(*(unsigned int*)&inputStr[index]); index += 4;
	Typ = *(char*)&inputStr[index]; index++;
	FrmlTyp = *(char*)&inputStr[index]; index++;
	L = *(char*)&inputStr[index]; index++;
	M = *(char*)&inputStr[index]; index++;
	NBytes = *(char*)&inputStr[index]; index++;
	Flg = *(char*)&inputStr[index]; index++;

	unsigned int DisplOrFrml = *(unsigned int*)&inputStr[index]; index += 4;
	if (DisplOrFrml > MaxTxtCols) {
		// jedna se o ukazatel
		Frml = reinterpret_cast<FrmlElem*>(DisplOrFrml);
	}
	else {
		// jedna se o delku
		Displ = DisplOrFrml;
	}
	Name[0] = inputStr[index]; index++;
	memcpy(&Name[1], &inputStr[index], Name[0]); index += Name[0];
}

FieldDescr::FieldDescr(const FieldDescr& orig)
{
	if (orig.pChain != nullptr) pChain = new FieldDescr(*(FieldDescr*)orig.pChain);
	Typ = orig.Typ;
	FrmlTyp = orig.FrmlTyp;
	L = orig.L; M = orig.M; NBytes = orig.NBytes; Flg = orig.Flg;
	Displ = orig.Displ;
	Frml = CopyFrmlElem(orig.Frml);
	Name = orig.Name;
}
