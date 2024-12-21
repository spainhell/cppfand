#include "FandFile.h"
#include "files.h"
#include "../Core/GlobalVariables.h"
#include "../Core/obaseww.h"


FandFile::FandFile()
{
	Handle = nullptr;
	_updateFlag = false;
}

FandFile::~FandFile()
= default;

void FandFile::ReadData(size_t position, size_t count, void* buf) const
{
	ReadWriteData(READ, position, count, buf);
}

void FandFile::WriteData(size_t position, size_t count, void* buf)
{
	SetUpdateFlag();
	ReadWriteData(WRITE, position, count, buf);
}

void FandFile::SetUpdateFlag()
{
	_updateFlag = true;
}

void FandFile::ClearUpdateFlag()
{
	_updateFlag = false;
}

bool FandFile::HasUpdateFlag() const
{
	return _updateFlag;
}

void FandFile::ReadWriteData(FileOperation operation, size_t position, size_t count, void* buf) const
{
	Logging* log = Logging::getInstance();

	short PgeIdx = 0, PgeRest = 0;
	WORD err = 0; int PgeNo = 0;
	//CachePage* Z = nullptr;

	if (Handle == nullptr) {
		RunError(706);
		return;
	}

	// soubor nema cache, cteme (zapisujeme) primo z disku (na disk)
	//log->log(loglevel::DEBUG, "RdWrCache() non cached file 0x%p operation.", handle);
	SeekH(Handle, position);

	if (operation == READ) {
		ReadH(Handle, count, buf);
	}
	else {
		WriteH(Handle, count, buf);
	}

	if (HandleError == 0) {
		return;
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
}
