#pragma once
#include "FieldDescr.h"

/// <summary>
/// Implicit Value class - #I
///	Represents an implicit value definition in '#I' of chapter F
/// </summary>
class ImplD
{
public:
	ImplD(FieldDescr* field, FrmlElem* frml);
	FieldDescr* FldD = nullptr;
	FrmlElem* Frml = nullptr;
};

