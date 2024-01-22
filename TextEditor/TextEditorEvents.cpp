#include "TextEditorEvents.h"

#include "../Drivers/constants.h"
#include "../Core/wwmix.h"
#include "../Core/GlobalVariables.h"
#include "../Core/runproc.h"

#include <cstdio>
#include <set>

#include "TextEditor.h"
#include "../DataEditor/runedi.h"
#include "../Core/obaseww.h"
#include "../Core/wwmenu.h"


TextEditorEvents::TextEditorEvents()
{
	_modes_handler = new TextEditorModes(this);
}

TextEditorEvents::~TextEditorEvents()
{
	delete _modes_handler;
	_modes_handler = nullptr;
}

void TextEditorEvents::CtrlShiftAlt(TextEditor* editor, char mode, std::string& LastS, WORD LastNr, bool IsWrScreen)
{
	bool Ctrl = false;  WORD Delta = 0; WORD flgs = 0;
	//(*MyTestEvent 1; *)
label1:
	WaitEvent(Delta);
	if (mode != HelpM) {
		editor->ScrollPress();
	}
	if (LLKeyFlags != 0) {
		// mouse
		flgs = LLKeyFlags;
		editor->DisplLL(LLKeyFlags);
		Ctrl = true;
	}
	else if ((KbdFlgs & 0x0F) != 0) {
		// Ctrl Shift Alt pressed
		if (!Ctrl) {
			if (Delta > 0) {
				flgs = KbdFlgs;
				editor->DisplLL(KbdFlgs);
				Ctrl = true;
			}
			else {
				Delta = spec.CtrlDelay;
			}
		}
	}
	else if (Ctrl) {
		flgs = 0;
		editor->WrLLMargMsg(LastS, LastNr);
		Ctrl = false; Delta = 0;
	}
	/*      WaitEvent(Delta);*/
	if (!(Event.What == evKeyDown || Event.What == evMouseDown))
	{
		ClrEvent();
		if (!IsWrScreen) editor->Background();
		goto label1;
	}
	if (flgs != 0) {
		LLKeyFlags = 0;
		editor->WrLLMargMsg(LastS, LastNr);
		AddCtrlAltShift(flgs);
	}
}

bool TextEditorEvents::My2GetEvent()
{
	bool result = true;

	while (true) {
		ClrEvent();
		GetEvent();
		if (Event.What != evKeyDown) {
			ClrEvent();
			result = false;
			break;
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
			break;
		}
		else {
			// no printable character, only CTRL, ALT, SHIFT, ...
			// what about 'ESC, ...' ?
			continue;
		}
	}

	return result;
}

