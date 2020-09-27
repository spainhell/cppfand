#include "obase.h"

#include <set>
#include "editor.h"
#include "obaseww.h"

bool SFlag, QFlag, WFlag, BFlag, DFlag, EFlag, AFlag, XFlag, VFlag, TFlag;
WORD CPState, CPCount;
bool PrintCtrlFlag;

void ResetCtrlFlags()
{
	SFlag = false; BFlag = false; QFlag = false; WFlag = false; DFlag = false;
	EFlag = false; AFlag = false; XFlag = false; VFlag = false; TFlag = false;
	CPState = 0;
}

bool IsPrintCtrl(char C)
{
	// ^S ^Q ^W ^B ^D ^E ^A ^X ^V ^T
	std::set<char> pc = { 0x13, 0x11, 0x17, 0x02, 0x04, 0x05, 0x01, 0x18, 0x16, 0x14 };
	return pc.count(C) > 0;
}

void PrintByte(BYTE B)
{
	const BYTE timeout = 0x01;
	const BYTE errorio = 0x08;
	const BYTE selected = 0x10;
	const BYTE outofpaper = 0x20;
	const BYTE acknowledge = 0x40;
	const BYTE notbusy = 0x80;

	// PØÍMÝ TISK NA PORT NEBO DO HANDLE -> NEUMÍME :-)

//	registers r;
//	WORD N, LptNr;
//	auto prnt = printer[prCurr];
//	{
//		if (prnt.ToHandle) { WriteH(prnt.Handle, 1, &B); return; }
//		LptNr = Lpti - 1;
//	}
//	if (ESCPressed && PromptYN(22)) {
//		r.AH = 0; r.AL = 0x18/*cancel buffer*/; r.DX = LptNr; Intr(0x17, r);
//		r.AH = 0; r.AL = 0x0C/*form feed*/; r.DX = LptNr; Intr(0x17, r);
//		WasLPTCancel = true; GoExit();
//	}
//label2:
//	r.AH = 0; r.AL = B; r.DX = LptNr; Intr(0x17, r);
//	if ((r.AH && errorio) != 0)
//		if ((r.AH && outofpaper) != 0) { N = 11;/*out of paper*/ goto label3; }
//		else { N = 12;/*turn printer on*/ goto label3; }
//	else if ((r.AH && timeout) != 0) {
//		N = 13; /*printer offline*/
//	label3:
//		F10SpecKey = _ESC_; WrLLF10Msg(N);
//		if (KbdChar == _ESC_) GoExit();
//		else goto label2;
//	}
}

void PrintByteStr(pstring S)
{
	WORD i;
	for (i = 1; i < S.length(); i++) PrintByte(S[i]);
}

pstring CtrlToESC(char C)
{
	switch (C) {
	case 0x08: // ^H
	case 0x0C: return C; break; // ^M
	case 0x0D: // { weak new - line, ^ j of hard new - line ignored }
	{
		pstring res = 0x0D; res += 0x0A; // M + J
		return res; break;
	}
	case 0x13: // ^S
		if (SFlag) { return PrTab(prUl2); SFlag = false; }
		else { SFlag = true; return PrTab(prUl1); }
		break;
	case 0x11: // ^Q
		if (QFlag) { return PrTab(prBr2); QFlag = false; }
		else { QFlag = true; return PrTab(prBr1); }
		break;
	case 0x17: // ^W
		if (WFlag) { return PrTab(prKv2); WFlag = false; }
		else { WFlag = true; return PrTab(prKv1); }
		break;
	case 0x02: // ^B
		if (BFlag) { return PrTab(prBd2); BFlag = false; }
		else { BFlag = true; return PrTab(prBd1); }
		break;
	case 0x04: // ^D
		if (DFlag) { return PrTab(prDb2); DFlag = false; }
		else { DFlag = true; return PrTab(prDb1); }
		break;
	case 0x05: // ^E
		if (EFlag) { return PrTab(prKp2); EFlag = false; }
		else { EFlag = true; return PrTab(prKp1); }
		break;
	case 0x01: // ^A
		if (AFlag) { return PrTab(prEl2); AFlag = false; }
		else { AFlag = true; return PrTab(prEl1); }
		break;
	case 0x18: // ^X
		if (XFlag) { return PrTab(prUs12); XFlag = false; }
		else { XFlag = true; return PrTab(prUs11); }
		break;
	case 0x16: // ^V
		if (VFlag) { return PrTab(prUs22); VFlag = false; }
		else { VFlag = true; return PrTab(prUs21); }
		break;
	case 0x14: // ^T
		if (TFlag) { return PrTab(prUs32); TFlag = false; }
		else { TFlag = true; return PrTab(prUs31); }
		break;
	default: return ""; break;
	}
}

WORD CPTest(char c)
{
	WORD result = 0;
	switch (CPState) {  /*0=normal; 1=lo count; 2=hi count; 3=binary */
	case 0:
		if (c < ' ') {
			if (c == 0x10) {
				CPState = 1;
				result = 3;
			}
			else result = 2;
		}
		else result = 1;
		break;
	case 1: { CPState = 2; CPCount = c; result = 3; break; }
	case 2: {
		CPCount = CPCount + (WORD(c) << 8);
		if (CPCount == 0) CPState = 0;
		else CPState = 3;
		result = 3;
		break;
	}
	case 3: {
		CPCount--;
		result = 0;
		if (CPCount == 0) CPState = 0;
	}
	}
	return result;
}

