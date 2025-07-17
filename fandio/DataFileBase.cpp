#include "DataFileBase.h"
#include "../Core/GlobalVariables.h"


DataFileBase::DataFileBase()
{
	Handle = nullptr;
	update_flag = false;
	CB = nullptr;
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

void DataFileBase::set_callbacks(DataFileCallbacks* callbacks)
{
	if (callbacks) {
		CB = callbacks;
	}
	else {
		CB = new DataFileCallbacks();
	}
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
		CB->runError(706);
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
		CB->runError(700 + err);
	}

	return result;
}
