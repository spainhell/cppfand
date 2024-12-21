#pragma once
#include "../Common/FileEnums.h"

typedef void* HANDLE;

class FandFile
{
public:
	FandFile();
	virtual ~FandFile();
	
	HANDLE Handle;

	void ReadData(size_t position, size_t count, void* buf);
	void WriteData(size_t position, size_t count, void* buf);

private:
	void ReadWriteData(FileOperation operation, size_t position, size_t count, void* buf);
};

