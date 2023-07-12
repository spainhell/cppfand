#pragma once

#include "Cfg.h"

#include "legacy.h"
#include "../Drivers/screen.h"
#include "../Drivers/keyboard.h"


// ******** Struktury *********
struct TPoint
{
	WORD X = 0;
	WORD Y = 0;
	void Assign(WORD XX, WORD YY);
};

const WORD mbLeftButton = 0x0001;
const WORD mbRightButton = 0x0002;
const WORD mbDoubleClick = 0x0100;

enum EventType {
	evNothing = 0,
	evMouseDown = 1, evMouseUp = 2, evMouseMove = 4, evMouseAuto = 8, evMouse = 15,
	evKeyDown = 16
};

struct TEvent
{
	EventType What = evNothing;
	PressedKey Pressed;
	WORD Buttons = 0;
	TPoint Where;
	TPoint WhereG;
	TPoint From;
};

extern TEvent Event; // r39
extern BYTE KbdFlgs; // TODO: absolute $417

// ******** Konstanty *********
const bool DemoAutoRd = false; // r. 82
//extern pstring KbdBuffer; // r. 83
extern Keyboard keyboard;
extern BYTE LLKeyFlags; // r. 84

extern enVideoCard VideoCard;
extern short GraphDriver, GraphMode;
extern WORD ScrSeg, ScrGrSeg;
extern BYTE NrVFont, BytesPerChar;
extern bool ChkSnow;
extern bool IsGraphMode;
extern BYTE GrBytesPerChar;
extern WORD GrBytesPerLine;

const BYTE MaxTxtCols = 132; // r132 {the best adapter}
const BYTE EventQSize = 16;
const bool BGIReload = true;
extern TPoint LastWhere, LastWhereG, DownWhere;
extern Wind WindMin, WindMax; // r137
extern BYTE TextAttr, StartAttr, StartMode; // r138
extern WORD LastMode;
extern void* FontArr; extern void* BGIDriver; extern void* BGILittFont; extern void* BGITripFont;
extern BYTE ButtonCount, MouseButtons, LastButtons, DownButtons, LastDouble;
extern WORD EventCount, EventQHead, EventQTail;
struct stEventQueue { WORD Time, Buttons, X, Y, GX, GY; };
extern stEventQueue EventQueue[EventQSize - 1];
extern Screen screen;
extern TCrs Crs;
const bool MausExist = false;
const WORD ofsTicks = 0x6C; // ř. 199
const TPoint MouseWhere;
const TPoint MouseWhereG;
const bool MausVisible = true;
const bool MausRefresh = false;

extern int trialInterval;
	// příznaky klávesnice - původně 0:$417 (is used to make control to keys(Num, Caps, Scroll, Alt, ShR, ShL, CtrlL, CtrlR)
extern void* OldIntr08;

/*EventQueue:array[0..EventQSize-1] of record
	Time,Buttons,
	X,Y,
	GX,GY :word {pixel}
	end;
 EventQLast:record end;*/

void ClearKbdBuf();
bool KbdPressed(); // { buffer + Bios }
bool ESCPressed(); // { other Bios input lost }
WORD ReadKbd(); // { buffer + Bios / + mouse / }
void Delay(WORD N);
void Sound(WORD N);
void NoSound();
void ClrScr();
void ClrEol();
void Beep();
void LockBeep();
void ScrBeep();
WORD WaitEvent(uint64_t Delta);
void GetEvent();
void ClrEvent();
void AssignCrt(pstring* filepath);
extern WORD AutoTicks, DownTicks, AutoDelay;
extern void* OldBreakIntr;
extern void* OldKbdIntr;

void GetMonoColor();
void CrsDraw();
void HideMausIn();
void Scroll(WORD X, WORD Y, WORD SizeX, WORD SizeY, bool Up);
void WrDirect();
void ScrollUp();
void LineFeed();

short WrOutput(TextFile* F);
short DummyCrt(TextFile* F);
short OpenCrt(TextFile* F);

unsigned long long getAvailPhysMemory();

uint64_t getMillisecondsNow();
bool KbdTimer(int cpu_delta, BYTE kind);
bool TestEvent();
WORD AddCtrlAltShift(BYTE Flgs);
void AddToKbdBuf(WORD KeyCode);
void ShowMouse();
void GetMouseEvent();
bool KeyPressed();
WORD ReadKey();
void SetMouse(WORD X, WORD Y, bool Visible);
void ClearKeyBuf();
void InitMouseEvents();
