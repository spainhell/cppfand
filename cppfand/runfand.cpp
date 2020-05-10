#include "runfand.h"
#include "base.h"
#include "legacy.h"
#include "pstring.h"
#include "drivers.h"
#include "access.h"
#include "editor.h"
#include "globconf.h"
#include "oaccess.h"
#include "obaseww.h"
#include "rdedit.h"
#include "rdfildcl.h"
#include "runedi.h"
#include "runfrml.h"
#include "runproj.h"
#include "wwmenu.h"
#include "wwmix.h"


void ScrGraphMode(bool Redraw, WORD OldScrSeg)
{
	// graf. mód není podporován

	//void* p; void* p1;
	//WORD* pofs = (WORD*)p;
	//WORD sz, cr, i; integer err;
	//pstring s(4);
	//if ((VideoCard == viCga) || (TxtCols != 80) || (TxtRows != 25)) RunError(643);
	//DoneMouseEvents();
	//bool b = Crs.Enabled;
	//CrsHide();
	//integer n = 80 * 25 * 2;
	//if (OldScrSeg != 0) p = ptr(OldScrSeg, 0);
	//else { AlignParagraph(); p = GetStore(n); }
	//Move(ptr(ScrSeg, 0), p, n);
	//if (OldScrSeg != 0) SetGraphMode(GraphMode);
	//else {
	//	if ((VideoCard == viVga) && (GraphDriver != EGAMono)) {
	//		if (BGIReload) {
	//			if (fonts.VFont == foLatin2) ResFile.Get(Vga8x19L, FontArr);
	//			else ResFile.Get(Vga8x19K, FontArr);
	//		}
	//		GrBytesPerChar = 19;
	//	}
	//	else {
	//		if (BGIReload) {
	//			if (fonts.VFont == foLatin2) ResFile.Get(Ega8x14L, FontArr);
	//			else ResFile.Get(Ega8x14K, FontArr);
	//		}
	//		GrBytesPerChar = 14;
	//	}
	//	GrBytesPerLine = 80 * GrBytesPerChar;
	//	if (VideoCard == viHercules) n = BgiHerc;
	//	else n = BgiEgaVga;
	//	if (BGIReload) ResFile.Get(n, BGIDriver);
	//	if (RegisterBGIDriver(BGIDriver) < 0) RunError(838);
	//	GetMem(p1, 7000); FreeMem(p1, 7000); FreeList = HeapPtr;
	//	InitGraph(GraphDriver, GraphMode, ""); FreeList = nullptr;
	//	err = GraphResult;
	//	if (err != grOK) { str(err,3,s); SetMsgPar(s); RunError(201); }
	//}
	//IsGraphMode = true; ScrSeg = seg(*p); CrsIntrInit();
	//if (Redraw) ScrPopToGraph(0, 0, TxtCols, TxtRows, p, TxtCols);
	//InitMouseEvents(); if (b) CrsShow();
	//if (BGIReload) {
	//	if (fonts.VFont == foLatin2) {
	//		ResFile.Get(ChrLittLat, BGILittFont); ResFile.Get(ChrTripLat, BGITripFont);
	//	}
	//	else {
	//		ResFile.Get(ChrLittKam, BGILittFont); ResFile.Get(ChrTripKam, BGITripFont);
	//	}
	//}
	//if (RegisterBGIFont(BGILittFont) < 0) RunError(838);
	//if (RegisterBGIFont(BGITripFont) < 0) RunError(838);
}

WORD ScrTextMode(bool Redraw, bool Switch)
{
	//bool b;
	//if (!IsGraphMode) return;
	//DoneMouseEvents();
	//b = Crs.Enabled; CrsHide(); CrsIntrDone();
	//if (Switch) RestoreCrtMode; else CloseGraph();
	//IsGraphMode = false;
	//LoadVideoFont();
	//if (Redraw) Move(ptr(ScrSeg, 0), ptr(video.address, 0), 80 * 25 * 2);
	//auto result = ScrSeg;
	//ScrSeg = video.address;
	//InitMouseEvents();
	//CrsShow(); /*is visible*/
	//if (!b) CrsHide();
	//return result;

	return 0;
}

bool IsAT()
{
	// snad to zjišuje, jestli je to PC-XT nebo PC-AT :-)
	return true;
}

void OpenXMS()
{
	// nebudeme otevírat XMS pamì, nejsme v DOSu
}

