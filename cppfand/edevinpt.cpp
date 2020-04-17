#include "edevinpt.h"


// { function MyTestEvent - in EDSCREEN }

void CtrlShiftAlt() {}

void ScrollPress() {

	/*BYTE* BP;
	bool old, fyz;
	longint L1;
	void* ptr;

	old = Scroll;
	fyz = *(BP(ptr(0, 0x417)) && 0x10) != 0;
	if (fyz == old) FirstScroll = false;
	Scroll = (fyz || FirstScroll) && (Mode != HelpM);
	HelpScroll = Scroll || (Mode == HelpM);
	L1 = LineAbs(ScrL);
	if (old != Scroll)
	{
		if (Scroll)
		{
			WrStatusLine();
			TestKod();
			CrsHide();
			PredScLn = LineAbs(LineL);
			PredScPos = Posi;
			if (UpdPHead)
			{
				SetPart(1);
				SimplePrintHead;
				DekFindLine(MaxL(L1, PHNum + 1));
			}
			else
			{
				DekFindLine(MaxL(L1, PHNum + 1));
			}
			ScrL = LineL;
			RScrL = NewRL(ScrL);
			if (L1 != LineAbs(ScrL)) ChangeScr = true; // { DekodLine; }
			BCol = Column(BPos);
			Colu = Column(Posi);
			ColScr = Part.ColorP;
			SetColorOrd(ColScr, 1, ScrI);
		}
		else
		{
			if ((PredScLn < L1) || (PredScLn >= L1 + PageS)) PredScLn = L1;
			if (!(PredScPos in[BPos + 1 ..BPos + LineS])) PredScPos = BPos + 1;
			PosDekFindLine(PredScLn, PredScPos, false);
			if (Mode in[ViewM, SinFM, DouFM, DelFM, NotFM]) CrsBig;
			else CrsNorm();
		}
		Background();
	}

	{
		CtrlShiftAlt();
		// *** Prekodovani klaves ***
		GetEvent();
		if (Event.What == evKeyDown)
			switch (Event.KeyCode) {
			case _S_: Event.KeyCode = _left_; break;
			case _D_: Event.KeyCode = _right_; break;
			case _E_: Event.KeyCode = _up_; break;
			case _X_: Event.KeyCode = _down_; break;
			case _R_: Event.KeyCode = _pgup_; break;
			case _C_: Event.KeyCode = _pgdn_; break;
			case _A_: KeyCode = _ctrlleft_; break;
			case _F_: KeyCode = _ctrlright_; break;
			case _V_: KeyCode = _ins_; break;
			case _P_:
			{
				Wr('^P');
				ww = Event.KeyCode;
				if (My2GetEvent())
				{
					Wr("");
					if (Event.KeyCode <= 0x31) KeyCode = (ww shl 8) || Event.KeyCode;
				}
				break;
			}
			case _Q_:
			{
				Wr("^Q");
				ww = Event.KeyCode;
				if (My2GetEvent)
				{
					Wr("");
					switch (Event.KeyCode) {
					case _S_: Event.KeyCode = _home_; break;
					case _D_: Event.KeyCode = _end_; break;
					case _R_: Event.KeyCode = _ctrlpgup_; break;
					case _C_: Event.KeyCode = _ctrlpgdn_; break;
					case _E_: case _X_: case _Y_: case _L_: case _B_: case _K_: case _I_: case _F_: case _A_:
					case 0x2D: // -
					case 0x2F: // /
					case 0x3D: // =
						Event.KeyCode = (ww shl 8) || Event.KeyCode;
						break;
					default: Event.KeyCode = 0;
					}
				}
			}
			case _K_:
			{
				Wr("^K");
				ww = Event.KeyCode;
				if (My2GetEvent)
				{
					Wr("");
					if (Event.KeyCode in[_B_, _K_, _H_, _S_, _Y_, _C_, _V_, _W_, _R_, _P_, _F_, _U_, _L_, _N_])
					{
						Event.KeyCode = ww shl 8 || Event.KeyCode;
					}
					else { Event.KeyCode = 0; }
				}
			}
			case _O_:
			{
				Wr("^O");
				ww = Event.KeyCode;
				if (My2GetEvent)
				{
					Wr("");
					switch (Event.KeyCode) {
					case _W_: case  _R_: case  _L_: case  _J_: case  _C_:
					{
						Event.KeyCode = ww shl 8 || KeyCode;
						break;
					}
					default: KeyCode = 0;
					}
				}
			}
			}
		// *** Rezim - test ***
		switch (Mode)
		{
		case HelpM:
		{
			MyGetEvent = HelpEvent;
			break;
		}
		case ViewM: {
			if (Scroll) MyGetEvent = ScrollEvent;
			else MyGetEvent = ViewEvent;
			break;
		}
		case TextM: {
			if (Scroll) MyGetEvent = ScrollEvent;
			else MyGetEvent = true;
			break;
		}
		default: ;
		}
	}*/
}


