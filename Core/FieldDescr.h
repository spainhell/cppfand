#pragma once
#include "../Common/FieldDescrBase.h"
#include "base.h"

class FrmlElem;

class FieldDescr : public FieldDescrBase
{
	public:
	FieldDescr();
	FieldDescr(const FieldDescr& orig);

	FrmlElem* Frml = nullptr;
};