void OpenCache()
{
	// pracuje s XMS -> ignorujeme
}

void DetectVideoCard()
{
	// ignorujeme
}

void InitDrivers()
{
	char kl;
	BreakIntrInit();
	//DetectGraph(GraphDriver, GraphMode);
	/* GraphDriver = EGAMono; Mark****/
	//DetectVideoCard();
	//AssignCrt(Output); Rewrite(Output);
	ClrEvent();
}

void InitAccess()
{
	ResetCompilePars(); SpecFDNameAllowed = false;
	FillChar(&XWork, sizeof(XWork), 0);
}

void RdColors(FILE* CfgHandle)
{
	WORD typ;
	if (StartMode == 7) typ = 1;
	else if (VideoCard >= enVideoCard::viEga) typ = 3;
	else typ = 2;
	SeekH(CfgHandle, PosH(CfgHandle) + (sizeof(video) + sizeof(colors)) * (typ - 1));
	ReadH(CfgHandle, sizeof(video), &video);
	ReadH(CfgHandle, sizeof(colors), &colors);
	SeekH(CfgHandle, PosH(CfgHandle) + (sizeof(video) + sizeof(colors)) * (3 - typ));
}

void RdPrinter(FILE* CfgHandle)
{
	const BYTE NPrintStrg = 32;
	BYTE l;
	WORD i, j, n;
	BYTE* p = nullptr;
	WORD* off = (WORD*)p;
	BYTE A[NPrintStrg * 256];
	ReadH(CfgHandle, 1, &prMax);
	for (j = 1; j < prMax; j++) {
		p = (BYTE*)(&A); n = 0;
		for (i = 0; i < NPrintStrg; i++) {
			ReadH(CfgHandle, 1, &l); if (l == 0xFF) goto label1; *p = l; off++;
			ReadH(CfgHandle, l, p); off += l; n += (l + 1);
		}
		ReadH(CfgHandle, 1, &l); if (l != 0xFF) {
		label1:
			printf("Invalid FAND.CFG\n"); wait(); Halt(-1);
		}
		/* !!! with printer[j-1] do!!! */ {
			auto ap = printer[j - 1];
			GetMem(ap.Strg, n); Move(A, ap.Strg, n);
			ReadH(CfgHandle, 4, &ap.Typ);
			ap.OpCls = false; ap.ToHandle = false; ap.ToMgr = false;
			if (ap.TmOut == 255) { ap.OpCls = true; ap.TmOut = 0; }
			else if (ap.TmOut == 254) { ap.ToHandle = true; ap.TmOut = 0; }
			else if (ap.TmOut == 253) { ap.ToMgr = true; ap.TmOut = 0; }
		}
	}
	SetCurrPrinter(0);
}

void RdWDaysTab(FILE* CfgHandle)
{
	ReadH(CfgHandle, sizeof(globconf::NWDaysTab), &globconf::NWDaysTab);
	ReadH(CfgHandle, sizeof(globconf::WDaysFirst), &globconf::WDaysFirst);
	ReadH(CfgHandle, sizeof(globconf::WDaysLast), &globconf::WDaysLast);
	//GetMem(WDaysTab, NWDaysTab * 3);
	globconf::WDaysTab = new wdaystt[3];
	ReadH(CfgHandle, globconf::NWDaysTab * 3, globconf::WDaysTab);
}

void RdCFG()
{
	FILE* CfgHandle;
	char ver[5] = { 0,0,0,0,0 };
	globconf::CVol = ""; globconf::CPath = MyFExpand("FAND.CFG", "FANDCFG");
	CfgHandle = OpenH(_isoldfile, RdOnly);
	if (globconf::HandleError != 0) { printf("%s !found", globconf::CPath.c_str()); wait(); Halt(-1); }
	ReadH(CfgHandle, 4, ver);
	if (strcmp(ver, CfgVersion) != 0) {
		printf("Invalid version of FAND.CFG"); wait(); Halt(-1);
	}
	ReadH(CfgHandle, sizeof(spec), &spec);
	RdColors(CfgHandle);
	ReadH(CfgHandle, sizeof(fonts), &fonts);
	ReadH(CfgHandle, sizeof(CharOrdTab), CharOrdTab);
	ReadH(CfgHandle, sizeof(UpcCharTab), UpcCharTab);
	RdPrinter(CfgHandle);
	RdWDaysTab(CfgHandle);
	CloseH(CfgHandle);
}

