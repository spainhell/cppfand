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

void FandFile::IncNRecs(int n)
{
#ifdef FandDemo
	if (NRecs > 100) RunError(884);
#endif
	NRecs += n;
	SetUpdHandle(Handle);
	if (file_type == FileType::INDEX) {
		SetUpdHandle(XF->Handle);
	}
}

void FandFile::DecNRecs(int N)
{
	NRecs -= N;
	SetUpdHandle(Handle);
	if (file_type == FileType::INDEX) SetUpdHandle(XF->Handle);
	WasWrRec = true;
}

void FandFile::PutRec(void* record, int& i_rec)
{
	NRecs++;
	RdWrCache(WRITE, Handle, NotCached(),i_rec * RecLen + FrstDispl, RecLen, record);
	i_rec++;
	Eof = true;
}

size_t FandFile::RecordSize()
{
	return RecLen;
}

int FandFile::_T(FieldDescr* F, void* record)
{
	int n = 0;
	short err = 0;
	char* source = (char*)record + F->Displ;

	if (file_type == FileType::DBF) {
		// tvarime se, ze CRecPtr je pstring ...
		// TODO: toto je asi blbe, nutno opravit pred 1. pouzitim
		//pstring* s = (pstring*)CRecPtr;
		//auto result = std::stoi(LeadChar(' ', *s));
		return 0; // result;
	}
	else {
		if (record == nullptr) return 0;
		return *(int*)source;
	}
}
