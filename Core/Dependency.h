#pragma once
#include "models/FrmlElem.h"

/// <summary>
/// Dependency class - #D
///	Represents a dependency definition in '#D' of chapter F
/// </summary>
class Dependency
{
public:
	Dependency(FrmlElem* condition, FrmlElem* frml);
	~Dependency();

	FrmlElem* Bool = nullptr;
	FrmlElem* Frml = nullptr;
};
