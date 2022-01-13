#include "EditorEvents.h"


#include "../cppfand/wwmix.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/runproc.h"

#include <cstdio>
#include <set>

#include "OldEditor.h"
#include "runedi.h"
#include "../cppfand/obaseww.h"
#include "../cppfand/wwmenu.h"


void CtrlShiftAlt(char Mode, std::string& LastS, WORD LastNr, bool IsWrScreen)
{
	bool Ctrl = false;  WORD Delta = 0; WORD flgs = 0;
	//(*MyTestEvent 1; *)
label1:
	WaitEvent(Delta);
	if (Mode != HelpM) ScrollPress();
	if (LLKeyFlags != 0)      /* mouse */
	{
		flgs = LLKeyFlags; DisplLL(LLKeyFlags); Ctrl = true;
	}
	else if ((KbdFlgs & 0x0F) != 0) /* Ctrl Shift Alt pressed */
	{
		if (!Ctrl)
			if (Delta > 0) { flgs = KbdFlgs; DisplLL(KbdFlgs); Ctrl = true; }
			else Delta = spec.CtrlDelay;
	}
	else if (Ctrl) {
		flgs = 0;
		WrLLMargMsg(LastS, LastNr);
		Ctrl = false; Delta = 0;
	}
	/*      WaitEvent(Delta);*/
	if (!(Event.What == evKeyDown || Event.What == evMouseDown))
	{
		ClrEvent();
		if (!IsWrScreen) Background(); goto label1;
	}
	if (flgs != 0) {
		LLKeyFlags = 0;
		WrLLMargMsg(LastS, LastNr);
		AddCtrlAltShift(flgs);
	}
}

bool My2GetEvent()
{
	ClrEvent();
	GetEvent();
	if (Event.What != evKeyDown) {
		ClrEvent();
		return false;
	}

	if (toupper(Event.Pressed.Char) >= 'A' && toupper(Event.Pressed.Char) <= 'Z') {
		Event.Pressed.UpdateKey(toupper(toupper(Event.Pressed.Char)) - '@');
		if ((Event.Pressed.Char == 'Y' || Event.Pressed.Char == 'Z') && (spec.KbdTyp == CsKbd || spec.KbdTyp == SlKbd)) {
			switch (Event.Pressed.Char) {
			case 'Z': Event.Pressed.Char = 'Y'; break;
			case 'Y': Event.Pressed.Char = 'Z'; break;
			default: break;
			}
		}
	}
	return true;
}

bool HelpEvent(std::vector<WORD>& breakKeys)
{
	WORD key = Event.Pressed.KeyCombination();
	bool result = false;
	if (Event.What == evKeyDown) {
		switch (key) {
		case _ESC_:
		case _left_:
		case _right_:
		case _up_:
		case _down_:
		case _PgDn_:
		case _PgUp_:
		case _M_: {
			result = true;
			break;
		}
		default:;
		}
	}
	//else if ((Lo(Event.Pressed.KeyCombination()) == 0x00) && (Breaks.first(Hi(Event.Pressed.KeyCombination()))) != 0) {
	else if (std::find(breakKeys.begin(), breakKeys.end(), key) != breakKeys.end()) {
		result = true;
	}
	if (Event.What == evMouseDown) {
		result = true;
	}
	return result;
}

void Wr(std::string s, std::string& OrigS, char Mode, BYTE SysLColor)
{
	CHAR_INFO ci2[2];
	if (Mode != HelpM) {
		if (s.empty()) s = OrigS;
		else {
			screen.ScrRdBuf(1, 1, ci2, 2);
			OrigS[0] = ci2[0].Char.AsciiChar;
			OrigS[1] = ci2[1].Char.AsciiChar;
		}
		screen.ScrWrStr(1, 1, s, SysLColor);
	}
}

bool ScrollEvent(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys)
{
	WORD key = Event.Pressed.KeyCombination();
	bool result = false;
	if (Event.What != evKeyDown) return result;
	// with Event do case KeyCode of
	switch (key) {
	case __ESC:
	case __LEFT:
	case __RIGHT:
	case __UP:
	case __DOWN:
	case __PAGEUP:
	case __PAGEDOWN:
	case __CTRL_PAGEUP:
	case __CTRL_PAGEDOWN:
	case __CTRL_F5:
	case __ALT_F8:
		result = true;
		break;
	default: {
		// TODO: toto bude delat problem
		//if ((Lo(Event.Pressed.KeyCombination()) == 0x00)  && (Breaks.first(Hi(Event.Pressed.KeyCombination())) != 0)) {
			if (std::find(breakKeys.begin(), breakKeys.end(), key) != breakKeys.end()) {
			result = true;
		}
		else {
			//EdExitD* X = ExitD;
			//while (X != nullptr) {
				for (auto& X : ExitD) {
				if (TestExitKey(Event.Pressed.KeyCombination(), X)) {
					result = true;
					break;
				}
				else {
					//X = (EdExitD*)X->pChain;
					continue;
				}
			}
		}
	}
	}
	return result;
}

