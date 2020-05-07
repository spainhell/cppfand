#include "runrprt.h"

#include "common.h"
#include "kbdww.h"
#include "legacy.h"
#include "lexanal.h"
#include "memory.h"
#include "oaccess.h"
#include "obase.h"
#include "obaseww.h"
#include "recacc.h"
#include "runfrml.h"
#include "runmerg.h"
#include "wwmix.h"

void RunReport(RprtOpt* RO)
{
	wwmix ww;
	
	LvDescr* L; ExitRecord er;
	pstring* s = nullptr;
	WORD i; bool frst, isLPT1;
	WORD Times; bool ex, b; BlkD* BD; BlkD* RFb; LockMode md;
	if (SelQuest) /* !!! with IDA[1]^ do!!! */ {
		CFile = IDA[1]->Scan->FD; if (!ww.PromptFilter("", IDA[1]->Bool, s)) {
			PrintView = false; return;
		}
	}
	if (PgeLimitZ != nullptr) PgeLimit = RunInt(PgeLimitZ);
	else PgeLimit = spec.AutoRprtLimit;
	if (PgeSizeZ != nullptr) PgeSize = RunInt(PgeSizeZ);
	else PgeSize = spec.AutoRprtLimit + spec.CpLines;
	if (PgeSize < 2) PgeSize = 2;
	if ((PgeLimit > PgeSize) || (PgeLimit == 0)) PgeLimit = PgeSize - 1;
	if (! RewriteRprt(RO, PgeSize, Times, isLPT1)) return;
	MarkStore2(Store2Ptr); ex = true;
	PushProcStk();
	//NewExit(Ovr(), er);
	goto label3;
	OpenInp();
	MergOpGroup.Group = 1.0; frst = true; NLinesOutp = 0; PrintDH = 2;
label0:
	RunMsgOn('R', NRecsAll); RecCount = 0;
	for (i = 1; i < MaxIi; i++) {
		if (frst) frst = false;
		else IDA[i]->Scan->SeekRec(0); ReadInpFile(IDA[i]);
	}
	RprtPage = 1; RprtLine = 1; SetPage = false; FirstLines = true; WasDot = false;
	WasFF2 = false; NoFF = false; LineLenLst = 0; FrstBlk = true; ResetY();
	L = LstLvM; RFb = &LstLvM->Ft;
	ZeroSumFlds(L); GetMinKey(); ZeroCount(); MoveFrstRecs();
	if (RprtHd != nullptr) {
		if (RprtHd->FF1) FormFeed(); RprtPage = 1;
		PrintBlkChn(RprtHd, false, false);
		TruncLine(); if (WasFF2) FormFeed();
		if (SetPage) { SetPage = false; RprtPage = PageNo; }
		else RprtPage = 1;
	}
	if (NEof == MaxIi) goto label2;
label1:
	if (WasFF2) PrintPageHd(); Headings(L, nullptr);
	MergeProc();
	MoveMFlds(NewMFlds, OldMFlds); GetMinKey();
	if (NEof == MaxIi) {
		if (FrstLvM != LstLvM) Footings(FrstLvM, LstLvM->ChainBack);
	label2:
		WasFF2 = false; TruncLine(); PrintBlkChn(RFb, false, true); b = WasFF2;
		if ((PageFt != nullptr) && !PageFt->NotAtEnd) PrintPageFt();
		TruncLine(); RunMsgOff();
		if (Times > 1 /*only LPT1*/) {
			Times--; printf("%s%c", Rprt.c_str(), 0x0C); goto label0;
		}
		if (b) FormFeed();
		ex = false;
	label3:
		RestoreExit(er);
		if (PrintView && (NLinesOutp == 0) && (LineLenLst == 0)) {
			RdMsg(159);
			printf("%s\n", Rprt.c_str());
			printf("%s%s", Rprt.c_str(), MsgLine.c_str());
		}
		Rprt.Close(); if (isLPT1) ClosePrinter(0);
		CloseInp(); PopProcStk();
		if (ex) { RunMsgOff(); if (!WasLPTCancel) GoExit(); }
		return;
	}
	L = GetDifLevel();
	Footings(FrstLvM, L); if (WasFF2) PrintPageFt();
	ZeroSumFlds(L); ZeroCount();
	MoveFrstRecs(); MergOpGroup.Group = MergOpGroup.Group + 1.0;
	goto label1;
}

