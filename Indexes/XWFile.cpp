#include "XWFile.h"

#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/obaseww.h"
#include "../cppfand/XFile.h"

void XWFile::Err(WORD N)
{
	if (this == &XWork) {
		SetMsgPar(FandWorkXName);
		RunError(N);
	}
	else {
		CFile->XF->SetNotValid();
		CFileMsg(N, 'X');
		CloseGoExit();
	}
}

void XWFile::TestErr()
{
	if (HandleError != 0) Err(700 + HandleError);
}

longint XWFile::UsedFileSize()
{
	return longint(MaxPage + 1) << XPageShft;
}

bool XWFile::NotCached()
{
	return (this != &XWork) && CFile->NotCached();
}

XWFile* XKey::GetXFile()
{
	if (InWork) return &XWork;
	return CFile->XF;
}

void XWFile::RdPage(XPage* P, longint N)
{
	P->Clean();
	if ((N == 0) || (N > MaxPage)) Err(831);
	// puvodne se nacitalo celych XPageSize z P, bylo nutno to rozhodit na jednotlive tridni promenne
	RdWrCache(true, Handle, NotCached(), N << XPageShft, 1, &P->IsLeaf);
	RdWrCache(true, Handle, NotCached(), (N << XPageShft) + 1, 4, &P->GreaterPage);
	RdWrCache(true, Handle, NotCached(), (N << XPageShft) + 5, 2, &P->NItems);
	RdWrCache(true, Handle, NotCached(), (N << XPageShft) + 7, XPageSize - 7, P->A);
	P->Deserialize();
}

void XWFile::WrPage(XPage* P, longint N, bool serialize)
{
	if (serialize) {
		P->Serialize();
	}
	if (UpdLockCnt > 0) Err(645);
	// puvodne se zapisovalo celych XPageSize z P, bylo nutno to rozhodit na jednotlive tridni promenne
	RdWrCache(false, Handle, NotCached(), N << XPageShft, 1, &P->IsLeaf);
	RdWrCache(false, Handle, NotCached(), (N << XPageShft) + 1, 4, &P->GreaterPage);
	RdWrCache(false, Handle, NotCached(), (N << XPageShft) + 5, 2, &P->NItems);
	RdWrCache(false, Handle, NotCached(), (N << XPageShft) + 7, XPageSize - 7, P->A);
}

longint XWFile::NewPage(XPage* P)
{
	longint result = 0;
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
	P->IsLeaf = false;
	P->GreaterPage = 0;
	P->NItems = 0;
	memset(P->A, 0, sizeof(P->A));
	return result;
}

void XWFile::ReleasePage(XPage* P, longint N)
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
