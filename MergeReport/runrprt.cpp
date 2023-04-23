#include "runrprt.h"

#include "../cppfand/compile.h"
#include "../cppfand/ChkD.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obase.h"
#include "../cppfand/obaseww.h"
#include "runmerg.h"
#include "shared.h"
#include "../cppfand/wwmix.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"

WORD PrintDH;
YRec Y;
bool FrstBlk, NoFF, WasFF2, SetPage, WasOutput;
short LineLenLst, PageNo, PgeSize;
void* Store2Ptr;
int RecCount;
WORD NEof;
InpD* MinID;
bool FirstLines, WasDot;
int NLinesOutp;


void RunReport(RprtOpt* RO)
{
	std::string ReportString;
	wwmix ww;
	LvDescr* L = nullptr;
	std::string* s = nullptr;
	WORD i = 0;
	bool frst = false, isLPT1 = false;
	WORD Times = 0;
	bool ex = false, b = false;
	BlkD* BD = nullptr;
	BlkD* RFb = nullptr;
	LockMode md;
	if (SelQuest) /* !!! with IDA[1]^ do!!! */ {
		CFile = IDA[1]->Scan->FD;
		if (!ww.PromptFilter("", &IDA[1]->Bool, s)) {
			PrintView = false;
			return;
		}
	}
	if (PgeLimitZ != nullptr) {
		PgeLimit = RunInt(PgeLimitZ);
	}
	else {
		PgeLimit = spec.AutoRprtLimit;
	}

	if (PgeSizeZ != nullptr) {
		PgeSize = RunInt(PgeSizeZ);
	}
	else {
		PgeSize = spec.AutoRprtLimit + spec.CpLines;
	}

	if (PgeSize < 2) PgeSize = 2;
	if ((PgeLimit > PgeSize) || (PgeLimit == 0)) PgeLimit = PgeSize - 1;
	if (!RewriteRprt(RO, PgeSize, Times, isLPT1)) return;  // pouze zajisti otevreni souboru
	MarkStore(Store2Ptr);
	ex = true;
	//PushProcStk();
	//NewExit(Ovr(), er);
	//goto label3;
	OpenInp();
	MergOpGroup.Group = 1.0; frst = true; NLinesOutp = 0; PrintDH = 2;
label0:
	RunMsgOn('R', NRecsAll);
	RecCount = 0;
	for (i = 1; i <= MaxIi; i++) {
		if (frst) frst = false;
		else IDA[i]->Scan->SeekRec(0);
		ReadInpFile(IDA[i]);
	}
	RprtPage = 1; RprtLine = 1; SetPage = false; FirstLines = true; WasDot = false;
	WasFF2 = false; NoFF = false; LineLenLst = 0; FrstBlk = true;
	ResetY();
	L = LstLvM;
	RFb = LstLvM->Ft;
	ZeroSumFlds(L);
	GetMinKey();
	ZeroCount();
	MoveFrstRecs();
	if (RprtHd != nullptr) {
		if (RprtHd->FF1) FormFeed(ReportString);
		RprtPage = 1;
		PrintBlkChn(RprtHd, ReportString, false, false);
		TruncLine(ReportString);
		if (WasFF2) FormFeed(ReportString);
		if (SetPage) {
			SetPage = false;
			RprtPage = PageNo;
		}
		else RprtPage = 1;
	}
	if (NEof == MaxIi) goto label2;
label1:
	if (WasFF2) PrintPageHd(ReportString);
	Headings(L, nullptr, ReportString);
	MergeProc(ReportString);
	MoveMFlds(NewMFlds, OldMFlds);
	GetMinKey();
	if (NEof == MaxIi) {
		if (FrstLvM != LstLvM) Footings(FrstLvM, LstLvM->ChainBack, ReportString);
	label2:
		WasFF2 = false;
		TruncLine(ReportString);
		PrintBlkChn(RFb, ReportString, false, true);
		b = WasFF2;
		if ((PageFt != nullptr) && !PageFt->NotAtEnd) PrintPageFt(ReportString);
		TruncLine(ReportString);
		RunMsgOff();
		if (Times > 1 /*only LPT1*/) {
			Times--;
			printf("%s%c", Rprt.c_str(), 0x0C);
			goto label0;
		}
		if (b) FormFeed(ReportString);
		ex = false;
	label3:
		//RestoreExit(er);
		if (PrintView && (NLinesOutp == 0) && (LineLenLst == 0)) {
			ReadMessage(159);
			printf("%s\n", ReportString.c_str());
			printf("%s%s", ReportString.c_str(), MsgLine.c_str());
		}
		Rprt.Close(ReportString.c_str());
		// if (isLPT1) ClosePrinter(0);
		CloseInp();
		//PopProcStk();
		if (ex) {
			RunMsgOff();
			if (!WasLPTCancel) GoExit();
		}
		return;
	}
	L = GetDifLevel();
	Footings(FrstLvM, L, ReportString);
	if (WasFF2) PrintPageFt(ReportString);
	ZeroSumFlds(L);
	ZeroCount();
	MoveFrstRecs();
	MergOpGroup.Group = MergOpGroup.Group + 1.0;
	goto label1;
}