void ResetY()
{
	FillChar(&Y, sizeof(Y), 0);
}

void IncPage()
{
	if (SetPage) { SetPage = false; RprtPage = PageNo; }
	else RprtPage++;
}

void NewLine()
{
	printf("%s\n", Rprt.c_str());
	if (WasDot) WasDot = false;
	else {
		RprtLine++; NLinesOutp++; FirstLines = false;
	}
	LineLenLst = 0;
	if (RprtLine > PgeSize) { RprtLine -= PgeSize; IncPage(); }
}

void FormFeed()
{
	integer I;
	if (NoFF) NoFF = false;
	else { printf("%s%c", Rprt.c_str(), 0x0C); RprtLine = 1; IncPage(); }
	LineLenLst = 0; WasFF2 = false;
}

void NewPage()
{
	PrintPageFt(); PrintPageHd(); TruncLine(); FrstBlk = false;
}

bool OutOfLineBound(BlkD* B)
{
	return ((B->LineBound != nullptr) && (RprtLine > RunReal(B->LineBound))
		|| B->AbsLine && (RunInt(B->LineNo) < RprtLine));
}

void Zero(FloatPtrList Z)
{
	while (Z != nullptr) { *Z->RPtr = 0; Z = Z->Chain; };
}

void WriteNBlks(integer N)
{
	if (N > 0) printf("%s%*c", Rprt.c_str(), N, ' ');
}

void NewTxtCol(LongStrPtr S, WORD Col, WORD Width, bool Wrap)
{
	bool Absatz;
	CharArrPtr TA = nullptr; WORD i, LL, Ln; TTD* TD = nullptr;
	pstring ss; StringList SL;
	LL = S->LL; TA = (CharArrPtr)(&S->A); Ln = 0; Absatz = true;
	if (Wrap) for (i = 1; i < LL; i++)
		if ((*TA[i] == 0x0D) && ((i == LL) || (*TA[i + 1] != 0x0A))) *TA[i] = ' ';
	ss = GetLine(TA, LL, Width, Wrap, Absatz);
	printf("%s%s", Rprt.c_str(), ss.c_str());
	while (LL > 0) {
		ss = GetLine(TA, LL, Width, Wrap, Absatz); Ln++;
		if (Ln == 1) {
			TD = (TTD*)GetStore2(sizeof(*TD)); TD->SL = nullptr;
			TD->Col = Col; TD->Width = Width;
		}
		SL = (StringListEl*)GetStore2(ss.length() + 5); SL->S = ss;
		ChainLast(TD->SL, SL);
	}
	if (Ln > 0) {
		TD->Ln = Ln; ChainLast(Y.TD, TD); if (Ln > Y.TLn) Y.TLn = Ln;
	}

}

