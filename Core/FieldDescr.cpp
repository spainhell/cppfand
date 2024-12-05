#include "FieldDescr.h"
#include "models/FrmlElem.h"


FieldDescr::FieldDescr()
{
}

FieldDescr::FieldDescr(const FieldDescr& orig)
{
	//if (orig.pChain != nullptr) pChain = new FieldDescr(*(FieldDescr*)orig.pChain);
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
