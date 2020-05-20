#pragma once

#include "drivers.h"

#include <windows.h>
#include <stdio.h>
#include <consoleapi.h>
#include <handleapi.h>
#include <iostream>
#include <winbase.h>

#include "../pascal/random.h"
#include "base.h"
#include "editor.h"
#include "keyboard.h"
#include "legacy.h"
#include "obaseww.h"
#include "screen.h"
#include "wwmenu.h"

// *** KONZOLA ***
Screen screen(&TxtCols, &TxtRows, &WindMin, &WindMax, &Crs);
Keyboard keyboard;
//SMALL_RECT hWin;
DWORD ConsoleError;
PINPUT_RECORD KbdBuf;
DWORD cNumRead = 0;
// ***

TEvent Event; // r39
WORD KbdChar;
BYTE KbdFlgs; // TODO: absolute $417
pstring KbdBuffer; // ø. 83
BYTE LLKeyFlags = 0; // ø. 84
integer GraphDriver, GraphMode;
WORD ScrSeg, ScrGrSeg;
BYTE NrVFont, BytesPerChar;
bool ChkSnow;
bool IsGraphMode;
BYTE GrBytesPerChar;
WORD GrBytesPerLine;
TPoint LastWhere, LastWhereG, DownWhere;
WORD LastMode;
void* FontArr; void* BGIDriver; void* BGILittFont; void* BGITripFont;
BYTE ButtonCount, MouseButtons, LastButtons, DownButtons, LastDouble;
WORD EventCount, EventQHead, EventQTail;
stEventQueue EventQueue[EventQSize - 1];
TCrs Crs;
int trialInterval;
void* OldIntr08 = nullptr;
WORD AutoTicks, DownTicks, AutoDelay;
void* OldBreakIntr;
void* OldKbdIntr;
Wind WindMin, WindMax;

BYTE TextAttr, StartAttr, StartMode; // r138
enVideoCard VideoCard = enVideoCard::viVga;

// *** KEYBOARD ***
BYTE ofsHeadKeyBuf = 0x1A;
BYTE ofsTailKeyBuf = 0x1C; /*Bios*/
bool BreakFlag = false;
BYTE diHacek = 1; const BYTE diCarka = 2; const BYTE diUmlaut = 3;
char Diak = 0; /*diHacek, diCarka*/

BYTE TabKtL[256] = {  /* Kamenicky to Latin2 */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0xac,0x81,0x82,0xd4,0x84,0xd2,0x9b,0x9f,0xd8,0xb7,0x91,0xd6,0x96,0x92,0x8e,0xb5,
	0x90,0xa7,0xa6,0x93,0x94,0xe0,0x85,0xe9,0xec,0x99,0x9a,0xe6,0x95,0xed,0xfc,0x9c,
	0xa0,0xa1,0xa2,0xa3,0xe5,0xd5,0xde,0xe2,0xe7,0xfd,0xea,0xe8,0x9e,0xf5,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0x8b,0xba,0xfb,0xeb,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xba,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff };

BYTE TabLtK[256] = {  /*Latin2 to Kamenicky */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x80,0x81,0x82,0x83,0x84,0x96,0x86,0x87,0x88,0x89,0xb6,0xb5,0x8c,0x8d,0x8e,0x8f,
	0x90,0x8a,0x8d,0x93,0x94,0x9c,0x8c,0x97,0x98,0x99,0x9a,0x86,0x9f,0x9d,0x9e,0x87,
	0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0x92,0x91,0xa8,0xa9,0xa0,0xab,0x80,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0x8f,0xb6,0x89,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0x85,0xd3,0x83,0xa5,0x8b,0xd7,0x88,0xd9,0xda,0xdb,0xdc,0xdd,0xa6,0xdf,
	0x95,0xe1,0xa7,0xe3,0xe4,0xa4,0x9b,0xa8,0xab,0x97,0xaa,0x55,0x98,0x9d,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xad,0xf6,0xf7,0xf8,0xf9,0xfa,0x75,0x9e,0xa9,0xfe,0xff };

BYTE TabKtN[256] = {  /* Kamenicky to NoDiakr */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x43,0x75,0x65,0x64,0x61,0x44,0x54,0x63,0x65,0x45,0x4c,0x49,0x6c,0x6c,0x41,0x41,
	0x45,0x7a,0x5a,0x6f,0x6f,0x4f,0x75,0x55,0x79,0x99,0x9a,0x53,0x4c,0x59,0x52,0x74,
	0x61,0x69,0x6f,0x75,0x6e,0x4e,0x55,0x4f,0x73,0x72,0x72,0x52,0xac,0xad,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
	0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff };

