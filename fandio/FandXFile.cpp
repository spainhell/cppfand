#include "FandXFile.h"

#include <memory>

#include "files.h"
#include "../Core/FileD.h"
#include "../Core/base.h"
#include "../Core/GlobalVariables.h"
#include "XWorkFile.h"


FandXFile::FandXFile(Fand0File* parent): XWFile(parent)
{
}

FandXFile::FandXFile(const FandXFile& orig, Fand0File* parent): XWFile(parent)
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
	ReadData(2, 4, &FreeRoot);
	ReadData(6, 4, &MaxPage);
	ReadData(10, 4, &NRecs);
	ReadData(14, 4, &NRecsAbs);
	ReadData(18, 1, &NotValid);
	ReadData(19, 1, &NrKeys);
}

void FandXFile::WrPrefix(int recs, unsigned char keys)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "FandXFile::WrPrefix() 0x%p writing 20 Bytes, NRecsAbs = %i, NrKeys = %i",
	//	Handle, CFile->NRecs, CFile->GetNrKeys());
	unsigned short Signum = 0x04FF;
	WriteData(0, 2, &Signum);
	NRecsAbs = recs;
	NrKeys = keys;
	WriteData(2, 4, &FreeRoot);
	WriteData(6, 4, &MaxPage);
	WriteData(10, 4, &NRecs);
	WriteData(14, 4, &NRecsAbs);
	WriteData(18, 1, &NotValid);
	WriteData(19, 1, &NrKeys);
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

void FandXFile::SetUpdateFlag()
{
	_updateFlag = true;
}

void FandXFile::ClearUpdateFlag()
{
	_updateFlag = false;
}

bool FandXFile::HasUpdateFlag()
{
	return _updateFlag;
}

void FandXFile::CloseFile()
{
	if (Handle != nullptr) {
		CloseClearH(&Handle);
		ClearUpdateFlag();
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
