#include "FandFile.h"
#include "files.h"
#include "../Core/GlobalVariables.h"
#include "../Core/obaseww.h"


FandFile::FandFile()
{
	Handle = nullptr;
	update_flag = false;
}

FandFile::~FandFile()
= default;

size_t FandFile::ReadData(size_t position, size_t count, void* buf) const
{
	return ReadWriteData(READ, position, count, buf);
}

size_t FandFile::WriteData(size_t position, size_t count, void* buf)
{
	SetUpdateFlag();
	return ReadWriteData(WRITE, position, count, buf);
}

void FandFile::SetUpdateFlag()
{
	update_flag = true;
}

void FandFile::ClearUpdateFlag()
{
	update_flag = false;
}

bool FandFile::HasUpdateFlag() const
{
	return update_flag;
}

size_t FandFile::ReadWriteData(FileOperation operation, size_t position, size_t count, void* buf) const
{
	Logging* log = Logging::getInstance();
	size_t result = 0;

	short PgeIdx = 0, PgeRest = 0;
	WORD err = 0; int PgeNo = 0;
	//CachePage* Z = nullptr;

	if (Handle == nullptr) {
		RunError(706);
		return result;
	}

	// soubor nema cache, cteme (zapisujeme) primo z disku (na disk)
	//log->log(loglevel::DEBUG, "RdWrCache() non cached file 0x%p operation.", handle);
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
		SetPathForH(Handle);
		SetMsgPar(CPath);
		RunError(700 + err);
	}

	return result;
}
