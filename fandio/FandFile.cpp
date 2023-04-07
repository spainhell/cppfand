#include "FandFile.h"

FandFile::FandFile()
{
}

FandFile::FandFile(const FandFile& orig)
{
	RecLen = orig.RecLen;
	file_type = orig.file_type;
	Drive = orig.Drive;

	if (orig.TF != nullptr) TF = new FandTFile(*orig.TF);
	if (orig.XF != nullptr) XF = new FandXFile(*orig.XF);
}


int FandFile::UsedFileSize()
{
	int n = int(NRecs) * RecLen + FrstDispl;
	if (file_type == FileType::DBF) n++;
	return n;
}

bool FandFile::IsShared()
{
	return (UMode == Shared) || (UMode == RdShared);
}

bool FandFile::NotCached()
{
	if (UMode == Shared) goto label1;
	if (UMode != RdShared) return false;
label1:
	if (LMode == ExclMode) return false;
	return true;
}

bool FandFile::Cached()
{
	return !NotCached();
}

void FandFile::Reset()
{
	RecLen = 0;
	RecPtr = nullptr;
	NRecs = 0;
	FrstDispl = 0;
	WasWrRec = false; WasRdOnly = false; Eof = false;
	file_type = FileType::UNKNOWN;        // 8= Fand 8; 6= Fand 16; X= .X; 0= RDB; C= CAT 
	Handle = nullptr;
	TF = nullptr;
	Drive = 0;         // 1=A, 2=B, else 0
	UMode = FileUseMode::Closed;
	LMode = NullMode; ExLMode = NullMode; TaLMode = NullMode;
	XF = nullptr;
}
