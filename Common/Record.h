#pragma once
#include <string>
#include <vector>

#include "../Common/FileD.h"

enum class ValueType : uint8_t {
	UNKNOWN = 0,
	F,  // Float
	A,  // Alphanumeric
	N,  // Numeric
	D,  // Date
	B,  // Boolean
	T,  // Text
	R   // Real
};

class BRS_Value
{
public:
	ValueType value_type = ValueType::UNKNOWN;

	bool B = false;
	double R = 0.0;
	std::string S;

	void Reset() { B = false; R = 0.0; S = ""; }
};

class Record
{
public:
	Record();
	Record(FileD* file_d);
	~Record();
	void CopyTo(Record* dst_record) const;
	//uint8_t* GetRecord();	// prepares record buffer from values and returns it
	FileD* GetFileD() const;
	Record* Clone() const;
	
	void Clear(); // delete the vector of values
	void Reset(); // all values to default

	bool LoadB(FieldDescr* field) const;
	double LoadR(FieldDescr* field) const;
	std::string LoadS(FieldDescr* field) const;

	void SaveB(FieldDescr* field, bool value);
	void SaveR(FieldDescr* field, double value);
	void SaveS(FieldDescr* field, const std::string& value);

	void SetUpdated();
	void ClearUpdated();
	bool IsUpdated() const;

	void SetDeleted(bool set_updated = true);
	void ClearDeleted();
	bool IsDeleted() const;

	std::map<std::string, BRS_Value> _values;

	static uint8_t Compare(Record* rec1, Record* rec2);

private:
	FileD* _file_d;
	
	bool _updated = false;
	bool _deleted = false;

	FieldDescr* _getFieldDescrByName(const std::string& field_name) const;
};