pstring GetLine(CharArr* TA, WORD& TLen, WORD Width, bool Wrap, bool Absatz)
{
	char CharArr0[11];
	char* T = nullptr;
	WORD* TAOff = (WORD*)TA;
	integer i, i2, j, l2, w, n, n1, n2, nWords, iWrdEnd, i2WrdEnd, wWrdEnd, nWrdEnd;
	bool WasWrd, Fill; pstring s; pstring s1; char c;
	i = 0;  i2 = 0; w = 0;

	T = (char*)TA;  WasWrd = false;  nWords = 0; Fill = false;
	while ((i < TLen) && (i2 < 255)) {
		c = T[i];
		if (c == 0x0D) { Absatz = true; goto label1; }
		if ((w >= Width) && (c == ' ') && Wrap) goto label1;
		if ((c != ' ') || WasWrd || !Wrap || Absatz)
		{
			i2++; s[i2] = c;  if (!IsPrintCtrl(c)) w++;
		}
		if (c == ' ') {
			if (WasWrd)
			{
				WasWrd = false; i2WrdEnd = i2 - 1; iWrdEnd = i;
				wWrdEnd = w - 1; nWrdEnd = nWords;
			}
		}
		else if (!WasWrd) { WasWrd = true; Absatz = false; nWords++; }
		i++;
	}
label1:
	if (Wrap && (nWords >= 2) && (w > Width))
	{
		i2 = i2WrdEnd; i = iWrdEnd;
		w = wWrdEnd; nWords = nWrdEnd; Fill = true; Absatz = false;
	}
	if ((i < TLen) && (T[i] == 0x0D))
	{
		i++;  if ((i < TLen) && (T[i] == 0x0A)) i++;
	}
	TAOff += i; TLen -= i;
	l2 = i2; if (w < Width) l2 += (Width - w); n = l2 - i2;
	if ((nWords <= 1) || (n == 0) || !Fill)
	{
		FillChar(&s[i2 + 1], n, ' '); s[0] = char(l2);
	}
	else {
		n1 = n / (nWords - 1); n2 = n % (nWords - 1);
		s[0] = char(i2); s1 = s; s[0] = char(l2); i2 = 1; WasWrd = false;
		for (i = 1; i < s1.length(); i++) {
			s[i2] = s1[i];
			if (s[i2] != ' ') WasWrd = true;
			else if (WasWrd) {
				WasWrd = false; for (j = 1; j < n1; j++) { i2++; s[i2] = ' '; }
				if (n2 > 0) { n2--; i2++; s[i2] = ' '; };
			}
			i2++;
		}
		;
	}
	return s;
}

void CheckPgeLimit()
{
	void* p2; YRec YY;
	if (Y.ChkPg && (RprtLine > PgeLimit) && (PgeLimit < PgeSize)) {
		p2 = Store2Ptr; MarkStore2(Store2Ptr);
		YY = Y; ResetY(); NewPage(); Y = YY; Store2Ptr = p2;
	}
}

void PendingTT()
{
	TTD* TD; WORD Col, l, lll; StringList SL;
	lll = LineLenLst; Col = LineLenLst + 1;
	while (Y.TLn > 0) {
		NewLine(); CheckPgeLimit(); TD = Y.TD; Col = 1;
		while (TD != nullptr) {
			if (TD->Ln > 0) {
				WriteNBlks(TD->Col - Col); SL = TD->SL;
				printf("%s%s", Rprt.c_str(), SL->S.c_str());
				l = LenStyleStr(SL->S); Col = TD->Col + l; TD->Ln--;
				TD->SL = SL->Chain;
			}
			TD = TD->Chain;
		}
		Y.TLn--;
		LineLenLst = lll;
	}
	WriteNBlks(LineLenLst + 1 - Col);
	if (Y.TD != nullptr) { ReleaseStore2(Store2Ptr); Y.TD = nullptr; Y.TLn = 0; }
}