bool MyGetEvent() {
	bool Ctrl;
	WORD Delta, flgs;

	Ctrl = false;
	Delta = 0;
	flgs = 0;

label1: WaitEvent(Delta);
	/*if (Mode != HelpM) ScrollPress();
	if (LLKeyFlags != 0) // { mouse }
	{
		flgs = LLKeyFlags;
		DisplLL(LLKeyFlags);
		Ctrl = true;
	}
	else
		if (KbdFlgs && 0x0F != 0) // { Ctrl Shift Alt pressed }
		{
			if (!Ctrl)
				if (Delta > 0)
				{
					flgs = KbdFlgs;
					DisplLL(KbdFlgs);
					Ctrl = true;
				}
				else Delta = spec.CtrlDelay;
		}
		else if (Ctrl)
		{
			flgs = 0;
			WrLLMargMsg(LastS, LastNr);
			Ctrl = false;
			Delta = 0;
		}

	if (!(Event.What == evKeyDown || Event.What == evMouseDown)) // pùvodnì 'if not (Event.What in [evKeyDown, evMouseDown]) then'
	{
		ClrEvent();
		if (!IsWrScreen) Background();
		goto label1;
	}
	if (flgs != 0)
	{
		LLKeyFlags = 0;
		WrLLMargMsg(LastS, LastNr);
		AddCtrlAltShift(flgs);
	}*/
}

void Wr(string s)
{
	/*if (Mode != HelpM)
	{
		if (s.empty()) s = OrigS;
		else {
			ScrRdBuf(0, 0, OrigS[1], 2);
			move(OrigS[3], OrigS[2], 1);
			OrigS[0] = chr(2);
		}
		ScrWrStr(0, 0, s, SysLColor);
	}*/
}

bool My2GetEvent()
{
	/*ClrEvent();
	GetEvent();
	if (Event.What != evKeyDown)
	{
		ClrEvent();
		return false;
	}
	if (upcase(chr(KeyCode)) in['A'..'Z']) // with Event do if upcase(chr(KeyCode)) in ['A'..'Z'] then
	{
		KeyCode = ord(upcase(chr(KeyCode))) - ord('@');
		if ((KeyCode == _Y_ || KeyCode == _Z_) && (spec.KbdTyp == CsKbd || spec.KbdTyp == SlKbd))
		{
			switch (KeyCode)
			{
			case _Z_: KeyCode = _Y_; break;
			case _Y_: KeyCode = _Z_; break;
			default:;
			}
		}
	}
	return true;*/
}


bool ScrollEvent() {
	/*EdExitDPtr X;
	bool result = false;

	if (Event.What != evKeyDown) return result;
	// with Event do case KeyCode of
	switch (Event.KeyCode) {
	case _ESC_:
	case _left_: case _right_:
	case _up_: case _down_: case _pgup_: case _pgdn_:
	case _ctrlpgup_: case _ctrlpgdn_: case _CtrlF5_: case _altF8_:
		ScrollEvent = true;
		break;
	default: {
		if ((lo(KeyCode) == 0x00) && (pos(chr(hi(KeyCode)), Breaks) != 0)) ScrollEvent = true;
		else
		{
			X = ExitD;
			while (X != nullptr) {
				if (TestExitKey(KeyCode, X))
				{
					ScrollEvent = true;
					return result;
				}
				else X = *X.Chain;
			}
		}
	}
	}*/
}

bool ViewEvent()
{
	/*bool result = ScrollEvent;
	if (Event.What != evKeyDown) return result;
	switch (Event.KeyCode)
	{
	case _QF_:
	case _L_: case _F7_: case _F8_: case _KP_: case _QB_:
	case _QK_: case _CtrlF5_: case _AltF8_: case _CtrlF3_:
	case _home_: case _end_:
	case _ctrlleft_: case _ctrlright_:
	case _QX_: case _QE_: case _Z_: case _W_:
	case _CtrlF6_:
	case _KW_: case _KB_:
	case _KK_: { result = true; break; }
	default:;
	}
	return result;*/
}


bool HelpEvent()
{
	/*bool result = false;
	if (Event.What = evKeyDown)
		switch (Event.KeyCode) {
		case _ESC_:
		case _left_: case _right_: case _up_: case _down_:
		case _pgdn_: case _pgup_: case _M_:
		{ result = true; break; }
		default:;
		}

	else if ((lo(KeyCode) == 0x00) && (pos(char(hi(KeyCode)), Breaks) != 0))
		result = true;
	if (Event.What == evMouseDown) result = true;
	return result;*/
}

void DisplLL(WORD Flags)
{
	/*if (Flags && 0x04 != 0) // { Ctrl }
		WrLLMargMsg(CtrlLastS, CtrlLastNr);
	else if (Flags && 0x03 != 0) // { Shift }
		WrLLMargMsg(ShiftLastS, 0);
	else if (Flags && 0x08 != 0) // { Alt }
		WrLLMargMsg(AltLastS, 0)*/
}

