#pragma once

#include "OldDrivers.h"
#include "../Common/codePages.h"
#include <Windows.h>
#include <stdio.h>
#include <consoleapi.h>
#include <handleapi.h>
#include <iostream>
#include <WinBase.h>
#include "../Common/random.h"
#include "base.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "obaseww.h"
#include "../Drivers/screen.h"
#include "../Drivers/mouse.h"
#include "../Drivers/constants.h"
#include "wwmenu.h"
#include <chrono>
#include <thread>
#include <vector>

// *** KONZOLA ***
Screen screen(TxtCols, TxtRows, &WindMin, &WindMax, &Crs);
Keyboard keyboard;
//SMALL_RECT hWin;
DWORD ConsoleError;
PINPUT_RECORD KbdBuf;
DWORD cNumRead = 0;
// ***

TEvent Event; // r39
uint8_t KbdFlgs; // TODO: absolute $417
uint8_t LLKeyFlags = 0;
short GraphDriver, GraphMode;
WORD ScrSeg, ScrGrSeg;
uint8_t NrVFont, BytesPerChar;
bool ChkSnow;
bool IsGraphMode;
uint8_t GrBytesPerChar;
WORD GrBytesPerLine;
TPoint LastWhere, LastWhereG, DownWhere;
WORD LastMode;
void* FontArr; void* BGIDriver; void* BGILittFont; void* BGITripFont;
uint8_t ButtonCount, MouseButtons, LastButtons, DownButtons, LastDouble;
bool MausExist = false;
TPoint MouseWhere, MouseWhereG;
bool MausVisible = true;
TCrs Crs;
int trialInterval;
//void* OldIntr08 = nullptr;
uint64_t AutoTicks, DownTicks, AutoDelay;
void* OldBreakIntr;
void* OldKbdIntr;
Wind WindMin, WindMax;

uint8_t TextAttr, StartAttr, StartMode; // r138
enVideoCard VideoCard = enVideoCard::viVga;

// *** KEYBOARD ***
uint8_t ofsHeadKeyBuf = 0x1A;
uint8_t ofsTailKeyBuf = 0x1C; /*Bios*/
bool BreakFlag = false;
uint8_t diHacek = 1; const uint8_t diCarka = 2; const uint8_t diUmlaut = 3;
char Diak = 0; /*diHacek, diCarka*/


const uint8_t CsKbdSize = 67;

void ClearKeyBuf()
{
}

void BreakCheck()
{
	if (BreakFlag) {
		BreakFlag = false;
		ClearKeyBuf();
		Halt(-1);
	}
}

unsigned char NoDiakr(unsigned char C)
{
	if (C < 0x80 || fonts.VFont == TVideoFont::foAscii) return C;
	if (fonts.VFont == TVideoFont::foLatin2) return TabLtN[C];
	return TabKtN[C];
}

void AddToKbdBuf(WORD KeyCode)
{
	if (KeyCode != 0) keyboard.AddToKeyBuf(KeyCode);
}

bool KeyPressed()
{
	KEY_EVENT_RECORD key;
	bool exists = keyboard.Get(key, true);
	return exists && key.bKeyDown;
}

WORD ReadKey()
{
	return ReadKbd();
}

bool GetKeyEvent()
{
	KEY_EVENT_RECORD key;
	bool exists;

	do {
		exists = keyboard.Get(key);
		if (exists && key.bKeyDown) {
			Event.Pressed = PressedKey(key);
			Event.What = evKeyDown;
			return true;
		}
		::Sleep(20); // to decrease CPU load
	} while (exists);
	return false;
}

/// Ceka na klavesu. Jde pres GetEvent, takze se uplatni i mys - kliknuti na
/// klavesu v poslednim radku nebo prave tlacitko (Esc) dodaji udalost evKeyDown.
/// KEYBD.PAS r606
WORD ReadKbd()
{
	while (Event.What != evKeyDown) {
		ClrEvent();
		GetEvent();
	}
	WORD key = Event.Pressed.KeyCombination();
	ClrEvent();
	return key;
}

uint64_t getMillisecondsNow()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()
	).count();
}

