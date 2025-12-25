#pragma once
#include "../fandio/FileEnums.h"
#include <cstdint>

class FileD;
class Record;

struct OutpFD
{
	FileD* FD = nullptr;
	LockMode Md = NullMode;
	Record* RecPtr = nullptr;
	FileD* InplFD = nullptr;
	bool Append = false;
#ifdef FandSQL
	SQLStreamPtr Strm;
#endif
};
