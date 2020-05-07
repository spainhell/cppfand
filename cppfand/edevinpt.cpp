#pragma once

#include "edevinpt.h"
#include "common.h"
#include "edevproc.h"
#include "editor.h"
#include "edscreen.h"
#include "keybd.h"


// { function MyTestEvent - in EDSCREEN }

void CtrlShiftAlt()
{
	bool Ctrl; WORD Delta, flgs;
	Ctrl = false;  Delta = 0; flgs = 0;
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
		flgs = 0; WrLLMargMsg(&LastS, LastNr);
		Ctrl = false; Delta = 0;
	}
	/*      WaitEvent(Delta);*/
	if (!(Event.What == evKeyDown || Event.What == evMouseDown))
	{
		ClrEvent(); if (!IsWrScreen) Background(); goto label1;
	}
	if (flgs != 0) {
		LLKeyFlags = 0; WrLLMargMsg(&LastS, LastNr);
		AddCtrlAltShift(flgs);
	}
}

void ScrollPress()
{
	BYTE* BP = nullptr;
	bool old = false, fyz = false;
	longint L1 = 0;
	void* ptr = nullptr;

	old = bScroll;
	//fyz = *(BP(ptr(0, 0x417)) && 0x10) != 0;
	if (fyz == old) FirstScroll = false;
	bScroll = (fyz || FirstScroll) && (Mode != HelpM);
	HelpScroll = bScroll || (Mode == HelpM);
	L1 = LineAbs(ScrL);
	if (old != bScroll)
	{
		if (bScroll)
		{
			WrStatusLine();
			TestKod();
			CrsHide();
			PredScLn = LineAbs(LineL);
			PredScPos = Posi;
			if (UpdPHead)
			{
				SetPart(1);
				SimplePrintHead();
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
			if (!(PredScPos >= BPos + 1 && PredScPos <= BPos + LineS)) PredScPos = BPos + 1;
			PosDekFindLine(PredScLn, PredScPos, false);
			if (Mode == ViewM || Mode == SinFM || Mode == DouFM
				|| Mode == DelFM || Mode == NotFM) CrsBig();
			else CrsNorm();
		}
		Background();
	}
}

void DisplLL(WORD Flags)
{
	if ((Flags & 0x04) != 0) // { Ctrl }
		WrLLMargMsg(&CtrlLastS, CtrlLastNr);
	else if ((Flags & 0x03) != 0) // { Shift }
		WrLLMargMsg(&ShiftLastS, 0);
	else if ((Flags & 0x08) != 0) // { Alt }
		WrLLMargMsg(&AltLastS, 0);
}


bool MyGetEvent() {
	pstring OrigS(4);
	OrigS = "    ";
	WORD ww;

	auto result = false;

	CtrlShiftAlt();
	// *** Prekodovani klaves ***
	GetEvent();
	if (Event.What == evKeyDown)
		switch (Event.KeyCode) {
		case _S_: Event.KeyCode = _left_; break;
		case _D_: Event.KeyCode = _right_; break;
		case _E_: Event.KeyCode = _up_; break;
		case _X_: Event.KeyCode = _down_; break;
		case _R_: Event.KeyCode = _PgUp_; break;
		case _C_: Event.KeyCode = _PgDn_; break;
		case _A_: Event.KeyCode = _CtrlLeft_; break;
		case _F_: Event.KeyCode = _CtrlRight_; break;
		case _V_: Event.KeyCode = _Ins_; break;
		case _P_:
		{
			Wr("\x10", OrigS);
			ww = Event.KeyCode;
			if (My2GetEvent())
			{
				Wr("", OrigS);
				if (Event.KeyCode <= 0x31) Event.KeyCode = (ww << 8) || Event.KeyCode;
			}
			break;
		}
		case _Q_:
		{
			Wr("\x11", OrigS);
			ww = Event.KeyCode;
			if (My2GetEvent())
			{
				Wr("", OrigS);
				switch (Event.KeyCode) {
				case _S_: Event.KeyCode = _Home_; break;
				case _D_: Event.KeyCode = _End_; break;
				case _R_: Event.KeyCode = _CtrlPgUp_; break;
				case _C_: Event.KeyCode = _CtrlPgDn_; break;
				case _E_: case _X_: case _Y_: case _L_: case _B_: case _K_: case _I_: case _F_: case _A_:
				case 0x2D: // -
				case 0x2F: // /
				case 0x3D: // =
					Event.KeyCode = (ww << 8) | Event.KeyCode;
					break;
				default: Event.KeyCode = 0;
				}
			}
		}
		case _K_:
		{
			Wr("\x0B", OrigS);
			ww = Event.KeyCode;
			if (My2GetEvent())
			{
				Wr("", OrigS);
				set<char> setKc = { _B_, _K_, _H_, _S_, _Y_, _C_, _V_, _W_, _R_, _P_, _F_, _U_, _L_, _N_ };
				if (setKc.count(Event.KeyCode) > 0)
				{
					Event.KeyCode = (ww << 8) | Event.KeyCode;
				}
				else { Event.KeyCode = 0; }
			}
		}
		case _O_:
		{
			Wr("\x0F", OrigS);
			ww = Event.KeyCode;
			if (My2GetEvent())
			{
				Wr("", OrigS);
				switch (Event.KeyCode) {
				case _W_: case  _R_: case  _L_: case  _J_: case  _C_:
				{
					Event.KeyCode = (ww << 8) | Event.KeyCode;
					break;
				}
				default: Event.KeyCode = 0;
				}
			}
		}
		}
	// *** Rezim - test ***
	switch (Mode)
	{
	case HelpM:
	{
		result = HelpEvent();
		break;
	}
	case ViewM: {
		if (bScroll) result = ScrollEvent();
		else result = ViewEvent();
		break;
	}
	case TextM: {
		if (bScroll) result = ScrollEvent();
		else result = true;
		break;
	}
	default:;
	}
	return result;
}

void Wr(pstring s, pstring OrigS)
{
	if (Mode != HelpM)
	{
		if (s.empty()) s = OrigS;
		else {
			ScrRdBuf(0, 0, &OrigS[1], 2);
			Move(&OrigS[3], &OrigS[2], 1);
			OrigS[0] = 2;
		}
		ScrWrStr(0, 0, s, SysLColor);
	}
}

bool My2GetEvent()
{
	ClrEvent();
	GetEvent();
	if (Event.What != evKeyDown)
	{
		ClrEvent();
		return false;
	}
	auto ek = &Event.KeyCode;
	if (*ek >= 'A' && *ek <= 'Z') // with Event do if upcase(chr(KeyCode)) in ['A'..'Z'] then
	{
		*ek = toupper(*ek) - '@';
		if ((*ek == _Y_ || *ek == _Z_) && (spec.KbdTyp == CsKbd || spec.KbdTyp == SlKbd))
		{
			switch (*ek)
			{
			case _Z_: *ek = _Y_; break;
			case _Y_: *ek = _Z_; break;
			default: break;
			}
		}
	}
	return true;
}

bool ScrollEvent() {
	EdExitD* X;
	bool result = false;

	if (Event.What != evKeyDown) return result;
	// with Event do case KeyCode of
	switch (Event.KeyCode) {
	case _ESC_:
	case _left_: case _right_:
	case _up_: case _down_: case _PgUp_: case _PgDn_:
	case _CtrlPgUp_: case _CtrlPgDn_: case _CtrlF5_: case _AltF8_:
		result = true;
		break;
	default: {
		if ((Lo(Event.KeyCode) == 0x00) && (Breaks.first(Hi(Event.KeyCode)) != 0)) result = true;
		else
		{
			X = ExitD;
			while (X != nullptr) {
				if (TestExitKey(Event.KeyCode, X))
				{
					result = true;
					return result;
				}
				else X = X->Chain;
			}
		}
	}
	}
	return result;
}

bool ViewEvent()
{
	bool result = ScrollEvent();
	if (Event.What != evKeyDown) return result;
	switch (Event.KeyCode)
	{
	case _QF_:
	case _L_: case _F7_: case _F8_: case _KP_: case _QB_:
	case _QK_: case _CtrlF5_: case _AltF8_: case _CtrlF3_:
	case _Home_: case _End_:
	case _CtrlLeft_: case _CtrlRight_:
	case _QX_: case _QE_: case _Z_: case _W_:
	case _CtrlF6_:
	case _KW_: case _KB_:
	case _KK_: { result = true; break; }
	default:;
	}
	return result;
}

bool HelpEvent()
{
	bool result = false;
	if (Event.What == evKeyDown)
		switch (Event.KeyCode) {
		case _ESC_:
		case _left_: case _right_: case _up_: case _down_:
		case _PgDn_: case _PgUp_: case _M_:
		{ result = true; break; }
		default:;
		}

	else if ((Lo(Event.KeyCode) == 0x00) && (Breaks.first(Hi(Event.KeyCode))) != 0)
		result = true;
	if (Event.What == evMouseDown) result = true;
	return result;
}