bool KbdTimer(int cpu_delta, uint8_t kind)
{
	// CPU = cca 55 ms, 1 sec = 18.2 CPU)
	const uint64_t end_time = getMillisecondsNow() + (uint64_t)(cpu_delta * 55);

	while (true) {
		switch (kind) {
		case 1: {
			// wait or break on ESC
			if (ESCPressed()) {
				return false;
				break;
			}
			break;
		}
		case 2: {
			// wait or break on any key
			if (KbdPressed()) {
				ReadKbd();
				return false;
				break;
			}
			break;
		}
		default: {
			// always wait
			break;
		}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (getMillisecondsNow() < end_time) {
			continue;
		}
		break;
	}

	return true;
}

// *** M Y S ***
// Port KEYBD.PAS r302-500. Obsluhu preruseni int 33H nahrazuji udalosti konzole,
// ktere sbira trida Mouse (Drivers/mouse.h). Ukazatel mysi kresli Windows, takze
// Show/HideMouse jen drzi priznak viditelnosti; puvodni HideMaus/ShowMaus, ktere
// ukazatel schovavaly pred zapisem do video pameti, uz nejsou potreba.

namespace
{
	// puvodne se merilo v ticich BIOSu (18.2 za sekundu)
	const uint64_t TickMs = 55;

	// FAND.CFG stare instalace muze mit nuly; pak by dvojklik ani opakovani nefungovaly
	uint64_t DoubleDelayMs() { return (spec.DoubleDelay != 0 ? spec.DoubleDelay : 8) * TickMs; }
	uint64_t RepeatDelayMs() { return (spec.RepeatDelay != 0 ? spec.RepeatDelay : 8) * TickMs; }

	/// Nazev "cervene" klavesy na miste, kam se kliklo v poslednim radku (KEYBD.PAS r411).
	/// Hleda nejblizsi usek s atributem colors.lFirst vlevo od kurzoru a vrati jeho text.
	std::string GetRedKeyName()
	{
		const uint8_t red = screen.colors.lFirst;
		const WORD row = Event.Where.Y;
		WORD x = Event.Where.X + 1;
		if (x >= TxtCols) x = TxtCols - 1;

		std::vector<CHAR_INFO> line(TxtCols);
		if (!screen.ScrRdBuf(1, row + 1, line.data(), TxtCols)) return std::string();

		int i = (int)x;
		while (i >= 0 && (uint8_t)line[i].Attributes != red) i--;
		if (i < 0) i = 0;
		else while (i > 0 && (uint8_t)line[i - 1].Attributes == red) i--;

		std::string name;
		while (i < (int)TxtCols && (uint8_t)line[i].Attributes == red && name.length() < 8) {
			name += line[i].Char.AsciiChar;
			i++;
		}
		return name;
	}

	/// "F7", "ShiftF10", ... -> kod klavesy; 0 = nerozpoznano
	WORD FunctionKey(const std::string& digits, WORD base)
	{
		if (digits.empty() || digits.length() > 2) return 0;
		int n = 0;
		for (char c : digits) {
			if (c < '0' || c > '9') return 0;
			n = n * 10 + (c - '0');
		}
		if (n < 1 || n > 10) return 0;
		return base + (WORD)(n - 1);
	}
}

void ShowMouse()
{
	if (!MausExist) return;
	MausVisible = true;
}

void HideMouse()
{
	if (!MausExist) return;
	MausVisible = false;
}

void ResetMouse()
{
	if (!MausExist) return;
	mouse.Clear();
}

void InitMouseEvents()
{
	MausExist = false;
	Event.What = evNothing;
	if (spec.NoMouseSupport || !mouse.Available()) {
		MausVisible = false;
		return;
	}

	ButtonCount = mouse.ButtonCount();
	MausExist = true;

	MouseButtons = 0; LastButtons = 0; DownButtons = 0; LastDouble = 0;
	DownTicks = 0; AutoTicks = 0; AutoDelay = 0;
	mouse.Clear();
	mouse.SetPosition(MouseWhere.X, MouseWhere.Y);

	MouseWhereG.X = MouseWhere.X * 8;
	MouseWhereG.Y = MouseWhere.Y * (IsGraphMode ? GrBytesPerChar : 8);
	LastWhere = MouseWhere;
	LastWhereG = MouseWhereG;
	DownWhere = MouseWhere;
}

void SetMouse(WORD X, WORD Y, bool Visible)
{
	if (!MausExist) return;
	// v textovem rezimu chodi souradnice 1-based, v grafickem uz v pixelech
	WORD cx, cy;
	if (IsGraphMode) { cx = X / 8; cy = GrBytesPerChar != 0 ? Y / GrBytesPerChar : Y; }
	else { cx = X > 0 ? X - 1 : 0; cy = Y > 0 ? Y - 1 : 0; }

	mouse.SetPosition(cx, cy);
	mouse.WarpTo(cx, cy);
	MouseWhere.X = cx; MouseWhere.Y = cy;
	MouseWhereG.X = cx * 8;
	MouseWhereG.Y = cy * (IsGraphMode ? GrBytesPerChar : 8);
	MausVisible = Visible;
}

void DoneMouseEvents()
{
	if (!MausExist) return;
	mouse.Clear();
	MausVisible = false;
	MausExist = false;
}

void GetMouseEvent()
{
	EventType what = evNothing;
	WORD buttons = 0, x = 0, y = 0, gx = 0, gy = 0;
	uint8_t dbl = 0;

	Event.From.X = 0; Event.From.Y = 0;

	if (MausExist) {
		keyboard.PumpInput();

		// aktualni stav ukazatele drzime stejne, jako to delala obsluha int 33H
		mouse.GetState(buttons, x, y, gx, gy);
		MouseButtons = (uint8_t)buttons;
		MouseWhere.X = x; MouseWhere.Y = y;
		MouseWhereG.X = gx; MouseWhereG.Y = gy;

		// udalost bereme z fronty, jinak plati aktualni stav a cas
		uint64_t time = getMillisecondsNow();
		MouseRawEvent raw;
		if (mouse.Pop(raw)) {
			time = raw.Time; buttons = raw.Buttons;
			x = raw.X; y = raw.Y; gx = raw.GX; gy = raw.GY;
		}

		if (spec.MouseReverse) {
			WORD lr = buttons & 0x0003;
			if (lr != 0 && lr != 0x0003) buttons ^= 0x0003;
		}

		dbl = LastDouble;
		if (LastButtons != buttons) {
			if (LastButtons == 0) {
				// stisk tlacitka; stejne tlacitko na stejnem miste v limitu = dvojklik
				dbl = 0;
				if (buttons == DownButtons && x == DownWhere.X && y == DownWhere.Y
					&& time - DownTicks < DoubleDelayMs()) {
					dbl = 1;
				}
				DownButtons = (uint8_t)buttons;
				DownWhere.X = x; DownWhere.Y = y;
				DownTicks = time;
				AutoTicks = time;
				AutoDelay = RepeatDelayMs();
				what = evMouseDown;
			}
			else if (buttons == 0) {
				what = evMouseUp;
			}
			else {
				// stisk/uvolneni dalsiho tlacitka nebo zamena leve<->prave: zmenu ignorujeme
				buttons = LastButtons;
			}
		}

		if (what == evNothing && buttons != 0) {
			const bool moved = IsGraphMode
				? (gx != LastWhereG.X || gy != LastWhereG.Y)
				: (x != LastWhere.X || y != LastWhere.Y);
			if (moved) {
				Event.From = LastWhere;
				what = evMouseMove;
			}
			else if (time - AutoTicks >= AutoDelay) {
				// drzeni tlacitka na miste: po RepeatDelay se opakuje kazdy tik
				AutoTicks = time;
				AutoDelay = TickMs;
				what = evMouseAuto;
			}
		}

		if (what == evNothing) {
			buttons = 0; x = 0; y = 0; gx = 0; gy = 0; dbl = 0;
		}
		else {
			LastButtons = (uint8_t)buttons;
			LastDouble = dbl;
			LastWhere.X = x; LastWhere.Y = y;
			LastWhereG.X = gx; LastWhereG.Y = gy;
		}
	}

	Event.What = what;
	Event.Buttons = buttons | (dbl != 0 ? mbDoubleClick : 0);
	Event.Where.X = x; Event.Where.Y = y;
	Event.WhereG.X = gx; Event.WhereG.Y = gy;
	Event.Pressed.UpdateKey(0);
}

void GetMouseKeyEvent()
{
	GetMouseEvent();

	if (Event.What != evMouseAuto) {
		if (Event.What != evMouseDown) return;
		if ((Event.Buttons & mbRightButton) != 0) {
			// prave tlacitko = Esc
			Event.Pressed.UpdateKey(__ESC);
			Event.What = evKeyDown;
			return;
		}
	}

	// kliknuti na klavesu v poslednim radku obrazovky ji "stiskne"
	if ((Event.Buttons & mbLeftButton) == 0) return;
	if (Event.Where.Y != TxtRows - 1) return;

	std::string name = GetRedKeyName();

	WORD key = 0;
	if (name == ">") key = '>';
	else if (name == "\x18") key = __UP;
	else if (name == "\x19") key = __DOWN;
	else if (name == "\x11\xD9" || name == "Enter") key = __ENTER;
	else if (name == "\xC4\x10\xB3") key = VK_TAB;
	else if (name == "\xB3\x11\xC4") key = SHIFT + VK_TAB;
	else if (name == "Esc") key = __ESC;
	else if (name == "PgUp") key = __PAGEUP;
	else if (name == "PgDn") key = __PAGEDOWN;
	else if (name == "CtrlHome") key = __CTRL_HOME;
	else if (name == "CtrlEnd") key = __CTRL_END;
	else if (name == "Home") key = __HOME;
	else if (name == "End") key = __END;
	else if (name == "CtrlY") key = __CTRL_Y;
	else if (name == "Ctrl" || name == "Alt" || name == "Shift") {
		// prepinac: dalsi stisknuta klavesa se bere s timto modifikatorem
		if (name == "Ctrl") LLKeyFlags = 0x04;
		else if (name == "Alt") LLKeyFlags = 0x08;
		else LLKeyFlags = 0x03;
		ClrEvent();
		return;
	}
	else if (name.compare(0, 6, "ShiftF") == 0) key = FunctionKey(name.substr(6), __SHIFT_F1);
	else if (name.compare(0, 5, "CtrlF") == 0) key = FunctionKey(name.substr(5), __CTRL_F1);
	else if (name.compare(0, 4, "AltF") == 0) key = FunctionKey(name.substr(4), __ALT_F1);
	else if (name[0] == 'F') key = FunctionKey(name.substr(1), __F1);

	if (key == 0) return;
	Event.Pressed.UpdateKey(key);
	Event.What = evKeyDown;
}

void TestGlobalKey()
{
	bool InMenu6 = false;
	bool InMenu8 = false;
	if (Event.What != evKeyDown) return;
	if (Event.Pressed.isChar()) return;
	switch (Event.Pressed.KeyCombination()) {
	case __ALT_F8: {
		if (!InMenu8) {
			ClrEvent(); InMenu8 = true;
			WORD i = Menu(45, spec.KbdTyp + 1);
			if (i != 0) spec.KbdTyp = TKbdConv(i - 1);
			InMenu8 = false;
		}
		break;
	}
	case __ALT_F6: {
		if (!InMenu6) {
			ClrEvent(); InMenu6 = true;
			PrinterMenu(46); InMenu6 = false;
		}
		break;
	}
	case __ESC: {
		if (LLKeyFlags != 0) {
			LLKeyFlags = 0;
			ClrEvent();
		}
		break;
	}
	default: break;
	}
}

WORD AddCtrlAltShift(uint8_t Flgs)
{
	WORD key = Event.Pressed.KeyCombination();
	WORD result = 0;
	if (Event.What != evKeyDown) return Event.What;
	if ((Flgs & 0x04) == 0) goto label3;
	if (key != __HOME) goto label1;
	result = __CTRL_HOME;
	goto label6;
label1:
	if (key != __END) goto label2;
	result = __CTRL_END;
	goto label6;
label2:
	if (key != 'Y') goto label3;
	result = 'Y';
	goto label6;
label3:
	if (key < __F1 || key > __F10) return key;
	if ((Flgs & 0x04) == 0) goto label4;
	result = key + CTRL;
	goto label6;
label4:
	if ((Flgs & 0x08) == 0) goto label5;
	result = key + ALT;
	goto label6;
label5:
	if ((Flgs & 0x03) == 0) goto label6;
	result = key + SHIFT;
label6:
	Event.Pressed.UpdateKey(result);
	TestGlobalKey();
	return result;
}

bool TestEvent()
{
	while (true) {
		if (Event.What == 0) GetMouseKeyEvent();
		if (Event.What == 0) GetKeyEvent();
		if (Event.What == 0) return false;
		TestGlobalKey();
		if (Event.What == 0) continue;
		break;
	}
	return true;
}


#ifdef Trial
int getSec()
{
	WORD h, m, s, ss;
	getTime(h, m, s, ss);
	return h * 3600 + m * 60 + s;
}

void TestTrial()
{
	int now;
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


void ClearKbdBuf()
{
	// keyboard.ClearBuf();
}

bool KbdPressed()
{
	if (keyboard.Exists()) return true;
	if (KeyPressed()) return true;
	Event.What = evNothing;
	GetMouseKeyEvent();
	if (Event.What == evKeyDown) {
		AddToKbdBuf(Event.Pressed.KeyCombination());
		ClrEvent();
		return true;
	}
	return false;
}

bool ESCPressed()
{
	if (KeyPressed()) {
		GetKeyEvent();
		if (Event.What == evKeyDown && Event.Pressed.KeyCombination() == __ESC) {
			return true;
		}
	}
	else {
		GetMouseKeyEvent();
		if (Event.What == evKeyDown) {
			if (Event.Pressed.KeyCombination() == __ESC) {
				ClrEvent();
				return true;
			}
		}
		ClrEvent();
	}
	return false;
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

void ClrScr(uint8_t Color)
{
	screen.ScrClr(WindMin.X, WindMin.Y, WindMax.X - WindMin.X + 1, WindMax.Y - WindMin.Y + 1, ' ', Color);
	screen.GotoXY(WindMin.X, WindMin.Y, absolute);
}

void ClrEol(uint8_t Color)
{
	short X = screen.WhereXabs();
	short Y = screen.WhereYabs();
	screen.ScrClr(X, Y, WindMax.X - X + 1, 1, ' ', Color);
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

WORD WaitEvent(uint64_t Delta)
{
	ULONGLONG t = 0;
	uint64_t t1 = 0;
	int pos = 0, l = 555;
	uint8_t Flgs = 0;
	WORD x = 0, y = 0;
	bool vis = false, ce = false;
	const uint64_t MoveDelay = 10;
	WORD result = 0;

	Flgs = KbdFlgs;
label0:
	pos = 0;
	t = GetTickCount64();
label1:
	if (Event.What == 0) { GetMouseKeyEvent(); }
	if (Event.What == 0) { GetKeyEvent(); }
	if (Event.What != 0) { result = 0; goto label2; }
	if (Flgs != KbdFlgs) { result = 1; goto label2; }
	if ((Delta != 0) && (GetTickCount64() > t + Delta)) { result = 2; goto label2; }
	if (pos != 0) {
		if (GetTickCount64() > t1 + MoveDelay * 1000) {
			screen.ScrWrStr(x, y, "       ", 7);
			x = Random(TxtCols - 8);
			y = Random(TxtRows - 1);
			screen.ScrWrStr(x, y, "PC FAND", 7);
			t1 = GetTickCount64();
		}
	}
	else {
		if ((spec.ScreenDelay > 0) && (GetTickCount() > t + spec.ScreenDelay)) {
			l = TxtCols * TxtRows * 2 + 50;
			ce = Crs.Enabled;
			screen.CrsHide();
			pos = PushW(1, 1, TxtCols, TxtRows, true, true);
			TextAttr = 0;
			ClrScr(TextAttr);
			vis = MausVisible;
			HideMouse();
			l = 555;
			t1 = GetTickCount() - MoveDelay;
		}
	}
	goto label1;
label2:
	if (pos != 0) {
		srand(l);
		if (vis) ShowMouse();
		PopW(pos);
		if (ce) screen.CrsShow();
		if (Event.What != 0) {
			Event.What = evNothing;
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
	do {
		WaitEvent(0);
	} while (Event.What == 0);
}

void ClrEvent()
{
	Event.What = evNothing;
}

void AssignCrt(pstring* filepath)
{
	TextFile* F = (TextFile*)filepath;
	F->Mode = "fmClosed";
	F->bufsize = 128; //BufPtr = buffer;
	F->openfunc = OpenCrt;
	F->name[0] = 0;
}

void GetMonoColor()
{
}

void CrsDraw()
{
}

short WrOutput(TextFile* F)
{
	return 0;
}

short DummyCrt(TextFile* F)
{
	return 0;
}

short OpenCrt(TextFile* F)
{
	F->inoutfunc = WrOutput;
	F->flushfunc = WrOutput;
	F->closefunc = DummyCrt;
	return 0;
}

unsigned long long getAvailPhysMemory()
{
	MEMORYSTATUSEX status = {};
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status.ullAvailPhys;
}