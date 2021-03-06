#include "XFile.h"
#include "base.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "obaseww.h"
#include "sort.h"
#include "XScan.h"
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

void XFNotValid()
{
	XFile* XF = CFile->XF;
	if (XF == nullptr) return;
	if (XF->Handle == nullptr) RunError(903);
	XF->SetNotValid();
}

void TestXFExist()
{
	XFile* xf = CFile->XF;
	if ((xf != nullptr) && xf->NotValid)
	{
		if (xf->NoCreate) CFileError(819);
		CreateIndexFile();
	}
}

void CreateIndexFile()
{
	Logging* log = Logging::getInstance();

	ExitRecord er;
	void* cr = nullptr; //void* p = nullptr;
	LockMode md = NullMode;
	bool fail = false;
	XWorkFile* XW = nullptr;
	XScan* Scan = nullptr;
	XFile* XF = nullptr;
	//NewExit(Ovr(), er);
	//goto label1;
	//MarkStore(p);
	fail = true;
	XF = CFile->XF;
	cr = CRecPtr;
	CRecPtr = GetRecSpace();
	md = NewLMode(RdMode);
	TryLockN(0, 0); /*ClearCacheCFile;*/
	if (XF->Handle == nullptr) RunError(903);
	log->log(loglevel::DEBUG, "CreateIndexFile() file 0x%p name '%s'", XF->Handle, CFile->Name.c_str());
	XF->RdPrefix();
	if (XF->NotValid) {
		XF->SetEmpty();
		//New(Scan, Init(CFile, nullptr, nullptr, false));
		Scan = new XScan(CFile, nullptr, nullptr, false);
		Scan->Reset(nullptr, false);
		//New(XW, Init(Scan, CFile->Keys));
		XW = new XWorkFile(Scan, CFile->Keys);
		XW->Main('X');
		delete XW;
		XF->NotValid = false;
		XF->WrPrefix();
		if (!SaveCache(0, CFile->Handle)) GoExit(); /*FlushHandles;*/;
	}
	fail = false;
label1:
	RestoreExit(er);
	//ReleaseStore(p);
	CRecPtr = cr;
	if (fail) {
		XF->SetNotValid();
		XF->NoCreate = true;
	}
	UnLockN(0);
	OldLMode(md);
	if (fail) GoExit();
}

void ClearXFUpdLock()
{
	if (CFile->XF != nullptr) CFile->XF->UpdLockCnt = 0;
}