void Print1NTupel(bool Skip)
{
	struct REditD { BYTE Char = 0; BYTE L = 0; BYTE M = 0; };
	REditD* RE;
	RFldD* RF;
	WORD L, M, i; BYTE C; double R; pstring Mask; LongStr* S;
	if (Y.Ln == 0) return; RF = (RFldD*)(&Y.Blk->RFD);
label1:
	WasOutput = true;
	while (Y.I < Y.Sz) {
		RE = (REditD*)(&Y.P[Y.I]); C = RE->Char;
		if (C == 0xFF) {
			RF = RF->Chain; if (RF == nullptr) return; L = RE->L; M = RE->M;
			if (RF->FrmlTyp == 'R') {
				if (!Skip) R = RunReal(RF->Frml);
				switch (RF->Typ) {
				case 'R':
				case 'F': {
					if (Skip) printf("%s%*c", Rprt.c_str(), L, ' ');
					else {
						if (RF->Typ == 'F') R = R / Power10[M];
						if (RF->BlankOrWrap && (R == 0))
							if (M == 0) printf("%s%*c", Rprt.c_str(), L, ' ');
							else printf("%s%*c.%*c", Rprt.c_str(), L - M - 1, ' ', M, ' ');
						else printf("%s%*.*f", Rprt.c_str(), L, M, R);
					}
					Y.I += 2;
					break;
				}
				case 'D': {
					if (RF->BlankOrWrap) Mask = "DD.MM.YYYY";
					else Mask = "DD.MM.YY"; goto label2; break;
				}
				case 'T': {
					Mask = copy("hhhhhh", 1, L) + copy(":ss mm.tt", 1, M);
					Y.I += 2;
				label2:
					if (Skip) printf("%s%*c", Rprt.c_str(), Mask.length(), ' ');
					else printf("%s%s", Rprt.c_str(), StrDate(R, Mask).c_str());
					break;
				}
				}
			}
			else {
				if (RF->Typ == 'P') {
					S = RunLongStr(RF->Frml); printf("%s%c", Rprt.c_str(), 0x10);
					for (i = 0; i < S->LL + 1; i++)
						printf("%s%c", Rprt.c_str(), *(char*)(S->A[i]));
					ReleaseStore(S);
					goto label3;
				}
				Y.I += 2;
				if (Skip) printf("%s%*c", Rprt.c_str(), L, ' ');
				else switch (RF->FrmlTyp) {
				case 'S': {
					S = RunLongStr(RF->Frml);
					while ((S->LL > 0) && (S->A[S->LL] == ' ')) S->LL--;
					NewTxtCol(S, M, L, RF->BlankOrWrap); ReleaseStore(S); break;
				}
				case 'B':
					if (RunBool(RF->Frml)) printf("%s%c", Rprt.c_str(), AbbrYes);
					else printf("%s%c", Rprt.c_str(), AbbrNo);
				}
			}
		}
		else {
			if ((C == '.') && (Y.I == 0) && FirstLines) WasDot = true;
			printf("%s%c", Rprt.c_str(), (char)C);
		}
	label3:
		Y.I++;
	}
	PendingTT(); Y.Ln--; if (Y.Ln > 0) {
		Y.P += Y.Sz; NewLine();
		L = *(Y.P); *(Y.P)++; Y.Sz = *(WORD*)(Y.P); *Y.P += 2; Y.I = 0;
		CheckPgeLimit(); LineLenLst = L; goto label1;
	}
	Y.Blk = nullptr;
}

void FinishTuple()
{
	while (Y.Blk != nullptr) Print1NTupel(true);
}

void RunAProc(AssignD* A)
{
	while (A != nullptr) {
		/* !!! with A^ do!!! */
		switch (A->Kind) {
		case _locvar: LVAssignFrml(A->LV, MyBP, A->Add, A->Frml); break;
		case _parfile: AsgnParFldFrml(A->FD, A->PFldD, A->Frml, A->Add); break;
		case _ifthenelseM: {
			if (RunBool(A->Bool)) RunAProc(A->Instr);
			else RunAProc(A->ElseInstr);
			break;
		}
		}
		A = A->Chain;
	}
}

void PrintTxt(BlkD* B, bool ChkPg)
{
	integer I;
	if (B == nullptr) return;
	if (B->SetPage) {
		PageNo = RunInt(B->PageNo); SetPage = true;
	}
	if (B != Y.Blk) {
		FinishTuple();
		if (B->AbsLine) for (I = RprtLine; I < RunInt(B->LineNo) - 1; I++) NewLine();
		if (B->NTxtLines > 0) {
			if (B->NBlksFrst < LineLenLst) NewLine();
			for (I = 1; I < B->NBlksFrst - LineLenLst; I++) printf("%s%c", Rprt.c_str(), ' ');
		}
		ResetY(); Y.Ln = B->NTxtLines; if (Y.Ln != 0) {
			Y.Blk = B; Y.P = B->Txt; Y.ChkPg = ChkPg;
			LineLenLst = *(Y.P); Y.P++; Y.Sz = *(WORD*)(Y.P); Y.P += 2;
		}
	}
	RunAProc(B->BeforeProc); Print1NTupel(false); RunAProc(&B->AfterProc);
}

void TruncLine()
{
	FinishTuple(); if (LineLenLst > 0) NewLine();
}

