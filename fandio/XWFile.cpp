#include "XWFile.h"

#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/obaseww.h"
#include "../fandio/FandXFile.h"

void XWFile::Err(unsigned short N)
{
	if (this == &XWork) {
		SetMsgPar(FandWorkXName);
		RunError(N);
	}
	else {
		CFile->FF->XF->SetNotValid();
		CFileMsg(N, 'X');
		CloseGoExit(CFile->FF);
	}
}

void XWFile::TestErr()
{
	if (HandleError != 0) Err(700 + HandleError);
}

int XWFile::UsedFileSize()
{
	return int(MaxPage + 1) << XPageShft;
}

bool XWFile::NotCached()
{
	return (this != &XWork) && CFile->FF->NotCached();
}


/**
 * \brief Returns working or regular index file
 * \return Pointer to working or regular index file
 */
XWFile* XKey::GetXFile()
{
	if (InWork) return &XWork;
	return CFile->FF->XF;
}

void XWFile::RdPage(XPage* P, int pageNr)
{
	P->Clean();
	if ((pageNr == 0) || (pageNr > MaxPage)) Err(831);
	// puvodne se nacitalo celych XPageSize z P, bylo nutno to rozhodit na jednotlive tridni promenne
	RdWrCache(READ, Handle, NotCached(), pageNr << XPageShft, 1, &P->IsLeaf);
	RdWrCache(READ, Handle, NotCached(), (pageNr << XPageShft) + 1, 4, &P->GreaterPage);
	RdWrCache(READ, Handle, NotCached(), (pageNr << XPageShft) + 5, 2, &P->NItems);
	RdWrCache(READ, Handle, NotCached(), (pageNr << XPageShft) + 7, XPageSize - 7, P->A);
	P->Deserialize();
}

void XWFile::WrPage(XPage* P, int pageNr, bool serialize)
{
	if (serialize) {
		P->Serialize();
	}
	if (UpdLockCnt > 0) Err(645);
	// puvodne se zapisovalo celych XPageSize z P, bylo nutno to rozhodit na jednotlive tridni promenne
	RdWrCache(WRITE, Handle, NotCached(), pageNr << XPageShft, 1, &P->IsLeaf);
	RdWrCache(WRITE, Handle, NotCached(), (pageNr << XPageShft) + 1, 4, &P->GreaterPage);
	RdWrCache(WRITE, Handle, NotCached(), (pageNr << XPageShft) + 5, 2, &P->NItems);
	RdWrCache(WRITE, Handle, NotCached(), (pageNr << XPageShft) + 7, XPageSize - 7, P->A);
}

void XWFile::WrPage(XXPage* p, int pageNr)
{
	if (UpdLockCnt > 0) Err(645);
	RdWrCache(WRITE, Handle, NotCached(), pageNr << XPageShft, 1, &p->IsLeaf);
	RdWrCache(WRITE, Handle, NotCached(), (pageNr << XPageShft) + 1, 4, &p->GreaterPage);
	RdWrCache(WRITE, Handle, NotCached(), (pageNr << XPageShft) + 5, 2, &p->NItems);
	RdWrCache(WRITE, Handle, NotCached(), (pageNr << XPageShft) + 7, XPageSize - 7, p->A);
}

int XWFile::NewPage(XPage* P)
{
	int result = 0;
	if (FreeRoot != 0) {
		result = FreeRoot;
		RdPage(P, FreeRoot);
		FreeRoot = P->GreaterPage;
	}
	else {
		MaxPage++;
		if (MaxPage > 0x1fffff) Err(887);
		result = MaxPage;
	}
	P->Clean();
	return result;
}

void XWFile::ReleasePage(XPage* P, int N)
{
	P->Clean();
	P->GreaterPage = FreeRoot;
	FreeRoot = N;
	WrPage(P, N);

	//P->IsLeaf = false;
	//P->NItems = 0;
	////FillChar(P, XPageSize, 0);
	//memset(P->A, 0, sizeof(P->A));
	//P->GreaterPage = FreeRoot;
	//FreeRoot = N;
	//WrPage(P, N);
}
