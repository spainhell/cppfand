#pragma once
#include <cstdint>
#include <vector>

#include "DBaseField.h"

const int DBaseHeaderSize = 32;

class DBaseHeader
{
public:
	DBaseHeader();
	~DBaseHeader();
	uint8_t Ver = 0;
	uint8_t Date[3]{ 0,0,0 };
	int32_t NRecs = 0;
	uint16_t HdLen = 0;
	uint16_t RecLen = 0;
	std::vector<DBaseField*> flds;
	uint8_t* GetData();
	size_t GetDataLength();

private:
	uint8_t data_[DBaseHeaderSize] = {};
};