void PrintBlkChn(BlkD* B, bool ChkPg, bool ChkLine)
{
	while (B != nullptr) {
		if (RunBool(B->Bool)) {
			if (ChkLine) {
				if (OutOfLineBound(B)) WasFF2 = true;
				if (B->FF1 || WasFF2) NewPage();
			}
			PrintTxt(B, ChkPg); WasFF2 = B->FF2;
		}
		B = B->Chain;
	}
}

void PrintPageFt()
{
	WORD Ln; bool b;
	if (!FrstBlk) {
		b = WasFF2; TruncLine(); Ln = RprtLine;
		PrintBlkChn(PageFt, false, false); TruncLine();
		NoFF = RprtLine < Ln; Zero(PFZeroLst); WasFF2 = b;
	}
}

void PrintPageHd()
{
	bool b;
	b = FrstBlk; if (!b) FormFeed(); PrintBlkChn(PageHd, false, false);
	if (!b) PrintDH = 2;
}

void SumUp(SumElem* S)
{
	while (S != nullptr) {
		S->R = S->R + RunReal(S->Frml); S = S->Chain;
	}
}

void PrintBlock(BlkD* B, BlkD* DH)
{
	WORD LAfter; BlkD* B1; bool pdh;
	pdh = false;
	while (B != nullptr) {
		if (RunBool(B->Bool)) {
			if (B != Y.Blk) {
				if ((B->NTxtLines > 0) && (B->NBlksFrst < LineLenLst)) TruncLine();
				if (OutOfLineBound(B)) WasFF2 = true;
				LAfter = RprtLine + MaxI(0, B->NTxtLines - 1);
				if ((DH != nullptr) && (PrintDH >= DH->DHLevel + 1)) {
					B1 = DH; while (B1 != nullptr) {
						if (RunBool(B1->Bool)) LAfter += B1->NTxtLines; B1 = B1->Chain;
					};
				}
				if (B->FF1 || WasFF2 || FrstBlk && (B->NTxtLines > 0) ||
					(PgeLimit < PgeSize) && (LAfter > PgeLimit)) NewPage();
				if ((DH != nullptr) && (PrintDH >= DH->DHLevel + 1)) {
					PrintBlkChn(DH, true, false); PrintDH = 0;
				}
			}
			WasOutput = false;
			PrintTxt(B, true);
			WasFF2 = B->FF2;
			if ((DH == nullptr) && WasOutput) pdh = true;
			SumUp(B->Sum);
		}
		B = B->Chain;
	}
	if (pdh) PrintDH = 2;
}

void Footings(LvDescr* L, LvDescr* L2)
{
	while (L != nullptr) { PrintBlock(&L->Ft, nullptr); if (L == L2) return; L = L->Chain; }
}

void Headings(LvDescr* L, LvDescr* L2)
{
	while ((L != nullptr) && (L != L2)) { PrintBlock(L->Hd, nullptr); L = L->ChainBack; }
}

void ZeroSumFlds(LvDescr* L)
{
	while (L != nullptr) { Zero(L->ZeroLst); L = L->ChainBack; }
}

void ReadInpFile(InpD* ID)
{
	/* !!! with ID^ do!!! */
	CRecPtr = ID->ForwRecPtr;
label1:
	ID->Scan->GetRec(); if (ID->Scan->eof) return;
	if (ESCPressed() && PromptYN(24)) {
		WasLPTCancel = true; GoExit();
	}
	RecCount++; RunMsgN(RecCount);
	if (!RunBool(ID->Bool)) goto label1;
}

void OpenInp()
{
	NRecsAll = 0;
	for (integer i = 1; i < MaxIi; i++) {
		/* !!! with IDA[i]^ do!!! */
		CFile = IDA[i]->Scan->FD;
		if (IDA[i]->Scan->Kind == 5) IDA[i]->Scan->SeekRec(0);
		else {
			IDA[i]->Md = NewLMode(RdMode);
			IDA[i]->Scan->ResetSort(IDA[i]->SK, IDA[i]->Bool, IDA[i]->Md, IDA[i]->SQLFilter);
		}
		NRecsAll += IDA[i]->Scan->NRecs;
	}
}

