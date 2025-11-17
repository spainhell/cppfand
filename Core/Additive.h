#pragma once

#include "../Common/typeDef.h"

class FrmlElem;
class LinkD;
class FileD;
class FieldDescr;
class LogicControl;


/// <summary>
/// Additive changes class - #A
///	Represents an additive change definition in '#A' of chapter F
/// </summary>
class Additive
{
public:
	Additive();
	//Additive(const Additive& orig);
	FieldDescr* Field = nullptr;
	FileD* File2 = nullptr;
	LinkD* LD = nullptr;
	uint8_t Create = 0; // { 0-no, 1-!, 2-!! }
	FrmlElem* Frml = nullptr;
	bool Assign = false;
	FrmlElem* Bool = nullptr;
	LogicControl* Chk = nullptr;
};