bool ViewEvent(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys)
{
	bool result = ScrollEvent(ExitD, breakKeys);
	if (Event.What != evKeyDown) return result;
	switch (Event.Pressed.KeyCombination()) {
	case _QF_:
	case _L_:
	case _F7_:
	case _F8_:
	case _KP_:
	case _QB_:
	case _QK_:
	case _CtrlF5_:
	case _AltF8_:
	case _CtrlF3_:
	case _Home_:
	case _End_:
	case _CtrlLeft_:
	case _CtrlRight_:
	case _QX_:
	case _QE_:
	case _Z_:
	case _W_:
	case _CtrlF6_:
	case _KW_:
	case _KB_:
	case _KK_:
	{
		result = true;
		break;
	}
	default:;
	}
	return result;
}

bool MyGetEvent(char Mode, BYTE SysLColor, std::string& LastS, WORD LastNr, bool IsWrScreen, bool bScroll, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys) {
	std::string OrigS = "    ";
	WORD ww;

	auto result = false;

	CtrlShiftAlt(Mode, LastS, LastNr, IsWrScreen);
	// *** Prekodovani klaves ***
	GetEvent();
	if (Event.What == evKeyDown)
		switch (Event.Pressed.KeyCombination()) {
		case _S_: Event.Pressed.Key()->wVirtualKeyCode = VK_LEFT; break;
		case _D_: Event.Pressed.Key()->wVirtualKeyCode = VK_RIGHT; break;
		case _E_: Event.Pressed.Key()->wVirtualKeyCode = VK_UP; break;
		case _X_: Event.Pressed.Key()->wVirtualKeyCode = VK_DOWN; break;
		case _R_: Event.Pressed.Key()->wVirtualKeyCode = VK_PRIOR; break;
		case _C_: Event.Pressed.Key()->wVirtualKeyCode = VK_NEXT; break;
		case _A_: Event.Pressed.Key()->wVirtualKeyCode = _CtrlLeft_; break;
		case _F_: Event.Pressed.Key()->wVirtualKeyCode = _CtrlRight_; break;
		case _V_: Event.Pressed.Key()->wVirtualKeyCode = VK_INSERT; break;
		case __CTRL_P:
		{
			Wr("^P", OrigS, Mode, SysLColor);
			ww = Event.Pressed.KeyCombination();
			if (My2GetEvent())
			{
				Wr("", OrigS, Mode, SysLColor);
				if (Event.Pressed.Char <= 0x31) {
					Event.Pressed.UpdateKey(CTRL + Event.Pressed.Char);
					//Event.KeyCode = (ww << 8) || Event.KeyCode;
				}
			}
			break;
		}
		case __CTRL_Q:
		{
			Wr("^Q", OrigS, Mode, SysLColor);
			ww = Event.Pressed.KeyCombination();
			if (My2GetEvent())
			{
				Wr("", OrigS, Mode, SysLColor);
				switch (Event.Pressed.KeyCombination()) {
				case _S_: Event.Pressed.Key()->wVirtualKeyCode = _Home_; break;
				case _D_: Event.Pressed.Key()->wVirtualKeyCode = _End_; break;
				case _R_: Event.Pressed.Key()->wVirtualKeyCode = _CtrlPgUp_; break;
				case _C_: Event.Pressed.Key()->wVirtualKeyCode = _CtrlPgDn_; break;
				case _E_: case _X_: case _Y_:
				case _L_: case _B_: case _K_:
				case _I_: case _F_: case _A_:
				case 0x2D: // -
				case 0x2F: // /
				case 0x3D: // =
					// Event.KeyCode = (ww << 8) | Event.KeyCode;
					break;
				default: {
					// Event.KeyCode = 0;
				}
				}
			}
			break;
		}
		case __CTRL_K:
		{
			Wr("^K", OrigS, Mode, SysLColor);
			ww = Event.Pressed.KeyCombination();
			if (My2GetEvent())
			{
				Wr("", OrigS, Mode, SysLColor);
				std::set<char> setKc = { _B_, _K_, _H_, _S_, _Y_, _C_, _V_, _W_, _R_, _P_, _F_, _U_, _L_, _N_ };
				if (setKc.count((char)Event.Pressed.KeyCombination()) > 0) {
					//Event.KeyCode = (ww << 8) | Event.KeyCode;
				}
				else {
					//Event.KeyCode = 0;
				}
			}
			break;
		}
		case __CTRL_O:
		{
			Wr("^O", OrigS, Mode, SysLColor);
			ww = Event.Pressed.KeyCombination();
			if (My2GetEvent())
			{
				Wr("", OrigS, Mode, SysLColor);
				switch (Event.Pressed.KeyCombination()) {
				case _W_: case  _R_: case  _L_: case  _J_: case  _C_:
				{
					//Event.KeyCode = (ww << 8) | Event.KeyCode;
					break;
				}
				default: {
					//Event.KeyCode = 0;
				}
				}
			}
			break;
		}
		}
	// *** Rezim - test ***
	switch (Mode)
	{
	case HelpM:
	{
		result = HelpEvent(breakKeys);
		break;
	}
	case ViewM: {
		if (bScroll) result = ScrollEvent(ExitD, breakKeys);
		else result = ViewEvent(ExitD, breakKeys);
		break;
	}
	case TextM: {
		if (bScroll) result = ScrollEvent(ExitD, breakKeys);
		else result = true;
		break;
	}
	default:;
	}
	return result;
}