void CloseInp()
{
	WORD i;
	for (i = 1; i < MaxIi; i++) {
		/* !!! with IDA[i]^ do!!! */
		if (IDA[i]->Scan->Kind != 5) {
			IDA[i]->Scan->Close(); ClearRecSpace(IDA[i]->ForwRecPtr); OldLMode(IDA[i]->Md);
		}
	}
}

WORD CompMFlds(ConstListEl* C, KeyFldD* M, integer& NLv)
{
	integer res = 0; XString x;
	NLv = 0;
	while (C != nullptr) {
		NLv++; x.Clear(); x.StoreKF(M);
		res = CompStr(x.S, C->S);
		if (res != _equ) { return res; }
		C = C->Chain; M = M->Chain;
	}
	return _equ;
}

void GetMFlds(ConstListEl* C, KeyFldD* M)
{
	XString* x;
	while (C != nullptr) {
		x = (XString*)(&C->S); x->Clear(); x->StoreKF(M); C = C->Chain; M = M->Chain;
	}
}

void MoveMFlds(ConstListEl* C1, ConstListEl* C2)
{
	while (C2 != nullptr) {
		C2->S = C1->S; C1 = C1->Chain; C2 = C2->Chain;
	}
}

void PutMFlds(KeyFldDPtr M)
{
	FieldDPtr f, f1; FileD* cf; FileD* cf1; void* cr; void* cr1; KeyFldD* m1;
	pstring s; double r; bool b;
	if (MinID == nullptr) return;
	cf = CFile; cf1 = MinID->Scan->FD; cr = CRecPtr; cr1 = MinID->ForwRecPtr;
	m1 = MinID->MFld;
	while (M != nullptr) {
		f = M->FldD; f1 = m1->FldD; CFile = cf1; CRecPtr = cr1;
		switch (f->FrmlTyp) {
		case 'S': { s = _ShortS(f1); CFile = cf; CRecPtr = cr; S_(f, s); break; }
		case 'R': { r = _R(f1); CFile = cf; CRecPtr = cr; R_(f, r); break; }
		default: b = _B(f1); CFile = cf; CRecPtr = cr; B_(f, b); break;
		}
		M = M->Chain; m1 = m1->Chain;
	}
}

void GetMinKey()
{
	integer i, nlv, mini, res;
	mini = 0; NEof = 0;
	for (i = 1; i < MaxIi; i++) {
		/* !!! with IDA[i]^ do!!! */
		CFile = IDA[i]->Scan->FD; if (IDA[i]->Scan->eof) NEof++;
		if (OldMFlds == nullptr) { IDA[i]->Exist = !IDA[i]->Scan->eof; mini = 1; }
		else {
			CRecPtr = IDA[i]->ForwRecPtr; IDA[i]->Exist = false;
			if (!IDA[i]->Scan->eof) {
				if (mini == 0) goto label1;
				res = CompMFlds(NewMFlds, IDA[i]->MFld, nlv);
				if (res != _gt) {
					if (res == _lt)
					{
					label1:
						GetMFlds(NewMFlds, IDA[i]->MFld); mini = i;
					}
					IDA[i]->Exist = true;
				}
			}
		}
	}
	if (mini > 0) {
		for (i = 1; i < mini - 1; i++) IDA[i]->Exist = false;
		MinID = IDA[mini];
	}
	else MinID = nullptr;
}

void ZeroCount()
{
	integer i;
	for (i = 1; i < MaxIi; i++) IDA[i]->Count = 0.0;
}

LvDescr* GetDifLevel()
{
	ConstListEl* C1; ConstListEl* C2; KeyFldDPtr M; LvDescr* L;
	C1 = NewMFlds; C2 = OldMFlds; M = IDA[1]->MFld; L = LstLvM->ChainBack;
	while (M != nullptr) {
		if (C1->S != C2->S) { return L; }
		C1 = C1->Chain; C2 = C2->Chain; M = M->Chain; L = L->ChainBack;
	}
	return nullptr;
}

