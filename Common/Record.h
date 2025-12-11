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
	Record(FileD* file_d, uint8_t* record, bool record_owner = false);
	~Record();
	void CopyTo(Record* dst_record) const;
	uint8_t* GetRecord();	// prepares record buffer from values and returns it
	FileD* GetFileD() const;
	Record* Clone() const;
	void Reset();
	void Expand();

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

private:
	FileD* _file_d;
	uint8_t* _buffer;
	std::vector<BRS_Value> _values;

	bool _delete_record_on_destroy = true;
	bool _updated = false;
	bool _deleted = false;

	void _getValuesFromRecord();
	void _setRecordFromValues();

	FieldDescr* _getFieldDescrByName(const std::string& field_name) const;
	size_t _getFieldDescrIndexByName(const std::string& field_name) const;
};
