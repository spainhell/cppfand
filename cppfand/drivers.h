#pragma once
#include "constants.h"
#include "pstring.h"


typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef int longint;
typedef short integer;


// ******** Struktury *********
struct TPoint
{
	WORD X;
	WORD Y;
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
	WORD What;
	WORD KeyCode;
	WORD Buttons;
	TPoint Where;
	TPoint WhereG;
	TPoint From;
};

TEvent Event; // r39
WORD KbdChar;
BYTE KbdFlgs; // TODO: absolute $417

// ******** Konstanty *********
const bool DemoAutoRd = false; // ø. 82
pstring KbdBuffer = ""; // ø. 83
const BYTE LLKeyFlags = 0; // ø. 84

enum VideoCard { viCga, viHercules, viEga, viVga };
integer GraphDriver, GraphMode;
WORD ScrSeg, ScrGrSeg;
BYTE NrVFont, BytesPerChar;
bool ChkSnow;
bool IsGraphMode;
BYTE GrBytesPerChar;
WORD GrBytesPerLine;

const BYTE MaxTxtCols = 132; // r132 {the best adapter}
const BYTE EventQSize = 16;
const bool BGIReload = true;
TPoint LastWhere, LastWhereG, DownWhere;
struct Wind { BYTE X, Y; } WindMin, WindMax; // r137
BYTE TextAttr, StartAttr, StartMode; // r138
WORD LastMode;
void* FontArr; void* BGIDriver; void* BGILittFont; void* BGITripFont;
BYTE ButtonCount, MouseButtons, LastButtons, DownButtons, LastDouble;
WORD EventCount, EventQHead, EventQTail;
struct stEventQueue { WORD Time, Buttons, X, Y, GX, GY; };
stEventQueue EventQueue[EventQSize - 1];

struct TCrs
{
	WORD X = 0; WORD Y = 0; bool Big = false; bool On = false; bool Enabled = false; WORD Ticks = 0;
};
const TCrs Crs;
const bool MausExist = false;
const WORD ofsTicks = 0x6C; // ø. 199
const char FrameChars[] = { 'Ú','Ä','¿','À','Ä','Ù','³',' ','³', 'É','Í','»','È','Í','¼','º',' ','º','Ã','Ä','´' };
const TPoint MouseWhere = { 0, 0 };
const TPoint MouseWhereG = { 0, 0 };
const bool MausVisible = true;
const bool MausRefresh = false;

enum TVideoFont { foAscii = 0, foLatin2 = 1, foKamen = 2 };

int trialInterval;
	// pøíznaky klávesnice - pùvodnì 0:$417 (is used to make control to keys(Num, Caps, Scroll, Alt, ShR, ShL, CtrlL, CtrlR)
void* OldIntr08;

/*EventQueue:array[0..EventQSize-1] of record
	Time,Buttons,
	X,Y,
	GX,GY :word {pixel}
	end;
 EventQLast:record end;*/

void Assign(WORD XX, WORD YY);
char CurrToKamen(char C);
void ConvKamenToCurr(WORD Buf, WORD L);
void ConvKamenLatin(WORD Buf, WORD L, bool ToLatin);
char ToggleCS(char C);
char NoDiakr(char C);
void ConvToNoDiakr(WORD Buf, WORD L, TVideoFont FromFont);
void ClearKeyBuf(); // { Bios }
void ClearKbdBuf();
bool KeyPressed(); // { Bios }
WORD ReadKey(); // { Bios }
bool KbdPressed(); // { buffer + Bios }
bool ESCPressed(); // { other Bios input lost }
WORD ReadKbd(); // { buffer + Bios / + mouse / }
bool KbdTimer(WORD Delta, BYTE Kind);
void AddToKbdBuf(WORD KeyCode);
void BreakIntrInit();
void BreakIntrDone();
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
void InitMouseEvents();
void DoneMouseEvents();
void ShowMouse();
void HideMouse();
void ShowMaus();
void HideMaus();
void GetMouseEvent();
void SetMouse(WORD X, WORD Y, bool Visible);
bool TestEvent();
WORD WaitEvent(WORD Delta);
void GetEvent();
void ClrEvent();
WORD AddCtrlAltShift(BYTE Flgs);
void AssignCrt(pstring filepath);
WORD AutoTicks, DownTicks, AutoDelay;
void* OldBreakIntr;
void* OldKbdIntr;

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

WORD WrOutput(pstring F);
WORD DummyCrt(pstring F);
WORD OpenCrt(pstring F);
