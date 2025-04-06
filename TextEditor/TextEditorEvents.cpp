#include "TextEditorEvents.h"

#include "../Drivers/constants.h"
#include "../Core/wwmix.h"
#include "../Core/GlobalVariables.h"
#include "../Core/runproc.h"

#include <cstdio>
#include <memory>
#include <set>

#include "TextEditor.h"
#include "../DataEditor/DataEditor.h"
#include "../Core/obaseww.h"
#include "../Core/wwmenu.h"
#include "../Common/textfunc.h"


TextEditorEvents::TextEditorEvents()
{
	_modes_handler = new TextEditorSpecialModes(this);
}

TextEditorEvents::~TextEditorEvents()
{
	delete _modes_handler;
	_modes_handler = nullptr;
}

bool TextEditorEvents::CtrlShiftAlt(TextEditor* editor, EditorMode mode, std::string& LastS, WORD LastNr, bool IsWrScreen)
{
	bool Ctrl = false;
	WORD Delta = 0;
	WORD flgs = 0;
	//(*MyTestEvent 1; *)

label1:
	WaitEvent(Delta);

	if (mode != EditorMode::Help) {
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

	return true;
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

void TextEditorEvents::Wr(std::string s, std::string& OrigS, EditorMode mode, BYTE SysLColor)
{
	CHAR_INFO ci2[2];
	if (mode != EditorMode::Help) {
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

bool TextEditorEvents::MyGetEvent(TextEditor* editor, EditorMode& mode, BYTE SysLColor, std::string& LastS, WORD LastNr, bool IsWrScreen, bool bScroll, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys) {
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
					mode = EditorMode::FrameSingle;
					screen.CrsBig();
					FrameDir = 0;
					result = true;
					ClrEvent();
					break;
				}
				case '=': {
					mode = EditorMode::FrameDouble;
					screen.CrsBig();
					FrameDir = 0;
					result = true;
					ClrEvent();
					break;
				}
				case '/': {
					mode = EditorMode::DeleteFrame;
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
	case EditorMode::FrameSingle:
	case EditorMode::FrameDouble:
	case EditorMode::DeleteFrame: {
		result = true;
		break;
	}
	case EditorMode::Help:
	{
		result = HelpEvent(breakKeys);
		break;
	}
	case EditorMode::View: {
		if (bScroll) result = ScrollEvent(ExitD, breakKeys);
		else result = ViewEvent(ExitD, breakKeys);
		break;
	}
	case EditorMode::Text: {
		if (bScroll) result = ScrollEvent(ExitD, breakKeys);
		else result = true;
		break;
	}
	default:;
	}
	return result;
}

bool TextEditorEvents::TestExitKeys(TextEditor* editor, EditorMode& mode, std::vector<EdExitD*>& ExitD, int& fs, LongStr*& sp, WORD key)
{
	std::unique_ptr<DataEditor> data_editor = std::make_unique<DataEditor>();
	std::string txt = JoinLines(editor->_lines);
	for (EdExitD*& X : ExitD) {
		if (TestExitKey(key, X)) {  // nastavuje i EdBreak
			if (UpdatedL) editor->KodLine();
			IndexT = editor->SetInd(textIndex, positionOnActualLine);
			ScrT = ((editor->TextLineNr - editor->ScreenFirstLineNr + 1) << 8) + positionOnActualLine - BPos;
			LastTxtPos = IndexT; // +Part.PosP;
			TxtXY = ScrT + ((int)positionOnActualLine << 16);
			if (X->Typ == 'Q') {
				Event.Pressed.UpdateKey(key);
				Konec = true; EditT = false;
				return true;
			}
			switch (editor->_text_type) {
			case TextType::File: {
				editor->TestUpdFile();
				//delete[] editor->_textT; editor->_textT = nullptr;
				//CloseH(&TxtFH);
				CloseHandle(TxtFH);
				TxtFH = NULL;
				break;
			}
			case TextType::Local:
			case TextType::Memo: {
				//DelEndT();

				// TODO: whas is this?
				/*char* T2 = new char[editor->_lenT + 2];
				memcpy(&T2[2], editor->_textT, editor->_lenT);
				delete[] editor->_textT;
				editor->_textT = T2;*/

				std::string data = txt;

				if (editor->_text_type == TextType::Local) {
					TWork.Delete(*LocalPPtr);
					*LocalPPtr = TWork.Store(data);
				}
				else if (UpdatT) {
					data_editor->UpdateEdTFld(data);
					UpdatT = false;
				}

				delete sp; sp = nullptr;
				break;
			}
			}
			//ep = SaveParams();
			screen.CrsHide();
			if (editor->_text_type == TextType::Memo) {
				data_editor->StartExit(X, false);
			}
			else {
				CallProcedure(X->Proc);
			}
			//NewExit(Ovr(), er);
			//goto Opet;
			if (!bScroll) {
				screen.CrsShow();
			}
			//RestoreParams(ep);
			switch (editor->_text_type) {
			case TextType::File: {
				fs = IndexT; // Part.PosP + IndexT;
				editor->OpenTxtFh(mode);
				editor->ReadTextFile();
				SimplePrintHead();
				//while ((fs > Part.PosP + Part.LenP) && !AllRd) { RdNextPart(); }
				IndexT = fs; // fs - Part.PosP;
				break;
			}
			case TextType::Local:
			case TextType::Memo: {
				std::string d = TWork.Read(*LocalPPtr);
					if (editor->_text_type == TextType::Local) {
				}
				else {
					throw("Check implementation! EditDRoot is probably not set.");
					CRecPtr = EditDRoot->NewRecPtr;
					//sp = CFile->loadLongS((*data_editor->CFld)->FldD, CRecPtr);
				}
				//editor->_lenT = sp->LL;
				//// _textT = (CharArr*)(sp)
				//Move(&editor->_textT[3], &editor->_textT[1], editor->_lenT);
					txt = d;
				break;
			}
			}

			//editor->WrEndT();
			IndexT = MinW(IndexT, txt.length());
			if (editor->_text_type != TextType::File) {
				AbsLenT = txt.length() - 1;
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

void TextEditorEvents::HandleEvent(TextEditor* editor, EditorMode& mode, bool& IsWrScreen, BYTE SysLColor, std::string& LastS, WORD LastNr, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys) {
	wwmix wwmix1;
	WORD I = 0;
	size_t I1 = 0, I2 = 0, I3 = 0;
	HANDLE F1 = nullptr;
	size_t W1 = 0, W2 = 0;
	int L1 = 0, L2 = 0, fs = 0;
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
	bool end = CtrlShiftAlt(editor, mode, LastS, LastNr, IsWrScreen);
	//if (end) {
	//	ClrEvent();
	//	return;
	//}
	GetEvent();
	TextEditorSpecialMode tm = _modes_handler->GetMode();

	std::string txt = JoinLines(editor->_lines);

	while (true) {
		if ((tm != TextEditorSpecialMode::normal) || (Event.What == evKeyDown && Event.Pressed.Ctrl() && Event.Pressed.Char > 0)) {
			// mode is not normal || Ctrl key pressed with any other key
			switch (tm = _modes_handler->HandleKeyPress(Event.Pressed)) {
			case TextEditorSpecialMode::CtrlK: {
				Wr("^K", OrigS, mode, SysLColor);
				break;
			}
			case TextEditorSpecialMode::CtrlO: {
				Wr("^O", OrigS, mode, SysLColor);
				break;
			}
			case TextEditorSpecialMode::CtrlP: {
				Wr("^P", OrigS, mode, SysLColor);
				break;
			}
			case TextEditorSpecialMode::CtrlQ: {
				Wr("^Q", OrigS, mode, SysLColor);
				break;
			}
			default: {
				Wr("", OrigS, mode, SysLColor);
				break;
			}
			}

			if (tm == TextEditorSpecialMode::CtrlK
				|| tm == TextEditorSpecialMode::CtrlO
				|| tm == TextEditorSpecialMode::CtrlP
				|| tm == TextEditorSpecialMode::CtrlQ) {
				ClrEvent();
				GetEvent();
				continue;
			}
			else if ((tm == TextEditorSpecialMode::SingleFrame || tm == TextEditorSpecialMode::DoubleFrame
				|| tm == TextEditorSpecialMode::DeleteFrame || tm == TextEditorSpecialMode::NoFrame) && !bScroll) {
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

		if (TestExitKeys(editor, mode, ExitD, fs, sp, key)) {
			goto Nic;
		}

		// test frame drawing mode
		if (!bScroll &&
			(mode == EditorMode::FrameSingle || mode == EditorMode::FrameDouble || mode == EditorMode::DeleteFrame || mode == EditorMode::NotFrame)) {
			editor->FrameStep(FrameDir, Event.Pressed);
		}
		else if (Event.Pressed.isChar() || (key >= CTRL + '\x01' && key <= CTRL + '\x31')) {
			// printable character
			editor->WrCharE(Lo(key));
			if (editor->Wrap) {
				if (positionOnActualLine > editor->RightMarg + 1) {
					W1 = editor->Arr[positionOnActualLine];
					editor->Arr[positionOnActualLine] = 0xFF;
					editor->KodLine();
					I1 = editor->LeftMarg;
					while (editor->Arr[I1] == ' ') { I1++; }
					if (I1 > editor->RightMarg) { I1 = editor->RightMarg; }
					L1 = textIndex; // Part.PosP + textIndex;
					editor->Format(I, L1, AbsLenT + txt.length(), I1, false);
					//editor->SetPart(L1);
					I = 1;
					// TODO: tady se pouzivalo 'I' ve FindCharPosition, ale k cemu je???
					I = editor->FindCharPosition(0xFF, 0);
					txt[I] = W1;
					editor->TextLineNr = editor->GetLineNumber(I);
					positionOnActualLine = I - textIndex + 1;
				}
			}
		}
		else {
			// control key
			switch (key) {
			case __ENTER: {
				if (mode == EditorMode::Help) {
					Konec = editor->WordExist();
					Event.Pressed.UpdateKey(key);
				}
				else {
					//if ((NextLineStartIndex >= _lenT) && !AllRd) NextPartDek();
					if (editor->TextLineNr == editor->_lines.size() /* && jsme_na_konci_radku? */ || editor->Insert) {
						editor->NewLine('m');
						positionOnActualLine = 1;
						// ClrEol(TextAttr);
						if (editor->TextLineNr - editor->ScreenFirstLineNr == PageS) {
							screen.GotoXY(1, 1);
							//MyDelLine();
							editor->ScreenFirstLineNr++;
							//editor->_change_scr = true;
						}
						else {
							screen.GotoXY(1, succ(editor->TextLineNr - editor->ScreenFirstLineNr));
							//MyInsLine();
						}
						if (editor->Indent) {
							//I1 = editor->SetPredI();
							//I = I1;
							//while ((txt[I] == ' ') && (txt[I] != __CR)) { I++; } // tento radek je nesmyslny
							//if (txt[I] != __CR) { positionOnActualLine = I - I1 + 1; }
						}
						else if (editor->Wrap) {
							positionOnActualLine = editor->LeftMarg;
						}
						//if (editor->TestLastPos(1, positionOnActualLine)) {
						//	FillChar(&editor->Arr[1], positionOnActualLine - 1, 32);
						//}
					}
					else if (NextLineStartIndex <= txt.length()) {
						editor->NextLine(true);
						positionOnActualLine = 1;
					}
				}
				break;
			}
			case __LEFT: {
				if (mode == EditorMode::Help) { editor->HelpLU('L'); }
				else
					if (bScroll) {
						if (columnOffset > 0) {
							Colu = columnOffset;
							positionOnActualLine = editor->Position(Colu);
						}
					}
					else {
						I1 = positionOnActualLine;
						if (positionOnActualLine > 1) positionOnActualLine--;
						editor->BlockLRShift(I1);
					}
				break;
			}
			case __RIGHT: {
				if (mode == EditorMode::Help) editor->HelpRD('R');
				else {
					if (bScroll) {
						positionOnActualLine = MinI(LineMaxSize, editor->Position(columnOffset + LineS + 1));
						Colu = editor->Column(positionOnActualLine);
					}
					else {
						I1 = positionOnActualLine;
						if (positionOnActualLine < LineMaxSize) positionOnActualLine++;
						editor->BlockLRShift(I1);
					}
				}
				break;
			}
			case __UP: {
				if (mode == EditorMode::Help) {
					editor->HelpLU('U');
				}
				else {
					if (bScroll) {
						if (RScrL == 1) goto Nic;
					}
					L1 = editor->LineAbs(editor->TextLineNr);
					editor->PreviousLine();
					editor->BlockUDShift(L1);
					if (bScroll) positionOnActualLine = editor->Position(Colu);
				}
				break;
			}
			case __DOWN: {
				if (mode == EditorMode::Help) {
					editor->HelpRD('D');
				}
				else {
					L1 = editor->LineAbs(editor->TextLineNr); // na kterem jsme prave radku textu (celkove, ne na obrazovce)
					editor->NextLine(true);
					editor->BlockUDShift(L1);
					if (bScroll) positionOnActualLine = editor->Position(Colu);
				}
				break;
			}
			case __PAGEUP: {
				editor->ProcessPageUp();
				break;
			}
			case __PAGEDOWN: {
				editor->ProcessPageDown();
				break;
			}
			case __SCROLL_LOCK: {
				editor->_change_scr = true;
				break;
				}
			case __CTRL_LEFT: {
				do {
					positionOnActualLine--;
					if (positionOnActualLine == 0) {
						I = textIndex;
						editor->PreviousLine();
						if (I > 1) {
							positionOnActualLine = editor->GetArrLineLength();
						}
						goto label1;
					}
				} while (Separ.count(editor->Arr[positionOnActualLine]) > 0);

				while (!(Separ.count(editor->Arr[positionOnActualLine]) > 0)) {
					positionOnActualLine--;
					if (positionOnActualLine == 0) goto label1;
				}
			label1:
				positionOnActualLine++;
				break;
			}
			case __CTRL_RIGHT:
			{
				while (!(Separ.count(editor->Arr[positionOnActualLine]) > 0)) {
					positionOnActualLine++;
					if (positionOnActualLine > editor->GetArrLineLength()) {
						goto label2;
					}
				}
				while (Separ.count(editor->Arr[positionOnActualLine]) > 0) {
					positionOnActualLine++;
					I = editor->GetArrLineLength();
					if (positionOnActualLine > I) {
						if ((NextLineStartIndex <= txt.length()) && ((I == 0) || (positionOnActualLine > I + 1))) {
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
				break;
			}
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
				if (editor->Wrap) {
					positionOnActualLine = MaxI(editor->LeftMarg, 1);
				}
				editor->BlockLRShift(I1);
				break;
			}
			case __END: {
				I1 = positionOnActualLine;
				positionOnActualLine = editor->GetArrLineLength();
				if (positionOnActualLine < LineMaxSize) {
					positionOnActualLine++;
				}
				editor->BlockLRShift(I1);
				break;
			}
			case _QE_: {
				if (UpdatedL) editor->KodLine();
				editor->TextLineNr = editor->ScreenFirstLineNr;
				//textIndex = ScreenIndex;
				editor->DekodLine();
				break;
			}
			case _QX_: {
				if (UpdatedL) editor->KodLine();
				editor->DekFindLine(editor->LineAbs(editor->ScreenFirstLineNr + PageS - 1));
				break;
			}
			case __CTRL_PAGEUP: {
				if (UpdatedL) editor->KodLine();
				//editor->SetPart(1);
				editor->SetScreen(1, 0, 0);
				break;
			}
			case __CTRL_PAGEDOWN: {
				if (UpdatedL) editor->KodLine();
				//editor->SetPart(AbsLenT /* - Part.LenP*/ + editor->_lenT);
				editor->SetScreen(txt.length(), 0, 0);
				break;
			}
			case __CTRL_F3: {
				ss = "";
				if (UpdatedL) editor->KodLine();
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
				screen.GotoXY(1, editor->TextLineNr - editor->ScreenFirstLineNr + 2);
				//MyInsLine();
				break;
			}

			case __INSERT: {
				editor->Insert = !editor->Insert;
				break;
			}
			case __DELETE:
			case 'G': {
				if (positionOnActualLine <= editor->GetArrLineLength()) {
					editor->DelChar();
				}
				else {
					editor->DeleteLine();
				}
				break;
			}
			case __BACK: {
				if (positionOnActualLine > 1) {
					positionOnActualLine--;
					editor->DelChar();
				}
				else {
					if (editor->TextLineNr > 1) {
						if (UpdatedL) editor->KodLine();
						editor->TextLineNr--;
						//textIndex = editor->GetLineStartIndex(editor->TextLineNr);
						//editor->CopyCurrentLineToArr(textIndex);
						editor->DekodLine();
						positionOnActualLine = MinW(255, succ(editor->GetArrLineLength()));
						editor->DeleteLine();
						if (editor->TextLineNr < editor->ScreenFirstLineNr) {
							editor->ScreenFirstLineNr--;
							editor->_change_scr = true;
						}
					}
					else {
						// on 1st line -> cannot go above
					}
				}
				break;
			}
			case __CTRL_Y: {
				// if ((NextLineStartIndex >= _lenT) && !AllRd) NextPartDek();
				NextLineStartIndex = MinW(NextLineStartIndex, txt.length());
				//TestLenText(&_textT, _lenT, NextLineStartIndex, textIndex);
				UpdatT = true;
				if (editor->BegBLn > editor->LineAbs(editor->TextLineNr)) {
					editor->BegBLn--;
				}
				else if (editor->BegBLn == editor->LineAbs(editor->TextLineNr)) {
					if (TypeB == TextBlock) {
						editor->BegBPos = 1;
					}
				}
				if (editor->EndBLn >= editor->LineAbs(editor->TextLineNr)) {
					if ((editor->EndBLn == editor->LineAbs(editor->TextLineNr)) && (TypeB == TextBlock)) {
						BPos = 1;
					}
					else {
						editor->EndBLn--;
					}
				}
				editor->DeleteLine();
				editor->DekodLine();
				positionOnActualLine = 1;
				break;
			}
			case __CTRL_T: {
				if (positionOnActualLine > editor->GetArrLineLength()) {
					editor->DeleteLine();
				}
				else {
					I = positionOnActualLine;
					if (Separ.count(editor->Arr[positionOnActualLine]) > 0) {
						editor->DelChar();
					}
					else {
						while ((I <= editor->GetArrLineLength()) && !(Separ.count(editor->Arr[positionOnActualLine]) > 0)) {
							I++;
						}
					}
					while ((I <= editor->GetArrLineLength()) && (editor->Arr[I] == ' ')) {
						I++;
					}
					// TODO: k cemu to tady je? if ((I>positionOnActualLine) and TestLastPos(I,positionOnActualLine))
				}
				break;
			}
			case _QI_: { editor->Indent = !editor->Indent; break; }
			case _QL_: {
				// CTRL Q+L - obnoveni obsahu radku (pouze editovany radek)
				if (UpdatedL) editor->DekodLine();
				break;
			}
			case _QY_: {
				// CTRL Q+Y - vymaz od pozice kurzoru do konce radku
				if (editor->TestLastPos(editor->GetArrLineLength() + 1, positionOnActualLine)) ClrEol(TextAttr);
				break;
			}
			case _QF_:
			case _QA_: {
				editor->Replace = false;
				if (MyPromptLL(405, editor->FindStr)) goto Nic;
				if (key == _QA_) {
					if (MyPromptLL(407, editor->ReplaceStr)) goto Nic;
					editor->Replace = true;
				}
				ss = editor->OptionStr;
				if (MyPromptLL(406, ss)) goto Nic;
				editor->OptionStr = ss;
				if (UpdatedL) editor->KodLine();
				if (editor->TestOptStr('l') && (!editor->BlockExist() || (TypeB == ColBlock))) goto Nic;
				if (editor->TestOptStr('l')) editor->SetBlockBound(L1, L2);
				else {
					L2 = AbsLenT /* - Part.LenP */ + txt.length();
					if (editor->TestOptStr('g') || editor->TestOptStr('e'))  L1 = 1;
					else L1 = /* Part.PosP + */ editor->SetInd(textIndex, positionOnActualLine);
				}
				editor->FindReplaceString(L1, L2);
				if (key == _QA_) editor->DekodLine();
				if (!Konec) { FirstEvent = false; editor->Background(); }
				break;
			}
			case 'L': {
				if (!editor->FindStr.empty()) {
					if (UpdatedL) editor->KodLine();
					if (editor->TestOptStr('l') && (!editor->BlockExist() || (TypeB == ColBlock))) goto Nic;
					fs = 1;
					L1 = /* Part.PosP + */ editor->SetInd(textIndex, positionOnActualLine);
					if (editor->TestOptStr('l')) editor->SetBlockBound(fs, L2);
					else L2 = AbsLenT /* - Part.LenP */ + txt.length();
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
				while ((txt[I] != ' ') && (txt[I] != __CR)) { I++; }
				while (txt[I] == ' ') { I++; }
				I2 = I - I1 + 1;
				if (editor->TestLastPos(positionOnActualLine, positionOnActualLine + I2)) FillChar(&editor->Arr[positionOnActualLine], I2, 32);
				positionOnActualLine += I2;
				break;
			}
			case __CTRL_J: {
				// tabelace zprava
				I1 = editor->SetPredI() + positionOnActualLine - 2;
				if ((I1 >= textIndex - 1) || (I1 == 0)) goto Nic;
				I = I1;
				while (txt[I] == ' ') { I++; }
				while ((txt[I] != ' ') && (txt[I] != __CR)) { I++; }
				if (I == I1) goto Nic;
				I2 = I - I1 - 1;
				I = positionOnActualLine;
				positionOnActualLine--;
				while ((positionOnActualLine > 0) && (editor->Arr[positionOnActualLine] != ' ')) { positionOnActualLine--; }
				positionOnActualLine++;
				if (editor->TestLastPos(positionOnActualLine, positionOnActualLine + I2)) FillChar(&editor->Arr[positionOnActualLine], I2, 32);
				positionOnActualLine = I + I2 + 1;
				break;
			}
			case _QB_: {
				if (UpdatedL) editor->KodLine();
				editor->PosDekFindLine(editor->BegBLn, MinW(editor->GetArrLineLength() + 1, editor->BegBPos), false);
				break;
			}
			case _QK_: {
				if (UpdatedL) editor->KodLine();
				editor->PosDekFindLine(editor->EndBLn, MinW(editor->GetArrLineLength() + 1, editor->EndBPos), false); break;
			}
			case _KB_:
			case __F7:
			case _KH_: {
				editor->BegBLn = editor->LineAbs(editor->TextLineNr);
				if (TypeB == TextBlock) editor->BegBPos = MinI(editor->GetArrLineLength() + 1, positionOnActualLine);
				else editor->BegBPos = positionOnActualLine;
				if (key == _KH_) goto OznB;
				break;
			}
			case _KK_:
			case __F8: {
			OznB:
				editor->EndBLn = editor->LineAbs(editor->TextLineNr);
				if (TypeB == TextBlock) editor->EndBPos = MinI(editor->GetArrLineLength() + 1, positionOnActualLine);
				else editor->EndBPos = positionOnActualLine;
				break;
			}
			case _KN_: {
				if (TypeB == TextBlock) TypeB = ColBlock;
				else TypeB = TextBlock;
				break;
			}
			case _KY_: {
				if (editor->BlockHandle(fs, F1, 'Y')) {
					editor->EndBLn = editor->BegBLn;
					editor->EndBPos = editor->BegBPos;
				}
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
				I1 = editor->BegBLn; I2 = editor->BegBPos;
				I3 = editor->EndBLn; I = editor->EndBPos;
				bb = TypeB;
				if (!editor->BlockExist()) {
					editor->BegBLn = 1; editor->EndBLn = 0x7FFF;
					editor->BegBPos = 1; editor->EndBPos = 0xFF;
					TypeB = TextBlock;
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
					WriteH(F1, 0, txt.c_str());
					/*truncH*/
					CloseH(&F1);
					HMsgExit(CPath);
				}
				// { PosDekFindLine(L1,I,true); }
				editor->BegBLn = I1; editor->BegBPos = I2;
				editor->EndBLn = I3; editor->EndBPos = I; TypeB = bb;
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
				editor->BegBLn = /* Part.LineP + */ editor->TextLineNr;
				editor->BegBPos = positionOnActualLine;
				L1 = /* Part.PosP + */ textIndex + positionOnActualLine - 1;
				editor->FillBlank();
				fs = FileSizeH(F1); L2 = 0;
				//NullChangePart();
				switch (TypeB) {
				case TextBlock: {
					do {
						I2 = 0x1000; if (fs - L2 < int(I2))  I2 = fs - L2;
						if ((editor->_text_type != TextType::File) && ((I2 >= MaxLenT - txt.length()) || (I2 >= MemoryAvailable()))) {
							if (I2 >= MemoryAvailable()) {
								I2 = MemoryAvailable();
							}
							I2 = MinW(I2, MaxLenT - txt.length()) - 2; fs = L2 + I2;
							WrLLF10Msg(404);
						}
						I1 = L1 + L2; // - Part.PosP;
						//TestLenText(&_textT, _lenT, I1, int(I1) + I2);
						UpdatT = true;
						//if (ChangePart) I1 -= Part.MovI;
						SeekH(F1, L2);
						ReadH(F1, I2, (void*)&txt.c_str()[I1]);
						HMsgExit("");
						L2 += I2;
					} while (L2 != fs);

					I = L1 + L2; // - Part.PosP;

					if (txt[I - 1] == 0x1A) {
						//TestLenText(&_textT, _lenT, I, I - 1);
						UpdatT = true;
						I--;
					}
					editor->TextLineNr = editor->GetLineNumber(I);
					editor->EndBLn = editor->TextLineNr; // Part.LineP + TextLineNr;
					editor->EndBPos = succ(I - textIndex);
					break;
				}
				case ColBlock: {
					editor->EndBPos = positionOnActualLine; I2 = 0x1000;
					MarkStore(P1);
					sp = new LongStr(I2 + 2); //ww =BegBPos;}
					do {
						if (fs - L2 < (int)I2) I2 = fs - L2;
						SeekH(F1, L2); ReadH(F1, I2, sp->A); HMsgExit("");
						L2 += I2; sp->LL = I2; editor->BlockCDrop('R', P1, sp);
					} while (L2 != fs);
					editor->EndBLn = /*Part.LineP +*/ editor->TextLineNr - 1;
					ReleaseStore(&P1);
					break;
				}
				}
				CloseH(&F1);
				HMsgExit("");
				SetPartLine(editor->BegBLn);
				editor->TextLineNr = editor->GetLineNumber(L1 /* - Part.PosP*/);
				UpdatedL = true;
				break;
			} // end case _KR_
			case _KP_: {
				if (!editor->BlockHandle(fs, F1, 'P')) {
					I1 = editor->BegBLn; I2 = editor->BegBPos;
					I3 = editor->EndBLn; I = editor->EndBPos;
					bb = TypeB;
					editor->BegBLn = 1; editor->EndBLn = 0x7FFF;
					editor->BegBPos = 1; editor->EndBPos = 0xFF;
					TypeB = TextBlock;
					editor->BegBLn = I1; editor->BegBPos = I2;
					editor->EndBLn = I3; editor->EndBPos = I;
					TypeB = bb;
				}
				break;
			case _KF_: {
				if (editor->BlockExist() && (TypeB == TextBlock)) {
					if (UpdatedL) editor->KodLine();
					screen.CrsHide();
					SetPartLine(editor->EndBLn);
					I2 = editor->EndBLn; // -Part.LineP;
					size_t nextLineIdx = editor->GetLineStartIndex(I2);
					L1 = editor->SetInd(nextLineIdx, editor->EndBPos); // +Part.PosP;
					L2 = editor->BegBLn; positionOnActualLine = editor->BegBPos;
					SetPartLine(L2);
					I2 = editor->BegBLn; // -Part.LineP;
					nextLineIdx = editor->GetLineStartIndex(I2);
					editor->Format(I, nextLineIdx /* + Part.PosP */, L1, editor->BegBPos, true);
					editor->DekFindLine(L2);
					if (!bScroll) screen.CrsShow();
				}
				break;
			}
			case _OJ_: { editor->Just = !editor->Just; break; }
			case _OW_: {
				editor->Wrap = !editor->Wrap;
				if (editor->Wrap) { LineS--; LastC--; }
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
					//TODO: val(ss, I1, I);
				} while (!((I1 < editor->RightMarg) && (I1 > 0)));
				editor->LeftMarg = I1;
				break;
			}
			case _OR_: {       //RightMarg
				do {
					ss = std::to_string(positionOnActualLine);
					if (MyPromptLL(409, ss)) goto Nic;
					//TODO: val(ss, I1, I); // inc(I1);
				} while (!((I1 <= 255) && (editor->LeftMarg < I1)));
				editor->RightMarg = I1;
				break;
			}
			case _OC_: {
				I1 = 1;
				while ((I1 < editor->GetArrLineLength()) && (editor->Arr[I1] == ' ')) { I1++; }
				I2 = editor->GetArrLineLength();
				while ((I2 > 1) && (editor->Arr[I2] == ' ')) { I2--; }
				j = (editor->LeftMarg + (editor->RightMarg - editor->LeftMarg) / 2) - int(I1 + (I2 - I1) / 2);
				if ((I2 < I1) || (j == 0)) goto Nic;
				if (j > 0) {
					if (editor->TestLastPos(1, j + 1)) FillChar(&editor->Arr[1], j, 32);
				}
				else {
					j = MinI(-j, I1 - 1);
					editor->TestLastPos(j + 1, 1);
				}
				positionOnActualLine = MinW(LineMaxSize, editor->GetArrLineLength() + 1);
				break;
			}
			case 'B': {
				if (UpdatedL) editor->KodLine();
				L1 = textIndex;
				editor->Format(I, L1, AbsLenT + txt.length(), MinI(editor->LeftMarg, positionOnActualLine), false);
				// editor->SetPart(L1);
				I2 = L1;
				editor->TextLineNr = editor->GetLineNumber(I2);
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
				char c = ToggleCS(editor->Arr[positionOnActualLine]);
				UpdatedL = c != editor->Arr[positionOnActualLine];
				editor->Arr[positionOnActualLine] = c;
				break;
			}
			case __CTRL_F5:
				editor->Calculate();
				break;
			case __CTRL_F6: {
				if ((editor->_text_type == TextType::File) || (editor->_text_type == TextType::Local)) {
					editor->BlockHandle(fs, F1, 'p');
				}
				break;
			}
			case __ALT_F8: {
				W1 = Menu(45, spec.KbdTyp + 1);
				if (W1 != 0) {
					spec.KbdTyp = TKbdConv(W1 - 1);
				}
				break;
			}
			case 0x1000: {
			Opet:
				if ((mode != EditorMode::Help) && (mode != EditorMode::View) && editor->Wrap) {
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
				if (editor->_text_type != TextType::File)
					if (PromptYN(108)) {
						IndexT = 1;
						Event.Pressed.UpdateKey('U');
						Konec = true;
						EdBreak = 0xFFFF;
					}
				break;
			}
			case __ESC: {
				if (UpdatedL) editor->KodLine();
				//Event.Pressed.UpdateKey(key);
				Konec = true;
				EdBreak = 0;
				break;
			}
			case __ALT_EQUAL: { // end without saving
				if (editor->_text_type != TextType::File) {
					if (UpdatedL) editor->KodLine();
					//Event.Pressed.UpdateKey(key);
					Konec = true;
					EdBreak = 0xFFFF;
				}
				break;
			}
			default: {
				if (std::find(breakKeys.begin(), breakKeys.end(), key) != breakKeys.end()) {
					// *** BREAK KEYS ***
					if (UpdatedL) editor->KodLine();
					//KbdChar: = ww;
					Konec = true;
					EdBreak = 0xFFFF;
				}
				else if (key >= 0x1000 && key <= 0x101F) {
					editor->WrCharE(Lo(key)); // ***CTRL-klavesy***
					if (key == 0x100D) {
						if (UpdatedL) editor->KodLine();
						editor->DekodLine();
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

	else if ((Event.What == evMouseDown) && ((mode == EditorMode::Help) || (mode == EditorMode::Text)))
	{
		if (mode == EditorMode::Text && UpdatedL) editor->KodLine();
		if (!((Event.Where.Y >= FirstR && Event.Where.Y <= LastR - 1)
			&& (Event.Where.X >= FirstC - 1 && Event.Where.X <= LastC - 1)))
		{
			ClrEvent();
			goto Nic;
		}
		I3 = textIndex;
		j = positionOnActualLine;
		W1 = Event.Where.Y - WindMin.Y + editor->ScreenFirstLineNr;
		if (mode == EditorMode::Help) {
			W2 = editor->WordNo2() + 1;
		}
		editor->DekFindLine(editor->LineAbs(W1));
		positionOnActualLine = Event.Where.X - WindMin.X + 1;
		if (mode != EditorMode::Text) positionOnActualLine = editor->Position(positionOnActualLine);
		positionOnActualLine += BPos;
		I = editor->SetInd(textIndex, positionOnActualLine);

		if (I < txt.length()) {
			if (mode == EditorMode::Help) {
				editor->ClrWord();
				editor->WordFind(editor->WordNo(I + 1), I1, I2, W1);
				if ((I1 <= I) && (I2 >= I)) {
					editor->SetWord(I1, I2);
					Event.Pressed.UpdateKey('M');
					Konec = true;
				}
				else if (editor->WordExist()) {
					editor->WordFind(W2, I1, I2, W1);
					editor->SetWord(I1, I2);
				}
				else {
					editor->TextLineNr = editor->GetLineNumber(I3);
				}
			}
		}
		else {
			editor->TextLineNr = editor->GetLineNumber(I3);
			positionOnActualLine = (WORD)j;
		}
		ClrEvent();
	} // else if ((Event.What == evMouseDown) && ((Mode == HelpM) || (Mode == TextM)))
	else {
		ClrEvent();
	}

Nic:
	//editor->_lines = GetAllLinesWithEnds(txt, editor->HardL);
	//ClrEvent;
	IsWrScreen = false;
}
