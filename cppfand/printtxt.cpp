#include "printtxt.h"

#include "legacy.h"
#include "oaccess.h"
#include "obase.h"
#include "obaseww.h"
#include "runfrml.h"
#include "wwmenu.h"

CharArr* pBlk;
WORD iBlk, nBlk, Po;
longint charrd;
bool printBlk, outpsw;
WORD prFileNr;
pstring Ln;

pstring replaceNo(pstring s, pstring sNew)
{
	integer i;
	i = s.first('#');
	if (i > 0) {
		s.Delete(i, 1);
		s.insert(sNew.c_str(), i);
	}
	return s;
}

void ExecMgrPgm()
{
	BYTE x = 0, y = 0;
	pstring pgmNm = PrTab(prMgrProg);
	if (pgmNm == "") return;
	pstring param = replaceNo(PrTab(prMgrParam), CPath);
	Wind wmin = WindMin;
	Wind wmax = WindMax;
	TCrs crs = screen.CrsGet();
	longint w = PushW(1, 1, TxtCols, 1);
	WindMin = wmin;
	WindMax = wmax;
	screen.CrsSet(crs);
	OSshell(pgmNm, param, true, true, false, false);
	//asm mov ah, 3; mov bh, 0; push bp; int 10H; pop bp; mov x, dl; mov y, dh;
	PopW(w); screen.GotoXY(x - WindMin.X + 1, y - WindMin.Y + 1);
}

FILE* OpenMgrOutput()
{
	pstring s; FILE* h;
	prFileNr = (prFileNr + 1) % 100; str(prFileNr, s);
	CPath = replaceNo(PrTab(prMgrFileNm), s);
	CVol = "";
	if (CPath.length() == 0) h = nullptr;
	else {
		h = OpenH(_isoverwritefile, Exclusive); if (HandleError != 0)
		{
			SetMsgPar(CPath); WrLLF10Msg(700 + HandleError); h = nullptr;
		}
	}
	return h;
}

void CopyToMgr()
{
	FILE* h1; FILE* h2;
	WORD n; void* buf; longint lbuf, sz; pstring s; void* cf; void* cr;
	h2 = OpenMgrOutput(); if (h2 == nullptr) return; cf = CFile; cr = CRecPtr;
	if (printBlk) WriteH(h2, nBlk, *pBlk);
	else { /* !!! with TextRec(Rprt) do!!! */
		h1 = Rprt.Handle; SeekH(h1, 0);
		lbuf = 1000; buf = GetStore(lbuf); sz = FileSizeH(h1);
		while (sz > 0) {
			if (sz > lbuf) n = lbuf;
			else n = sz;
			sz -= n;
			ReadH(h1, n, buf);
			WriteH(h2, n, buf);
		}
		ReleaseStore(buf); CloseH(h1);
		if (h1 == WorkHandle) WorkHandle = nullptr;
		/* !!! with TextRec(Rprt) do!!! */
		Rprt.Handle = nullptr;
	}
	CloseH(h2);
	ExecMgrPgm();
	CFile = (FileD*)cf;
	CRecPtr = cr;
}

void PrintTxtFBlk(longint BegPos, bool CtrlL)
{
	WORD Ti = 0, Times = 0, Cp = 0, Pl = 0, MaxLine = 0;
	bool FrstRun, AutoFF, FFOpt, NMOpt, He, Fo, adj;
	pstring FoTxt, HeTxt; pstring s(3); ExitRecord er;
	//NewExit(Ovr(), er);
	goto label3;
	RunMsgOn('P', 0);
	FrstRun = true; outpsw = false; charrd = 0;
	do {
		AutoFF = false; FFOpt = false; NMOpt = false; He = false; Fo = false;
		Po = 0; Ti = 1; Cp = spec.CpLines; Pl = spec.AutoRprtLimit + Cp;
		ResetInp();
		while (!EofInp())
		{
			RdLnInp(); s = copy(Ln, 1, 3);
			if (SEquUpcase(s, ".cp")) { AutoFF = true; GetNum(Cp); }
			else if (SEquUpcase(s, ".pl")) GetNum(Pl);
			else if (SEquUpcase(s, ".po")) GetNum(Po);
			else if (SEquUpcase(s, ".ti")) GetNum(Ti);
			else if (SEquUpcase(s, ".he"))
			{
				He = true; AutoFF = true; HeTxt = copy(Ln, 4, 255);
			}
			else if (SEquUpcase(s, ".fo"))
			{
				Fo = true; AutoFF = true; FoTxt = copy(Ln, 4, 255);
			}
			else if (SEquUpcase(s, ".ff")) FFOpt = true;
			else if (SEquUpcase(s, ".nm")) NMOpt = true;
			else goto label1;
		}
		goto label3;
	label1:
		adj = FrstRun && !FFOpt && !NMOpt;
		if (adj && spec.ChoosePrMsg) if (!PrinterMenu(62)) goto label3;
		if (printer[prCurr].ToMgr) { CopyToMgr(); goto label3; }
		if (!ResetPrinter(Pl, Po, adj, FrstRun)) goto label3;
		RprtPage = 1; RprtLine = 1;
		if (FrstRun) { FrstRun = false; Times = Ti; }
		if (Fo) Cp += 2;
		MaxLine = Pl - Cp;
		if (He) { PrintHeFo(HeTxt); NewLine(); }
		PrintStr(Ln);
		NewLine();
		while (!EofInp)
		{
			RdLnInp();
			if (AutoFF && ((RprtLine > MaxLine) || (Ln[1] == 0x0C)))
			{
				if (Fo)
				{
					while (RprtLine <= MaxLine) NewLine(); NewLine();
					PrintHeFo(FoTxt);
				}
				PrintChar_T(0x0C); RprtPage++; RprtLine = 1;
				if (He) { PrintHeFo(HeTxt); NewLine(); }
			}
			else if (Ln[1] == 0x0C) PrintChar_T(0x0C);
			if (Ln[1] == 0x0C) Ln = copy(Ln, 2, 255);
			PrintStr(Ln); NewLine();
			;
		}
		if (Fo)
		{
			while (RprtLine <= MaxLine) NewLine(); NewLine();
			PrintHeFo(FoTxt);
		}
		if (!FFOpt && CtrlL)
			if (PrTab(prClose) != "ff") PrintChar_T(0x0C); /*  Mark*** */
		Times--;
	} while (Times != 0);
	ClosePrinter(Po);
label3:
	RestoreExit(er);
	RunMsgOff();
}

