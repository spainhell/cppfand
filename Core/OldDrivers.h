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
extern uint8_t KbdFlgs; // TODO: absolute $417

// ******** Konstanty *********
const bool DemoAutoRd = false; // r. 82
//extern pstring KbdBuffer; // r. 83
extern Keyboard keyboard;
extern uint8_t LLKeyFlags; // r. 84

extern enVideoCard VideoCard;
extern short GraphDriver, GraphMode;
extern WORD ScrSeg, ScrGrSeg;
extern uint8_t NrVFont, BytesPerChar;
extern bool ChkSnow;
extern bool IsGraphMode;
extern uint8_t GrBytesPerChar;
extern WORD GrBytesPerLine;

const uint8_t MaxTxtCols = 132; // r132 {the best adapter}
const bool BGIReload = true;
extern TPoint LastWhere, LastWhereG, DownWhere;
extern Wind WindMin, WindMax; // r137
extern uint8_t TextAttr, StartAttr, StartMode; // r138
extern WORD LastMode;
extern void* FontArr; extern void* BGIDriver; extern void* BGILittFont; extern void* BGITripFont;
extern uint8_t ButtonCount, MouseButtons, LastButtons, DownButtons, LastDouble;
extern Screen screen;
extern TCrs Crs;
extern bool MausExist;      // je mys k dispozici?
extern TPoint MouseWhere;   // posledni poloha mysi ve znacich, 0-based
extern TPoint MouseWhereG;  // posledni poloha mysi v pixelech
extern bool MausVisible;    // je ukazatel mysi zobrazen?

extern int trialInterval;
// priznaky klavesnice - pùvodnì 0:$417 (is used to make control to keys(Num, Caps, Scroll, Alt, ShR, ShL, CtrlL, CtrlR)
//extern void* OldIntr08;

// Frontu udalosti mysi (puvodne EventQueue plnena obsluhou int 33H) drzi Mouse, viz Drivers/mouse.h.

void ClearKbdBuf();
bool KbdPressed(); // { buffer + Bios }
bool ESCPressed(); // { other Bios input lost }
WORD ReadKbd(); // { buffer + Bios / + mouse / }
void Delay(WORD N);
void Sound(WORD N);
void NoSound();
void ClrScr(uint8_t Color);
void ClrEol(uint8_t Color);
void Beep();
void LockBeep();
void ScrBeep();
WORD WaitEvent(uint64_t Delta);
void GetEvent();
void ClrEvent();
void AssignCrt(pstring* filepath);
extern uint64_t AutoTicks, DownTicks, AutoDelay; // v ms
extern void* OldBreakIntr;
extern void* OldKbdIntr;

void GetMonoColor();
void CrsDraw();

short WrOutput(TextFile* F);
short DummyCrt(TextFile* F);
short OpenCrt(TextFile* F);

unsigned long long getAvailPhysMemory();

uint64_t getMillisecondsNow();
bool KbdTimer(int cpu_delta, uint8_t kind);
bool TestEvent();
WORD AddCtrlAltShift(uint8_t Flgs);
void AddToKbdBuf(WORD KeyCode);
bool KeyPressed();
WORD ReadKey();
void ClearKeyBuf();

// *** MYS ***
void InitMouseEvents();
void DoneMouseEvents();
void ShowMouse();
void HideMouse();
void ResetMouse();
void SetMouse(WORD X, WORD Y, bool Visible);
void GetMouseEvent();
void GetMouseKeyEvent();