void ResetY()
{
	Y.Blk = nullptr;
	Y.ChkPg = false;
	Y.I = 0;
	Y.Ln = 0;
	Y.P = nullptr;
	Y.Sz = 0;
	Y.TD = nullptr;
	Y.TLn = 0;
}

void IncPage()
{
	if (SetPage) {
		SetPage = false;
		RprtPage = PageNo;
	}
	else RprtPage++;
}

void NewLine(std::string& text)
{
	//printf("%s\n", Rprt.c_str());
	text += '\n';
	if (WasDot) WasDot = false;
	else {
		RprtLine++;
		NLinesOutp++;
		FirstLines = false;
	}
	LineLenLst = 0;
	if (RprtLine > PgeSize) {
		RprtLine -= PgeSize;
		IncPage();
	}
}

void FormFeed(std::string& text)
{
	short I = 0;
	if (NoFF) NoFF = false;
	else {
		text += (char)0x0C; // ^L 012 0x0C Form feed
		RprtLine = 1;
		IncPage();
	}
	LineLenLst = 0;
	WasFF2 = false;
}

void NewPage(std::string& text)
{
	PrintPageFt(text);
	PrintPageHd(text);
	TruncLine(text);
	FrstBlk = false;
}

bool OutOfLineBound(BlkD* B)
{
	return ((B->LineBound != nullptr) && (RprtLine > RunReal(B->LineBound))
		|| B->AbsLine && (RunInt(B->LineNo) < RprtLine));
}

void WriteNBlks(std::string& text, short N)
{
	if (N > 0) {
		char buffer[256]{ '\0' };
		snprintf(buffer, sizeof(buffer), "%*c", N, ' ');
		//printf("%s%*c", Rprt.c_str(), N, ' ');
		text += buffer;
	}
}

/// <summary>
/// Vlozi text na dany radek na dane misto o max. definovane delce
/// </summary>
/// <param name="S">Vstupni text</param>
/// <param name="Col">Pozice, kam text vlozit</param>
/// <param name="Width">Delka textu</param>
/// <param name="Wrap"></param>
/// <returns></returns>
std::string NewTxtCol(std::string S, WORD Col, WORD Width, bool Wrap)
{
	WORD i = 0, Ln = 0;
	TTD* TD = nullptr;
	std::string ss;
	StringListEl* SL = nullptr;

	Ln = 0;
	bool Absatz = true;
	if (Wrap) for (i = 0; i < S.length(); i++) {
		if ((S[i] == 0x0D) && ((i == S.length()) || (S[i + 1] != 0x0A))) S[i] = ' ';
	}

	ss = GetLine(S, Width, Wrap, Absatz);
	//printf("%s%s", Rprt.c_str(), ss.c_str());

	while (S.length() > 0) {
		std::string strLine = GetLine(S, Width, Wrap, Absatz);
		Ln++;
		if (Ln == 1) {
			TD = new TTD(); // (TTD*)GetStore2(sizeof(*TD));
			TD->SL = nullptr;
			TD->Col = Col;
			TD->Width = Width;
		}
		SL = new StringListEl(); // (StringListEl*)GetStore2(ss.length() + 5);
		SL->S = strLine;
		if (TD->SL == nullptr) TD->SL = SL;
		else ChainLast(TD->SL, SL);
	}
	if (Ln > 0) {
		TD->Ln = Ln;
		if (Y.TD == nullptr) Y.TD = TD;
		else ChainLast(Y.TD, TD);
		if (Ln > Y.TLn) Y.TLn = Ln;
	}
	return ss;
}

