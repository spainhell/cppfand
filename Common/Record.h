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
};
