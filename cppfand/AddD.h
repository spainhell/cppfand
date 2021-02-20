#pragma once
#include "constants.h"

class FrmlElem;
struct LinkD;
class FileD;
class FieldDescr;
class ChkD;

class AddD // r135
{
public:
	AddD() {}
	AddD(const AddD& orig);
	AddD* Chain = nullptr;
	FieldDescr* Field = nullptr;
	FileD* File2 = nullptr;
	LinkD* LD = nullptr;
	BYTE Create = 0; // { 0-no, 1-!, 2-!! }
	FrmlElem* Frml = nullptr;
	bool Assign = false;
	FrmlElem* Bool = nullptr;
	ChkD* Chk = nullptr;
};
typedef AddD* AddDPtr;
