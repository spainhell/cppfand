#include "DataFileBase.h"
#include "../Common/CommonVariables.h"
#include "../Core/base.h"

using namespace fandio;

DataFileBase::DataFileBase()
{
	Handle = nullptr;
	update_flag = false;
}

DataFileBase::~DataFileBase()
= default;

// New Result-based interface (preferred)
Result<size_t> DataFileBase::ReadDataChecked(size_t position, size_t count, void* buf) const
{
	return read_write_data(READ, position, count, buf);
}

Result<size_t> DataFileBase::WriteDataChecked(size_t position, size_t count, void* buf)
{
	SetUpdateFlag();
	return read_write_data(WRITE, position, count, buf);
}

// Legacy interface for backward compatibility
size_t DataFileBase::ReadData(size_t position, size_t count, void* buf) const
{
	auto result = read_write_data(READ, position, count, buf);
	if (result.is_ok()) {
		return result.value();
	}
	// Error is already in HandleError from read_write_data
	return 0;
}

size_t DataFileBase::WriteData(size_t position, size_t count, void* buf)
{
	SetUpdateFlag();
	auto result = read_write_data(WRITE, position, count, buf);
	if (result.is_ok()) {
		return result.value();
	}
	// Error is already in HandleError from read_write_data
	return 0;
}

void DataFileBase::SetUpdateFlag()
{
	update_flag = true;
}

void DataFileBase::ClearUpdateFlag()
{
	update_flag = false;
}

bool DataFileBase::HasUpdateFlag() const
{
	return update_flag;
}

Result<size_t> DataFileBase::read_write_data(FileOperation operation, size_t position, size_t count, void* buf) const
{
	if (Handle == nullptr) {
		return Result<size_t>::Err(ErrorCode::FileNotOpen, "File handle is null");
	}

	SeekH(Handle, position);

	size_t result = 0;
	if (operation == READ) {
		result = ReadH(Handle, count, buf);
	}
	else {
		result = WriteH(Handle, count, buf);
	}

	if (HandleError != 0) {
		ErrorCode code = static_cast<ErrorCode>(700 + HandleError);
		return Result<size_t>::Err(code,
			operation == READ ? "File read error" : "File write error");
	}

	return Result<size_t>::Ok(result);
}
