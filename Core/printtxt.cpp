#include "printtxt.h"

#include <fstream>
#include "../Core/DateTime.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "obase.h"
#include "obaseww.h"
#include "RunMessage.h"
#include "runfrml.h"
#include "wwmenu.h"

char* pBlk;
WORD iBlk, nBlk, Po;
int charrd;
bool printBlk, outpsw;
WORD prFileNr;
pstring Ln;

void NewLine()
{
	
}

void PrintHeFo(pstring T)
{
	WORD i = 1; pstring m, s; bool point = false;
	while (i <= T.length())
		if (T[i] == '_')
		{
			m = "";
			point = false;
			while ((i <= T.length()) && (T[i] == '_' || T[i] == '.' || T[i] == ':')) {
				if (T[i] != '_') point = true;
				m = m + T[i];
				i++;
			}
			if (point)
				if (m == "__.__.__") PrintStr(StrDate(Today(), "DD.MM.YY"));
				else if (m == "__.__.____") PrintStr(StrDate(Today(), "DD.MM.YYYY"));
				else if (m == "__:__") PrintStr(StrDate(CurrTime(), "mm hh"));
				else PrintStr(m);
			else {
				str(RprtPage, m.length(), m);
				PrintStr(m);
			}
		}
		else {
			PrintChar_T(T[i]);
			i++;
		}
	NewLine();
}

std::string replaceNo(std::string s, std::string sNew)
{
	size_t i = s.find('#');
	if (i != std::string::npos) {
		s.replace(i, 1, sNew);
	}
	return s;
}

void ExecPrintManagerProgram()
{
	uint8_t x = 0, y = 0;
	std::string pgmNm = PrTab(prCurr, prMgrProg);
	if (pgmNm.empty()) return;
	std::string param = replaceNo(PrTab(prCurr, prMgrParam), CPath);
	Wind wmin = WindMin;
	Wind wmax = WindMax;
	TCrs crs = screen.CrsGet();
	int w = PushW(1, 1, TxtCols, 1);
	WindMin = wmin;
	WindMax = wmax;
	screen.CrsSet(crs);
	OSshell(pgmNm, param, true, true, false, false);
	PopW(w);
	screen.GotoXY(x - WindMin.X + 1, y - WindMin.Y + 1);
}

HANDLE OpenPrintManagerOutput()
{
	HANDLE h;
	prFileNr = (prFileNr + 1) % 100;
	std::string s = std::to_string(prFileNr);
	CPath = replaceNo(PrTab(prCurr, prMgrFileNm), s);
	CVol = "";
	if (CPath.length() == 0) h = nullptr;
	else {
		h = OpenH(CPath, _isOverwriteFile, Exclusive);
		if (HandleError != 0) {
			SetMsgPar(CPath);
			WrLLF10Msg(700 + HandleError);
			h = nullptr;
		}
	}
	return h;
}

void CopyToPrintManager(const std::string& text)
{
	HANDLE h2 = OpenPrintManagerOutput();
	if (h2 == nullptr) return;
	//FileD* cf = CFile; void* cr = CRecPtr;
	if (printBlk) {
		WriteH(h2, nBlk, pBlk);
	}
	else {
		//int n;
		//FILE* h1 = Rprt.Handle;
		//SeekH(h1, 0);
		//int lbuf = 1000;
		//void* buf = GetStore(lbuf);
		//int sz = FileSizeH(h1);
		//while (sz > 0) {
		//	if (sz > lbuf) n = lbuf;
		//	else n = sz;
		//	sz -= n;
		//	ReadH(h1, n, buf);
		//	WriteH(h2, n, buf);
		//}
		//ReleaseStore(buf);
		//CloseH(&h1);
		//if (h1 == WorkHandle) WorkHandle = nullptr;
		///* !!! with TextRec(Rprt) do!!! */
		//Rprt.Handle = nullptr;
		WriteH(h2, text.length(), (void*)text.c_str());
	}
	CloseH(&h2);
	ExecPrintManagerProgram();
	//CFile = cf; CRecPtr = cr;
}

