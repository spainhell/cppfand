#pragma once
#include <string>
#include "base.h"
#include "constants.h"
//#include "models/FrmlElem.h"

class FrmlElem;

class FieldDescr : public Chained // ø. 100
{
public:
	FieldDescr();
	FieldDescr(BYTE* inputStr);
	FieldDescr(const FieldDescr& orig);
	char Typ = 0, FrmlTyp = 0;
	BYTE L = 0, M = 0, NBytes = 0, Flg = 0;
	// case boolean {Stored} of True:(Displ:integer); False:(Frml:FrmlPtr; Name:string[1]{ curr.length });
	integer Displ = 0;
	FrmlElem* Frml = nullptr;
	std::string Name;
	std::string Mask; // added! mask for item 'A' or 'D'
};
typedef FieldDescr* FieldDPtr;
