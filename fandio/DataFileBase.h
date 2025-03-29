#pragma once
#include "../Common/FileEnums.h"

typedef void* HANDLE;

class DataFileBase
{
public:
	DataFileBase();
	virtual ~DataFileBase();
	
	HANDLE Handle;

	size_t ReadData(size_t position, size_t count, void* buf) const;
	size_t WriteData(size_t position, size_t count, void* buf);

	void SetUpdateFlag();
	virtual void ClearUpdateFlag();
	bool HasUpdateFlag() const;

protected:
	bool update_flag;

private:
	size_t read_write_data(FileOperation operation, size_t position, size_t count, void* buf) const;
};