/// <summary>
/// 
/// </summary>
/// <param name="S"></param>
/// <param name="Width"></param>
/// <param name="Wrap"></param>
/// <param name="paragraph">odstavec (v originale Absatz)</param>
/// <returns></returns>
std::string GetLine(std::string& S, WORD Width, bool Wrap, bool& paragraph)
{
	WORD TAOff = 0;
	short j = 0, iWrdEnd = 0, i2WrdEnd = 0, wWrdEnd = 0, nWrdEnd = 0;
	pstring s;
	pstring s1;
	char c = '\0';
	short i = 0; short i2 = 0; short w = 0;

	bool WasWrd = false;
	short nWords = 0;
	bool Fill = false;
	while ((i < S.length()) && (i2 < 255)) {
		c = S[TAOff + i];
		if (c == 0x0D) {
			paragraph = true;
			goto label1;
		}
		if ((w >= Width) && (c == ' ') && Wrap) goto label1;
		if ((c != ' ') || WasWrd || !Wrap || paragraph) {
			i2++;
			s[TAOff + i2] = c;
			if (!IsPrintCtrl(c)) w++;
		}
		if (c == ' ') {
			if (WasWrd)
			{
				WasWrd = false;
				i2WrdEnd = i2 - 1;
				iWrdEnd = i;
				wWrdEnd = w - 1;
				nWrdEnd = nWords;
			}
		}
		else if (!WasWrd) {
			WasWrd = true;
			paragraph = false;
			nWords++;
		}
		i++;
	}
label1:
	if (Wrap && (nWords >= 2) && (w > Width)) {
		i2 = i2WrdEnd;
		i = iWrdEnd;
		w = wWrdEnd;
		nWords = nWrdEnd;
		Fill = true;
		paragraph = false;
	}
	if ((i < S.length()) && (S[TAOff + i] == 0x0D)) {
		i++;
		if ((i < S.length()) && (S[TAOff + i] == 0x0A)) i++;
	}
	TAOff += i;
	S = S.erase(0, i); // TLen -= i;
	short l2 = i2;
	if (w < Width) l2 += (Width - w);
	short n = l2 - i2;
	if ((nWords <= 1) || (n == 0) || !Fill)
	{
		FillChar(&s[i2 + 1], n, ' ');
		s[0] = (char)l2;
	}
	else {
		short n1 = n / (nWords - 1);
		short n2 = n % (nWords - 1);
		s[0] = (char)i2;
		s1 = s;
		s[0] = (char)l2;
		i2 = 1;
		WasWrd = false;
		for (i = 1; i <= s1.length(); i++) {
			s[i2] = s1[i];
			if (s[i2] != ' ') WasWrd = true;
			else if (WasWrd) {
				WasWrd = false;
				for (j = 1; j <= n1; j++) {
					i2++;
					s[i2] = ' ';
				}
				if (n2 > 0) {
					n2--;
					i2++;
					s[i2] = ' ';
				}
			}
			i2++;
		}
	}
	return s;
}

void CheckPgeLimit(std::string& text)
{
	void* p2;
	YRec YY;
	if (Y.ChkPg && (RprtLine > PgeLimit) && (PgeLimit < PgeSize)) {
		p2 = Store2Ptr;
		MarkStore(Store2Ptr);
		YY = Y;
		ResetY();
		NewPage(text);
		Y = YY;
		Store2Ptr = p2;
	}
}

void PendingTT(std::string& text)
{
	StringListEl* SL = nullptr;
	WORD lll = LineLenLst;
	WORD Col = LineLenLst + 1;
	while (Y.TLn > 0) {
		NewLine(text);
		CheckPgeLimit(text);
		TTD* TD = Y.TD;
		Col = 1;
		while (TD != nullptr) {
			if (TD->Ln > 0) {
				WriteNBlks(text, TD->Col - Col);
				SL = TD->SL;
				text += SL->S;
				Col = TD->Col + GetLengthOfStyledString(SL->S);
				TD->Ln--;
				TD->SL = SL->pChain;
			}
			TD = TD->pChain;
		}
		Y.TLn--;
		LineLenLst = lll;
	}
	WriteNBlks(text, LineLenLst + 1 - Col);
	if (Y.TD != nullptr) {
		ReleaseStore(Store2Ptr);
		Y.TD = nullptr;
		Y.TLn = 0;
	}
}