BYTE TabLtN[256] = {  /* Latin2 to NoDiakr */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0x43,0x75,0x65,0x61,0x61,0x75,0x63,0x63,0x6c,0x65,0x4f,0x6f,0x69,0x5a,0x41,0x43,
	0x45,0x4c,0x6c,0x6f,0x6f,0x4c,0x6c,0x53,0x73,0x4f,0x55,0x54,0x74,0x4c,0x9e,0x63,
	0x61,0x69,0x6f,0x75,0x41,0x61,0x5a,0x7a,0x45,0x65,0x61,0x7a,0x43,0x73,0xae,0xaf,
	0xb0,0xb1,0xb2,0xb3,0xb4,0x41,0x41,0x45,0x53,0xb9,0xba,0xbb,0xbc,0x5a,0x7a,0xbf,
	0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0x41,0x61,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
	0x64,0x44,0x44,0x45,0x64,0x4e,0x49,0x49,0x65,0xd9,0xda,0xdb,0xdc,0x54,0x55,0xdf,
	0x4f,0xe1,0x4f,0x4e,0x6e,0x6e,0x53,0x73,0x52,0x55,0x72,0x55,0x79,0x59,0x74,0xef,
	0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0x75,0x52,0x72,0xfe,0xff };

const BYTE CsKbdSize = 67;

void BreakIntHandler()
{
}

void BreakIntrInit()
{
}

void BreakIntrDone()
{
}

void ClearKeyBuf()
{
}

void BreakCheck()
{
	if (BreakFlag) {
		BreakFlag = false; ClearKeyBuf(); Halt(-1);
	}
}

unsigned char CurrToKamen(unsigned char C)
{
	return C;
}

unsigned char ToggleCS(unsigned char C)
{
	return C;
}

unsigned char NoDiakr(unsigned char C)
{
	if (C < 0x80 || fonts.VFont == TVideoFont::foAscii) return C;
	if (fonts.VFont == TVideoFont::foLatin2) return TabLtN[C];
	return TabKtN[C];
}

void ConvToNoDiakr(unsigned char* Buf, WORD L, TVideoFont FromFont)
{
}

void AddToKbdBuf(WORD KeyCode)
{
}

bool KeyPressed()
{
	return keyboard.Exists();
}

WORD ReadKey()
{
	return ReadKbd();
}

WORD ConvHCU()
{
	return 0;
}

void GetKeyEvent()
{
	KEY_EVENT_RECORD key;
	bool exists = false;
	bool pressed = false;
	WORD code = 0;

	do {
		exists = keyboard.Get(key);
		if (exists && key.bKeyDown)
		{
			code = key.wVirtualKeyCode;
			if (code == 0) continue;
			Event.What = evKeyDown;
			Event.KeyCode = code;
			KbdChar = key.uChar.AsciiChar & 0x00FF; // èeské znaky jsou 0xFFxx
			return;
		}
	} while (exists);
}

bool KbdTimer(WORD Delta, BYTE Kind)
{
	longint EndTime;
	auto result = false;
	EndTime = GetTickCount() + Delta;
	result = false;
	label1:
	switch (Kind) {          /* 0 - wait, 1 - wait || ESC, 2 - wait || any key */
	case 1: if (KeyPressed() && (ReadKey() == _ESC_)) return result;
	case 2: if (KbdPressed()) { ReadKbd(); return result; }
	}
	if (GetTickCount() < EndTime) goto label1;
	result = true;
	return result;
}

void ShowMouse()
{
}

void HideMouse()
{
}

void ResetMouse()
{
}

void MouseEvHandler()
{
}

void InitMouseEvents()
{
}

void SetMouse(WORD X, WORD Y, bool Visible)
{
}

void DoneMouseEvents()
{
}

void HideMaus()
{
}

void ShowMaus()
{
}

void GetRedKeyName()
{
	// souvisi s mysi
}

void GetMouseEvent()
{
}

void GetMouseKeyEvent()
{
}

void TestGlobalKey()
{
	WORD i;
	bool InMenu6 = false; bool InMenu8 = false;
	if (Event.What != evKeyDown) return;
	switch (Event.KeyCode) {
	case _AltF8_:
	{
		if (!InMenu8)
		{
			ClrEvent(); InMenu8 = true; i = Menu(45, spec.KbdTyp + 1);
			if (i != 0) spec.KbdTyp = TKbdConv(i - 1);
			InMenu8 = false;
		}
		break;
	}
	case _AltF6_:
	{
		if (!InMenu6)
		{
			ClrEvent(); InMenu6 = true; PrinterMenu(46); InMenu6 = false;
		}
		break;
	}
	case _ESC_:
	{
		if (LLKeyFlags != 0) { LLKeyFlags = 0; ClrEvent(); }
		break;
	}
	}
}

