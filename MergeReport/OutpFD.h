#pragma once
#include "../fandio/FileEnums.h"
#include <cstdint>

class FileD;

struct OutpFD
{
	FileD* FD = nullptr;
	LockMode Md = NullMode;
	uint8_t* RecPtr = nullptr;
	FileD* InplFD = nullptr;
	bool Append = false;
#ifdef FandSQL
	SQLStreamPtr Strm;
#endif
};
