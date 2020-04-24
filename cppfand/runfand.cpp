#include "runfand.h"
#include "base.h"
#include "common.h"
#include "legacy.h"
#include "pstring.h"
#include "drivers.h"
#include "kbdww.h"
#include "access.h"
#include "memory.h"
#include "oaccess.h"
#include "handle.h"
#include "keybd.h"
#include "obaseww.h"
#include "runedi.h"
#include "runfrml.h"
#include "wwmenu.h"
#include "wwmix.h"


void runfand::InitRunFand()
{
	Drivers driver = Drivers(); // HLAVNI OVLADAC

	WORD n = 0, l = 0, err = 0, hourmin = 0;
	FILE* h = nullptr;
	pstring s;
	BYTE nb, sec = 0;
	ExitRecord* er = nullptr;
	integer i, j, MsgNr;
	// TODO: PMenuBoxS mb;
	longint w = 0;
	void* p = nullptr;
	pstring* x;
	unsigned int xofs = 0; // x:StringPtr; xofs:word absolute x;
	pstring txt;
	double r;

	InitDrivers();
	//WasInitDrivers = true;
	InitAccess();
#ifdef FandDML
	InitDML();
#endif

	NewExit(Ovr(), *er);
	// StackLimit += 256;
	OldPrTimeOut = PrTimeOut;
	//CallOpenFandFiles = OpenFandFiles;  // TODO: CallOpenFandFiles: procedure(FromDML:boolean);
	//CallCloseFandFiles = CloseFandFiles;  // TODO: CallCloseFandFiles: procedure(FromDML:boolean);
	video.CursOn = 0x0607; // {if exit before reading.CFG}
	KbdBuffer[0] = 0x0;
	F10SpecKey = 0;
	if (!GetEnv("DMLADDR").empty()) {
		printf("type 'exit' to return to FAND");
		wait();
		return; // pùvodnì wait;
	}
	WrkDir = GetEnv("FANDWORK");
	if (WrkDir == "") WrkDir = FandDir;
	AddBackSlash(WrkDir);
	s = WrkDir + "FANDWORK";
	FandWorkName = s + ".$$$";
	FandWorkXName = s + ".X$$";
	FandWorkTName = s + ".T$$";
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

		h = ResFile.Handle;
		ReadH(h, 2, reinterpret_cast<void*>(n));
		if (n != ResVersion) {
			printf("FAND.RES incorr. version");
			wait(); Halt(0);
		}

		OvrHandle = GetOverHandle(h, - 1); // TODO: pùvodnì to byl WORD - 1, teï je to blbost;
		ReadH(h, sizeof(ResFile.A), ResFile.A);
		ReadH(h, 2, reinterpret_cast<void*>(MsgIdxN));
		l = sizeof(TMsgIdxItem) * MsgIdxN;
		MsgIdx = new TMsgIdxItem[1]; // GetMem(MsgIdx, l);
		ReadH(h, l, MsgIdx);
		FrstMsgPos = PosH(h);
		RdMsg(50);
		Move((void*)MsgLine[1], (void*)&AbbrYes, 2);
		RdCFG();
		ProcAttr = colors.uNorm;
		// ScrSeg = video.address; TODO: nepotøebujeme, nezapisujeme pøímo do GK
		if (video.TxtRows != 0) TxtRows = video.TxtRows;

		/* FONTY ASI NENÍ POTØEBA NIKDE NAHRÁVAT
		 *if (Fonts.LoadVideoAllowed && (VideoCard >= viEga))
			switch (BytesPerChar) {
			case 14: {if (Fonts.VFont == foKamen) NrVFont = Ega8x14K; else NrVFont = Ega8x14L; break; }
			case 16: {if (Fonts.VFont == foKamen) NrVFont = Vga8x16K; else NrVFont = Vga8x16L; break; }
			}*/
	}

	// Access
	// GetIntVec(0x3f, FandInt3f); // toto je vektor pøerušení INT 3fH - Overlay a DLL
	// FillChar(XWork, sizeof(XWork), 0); // celý objekt nulovat nemusíme, snad ...
	// FillChar(TWork, sizeof(TWork), 0); //  -"-
	CRdb = nullptr;
	for (int i = 0; i < FloppyDrives; i++) { MountedVol[i] = ""; }
	// Ww
	wwmix::ss.Empty = true;
	wwmix::ss.Pointto = nullptr;
	TxtEdCtrlUBrk = false;
	TxtEdCtrlF4Brk = false;
	InitMouseEvents();
	// Editor
	// TODO: InitTxtEditor();
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

	if (!paramstr.at(1).empty() && paramstr.at(1) != "?") {
		{
#ifndef FandRunV
			if (SEquUpcase(paramstr(2), 'D')) {
				IsTestRun = true;
				goto label0;
			}
			else
#endif
				if (SEquUpcase(paramstr.at(2), "T")) {
					CPath = paramstr.at(1);
					if (copy(CPath, 1, 2) == "*.")
						SelectEditTxt(copy(CPath, 2, 4), false);
					else CallEditTxt();
					return;
				}
				else {
				label0:
					if (copy(paramstr.at(1), 1, 2) == "*.") SelectRunRdb(false);
					else RunRdb(paramstr.at(1));
					if (IsTestRun) IsTestRun = false;
					else return;
				}
		}

		TextAttr = colors.DesktopColor;
		Drivers::Window(1, 1, (BYTE)TxtCols, TxtRows - 1);
		WriteWFrame(WHasFrame + WDoubleFrame, "", "");
		Drivers::ScrClr(1, 1, TxtCols - 2, TxtRows - 13, (char)0xb1, TextAttr);
		Drivers::ScrClr(1, TxtRows - 12, TxtCols - 2, 10, (char)0xb2, TextAttr);
		ResFile.Get(FandFace, p);
		x = (pstring*)p;
		xofs++;
		for (int i = -11; i < -6; i++) {
			x[0] = char(TxtCols - 2);
			Drivers::ScrWrStr(1, TxtRows + i, *x, TextAttr);
			xofs += 82;
		}
		TextAttr = colors.mHili;
		Drivers::ScrClr(3, TxtRows - 4, TxtCols - 6, 1, ' ', TextAttr);

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
			MsgLine = MsgLine + "x (" + txt;
		}
		else MsgLine += 'x';

		Drivers::GotoXY(5, TxtRows - 3); printf(MsgLine.c_str());


#ifdef FandRunV 
#ifndef FandNetV
		goto label2;
#endif
#endif

#ifndef FandDemo
		if (TxtCols >= 80) {
			RdMsg(40);
			Drivers::GotoXY(51, TxtRows - 3);
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
		// TODO: mb = new TMenuBoxS(4, 3, &MsgLine);
		i = 1;
	label1:
		// TODO: i = mb->Exec(i);
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
