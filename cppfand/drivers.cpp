#pragma once

#include "drivers.h"

#include <windows.h>
#include <stdio.h>
#include <consoleapi.h>
#include <handleapi.h>
#include <iostream>
#include <winbase.h>
#include "base.h"
#include "editor.h"
#include "keyboard.h"
#include "legacy.h"

// *** KONZOLA ***
HANDLE hConsOutput;
Keyboard keyboard;
SMALL_RECT hWin;
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

void ConsoleInit()
{
	// inicializace vystupu na konzolu
	hConsOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	
	// inicializace vstupu z konzoly, nastaveni bufferu klavesnice
	//keyboard = new Keyboard();
}

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
	return true;
}

WORD ReadKey()
{
	//auto key = keyboard.Get();
	//auto d = key.wVirtualKeyCode;
	//Event.What = evKeyDown;
	//Event.KeyCode = d;
	//return d;
	return 0;
}

WORD ConvHCU()
{
	return 0;
}

void GetKeyEvent()
{
}

bool KbdTimer(WORD Delta, BYTE Kind)
{
	//longint EndTime;
	auto result = false;
	//EndTime = Timer + Delta;
	//result = false;
	//label1:
	//switch (Kind) {          /* 0 - wait, 1 - wait || ESC, 2 - wait || any key */
	//case 1: if (KeyPressed() && (ReadKey() == _ESC_)) return result;
	//case 2: if (KbdPressed()) { ReadKbd(); return result; }
	//}
	//if (Timer < EndTime) goto label1;
	//result = true;
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
}

void GetMouseEvent()
{
}

void GetMouseKeyEvent()
{
}

void TestGlobalKey()
{
}

WORD AddCtrlAltShift(BYTE Flgs)
{
	return 0;
}

bool TestEvent()
{
	return false;
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
}

bool KbdPressed()
{
	return true;
}

bool ESCPressed()
{
	return false;
}

