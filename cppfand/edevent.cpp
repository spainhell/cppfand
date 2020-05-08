#pragma once

#include "edevent.h"
#include "access.h"
#include "base.h"
#include "common.h"
#include "edevinpt.h"
#include "editor.h"
#include "legacy.h"
#include "constants.h"
#include "edevproc.h"
#include "edscreen.h"
#include "edtextf.h"
#include "genrprt.h"
#include "handle.h"
#include "kbdww.h"
#include "memory.h"
#include "recacc.h"
#include "runedi.h"
#include "runproc.h"
#include "wwmenu.h"
#include "wwmix.h"

void HandleEvent() {
	wwmix wwmix1;
	
	WORD I, I1, I2, I3;
	FILE* F1 = nullptr;
	WORD W1 = 0, W2 = 0, ww = 0;
	longint L1, L2, fs;
	pstring ss;
	int j;
	WORD LastL[161];
	LongStr* sp = nullptr;
	void* P1 = nullptr;
	bool bb;

	ExitRecord er;
	EdExitD* X = nullptr;

	IsWrScreen = false;

	if (!MyGetEvent()) { ClrEvent(); IsWrScreen = false; return; }
	if (!bScroll) CleanFrameM();
	//NewExit(Ovr(), er);
	goto Opet;
	// !!! with Event do:
	if (Event.What == evKeyDown) {
		EdOk = false; ww = Event.KeyCode; ClrEvent();
		X = ExitD;                         // Exit-procedure
		while (X != nullptr) {
			if (TestExitKey(ww, X)) {  // nastavuje i EdBreak
				TestKod(); IndT = SetInd(LineI, Posi);
				ScrT = ((LineL - ScrL + 1) << 8) + Posi - BPos;
				LastTxtPos = IndT + Part.PosP;
				TxtXY = ScrT + (longint(Posi) << 16);
				if (X->Typ == 'Q') {
					KbdChar = ww; Konec = true; EditT = false;
					goto Nic;
				}
				switch (TypeT) {
				case FileT: { TestUpdFile(); ReleaseStore(T); CloseH(TxtFH); break; }
				case LocalT:
				case MemoT: {
					DelEndT(); GetStore(2); Move(T, T[3], LenT);
					sp = (LongStr*)(T); sp->LL = LenT;
					if (TypeT == LocalT) {
						TWork.Delete(*LocalPPtr);
						LocalPPtr = (longint*)StoreInTWork(sp);
					}
					else if (UpdatT)
					{
						UpdateEdTFld(sp); UpdatT = false;
					}
					ReleaseStore(sp);
					break;
				}
				}
				L2 = SavePar(); CrsHide();
				RestoreExit(er);
				if (TypeT == MemoT) StartExit(X, false);
				else CallProcedure(X->Proc);
				//NewExit(Ovr(), er);
				goto Opet;
				if (!bScroll) CrsShow(); RestorePar(L2);
				switch (TypeT) {
				case FileT: {
					fs = Part.PosP + IndT;
					OpenTxtFh(Mode); RdFirstPart(); SimplePrintHead();
					while ((fs > Part.PosP + Part.LenP) && !AllRd) { RdNextPart(); }
					IndT = fs - Part.PosP;
					break;
				}
				case LocalT:
				case MemoT:
				{
					if (TypeT == LocalT) sp = TWork.Read(1, *LocalPPtr);
					else {
						CRecPtr = EditDRoot->NewRecPtr;
						sp = _LongS(CFld->FldD);
					}
					LenT = sp->LL; T = (CharArr*)(sp); Move(T[3], T[1], LenT);
					break;
				}
				}

				WrEndT();
				IndT = MinW(IndT, LenT);
				if (TypeT != FileT)  // !!! with Part do
				{
					AbsLenT = LenT - 1; Part.LenP = AbsLenT; SimplePrintHead();
				}
				SetScreen(IndT, ScrT, Posi);
				if (!bScroll) { CrsShow(); }
				if (!EdOk) { goto Nic; }
			}
			X = X->Chain;

			if (((Mode == SinFM) || (Mode == DouFM) || (Mode == DelFM) || (Mode == NotFM)) && !bScroll)
				FrameStep(FrameDir, ww);
			else
				switch (ww) {
				case _left_: {
					if (Mode == HelpM) HelpLU('L');
					else
						if (bScroll)
						{
							if (BCol > 0) { Colu = BCol; Posi = Position(Colu); }
						}
						else {
							I1 = Posi;
							if (Posi > 1) Posi--;
							BlockLRShift(I1);
						}
					break;
				}
				case _right_: {
					if (Mode == HelpM) HelpRD('R');
					else {
						if (bScroll) {
							Posi = MinI(LineSize, Position(BCol + LineS + 1));
							Colu = Column(Posi);
						}
						else {
							I1 = Posi; if (Posi < LineSize) Posi++;
							BlockLRShift(I1);
						}
					}
					break;
				}
				case _up_: {
					if (Mode == HelpM) HelpLU('U');
					else {
						if (bScroll) if (RScrL == 1) goto Nic;
						L1 = LineAbs(LineL);
						PredLine();
						BlockUDShift(L1);
						if (bScroll) Posi = Position(Colu);
					}
					break;
				}
				case _down_: {
					if (Mode == HelpM) HelpRD('D');
					else {
						L1 = LineAbs(LineL); NextLine(true); BlockUDShift(L1);
						if (bScroll) Posi = Position(Colu);
					}
					break;
				}
				case _PgUp_: {
					if (Mode == HelpM) TestKod();
					else { ClrWord(); LineL = ScrL; }
					L1 = LineAbs(LineL);
					if (bScroll)
					{
						RScrL = MaxL(1, RScrL - PageS);
						if (ModPage(RScrL)) RScrL++;
						ScrL = NewL(RScrL); LineL = ScrL;
						DekFindLine(LineAbs(LineL)); Posi = Position(Colu);
						j = CountChar(0x0C, LineI, ScrI);
						if ((j > 0) && InsPg) {
							DekFindLine(LineAbs(LineL + j));
							ScrL = LineL; RScrL = NewRL(ScrL);
						}
					}
					else
					{
						ScrL -= PageS; DekFindLine(LineAbs(LineL - PageS));
					}
					ChangeScr = true;
					if (Mode == HelpM)
					{
						ScrI = FindLine(ScrL);
						Posi = Position(Colu);
						if (WordFind(WordNo2() + 1, I1, I2, WordL) && WordExist())
							SetWord(I1, I2);
						else WordL = 0;
					}
					else BlockUDShift(L1);
					break;
				}
				case _PgDn_: {
					if (Mode != HelpM) TestKod();
					else {
						ClrWord(); LineL = ScrL;
					}
					L1 = LineAbs(LineL);
					if (bScroll)
					{
						RScrL += PageS; if (ModPage(RScrL)) RScrL--;
						DekFindLine(LineAbs(NewL(RScrL))); Posi = Position(Colu);
						j = CountChar(0x0C, ScrI, LineI);
						if ((j > 0) && InsPg) DekFindLine(LineAbs(LineL - j));
						ScrL = LineL; RScrL = NewRL(ScrL);
					}
					else
					{
						DekFindLine(LineAbs(LineL) + PageS);
						if (LineL >= ScrL + PageS)  ScrL += PageS;
					}
					ChangeScr = true;
					if (Mode == HelpM) {
						ScrI = FindLine(ScrL);
						Posi = Position(Colu); W1 = WordNo2(); I3 = WordL;
						if (WordFind(W1 + 1, I1, I2, WordL) && WordExist()) SetWord(I1, I2);
						else if (WordFind(W1, I1, I2, WordL) && WordExist()) SetWord(I1, I2);
						else WordL = 0;
					}
					else BlockUDShift(L1);
					break;
				}
				case _CtrlLeft_: {
					do {
						Posi--;
						if (Posi == 0)
						{
							I = LineI; PredLine();
							if ((I > 1) || ChangePart) Posi = LastPosLine();
							goto label1;
						}
					} while (Oddel.count(Arr[Posi]) > 0);

					while (!(Oddel.count(Arr[Posi]) > 0))
					{
						Posi--;
						if (Posi == 0) goto label1;
					}
				label1:
					Posi++;
					break;
				}
				case _CtrlRight_:
				{
					while (!(Oddel.count(Arr[Posi]) > 0))
					{
						Posi++; if (Posi > LastPosLine()) goto label2;
					}
					while (Oddel.count(Arr[Posi]) > 0)
					{
						Posi++;
						I = LastPosLine();
						if (Posi > I)
							if ((NextI <= LenT) && ((I == 0) || (Posi > I + 1)))
							{
								NextLine(true); Posi = 1;
							}
							else { Posi = I + 1; goto label2; }
					}
				label2:
					break; }
				case _Z_: { RollNext(); break; }
				case _W_: { RollPred(); break; }
				case _Home_: {
					I1 = Posi; Posi = 1; if (Wrap) Posi = MaxI(LeftMarg, 1);
					BlockLRShift(I1);
					break;
				}
				case _End_: {
					I1 = Posi; Posi = LastPosLine(); if (Posi < LineSize) Posi++;
					BlockLRShift(I1);
					break;
				}
				case _QE_: { TestKod(); LineL = ScrL; LineI = ScrI; DekodLine(); break; }
				case _QX_: { TestKod(); DekFindLine(LineAbs(ScrL + PageS - 1)); break; }
				case _CtrlPgUp_: { TestKod(); SetPart(1); SetScreen(1, 0, 0); break; }
				case _CtrlPgDn_: {
					TestKod(); SetPart(AbsLenT - Part.LenP + LenT);
					SetScreen(LenT, 0, 0); break;
				}
				case _CtrlF3_: {
					ss = ""; TestKod();
					do {
						if (MyPromptLL(420, &ss)) goto Nic;
						val(ss, L1, I);
					} while (!(L1 > 0));
					DekFindLine(L1);
					break;
				}
							 // *****************************************
							 // tady byly pùvodnì *********CHAR**********
							 // *****************************************
				case _M_: {
					if (Mode == HelpM) { Konec = WordExist(); KbdChar = ww; }
					else
					{
						if ((NextI >= LenT) && !AllRd) NextPartDek();
						if ((NextI > LenT) || Insert)
						{
							NewLine('m'); Posi = 1; ClrEol();
							if (LineL - ScrL == PageS)
							{
								GotoXY(1, 1); MyDelLine(); ScrL++; ChangeScr = true;
							}
							else { GotoXY(1, succ(LineL - ScrL)); MyInsLine(); }
							if (Indent)
							{
								I1 = SetPredI(); I = I1;
								while ((*T[I] == ' ') && (*T[I] != _CR)) { I++; }
								if (*T[I] != _CR) Posi = I - I1 + 1;
							}
							else if (Wrap) Posi = LeftMarg;
							if (TestLastPos(1, Posi)) FillChar(&Arr[1], Posi - 1, 32);
						}
						else if (NextI <= LenT) { NextLine(true); Posi = 1; }
					}
					break;
				}
				case _N_: { NewLine('n'); ClrEol(); GotoXY(1, LineL - ScrL + 2); MyInsLine(); break; }

				case _Ins_: { Insert = !Insert; break; }
				case _Del_:
				case _G_: {
					if (Posi <= LastPosLine()) DelChar();
					else DeleteL(); break;
				}
				case _H_: {
					if (Posi > 1) { Posi--; DelChar(); }
					else {
						if ((LineL == 1) && (Part.PosP > 0)) PredPart();
						if (LineI > 1)
						{
							TestKod(); LineL--;
							if (*T[LineI - 1] == _LF) SetDekCurrI(LineI - 2);
							else SetDekCurrI(LineI - 1);
							Posi = MinW(255, succ(LastPosLine()));
							DeleteL();
							if (LineL < ScrL) { ScrL--; ChangeScr = true; }
						}
					}
					break;
				}
				case _Y_: {
					if ((NextI >= LenT) && !AllRd) NextPartDek();
					NextI = MinW(NextI, LenT); TestLenText(NextI, LineI);
					if (BegBLn > LineAbs(LineL)) BegBLn--;
					else if (BegBLn == LineAbs(LineL)) if (TypeB == TextBlock) BegBPos = 1;
					if (EndBLn >= LineAbs(LineL))
						if ((EndBLn == LineAbs(LineL)) && (TypeB == TextBlock)) BPos = 1;
						else EndBLn--;
					MyDelLine(); DekodLine(); Posi = 1;
					break;
				}
				case _T_: {
					if (Posi > LastPosLine()) DeleteL();
					else
					{
						I = Posi;
						if (Oddel.count(Arr[Posi]) > 0) DelChar();
						else while ((I <= LastPosLine()) && !(Oddel.count(Arr[Posi]) > 0)) { I++; }
						while ((I <= LastPosLine()) && (Arr[I] == ' ')) { I++; }
						// TODO: k èemu to tady je? if ((I>Posi) and TestLastPos(I,Posi))
					}
					break;
				}
				case _QI_: { Indent = !Indent; break; }
				case _QL_: { if (UpdatedL) DekodLine(); break; }
				case _QY_: {if (TestLastPos(LastPosLine() + 1, Posi)) ClrEol(); break; }
				case _QF_:
				case _QA_: {
					Replace = false;
					if (MyPromptLL(405, &FindStr)) goto Nic;
					if (ww == _QA_)
					{
						if (MyPromptLL(407, &ReplaceStr)) goto Nic;
						Replace = true;
					}
					ss = OptionStr; if (MyPromptLL(406, &ss)) goto Nic; OptionStr = ss;
					TestKod();
					if (TestOptStr('l') && (!BlockExist() || (TypeB = ColBlock))) goto Nic;
					if (TestOptStr('l')) SetBlockBound(L1, L2);
					else {
						L2 = AbsLenT - Part.LenP + LenT;
						if (TestOptStr('g') || TestOptStr('e'))  L1 = 1;
						else L1 = Part.PosP + SetInd(LineI, Posi);
					}
					FindReplaceString(L1, L2); if (ww == _QA_) DekodLine();
					if (!Konec) { FirstEvent = false; Background(); }
					break;
				}
				case _L_: {
					if (FindStr != "")
					{
						TestKod();
						if (TestOptStr('l') && (!BlockExist() || (TypeB == ColBlock))) goto Nic;
						fs = 1; L1 = Part.PosP + SetInd(LineI, Posi);
						if (TestOptStr('l'))  SetBlockBound(fs, L2);
						else L2 = AbsLenT - Part.LenP + LenT;
						if (L1 < fs)  L1 = fs;  // { if L1>=L2  goto Nic;}
						FindReplaceString(L1, L2);
						if (!Konec) { FirstEvent = false; Background(); };
					}
					break;
				}
				case _I_: {
					I1 = SetPredI() + Posi;
					if (I1 >= LineI - 1) goto Nic;
					I = I1;
					while ((*T[I] != ' ') && (*T[I] != _CR)) { I++; }
					while (*T[I] == ' ') { I++; }
					I2 = I - I1 + 1;
					if (TestLastPos(Posi, Posi + I2)) FillChar(&Arr[Posi], I2, 32);
					Posi += I2;
					break;
				}
				case _J_: {
					I1 = SetPredI() + Posi - 2;
					if ((I1 >= LineI - 1) || (I1 == 0)) goto Nic;
					I = I1; while (*T[I] == ' ') { I++; }
					while ((*T[I] != ' ') && (*T[I] != _CR)) { I++; }
					if (I == I1) goto Nic;
					I2 = I - I1 - 1;
					I = Posi;
					Posi--;
					while ((Posi > 0) and (Arr[Posi] != ' ')) { Posi--; }
					Posi++;
					if (TestLastPos(Posi, Posi + I2)) FillChar(&Arr[Posi], I2, 32);
					Posi = I + I2 + 1;
					break;
				}
				case _QB_: {
					TestKod();
					PosDekFindLine(BegBLn, MinW(LastPosLine() + 1, BegBPos), false); break; }
				case _QK_: {
					TestKod();
					PosDekFindLine(EndBLn, MinW(LastPosLine() + 1, EndBPos), false); break; }
				case _KB_:
				case _F7_:
				case _KH_:
				{
					BegBLn = LineAbs(LineL);
					if (TypeB == TextBlock) BegBPos = MinI(LastPosLine() + 1, Posi);
					else BegBPos = Posi;
					if (ww == _KH_) goto OznB;
					break;
				}
				case _KK_:
				case _F8_: {
				OznB:
					EndBLn = LineAbs(LineL);
					if (TypeB == TextBlock) EndBPos = MinI(LastPosLine() + 1, Posi);
					else EndBPos = Posi;
					break;
				}
				case _KN_: {
					if (TypeB == TextBlock) TypeB = ColBlock;
					else TypeB = TextBlock;
					break;
				}
				case _KY_: {
					if (BlockHandle(fs, F1, 'Y')) { EndBLn = BegBLn; EndBPos = BegBPos; };
					break;
				}
				case _KC_: BlockCopyMove('C', P1, sp); break;
				case _KV_: BlockCopyMove('M', P1, sp); break;
				case _KU_: BlockHandle(fs, F1, 'U'); break;
				case _KL_: BlockHandle(fs, F1, 'L'); break;
				case _CtrlF7_: {
					if (TypeB == TextBlock) BlockGrasp('G', P1, sp);
					else BlockCGrasp('G', P1, sp);
					break;
				}
				case _KW_: {
					I1 = BegBLn; I2 = BegBPos; I3 = EndBLn; I = EndBPos; bb = TypeB;
					if (!BlockExist())
					{
						BegBLn = 1; EndBLn = 0x7FFF; BegBPos = 1; EndBPos = 0xFF; TypeB = TextBlock;
					}
					CPath = wwmix1.SelectDiskFile(".TXT", 401, false);
					if (CPath == "")  goto Nic;
					CVol = "";
					F1 = OpenH(_isnewfile, Exclusive);
					if (HandleError == 80)
					{
						SetMsgPar(CPath);
						if (PromptYN(780)) F1 = OpenH(_isoverwritefile, Exclusive);
						else goto Nic;
					}
					if (HandleError != 0) { MyWrLLMsg(CPath); goto Nic; }
					fs = 0; // {L1 =LineAbs(LineL);I =Posi;}
					if (BlockHandle(fs, F1, 'W'))
					{
						WriteH(F1, 0, *T); /*truncH*/ CloseH(F1); HMsgExit(CPath);
					}
					// { PosDekFindLine(L1,I,true); }
					BegBLn = I1; BegBPos = I2; EndBLn = I3; EndBPos = I; TypeB = bb;
					break;
				}
				case _ShiftF7_: {
					if (TypeB == TextBlock) BlockDrop('D', P1, sp);
					else BlockCDrop('D', P1, sp);
					break;
				}
				case _KR_: {
					CPath = wwmix1.SelectDiskFile(".TXT", 400, false);
					if (CPath == "") goto Nic;
					CVol = ""; F1 = OpenH(_isoldfile, RdOnly);
					if (HandleError != 0) { MyWrLLMsg(CPath); goto Nic; }
					BegBLn = Part.LineP + LineL; BegBPos = Posi;
					L1 = Part.PosP + LineI + Posi - 1;
					FillBlank();
					fs = FileSizeH(F1); L2 = 0;
					NullChangePart();
					switch (TypeB) {
					case TextBlock: {
						do {
							I2 = 0x1000; if (fs - L2 < longint(I2))  I2 = fs - L2;
							if ((TypeT != FileT) && ((I2 >= MaxLenT - LenT) || (I2 >= StoreAvail())))
							{
								if (I2 >= StoreAvail())  I2 = StoreAvail();
								I2 = MinW(I2, MaxLenT - LenT) - 2; fs = L2 + I2;
								WrLLF10Msg(404);
							}
							I1 = L1 + L2 - Part.PosP;
							TestLenText(I1, longint(I1) + I2);
							if (ChangePart) I1 -= Part.MovI;
							SeekH(F1, L2); ReadH(F1, I2, T[I1]); HMsgExit("");
							L2 += I2;
						} while (L2 != fs);
						I = L1 + L2 - Part.PosP;
						if (*T[I - 1] == 0x1A) { TestLenText(I, I - 1); I--; }
						SetDekLnCurrI(I); EndBLn = Part.LineP + LineL; EndBPos = succ(I - LineI);
					}
					case ColBlock: {
						EndBPos = Posi; I2 = 0x1000;
						MarkStore2(P1); sp = (LongStr*)GetStore2(I2 + 2); //ww =BegBPos;}
						do {
							if (fs - L2 < longint(I2)) I2 = fs - L2;
							SeekH(F1, L2); ReadH(F1, I2, sp->A); HMsgExit("");
							L2 += I2; sp->LL = I2; BlockCDrop('R', P1, sp);
						} while (L2 != fs);
						EndBLn = Part.LineP + LineL - 1; ReleaseStore2(P1);
					}
					}
					CloseH(F1); HMsgExit("");
					SetPartLine(BegBLn); SetDekLnCurrI(L1 - Part.PosP); UpdatedL = true;
					break;
				} // end case _KR_
				case _KP_: { if (!BlockHandle(fs, F1, 'P'))
				{
					I1 = BegBLn; I2 = BegBPos; I3 = EndBLn; I = EndBPos; bb = TypeB;
					BegBLn = 1; EndBLn = 0x7FFF; BegBPos = 1; EndBPos = 0xFF;
					TypeB = TextBlock;
					BegBLn = I1; BegBPos = I2; EndBLn = I3; EndBPos = I; TypeB = bb;
					break;
				}
				case _KF_: {
					if (BlockExist() && (TypeB == TextBlock))
					{
						TestKod(); CrsHide();
						SetPartLine(EndBLn); I2 = EndBLn - Part.LineP;
						L1 = SetInd(FindLine(integer(I2)), EndBPos) + Part.PosP;
						L2 = BegBLn; Posi = BegBPos; SetPartLine(L2); I2 = BegBLn - Part.LineP;
						Format(I, FindLine(integer(I2)) + Part.PosP, L1, BegBPos, true);
						DekFindLine(L2);
						if (!bScroll) CrsShow();
					}
					break;
				}
				case _OJ_: { Just = !Just; break; }
				case _OW_: {
					Wrap = !Wrap;
					if (Wrap) { LineS--; LastC--; }
					else {
						LastC++; LineS++;
						ScrRdBuf(FirstC - 1, TxtRows - 1, &LastL[1], LineS);
						LastL[MargLL[0]] = MargLL[1];
						LastL[MargLL[2]] = MargLL[3];
						ScrWrBuf(FirstC - 1, TxtRows - 1, &LastL[1], LineS);
					}
					break;
				}
				case _OL_: {       // LeftMarg
					do {
						str(Posi, ss);
						if (MyPromptLL(410, &ss)) goto Nic;
						val(ss, I1, I);
					} while (!((I1 < RightMarg) && (I1 > 0)));
					LeftMarg = I1;
					break;
				}
				case _OR_: {       //RightMarg
					do {
						str(Posi, ss);
						if (MyPromptLL(409, &ss)) goto Nic;
						val(ss, I1, I); // inc(I1);
					} while (!((I1 <= 255) && (LeftMarg < I1)));
					RightMarg = I1;
					break;
				}
				case _OC_: {
					I1 = 1; while ((I1 < LastPosLine()) && (Arr[I1] == ' ')) { I1++; }
					I2 = LastPosLine(); while ((I2 > 1) && (Arr[I2] == ' ')) { I2--; }
					j = (LeftMarg + (RightMarg - LeftMarg) / 2) - int(I1 + (I2 - I1) / 2);
					if ((I2 < I1) || (j == 0)) goto Nic;
					if (j > 0)
					{
						if (TestLastPos(1, j + 1)) FillChar(&Arr[1], j, 32);
					}
					else {
						j = MinI(-j, I1 - 1);
						TestLastPos(j + 1, 1);
					}
					Posi = MinW(LineSize, LastPosLine() + 1);
					break;
				}
				case _B_: {
					TestKod(); L1 = Part.PosP + LineI;
					Format(I, L1, AbsLenT + LenT - Part.LenP, MinI(LeftMarg, Posi), false);
					SetPart(L1); I2 = L1 - Part.PosP; SetDekLnCurrI(I2); Posi = 1;
					break;
				}
				case _framesingle_: { Mode = SinFM; CrsBig(); FrameDir = 0; break; }
				case _framedouble_: { Mode = DouFM; CrsBig(); FrameDir = 0; break; }
				case _delframe_: { Mode = DelFM; CrsBig(); FrameDir = 0; break; }
				case _F4_: {
					W1 = ToggleCS(Arr[Posi]);
					UpdatedL = W1 != Arr[Posi];
					Arr[Posi] = W1;
					break;
				}
				case _CtrlF5_:
					Calculate(); break;
				case _AltF8_: {
					L2 = SavePar();
					W1 = Menu(45, spec.KbdTyp + 1);
					if (W1 != 0) spec.KbdTyp = TKbdConv(W1 - 1);
					RestorePar(L2);
					break;
				}
				case _CtrlF6_: {
					if ((TypeT == FileT) || (TypeT == LocalT)) BlockHandle(fs, F1, 'p');
					break;
				}

				case 0x1000: {
				Opet:
					if ((Mode != HelpM) && (Mode != ViewM) && Wrap)
						Window(FirstC, FirstR + 1, LastC + 1, LastR);
					else Window(FirstC, FirstR + 1, LastC, LastR);

					if (!Scroll) CrsShow();
					SetScreen(LineI, 0, 0); }

				case _U_: {
					if (TypeT != FileT)
						if (PromptYN(108))
						{
							IndT = 1; KbdChar = _U_; Konec = true; EdBreak = 0xFFFF;

						}
					break;
				}
				default:
				{
					if (ww >= 0x1000 && ww <= 0x101F) {
						WrChar(Lo(ww)); // ***CTRL-klavesy***
						if (ww == 0x100D) { TestKod(); DekodLine(); Posi--; }
					}
					if (ww >= 0x0020 && ww <= 0x00FF) { // *********CHAR********** }
						WrChar(Lo(ww));
						if (Wrap) if (Posi > RightMarg + 1)
						{
							W1 = Arr[Posi]; Arr[Posi] = 0xFF; KodLine();
							I1 = LeftMarg; while (Arr[I1] == ' ') I1++;
							if (I1 > RightMarg) I1 = RightMarg;
							L1 = Part.PosP + LineI;
							Format(I, L1, AbsLenT + LenT - Part.LenP, I1, false);
							SetPart(L1); I = 1;
							I = FindChar(I, 0xFF, 1, LenT); *T[I] = W1;
							SetDekLnCurrI(I); Posi = I - LineI + 1;
						}
					}
					if (((Lo(ww) == 0x00) && (Breaks.first(Hi(ww)) != 0))
						|| ((ww == _AltEqual_) && (TypeT != FileT)))
					{
						TestKod(); KbdChar = ww; Konec = true; EdBreak = 0xFFFF;
					}
					else if (ww == _ESC_)
					{
						TestKod(); KbdChar = ww; Konec = true; EdBreak = 0;
					}
					break;
					// ***ERROR TESTLENTEXT***
				}
				}
				}
		}
	}
	else if ((Event.What == evMouseDown) && ((Mode == HelpM) || (Mode == TextM)))
	{
		if (Mode == TextM) TestKod();
		if (!((Event.Where.Y >= FirstR && Event.Where.Y <= LastR - 1)
			&& (Event.Where.X >= FirstC - 1 && Event.Where.X <= LastC - 1)))
		{
			ClrEvent();
			goto Nic;
		}
		I3 = LineI; j = Posi;
		W1 = Event.Where.Y - WindMin.Y + ScrL;
		if (Mode == HelpM) W2 = WordNo2() + 1;
		DekFindLine(LineAbs(W1));
		Posi = Event.Where.X - WindMin.X + 1;
		if (Mode != TextM) Posi = Position(Posi);
		Posi += BPos;
		I = SetInd(LineI, Posi);
		if (I < LenT)
		{
			if (Mode == HelpM)
			{
				ClrWord();
				WordFind(WordNo(I + 1), I1, I2, W1);
				if ((I1 <= I) && (I2 >= I)) {
					SetWord(I1, I2); KbdChar = _M_;
					Konec = true;
				}
				else if (WordExist())
				{
					WordFind(W2, I1, I2, W1);
					SetWord(I1, I2);
				}
				else SetDekLnCurrI(I3);
			}
		}
		else { SetDekLnCurrI(I3); Posi = j; }
		ClrEvent();
	}
	else ClrEvent();

Nic:
	//ClrEvent;
	RestoreExit(er); IsWrScreen = false;

}
