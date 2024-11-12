#pragma once

#include "../Common/typeDef.h"

class FrmlElem;
class LinkD;
class FileD;
class FieldDescr;
class LogicControl;

class AddD // r135
{
public:
	AddD() {}
	//AddD(const AddD& orig);
	FieldDescr* Field = nullptr;
	FileD* File2 = nullptr;
	LinkD* LD = nullptr;
	BYTE Create = 0; // { 0-no, 1-!, 2-!! }
	FrmlElem* Frml = nullptr;
	bool Assign = false;
	FrmlElem* Bool = nullptr;
	LogicControl* Chk = nullptr;
};
