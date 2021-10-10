#include "ThFile.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"


ThFile::ThFile(std::string APath, WORD CatIRec, InOutMode AMode, byte aCompress, ThFile* F)
{
	std::string mode;
	std::string d, Nm, e;
	switch (AMode) {
	case _inp: mode = "r"; break;
	case _outp: mode = "w"; break;
	case _append: mode = "a+"; break;
	default: break;
	}

	compress = aCompress;

	SetTxtPathVol(APath, CatIRec); Path = CPath; Vol = CVol;

	if (F != nullptr && F->Path == CPath) {
		SetMsgPar(CPath);
		RunError(660);
	}

	//if (CVol != "#" && CVol != "" && pos('.', CPath) == l - 3 && CPath[l] >= '0' && CPath[l] <= '9' && CPath[l - 1] >= '0' && CPath[l - 1] <= '4') Floppy = true;
	FSplit(CPath, d, Nm, e);
	//ForAllFDs(CloseEqualFD);

	HandleError = fopen_s(&Handle, CPath.c_str(), mode.c_str());
	if (Handle != nullptr) {
		this->ReadBuf();
	}
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
