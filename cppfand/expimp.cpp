#include "expimp.h"
#include "oaccess.h"

bool OldToNewCat(longint& FilSz)
{
	struct stX { longint NRecs; WORD  RecLen; } x;
	longint off, offNew, i;
	BYTE a[91]; // budeme èíslovat od 1, jako v Pascalu (a:array[1..90] of byte;)
	/* !!! with CFile^ do!!! */
	auto result = false;
	if (CFile->Typ != 'C') return result;
	RdWrCache(true, CFile->Handle, CFile->NotCached(), 0, 6, &x);
	if (x.RecLen != 106) return result;
	x.RecLen = 107;
	RdWrCache(false, CFile->Handle, CFile->NotCached(), 0, 6, &x);
	for (i = x.NRecs; i <= 1; i--) {
		off = 6 + (i - 1) * 106;
		offNew = off + (i - 1);
		RdWrCache(true, CFile->Handle, CFile->NotCached(), off + 16, 90, a);
		RdWrCache(false, CFile->Handle, CFile->NotCached(), offNew + 17, 90, a);
		a[17] = 0;
		RdWrCache(true, CFile->Handle, CFile->NotCached(), off, 16, a);
		RdWrCache(false, CFile->Handle, CFile->NotCached(), offNew, 17, a);
	}
	CFile->NRecs = x.NRecs;
	FilSz = x.NRecs * 107 + 6;
	result = true;
	return result;
}

void CheckFile(FileD* FD)
{
	struct stPrfx { longint NRecs; WORD RecLen; } Prfx;
	FILE* h = nullptr;
	pstring d; pstring n; pstring e;
	longint fs = 0;

	TestMountVol(CPath[1]);
	h = OpenH(_isoldfile, RdShared);
	LastExitCode = 0;
	if (HandleError != 0) {
		if (HandleError == 2) LastExitCode = 1;
		else LastExitCode = 2;
		return;
	}
	ReadH(h, 4, &Prfx.NRecs);
	ReadH(h, 2, &Prfx.RecLen);
	fs = FileSizeH(h);
	CloseH(h);
	if ((FD->RecLen != Prfx.RecLen) || (Prfx.NRecs < 0) && (FD->Typ != 'X') ||
		((fs - FD->FrstDispl) / Prfx.RecLen < Prfx.NRecs) ||
		(Prfx.NRecs > 0) && (FD->Typ == 'X')) {
		LastExitCode = 3;
		return;
	}
	if (FD->TF == nullptr) return;
	FSplit(CPath, d, n, e); 
	if (SEquUpcase(e, ".RDB")) e = ".TTT"; 
	else e[2] = 'T';
	CPath = d + n + e;
	h = OpenH(_isoldfile, RdShared);
	if (HandleError == 0) CloseH(h);
	else LastExitCode = 4;
}
