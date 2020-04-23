#include "editor.h"

#include "kbdww.h"
#include "legacy.h"
#include "memory.h"
#include "printtxt.h"
#include "common.h"
#include "obaseww.h"
#include "rdrun.h"
#include "runfrml.h"

longint SavePar()
{
	LongStr* sp; WORD len;
	len = InterfL + ofs(T) - ofs(Mode) + 4;
	sp = (LongStr*)GetStore(len + 2);
	sp->LL = len;
	Move(&Insert, &sp->A, InterfL);
	Move(&Mode, &sp->A[InterfL + 1], len - InterfL);
	auto result = StoreInTWork(sp);
	ReleaseStore(sp);
	return result;
}

void RestorePar(longint l)
{
	LongStr* sp;
	sp = ReadDelInTWork(l);
	Move(sp->A, &Insert, InterfL);
	Move(&sp->A[InterfL + 1], &Mode, sp->LL - InterfL);
	ReleaseStore(sp);
}

void SetEditTxt(Instr* PD)
{
	if (PD->Insert != nullptr) Insert = !runfrml::RunBool(PD->Insert);
	if (PD->Indent != nullptr) Indent = runfrml::RunBool(PD->Indent);
	if (PD->Wrap != nullptr) Wrap = runfrml::RunBool(PD->Wrap);
	if (PD->Just != nullptr) Just = runfrml::RunBool(PD->Just);
	if (PD->ColBlk != nullptr) TypeB = runfrml::RunBool(PD->ColBlk);
	if (PD->Left != nullptr) LeftMarg = maxi(1, runfrml::RunInt(PD->Left));
	if (PD->Right != nullptr) RightMarg = maxi(LeftMarg, mini(255, runfrml::RunInt(PD->Right)));
}

void GetEditTxt(bool& pInsert, bool& pIndent, bool& pWrap, bool& pJust, bool& pColBlk, integer& pLeftMarg,
	integer& pRightMarg)
{
	pInsert = Insert; pIndent = Indent; pWrap = Wrap; pJust = Just; pColBlk = TypeB;
	pLeftMarg = LeftMarg; pRightMarg = RightMarg;
}

bool EditText(char pMode, char pTxtType, pstring pName, pstring pErrMsg, CharArr* pTxtPtr, WORD pMaxLen, WORD& pLen,
	WORD& pInd, longint pScr, pstring pBreaks, EdExitDPtr pExD, bool& pSrch, bool& pUpdat, WORD pLastNr,
	WORD pCtrlLastNr, MsgStrPtr pMsgS)
{
	bool oldEdOK;
	oldEdOK = EdOk; EditT = true;
	Mode = pMode; TypeT = pTxtType; NameT = pName; ErrMsg = pErrMsg;
	T = pTxtPtr; MaxLenT = pMaxLen;
	LenT = pLen; IndT = pInd;
	ScrT = pScr & 0xFFFF;
	Posi = pScr >> 16;
	Breaks = pBreaks;
	ExitD = pExD;
	SrchT = pSrch; UpdatT = pUpdat;
	LastNr = pLastNr; CtrlLastNr = pCtrlLastNr;
	if (pMsgS != nullptr)
	{
		LastS = pMsgS->Last; CtrlLastS = pMsgS->CtrlLast;
		ShiftLastS = pMsgS->ShiftLast; AltLastS = pMsgS->AltLast;
		HeadS = *pMsgS->Head;
	}
	else
	{
		LastS = nullptr; CtrlLastS = nullptr; ShiftLastS = nullptr; AltLastS = nullptr;
		HeadS = nullptr;
	}
	if (Mode != HelpM) TxtColor = TextAttr;
	FirstEvent = !SrchT;
	if (SrchT)
	{
		SrchT = false;
		KbdBuffer = *l + KbdBuffer;
		KbdChar = _L_;
		IndT = 0;
	}

	Edit;
	if (Mode != HelpM) TextAttr = TxtColor;
	pUpdat = UpdatT; pSrch = SrchT; pLen = LenT;
	pInd = IndT; pScr = ScrT + longint(Posi) << 16;
	EdOk = oldEdOK;
	return EditT;
}