WORD ReadKbd()
{
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
	KbdChar = key.uChar.AsciiChar;
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

void ScrClr(WORD X, WORD Y, WORD SizeX, WORD SizeY, char C, BYTE Color)
{
	//SetConsoleScreenBufferSize(hConsOutput, { 80,25 });
	DWORD written = 0;
	CHAR_INFO* _buf = new CHAR_INFO[SizeX * SizeY];
	COORD BufferSize = { (short)SizeX, (short)SizeY };
	SMALL_RECT rect = { X, Y, X + SizeX, Y + SizeY };
	
	CHAR_INFO ci; ci.Char.AsciiChar = C; ci.Attributes = Color;
	for (int i = 0; i < SizeX * SizeY; i++) { _buf[i] = ci; }
	WriteConsoleOutput(hConsOutput, _buf, BufferSize, {0, 0}, &rect);
	
	//SetConsoleCursorPosition(hConsOutput, leftTop);
	//WriteConsoleOutput(hConsOutput, _buf, rightBottom, leftTop, &hWin);
	//FillConsoleOutputAttribute(hConsOutput, Color, SizeX * SizeY, coordScreen, &written);
	//FillConsoleOutputCharacter(hConsOutput, C, SizeX * SizeY, coordScreen, &written);

	delete[] _buf;
}

void ScrWrChar(WORD X, WORD Y, char C, BYTE Color)
{
	DWORD written = 0;
	WORD attr = Color;
	WriteConsoleOutputCharacterA(hConsOutput, &C, 1, { (short)X, (short)Y }, &written);
	WriteConsoleOutputAttribute(hConsOutput, &attr, 1, { (short)X, (short)Y }, &written);
}

void ScrWrStr(WORD X, WORD Y, std::string S, BYTE Color)
{
	short len = S.length();
	CHAR_INFO* _buf = new CHAR_INFO[len];
	COORD BufferSize = { len, 1 };
	SMALL_RECT rect = { (short)X, (short)Y, (short)X + len, (short)Y };

	CHAR_INFO ci;
	ci.Attributes = Color;
	for (int i = 0; i < len; i++)
	{
		ci.Char.AsciiChar = S[i];
		_buf[i] = ci;
	}
	WriteConsoleOutputA(hConsOutput, _buf, BufferSize, { 0, 0 }, &rect);
}

void ScrWrFrameLn(WORD X, WORD Y, BYTE Typ, BYTE Width, BYTE Color)
{
	pstring txt;
	txt[0] = Width;
	txt[1] = FrameChars[Typ];
	txt[Width] = FrameChars[Typ + 2];
	for (int i = 2; i <= Width - 1; i++) { txt[i] = FrameChars[Typ + 1]; }
	ScrWrStr(X, Y, txt, Color);
 }

void ScrWrText(WORD X, WORD Y, const char* S)
{
	DWORD written = 0;
	size_t len = strlen(S);
	WriteConsoleOutputCharacterA(hConsOutput, S, len, { (short)X, (short)Y }, &written);
}

void ScrWrBuf(WORD X, WORD Y, void* Buf, WORD L)
{
}

void ScrRdBuf(WORD X, WORD Y, void* Buf, WORD L)
{
}

void* ScrPush(WORD X, WORD Y, WORD SizeX, WORD SizeY)
{
	return nullptr;
}

void ScrPop(WORD X, WORD Y, void* P)
{
}

void ScrPopToGraph(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P, WORD DOfs)
{
}

void ScrMove(WORD X, WORD Y, WORD ToX, WORD ToY, WORD L)
{
}

void ScrColor(WORD X, WORD Y, WORD L, BYTE Color)
{
	DWORD written = 0;
	FillConsoleOutputAttribute(hConsOutput, Color, L, {(short)X, (short)Y}, &written);
}

TCrs CrsGet()
{
	return Crs;
}

void CrsSet(TCrs S)
{
	Crs = S;
}

void CrsShow()
{
	CONSOLE_CURSOR_INFO visible{ 1, true };
	SetConsoleCursorInfo(hConsOutput, &visible);
	Crs.Enabled = true;
}

void CrsHide()
{
	CONSOLE_CURSOR_INFO invisible { 1, false };
	SetConsoleCursorInfo(hConsOutput, &invisible);
	Crs.Enabled = false;
}

void CrsBig()
{
	if (!Crs.Big) { CrsHide(); Crs.Big = true; } CrsShow();
}

void CrsNorm()
{
	if (Crs.Big) { CrsHide(); Crs.Big = false; } CrsShow();
}

void CrsIntrInit()
{
}

void CrsIntrDone()
{
}

void GotoXY(WORD X, WORD Y)
{
	SetConsoleCursorPosition(hConsOutput, { (short)X, (short)Y });
}

BYTE WhereX()
{
	return 1;
}

BYTE WhereY()
{
	return 1;
}

void Window(BYTE X1, BYTE Y1, BYTE X2, BYTE Y2)
{
	hWin.Left = X1;
	hWin.Right = X2;
	hWin.Top = Y1;
	hWin.Bottom = Y2;
}

void ClrScr()
{
}

void ClrEol()
{
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
	GetKeyEvent();
	return ReadKey();
}

void GetEvent()
{
}

void ClrEvent()
{
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

//void HercGetOfs()
//{
//}
//
//void HercWriteArr(WORD X, WORD Y, WORD L, void* From)
//{
//}

void CrsDraw()
{
}

void ScrGetPtr(WORD X, WORD Y)
{
	//WORD mul = TxtCols * Y;
	//mul = mul << 1;
	//X = X << 1;
	//X = X + mul;
	//
	//// v AX i v ES bude ScrSeg
	//// v DI bude X
}

void HideMausIn()
{
}

void ScrWr()
{
}

void CrsDark()
{
}

void CrsBlink()
{
}

void CrsGotoXY(WORD aX, WORD aY)
{
}

void CrsGotoDX()
{
}

void CrsIntr08()
{
}

void ScrPush1(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P)
{
	
}

void Scroll(WORD X, WORD Y, WORD SizeX, WORD SizeY, bool Up)
{
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
