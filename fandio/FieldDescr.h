#pragma once
#include <string>


class FrmlElem;

// ********** CONST **********
const uint8_t LeftJust = 1;  // {RightJust=0  coded in M for Typ='N','A'}
const uint8_t Ascend = 0;
const uint8_t Descend = 6;   // {used in SortKey}
const uint8_t f_Stored = 1;
const uint8_t f_Encryp = 2;  // {FieldD flags}
const uint8_t f_Mask = 4;
const uint8_t f_Comma = 8;   // {FieldD flags}

enum class FieldType
{
	UNKNOWN, FIXED, ALFANUM, NUMERIC, DATE,	TEXT, BOOL,	REAL
};

class FieldDescr
{
public:
	FieldDescr();
	FieldDescr(const FieldDescr& orig);
	FieldType field_type = FieldType::UNKNOWN;
	std::string Name;
	std::string Mask; // added! mask for item 'A' or 'D'
	bool field_flag = false;
	char frml_type = 0;
	uint8_t L = 0;
	uint8_t M = 0;
	uint8_t NBytes = 0;
	uint8_t Flg = 0;
	short Displ = 0;
	FrmlElem* Frml = nullptr;

	bool isStored() const;
	bool isEncrypted() const;

	/* std::string S; // A, N, T
	double rdb;      // F, D, rdb
	bool B;        // B */

	static FieldType GetFieldType(char type);
	static char GetFieldTypeChar(FieldType type);
};