void SimpleEditText(char pMode, pstring pErrMsg, pstring pName, CharArr* TxtPtr, WORD MaxLen, WORD& Len, WORD& Ind,
	bool& Updat)
{
	bool Srch; longint Scr;
	Srch = false; Scr = 0;
	EditText(pMode, LocalT, pName, pErrMsg, TxtPtr, MaxLen, Len, Ind, Scr, "", nullptr, Srch, Updat, 0, 0, nullptr);
}

WORD FindText(const pstring& Pstr, pstring Popt, CharArr* PTxtPtr, WORD PLen)
{
	CharArrPtr tt = T; T = PTxtPtr;
	pstring f = FindStr; pstring o = OptionStr;
	bool r = Replace;
	FindStr = Pstr; OptionStr = Popt; Replace = false;
	WORD I = 1;
	WORD result;
	if (FindString(I, PLen + 1)) result = I;
	else result = 0;
	FindStr = f; OptionStr = o; Replace = r; T = tt;
	return result;
}

void EditTxtFile(longint* LP, char Mode, pstring& ErrMsg, EdExitDPtr ExD,
	longint TxtPos, longint Txtxy, WRect* V,
	WORD Atr, const pstring Hd, BYTE WFlags, MsgStrPtr MsgS)
{
	bool Srch, Upd; longint Size, L;
	longint w1;
	ExitRecord er;
	bool Loc;
	WORD Ind, oldInd;
	longint oldTxtxy;
	LongStr* LS; pstring compErrTxt;

	if (Atr == 0) Atr = colors.tNorm;
	longint w2 = 0; longint w3 = 0;
	if (V != nullptr) {
		w1 = PushW1(1, 1, TxtCols, 1, (WFlags & WPushPixel) != 0, false);
		w2 = PushW1(1, TxtRows, TxtCols, TxtRows, (WFlags & WPushPixel) != 0, false);
		w3 = PushWFramed(V->C1, V->R1, V->C2, V->R2, Atr, Hd, "", WFlags);
	}
	else
	{
		w1 = PushW(1, 1, TxtCols, TxtRows);
		TextAttr = Atr;
	}
	NewExit(Ovr, er);
	goto label4;
	Loc = (LP != nullptr);
	LocalPPtr = LP;
	if (!Loc)
	{
		MaxLenT = 0xFFF0; LenT = 0; Part.UpdP = false;
		TxtPath = CPath; TxtVol = CVol; OpenTxtFh(Mode);
		RdFirstPart;
		SimplePrintHead;
		while ((TxtPos > Part.PosP + Part.LenP) && !AllRd) RdNextPart;
		Ind = TxtPos - Part.PosP;
	}
	else {
		LS = TWork.Read(1, *LP);
		Ind = TxtPos; L = StoreInTWork(LS);
	}
	oldInd = Ind; oldTxtxy = Txtxy;

label1:
	Srch = false; Upd = false;
	if (!Loc)
		EditText(Mode, FileT, TxtPath, ErrMsg, T, 0xFFF0, LenT, Ind, Txtxy,
			_F1 + _F6 + _F9 + _AltF10, ExD,
			Srch, Upd, 126, 143, MsgS);
	else EditText(Mode, LocalT, "", ErrMsg, &LS->A, MaxLStrLen, LS->LL, Ind, Txtxy,
		_F1 + _F6, ExD, Srch, Upd, 126, 143, MsgS);
	TxtPos = Ind + Part.PosP;
	if (Upd) EdUpdated = true;
	if ((KbdChar == _AltEqual_) || (KbdChar == _U_))
	{
		ReleaseStore(LS); LS = TWork.Read(1, L);
		if (KbdChar == _AltEqual_) { KbdChar = _ESC_; goto label4; }
		else { Ind = oldInd; Txtxy = oldTxtxy; goto label1; }
	}
	if (!Loc) ReleaseStore(T);
	if (EdBreak == 0xFFFF)
		switch (KbdChar) {
		case _F9_: {
			if (Loc) { TWork.Delete(*LP); *LP = StoreInTWork(LS); }
			else RdPart;
			goto label1;
		}
		case _AltF10_: { Help(nullptr, "", false); goto label2; }
		case _F1_: {
			RdMsg(6); Help(RdbDPtr(HelpFD), MsgLine, false);
		label2:
			if (!Loc) RdPart; goto label1; }
		}
	if (!Loc) { Size = FileSizeH(TxtFh); CloseH(TxtFh); }
	if ((EdBreak == 0xFFFF) && (KbdChar = _F6_))
		if (Loc) { PrintArray(*T, LenT, false); goto label1; }
		else {
			CPath = TxtPath; CVol = TxtVol; PrintTxtFile(0);
			OpenTxtFh(Mode); RdPart; goto label1;
		}
	if (!Loc && (Size < 1)) DeleteFile(TxtPath);
	if (Loc and (KbdChar == _ESC_)) LS->LL = LenT;
label4:
	if (IsCompileErr) {
		IsCompileErr = false;
		compErrTxt = MsgLine;
		SetMsgPar(compErrTxt); WrLLF10Msg(110);
	}
	if (Loc)
	{
		TWork.Delete(L);
		TWork.Delete(*LP); *LP = StoreInTWork(LS);
		ReleaseStore(LS);
	}
	if (w3 != 0) PopW2(w3, (WFlags & WNoPop) == 0); if (w2 != 0) PopW(w2);
	PopW(w1);
	LastTxtPos = Ind + Part.PosP;
	RestoreExit(er);
}

