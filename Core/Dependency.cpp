#include "Dependency.h"

Dependency::Dependency(FrmlElem* condition, FrmlElem* frml)
{
	Bool = condition;
	Frml = frml;
}

Dependency::~Dependency()
{
	delete Bool; Bool = nullptr;
	delete Frml; Frml = nullptr;
}