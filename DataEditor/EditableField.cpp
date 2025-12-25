#include "EditableField.h"
#include "../fandio/FieldDescr.h"

bool EditableField::Ed(bool IsNewRec)
{
	return FldD->isStored() && (EdU || IsNewRec && EdN);
}