void ViewPrinterTxt()
{
	WRect V = { 1, 2, 80, 24 };
	if (!PrintView) return;
	SetPrintTxtPath;
	V.C2 = TxtCols; V.R2 = TxtRows - 1;
	pstring temp(1);
	EditTxtFile(nullptr, 'T', temp, nullptr, 1, 0, &V, 0, "", WPushPixel, nullptr);
}

void Help(RdbDPtr R, pstring Name, bool InCWw)
{
	void* p; ExitRecord er; FileDPtr fd;
	WORD c1, c2, r1, r2; longint w, w2; WORD i, l, l2; WORD iRec, oldIRec;
	LongStr* s; LongStr* s2;
	WORD* os = (WORD*)s; WORD* os2 = (WORD*)s2;
	integer delta; bool frst, byName, backw;
	FileDPtr cf, cf2;

	if (R == nullptr) {
		if (iStk == 0) return; R = Stk[iStk].Rdb; backw = true;
	}
	else { if (Name == "") return; backw = false; }
	if (R == RdbDPtr(HelpFD)) {
		fd = HelpFD; if (HelpFD->Handle == nullptr) { WrLLF10Msg(57); return; }
	}
	else { fd = R->HelpFD; if (fd == nullptr) return; }
	MarkStore(p); cf = CFile; w = 0; w2 = 0;
	NewExit(Ovr, er);
	goto label4;
	if (InCWw) {
		c1 = WindMin.X + 1; c2 = WindMax.X + 1; r1 = WindMin.Y + 1; r2 = WindMax.Y + 1;
		if ((c1 == 1) && (c2 == TxtCols) && (r1 == 2) && (r2 == TxtRows)) r1 = 1;
		w = PushW(1, TxtRows, TxtCols, TxtRows);
		w2 = PushW1(c1, r1, c2, r2, true, true);
	}
	else w = PushW1(1, 1, TxtCols, TxtRows, true, true);
	i = 1; frst = true; delta = 0;
	if (backw) { byName = false; goto label3; }
label1:
	byName = true;
label2:
	s = GetHlpText(R, Name, byName, iRec); cf2 = CFile;
	if (s == nullptr)
		if (frst && (R == RdbDPtr(HelpFD)) && (KbdChar == _CtrlF1_)) {
			KbdChar = 0; Name = "Ctrl-F1 error"; goto label1;
		}
		else { Set2MsgPar(Name, fd->Name); WrLLF10Msg(146); }
	else {
		frst = false; byName = false;
		s2 = s;
		if ((s->LL > 0) && (s->A[1] == '{')) {
			{ //view after 1. line
				l = FindCtrlM(s, 1, 1);
				l = SkipCtrlMJ(s, l) - 1;
				os2 += l; s2->LL = s->LL - l;
			}
			if (s2->LL == 0) { if (delta == 0) goto label4; }
			else if (iRec != oldIRec) {
				oldIRec = iRec; iRec += delta; goto label2;
			}
			ViewHelpText(s2, i);
			if (iStk < maxStk) iStk++;
			else Move(&Stk[2], Stk, sizeof(Stk) - 4);
			/* !!! with Stk[iStk] do!!! */
			Stk[iStk].Rdb = R; Stk[iStk].FD = cf2; Stk[iStk].iR = iRec; Stk[iStk].iT = i;
			oldIRec = iRec; i = 1; delta = 0; ReleaseStore(s); CFile = cf2;
			switch (KbdChar) {
			case _ESC_: break;
			case _F10_: {
				iStk--;
			label3:
				if (iStk > 0) {
					/* !!! with Stk[iStk] do!!! */
					R = Stk[iStk].Rdb; CFile = Stk[iStk].FD; iRec = Stk[iStk].iR; i = Stk[iStk].iT;
					iStk--;
					goto label2;
				}
			}
			case _CtrlHome_: { iRec--; delta = -1; goto label2; }
			case _CtrlEnd_: { iRec++; delta = 1; goto label2; }
			default:
				if (KbdChar == _F1_) Name = "root"; else Name = LexWord; goto label1;
			}
		}
	label4:
		RestoreExit(er);
		if (w2 != 0) PopW(w2); if (w != 0) PopW(w);
		ReleaseStore(p); CFile = cf;
	}
}