void FinishTuple(std::string& text)
{
	while (Y.Blk != nullptr) {
		Print1NTupel(text, true);
	}
}

void RunAProc(std::vector<AssignD*> vAssign)
{
	for (AssignD* A : vAssign) {
		switch (A->Kind) {
		case _locvar: { LVAssignFrml(A->LV, A->Add, A->Frml); break; }
		case _parfile: { AsgnParFldFrml(A->FD, A->PFldD, A->Frml, A->Add); break; }
		case _ifthenelseM: {
			if (RunBool(A->Bool)) {
				RunAProc(A->Instr);
			}
			else {
				RunAProc(A->ElseInstr);
			}
			break;
		}
		default: 
			break;
		}
	}
}

void PrintBlock(BlkD* B, std::string& text, BlkD* DH)
{
	WORD LAfter = 0; BlkD* B1 = nullptr;
	bool pdh = false;
	while (B != nullptr) {
		if (RunBool(B->Bool)) {
			if (B != Y.Blk) {
				if ((B->NTxtLines > 0) && (B->NBlksFrst < LineLenLst)) TruncLine(text);
				if (OutOfLineBound(B)) WasFF2 = true;
				LAfter = RprtLine + MaxI(0, B->NTxtLines - 1);
				if ((DH != nullptr) && (PrintDH >= DH->DHLevel + 1)) {
					B1 = DH;
					while (B1 != nullptr) {
						if (RunBool(B1->Bool)) LAfter += B1->NTxtLines;
						B1 = B1->pChain;
					}
				}
				if (B->FF1 || WasFF2 || FrstBlk && (B->NTxtLines > 0) ||
					(PgeLimit < PgeSize) && (LAfter > PgeLimit))
					NewPage(text);
				if ((DH != nullptr) && (PrintDH >= DH->DHLevel + 1)) {
					PrintBlkChn(DH, text, true, false);
					PrintDH = 0;
				}
			}
			WasOutput = false;
			PrintTxt(B, text, true);
			WasFF2 = B->FF2;
			if ((DH == nullptr) && WasOutput) pdh = true;
			SumUp(B->Sum);
		}
		B = B->pChain;
	}
	if (pdh) PrintDH = 2;
}

void PrintTxt(BlkD* B, std::string& text, bool ChkPg)
{
	if (B == nullptr) return;
	if (B->SetPage) {
		PageNo = RunInt(B->PageNo);
		SetPage = true;
	}
	if (B != Y.Blk) {
		FinishTuple(text);
		if (B->AbsLine) {
			for (int i = RprtLine; i <= RunInt(B->LineNo) - 1; i++) {
				NewLine(text);
			}
		}
		if (B->NTxtLines > 0) {
			if (B->NBlksFrst < LineLenLst) NewLine(text);
			if (B->NBlksFrst - LineLenLst > 0) {
				char buffer[256]{ '\0' };
				snprintf(buffer, sizeof(buffer), "%*c", B->NBlksFrst - LineLenLst, ' ');
				text += buffer;
			}
			//for (int i = 1; i <= B->NBlksFrst - LineLenLst; i++) {
			//	text += ' ';
			//}
		}
		ResetY();
		Y.Ln = B->NTxtLines;
		if (Y.Ln != 0) {
			Y.Blk = B;
			Y.P = B->lines[0].c_str();
			Y.ChkPg = ChkPg;
			LineLenLst = B->lineLength;
			Y.Sz = B->lines[0].length();
		}
	}
	RunAProc(B->BeforeProc);
	Print1NTupel(text, false);
	RunAProc(B->AfterProc);
}