WORD AddCtrlAltShift(BYTE Flgs)
{
	WORD result = 0;
	if (Event.What != evKeyDown) return Event.What;
	if ((Flgs & 0x04) == 0) goto label3;
	if (Event.KeyCode != VK_HOME) goto label1;
	result = _CtrlHome_;
	goto label6;
label1:
	if (Event.KeyCode != VK_END) goto label2;
	result = _CtrlEnd_;
	goto label6;
label2:
	if (Event.KeyCode != 'Y') goto label3;
	result = _Y_;
	goto label6;
label3:
	if (Event.KeyCode < VK_F1 || Event.KeyCode > VK_F10) return Event.KeyCode;
	if ((Flgs & 0x04) == 0) goto label4;
	result = Event.KeyCode + _CtrlF1_ - _F1_;
	goto label6;
label4:
	if ((Flgs & 0x08) == 0) goto label5;
	result = Event.KeyCode + _AltF1_ - _F1_;
	goto label6;
label5:
	if ((Flgs & 0x03) == 0) goto label6;
	result = Event.KeyCode + _ShiftF1_ - _F1_;
label6:
	Event.KeyCode = result;
	TestGlobalKey();
	return Event.KeyCode;
}

bool TestEvent()
{
label1:
	if (Event.What == 0) GetMouseKeyEvent();
	if (Event.What == 0) GetKeyEvent();
	if (Event.What == 0) return false;
	TestGlobalKey();
	if (Event.What == 0) goto label1;
	return true;
}


#ifdef Trial
longint getSec()
{
	WORD h, m, s, ss;
	getTime(h, m, s, ss);
	return h * 3600 + m * 60 + s;
}

void TestTrial()
{
	longint now;
	if ((trialStartFand = 0)) { trialStartFand = getSec(); trialInterval = 900; }
	else {
		now = getSec();
		if (now > trialStartFand + trialInterval) {
			trialStartFand = now;
			trialInterval = trialInterval / 3;
			if (trialInterval < 10) trialInterval = 10;
			WrLLF10Msg(71);
		}
	}
}
#endif

void TPoint::Assign(WORD XX, WORD YY)
{
	// asm les di,Self; mov ax,XX; mov es:[di].TPoint.X,ax;
	// mov ax, YY; mov es : [di] .TPoint.Y, ax end;
}

void Assign(WORD XX, WORD YY)
{
}

void Assign(pstring XX, pstring YY)
{
}

char CurrToKamen(char C)
{
	return C;
}

void ConvKamenToCurr(void* Buf, WORD L)
{
	BYTE* bBuf = (BYTE*)Buf;
	BYTE* tab = TabKtN;
	if (!fonts.NoDiakrSupported) {
		tab = TabKtL;
		if (fonts.VFont != TVideoFont::foLatin2) return;
	}
	for (WORD i = 0; i < L; i++) {
		// v ES a DI budou segment a offset na Buf
		short index = (BYTE)bBuf[i] - 0x80;
		if (index > 0)
		{
			BYTE kam = bBuf[i];
			BYTE lat = tab[kam];
			bBuf[i] = lat;
		}
	}
}

void ConvKamenLatin(WORD* Buf, WORD L, bool ToLatin)
{
}

char ToggleCS(char C)
{
	return C;
}

char NoDiakr(char C)
{
	return C;
}

void ConvToNoDiakr(WORD* Buf, WORD L, TVideoFont FromFont)
{
}

void ClearKbdBuf()
{
	keyboard.ClearBuf();
}

bool KbdPressed()
{
	if (keyboard.Exists()) return true;
	if (KeyPressed()) return true;
	Event.What = 0;
	GetMouseKeyEvent();
	if (Event.What == evKeyDown)
	{
		AddToKbdBuf(Event.KeyCode);
		ClrEvent();
		return true;
	}
	return false;
}

bool ESCPressed()
{
	if (KeyPressed())
	{
		if (ReadKey() == VK_ESCAPE) return true;
	}
	else
	{
		GetMouseKeyEvent();
		if (Event.What == evKeyDown)
		{
			if (Event.KeyCode == VK_ESCAPE)
			{
				ClrEvent(); return true;
			}
		}
		ClrEvent();
	}
	return false;
}

WORD ReadKbd()
{
	// KEYBD.PAS r606
	// v puvodnim kodu cekala, dokud neexistovala udalost Event.What == evKeyDown
	// dokola volala ClrEvent + GetEvent
	// pak vratila KeyCode a do KbdChar ulozila take KeyCode
	// na konci zavolala ClrEvent

	KEY_EVENT_RECORD key;
	//key.uChar.AsciiChar = '\0';
	bool exists = false;
	bool pressed = false;
	WORD code = 0;

	do {
		exists = keyboard.Get(key);
		pressed = key.bKeyDown;
		code = key.wVirtualKeyCode;
		Sleep(50);
	} while (!exists || code == 0 || !pressed);

	Event.What = evKeyDown;
	Event.KeyCode = code;
	KbdChar = key.uChar.AsciiChar & 0x00FF; // èeské znaky jsou 0xFFxx;
	return code;
}

