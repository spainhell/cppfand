#include "ThFile.h"

ThFile::ThFile(std::string APath, WORD CatIRec, InOutMode AMode, byte aCompress, ThFile* F)
{
	std::string mode;
	switch (AMode) {
	case _inp: mode = "r"; break;
	case _outp: mode = "w"; break;
	case _append: mode = "a+"; break;
	default: break;
	}
	auto HandleError = fopen_s(&Handle, APath.c_str(), mode.c_str());
	this->ReadBuf();
}

ThFile::~ThFile()
{
	fclose(Handle);
	Handle = nullptr;
}

void ThFile::ReadBuf()
{
	lBuf = fread_s(Buf, BUFFER_SIZE, 1, BUFFER_SIZE, Handle);
	if (lBuf == 0) this->eof = true;
}

void ThFile::WriteBuf(bool cond)
{
	lBuf = fwrite(Buf, 1, lBuf, Handle);
}

void ThFile::ClearBuf()
{
	memset(Buf, 0, sizeof(Buf));
	lBuf = 16384;
}
