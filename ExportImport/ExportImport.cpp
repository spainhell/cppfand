#include "ExportImport.h"

#include "ThFile.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"
#include "../FileSystem/directory.h"
#include "../cppfand/compile.h"
#include "../MergeReport/rdmerg.h"
#include "../MergeReport/runmerg.h"


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

//void ImportTxt(CopyD* CD)
//{
//	ThFile* F1;
//	ExitRecord er;
//	LockMode md;
//	FrmlElem4* FE = new FrmlElem4(_const, 0);
//#ifdef FandSQL
//	SQLStreamPtr q;
//#endif
//label1:
//	//NewExit(Ovr, er);
//	//goto label1;
//	F1 = nullptr;
//#ifdef FandSQL
//	q = nullptr;
//#endif
//	//with CD^ do begin
//	F1 = new ThFile(CD->Path1, CD->CatIRec1, _inp, 0, nullptr);
//	if (CD->HdFD != nullptr) {
//		FE->Op = _const;
//		FE->S = F1->RdDelim(0x1A); //^Z
//		AsgnParFldFrml(CD->HdFD, CD->HdF, FE, false);
//	}
//	CFile = CD->FD2;
//	CRecPtr = GetRecSpace();
//#ifdef FandSQL
//	if (CFile->IsSQLFile) {
//		New(q, Init);
//		q->OutpRewrite(Append);
//	}
//	else
//#endif
//		md = RewriteF(CD->Append);
//	while (!(F1->eof) && (F1->ForwChar != 0x1A)) {
//		ZeroAllFlds();
//		ClearDeletedFlag();
//		VarFixImp(F1, CD->Opt1);
//		F1->ForwChar; //{set IsEOF at End}
//#ifdef FandSQL
//		if (CFile->IsSQLFile) q->PutRec();
//		else
//#endif
//		{
//			PutRec(CFile, CRecPtr);
//			if (CD->Append && (CFile->Typ == 'X')) TryInsertAllIndexes(CFile->IRec);
//		}
//	}
//	LastExitCode = 0;
//label1:
//	RestoreExit(er);
//#ifdef FandSQL
//	if (q != nullptr) {
//		q->OutpClose();
//		ClearRecSpace(CRecPtr);
//	};
//#endif
//	if ((F1!=nullptr) && (F1->Handle != nullptr)) {
//		F1->Done();
//		OldLMode(md);
//	}
//}


void MakeCopy(CopyD* CD)
{
	try {
		ThFile F1 = ThFile(CD->Path1, CD->CatIRec1, _inp, 0, nullptr);

		ThFile F2 = ThFile(CD->Path2, CD->CatIRec2, CD->Append ? InOutMode::_append : InOutMode::_outp, 0, &F1);
		if (HandleError != 0) {
			//delete F1;
			LastExitCode = 1;
			return;
		}

		while (!F1.eof) {
			memcpy(F2.Buf, F1.Buf, F1.lBuf);
			F2.lBuf = F1.lBuf;
			switch (CD->Mode) {
			case 1: {
				ConvKamenLatin(F2.Buf, F2.lBuf, true);
				break;
			}
			case 2: {
				ConvKamenLatin(F2.Buf, F2.lBuf, false);
				break;
			}
			case 3: {
				ConvToNoDiakr(F2.Buf, F2.lBuf, TVideoFont::foKamen);
				break;
			}
			case 4: {
				ConvToNoDiakr(F2.Buf, F2.lBuf, TVideoFont::foLatin2);
				break;
			}
			case 5: {
				std::string pKod = ResFile.Get(LatToWinCp);
				ConvWinCp((unsigned char*)F2.Buf, (unsigned char*)pKod.c_str(), F2.lBuf);
				break;
			}
			case 6: {
				std::string pKod = ResFile.Get(KamToWinCp);
				ConvWinCp((unsigned char*)F2.Buf, (unsigned char*)pKod.c_str(), F2.lBuf);
				break;
			}
			case 7: {
				std::string pKod = ResFile.Get(WinCpToLat);
				ConvWinCp((unsigned char*)F2.Buf, (unsigned char*)pKod.c_str(), F2.lBuf);
				break;
			}
			default: break;
			}
			F2.WriteBuf(false);
			F1.ReadBuf();
		}
		LastExitCode = 0;
	}

	catch (std::exception& e) {
		LastExitCode = 1;
	}
}

void FileCopy(CopyD* CD)
{
	void* p = nullptr, * p2;
	LastExitCode = 2;
	if (CD->Opt1 == CpOption::cpFix || CD->Opt1 == CpOption::cpVar) {
		//ImportTxt();
		screen.ScrWrText(1, 1, "ImportTxt()");
	}
	else if (CD->Opt2 == CpOption::cpFix || CD->Opt2 == CpOption::cpVar) {
		//ExportTxt();
		screen.ScrWrText(1, 1, "ExportTxt()");
	}
	else if (CD->FD1 != nullptr) {
		if (CD->FD2 != nullptr) {
			MakeMerge(CD);
		}
		else {
			//ExportFD();
			screen.ScrWrText(1, 1, "ExportFD()");
		}
	}
	else if (CD->Opt1 == CpOption::cpTxt) {
		//TxtCtrlJ();
		screen.ScrWrText(1, 1, "TxtCtrlJ()");
	}
	else {
		MakeCopy(CD);
	}
	SaveFiles();
	RunMsgOff();
	if (LastExitCode != 0 && !CD->NoCancel) GoExit();
}

void MakeMerge(CopyD* CD)
{
	try
	{
		std::string s = "#I1_" + CD->FD1->Name;
		if (CD->ViewKey != nullptr) {
			std::string ali = CD->ViewKey->Alias;
			if (ali.empty()) ali = '@';
			s = s + '/' + ali;
		}
		s = s + " #O1_" + CD->FD2->Name;
		if (CD->Append) s = s + '+';

		SetInpStr(s);
		ReadMerge();
		RunMerge();
		LastExitCode = 0;
	}

	catch (std::exception& e)
	{
	}
}


void CheckFile(FileD* FD)
{
	struct stPrfx { longint NRecs; WORD RecLen; } Prfx;
	FILE* h = nullptr;
	pstring d; pstring n; pstring e;
	longint fs = 0;

	TestMountVol(CPath[0]);

	const int fileStatus = fileExists(CPath);
	if (fileStatus != 0) {
		// path not exists
		if (fileStatus == 1) LastExitCode = 1;
		else LastExitCode = 2;
		return;
	}

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