void Print1NTupel(std::string& text, bool Skip)
{
	WORD L;
	double R = 0.0;
	pstring Mask;
	LongStr* S = nullptr;
	if (Y.Ln == 0) return;
	RFldD* RF = nullptr;
	auto reportFieldsIt = Y.Blk->ReportFields.begin();
label1:
	WasOutput = true;
	while (Y.I < Y.Sz) {
		char buffer[256]{ '\0' };
		BYTE C = (BYTE)Y.P[Y.I];
		if (C == 0xFF) {
			//if (RF == nullptr) RF = Y.Blk->RFD;
			//else RF = (RFldD*)RF->pChain;
			//if (RF == nullptr) return;
			if (RF == nullptr) {
				RF = *reportFieldsIt; // 1st time
			}
			else {
				++reportFieldsIt;
				if (reportFieldsIt == Y.Blk->ReportFields.end()) return;
				RF = *reportFieldsIt;
			}
			
			L = (BYTE)Y.P[Y.I + 1];
			WORD M = (BYTE)Y.P[Y.I + 2];
			if (RF->FrmlTyp == 'R') {
				if (!Skip) R = RunReal(RF->Frml);
				switch (RF->Typ) {
				case 'R':
				case 'F': {
					if (Skip) {
						snprintf(buffer, sizeof(buffer), "%*c", L, ' ');
						//printf("%s%*c", Rprt.c_str(), L, ' ');
						text += buffer;
					}
					else {
						if (RF->Typ == 'F') R = R / Power10[M];
						if (RF->BlankOrWrap && (R == 0)) {
							if (M == 0) {
								snprintf(buffer, sizeof(buffer), "%*c", L, ' ');
								//printf("%s%*c", Rprt.c_str(), L, ' ');
							}
							else {
								snprintf(buffer, sizeof(buffer), "%*c.%*c", L - M - 1, ' ', M, ' ');
								//printf("%s%*c.%*c", Rprt.c_str(), L - M - 1, ' ', M, ' ');
							}
							text += buffer;
						}
						else {
							snprintf(buffer, sizeof(buffer), "%*.*f", L, M, R);
							text += buffer;
							//printf("%s%*.*f", Rprt.c_str(), L, M, R);
						}
					}
					Y.I += 2;
					break;
				}
				case 'D': {
					if (RF->BlankOrWrap) Mask = "DD.MM.YYYY";
					else Mask = "DD.MM.YY";
					goto label2;
					break;
				}
				case 'T': {
					Mask = copy("hhhhhh", 1, L) + copy(":ss mm.tt", 1, M);
					Y.I += 2;
				label2:
					if (Skip) {
						snprintf(buffer, sizeof(buffer), "%*c", Mask.length(), ' ');
						//printf("%s%*c", Rprt.c_str(), Mask.length(), ' ');
						text += buffer;
					}
					else {
						snprintf(buffer, sizeof(buffer), "%s", StrDate(R, Mask).c_str());
						//printf("%s%s", Rprt.c_str(), StrDate(R, Mask).c_str());
						text += buffer;
					}
					break;
				}
				}
			}
			else {
				if (RF->Typ == 'P') {
					S = RunLongStr(RF->Frml);
					//printf("%s%c", Rprt.c_str(), 0x10);
					text += 0x10;
					for (WORD i = 0; i <= S->LL + 1; i++) {
						//printf("%s%c", Rprt.c_str(), *(char*)(S->A[i]));
						text += S->A[i];
					}
					ReleaseStore(S);
					goto label3;
				}
				Y.I += 2;
				if (Skip) {
					snprintf(buffer, sizeof(buffer), "%*c", L, ' ');
					//printf("%s%*c", Rprt.c_str(), L, ' ');
					text += buffer;
				}
				else
					switch (RF->FrmlTyp) {
					case 'S': {
						std::string S = RunStdStr(RF->Frml);
						S = TrailChar(S, ' ');
						text += NewTxtCol(S, M, L, RF->BlankOrWrap);
						break;
					}
					case 'B':
						if (RunBool(RF->Frml)) {
							//printf("%s%c", Rprt.c_str(), AbbrYes);
							text += AbbrYes;
						}
						else {
							//printf("%s%c", Rprt.c_str(), AbbrNo);
							text += AbbrNo;
						}
					}
			}
		}
		else {
			if ((C == '.') && (Y.I == 0) && FirstLines) WasDot = true;
			//printf("%s%c", Rprt.c_str(), (char)C);
			text += C;
		}
	label3:
		Y.I++;
	}
	PendingTT(text);
	Y.Ln--;
	if (Y.Ln > 0) {
		//Y.P += Y.Sz;
		Y.P = Y.Blk->lines[Y.Blk->NTxtLines - Y.Ln].c_str();
		NewLine(text);
		L = Y.Blk->lines[Y.Blk->NTxtLines - Y.Ln].length();
		//*(Y.P)++;
		//Y.Sz = *(WORD*)(Y.P);
		Y.Sz = Y.Blk->lines[Y.Blk->NTxtLines - Y.Ln].length();
		//*Y.P += 2;
		Y.I = 0;
		CheckPgeLimit(text);
		LineLenLst = L;
		// jedna se o posledni radek a je prazdny? -> pridame prazdny radek
		if (Y.Blk->NTxtLines - Y.Ln + 1 == Y.Blk->lines.size()) {
			if (Y.Blk->lines[Y.Blk->NTxtLines - Y.Ln].empty()) NewLine(text);
		}
		goto label1;
	}
	Y.Blk = nullptr;
}

