#pragma once
#include "constants.h"
#include "legacy.h"
#include "pstring.h"


// ******** Struktury *********
struct TPoint
{
	WORD X = 0;
	WORD Y = 0;
	void Assign(WORD XX, WORD YY);
};

const WORD evMouseDown = 0x0001;
const WORD evMouseUp = 0x0002;
const WORD evMouseMove = 0x0004;
const WORD evMouseAuto = 0x0008;
const WORD evNothing = 0x0000;
const WORD evMouse = 0x000F;
const WORD evKeyDown = 0x0010;

const WORD mbLeftButton = 0x0001;
const WORD mbRightButton = 0x0002;
const WORD mbDoubleClick = 0x0100;

struct TEvent
{
	WORD What = 0;
	WORD KeyCode = 0;
	WORD Buttons = 0;
	TPoint Where;
	TPoint WhereG;
	TPoint From;
};

extern TEvent Event; // r39
extern WORD KbdChar;
extern BYTE KbdFlgs; // TODO: absolute $417

// ******** Konstanty *********
const bool DemoAutoRd = false; // ř. 82
extern pstring KbdBuffer; // ř. 83
extern BYTE LLKeyFlags; // ř. 84

enum class enVideoCard { viCga = 0, viHercules = 1, viEga = 2, viVga = 3 };
extern enVideoCard VideoCard;// = enVideoCard::viVga;
extern integer GraphDriver, GraphMode;
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
struct Wind { BYTE X, Y; };
extern Wind WindMin, WindMax; // r137
extern BYTE TextAttr, StartAttr, StartMode; // r138
extern WORD LastMode;
extern void* FontArr; extern void* BGIDriver; extern void* BGILittFont; extern void* BGITripFont;
extern BYTE ButtonCount, MouseButtons, LastButtons, DownButtons, LastDouble;
extern WORD EventCount, EventQHead, EventQTail;
struct stEventQueue { WORD Time, Buttons, X, Y, GX, GY; };
extern stEventQueue EventQueue[EventQSize - 1];

struct TCrs
{
	WORD X = 0; WORD Y = 0; bool Big = false; bool On = false; bool Enabled = false; WORD Ticks = 0;
};
extern TCrs Crs;
const bool MausExist = false;
const WORD ofsTicks = 0x6C; // ř. 199
const char FrameChars[] = { '┌', '─', '┐', '└', '─', '┘', '│', ' ', '│', '╔', '═', '╗', '╚', '═', '╝', '║', ' ', '║', '├', '─', '┤' };
const TPoint MouseWhere = { 0, 0 };
const TPoint MouseWhereG = { 0, 0 };
const bool MausVisible = true;
const bool MausRefresh = false;

enum class TVideoFont { foAscii = 0, foLatin2 = 1, foKamen = 2 };

extern int trialInterval;
	// příznaky klávesnice - původně 0:$417 (is used to make control to keys(Num, Caps, Scroll, Alt, ShR, ShL, CtrlL, CtrlR)
extern void* OldIntr08;

/*EventQueue:array[0..EventQSize-1] of record
	Time,Buttons,
	X,Y,
	GX,GY :word {pixel}
	end;
 EventQLast:record end;*/

void Assign(WORD XX, WORD YY);
void Assign(pstring XX, pstring YY);
char CurrToKamen(char C);
void ConvKamenToCurr(WORD* Buf, WORD L);
void ConvKamenLatin(WORD* Buf, WORD L, bool ToLatin);
char ToggleCS(char C);
char NoDiakr(char C);
void ConvToNoDiakr(WORD* Buf, WORD L, TVideoFont FromFont);
//void ClearKeyBuf(); // { Bios }
void ClearKbdBuf();
//bool KeyPressed(); // { Bios }
//WORD ReadKey(); // { Bios }
bool KbdPressed(); // { buffer + Bios }
bool ESCPressed(); // { other Bios input lost }
WORD ReadKbd(); // { buffer + Bios / + mouse / }
//bool KbdTimer(WORD Delta, BYTE Kind);
//void AddToKbdBuf(WORD KeyCode);
//void BreakIntrInit();
//void BreakIntrDone();
void Delay(WORD N);
void Sound(WORD N);
void NoSound();
void LoadVideoFont();
void ScrClr(WORD X, WORD Y, WORD SizeX, WORD SizeY, char C, BYTE Color);
void ScrWrChar(WORD X, WORD Y, char C, BYTE Color);
void ScrWrStr(WORD X, WORD Y, pstring S, BYTE Color);
void ScrWrFrameLn(WORD X, WORD Y, BYTE Typ, BYTE Width, BYTE Color);
void ScrWrBuf(WORD X, WORD Y, void* Buf, WORD L);
void ScrRdBuf(WORD X, WORD Y, void* Buf, WORD L);
void* ScrPush(WORD X, WORD Y, WORD SizeX, WORD SizeY);
void ScrPop(WORD X, WORD Y, void* P);
void ScrPopToGraph(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P, WORD DOfs);
void ScrMove(WORD X, WORD Y, WORD ToX, WORD ToY, WORD L);
void ScrColor(WORD X, WORD Y, WORD L, BYTE Color);
longint CrsGet();
void CrsSet(longint S);
void CrsShow();
void CrsHide();
void CrsBig();
void CrsNorm();
void CrsIntrInit();
void CrsIntrDone();
void GotoXY(WORD X, WORD Y);
BYTE WhereX();
BYTE WhereY();
void Window(BYTE X1, BYTE Y1, BYTE X2, BYTE Y2);
void ClrScr();
void ClrEol();
void TextBackGround(BYTE Color);
void TextColor(BYTE Color);
void InsLine();
void DelLine();
void Beep();
void LockBeep();
void ScrBeep();
//void InitMouseEvents();
//void DoneMouseEvents();
//void ShowMouse();
//void HideMouse();
//void ShowMaus();
//void HideMaus();
//void GetMouseEvent();
//void SetMouse(WORD X, WORD Y, bool Visible);
//bool TestEvent();
WORD WaitEvent(WORD Delta);
void GetEvent();
void ClrEvent();
//WORD AddCtrlAltShift(BYTE Flgs);
void AssignCrt(pstring* filepath);
extern WORD AutoTicks, DownTicks, AutoDelay;
extern void* OldBreakIntr;
extern void* OldKbdIntr;

void GetMonoColor();
void EgaWriteArr(WORD X, WORD Y, WORD L, void* From);
void EgaScroll(WORD X, WORD Y, WORD SizeX, WORD SizeY, bool Up);
void HercGetOfs();
void HercWriteArr(WORD X, WORD Y, WORD L, void* From);
void CrsDraw();
void ScrGetPtr();
void HideMausIn();
void ScrWr();
void CrsDark();
void CrsBlink();
void CrsGotoXY(WORD aX, WORD aY);
void CrsGotoDX();
void CrsIntr08();

void ScrPush1(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P);
void Scroll(WORD X, WORD Y, WORD SizeX, WORD SizeY, bool Up);
void WrDirect();
void ScrollUp();
void LineFeed();

WORD WrOutput(TextFile* F);
WORD DummyCrt(TextFile* F);
WORD OpenCrt(TextFile* F);

void BreakIntrDone();
bool KbdTimer(WORD Delta, BYTE Kind);
bool TestEvent();
WORD AddCtrlAltShift(BYTE Flgs);
void AddToKbdBuf(WORD KeyCode);
void ShowMouse();
void GetMouseEvent();
bool KeyPressed();
WORD ReadKey();
void SetMouse(WORD X, WORD Y, bool Visible);
void ClearKeyBuf();
void BreakIntrInit();
void InitMouseEvents();

