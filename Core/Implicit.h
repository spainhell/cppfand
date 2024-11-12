#pragma once
#include "FieldDescr.h"

/// <summary>
/// Implicit Value class - #I
///	Represents an implicit value definition in '#I' of chapter F
/// </summary>
class Implicit
{
public:
	Implicit(FieldDescr* field, FrmlElem* frml);
	FieldDescr* FldD = nullptr;
	FrmlElem* Frml = nullptr;
};

