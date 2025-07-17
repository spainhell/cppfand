#include "DataFileBase.h"
#include "../Core/GlobalVariables.h"


DataFileBase::DataFileBase(DataFileCallbacks* callbacks)
{
	Handle = nullptr;
	update_flag = false;
	if (callbacks == nullptr) {
		CB = new DataFileCallbacks();
	}
	else {
		CB = callbacks;
	}
}

DataFileBase::~DataFileBase()
= default;

size_t DataFileBase::ReadData(size_t position, size_t count, void* buf) const
{
	return read_write_data(READ, position, count, buf);
}

size_t DataFileBase::WriteData(size_t position, size_t count, void* buf)
{
	SetUpdateFlag();
	return read_write_data(WRITE, position, count, buf);
}

DataFileCallbacks* DataFileBase::get_callbacks() const
{
	return CB;
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

size_t DataFileBase::read_write_data(FileOperation operation, size_t position, size_t count, void* buf) const
{
	Logging* log = Logging::getInstance();
	size_t result = 0;

	WORD err = 0;

	if (Handle == nullptr) {
		if (CB->errorCb) {
			// RunError(706);
			CB->errorCb(706);
		}
		return result;
	}

	SeekH(Handle, position);

	if (operation == READ) {
		result = ReadH(Handle, count, buf);
	}
	else {
		result = WriteH(Handle, count, buf);
	}

	if (HandleError == 0) {

	}
	else {
		if (operation == READ) {
			log->log(loglevel::DEBUG, "RdWrCache(READ) non cached file 0x%p operation error: %i.", Handle, HandleError);
		}
		else {
			log->log(loglevel::DEBUG, "RdWrCache(WRITE) non cached file 0x%p operation error: %i.", Handle, HandleError);
		}

		err = HandleError;
		FileD::SetPathForH(Handle);
		SetMsgPar(CPath);
		if (CB->errorCb) {
			// RunError(700 + err);
			CB->errorCb(700 + err);
		}
	}

	return result;
}
