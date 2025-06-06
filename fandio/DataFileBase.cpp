#include "DataFileBase.h"
#include "../Core/GlobalVariables.h"
#include "../Core/obaseww.h"


DataFileBase::DataFileBase()
{
	Handle = nullptr;
	update_flag = false;
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
		RunError(706);
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
		RunError(700 + err);
	}

	return result;
}