void TruncLine(std::string& text)
{
	FinishTuple(text);
	if (LineLenLst > 0) NewLine(text);
}

void PrintBlkChn(BlkD* B, std::string& text, bool ChkPg, bool ChkLine)
{
	while (B != nullptr) {
		if (RunBool(B->Bool)) {
			if (ChkLine) {
				if (OutOfLineBound(B)) WasFF2 = true;
				if (B->FF1 || WasFF2) NewPage(text);
			}
			PrintTxt(B, text, ChkPg);
			WasFF2 = B->FF2;
		}
		B = B->pChain;
	}
}

void PrintPageFt(std::string& text)
{
	if (!FrstBlk) {
		bool b = WasFF2;
		TruncLine(text);
		WORD Ln = RprtLine;
		PrintBlkChn(PageFt, text, false, false);
		TruncLine(text);
		NoFF = RprtLine < Ln;
		PFZeroLst.clear();
		WasFF2 = b;
	}
}

void PrintPageHd(std::string& text)
{
	bool b = FrstBlk;
	if (!b) FormFeed(text);
	PrintBlkChn(PageHd, text, false, false);
	if (!b) PrintDH = 2;
}

void Footings(LvDescr* L, LvDescr* L2, std::string& text)
{
	while (L != nullptr) {
		PrintBlock(L->Ft, text, nullptr);
		if (L == L2) return;
		L = L->Chain;
	}
}

void Headings(LvDescr* L, LvDescr* L2, std::string& text)
{
	while ((L != nullptr) && (L != L2)) {
		PrintBlock(L->Hd, text, nullptr);
		L = L->ChainBack;
	}
}

void ReadInpFile(InpD* ID)
{
	CRecPtr = ID->ForwRecPtr;
label1:
	ID->Scan->GetRec();
	if (ID->Scan->eof) return;
	if (ESCPressed() && PromptYN(24)) {
		WasLPTCancel = true;
		GoExit();
	}
	RecCount++;
	RunMsgN(RecCount);
	if (!RunBool(ID->Bool)) goto label1;
}

void OpenInp()
{
	NRecsAll = 0;
	for (short i = 1; i <= MaxIi; i++) {
		CFile = IDA[i]->Scan->FD;
		if (IDA[i]->Scan->Kind == 5) IDA[i]->Scan->SeekRec(0);
		else {
			IDA[i]->Md = CFile->NewLockMode(RdMode);
			IDA[i]->Scan->ResetSort(IDA[i]->SK, IDA[i]->Bool, IDA[i]->Md, IDA[i]->SQLFilter);
		}
		NRecsAll += IDA[i]->Scan->NRecs;
	}
}

void CloseInp()
{
	for (WORD i = 1; i <= MaxIi; i++) {
		if (IDA[i]->Scan->Kind != 5) {
			IDA[i]->Scan->Close();
			ClearRecSpace(IDA[i]->ForwRecPtr);
			CFile->OldLockMode(IDA[i]->Md);
		}
	}
}

