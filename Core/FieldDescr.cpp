#include "FieldDescr.h"

#include "models/FrmlElem.h"

FieldDescr::FieldDescr()
{
}

FieldDescr::FieldDescr(const FieldDescr& orig)
{
	field_type = orig.field_type;
	field_flag = orig.field_flag;
	frml_type = orig.frml_type;
	L = orig.L; M = orig.M; NBytes = orig.NBytes; Flg = orig.Flg;
	Displ = orig.Displ;
	Frml = CopyFrmlElem(orig.Frml);
	Name = orig.Name;
}