void CompileHelpCatDcl()
{
	pstring s; void* p2 = nullptr;
	FileDRoot = nullptr; CRdb = nullptr; MarkStore2(p2);
	RdMsg(56); s = globconf::MsgLine; SetInpStr(s);
#ifdef FandRunV
	RdFileD("UFANDHLP", '6', "");
#else
	RdFileD("FANDHLP", '6', "");
#endif

	HelpFD = *CFile;
	RdMsg(52); s = globconf::MsgLine; SetInpStr(s); RdFileD("Catalog", 'C', "");
	CatFD = CFile; FileDRoot = nullptr;
	CatRdbName = CatFD->FldD; CatFileName = CatRdbName->Chain;
	CatArchiv = CatFileName->Chain;
	CatPathName = CatArchiv->Chain; CatVolume = CatPathName->Chain;
	MarkStore(AfterCatFD); ReleaseStore2(p2);
}

bool SetTopDir(pstring& p, pstring& n)
{
	pstring e; ExitRecord er;
	auto result = false; FSplit(FExpand(p), TopRdbDir, n, e);
	if (!IsIdentifStr(n)) { WrLLF10Msg(881); return result; }
	EditDRoot = nullptr; LinkDRoot = nullptr; FuncDRoot = nullptr;
	TopDataDir = GetEnv("FANDDATA");
	DelBackSlash(TopRdbDir); DelBackSlash(TopDataDir);
	if (TopDataDir != "") TopDataDir = FExpand(TopDataDir);
	ChDir(TopRdbDir);
	if (IOResult != 0) { SetMsgPar(p); WrLLF10Msg(703); return result; }
	CatFDName = n; //NewExit(Ovr(), er);
	goto label1;
	CFile = CatFD; OpenF(Exclusive); result = true;
label1:
	RestoreExit(er);
	return result;
}

void RunRdb(pstring p)
{
	pstring n;
	if ((p != "") && SetTopDir(p, n))
	{
		pstring main = "main";
		EditExecRdb(&n, &main, nullptr);
		CFile = CatFD;
		CloseFile();
	}
}

void SelectRunRdb(bool OnFace)
{
	wwmix ww;
	pstring p;
	p = ww.SelectDiskFile(".RDB", 34, OnFace); RunRdb(p);
}

void CallInstallRdb()
{
	wwmix ww;
	pstring p; pstring n;
	p = ww.SelectDiskFile(".RDB", 35, true);
	if ((p != "") && SetTopDir(p, n))
	{
		InstallRdb(n); CFile = CatFD; CloseFile();
	}
}

void CallEditTxt()
{
	globconf::CPath = FExpand(globconf::CPath); globconf::CVol = "";
	pstring errmsg = "";
	EditTxtFile(nullptr, 'T', errmsg, nullptr, 1, 0, nullptr, 0, "", 0, nullptr);
}

void SelectEditTxt(pstring E, bool OnFace)
{
	wwmix ww;
	globconf::CPath = ww.SelectDiskFile(E, 35, OnFace); if (globconf::CPath == "") return;
	CallEditTxt();
}