void PrintTxtFBlk(const std::string& text, int BegPos, bool CtrlL)
{
	WORD Ti = 0, Times = 0, Cp = 0, Pl = 0, MaxLine = 0;
	pstring FoTxt, HeTxt;
	pstring s(3);
	// NewExit(Ovr(), er);
	// goto label3;
	RunMsgOn('P', 0);

	auto lines = GetAllLines(text);

	bool FrstRun = true; outpsw = false; charrd = 0;
	do {
		bool AutoFF = false; bool FFOpt = false;
		bool NMOpt = false; bool He = false;
		bool Fo = false;
		Po = 0; Ti = 1;
		Cp = spec.CpLines;
		Pl = spec.AutoRprtLimit + Cp;
		ResetInp();
		for (std::string& line : lines) {
			// RdLnInp(); // TODO: tisk bloku
			std::string s = line.substr(0, 3);
			if (EquUpCase(s, ".cp")) {
				AutoFF = true;
				GetNum(Cp);
			}
			else if (EquUpCase(s, ".pl")) GetNum(Pl);
			else if (EquUpCase(s, ".po")) GetNum(Po);
			else if (EquUpCase(s, ".ti")) GetNum(Ti);
			else if (EquUpCase(s, ".he")) {
				He = true; AutoFF = true;
				HeTxt = copy(Ln, 4, 255);
			}
			else if (EquUpCase(s, ".fo")) {
				Fo = true; AutoFF = true;
				FoTxt = copy(Ln, 4, 255);
			}
			else if (EquUpCase(s, ".ff")) FFOpt = true;
			else if (EquUpCase(s, ".nm")) NMOpt = true;
			else goto label1;
		}
		goto label3;
	label1:
		bool adj = FrstRun && !FFOpt && !NMOpt;
		if (adj && spec.ChoosePrMsg) if (!PrinterMenu(62)) goto label3;
		if (printer[prCurr].ToMgr) {
			CopyToPrintManager(text);
			goto label3;
		}
		if (!ResetPrinter(Pl, Po, adj, FrstRun)) goto label3;
		RprtPage = 1; RprtLine = 1;
		if (FrstRun) {
			FrstRun = false;
			Times = Ti;
		}
		if (Fo) Cp += 2;
		MaxLine = Pl - Cp;
		if (He)	{
			PrintHeFo(HeTxt);
			NewLine();
		}
		PrintStr(Ln);
		NewLine();
		while (!EofInp())
		{
			RdLnInp();
			if (AutoFF && ((RprtLine > MaxLine) || (Ln[1] == 0x0C))) {
				if (Fo) {
					while (RprtLine <= MaxLine) NewLine();
					NewLine();
					PrintHeFo(FoTxt);
				}
				PrintChar_T(0x0C);
				RprtPage++;
				RprtLine = 1;
				if (He) {
					PrintHeFo(HeTxt);
					NewLine();
				}
			}
			else if (Ln[1] == 0x0C) PrintChar_T(0x0C);
			if (Ln[1] == 0x0C) Ln = copy(Ln, 2, 255);
			PrintStr(Ln);
			NewLine();
		}
		if (Fo) {
			while (RprtLine <= MaxLine) NewLine();
			NewLine();
			PrintHeFo(FoTxt);
		}
		if (!FFOpt && CtrlL)
			if (PrTab(prCurr, prClose) != "ff") PrintChar_T(0x0C); /*  Mark*** */
		Times--;
	} while (Times != 0);
	ClosePrinter(Po);
label3:
	//RestoreExit(er);
	RunMsgOff();
}

void PrintTxtFile(int BegPos)
{
	//TestMountVol(CPath[1]);
	//if (!Rprt.ResetTxt())
	//{
	//	SetMsgPar(CPath);
	//	WrLLF10Msg(700 + HandleError);
	//	return;
	//}
	std::string text;
	try {
		//std::ifstream t(CPath);
		//text = std::string((std::istreambuf_iterator<char>(t)),	std::istreambuf_iterator<char>());
		//t.close();
		FILE* handle;
		fopen_s(&handle, CPath.c_str(), "r");
		fseek(handle, 0, std::ios::end);
		size_t size = ftell(handle);
		text = std::string(size, ' ');
		fseek(handle, 0, std::ios::beg);
		fread(&text[0], 1, size, handle);
		fclose(handle);
	}
	catch (std::exception& e) {
		SetMsgPar(CPath);
		WrLLF10Msg(700 + HandleError);
		return;
	}

	printBlk = false;
	PrintTxtFBlk(text, BegPos, true);

	//if (Rprt.Handle == nullptr) return;
	//Rprt.Close(nullptr); // nullptr je tady navic po uprave metody Close()
}



void PrintArray(void* P, WORD N, bool CtrlL)
{
	printBlk = true;
	std::string text = std::string((char*)P);
	//pBlk = CharArrPtr(P);
	//nBlk = N;
	PrintTxtFBlk(text, 0, CtrlL);
}

void PrintArray(const std::string& arr, bool CtrlL)
{
	printBlk = true;
	PrintTxtFBlk(arr, 0, CtrlL);
}

void PrintFandWork()
{
	//CloseH(&WorkHandle);
	//Rprt.Assign(FandWorkName.c_str());
	///* !!! with TextRec(Rprt) do!!! */
	//{
	//	Rprt.openfunc = Rprt.opentxt;
	//	OpenWorkH();
	//	Rprt.Handle = WorkHandle;
	//}
	//Rprt.Reset();
	//printBlk = false;
	//PrintTxtFBlk(0, true);
	//if (WorkHandle == nullptr) OpenWorkH();
}

void PrintChar_T(char c)
{
	if (outpsw) PrintChar(c);
}

void PrintStr(pstring s)
{
	for (WORD i = 1; i <= s.length(); i++) PrintChar_T(s[i]);
}

void GetNum(WORD& NN)
{
	WORD i = 0, n = 0;
	val(LeadChar(' ', OldTrailChar(' ', copy(Ln, 4, 255))), n, i);
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
			c = pBlk[iBlk];
			iBlk++;
			if (c == 0x0D) {
				charrd++;
				if (pBlk[iBlk] == 0x0A) { iBlk++; charrd++; }
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
	//else Seek0Txt(&Rprt);
}