void TranslateCodePage(WORD& c)
{
	// pøeklady nejsou podporovány - bude jen Latin2
	if (c >= 0x80)
		switch (printer[prCurr].Kod) {
		case 'K': ConvKamenLatin(&c, 1, true);
		case 'k': ConvToNoDiakr(&c, 1, TVideoFont::foKamen);
		case 'L': ConvKamenLatin(&c, 1, false);
		case 'l': ConvToNoDiakr(&c, 1, TVideoFont::foLatin2);
		}
}

void PrintChar(char C)
{
	WORD wC = C;
	switch (CPTest(C)) {
	case 0: PrintByte(C); break;
	case 1: { TranslateCodePage(wC); PrintByte(C); break; }
	case 2: PrintByteStr(CtrlToESC(C)); break;
	}
}

WORD OpenLPTHandle()
{
	return 0;
}

bool ResetPrinter(WORD PgeLength, WORD LeftMargin, bool Adj, bool Frst)
{
	return true;
}

void ClosePrinter(WORD LeftMargin)
{

}

void TestTxtHError(TextFile* F)
{
	pstring s;
	if (HandleError != 0) {
		SetMsgPar(StrPas(F->name.c_str()));
		WrLLF10Msg(700 + HandleError);
		GoExit();
	}
}

integer InputTxt(TextFile* F)
{
	F->bufend = ReadH(F->Handle, F->bufsize, F->buffer);
	F->bufpos = 0;
	TestTxtHError(F); return 0;
}

integer OutputTxt(TextFile* F)
{
	/*void* p; WORD n; pstring s;
	CharPtr x1; WORD absolute x1 x1ofs; CharPtr x2; WORD absolute x2 x2ofs;
	if (PrintCtrlFlag) {
		p = GetStore(1024); x1 = void* (F.BufPtr); x2 = p; n = 0;
		while (F.BufPos > 0) {
			switch (CPTest(x1^)) {
				0{ x2^ = x1*; inc(n); inc(x2ofs); }
				1:{ x2^ = x1*; inc(n); TranslateCodePage(x2^); inc(x2ofs); }
				2{ s = CtrlToESC(x1^); move(s[1], x2^, length(s));
				inc(n, length(s)); inc(x2ofs, length(s))};
			}
			inc(x1ofs); dec(F.BufPos);
		}
		WriteH(F.Handle, n, p^); ReleaseStore(p)
	}
	else WriteH(F.Handle, F.BufPos, F.BufPtr^);
	F.BufPos = 0; TestTxtHError(F);*/
	return 0;
}

integer OutputLPT1(TextFile* F)
{
	//CharPtr c; WORD absolute c COfs;
	///* !!! with F do!!! */ {
	//	c = CharPtr(BufPtr); while (BufPos > 0) {
	//		if (not WasLPTCancel) PrintChar(c^); dec(BufPos); inc(COfs)
	//	}
	//}
	return 0;
}

integer FlushTxt(TextFile* F)
{
	return 0;
}

integer CloseTxt(TextFile* F)
{
	CloseH(F->Handle); TestTxtHError(F);
	return 0;
}

integer CloseLPT1(TextFile* F)
{
	if (!WasLPTCancel) PrintChar(0x0C);
	return 0;
}

integer OpenTxt(TextFile* F)
{
	/* !!! with F do!!! */
	if (F->Mode == "rb" /*append*/) SeekH(F->Handle, FileSizeH(F->Handle));
	if (F->Mode == "rb") F->inoutfunc = &InputTxt;
	else F->inoutfunc = &OutputTxt;
	F->flushfunc = &FlushTxt;
	F->closefunc = &CloseTxt;
	
	if (PrintCtrlFlag) ResetCtrlFlags();
	return 0;
}

integer OpenLPT1(TextFile* F)
{
	/* !!! with F do!!! */
	// InOutFunc = @OutputLPT1; CloseFunc = @CloseLPT1;
	return 0;
}

void Seek0Txt(TextFile* F)
{
	SeekH(F->Handle, 0);
	F->bufend = F->bufsize;
	F->bufpos = F->bufend;
}

bool ResetTxt(TextFile* F)
{
	F->Assign(CPath.c_str());
	/* !!! with TextRec(F) do!!! */
	{
		F->openfunc = &OpenTxt; F->Handle = nullptr; /* for error detection in OpenH */
		F->Handle = OpenH(_isoldfile, RdOnly);
	}
	if (HandleError != 0) { return false; }
	F->Reset();
	return true;
}

bool RewriteTxt(TextFile* F, bool PrintCtrl)
{
	F->Assign(CPath.c_str());
	if (CPath == "LPT1") F->openfunc = &OpenLPT1;
	else {
		PrintCtrlFlag = PrintCtrl;
		F->openfunc = &OpenTxt;
		F->Handle = OpenH(_isoverwritefile, Exclusive);
		if (HandleError != 0) { return false; }
	}
	F->Rewrite();
	return true;
}

void SetPrintTxtPath()
{
	CPath = WrkDir + "PRINTER.TXT";
	CVol = "";
}