void ViewHelpText(LongStr* S, WORD& TxtPos)
{
	longint L = SavePar();
	TxtColor = colors.hNorm; FillChar(ColKey, 8, colors.tCtrl);
	ColKey[5] = colors.hSpec; ColKey[3] = colors.hHili; ColKey[1] = colors.hMenu;
	bool Srch = false; bool Upd = false; longint Scr = 0;
label1:
	EditText(HelpM, MemoT, "", "", &S->A, 0xFFF0, S->LL, TxtPos, Scr,
		_F1 + _F10 + _F6 + _CtrlHome + _CtrlEnd, nullptr, Srch, Upd, 142, 145, nullptr);
	if (KbdChar == _F6_) { PrintArray(&S->A, S->LL, true); goto label1; }
	RestorePar(L);
}

void ClearHelpStkForCRdb()
{
	WORD i = 1;
	while (i <= iStk)
		if (Stk[i].Rdb == CRdb) { move(Stk[i + 1], Stk[i], (iStk - i) * 12); iStk--; }
		else i++;
}

void InitTxtEditor()
{
	FindStr[0] = 0; ReplaceStr[0] = 0; OptionStr[0] = 0; Replace = false;
	TxtColor = colors.tNorm; BlockColor = colors.tBlock;
	SysLColor = colors.fNorm;
	ColKey[0] = colors.tCtrl; move(colors.tUnderline, ColKey[1], 7);
	RdMsg(411); InsMsg = MsgLine; RdMsg(412); nInsMsg = MsgLine;
	RdMsg(413); IndMsg = MsgLine; RdMsg(414); WrapMsg = MsgLine;
	RdMsg(415); JustMsg = MsgLine; RdMsg(417); BlockMsg = MsgLine;
	RdMsg(416); ViewMsg = MsgLine;
	Insert = true; Indent = true; Wrap = false; Just = false; TypeB = false;
	LeftMarg = 1; RightMarg = 78;
	CharPg = /*char(250)*/ spec.TxtCharPg; InsPg = /*true*/ spec.TxtInsPg;
}
