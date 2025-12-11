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
	uint8_t* GetRecord() const;
	FileD* GetFileD() const;
	Record* Clone() const;
	void Reset();

	bool LoadB(const Record* record, const std::string& field_name) const;
	double LoadR(const Record* record, const std::string& field_name) const;
	std::string LoadS(const Record* record, const std::string& field_name) const;

	void SaveB(Record* record, const std::string& field_name, bool value) const;
	void SaveR(Record* record, const std::string& field_name, double value) const;
	void SaveS(Record* record, const std::string& field_name, const std::string& value) const;

	void SetUpdated();
	void ClearUpdated();
	bool IsUpdated() const;

	void SetDeleted();
	void ClearDeleted();
	bool IsDeleted() const;

	uint8_t* PrepareRecord();
	void Expand();

private:
	FileD* _file_d;
	uint8_t* _buffer;
	std::vector<BRS_Value> _values;

	bool _delete_record_on_destroy = true;
	bool _updated = false;
	bool _deleted = false;

	std::vector<BRS_Value> _getValuesFromRecord();
	void _setRecordFromValues();

	FieldDescr* _getFieldDescrByName(const std::string& field_name) const;
	size_t _getFieldDescrIndexByName(const std::string& field_name) const;
};