bool TextEditorEvents::HelpEvent(std::vector<WORD>& breakKeys)
{
	WORD key = Event.Pressed.KeyCombination();
	bool result = false;
	if (Event.What == evKeyDown) {
		switch (key) {
		case __ESC:
		case __LEFT:
		case __RIGHT:
		case __UP:
		case __DOWN:
		case __PAGEDOWN:
		case __PAGEUP:
		case 'M': {
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

void TextEditorEvents::Wr(std::string s, std::string& OrigS, char Mode, BYTE SysLColor)
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

bool TextEditorEvents::ScrollEvent(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys)
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

bool TextEditorEvents::ViewEvent(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys)
{
	bool result = ScrollEvent(ExitD, breakKeys);
	if (Event.What != evKeyDown) return result;
	switch (Event.Pressed.KeyCombination()) {
	case _QF_:
	case 'L':
	case __F7:
	case __F8:
	case _KP_:
	case _QB_:
	case _QK_:
	case __CTRL_F5:
	case __ALT_F8:
	case __CTRL_F3:
	case __HOME:
	case __END:
	case __CTRL_LEFT:
	case __CTRL_RIGHT:
	case _QX_:
	case _QE_:
	case 'Z':
	case 'W':
	case __CTRL_F6:
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

bool TextEditorEvents::MyGetEvent(TextEditor* editor, char& mode, BYTE SysLColor, std::string& LastS, WORD LastNr, bool IsWrScreen, bool bScroll, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys) {
	std::string OrigS = "    ";
	WORD ww;

	auto result = false;

	CtrlShiftAlt(editor, mode, LastS, LastNr, IsWrScreen);
	// *** Prekodovani klaves ***
	GetEvent();
	if (Event.What == evKeyDown)
		switch (Event.Pressed.KeyCombination()) {
		case 'S': Event.Pressed.Key()->wVirtualKeyCode = VK_LEFT; break;
		case 'D': Event.Pressed.Key()->wVirtualKeyCode = VK_RIGHT; break;
		case 'E': Event.Pressed.Key()->wVirtualKeyCode = VK_UP; break;
		case 'X': Event.Pressed.Key()->wVirtualKeyCode = VK_DOWN; break;
		case 'R': Event.Pressed.Key()->wVirtualKeyCode = VK_PRIOR; break;
		case 'C': Event.Pressed.Key()->wVirtualKeyCode = VK_NEXT; break;
			//case 'A': Event.Pressed.Key()->wVirtualKeyCode = _CtrlLeft_; break;
			//case 'F': Event.Pressed.Key()->wVirtualKeyCode = _CtrlRight_; break;
		case 'V': Event.Pressed.Key()->wVirtualKeyCode = VK_INSERT; break;
		case __CTRL_P:
		{
			Wr("^P", OrigS, mode, SysLColor);
			ww = Event.Pressed.KeyCombination();
			if (My2GetEvent())
			{
				Wr("", OrigS, mode, SysLColor);
				if (Event.Pressed.Char <= 0x31) {
					Event.Pressed.UpdateKey(CTRL + Event.Pressed.Char);
					//Event.KeyCode = (ww << 8) || Event.KeyCode;
				}
			}
			break;
		}
		case __CTRL_Q:
		{
			Wr("^Q", OrigS, mode, SysLColor);
			ww = Event.Pressed.KeyCombination();
			if (My2GetEvent())
			{
				Wr("", OrigS, mode, SysLColor);
				switch (Event.Pressed.KeyCombination()) {
				case 'S': Event.Pressed.Key()->wVirtualKeyCode = __HOME; break;
				case 'D': Event.Pressed.Key()->wVirtualKeyCode = __END; break;
				case 'R': Event.Pressed.Key()->wVirtualKeyCode = __CTRL_PAGEUP; break;
				case 'C': Event.Pressed.Key()->wVirtualKeyCode = __CTRL_PAGEDOWN; break;

				case 'E': case 'X': case 'Y':
				case 'L': case 'B': case 'K':
				case 'I': case 'F': case 'A': {
					break;
				}

				case '-': {
					mode = SinFM;
					screen.CrsBig();
					FrameDir = 0;
					result = true;
					ClrEvent();
					break;
				}
				case '=': {
					mode = DouFM;
					screen.CrsBig();
					FrameDir = 0;
					result = true;
					ClrEvent();
					break;
				}
				case '/': {
					mode = DelFM;
					screen.CrsBig();
					FrameDir = 0;
					result = true;
					ClrEvent();
					break;
				}
				default: {
					ClrEvent();
				}
				}
			}
			break;
		}
		case __CTRL_K:
		{
			Wr("^K", OrigS, mode, SysLColor);
			ww = Event.Pressed.KeyCombination();
			if (My2GetEvent())
			{
				Wr("", OrigS, mode, SysLColor);
				std::set<char> setKc = { 'B', 'K', 'H', 'S', 'Y', 'C', 'V', 'W', 'R', 'P', 'F', 'U', 'L', 'N' };
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
			Wr("^O", OrigS, mode, SysLColor);
			ww = Event.Pressed.KeyCombination();
			if (My2GetEvent())
			{
				Wr("", OrigS, mode, SysLColor);
				switch (Event.Pressed.Char) {
				case 'W': // wrap
				case 'R':
				case 'L':
				case 'J':
				case 'C': {
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

	switch (mode)
	{
	case SinFM:
	case DouFM:
	case DelFM: {
		result = true;
		break;
	}
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

bool TextEditorEvents::TestExitKeys(TextEditor* editor, char& mode, std::vector<EdExitD*>& ExitD, int& fs, stEditorParams& ep, LongStr*& sp, WORD key)
{
	for (auto& X : ExitD) {
		if (TestExitKey(key, X)) {  // nastavuje i EdBreak
			editor->TestKod();
			IndexT = editor->SetInd(editor->_textT, editor->_lenT, textIndex, positionOnActualLine);
			ScrT = ((TextLineNr - ScreenFirstLineNr + 1) << 8) + positionOnActualLine - BPos;
			LastTxtPos = IndexT; // +Part.PosP;
			TxtXY = ScrT + ((int)positionOnActualLine << 16);
			if (X->Typ == 'Q') {
				Event.Pressed.UpdateKey(key);
				Konec = true; EditT = false;
				return true;
			}
			switch (TypeT) {
			case FileT: {
				editor->TestUpdFile();
				delete[] editor->_textT; editor->_textT = nullptr;
				//CloseH(&TxtFH);
				CloseHandle(TxtFH);
				TxtFH = NULL;
				break;
			}
			case LocalT:
			case MemoT: {
				//DelEndT();

				char* T2 = new char[editor->_lenT + 2];
				memcpy(&T2[2], editor->_textT, editor->_lenT);
				delete[] editor->_textT;
				editor->_textT = T2;

				sp->A = editor->_textT;
				sp->LL = (WORD)editor->_lenT;
				if (TypeT == LocalT) {
					TWork.Delete(*LocalPPtr);
					*LocalPPtr = TWork.Store(sp->A, sp->LL);
				}
				else if (UpdatT) {
					UpdateEdTFld(sp);
					UpdatT = false;
				}
				delete sp; sp = nullptr;
				break;
			}
			}
			ep = SaveParams();
			screen.CrsHide();
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
			RestoreParams(ep);
			switch (TypeT) {
			case FileT: {
				fs = IndexT; // Part.PosP + IndexT;
				OpenTxtFh(mode);
				editor->ReadTextFile();
				SimplePrintHead();
				//while ((fs > Part.PosP + Part.LenP) && !AllRd) { RdNextPart(); }
				IndexT = fs; // fs - Part.PosP;
				break;
			}
			case LocalT:
			case MemoT:
			{
				if (TypeT == LocalT) sp = TWork.Read(*LocalPPtr);
				else {
					CRecPtr = EditDRoot->NewRecPtr;
					sp = CFile->loadLongS(CFld->FldD, CRecPtr);
				}
				editor->_lenT = sp->LL;
				// _textT = (CharArr*)(sp)
				Move(&editor->_textT[3], &editor->_textT[1], editor->_lenT);
				break;
			}
			}

			editor->WrEndT();
			IndexT = MinW(IndexT, editor->_lenT);
			if (TypeT != FileT) {
				AbsLenT = editor->_lenT - 1;
				//Part.LenP = AbsLenT;
				SimplePrintHead();
			}
			editor->SetScreen(IndexT, ScrT, positionOnActualLine);
			if (!bScroll) {
				screen.CrsShow();
			}
			if (!EdOk) {
				return true;
			}
		}
	}
	return false;
}

void TextEditorEvents::HandleEvent(TextEditor* editor, char& mode, bool& IsWrScreen, BYTE SysLColor, std::string& LastS, WORD LastNr, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys) {
	wwmix wwmix1;
	WORD I = 0, I1 = 0;
	short I2 = 0, I3 = 0;
	HANDLE F1 = nullptr;
	WORD W1 = 0, W2 = 0;
	int L1 = 0, L2 = 0, fs = 0;
	stEditorParams ep;
	std::string ss;
	int j = 0;
	CHAR_INFO LastL[161];
	LongStr* sp = nullptr;
	void* P1 = nullptr;
	bool bb = false;
	//EdExitD* X = nullptr;

	IsWrScreen = false;

	//if (!MyGetEvent(editor, mode, SysLColor, LastS, LastNr, IsWrScreen, bScroll, ExitD, breakKeys)) {
	//	ClrEvent();
	//	IsWrScreen = false;
	//	return;
	//}

	std::string OrigS = "    ";
	CtrlShiftAlt(editor, mode, LastS, LastNr, IsWrScreen);
	GetEvent();
	TextEditorMode tm = _modes_handler->GetMode();

	while (true) {
		if ((tm != TextEditorMode::normal) || (Event.What == evKeyDown && Event.Pressed.Ctrl() && Event.Pressed.Char > 0)) {
			// mode is not normal || Ctrl key pressed with any other key
			switch (tm = _modes_handler->HandleKeyPress(Event.Pressed)) {
			case TextEditorMode::CtrlK: {
				Wr("^K", OrigS, mode, SysLColor);
				break;
			}
			case TextEditorMode::CtrlO: {
				Wr("^O", OrigS, mode, SysLColor);
				break;
			}
			case TextEditorMode::CtrlP: {
				Wr("^P", OrigS, mode, SysLColor);
				break;
			}
			case TextEditorMode::CtrlQ: {
				Wr("^Q", OrigS, mode, SysLColor);
				break;
			}
			default: {
				Wr("", OrigS, mode, SysLColor);
				break;
			}
			}

			if (   tm == TextEditorMode::CtrlK 
				|| tm == TextEditorMode::CtrlO 
				|| tm == TextEditorMode::CtrlP 
				|| tm == TextEditorMode::CtrlQ) {
				ClrEvent();
				GetEvent();
				continue;
			} else if ((tm == TextEditorMode::SingleFrame || tm == TextEditorMode::DoubleFrame 
				|| tm == TextEditorMode::DeleteFrame || tm == TextEditorMode::NoFrame) && !bScroll) {
				editor->FrameStep(FrameDir, Event.Pressed);
				ClrEvent();
				GetEvent();
				continue;
			}
			else {
				break;
			}
		}
		else {
			break;
		}
	}
	if (!bScroll) {
		editor->CleanFrame(ExitD, breakKeys);
	}
	//NewExit(Ovr(), er);
	//goto Opet;
	if (Event.What == evKeyDown) {
		WORD key = Event.Pressed.KeyCombination();
		EdOk = false;
		ClrEvent();
		//X = ExitD; // Exit-procedure

		if (TestExitKeys(editor, mode, ExitD, fs, ep, sp, key)) {
			goto Nic;
		}

		// test frame drawing mode
		if ((mode == SinFM || mode == DouFM || mode == DelFM || mode == NotFM) && !bScroll) {
			editor->FrameStep(FrameDir, Event.Pressed);
		}
		else if (Event.Pressed.isChar() || (key >= CTRL + '\x01' && key <= CTRL + '\x31')) {
			// printable character
			WrCharE(Lo(key));
			if (Wrap) {
				if (positionOnActualLine > RightMarg + 1) {
					W1 = Arr[positionOnActualLine];
					Arr[positionOnActualLine] = 0xFF;
					editor->KodLine();
					I1 = LeftMarg;
					while (Arr[I1] == ' ') { I1++; }
					if (I1 > RightMarg) { I1 = RightMarg; }
					L1 = textIndex; // Part.PosP + textIndex;
					editor->Format(I, L1, AbsLenT + editor->_lenT /*- Part.LenP*/, I1, false);
					editor->SetPart(L1);
					I = 1;
					// TODO: tady se pouzivalo 'I' ve FindCharPosition, ale k cemu je???
					I = editor->FindCharPosition(editor->_textT, editor->_lenT, 0xFF, 0);
					editor->_textT[I] = W1;
					TextLineNr = editor->GetLineNumber(I);
					positionOnActualLine = I - textIndex + 1;
				}
			}
		}
		else {
			// control key
			switch (key) {
			case __ENTER: {
				if (mode == HelpM) {
					Konec = WordExist();
					Event.Pressed.UpdateKey(key);
				}
				else {
					//if ((NextLineStartIndex >= _lenT) && !AllRd) NextPartDek();
					if ((NextLineStartIndex > editor->_lenT) || Insert) {
						editor->NewLine('m');
						positionOnActualLine = 1;
						ClrEol(TextAttr);
						if (TextLineNr - ScreenFirstLineNr == PageS) {
							screen.GotoXY(1, 1);
							//MyDelLine();
							ScreenFirstLineNr++;
							ChangeScr = true;
						}
						else {
							screen.GotoXY(1, succ(TextLineNr - ScreenFirstLineNr));
							//MyInsLine();
						}
						if (Indent) {
							I1 = editor->SetPredI();
							I = I1;
							while ((editor->_textT[I] == ' ') && (editor->_textT[I] != __CR)) { I++; } // tento radek je nesmyslny
							if (editor->_textT[I] != __CR) { positionOnActualLine = I - I1 + 1; }
						}
						else if (Wrap) {
							positionOnActualLine = LeftMarg;
						}
						if (TestLastPos(1, positionOnActualLine)) {
							FillChar(&Arr[1], positionOnActualLine - 1, 32);
						}
					}
					else if (NextLineStartIndex <= editor->_lenT) {
						editor->NextLine(true);
						positionOnActualLine = 1;
					}
				}
				break;
			}
			case __LEFT: {
				if (mode == HelpM) { editor->HelpLU('L'); }
				else
					if (bScroll) {
						if (columnOffset > 0) {
							Colu = columnOffset;
							positionOnActualLine = Position(Colu);
						}
					}
					else {
						I1 = positionOnActualLine;
						if (positionOnActualLine > 1) positionOnActualLine--;
						BlockLRShift(I1);
					}
				break;
			}
			case __RIGHT: {
				if (mode == HelpM) editor->HelpRD('R');
				else {
					if (bScroll) {
						positionOnActualLine = MinI(LineMaxSize, Position(columnOffset + LineS + 1));
						Colu = Column(positionOnActualLine);
					}
					else {
						I1 = positionOnActualLine;
						if (positionOnActualLine < LineMaxSize) positionOnActualLine++;
						BlockLRShift(I1);
					}
				}
				break;
			}
			case __UP: {
				if (mode == HelpM) {
					editor->HelpLU('U');
				}
				else {
					if (bScroll) {
						if (RScrL == 1) goto Nic;
					}
					L1 = blocks->LineAbs(TextLineNr);
					editor->PreviousLine();
					BlockUDShift(L1);
					if (bScroll) positionOnActualLine = Position(Colu);
				}
				break;
			}
			case __DOWN: {
				if (mode == HelpM) editor->HelpRD('D');
				else {
					L1 = blocks->LineAbs(TextLineNr); // na kterem jsme prave radku textu (celkove, ne na obrazovce)
					editor->NextLine(true);
					BlockUDShift(L1);
					if (bScroll) positionOnActualLine = Position(Colu);
				}
				break;
			}
			case __PAGEUP: {
				if (mode == HelpM) { editor->TestKod(); }
				else {
					ClrWord();
					TextLineNr = ScreenFirstLineNr;
				}
				L1 = blocks->LineAbs(TextLineNr);
				if (bScroll) {
					RScrL = MaxL(1, RScrL - PageS);
					if (ModPage(RScrL)) { RScrL++; }
					ScreenFirstLineNr = NewL(RScrL);
					TextLineNr = ScreenFirstLineNr;
					editor->DekFindLine(blocks->LineAbs(TextLineNr));
					positionOnActualLine = Position(Colu);
					j = editor->CountChar(editor->_textT, editor->_lenT, 0x0C, textIndex, ScreenIndex);
					if ((j > 0) && InsPg) {
						editor->DekFindLine(blocks->LineAbs(TextLineNr + j));
						ScreenFirstLineNr = TextLineNr;
						RScrL = NewRL(ScreenFirstLineNr);
					}
				}
				else {
					if (ScreenFirstLineNr > PageS) {
						ScreenFirstLineNr -= PageS;
					}
					else {
						ScreenFirstLineNr = 1;
					}
					editor->DekFindLine(blocks->LineAbs(TextLineNr - PageS));
				}
				ChangeScr = true;
				if (mode == HelpM) {
					ScreenIndex = editor->GetLineStartIndex(ScreenFirstLineNr);
					positionOnActualLine = Position(Colu);
					if (editor->WordFind(editor->WordNo2() + 1, I1, I2, WordL) && WordExist()) {
						editor->SetWord(I1, I2);
					}
					else { WordL = 0; }
				}
				else { BlockUDShift(L1); }
				break;
			}
			case __PAGEDOWN: {
				if (mode != HelpM) editor->TestKod();
				else {
					ClrWord();
					TextLineNr = ScreenFirstLineNr;
				}
				L1 = blocks->LineAbs(TextLineNr);
				if (bScroll) {
					RScrL += PageS;
					if (ModPage(RScrL)) {
						RScrL--;
					}
					editor->DekFindLine(blocks->LineAbs(NewL(RScrL)));
					positionOnActualLine = Position(Colu);
					j = editor->CountChar(editor->_textT, editor->_lenT, 0x0C, ScreenIndex, textIndex);
					if ((j > 0) && InsPg) {
						editor->DekFindLine(blocks->LineAbs(TextLineNr - j));
					}
					ScreenFirstLineNr = TextLineNr;
					RScrL = NewRL(ScreenFirstLineNr);
				}
				else {
					editor->DekFindLine(blocks->LineAbs(TextLineNr) + PageS);
					if (TextLineNr >= ScreenFirstLineNr + PageS) {
						ScreenFirstLineNr += PageS;
					}
				}
				ChangeScr = true;
				if (mode == HelpM) {
					ScreenIndex = editor->GetLineStartIndex(ScreenFirstLineNr);
					positionOnActualLine = Position(Colu);
					W1 = editor->WordNo2();
					I3 = WordL;
					if (editor->WordFind(W1 + 1, I1, I2, WordL) && WordExist()) {
						editor->SetWord(I1, I2);
					}
					else if (editor->WordFind(W1, I1, I2, WordL) && WordExist()) {
						editor->SetWord(I1, I2);
					}
					else {
						WordL = 0;
					}
				}
				else {
					BlockUDShift(L1);
				}
				break;
			}
			case __CTRL_LEFT: {
				do {
					positionOnActualLine--;
					if (positionOnActualLine == 0) {
						I = textIndex;
						editor->PreviousLine();
						if ((I > 1) || ChangePart) positionOnActualLine = GetArrLineLength();
						goto label1;
					}
				} while (Separ.count(Arr[positionOnActualLine]) > 0);

				while (!(Separ.count(Arr[positionOnActualLine]) > 0)) {
					positionOnActualLine--;
					if (positionOnActualLine == 0) goto label1;
				}
			label1:
				positionOnActualLine++;
				break;
			}
			case __CTRL_RIGHT:
			{
				while (!(Separ.count(Arr[positionOnActualLine]) > 0)) {
					positionOnActualLine++;
					if (positionOnActualLine > GetArrLineLength()) {
						goto label2;
					}
				}
				while (Separ.count(Arr[positionOnActualLine]) > 0) {
					positionOnActualLine++;
					I = GetArrLineLength();
					if (positionOnActualLine > I) {
						if ((NextLineStartIndex <= editor->_lenT) && ((I == 0) || (positionOnActualLine > I + 1))) {
							editor->NextLine(true);
							positionOnActualLine = 1;
						}
						else {
							positionOnActualLine = I + 1;
							goto label2;
						}
					}
				}
			label2:
				break; }
			case 'Z': {
				editor->RollNext();
				break;
			}
			case 'W': {
				editor->RollPred();
				break;
			}
			case __HOME: {
				I1 = positionOnActualLine;
				positionOnActualLine = 1;
				if (Wrap) {
					positionOnActualLine = MaxI(LeftMarg, 1);
				}
				BlockLRShift(I1);
				break;
			}
			case __END: {
				I1 = positionOnActualLine;
				positionOnActualLine = GetArrLineLength();
				if (positionOnActualLine < LineMaxSize) {
					positionOnActualLine++;
				}
				BlockLRShift(I1);
				break;
			}
			case _QE_: {
				editor->TestKod();
				TextLineNr = ScreenFirstLineNr;
				textIndex = ScreenIndex;
				editor->DekodLine(textIndex);
				break;
			}
			case _QX_: {
				editor->TestKod();
				editor->DekFindLine(blocks->LineAbs(ScreenFirstLineNr + PageS - 1));
				break;
			}
			case __CTRL_PAGEUP: {
				editor->TestKod();
				editor->SetPart(1);
				editor->SetScreen(1, 0, 0);
				break;
			}
			case __CTRL_PAGEDOWN: {
				editor->TestKod();
				editor->SetPart(AbsLenT /* - Part.LenP*/ + editor->_lenT);
				editor->SetScreen(editor->_lenT, 0, 0);
				break;
			}
			case __CTRL_F3: {
				ss = "";
				editor->TestKod();
				do {
					if (MyPromptLL(420, ss)) goto Nic;
					val(ss, L1, I);
				} while (!(L1 > 0));
				editor->DekFindLine(L1);
				break;
			}
			case __CTRL_N: {
				editor->NewLine('n');
				ClrEol(TextAttr);
				screen.GotoXY(1, TextLineNr - ScreenFirstLineNr + 2);
				//MyInsLine();
				break;
			}

			case __INSERT: {
				Insert = !Insert;
				break;
			}
			case __DELETE:
			case 'G': {
				if (positionOnActualLine <= GetArrLineLength()) {
					DelChar();
				}
				else {
					editor->DeleteLine();
				}
				break;
			}
			case __BACK: {
				if (positionOnActualLine > 1) {
					positionOnActualLine--;
					DelChar();
				}
				else {
					if (textIndex > 1) {
						editor->TestKod();
						TextLineNr--;
						textIndex = editor->GetLineStartIndex(TextLineNr);
						editor->CopyCurrentLineToArr(textIndex);
						positionOnActualLine = MinW(255, succ(GetArrLineLength()));
						editor->DeleteLine();
						if (TextLineNr < ScreenFirstLineNr) {
							ScreenFirstLineNr--;
							ChangeScr = true;
						}
					}
				}
				break;
			}
			case __CTRL_Y: {
				// if ((NextLineStartIndex >= _lenT) && !AllRd) NextPartDek();
				NextLineStartIndex = MinW(NextLineStartIndex, editor->_lenT);
				//TestLenText(&_textT, _lenT, NextLineStartIndex, textIndex);
				UpdatT = true;
				if (blocks->BegBLn > blocks->LineAbs(TextLineNr)) {
					blocks->BegBLn--;
				}
				else if (blocks->BegBLn == blocks->LineAbs(TextLineNr)) {
					if (TypeB == TextBlock) {
						blocks->BegBPos = 1;
					}
				}
				if (blocks->EndBLn >= blocks->LineAbs(TextLineNr)) {
					if ((blocks->EndBLn == blocks->LineAbs(TextLineNr)) && (TypeB == TextBlock)) {
						BPos = 1;
					}
					else {
						blocks->EndBLn--;
					}
				}
				editor->DeleteLine();
				editor->DekodLine(textIndex);
				positionOnActualLine = 1;
				break;
			}
			case __CTRL_T: {
				if (positionOnActualLine > GetArrLineLength()) {
					editor->DeleteLine();
				}
				else {
					I = positionOnActualLine;
					if (Separ.count(Arr[positionOnActualLine]) > 0) {
						DelChar();
					}
					else {
						while ((I <= GetArrLineLength()) && !(Separ.count(Arr[positionOnActualLine]) > 0)) {
							I++;
						}
					}
					while ((I <= GetArrLineLength()) && (Arr[I] == ' ')) {
						I++;
					}
					// TODO: k cemu to tady je? if ((I>positionOnActualLine) and TestLastPos(I,positionOnActualLine))
				}
				break;
			}
			case _QI_: { Indent = !Indent; break; }
			case _QL_: {
				// CTRL Q+L - obnoveni obsahu radku (pouze editovany radek)
				if (UpdatedL) editor->DekodLine(textIndex);
				break;
			}
			case _QY_: {
				// CTRL Q+Y - vymaz od pozice kurzoru do konce radku
				if (TestLastPos(GetArrLineLength() + 1, positionOnActualLine)) ClrEol(TextAttr);
				break;
			}
			case _QF_:
			case _QA_: {
				Replace = false;
				if (MyPromptLL(405, FindStr)) goto Nic;
				if (key == _QA_)
				{
					if (MyPromptLL(407, ReplaceStr)) goto Nic;
					Replace = true;
				}
				ss = OptionStr;
				if (MyPromptLL(406, ss)) goto Nic;
				OptionStr = ss;
				editor->TestKod();
				if (TestOptStr('l') && (!BlockExist() || (TypeB == ColBlock))) goto Nic;
				if (TestOptStr('l')) editor->SetBlockBound(L1, L2);
				else {
					L2 = AbsLenT /* - Part.LenP */ + editor->_lenT;
					if (TestOptStr('g') || TestOptStr('e'))  L1 = 1;
					else L1 = /* Part.PosP + */ editor->SetInd(editor->_textT, editor->_lenT, textIndex, positionOnActualLine);
				}
				editor->FindReplaceString(L1, L2);
				if (key == _QA_) editor->DekodLine(textIndex);
				if (!Konec) { FirstEvent = false; editor->Background(); }
				break;
			}
			case 'L': {
				if (!FindStr.empty()) {
					editor->TestKod();
					if (TestOptStr('l') && (!BlockExist() || (TypeB == ColBlock))) goto Nic;
					fs = 1;
					L1 = /* Part.PosP + */ editor->SetInd(editor->_textT, editor->_lenT, textIndex, positionOnActualLine);
					if (TestOptStr('l')) editor->SetBlockBound(fs, L2);
					else L2 = AbsLenT /* - Part.LenP */ + editor->_lenT;
					if (L1 < fs)  L1 = fs;  // { if L1>=L2  goto Nic;}
					editor->FindReplaceString(L1, L2);
					if (!Konec) { FirstEvent = false; editor->Background(); }
				}
				break;
			}
			case 'I': {
				I1 = editor->SetPredI() + positionOnActualLine;
				if (I1 >= textIndex - 1) goto Nic;
				I = I1;
				while ((editor->_textT[I] != ' ') && (editor->_textT[I] != __CR)) { I++; }
				while (editor->_textT[I] == ' ') { I++; }
				I2 = I - I1 + 1;
				if (TestLastPos(positionOnActualLine, positionOnActualLine + I2)) FillChar(&Arr[positionOnActualLine], I2, 32);
				positionOnActualLine += I2;
				break;
			}
			case __CTRL_J: {
				// tabelace zprava
				I1 = editor->SetPredI() + positionOnActualLine - 2;
				if ((I1 >= textIndex - 1) || (I1 == 0)) goto Nic;
				I = I1;
				while (editor->_textT[I] == ' ') { I++; }
				while ((editor->_textT[I] != ' ') && (editor->_textT[I] != __CR)) { I++; }
				if (I == I1) goto Nic;
				I2 = I - I1 - 1;
				I = positionOnActualLine;
				positionOnActualLine--;
				while ((positionOnActualLine > 0) && (Arr[positionOnActualLine] != ' ')) { positionOnActualLine--; }
				positionOnActualLine++;
				if (TestLastPos(positionOnActualLine, positionOnActualLine + I2)) FillChar(&Arr[positionOnActualLine], I2, 32);
				positionOnActualLine = I + I2 + 1;
				break;
			}
			case _QB_: {
				editor->TestKod();
				editor->PosDekFindLine(blocks->BegBLn, MinW(GetArrLineLength() + 1, blocks->BegBPos), false); break; }
			case _QK_: {
				editor->TestKod();
				editor->PosDekFindLine(blocks->EndBLn, MinW(GetArrLineLength() + 1, blocks->EndBPos), false); break; }
			case _KB_:
			case __F7:
			case _KH_: {
				blocks->BegBLn = blocks->LineAbs(TextLineNr);
				if (TypeB == TextBlock) blocks->BegBPos = MinI(GetArrLineLength() + 1, positionOnActualLine);
				else blocks->BegBPos = positionOnActualLine;
				if (key == _KH_) goto OznB;
				break;
			}
			case _KK_:
			case __F8: {
			OznB:
				blocks->EndBLn = blocks->LineAbs(TextLineNr);
				if (TypeB == TextBlock) blocks->EndBPos = MinI(GetArrLineLength() + 1, positionOnActualLine);
				else blocks->EndBPos = positionOnActualLine;
				break;
			}
			case _KN_: {
				if (TypeB == TextBlock) TypeB = ColBlock;
				else TypeB = TextBlock;
				break;
			}
			case _KY_: {
				if (editor->BlockHandle(fs, F1, 'Y')) { blocks->EndBLn = blocks->BegBLn; blocks->EndBPos = blocks->BegBPos; }
				break;
			}
			case _KC_: editor->BlockCopyMove('C', P1, sp); break;
			case _KV_: editor->BlockCopyMove('M', P1, sp); break;
			case _KU_: editor->BlockHandle(fs, F1, 'U'); break;
			case _KL_: editor->BlockHandle(fs, F1, 'L'); break;
			case __CTRL_F7: {
				if (TypeB == TextBlock) editor->BlockGrasp('G', P1, sp);
				else editor->BlockCGrasp('G', P1, sp);
				break;
			}
			case _KW_: {
				I1 = blocks->BegBLn; I2 = blocks->BegBPos; I3 = blocks->EndBLn; I = blocks->EndBPos; bb = TypeB;
				if (!BlockExist()) {
					blocks->BegBLn = 1; blocks->EndBLn = 0x7FFF; blocks->BegBPos = 1; blocks->EndBPos = 0xFF; TypeB = TextBlock;
				}
				CPath = wwmix1.SelectDiskFile(".TXT", 401, false);
				if (CPath.empty()) goto Nic;
				CVol = "";
				F1 = OpenH(CPath, _isNewFile, Exclusive);
				if (HandleError == 80)
				{
					SetMsgPar(CPath);
					if (PromptYN(780)) F1 = OpenH(CPath, _isOverwriteFile, Exclusive);
					else goto Nic;
				}
				if (HandleError != 0) { MyWrLLMsg(CPath); goto Nic; }
				fs = 0; // {L1 =blocks->LineAbs(TextLineNr);I =positionOnActualLine;}
				if (editor->BlockHandle(fs, F1, 'W')) {
					WriteH(F1, 0, editor->_textT);
					/*truncH*/
					CloseH(&F1);
					HMsgExit(CPath);
				}
				// { PosDekFindLine(L1,I,true); }
				blocks->BegBLn = I1; blocks->BegBPos = I2; blocks->EndBLn = I3; blocks->EndBPos = I; TypeB = bb;
				break;
			}
			case __SHIFT_F7: {
				if (TypeB == TextBlock) { editor->BlockDrop('D', P1, sp); }
				else { editor->BlockCDrop('D', P1, sp); }
				break;
			}
			case _KR_: {
				CPath = wwmix1.SelectDiskFile(".TXT", 400, false);
				if (CPath.empty()) goto Nic;
				CVol = "";
				F1 = OpenH(CPath, _isOldFile, RdOnly);
				if (HandleError != 0) { MyWrLLMsg(CPath); goto Nic; }
				blocks->BegBLn = /* Part.LineP + */ TextLineNr;
				blocks->BegBPos = positionOnActualLine;
				L1 = /* Part.PosP + */ textIndex + positionOnActualLine - 1;
				editor->FillBlank();
				fs = FileSizeH(F1); L2 = 0;
				//NullChangePart();
				switch (TypeB) {
				case TextBlock: {
					do {
						I2 = 0x1000; if (fs - L2 < int(I2))  I2 = fs - L2;
						if ((TypeT != FileT) && ((I2 >= MaxLenT - editor->_lenT) || (I2 >= StoreAvail()))) {
							if (I2 >= StoreAvail()) {
								I2 = StoreAvail();
							}
							I2 = MinW(I2, MaxLenT - editor->_lenT) - 2; fs = L2 + I2;
							WrLLF10Msg(404);
						}
						I1 = L1 + L2; // - Part.PosP;
						//TestLenText(&_textT, _lenT, I1, int(I1) + I2);
						UpdatT = true;
						//if (ChangePart) I1 -= Part.MovI;
						SeekH(F1, L2); ReadH(F1, I2, &editor->_textT[I1]); HMsgExit("");
						L2 += I2;
					} while (L2 != fs);
					I = L1 + L2; // - Part.PosP;
					if (editor->_textT[I - 1] == 0x1A) {
						//TestLenText(&_textT, _lenT, I, I - 1);
						UpdatT = true;
						I--;
					}
					TextLineNr = editor->GetLineNumber(I);
					blocks->EndBLn = TextLineNr; // Part.LineP + TextLineNr;
					blocks->EndBPos = succ(I - textIndex);
					break;
				}
				case ColBlock: {
					blocks->EndBPos = positionOnActualLine; I2 = 0x1000;
					MarkStore(P1);
					sp = new LongStr(I2 + 2); //ww =BegBPos;}
					do {
						if (fs - L2 < (int)I2) I2 = fs - L2;
						SeekH(F1, L2); ReadH(F1, I2, sp->A); HMsgExit("");
						L2 += I2; sp->LL = I2; editor->BlockCDrop('R', P1, sp);
					} while (L2 != fs);
					blocks->EndBLn = /*Part.LineP +*/ TextLineNr - 1;
					ReleaseStore(&P1);
					break;
				}
				}
				CloseH(&F1);
				HMsgExit("");
				SetPartLine(blocks->BegBLn);
				TextLineNr = editor->GetLineNumber(L1 /* - Part.PosP*/);
				UpdatedL = true;
				break;
			} // end case _KR_
			case _KP_: {
				if (!editor->BlockHandle(fs, F1, 'P')) {
					I1 = blocks->BegBLn; I2 = blocks->BegBPos; I3 = blocks->EndBLn; I = blocks->EndBPos; bb = TypeB;
					blocks->BegBLn = 1; blocks->EndBLn = 0x7FFF; blocks->BegBPos = 1; blocks->EndBPos = 0xFF;
					TypeB = TextBlock;
					blocks->BegBLn = I1; blocks->BegBPos = I2; blocks->EndBLn = I3; blocks->EndBPos = I; TypeB = bb;
				}
				break;
			case _KF_: {
				if (BlockExist() && (TypeB == TextBlock)) {
					editor->TestKod(); screen.CrsHide();
					SetPartLine(blocks->EndBLn);
					I2 = blocks->EndBLn; // -Part.LineP;
					size_t nextLineIdx = editor->GetLineStartIndex(I2);
					L1 = editor->SetInd(editor->_textT, editor->_lenT, nextLineIdx, blocks->EndBPos); // +Part.PosP;
					L2 = blocks->BegBLn; positionOnActualLine = blocks->BegBPos;
					SetPartLine(L2);
					I2 = blocks->BegBLn; // -Part.LineP;
					nextLineIdx = editor->GetLineStartIndex(I2);
					editor->Format(I, nextLineIdx /* + Part.PosP */, L1, blocks->BegBPos, true);
					editor->DekFindLine(L2);
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
					screen.ScrRdBuf(FirstC, TxtRows, LastL, LineS);
					LastL[MargLL[0]].Attributes = MargLL[1] >> 8;
					LastL[MargLL[0]].Char.AsciiChar = MargLL[1] & 0x00FF;
					LastL[MargLL[2]].Attributes = MargLL[3] >> 8;
					LastL[MargLL[2]].Char.AsciiChar = MargLL[3] & 0x00FF;
					screen.ScrWrBuf(FirstC, TxtRows, LastL, LineS);
				}
				break;
			}
			case _OL_: {       // LeftMarg
				do {
					ss = std::to_string(positionOnActualLine);
					if (MyPromptLL(410, ss)) goto Nic;
					val(ss, I1, I);
				} while (!((I1 < RightMarg) && (I1 > 0)));
				LeftMarg = I1;
				break;
			}
			case _OR_: {       //RightMarg
				do {
					ss = std::to_string(positionOnActualLine);
					if (MyPromptLL(409, ss)) goto Nic;
					val(ss, I1, I); // inc(I1);
				} while (!((I1 <= 255) && (LeftMarg < I1)));
				RightMarg = I1;
				break;
			}
			case _OC_: {
				I1 = 1;
				while ((I1 < GetArrLineLength()) && (Arr[I1] == ' ')) { I1++; }
				I2 = GetArrLineLength();
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
				positionOnActualLine = MinW(LineMaxSize, GetArrLineLength() + 1);
				break;
			}
			case 'B': {
				editor->TestKod();
				L1 = /*Part.PosP +*/ textIndex;
				editor->Format(I, L1, AbsLenT + editor->_lenT /* - Part.LenP*/, MinI(LeftMarg, positionOnActualLine), false);
				editor->SetPart(L1);
				I2 = L1; // -Part.PosP;
				TextLineNr = editor->GetLineNumber(I2);
				positionOnActualLine = 1;
				break;
			}
					//case _framesingle_: {
					//	Mode = SinFM;
					//	screen.CrsBig();
					//	FrameDir = 0;
					//	break;
					//}
					//case _framedouble_: {
					//	Mode = DouFM;
					//	screen.CrsBig();
					//	FrameDir = 0;
					//	break;
					//}
					//case _delframe_: {
					//	Mode = DelFM;
					//	screen.CrsBig();
					//	FrameDir = 0;
					//	break;
					//}
			case __F4: {
				char c = ToggleCS(Arr[positionOnActualLine]);
				UpdatedL = c != Arr[positionOnActualLine];
				Arr[positionOnActualLine] = c;
				break;
			}
			case __CTRL_F5:
				Calculate();
				break;
			case __CTRL_F6: {
				if ((TypeT == FileT) || (TypeT == LocalT)) {
					editor->BlockHandle(fs, F1, 'p');
				}
				break;
			}
			case __ALT_F8: {
				ep = SaveParams();
				W1 = Menu(45, spec.KbdTyp + 1);
				if (W1 != 0) {
					spec.KbdTyp = TKbdConv(W1 - 1);
				}
				RestoreParams(ep);
				break;
			}
			case 0x1000: {
			Opet:
				if ((mode != HelpM) && (mode != ViewM) && Wrap) {
					screen.Window(FirstC, FirstR + 1, LastC + 1, LastR);
				}
				else {
					screen.Window(FirstC, FirstR + 1, LastC, LastR);
				}

				if (!bScroll) {
					screen.CrsShow();
				}
				editor->SetScreen(textIndex, 0, 0);
				break;
			}
			case 'U': { // previous state recovery
				if (TypeT != FileT)
					if (PromptYN(108)) {
						IndexT = 1;
						Event.Pressed.UpdateKey('U');
						Konec = true;
						EdBreak = 0xFFFF;
					}
				break;
			}
			case __ESC: {
				editor->TestKod();
				//Event.Pressed.UpdateKey(key);
				Konec = true;
				EdBreak = 0;
				break;
			}
			case __ALT_EQUAL: { // end without saving
				if (TypeT != FileT) {
					editor->TestKod();
					//Event.Pressed.UpdateKey(key);
					Konec = true;
					EdBreak = 0xFFFF;
				}
				break;
			}
			default: {
				if (std::find(breakKeys.begin(), breakKeys.end(), key) != breakKeys.end()) {
					// *** BREAK KEYS ***
					editor->TestKod();
					//KbdChar: = ww;
					Konec = true;
					EdBreak = 0xFFFF;
				}
				else if (key >= 0x1000 && key <= 0x101F) {
					WrCharE(Lo(key)); // ***CTRL-klavesy***
					if (key == 0x100D) {
						editor->TestKod();
						editor->DekodLine(textIndex);
						positionOnActualLine--;
					}
				}
				break;
				// ***ERROR TESTLENTEXT***
			}
			}
			}
		} // else
	} // if (Event.What == evKeyDown)

	else if ((Event.What == evMouseDown) && ((mode == HelpM) || (mode == TextM)))
	{
		if (mode == TextM) editor->TestKod();
		if (!((Event.Where.Y >= FirstR && Event.Where.Y <= LastR - 1)
			&& (Event.Where.X >= FirstC - 1 && Event.Where.X <= LastC - 1)))
		{
			ClrEvent();
			goto Nic;
		}
		I3 = textIndex;
		j = positionOnActualLine;
		W1 = Event.Where.Y - WindMin.Y + ScreenFirstLineNr;
		if (mode == HelpM) {
			W2 = editor->WordNo2() + 1;
		}
		editor->DekFindLine(blocks->LineAbs(W1));
		positionOnActualLine = Event.Where.X - WindMin.X + 1;
		if (mode != TextM) positionOnActualLine = Position(positionOnActualLine);
		positionOnActualLine += BPos;
		I = editor->SetInd(editor->_textT, editor->_lenT, textIndex, positionOnActualLine);
		if (I < editor->_lenT) {
			if (mode == HelpM) {
				ClrWord();
				editor->WordFind(editor->WordNo(I + 1), I1, I2, W1);
				if ((I1 <= I) && (I2 >= I)) {
					editor->SetWord(I1, I2);
					Event.Pressed.UpdateKey('M');
					Konec = true;
				}
				else if (WordExist()) {
					editor->WordFind(W2, I1, I2, W1);
					editor->SetWord(I1, I2);
				}
				else {
					TextLineNr = editor->GetLineNumber(I3);
				}
			}
		}
		else {
			TextLineNr = editor->GetLineNumber(I3);
			positionOnActualLine = (WORD)j;
		}
		ClrEvent();
	} // else if ((Event.What == evMouseDown) && ((Mode == HelpM) || (Mode == TextM)))
	else {
		ClrEvent();
	}

Nic:
	//ClrEvent;
	IsWrScreen = false;
}
