#include "XFile.h"
#include "base.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "../Logging/Logging.h"

XFile::XFile(const XFile& orig)
{
	NRecs = orig.NRecs;
	NRecsAbs = orig.NRecsAbs;
	NotValid = orig.NotValid;
	NrKeys = orig.NrKeys;
	NoCreate = orig.NoCreate;
	FirstDupl = orig.FirstDupl;
}

void XFile::SetEmpty()
{
	auto p = new XPage(); // (XPage*)GetZStore(XPageSize);
	WrPage(p, 0);
	p->IsLeaf = true;
	FreeRoot = 0;
	NRecs = 0;
	KeyD* k = CFile->Keys;
	while (k != nullptr) {
		longint n = k->IndexRoot;
		MaxPage = n;
		WrPage(p, n);
		k = k->Chain;
	}
	ReleaseStore(p);
	WrPrefix();
}

void XFile::RdPrefix()
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "XFile::RdPrefix() 0x%p reading 18 Bytes", Handle);
	RdWrCache(true, Handle, NotCached(), 2, 4, &FreeRoot);
	RdWrCache(true, Handle, NotCached(), 6, 4, &MaxPage);
	RdWrCache(true, Handle, NotCached(), 10, 4, &NRecs);
	RdWrCache(true, Handle, NotCached(), 14, 4, &NRecsAbs);
	RdWrCache(true, Handle, NotCached(), 18, 1, &NotValid);
	RdWrCache(true, Handle, NotCached(), 19, 1, &NrKeys);
}

void XFile::WrPrefix()
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "XFile::WrPrefix() 0x%p writing 20 Bytes, NRecsAbs = %i, NrKeys = %i",
		Handle, CFile->NRecs, CFile->GetNrKeys());
	WORD Signum = 0x04FF;
	RdWrCache(false, Handle, NotCached(), 0, 2, &Signum);
	NRecsAbs = CFile->NRecs;
	NrKeys = CFile->GetNrKeys();
	RdWrCache(false, Handle, NotCached(), 2, 4, &FreeRoot);
	RdWrCache(false, Handle, NotCached(), 6, 4, &MaxPage);
	RdWrCache(false, Handle, NotCached(), 10, 4, &NRecs);
	RdWrCache(false, Handle, NotCached(), 14, 4, &NRecsAbs);
	RdWrCache(false, Handle, NotCached(), 18, 1, &NotValid);
	RdWrCache(false, Handle, NotCached(), 19, 1, &NrKeys);

}

void XFile::SetNotValid()
{
	NotValid = true;
	MaxPage = 0;
	WrPrefix();
	SaveCache(0, CFile->Handle);
}
