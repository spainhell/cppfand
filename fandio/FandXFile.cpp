#include "FandXFile.h"
#include "Fand0File.h"

#include <memory>

#include "../Common/FileD.h"
#include "../Common/CommonVariables.h"

// TODO: Remove these dependencies - needed for XWork global and FandWorkXName
// XWork should be passed as parameter or use a flag instead of comparing &XWork
#include "../Core/base.h"
#include "../Core/GlobalVariables.h"

using namespace fandio;

FandXFile::FandXFile(Fand0File* parent)
	: _parent(parent), _callbacks(FandioCallbacks::Default())
{
}

FandXFile::FandXFile(Fand0File* parent, FandioCallbacks callbacks)
	: _parent(parent), _callbacks(callbacks)
{
}

FandXFile::FandXFile(const FandXFile& orig, Fand0File* parent)
	: _parent(parent), _callbacks(orig._callbacks)
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
	ReadData(2, 4, &FreeRoot);
	ReadData(6, 4, &MaxPage);
	ReadData(10, 4, &NRecs);
	ReadData(14, 4, &NRecsAbs);
	ReadData(18, 1, &NotValid);
	ReadData(19, 1, &NrKeys);
}

void FandXFile::WrPrefix(int recs, unsigned char keys)
{
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
	//SaveCache(0, _parent->Handle);
}

void FandXFile::TestErr()
{
	if (HandleError != 0) {
		Err(700 + HandleError);
	}
}

int FandXFile::UsedFileSize()
{
	return (MaxPage + 1) << XPageShft;
}

void FandXFile::ClearUpdLock()
{
	UpdLockCnt = 0;
}

int FandXFile::XFNotValid(int recs, unsigned char keys)
{
	if (Handle == nullptr) {
		// Report error via callback instead of RunError
		Error err(ErrorCode::IndexFileMissing, "Index file handle is null",
			_parent ? _parent->GetFileD()->FullPath : "", 'X');
		if (_callbacks.on_error) {
			_callbacks.on_error(err);
		}
		return 903;
	}
	else {
		SetNotValid(recs, keys);
		return 0;
	}
}

bool FandXFile::Cached() const
{
	return !NotCached();
}

bool FandXFile::NotCached() const
{
	return (this != &XWork) && _parent->GetFileD()->NotCached();
}

void FandXFile::CloseFile()
{
	if (Handle != nullptr) {
		CloseClearH(&Handle);
		ClearUpdateFlag();
		if (!_parent->IsShared()) {
			if (NotValid) {
				_parent->GetFileD()->SetPathAndVolume();
				CPath = CExtToX(CDir, CName, CExt);
				MyDeleteFile(CPath);
			}
			else if ((NRecs == 0) || _parent->NRecs == 0) {
				NRecs = 0;
				_parent->GetFileD()->SetPathAndVolume();
				CPath = CExtToX(CDir, CName, CExt);
				MyDeleteFile(CPath);
			}
		}
	}
}

void FandXFile::RdPage(XPage* P, int pageNr)
{
	P->Clean();
	if ((pageNr == 0) || (pageNr > MaxPage)) Err(831);
	// puvodne se nacitalo celych XPageSize z P, bylo nutno to rozhodit na jednotlive tridni promenne
	ReadData(pageNr << XPageShft, 1, &P->IsLeaf);
	ReadData((pageNr << XPageShft) + 1, 4, &P->GreaterPage);
	ReadData((pageNr << XPageShft) + 5, 2, &P->NItems);
	ReadData((pageNr << XPageShft) + 7, XPageSize - 7, P->A);
	P->Deserialize();
}

void FandXFile::WrPage(XPage* P, int pageNr, bool serialize)
{
	if (serialize) {
		P->Serialize();
	}
	if (UpdLockCnt > 0) Err(645);
	// puvodne se zapisovalo celych XPageSize z P, bylo nutno to rozhodit na jednotlive tridni promenne
	WriteData(pageNr << XPageShft, 1, &P->IsLeaf);
	WriteData((pageNr << XPageShft) + 1, 4, &P->GreaterPage);
	WriteData((pageNr << XPageShft) + 5, 2, &P->NItems);
	WriteData((pageNr << XPageShft) + 7, XPageSize - 7, P->A);
}

void FandXFile::WrPage(XXPage* p, int pageNr)
{
	if (UpdLockCnt > 0) Err(645);
	WriteData(pageNr << XPageShft, 1, &p->IsLeaf);
	WriteData((pageNr << XPageShft) + 1, 4, &p->GreaterPage);
	WriteData((pageNr << XPageShft) + 5, 2, &p->NItems);
	WriteData((pageNr << XPageShft) + 7, XPageSize - 7, p->A);
}

int FandXFile::NewPage(XPage* P)
{
	int result = 0;
	if (FreeRoot != 0) {
		result = FreeRoot;
		RdPage(P, FreeRoot);
		FreeRoot = P->GreaterPage;
	}
	else {
		MaxPage++;
		if (MaxPage > 0x1fffff) {
			Err(887);
		}
		result = MaxPage;
	}
	P->Clean();
	return result;
}

void FandXFile::ReleasePage(XPage* P, int N)
{
	P->Clean();
	P->GreaterPage = FreeRoot;
	FreeRoot = N;
	WrPage(P, N);
}

// Legacy error handling - now uses callback instead of direct UI
void FandXFile::Err(unsigned short N)
{
	bool isWorkFile = (this == &XWork);
	std::string filePath = isWorkFile ? FandWorkXName :
		(_parent ? _parent->GetFileD()->FullPath : "");

	Error err(static_cast<ErrorCode>(N), "", filePath, 'X');

	if (_callbacks.on_error) {
		_callbacks.on_error(err);
	}

	if (!isWorkFile && _parent) {
		_parent->XF->SetNotValid(_parent->NRecs, _parent->GetFileD()->GetNrKeys());
		_parent->Close();
		// Instead of GoExit, we just report the error
		// The caller should check for errors using Result-based API
	}
}

// New Result-based error reporting
Result<void> FandXFile::ReportError(ErrorCode code) const
{
	bool isWorkFile = (this == &XWork);
	std::string filePath = isWorkFile ? FandWorkXName :
		(_parent ? _parent->GetFileD()->FullPath : "");

	Error err(code, "", filePath, 'X');

	if (_callbacks.on_error) {
		_callbacks.on_error(err);
	}

	if (!isWorkFile && _parent) {
		_parent->XF->SetNotValid(_parent->NRecs, _parent->GetFileD()->GetNrKeys());
		_parent->Close();
	}

	return Result<void>::Err(err);
}

Result<void> FandXFile::CheckHandleError() const
{
	if (HandleError != 0) {
		return ReportError(static_cast<ErrorCode>(700 + HandleError));
	}
	return Result<void>::Ok();
}
