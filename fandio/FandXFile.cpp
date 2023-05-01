#include "FandXFile.h"

#include <memory>

#include "files.h"
#include "../cppfand/FileD.h"
#include "../cppfand/base.h"
#include "../cppfand/GlobalVariables.h"
#include "XWorkFile.h"


FandXFile::FandXFile(FandFile* parent): XWFile(parent)
{
}

FandXFile::FandXFile(const FandXFile& orig, FandFile* parent): XWFile(parent)
{
	NRecs = orig.NRecs;
	NRecsAbs = orig.NRecsAbs;
	NotValid = orig.NotValid;
	NrKeys = orig.NrKeys;
	NoCreate = orig.NoCreate;
	FirstDupl = orig.FirstDupl;
}

FandXFile::~FandXFile()
{
	if (Handle != nullptr) {
		CloseH(&Handle);
	}
}

void FandXFile::SetEmpty(int recs, unsigned char keys)
{
	auto p = std::make_unique<XPage>();
	WrPage(p.get(), 0);
	p->IsLeaf = true;
	FreeRoot = 0;
	NRecs = 0;
	for (auto& k : _parent->GetFileD()->Keys) {
		int n = k->IndexRoot;
		MaxPage = n;
		WrPage(p.get(), n);
	}
	WrPrefix(recs, keys);
}

void FandXFile::RdPrefix()
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "FandXFile::RdPrefix() 0x%p reading 18 Bytes", Handle);
	RdWrCache(READ, Handle, NotCached(), 2, 4, &FreeRoot);
	RdWrCache(READ, Handle, NotCached(), 6, 4, &MaxPage);
	RdWrCache(READ, Handle, NotCached(), 10, 4, &NRecs);
	RdWrCache(READ, Handle, NotCached(), 14, 4, &NRecsAbs);
	RdWrCache(READ, Handle, NotCached(), 18, 1, &NotValid);
	RdWrCache(READ, Handle, NotCached(), 19, 1, &NrKeys);
}

void FandXFile::WrPrefix(int recs, unsigned char keys)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "FandXFile::WrPrefix() 0x%p writing 20 Bytes, NRecsAbs = %i, NrKeys = %i",
	//	Handle, CFile->NRecs, CFile->GetNrKeys());
	unsigned short Signum = 0x04FF;
	RdWrCache(WRITE, Handle, NotCached(), 0, 2, &Signum);
	NRecsAbs = recs;
	NrKeys = keys;
	RdWrCache(WRITE, Handle, NotCached(), 2, 4, &FreeRoot);
	RdWrCache(WRITE, Handle, NotCached(), 6, 4, &MaxPage);
	RdWrCache(WRITE, Handle, NotCached(), 10, 4, &NRecs);
	RdWrCache(WRITE, Handle, NotCached(), 14, 4, &NRecsAbs);
	RdWrCache(WRITE, Handle, NotCached(), 18, 1, &NotValid);
	RdWrCache(WRITE, Handle, NotCached(), 19, 1, &NrKeys);
}

void FandXFile::SetNotValid(int recs, unsigned char keys)
{
	NotValid = true;
	MaxPage = 0;
	WrPrefix(recs, keys);
	SaveCache(0, _parent->Handle);
}

void FandXFile::ClearUpdLock()
{
	UpdLockCnt = 0;
}

int FandXFile::XFNotValid(int recs, unsigned char keys)
{
	if (Handle == nullptr) {
		//RunError(903);
		return 903;
	}
	else {
		SetNotValid(recs, keys);
		return 0;
	}
}

void FandXFile::CloseFile()
{
	if (Handle != nullptr) {
		CloseClearH(&Handle);
		if (!_parent->IsShared()) {
			if (NotValid) {
				SetPathAndVolume(_parent->GetFileD());
				CPath = CExtToX(CDir, CName, CExt);
				MyDeleteFile(CPath);
			}
			else if ((NRecs == 0) || _parent->NRecs == 0) {
				NRecs = 0;
				SetPathAndVolume(_parent->GetFileD());
				CPath = CExtToX(CDir, CName, CExt);
				MyDeleteFile(CPath);
			}
		}
	}
}
