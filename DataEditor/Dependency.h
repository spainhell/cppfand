#pragma once
#include "../Core/models/FrmlElem.h"

class FrmlElem;

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
