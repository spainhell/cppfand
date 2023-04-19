#include "FandFile.h"

#include "DbfFile.h"


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

void FandFile::DecNRecs(int n)
{
	NRecs -= n;
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

unsigned short FandFile::RdPrefix()
{
	// NRs - celkovy pocet zaznamu v souboru;
	// RLen - delka 1 zaznamu
	struct x6 { int NRs = 0; unsigned short RLen = 0; } X6;
	struct x8 { unsigned short NRs = 0, RLen = 0; } X8;
	struct xD {
		unsigned char Ver = 0; unsigned char Date[3] = { 0,0,0 };
		int NRecs = 0;
		unsigned short HdLen = 0; unsigned short RecLen = 0;
	} XD;
	auto result = 0xffff;
	const bool not_cached = NotCached();
	switch (file_type) {
	case FileType::FAND8: {
		RdWrCache(READ, Handle, not_cached, 0, 2, &X8.NRs);
		RdWrCache(READ, Handle, not_cached, 2, 2, &X8.RLen);
		NRecs = X8.NRs;
		if (RecLen != X8.RLen) { return X8.RLen; }
		break;
	}
	case FileType::DBF: {
		RdWrCache(READ, Handle, not_cached, 0, 1, &XD.Ver);
		RdWrCache(READ, Handle, not_cached, 1, 1, &XD.Date[0]);
		RdWrCache(READ, Handle, not_cached, 2, 1, &XD.Date[1]);
		RdWrCache(READ, Handle, not_cached, 3, 1, &XD.Date[2]);
		RdWrCache(READ, Handle, not_cached, 4, 4, &XD.NRecs);
		RdWrCache(READ, Handle, not_cached, 8, 2, &XD.HdLen);
		RdWrCache(READ, Handle, not_cached, 10, 2, &XD.RecLen);
		NRecs = XD.NRecs;
		if ((RecLen != XD.RecLen)) { return XD.RecLen; }
		FrstDispl = XD.HdLen;
		break;
	}
	default: {
		RdWrCache(READ, Handle, not_cached, 0, 4, &X6.NRs);
		RdWrCache(READ, Handle, not_cached, 4, 2, &X6.RLen);
		NRecs = abs(X6.NRs);
		if ((X6.NRs < 0) && (file_type != FileType::INDEX) || (X6.NRs > 0) && (file_type == FileType::INDEX)
			|| (RecLen != X6.RLen)) {
			return X6.RLen;
		}
		break;
	}
	}
	return result;
}

int FandFile::RdPrefixes()
{
	if (RdPrefix() != 0xffff) {
		//CFileError(CFile, 883);
		return 883;
	}
	if ((XF != nullptr) && (XF->Handle != nullptr)) {
		XF->RdPrefix();
	}
	if ((TF != nullptr)) {
		TF->RdPrefix(false);
	}
	return 0;
}

void FandFile::WrPrefix()
{
	struct { int NRs; unsigned short RLen; } Pfx6 = { 0, 0 };
	struct { unsigned short NRs; unsigned short RLen; } Pfx8 = { 0, 0 };

	if (IsUpdHandle(Handle)) {
		const bool not_cached = NotCached();
		switch (file_type) {
		case FileType::FAND8: {
			Pfx8.RLen = RecLen;
			Pfx8.NRs = static_cast<unsigned short>(NRecs);
			RdWrCache(WRITE, Handle, not_cached, 0, 2, &Pfx8.NRs);
			RdWrCache(WRITE, Handle, not_cached, 2, 2, &Pfx8.RLen);
			break;
		}
		case FileType::DBF: {
			DbfFile::WrDBaseHd(); // TODO: doplnit parametry
			break;
		}
		default: {
			Pfx6.RLen = RecLen;
			if (file_type == FileType::INDEX) Pfx6.NRs = -NRecs;
			else Pfx6.NRs = NRecs;
			RdWrCache(WRITE, Handle, not_cached, 0, 4, &Pfx6.NRs);
			RdWrCache(WRITE, Handle, not_cached, 4, 2, &Pfx6.RLen);
		}
		}
	}
}

void FandFile::WrPrefixes()
{
	WrPrefix();
	if (TF != nullptr && IsUpdHandle(TF->Handle)) {
		TF->WrPrefix();
	}
	if (file_type == FileType::INDEX && XF->Handle != nullptr
		&& /*{ call from CopyDuplF }*/ (IsUpdHandle(XF->Handle) || IsUpdHandle(Handle))) {
		XF->WrPrefix();
	}
}

void FandFile::CloseFile()
{

}
