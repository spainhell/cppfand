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
	Record(FileD* file_d);
	Record(FileD* file_d, uint8_t* record);
	~Record();
	uint8_t* GetRecord() const;
	Record* Clone() const;
	void Reset();
	void SetRecordUpdateFlag();
	uint8_t* PrepareRecord();

private:
	FileD* _file_d;
	uint8_t* _record;
	std::vector<BRS_Value> _values;

	bool _delete_record_on_destroy = true;
	bool _was_updated = false;

	std::vector<BRS_Value> _getValuesFromRecord();
	void _setRecordFromValues();
};