void PrintTxtFile(longint BegPos)
{
	TestMountVol(CPath[1]); if (!Rprt.ResetTxt())
	{
		SetMsgPar(CPath);
		WrLLF10Msg(700 + HandleError);
		return;
	}
	printBlk = false;
	PrintTxtFBlk(BegPos, true);
	/* !!! with TextRec(Rprt) do!!! */
	if (Rprt.Handle == nullptr) return;
	Rprt.Close();
}

void PrintArray(void* P, WORD N, bool CtrlL)
{
	printBlk = true; pBlk = CharArrPtr(P); nBlk = N; PrintTxtFBlk(0, CtrlL);
}

void PrintFandWork()
{
	CloseH(WorkHandle);
	Rprt.Assign(FandWorkName.c_str());
	/* !!! with TextRec(Rprt) do!!! */
	{
		Rprt.openfunc = &Rprt.opentxt;
		OpenWorkH();
		Rprt.Handle = WorkHandle;
	}
	Rprt.Reset(); printBlk = false;
	PrintTxtFBlk(0, true);
	if (WorkHandle == nullptr) OpenWorkH();
}

void PrintChar_T(char c)
{
	if (outpsw) PrintChar(c);
}

void PrintStr(pstring s)
{
	WORD i;
	for (i = 1; i < s.length(); i++) PrintChar_T(s[i]);
}

void PrintHeFo(pstring T)
{
	WORD i; pstring m, s; bool point;
	i = 1;
	while (i <= T.length())
		if (T[i] == '_')
		{
			m = ""; point = false;
			while ((i <= T.length()) && (T[i] == '_' || T[i] == '.' || T[i] == ':'))
			{
				if (T[i] != '_') point = true; m = m + T[i]; i++;
			}
			if (point)
				if (m == "__.__.__") PrintStr(StrDate(Today(), "DD.MM.YY"));
				else if (m == "__.__.____") PrintStr(StrDate(Today(), "DD.MM.YYYY"));
				else if (m == "__:__") PrintStr(StrDate(CurrTime(), "mm hh"));
				else PrintStr(m);
			else { str(RprtPage, m.length(), m); PrintStr(m); }
		}
		else { PrintChar_T(T[i]); i++; }
	NewLine();
}

void GetNum(WORD& NN)
{
	WORD i, n;
	val(LeadChar(' ', TrailChar(' ', copy(Ln, 4, 255))), n, i);
	if (i == 0) NN = n;
}

bool EofInp()
{
	if (printBlk) return iBlk > nBlk;
	else return Rprt.eof;
}

void RdLnInp()
{
	char c;
	TextFile* F = &Rprt;
	Ln[1] = ' ';
	if (printBlk) {
		Ln = "";
		while (iBlk <= nBlk) {
			c = *pBlk[iBlk]; iBlk++;
			if (c == 0x0D) {
				charrd++;
				if (*pBlk[iBlk] == 0x0A) { iBlk++; charrd++; }
				goto label1;
			}
			Ln = Ln + c;
		}
	label1:
		charrd += Ln.length();
		//if (charrd > BegPos) outpsw = true;
	}
	else {
		//readln(Rprt, Ln);
		//if (!outpsw) /* !!! with F do!!! */
		//	if (PosH(F->Handle) - (F->BufEnd - F->BufPos) >= F->BegPos) outpsw = true;
	}
}

void ResetInp()
{
	if (printBlk) iBlk = 1;
	else Seek0Txt(&Rprt);
}


