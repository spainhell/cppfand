#pragma once
#include "FileEnums.h"
#include "FandioError.h"

typedef void* HANDLE;

class DataFileBase
{
public:
	DataFileBase();
	virtual ~DataFileBase();

	HANDLE Handle;

	// New Result-based interface (preferred)
	fandio::Result<size_t> ReadDataChecked(size_t position, size_t count, void* buf) const;
	fandio::Result<size_t> WriteDataChecked(size_t position, size_t count, void* buf);

	// Legacy interface for backward compatibility (will be deprecated)
	// These return 0 on error and set HandleError
	size_t ReadData(size_t position, size_t count, void* buf) const;
	size_t WriteData(size_t position, size_t count, void* buf);

	void SetUpdateFlag();
	virtual void ClearUpdateFlag();
	bool HasUpdateFlag() const;

protected:
	bool update_flag;

private:
	fandio::Result<size_t> read_write_data(FileOperation operation, size_t position, size_t count, void* buf) const;
};

