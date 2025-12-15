#pragma once
#include <string>
#include <vector>

#include "../Common/FileD.h"


class BRS_Value
{
public:
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

	bool LoadB(const std::string& field_name) const;
	double LoadR(const std::string& field_name) const;
	std::string LoadS(const std::string& field_name) const;

	void SaveB(const std::string& field_name, bool value);
	void SaveR(const std::string& field_name, double value);
	void SaveS(const std::string& field_name, const std::string& value);

	void SetUpdated();
	void ClearUpdated();
	bool IsUpdated() const;

	void SetDeleted();
	void ClearDeleted();
	bool IsDeleted() const;

	std::vector<BRS_Value> _values;

	static uint8_t Compare(Record* rec1, Record* rec2);

private:
	FileD* _file_d;
	
	bool _updated = false;
	bool _deleted = false;

	FieldDescr* _getFieldDescrByName(const std::string& field_name) const;
	size_t _getFieldDescrIndexByName(const std::string& field_name) const;
};
