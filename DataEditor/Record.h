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
};

class Record
{
public:
	Record(FileD* file_d);
	Record(FileD* file_d, uint8_t* record);
	~Record();
	uint8_t* GetRecord() const;

private:
	FileD* _file_d;
	uint8_t* _record;
	std::vector<BRS_Value> _values;

	bool _delete_record_on_destroy = true;
};
