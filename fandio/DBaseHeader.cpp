#include "DBaseHeader.h"

DBaseHeader::DBaseHeader()
{
}

DBaseHeader::~DBaseHeader()
{
	for (DBaseField* fld : flds) {
		delete fld;
	}
}

uint8_t* DBaseHeader::GetData()
{
	memset(data_, 0, DBaseHeaderSize);
	memcpy(data_, &Ver, 1);
	memcpy(data_ + 1, Date, 3);
	memcpy(data_ + 4, &NRecs, 4);
	memcpy(data_ + 8, &HdLen, 2);
	memcpy(data_ + 10, &RecLen, 2);
	return data_;
}

size_t DBaseHeader::GetDataLength()
{
	return DBaseHeaderSize;
}