void HandleEvent(char Mode, bool& IsWrScreen, BYTE SysLColor, std::string& LastS, WORD LastNr, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys) {
	wwmix wwmix1;
	WORD I = 0, I1 = 0;
	integer I2 = 0, I3 = 0;
	FILE* F1 = nullptr;
	WORD W1 = 0, W2 = 0;
	longint L1 = 0, L2 = 0, fs = 0;
	stEditorPar ep;
	std::string ss;
	int j = 0;
	CHAR_INFO LastL[161];
	LongStr* sp = nullptr;
	void* P1 = nullptr;
	bool bb = false;

	ExitRecord er;
	//EdExitD* X = nullptr;

	IsWrScreen = false;

	if (!MyGetEvent(Mode, SysLColor, LastS, LastNr, IsWrScreen, bScroll, ExitD, breakKeys)) {
		ClrEvent();
		IsWrScreen = false;
		return;
	}
	if (!bScroll) { CleanFrameM(ExitD, breakKeys); }
	//NewExit(Ovr(), er);
	//goto Opet;
	// !!! with Event do:
	if (Event.What == evKeyDown) {
		WORD key = Event.Pressed.KeyCombination();
		EdOk = false;
		ClrEvent();
		//X = ExitD; // Exit-procedure

		// test all exit keys
		//while (X != nullptr) {
		for (auto& X : ExitD) {
			if (TestExitKey(key, X)) {  // nastavuje i EdBreak
				TestKod();
				IndT = SetInd(T, LenT, LineI, Posi);
				ScrT = ((LineL - ScrL + 1) << 8) + Posi - BPos;
				LastTxtPos = IndT + Part.PosP;
				TxtXY = ScrT + ((longint)Posi << 16);
				if (X->Typ == 'Q') {
					Event.Pressed.UpdateKey(key);
					Konec = true; EditT = false;
					goto Nic;
				}
				switch (TypeT) {
				case FileT: {
					TestUpdFile();
					ReleaseStore(T);
					CloseH(&TxtFH);
					break;
				}
				case LocalT:
				case MemoT: {
					DelEndT();
					GetStore(2);
					Move(T, &T[3], LenT);
					sp->A = T;
					sp->LL = (WORD)LenT;
					if (TypeT == LocalT) {
						TWork.Delete(*LocalPPtr);
						LocalPPtr = (longint*)StoreInTWork(sp);
					}
					else if (UpdatT)
					{
						UpdateEdTFld(sp);
						UpdatT = false;
					}
					ReleaseStore(sp);
					break;
				}
				}
				ep = SavePar();
				screen.CrsHide();
				RestoreExit(er);
				if (TypeT == MemoT) {
					StartExit(X, false);
				}
				else {
					CallProcedure(X->Proc);
				}
				//NewExit(Ovr(), er);
				//goto Opet;
				if (!bScroll) {
					screen.CrsShow();
				}
				RestorePar(ep);
				switch (TypeT) {
				case FileT: {
					fs = Part.PosP + IndT;
					OpenTxtFh(Mode);
					RdFirstPart();
					SimplePrintHead();
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
					LenT = sp->LL;
					// T = (CharArr*)(sp)
					Move(&T[3], &T[1], LenT);
					break;
				}
				}

				WrEndT();
				IndT = MinW(IndT, LenT);
				if (TypeT != FileT)  // !!! with Part do
				{
					AbsLenT = LenT - 1;
					Part.LenP = AbsLenT;
					SimplePrintHead();
				}
				SetScreen(IndT, ScrT, Posi);
				if (!bScroll) { screen.CrsShow(); }
				if (!EdOk) { goto Nic; }
			}
			//X = (EdExitD*)X->pChain;
		}  // while

		// test frame drawing mode
		if ((Mode == SinFM || Mode == DouFM || Mode == DelFM || Mode == NotFM) && !bScroll) {
			FrameStep(FrameDir, key);
		}
		else if (Event.Pressed.isChar() || (key >= CTRL + '\x01' && key <= CTRL + '\x31')) {
			// printable character
			WrCharE(Lo(key));
			if (Wrap) {
				if (Posi > RightMarg + 1) {
					W1 = Arr[Posi];
					Arr[Posi] = 0xFF;
					KodLine();
					I1 = LeftMarg;
					while (Arr[I1] == ' ') { I1++; }
					if (I1 > RightMarg) { I1 = RightMarg; }
					L1 = Part.PosP + LineI;
					Format(I, L1, AbsLenT + LenT - Part.LenP, I1, false);
					SetPart(L1);
					I = 1;
					// TODO: tady se pouzivalo 'I' ve FindChar, ale k cemu je???
					I = FindChar(T, LenT, 0xFF, 1);
					T[I] = W1;
					SetDekLnCurrI(I);
					Posi = I - LineI + 1;
				}
			}
		}
		else {
			// control key
			switch (key) {
			case __ENTER: {
				if (Mode == HelpM) {
					Konec = WordExist();
					Event.Pressed.UpdateKey(key);
				}
				else {
					if ((NextI >= LenT) && !AllRd) NextPartDek();
					if ((NextI > LenT) || Insert) {
						NewLine('m');
						Posi = 1;
						ClrEol();
						if (LineL - ScrL == PageS) {
							screen.GotoXY(1, 1);
							//MyDelLine();
							ScrL++;
							ChangeScr = true;
						}
						else {
							screen.GotoXY(1, succ(LineL - ScrL));
							//MyInsLine();
						}
						if (Indent) {
							I1 = SetPredI();
							I = I1;
							while ((T[I] == ' ') && (T[I] != _CR)) { I++; } // tento radek je nesmyslny
							if (T[I] != _CR) { Posi = I - I1 + 1; }
						}
						else if (Wrap) {
							Posi = LeftMarg;
						}
						if (TestLastPos(1, Posi)) {
							FillChar(&Arr[1], Posi - 1, 32);
						}
					}
					else if (NextI <= LenT) {
						NextLine(true);
						Posi = 1;
					}
				}
				break;
			}
			case __LEFT: {
				if (Mode == HelpM) { HelpLU('L'); }
				else
					if (bScroll) {
						if (BCol > 0) {
							Colu = BCol;
							Posi = Position(Colu);
						}
					}
					else {
						I1 = Posi;
						if (Posi > 1) Posi--;
						BlockLRShift(I1);
					}
				break;
			}
			case __RIGHT: {
				if (Mode == HelpM) HelpRD('R');
				else {
					if (bScroll) {
						Posi = MinI(LineSize, Position(BCol + LineS + 1));
						Colu = Column(Posi);
					}
					else {
						I1 = Posi;
						if (Posi < LineSize) Posi++;
						BlockLRShift(I1);
					}
				}
				break;
			}
			case __UP: {
				if (Mode == HelpM) {
					HelpLU('U');
				}
				else {
					if (bScroll) if (RScrL == 1) goto Nic;
					L1 = LineAbs(LineL);
					PredLine();
					BlockUDShift(L1);
					if (bScroll) Posi = Position(Colu);
				}
				break;
			}
			case __DOWN: {
				if (Mode == HelpM) HelpRD('D');
				else {
					L1 = LineAbs(LineL); // na kterem jsme prave radku textu (celkove, ne na obrazovce)
					NextLine(true);
					BlockUDShift(L1);
					if (bScroll) Posi = Position(Colu);
				}
				break;
			}
			case __PAGEUP: {
				if (Mode == HelpM) { TestKod(); }
				else {
					ClrWord();
					LineL = ScrL;
				}
				L1 = LineAbs(LineL);
				if (bScroll) {
					RScrL = MaxL(1, RScrL - PageS);
					if (ModPage(RScrL)) { RScrL++; }
					ScrL = NewL(RScrL);
					LineL = ScrL;
					DekFindLine(LineAbs(LineL));
					Posi = Position(Colu);
					j = CountChar(T, LenT, 0x0C, LineI, ScrI);
					if ((j > 0) && InsPg) {
						DekFindLine(LineAbs(LineL + j));
						ScrL = LineL;
						RScrL = NewRL(ScrL);
					}
				}
				else {
					ScrL -= PageS;
					DekFindLine(LineAbs(LineL - PageS));
				}
				ChangeScr = true;
				if (Mode == HelpM) {
					ScrI = FindLine(ScrL);
					Posi = Position(Colu);
					if (WordFind(WordNo2() + 1, I1, I2, WordL) && WordExist()) {
						SetWord(I1, I2);
					}
					else { WordL = 0; }
				}
				else { BlockUDShift(L1); }
				break;
			}
			case __PAGEDOWN: {
				if (Mode != HelpM) TestKod();
				else {
					ClrWord(); LineL = ScrL;
				}
				L1 = LineAbs(LineL);
				if (bScroll) {
					RScrL += PageS; if (ModPage(RScrL)) RScrL--;
					DekFindLine(LineAbs(NewL(RScrL))); Posi = Position(Colu);
					j = CountChar(T, LenT, 0x0C, ScrI, LineI);
					if ((j > 0) && InsPg) DekFindLine(LineAbs(LineL - j));
					ScrL = LineL; RScrL = NewRL(ScrL);
				}
				else {
					DekFindLine(LineAbs(LineL) + PageS);
					if (LineL >= ScrL + PageS)  ScrL += PageS;
				}
				ChangeScr = true;
				if (Mode == HelpM) {
					ScrI = FindLine(ScrL);
					Posi = Position(Colu); W1 = WordNo2(); I3 = WordL;
					if (WordFind(W1 + 1, I1, I2, WordL) && WordExist()) { SetWord(I1, I2); }
					else if (WordFind(W1, I1, I2, WordL) && WordExist()) { SetWord(I1, I2); }
					else { WordL = 0; }
				}
				else { BlockUDShift(L1); }
				break;
			}
			case __CTRL_LEFT: {
				do {
					Posi--;
					if (Posi == 0) {
						I = LineI; PredLine();
						if ((I > 1) || ChangePart) Posi = LastPosLine();
						goto label1;
					}
				} while (Separ.count(Arr[Posi]) > 0);

				while (!(Separ.count(Arr[Posi]) > 0)) {
					Posi--;
					if (Posi == 0) goto label1;
				}
			label1:
				Posi++;
				break;
			}
			case __CTRL_RIGHT:
			{
				while (!(Separ.count(Arr[Posi]) > 0))
				{
					Posi++; if (Posi > LastPosLine()) goto label2;
				}
				while (Separ.count(Arr[Posi]) > 0)
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
			case _Z_: {
				RollNext();
				break;
			}
			case _W_: {
				RollPred();
				break;
			}
			case __HOME: {
				I1 = Posi; Posi = 1;
				if (Wrap) Posi = MaxI(LeftMarg, 1);
				BlockLRShift(I1);
				break;
			}
			case __END: {
				I1 = Posi;
				Posi = LastPosLine();
				if (Posi < LineSize) Posi++;
				BlockLRShift(I1);
				break;
			}
			case _QE_: {
				TestKod();
				LineL = ScrL; LineI = ScrI;
				DekodLine();
				break;
			}
			case _QX_: {
				TestKod();
				DekFindLine(LineAbs(ScrL + PageS - 1));
				break;
			}
			case __CTRL_PAGEUP: {
				TestKod();
				SetPart(1);
				SetScreen(1, 0, 0);
				break;
			}
			case __CTRL_PAGEDOWN: {
				TestKod();
				SetPart(AbsLenT - Part.LenP + LenT);
				SetScreen(LenT, 0, 0);
				break;
			}
			case __CTRL_F3: {
				ss = "";
				TestKod();
				do {
					if (MyPromptLL(420, ss)) goto Nic;
					val(ss, L1, I);
				} while (!(L1 > 0));
				DekFindLine(L1);
				break;
			}
			case _N_: {
				NewLine('n');
				ClrEol();
				screen.GotoXY(1, LineL - ScrL + 2);
				//MyInsLine();
				break;
			}

			case __INSERT: { Insert = !Insert; break; }
			case __DELETE:
			case _G_: {
				if (Posi <= LastPosLine()) DelChar();
				else DeleteL();
				break;
			}
			case __BACK: {
				if (Posi > 1) { Posi--; DelChar(); }
				else {
					if ((LineL == 1) && (Part.PosP > 0)) PredPart();
					if (LineI > 1) {
						TestKod();
						LineL--;
						if (T[LineI - 1] == _LF) SetDekCurrI(LineI - 2);
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
				NextI = MinW(NextI, LenT);
				TestLenText(&T, LenT, NextI, LineI);
				if (BegBLn > LineAbs(LineL)) BegBLn--;
				else if (BegBLn == LineAbs(LineL)) if (TypeB == TextBlock) BegBPos = 1;
				if (EndBLn >= LineAbs(LineL))
					if ((EndBLn == LineAbs(LineL)) && (TypeB == TextBlock)) BPos = 1;
					else EndBLn--;
				//MyDelLine();
				DekodLine();
				Posi = 1;
				break;
			}
			case _T_: {
				if (Posi > LastPosLine()) { DeleteL(); }
				else {
					I = Posi;
					if (Separ.count(Arr[Posi]) > 0) DelChar();
					else while ((I <= LastPosLine()) && !(Separ.count(Arr[Posi]) > 0)) { I++; }
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
				if (MyPromptLL(405, FindStr)) goto Nic;
				if (key == _QA_)
				{
					if (MyPromptLL(407, ReplaceStr)) goto Nic;
					Replace = true;
				}
				ss = OptionStr; if (MyPromptLL(406, ss)) goto Nic; OptionStr = ss;
				TestKod();
				if (TestOptStr('l') && (!BlockExist() || (TypeB == ColBlock))) goto Nic;
				if (TestOptStr('l')) SetBlockBound(L1, L2);
				else {
					L2 = AbsLenT - Part.LenP + LenT;
					if (TestOptStr('g') || TestOptStr('e'))  L1 = 1;
					else L1 = Part.PosP + SetInd(T, LenT, LineI, Posi);
				}
				FindReplaceString(L1, L2); if (key == _QA_) DekodLine();
				if (!Konec) { FirstEvent = false; Background(); }
				break;
			}
			case _L_: {
				if (!FindStr.empty()) {
					TestKod();
					if (TestOptStr('l') && (!BlockExist() || (TypeB == ColBlock))) goto Nic;
					fs = 1; L1 = Part.PosP + SetInd(T, LenT, LineI, Posi);
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
				while ((T[I] != ' ') && (T[I] != _CR)) { I++; }
				while (T[I] == ' ') { I++; }
				I2 = I - I1 + 1;
				if (TestLastPos(Posi, Posi + I2)) FillChar(&Arr[Posi], I2, 32);
				Posi += I2;
				break;
			}
			case _J_: {
				I1 = SetPredI() + Posi - 2;
				if ((I1 >= LineI - 1) || (I1 == 0)) goto Nic;
				I = I1;
				while (T[I] == ' ') { I++; }
				while ((T[I] != ' ') && (T[I] != _CR)) { I++; }
				if (I == I1) goto Nic;
				I2 = I - I1 - 1;
				I = Posi;
				Posi--;
				while ((Posi > 0) && (Arr[Posi] != ' ')) { Posi--; }
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
			case __F7:
			case _KH_: {
				BegBLn = LineAbs(LineL);
				if (TypeB == TextBlock) BegBPos = MinI(LastPosLine() + 1, Posi);
				else BegBPos = Posi;
				if (key == _KH_) goto OznB;
				break;
			}
			case _KK_:
			case __F8: {
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
				if (BlockHandle(fs, F1, 'Y')) { EndBLn = BegBLn; EndBPos = BegBPos; }
				break;
			}
			case _KC_: BlockCopyMove('C', P1, sp); break;
			case _KV_: BlockCopyMove('M', P1, sp); break;
			case _KU_: BlockHandle(fs, F1, 'U'); break;
			case _KL_: BlockHandle(fs, F1, 'L'); break;
			case __CTRL_F7: {
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
				if (CPath.empty()) goto Nic;
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
					WriteH(F1, 0, T); /*truncH*/ CloseH(&F1); HMsgExit(CPath);
				}
				// { PosDekFindLine(L1,I,true); }
				BegBLn = I1; BegBPos = I2; EndBLn = I3; EndBPos = I; TypeB = bb;
				break;
			}
			case __SHIFT_F7: {
				if (TypeB == TextBlock) { BlockDrop('D', P1, sp); }
				else { BlockCDrop('D', P1, sp); }
				break;
			}
			case _KR_: {
				CPath = wwmix1.SelectDiskFile(".TXT", 400, false);
				if (CPath.empty()) goto Nic;
				CVol = "";
				F1 = OpenH(_isoldfile, RdOnly);
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
						if ((TypeT != FileT) && ((I2 >= MaxLenT - LenT) || (I2 >= StoreAvail()))) {
							if (I2 >= StoreAvail()) {
								I2 = StoreAvail();
							}
							I2 = MinW(I2, MaxLenT - LenT) - 2; fs = L2 + I2;
							WrLLF10Msg(404);
						}
						I1 = L1 + L2 - Part.PosP;
						TestLenText(&T, LenT, I1, longint(I1) + I2);
						if (ChangePart) I1 -= Part.MovI;
						SeekH(F1, L2); ReadH(F1, I2, &T[I1]); HMsgExit("");
						L2 += I2;
					} while (L2 != fs);
					I = L1 + L2 - Part.PosP;
					if (T[I - 1] == 0x1A) {
						TestLenText(&T, LenT, I, I - 1);
						I--;
					}
					SetDekLnCurrI(I);
					EndBLn = Part.LineP + LineL;
					EndBPos = succ(I - LineI);
					break;
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
					break;
				}
				}
				CloseH(&F1);
				HMsgExit("");
				SetPartLine(BegBLn);
				SetDekLnCurrI(L1 - Part.PosP);
				UpdatedL = true;
				break;
			} // end case _KR_
			case _KP_: {
				if (!BlockHandle(fs, F1, 'P'))
				{
					I1 = BegBLn; I2 = BegBPos; I3 = EndBLn; I = EndBPos; bb = TypeB;
					BegBLn = 1; EndBLn = 0x7FFF; BegBPos = 1; EndBPos = 0xFF;
					TypeB = TextBlock;
					BegBLn = I1; BegBPos = I2; EndBLn = I3; EndBPos = I; TypeB = bb;
				}
				break;
			case _KF_: {
				if (BlockExist() && (TypeB == TextBlock))
				{
					TestKod(); screen.CrsHide();
					SetPartLine(EndBLn); I2 = EndBLn - Part.LineP;
					L1 = SetInd(T, LenT, FindLine(I2), EndBPos) + Part.PosP;
					L2 = BegBLn; Posi = BegBPos; SetPartLine(L2); I2 = BegBLn - Part.LineP;
					Format(I, FindLine(I2) + Part.PosP, L1, BegBPos, true);
					DekFindLine(L2);
					if (!bScroll) screen.CrsShow();
				}
				break;
			}
			case _OJ_: { Just = !Just; break; }
			case _OW_: {
				Wrap = !Wrap;
				if (Wrap) { LineS--; LastC--; }
				else {
					LastC++; LineS++;
					screen.ScrRdBuf(FirstC - 1, TxtRows - 1, LastL, LineS);
					LastL[MargLL[0]].Attributes = MargLL[1] >> 8;
					LastL[MargLL[0]].Char.AsciiChar = MargLL[1] & 0x00FF;
					LastL[MargLL[2]].Attributes = MargLL[3] >> 8;
					LastL[MargLL[2]].Char.AsciiChar = MargLL[3] & 0x00FF;
					screen.ScrWrBuf(FirstC - 1, TxtRows - 1, LastL, LineS);
				}
				break;
			}
			case _OL_: {       // LeftMarg
				do {
					ss = std::to_string(Posi);
					if (MyPromptLL(410, ss)) goto Nic;
					val(ss, I1, I);
				} while (!((I1 < RightMarg) && (I1 > 0)));
				LeftMarg = I1;
				break;
			}
			case _OR_: {       //RightMarg
				do {
					ss = std::to_string(Posi);
					if (MyPromptLL(409, ss)) goto Nic;
					val(ss, I1, I); // inc(I1);
				} while (!((I1 <= 255) && (LeftMarg < I1)));
				RightMarg = I1;
				break;
			}
			case _OC_: {
				I1 = 1;
				while ((I1 < LastPosLine()) && (Arr[I1] == ' ')) { I1++; }
				I2 = LastPosLine();
				while ((I2 > 1) && (Arr[I2] == ' ')) { I2--; }
				j = (LeftMarg + (RightMarg - LeftMarg) / 2) - int(I1 + (I2 - I1) / 2);
				if ((I2 < I1) || (j == 0)) goto Nic;
				if (j > 0) {
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
				TestKod();
				L1 = Part.PosP + LineI;
				Format(I, L1, AbsLenT + LenT - Part.LenP, MinI(LeftMarg, Posi), false);
				SetPart(L1);
				I2 = L1 - Part.PosP;
				SetDekLnCurrI(I2); Posi = 1;
				break;
			}
			case _framesingle_: {
				Mode = SinFM; screen.CrsBig(); FrameDir = 0; break;
			}
			case _framedouble_: {
				Mode = DouFM; screen.CrsBig(); FrameDir = 0; break;
			}
			case _delframe_: {
				Mode = DelFM; screen.CrsBig(); FrameDir = 0; break;
			}
			case _F4_: {
				char c = ToggleCS(Arr[Posi]);
				UpdatedL = c != Arr[Posi];
				Arr[Posi] = c;
				break;
			}
			case __CTRL_F5:
				Calculate();
				break;
			case __ALT_F8: {
				ep = SavePar();
				W1 = Menu(45, spec.KbdTyp + 1);
				if (W1 != 0) spec.KbdTyp = TKbdConv(W1 - 1);
				RestorePar(ep);
				break;
			}
			case __CTRL_F6: {
				if ((TypeT == FileT) || (TypeT == LocalT)) BlockHandle(fs, F1, 'p');
				break;
			}
			case 0x1000: {
			Opet:
				if ((Mode != HelpM) && (Mode != ViewM) && Wrap)
					screen.Window(FirstC, FirstR + 1, LastC + 1, LastR);
				else screen.Window(FirstC, FirstR + 1, LastC, LastR);

				if (!bScroll) screen.CrsShow();
				SetScreen(LineI, 0, 0);
				break;
			}
			case _U_: { // previous state recovery
				if (TypeT != FileT)
					if (PromptYN(108)) {
						IndT = 1;
						Event.Pressed.UpdateKey('U');
						Konec = true;
						EdBreak = 0xFFFF;
					}
				break;
			}
			case __ESC: {
				TestKod();
				//Event.Pressed.UpdateKey(key);
				Konec = true;
				EdBreak = 0;
				break;
			}
			case __ALT_EQUAL: { // end without saving
				if (TypeT != FileT) {
					TestKod();
					//Event.Pressed.UpdateKey(key);
					Konec = true;
					EdBreak = 0xFFFF;
				}
				break;
			}
			default: {
				if (std::find(breakKeys.begin(), breakKeys.end(), key) != breakKeys.end()) {
					// *** BREAK KEYS ***
					TestKod();
					//KbdChar: = ww;
					Konec = true;
					EdBreak = 0xFFFF;
				}
				else if (key >= 0x1000 && key <= 0x101F) {
					WrCharE(Lo(key)); // ***CTRL-klavesy***
					if (key == 0x100D) {
						TestKod();
						DekodLine();
						Posi--;
					}
				}
				break;
				// ***ERROR TESTLENTEXT***
			}
			}
			}
		} // else
	} // if (Event.What == evKeyDown)

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
		I = SetInd(T, LenT, LineI, Posi);
		if (I < LenT) {
			if (Mode == HelpM) {
				ClrWord();
				WordFind(WordNo(I + 1), I1, I2, W1);
				if ((I1 <= I) && (I2 >= I)) {
					SetWord(I1, I2);
					Event.Pressed.UpdateKey(_M_);
					Konec = true;
				}
				else if (WordExist()) {
					WordFind(W2, I1, I2, W1);
					SetWord(I1, I2);
				}
				else {
					SetDekLnCurrI(I3);
				}
			}
		}
		else {
			SetDekLnCurrI(I3);
			Posi = (WORD)j;
		}
		ClrEvent();
	} // else if ((Event.What == evMouseDown) && ((Mode == HelpM) || (Mode == TextM)))
	else {
		ClrEvent();
	}

Nic:
	//ClrEvent;
	RestoreExit(er);
	IsWrScreen = false;
}
