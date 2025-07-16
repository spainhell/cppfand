#pragma once
#include <cstdint>

#include "FileEnums.h"

class FileD;
typedef void* HANDLE;

struct DataFileCallbacks
{
	void (*progressOnCb)(int8_t, int32_t);
	void (*progressUpdateCb)(int32_t);
	void (*progressOffCb)();
	void (*shortMsgCb)(int32_t);
	void (*fileMsgCb)(FileD*, unsigned long, int8_t);
	void (*errorCb)(int32_t);
};

class DataFileBase
{
public:
	DataFileBase(DataFileCallbacks* callbacks);
	virtual ~DataFileBase();
	
	HANDLE Handle;

	size_t ReadData(size_t position, size_t count, void* buf) const;
	size_t WriteData(size_t position, size_t count, void* buf);

	DataFileCallbacks* get_callbacks() const;

	void SetUpdateFlag();
	virtual void ClearUpdateFlag();
	bool HasUpdateFlag() const;

protected:
	bool update_flag;
	DataFileCallbacks* CB;

private:
	size_t read_write_data(FileOperation operation, size_t position, size_t count, void* buf) const;
};