WORD CompMFlds(std::vector<ConstListEl>& C, KeyFldD* M, short& NLv)
{
	XString x;
	NLv = 0;
	for (ConstListEl& c : C) { 
		NLv++;
		x.Clear();
		x.StoreKF(M);
		std::string s = x.S;
		int res = CompStr(s, c.S);
		if (res != _equ) {
			return res;
		}
		M = M->pChain;
	}
	return _equ;
}

void GetMFlds(std::vector<ConstListEl>& C, KeyFldD* M)
{
	for (auto& c : C) {
		XString x;
		x.Clear();
		x.StoreKF(M);
		c.S = x.S;
		M = M->pChain;
	}
}

void MoveMFlds(std::vector<ConstListEl>& C1, std::vector<ConstListEl>& C2)
{
	for (size_t i = 0; i < C2.size(); i++) {
		// puvodne se kopiroval jen pstring z C1 do C2
		C2[i] = C1[i];
	}
}

void PutMFlds(KeyFldD* M)
{
	if (MinID == nullptr) return;
	FileD* cf = CFile;
	FileD* cf1 = MinID->Scan->FD;
	void* cr = CRecPtr;
	void* cr1 = MinID->ForwRecPtr;
	KeyFldD* m1 = MinID->MFld;
	while (M != nullptr) {
		FieldDescr* f = M->FldD;
		FieldDescr* f1 = m1->FldD;
		CFile = cf1;
		CRecPtr = cr1;
		switch (f->frml_type) {
		case 'S': {
				pstring s = _ShortS(f1);
				CFile = cf; CRecPtr = cr;
				CFile->saveS(f, s, CRecPtr);
				break;
			}
		case 'R': {
				double r = CFile->_R(f1, CRecPtr);
				CFile = cf; CRecPtr = cr;
				CFile->saveR(f, r, CRecPtr);
				break;
			}
		default: {
				bool b = CFile->loadB(f1, CRecPtr);
				CFile = cf; CRecPtr = cr;
				CFile->saveB(f, b, CRecPtr);
				break;
			}
		}
		M = M->pChain;
		m1 = m1->pChain;
	}
}

void GetMinKey()
{
	short i, nlv;
	short mini = 0; NEof = 0;
	for (i = 1; i <= MaxIi; i++) {
		CFile = IDA[i]->Scan->FD;
		if (IDA[i]->Scan->eof) NEof++;
		if (OldMFlds.empty()) {
			IDA[i]->Exist = !IDA[i]->Scan->eof;
			mini = 1;
		}
		else {
			CRecPtr = IDA[i]->ForwRecPtr;
			IDA[i]->Exist = false;
			if (!IDA[i]->Scan->eof) {
				WORD res;
				if (mini == 0) goto label1;
				res = CompMFlds(NewMFlds, IDA[i]->MFld, nlv);
				if (res != _gt) {
					if (res == _lt)
					{
					label1:
						GetMFlds(NewMFlds, IDA[i]->MFld);
						mini = i;
					}
					IDA[i]->Exist = true;
				}
			}
		}
	}
	if (mini > 0) {
		for (i = 1; i <= mini - 1; i++) {
			IDA[i]->Exist = false;
		}
		MinID = IDA[mini];
	}
	else {
		MinID = nullptr;
	}
}

void ZeroCount()
{
	for (short i = 1; i <= MaxIi; i++) {
		IDA[i]->Count = 0.0;
	}
}

LvDescr* GetDifLevel()
{
	//ConstListEl* C1 = NewMFlds;
	//ConstListEl* C2 = OldMFlds;
	KeyFldD* M = IDA[1]->MFld;
	LvDescr* L = LstLvM->ChainBack;
	size_t vIndex = 0;
	while (M != nullptr) {
		//if (C1->S != C2->S) {
		if (NewMFlds[vIndex].S != OldMFlds[vIndex].S) {
			return L;
		}
		//C1 = (ConstListEl*)C1->pChain;
		//C2 = (ConstListEl*)C2->pChain;
		vIndex++;
		M = (KeyFldD*)M->pChain;
		L = L->ChainBack;
	}
	return nullptr;
}

