#include "FieldDescr.h"
#include "models/FrmlElem.h"


FieldDescr::FieldDescr()
{
}

FieldDescr::FieldDescr(BYTE* inputStr)
{
	size_t index = 0;
	pChain = reinterpret_cast<FieldDescr*>(*(unsigned int*)&inputStr[index]); index += 4;
	field_type = GetFieldType(*(char*)&inputStr[index]); index++;
	frml_type = *(char*)&inputStr[index]; index++;
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
	field_type = orig.field_type;
	field_flag = orig.field_flag;
	frml_type = orig.frml_type;
	L = orig.L; M = orig.M; NBytes = orig.NBytes; Flg = orig.Flg;
	Displ = orig.Displ;
	Frml = CopyFrmlElem(orig.Frml);
	Name = orig.Name;
}

bool FieldDescr::isStored() const
{
	return (Flg & f_Stored) != 0;
}

bool FieldDescr::isEncrypted() const
{
	return (Flg & f_Encryp) != 0;
}

FieldType FieldDescr::GetFieldType(char type)
{
	switch (type) {
	case 'F': return FieldType::FIXED;
	case 'A': return FieldType::ALFANUM;
	case 'N': return FieldType::NUMERIC;
	case 'D': return FieldType::DATE;
	case 'T': return FieldType::TEXT;
	case 'B': return FieldType::BOOL;
	case 'R': return FieldType::REAL;
	default: {
		return FieldType::UNKNOWN;
	}
	}
}

char FieldDescr::GetFieldTypeChar(FieldType type)
{
	switch (type) {
	case FieldType::FIXED: return 'F';
	case FieldType::ALFANUM: return 'A';
	case FieldType::NUMERIC: return 'N';
	case FieldType::DATE: return 'D';
	case FieldType::TEXT: return 'T';
	case FieldType::BOOL: return 'B';
	case FieldType::REAL: return 'R';
	default: return '?';
	}
}
