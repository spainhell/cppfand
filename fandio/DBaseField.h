#pragma once

#include <string>

const int DBaseFieldSize = 32;

class DBaseField
{
public:
	std::string name;
	char typ = 0;
	int displ = 0;
	uint8_t len = 0;
	uint8_t dec = 0;
	//uint8_t x2[14];
	uint8_t* GetData();
	size_t GetDataLength();

private:
	uint8_t data_[DBaseFieldSize] = {};
};
