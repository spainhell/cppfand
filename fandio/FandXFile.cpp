#include "FandXFile.h"

#include <memory>
#include "../cppfand/FileD.h"
#include "../cppfand/base.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/obaseww.h"
#include "sort.h"
#include "XScan.h"
#include "XWorkFile.h"


FandXFile::FandXFile(const FandXFile& orig)
{
	NRecs = orig.NRecs;
	NRecsAbs = orig.NRecsAbs;
	NotValid = orig.NotValid;
	NrKeys = orig.NrKeys;
	NoCreate = orig.NoCreate;
	FirstDupl = orig.FirstDupl;
}

void FandXFile::SetEmpty()
{
	auto p = std::make_unique<XPage>();
	WrPage(p.get(), 0);
	p->IsLeaf = true;
	FreeRoot = 0;
	NRecs = 0;
	for (auto& k : CFile->Keys) {
		int n = k->IndexRoot;
		MaxPage = n;
		WrPage(p.get(), n);
	}
	WrPrefix();
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

void FandXFile::WrPrefix()
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "FandXFile::WrPrefix() 0x%p writing 20 Bytes, NRecsAbs = %i, NrKeys = %i",
	//	Handle, CFile->NRecs, CFile->GetNrKeys());
	unsigned short Signum = 0x04FF;
	RdWrCache(WRITE, Handle, NotCached(), 0, 2, &Signum);
	NRecsAbs = CFile->FF->NRecs;
	NrKeys = CFile->GetNrKeys();
	RdWrCache(WRITE, Handle, NotCached(), 2, 4, &FreeRoot);
	RdWrCache(WRITE, Handle, NotCached(), 6, 4, &MaxPage);
	RdWrCache(WRITE, Handle, NotCached(), 10, 4, &NRecs);
	RdWrCache(WRITE, Handle, NotCached(), 14, 4, &NRecsAbs);
	RdWrCache(WRITE, Handle, NotCached(), 18, 1, &NotValid);
	RdWrCache(WRITE, Handle, NotCached(), 19, 1, &NrKeys);

}

void FandXFile::SetNotValid()
{
	NotValid = true;
	MaxPage = 0;
	WrPrefix();
	SaveCache(0, CFile->FF->Handle);
}

void XFNotValid()
{
	FandXFile* XF = CFile->FF->XF;
	if (XF == nullptr) return;
	if (XF->Handle == nullptr) RunError(903);
	XF->SetNotValid();
}

void TestXFExist()
{
	FandXFile* xf = CFile->FF->XF;
	if ((xf != nullptr) && xf->NotValid) {
		if (xf->NoCreate) {
			CFileError(CFile, 819);
		}
		CreateIndexFile();
	}
}

void CreateIndexFile()
{
	Logging* log = Logging::getInstance();

	void* cr = nullptr;
	LockMode md = NullMode;
	bool fail = false;
	FandXFile* XF = nullptr;

	try {
		fail = true;
		XF = CFile->FF->XF;
		cr = CRecPtr;
		CRecPtr = GetRecSpace(CFile->FF);
		md = NewLMode(CFile, RdMode);
		TryLockN(CFile->FF, 0, 0);
		/*ClearCacheCFile;*/
		if (XF->Handle == nullptr) RunError(903);
		log->log(loglevel::DEBUG, "CreateIndexFile() file 0x%p name '%s'", XF->Handle, CFile->Name.c_str());
		XF->RdPrefix();
		if (XF->NotValid) {
			XF->SetEmpty();
			XScan* Scan = new XScan(CFile, nullptr, nullptr, false);
			Scan->Reset(nullptr, false);
			XWorkFile* XW = new XWorkFile(Scan, CFile->Keys[0]);
			XW->Main('X');
			delete XW; XW = nullptr;
			XF->NotValid = false;
			XF->WrPrefix();
			if (!SaveCache(0, CFile->FF->Handle)) GoExit();
			/*FlushHandles; */
		}
		fail = false;
	}
	catch (std::exception& e) {
		// TODO: log error
	}

	CRecPtr = cr;
	if (fail) {
		XF->SetNotValid();
		XF->NoCreate = true;
	}
	UnLockN(CFile->FF, 0);
	OldLMode(CFile, md);
	if (fail) GoExit();
}

void ClearXFUpdLock()
{
	if (CFile->FF->XF != nullptr) {
		CFile->FF->XF->UpdLockCnt = 0;
	}
}