void InitRunFand()
{
	WORD n = 0, l = 0, err = 0, hourmin = 0;
	FILE* h = nullptr;
	pstring s;
	BYTE nb, sec = 0;
	ExitRecord* er = new ExitRecord();
	integer i, j, MsgNr;
	TMenuBoxS* mb = nullptr;
	longint w = 0;
	void* p = nullptr;
	pstring* x = nullptr;
	WORD* xofs = (WORD*)x;
	pstring txt(16);
	double r;

	InitDrivers();
	//WasInitDrivers = true;
	InitAccess();

	NonameStartFunction();
#ifdef FandDML
	InitDML();
#endif

	//NewExit(Ovr(), *er);
	// StackLimit += 256;
	OldPrTimeOut = PrTimeOut;
	//CallOpenFandFiles = OpenFandFiles;  // TODO: CallOpenFandFiles: procedure(FromDML:boolean);
	//CallCloseFandFiles = CloseFandFiles;  // TODO: CallCloseFandFiles: procedure(FromDML:boolean);
	video.CursOn = 0x0607; // {if exit before reading.CFG}
	KbdBuffer[0] = 0x0;
	globconf::F10SpecKey = 0;
	if (!GetEnv("DMLADDR").empty()) {
		printf("type 'exit' to return to FAND");
		wait();
		return; // pùvodnì wait;
	}
	globconf::WrkDir = GetEnv("FANDWORK");
	if (globconf::WrkDir == "") globconf::WrkDir = globconf::FandDir;
	AddBackSlash(globconf::WrkDir);
	s = globconf::WrkDir + "FANDWORK";
	globconf::FandWorkName = s + ".$$$";
	globconf::FandWorkXName = s + ".X$$";
	globconf::FandWorkTName = s + ".T$$";
	LANNode = 0;
	s = GetEnv("LANNODE");
	s = TrailChar(' ', s);
	if (!s.empty()) {
		val(s, nb, err);
#ifndef FandRunV
		if (nb <= 3) {
#endif 
			if (err == 0) LANNode = nb;
#ifndef FandRunV
		}
#endif 
	}
	h = globconf::ResFile.Handle;
	ReadH(h, 2, &n);
	if (n != ResVersion) {
		printf("FAND.RES incorr. version");
		wait(); Halt(0);
	}

	//OvrHandle = GetOverHandle(h, -1); // TODO: pùvodnì to byl WORD - 1, teï je to blbost;
	//ReadH(h, sizeof(globconf::ResFile.A), globconf::ResFile.A);

	for(int readindexes = 0; readindexes < FandFace; readindexes++)
	{
		ReadH(h, sizeof(globconf::ResFile.A->Pos), &globconf::ResFile.A[readindexes].Pos);
		ReadH(h, sizeof(globconf::ResFile.A->Size), &globconf::ResFile.A[readindexes].Size);
	}
	
	//ReadH(h, 4, &globconf::ResFile.A);
	ReadH(h, 2, &globconf::MsgIdxN);

	l = /*sizeof(TMsgIdxItem)*/ 5 * globconf::MsgIdxN;
	globconf::MsgIdx = new TMsgIdxItem[l]; // GetMem(MsgIdx, l);

	for (int readindexes = 0; readindexes < l; readindexes++)
	{
		ReadH(h, sizeof(globconf::MsgIdx->Nr), &globconf::MsgIdx[readindexes].Nr);
		ReadH(h, sizeof(globconf::MsgIdx->Ofs), &globconf::MsgIdx[readindexes].Ofs);
		ReadH(h, sizeof(globconf::MsgIdx->Count), &globconf::MsgIdx[readindexes].Count);
	}
	
	ReadH(h, l, globconf::MsgIdx);
	globconf::FrstMsgPos = PosH(h);
	RdMsg(50);
	Move((void*)&globconf::MsgLine[1], (void*)&globconf::AbbrYes, 2);
	RdCFG();
	globconf::ProcAttr = colors.uNorm;
	// ScrSeg = video.address; TODO: nepotøebujeme, nezapisujeme pøímo do GK
	if (video.TxtRows != 0) TxtRows = video.TxtRows;

	/* FONTY ASI NENÍ POTØEBA NIKDE NAHRÁVAT
	 *if (Fonts.LoadVideoAllowed && (VideoCard >= viEga))
		switch (BytesPerChar) {
		case 14: {if (Fonts.VFont == foKamen) NrVFont = Ega8x14K; else NrVFont = Ega8x14L; break; }
		case 16: {if (Fonts.VFont == foKamen) NrVFont = Vga8x16K; else NrVFont = Vga8x16L; break; }
		}*/

	// Access
	// GetIntVec(0x3f, FandInt3f); // toto je vektor pøerušení INT 3fH - Overlay a DLL
	FillChar(&XWork, sizeof(XWork), 0); // celý objekt nulovat nemusíme, snad ...
	FillChar(&TWork, sizeof(TWork), 0); //  -"-
	CRdb = nullptr;
	for (i = 0; i < FloppyDrives; i++) { MountedVol[i] = ""; }
	// Ww
	ss.Empty = true;
	ss.Pointto = nullptr;
	TxtEdCtrlUBrk = false;
	TxtEdCtrlF4Brk = false;
	InitMouseEvents();
	// Editor
	InitTxtEditor();
	//
	OpenCache();

	WasInitPgm = true;
	CompileHelpCatDcl();

	// zøejmì není dùvod zavádìt fonty do graf. karty
	//LoadVideoFont();
	//if ((VideoCard == viHercules) && Fonts.LoadVideoAllowed) {
	//	ScrGraphMode(false, 0);
	//	BGIReload = false;
	//}

	OpenWorkH();
	OpenFANDFiles(false);

	if (!globconf::paramstr.at(1).empty() && globconf::paramstr.at(1) != "?") {
		{
#ifndef FandRunV
			if (SEquUpcase(paramstr(2), 'D')) {
				IsTestRun = true;
				goto label0;
			}
			else
#endif
				if (SEquUpcase(globconf::paramstr.at(2), "T")) {
					globconf::CPath = globconf::paramstr.at(1);
					if (copy(globconf::CPath, 1, 2) == "*.")
						SelectEditTxt(copy(globconf::CPath, 2, 4), false);
					else CallEditTxt();
					return;
				}
				else {
				label0:
					if (copy(globconf::paramstr.at(1), 1, 2) == "*.") SelectRunRdb(false);
					else RunRdb(globconf::paramstr.at(1));
					if (IsTestRun) IsTestRun = false;
					else return;
				}
		}

		TextAttr = colors.DesktopColor;
		Window(1, 1, (BYTE)TxtCols, TxtRows - 1);
		WriteWFrame(WHasFrame + WDoubleFrame, "", "");
		ScrClr(1, 1, TxtCols - 2, TxtRows - 13, (char)0xb1, TextAttr);
		ScrClr(1, TxtRows - 12, TxtCols - 2, 10, (char)0xb2, TextAttr);
		globconf::ResFile.Get(FandFace, p);
		x = (pstring*)p;
		xofs++;
		for (int i = -11; i < -6; i++) {
			x[0] = char(TxtCols - 2);
			ScrWrStr(1, TxtRows + i, *x, TextAttr);
			xofs += 82;
		}
		TextAttr = colors.mHili;
		ScrClr(3, TxtRows - 4, TxtCols - 6, 1, ' ', TextAttr);

#ifdef Trial
		RdMsg(70);
#elif FandRunV
		RdMsg(42);
#elif FandDemo
		RdMsg(43);
#else
		RdMsg(41);
#endif
		txt = "";
#ifdef FandNetV
		txt = "LAN,";
#endif
#ifdef FandSQL
		txt = "SQL,";
#endif
#ifndef FandGraph
		txt += "~GRAPH,";
#endif
#ifndef FandProlog
		txt += "~PRL,";
#endif
#ifndef FandDML
		txt += "~DML,";
#endif
#ifdef Coproc
		txt = txt + "COPROC,";
#endif
#ifdef FandTest
		txt = "test," + txt;
#endif

#ifdef FandAng
		txt = txt + "En ";
#endif

		if (!txt.empty()) {
			txt += ")";
			globconf::MsgLine = globconf::MsgLine + "x (" + txt;
		}
		else globconf::MsgLine += 'x';

		GotoXY(5, TxtRows - 3); printf(globconf::MsgLine.c_str());


#ifdef FandRunV 
#ifndef FandNetV
		goto label2;
#endif
#endif

#ifndef FandDemo
		if (TxtCols >= 80) {
			RdMsg(40);
			GotoXY(51, TxtRows - 3);
			//printf(MsgLine, UserLicNrShow:7);
		}
#endif

	label2:
		ReleaseStore(p);
		MsgNr = 2;

#ifdef FandRunV
		MsgNr = 14;
#endif

#ifdef FandDemo
		if (Today() > 731215.0) WrLLF10Msg(47);
#endif

#ifdef FandTest
		// if (today > 730210.0) { WrLLF10Msg(47); /*exit;*/ }
#endif

		RdMsg(MsgNr);
		mb = new TMenuBoxS(4, 3, &globconf::MsgLine);
		i = 1;
	label1:
		i = mb->Exec(i);
		j = i;
#ifdef FandRunV
		if (j != 0) j++;
#endif
		w = PushW(1, 1, TxtCols, TxtRows);

		switch (j) {
		case 1: { IsTestRun = true; SelectRunRdb(true); IsTestRun = false; break; }
		case 2: { SelectRunRdb(true); IsTestRun = false; break; }
		case 3: { IsInstallRun = true; CallInstallRdb(); IsInstallRun = false; break; }
		case 4: SelectEditTxt(".TXT", true); break;
		case 5: OSshell("", "", false, true, true, true); break;
		case 0:
		case 6: { CloseH(WorkHandle); CloseFANDFiles(false); return; break; }
		default:;
		}
	}
	PopW(w);
	goto label1;
}
