#pragma once
#include "../Common/FileEnums.h"

class FileD;

struct OutpFD
{
	FileD* FD = nullptr;
	LockMode Md = NullMode;
	void* RecPtr = nullptr;
	FileD* InplFD = nullptr;
	bool Append = false;
#ifdef FandSQL
	SQLStreamPtr Strm;
#endif
};