void MoveForwToRec(InpD* ID)
{
	ChkDPtr C;
	/* !!! with ID^ do!!! */
	CFile = ID->Scan->FD; CRecPtr = CFile->RecPtr;
	Move(ID->ForwRecPtr, CRecPtr, CFile->RecLen + 1);
	ID->Count = ID->Count + 1;
	C = ID->Chk; if (C != nullptr) {
		ID->Error = false; ID->Warning = false; ID->ErrTxtFrml->S[0] = 0;
		while (C != nullptr) {
			if (!RunBool(C->Bool)) {
				ID->Warning = true; ID->ErrTxtFrml->S = RunShortStr(C->TxtZ);
				if (!C->Warning) { ID->Error = true; return; };
			}
			C = C->Chain;
		}
	}
}

void MoveFrstRecs()
{
	integer i;
	for (i = 1; i < MaxIi; i++) {
		/* !!! with IDA[i]^ do!!! */
		if (IDA[i]->Exist) MoveForwToRec(IDA[i]);
		else {
			CFile = IDA[i]->Scan->FD; CRecPtr = CFile->RecPtr; ZeroAllFlds();
			PutMFlds(IDA[i]->MFld);
		}
	}
}

void MergeProc()
{
	integer i, res, nlv; LvDescr* L;
	for (i = 1; i < MaxIi; i++) {
		InpD* ID = IDA[i];
		/* !!! with ID^ do!!! */
		{ if (ID->Exist) {
			CFile = ID->Scan->FD; CRecPtr = CFile->RecPtr; L = ID->LstLvS;
		label1:
			ZeroSumFlds(L); GetMFlds(ID->OldSFlds, ID->SFld);
			if (WasFF2) PrintPageHd(); Headings(L, ID->FrstLvS);
			if (PrintDH == 0) PrintDH = 1;
		label2:
			PrintBlock(&ID->FrstLvS->Ft, ID->FrstLvS->Hd); /*DE*/
			SumUp(ID->Sum);
			ReadInpFile(ID);
			if (ID->Scan->eof) goto label4;
			res = CompMFlds(NewMFlds, ID->MFld, nlv);
			if ((res == _lt) && (MaxIi > 1)) {
				SetMsgPar(ID->Scan->FD->Name); RunError(607);
			}
			if (res != _equ) goto label4;
			res = CompMFlds(ID->OldSFlds, ID->SFld, nlv);
			if (res == _equ) { MoveForwToRec(ID); goto label2; }
			L = ID->LstLvS;
			while (nlv > 1) { L = L->ChainBack; nlv--; }
			Footings(ID->FrstLvS->Chain, L);
			if (WasFF2) PrintPageFt();
			MoveForwToRec(ID);
			goto label1;
		label4:
			Footings(ID->FrstLvS->Chain, ID->LstLvS);
		}
		}
	}
}

bool RewriteRprt(RprtOpt* RO, WORD Pl, WORD& Times, bool& IsLPT1)
{
	bool PrintCtrl;
	auto result = false;
	result = false; WasLPTCancel = false; IsLPT1 = false; result = false;
	Times = 1; PrintCtrl = RO->PrintCtrl;
	if ((RO != nullptr) && (RO->Times != nullptr)) Times = RunInt(RO->Times);
	if ((RO == nullptr) || (RO->Path == nullptr) && (RO->CatIRec == 0))
	{
		SetPrintTxtPath(); PrintView = true; PrintCtrl = false;
	}
	else {
		if (SEquUpcase(*RO->Path, "LPT1"))
		{
			CPath = "LPT1"; CVol = ""; IsLPT1 = true;
			result = ResetPrinter(Pl, 0, true, true) && RewriteTxt(&Rprt, false);
			return result;
		}
		SetTxtPathVol(*RO->Path, RO->CatIRec);
	}
	TestMountVol(CPath[1]);
	if (!RewriteTxt(&Rprt, PrintCtrl))
	{
		SetMsgPar(CPath); WrLLF10Msg(700 + HandleError);
		PrintView = false; return result;
	}
	if (Times > 1) { printf("%s.ti %1i\n", Rprt.c_str(), Times); Times = 1; }
	if (Pl != 72) printf("%s.pl %i\n", Rprt.c_str(), Pl);
	result = true;
}