void MoveForwToRec(InpD* ID)
{
	/* !!! with ID^ do!!! */
	CFile = ID->Scan->FD;
	CRecPtr = CFile->FF->RecPtr;
	Move(ID->ForwRecPtr, CRecPtr, CFile->FF->RecLen + 1);
	ID->Count = ID->Count + 1;
	ChkD* C = ID->Chk;
	if (C != nullptr) {
		ID->Error = false;
		ID->Warning = false;
		ID->ErrTxtFrml->S[0] = 0;
		while (C != nullptr) {
			if (!RunBool(C->Bool)) {
				ID->Warning = true;
				ID->ErrTxtFrml->S = RunShortStr(C->TxtZ);
				if (!C->Warning) {
					ID->Error = true;
					return;
				}
			}
			C = (ChkD*)C->pChain;
		}
	}
}

void MoveFrstRecs()
{
	for (short i = 1; i <= MaxIi; i++) {
		if (IDA[i]->Exist) MoveForwToRec(IDA[i]);
		else {
			CFile = IDA[i]->Scan->FD;
			CRecPtr = CFile->FF->RecPtr;
			ZeroAllFlds(CFile, CRecPtr);
			PutMFlds(IDA[i]->MFld);
		}
	}
}

void MergeProc(std::string& text)
{
	short nlv = 0;
	short res = 0;
	for (short i = 1; i <= MaxIi; i++) {
		InpD* ID = IDA[i];
		/* !!! with ID^ do!!! */
		{ if (ID->Exist) {
			CFile = ID->Scan->FD;
			CRecPtr = CFile->FF->RecPtr;
			LvDescr* L = ID->LstLvS;
		label1:
			ZeroSumFlds(L);
			GetMFlds(ID->OldSFlds, ID->SFld);
			if (WasFF2) PrintPageHd(text);
			Headings(L, ID->FrstLvS, text);
			if (PrintDH == 0) PrintDH = 1;
		label2:
			PrintBlock(ID->FrstLvS->Ft, text, ID->FrstLvS->Hd); /*DE*/
			SumUp(ID->Sum);
			ReadInpFile(ID);
			if (ID->Scan->eof) goto label4;
			res = CompMFlds(NewMFlds, ID->MFld, nlv);
			if ((res == _lt) && (MaxIi > 1)) {
				SetMsgPar(ID->Scan->FD->Name);
				RunError(607);
			}
			if (res != _equ) goto label4;
			res = CompMFlds(ID->OldSFlds, ID->SFld, nlv);
			if (res == _equ) {
				MoveForwToRec(ID);
				goto label2;
			}
			L = ID->LstLvS;
			while (nlv > 1) {
				L = L->ChainBack;
				nlv--;
			}
			Footings(ID->FrstLvS->Chain, L, text);
			if (WasFF2) PrintPageFt(text);
			MoveForwToRec(ID);
			goto label1;
		label4:
			Footings(ID->FrstLvS->Chain, ID->LstLvS, text);
		}
		}
	}
}

bool RewriteRprt(RprtOpt* RO, WORD pageLimit, WORD& Times, bool& IsLPT1)
{
	auto result = false;
	WasLPTCancel = false;
	IsLPT1 = false;
	Times = 1;
	bool PrintCtrl = false;

	if (RO != nullptr) {
		PrintCtrl = RO->PrintCtrl;
		if (RO->Times != nullptr) Times = (WORD)RunInt(RO->Times);
	}

	if (RO == nullptr || RO->Path.empty() && RO->CatIRec == 0) {
		SetPrintTxtPath();
		PrintView = true;
		PrintCtrl = false;
	}
	else {
		if (!RO->Path.empty() && EquUpCase(RO->Path, "LPT1")) {
			CPath = "LPT1";
			CVol = "";
			IsLPT1 = true;
			result = ResetPrinter(pageLimit, 0, true, true) && RewriteTxt(CPath, &Rprt, false);
			return result;
		}
		SetTxtPathVol(RO->Path, RO->CatIRec);
	}
	TestMountVol(CPath[0]);
	if (!RewriteTxt(CPath, &Rprt, PrintCtrl)) {
		SetMsgPar(CPath);
		WrLLF10Msg(700 + HandleError);
		PrintView = false;
		return result;
	}
	if (Times > 1) {
		printf("%s.ti %1i\n", Rprt.c_str(), Times);
		Times = 1;
	}
	if (pageLimit != 72) {
		printf("%s.pl %i\n", Rprt.c_str(), pageLimit);
	}
	result = true;
	return result;
}
