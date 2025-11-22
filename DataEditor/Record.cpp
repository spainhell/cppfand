#include "Record.h"

Record::Record(FileD* file_d)
{
	_file_d = file_d;
	_record = file_d->GetRecSpace();
}

Record::Record(FileD* file_d, uint8_t* record)
{
	_file_d = file_d;
	_record = record;
	_delete_record_on_destroy = false;
}

Record::~Record()
{
	if (_delete_record_on_destroy) {
		delete[] _record;
		_record = nullptr;
	}
}

uint8_t* Record::GetRecord() const
{
	return _record;
}
