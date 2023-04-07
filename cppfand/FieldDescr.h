#pragma once
#include <string>
#include "base.h"
#include "Chained.h"
#include "constants.h"


class FrmlElem;

enum class FieldType
{
	UNKNOWN,
	FIXED,
	ALFANUM,
	NUMERIC,
	DATE,
	TEXT,
	BOOL,
	REAL
};

class FieldDescr : public Chained<FieldDescr>
{
public:
	FieldDescr();
	FieldDescr(BYTE* inputStr);
	FieldDescr(const FieldDescr& orig);
	FieldType field_type = FieldType::UNKNOWN;
	bool field_flag = false;
	char frml_type = 0;
	BYTE L = 0, M = 0, NBytes = 0, Flg = 0;
	// case boolean {Stored} of True:(Displ:short); False:(Frml:FrmlPtr; Name:string[1]{ curr.length });
	short Displ = 0;
	FrmlElem* Frml = nullptr;
	std::string Name;
	std::string Mask; // added! mask for item 'A' or 'D'
	static FieldType GetFieldType(char type);
	static char GetFieldTypeChar(FieldType type);
};
