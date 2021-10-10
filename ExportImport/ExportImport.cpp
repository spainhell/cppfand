#include "ExportImport.h"

#include "ThFile.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"


bool OldToNewCat(longint& FilSz)
{
	struct stX { longint NRecs; WORD  RecLen; } x;
	longint off, offNew, i;
	BYTE a[91]; // budeme èíslovat od 1, jako v Pascalu (a:array[1..90] of byte;)
	/* !!! with CFile^ do!!! */
	auto result = false;
	bool cached = CFile->NotCached();
	if (CFile->Typ != 'C') return result;
	RdWrCache(true, CFile->Handle, cached, 0, 6, &x);
	if (x.RecLen != 106) return result;
	x.RecLen = 107;
	RdWrCache(false, CFile->Handle, cached, 0, 6, &x);
	for (i = x.NRecs; i >= 1; i--) {
		off = 6 + (i - 1) * 106;
		offNew = off + (i - 1);
		RdWrCache(true, CFile->Handle, cached, off + 16, 90, a);
		RdWrCache(false, CFile->Handle, cached, offNew + 17, 90, a);
		a[17] = 0;
		RdWrCache(true, CFile->Handle, cached, off, 16, a);
		RdWrCache(false, CFile->Handle, cached, offNew, 17, a);
	}
	CFile->NRecs = x.NRecs;
	FilSz = x.NRecs * 107 + 6;
	result = true;
	return result;
}

void ConvWinCp(unsigned char* pBuf, unsigned char* pKod, WORD L)
{
	for (size_t i = 0; i < L; i++) {
		if (pBuf[i] < 0x80) continue;
		pBuf[i] = pKod[pBuf[i] - 0x80];
	}
}

void MakeCopy(CopyD* CD)
{
	std::string pKod;
	// NewExit(Ovr,er); goto 1;
	// with CD^ do begin
	ThFile* F1 = new ThFile(CD->Path1, CD->CatIRec1, _inp, 0, nullptr);
	if (HandleError != 0) {
		LastExitCode = 1;
		return;
	}

	InOutMode m = _outp;
	if (CD->Append) m = _append;

	ThFile* F2 = new ThFile(CD->Path2, CD->CatIRec2, m, 0, F1);
	if (HandleError != 0) {
		delete F1;
		LastExitCode = 1;
		return;
	}
	
	WORD kod;
	switch (CD->Mode) {
	case 5: {
		kod = LatToWinCp;
		pKod = ResFile.Get(kod);
		break;
	}
	case 6: {
		kod = KamToWinCp;
		pKod = ResFile.Get(kod);
		break;
	}
	case 7: {
		kod = WinCpToLat;
		pKod = ResFile.Get(kod);
		break;
	}
	default: ;
	}
	while (!F1->eof) {
		memcpy(F2->Buf, F1->Buf, F1->lBuf);
		F2->lBuf = F1->lBuf;
		switch (CD->Mode) {
		case 1: ConvKamenLatin(F2->Buf, F2->lBuf, true); break;
		case 2: ConvKamenLatin(F2->Buf, F2->lBuf, false); break;
		case 3: ConvToNoDiakr(F2->Buf, F2->lBuf, TVideoFont::foKamen); break;
		case 4: ConvToNoDiakr(F2->Buf, F2->lBuf, TVideoFont::foLatin2); break;
		case 5:
		case 6:
		case 7:	ConvWinCp((unsigned char*)F2->Buf, (unsigned char*)pKod.c_str(), F2->lBuf); break;
		default: ;
		}
		F2->WriteBuf(false);
		F1->ReadBuf();
	}
	LastExitCode = 0;
label1:
	// RestoreExit(er);
	if (F1 != nullptr && F1->Handle != nullptr) delete F1;
	if (F2 != nullptr && F2->Handle != nullptr) {
		if (LastExitCode != 0) F2->ClearBuf();
		delete F2;
	}
}

void CopyFile(CopyD* CD)
{
	void* p = nullptr, * p2;
	LastExitCode = 2;
	//MarkStore(p);
	if (CD->Opt1 == cpFix || CD->Opt1 == cpVar) {} //ImportTxt();
	else if (CD->Opt2 == cpFix || CD->Opt2 == cpVar) {} //ExportTxt();
	else if (CD->FD1 != nullptr) {
		if (CD->FD2 != nullptr) {} //MakeMerge();
		else {} //ExportFD();
	}
	else if (CD->Opt1 == cpTxt) {
		//TxtCtrlJ();
	}
	else {
		// MakeCopy(CD);
	}
	SaveFiles();
	RunMsgOff();
	//ReleaseStore(p);
	if (LastExitCode != 0 && !CD->NoCancel) GoExit();
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
	CloseH(&h);
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
	if (HandleError == 0) CloseH(&h);
	else LastExitCode = 4;
}