void Delay(WORD N)
{
}

void Sound(WORD N)
{
}

void NoSound()
{
}

void ClrScr()
{
	screen.ScrClr(WindMin.X, WindMin.Y, WindMax.X - WindMin.X + 1, WindMax.Y - WindMin.Y + 1,
		' ', TextAttr);
	screen.GotoXY(WindMin.X, WindMin.Y);
}

void ClrEol()
{
	screen.ScrClr(Crs.X, Crs.Y, WindMax.X - Crs.X + 1, 1, ' ', TextAttr);
}

void TextBackGround(BYTE Color)
{
}

void TextColor(BYTE Color)
{
}

void InsLine()
{
	Scroll(WindMin.X, Crs.Y, WindMax.X - WindMin.X + 1, WindMax.Y - Crs.Y + 1, false);
}

void DelLine()
{
	Scroll(WindMin.X, Crs.Y, WindMax.X - WindMin.X + 1, WindMax.Y - Crs.Y + 1, true);
}

void Beep()
{
	printf("%c", '\a');
}

void LockBeep()
{
	if (spec.LockBeepAllowed) printf("%c", '\a');
}

void ScrBeep()
{
}

WORD WaitEvent(WORD Delta)
{
	longint t = 0, t1 = 0, pos = 0, l = 555;
	BYTE Flgs = 0;
	WORD x = 0, y = 0;
	bool vis = false, ce = false;
	const BYTE MoveDelay = 10;
	WORD result = 0;

	Flgs = KbdFlgs;
label0:
	pos = 0; t = GetTickCount();
label1:
	if (Event.What == 0) { GetMouseKeyEvent(); }
	if (Event.What == 0) { GetKeyEvent(); }
	if (Event.What != 0) { result = 0; goto label2; }
	if (Flgs != KbdFlgs) { result = 1; goto label2; }
	if ((Delta != 0) && (GetTickCount() > t + Delta)) { result = 2; goto label2; }
 	if (pos != 0)
	{
		if (GetTickCount() > t1 + MoveDelay)
		{
			screen.ScrWrStr(x, y, "       ", 7);
			x = Random(TxtCols - 8); y = Random(TxtRows - 1);
			screen.ScrWrStr(x, y, "PC FAND", 7);
			t1 = GetTickCount();
		}
	}
	else
	{
		if ((spec.ScreenDelay > 0) && (GetTickCount() > t + spec.ScreenDelay)) {
			l = TxtCols * TxtRows * 2 + 50;
			ce = Crs.Enabled;
			screen.CrsHide();
			pos = PushW1(1, 1, TxtCols, TxtRows, true, true);
			TextAttr = 0; ClrScr(); vis = MausVisible; HideMouse(); l = 555;
			t1 = GetTickCount() - MoveDelay;
		}
	}
	goto label1;
label2:
	if (pos != 0)
	{
		srand(l);
		if (vis) ShowMouse();
		PopW(pos);
		if (ce) screen.CrsShow();
		if (Event.What != 0)
		{
			Event.What = 0;
			goto label0;
		}
	}
	TestGlobalKey();
#ifdef Trial
	TestTrial();
#endif
	return result;

	//GetKeyEvent();
	//return ReadKey();
}

void GetEvent()
{
	do { WaitEvent(0); } while (Event.What == 0);
}

void ClrEvent()
{
	Event.What = 0;
}

void AssignCrt(pstring* filepath)
{
	TextFile* F = (TextFile*)filepath;
	/* !!! with F do!!! */
	F->Mode = "fmClosed";
	F->bufsize = 128; //BufPtr = buffer;
	F->openfunc = OpenCrt;
	F->name[0] = 0;
}

void GetMonoColor()
{
}

void EgaWriteArr(WORD X, WORD Y, WORD L, void* From)
{
}

void EgaScroll(WORD X, WORD Y, WORD SizeX, WORD SizeY, bool Up)
{
}

void CrsDraw()
{
}

void HideMausIn()
{
}


void Scroll(WORD X, WORD Y, WORD SizeX, WORD SizeY, bool Up)
{
	printf("Scroll neumime! Zatim ...");
}

void WrDirect()
{
}

void ScrollUp()
{
	Scroll(WindMin.X, WindMin.Y, WindMax.X - WindMin.X + 1, WindMax.Y - WindMin.Y + 1, true);
}

void LineFeed()
{
}

WORD WrOutput(TextFile* F)
{
	return 0;
}

WORD DummyCrt(TextFile* F)
{
	return 0;
}

WORD OpenCrt(TextFile* F)
{
	/* !!! with F do!!! */
	F->inoutfunc = WrOutput; F->flushfunc = WrOutput; F->closefunc = DummyCrt;
	return 0;
}
