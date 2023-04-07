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
	std::string Name;
	std::string Mask; // added! mask for item 'A' or 'D'
	bool field_flag = false;
	char frml_type = 0;
	BYTE L = 0;
	BYTE M = 0;
	BYTE NBytes = 0;
	BYTE Flg = 0;
	short Displ = 0;
	FrmlElem* Frml = nullptr;

	/* std::string S; // A, N, T
	double R;      // F, D, R
	bool B;        // B */

	static FieldType GetFieldType(char type);
	static char GetFieldTypeChar(FieldType type);
};
