#pragma once
#include "constants.h"


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

struct TEvent
{
	WORD What;
	WORD KeyCode;
	WORD Buttons;
	TPoint Where;
	TPoint WhereG;
	TPoint From;
};

// ******** Konstanty *********
const bool DemoAutoRd = false; // ø. 82
pstring KbdBuffer = ""; // ø. 83
const BYTE LLKeyFlags = 0; // ø. 84

struct Wind { BYTE X, Y; } WindMin, WindMax; // ø. 137
BYTE TextAttr, StartAttr, StartMode; // ø. 138

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


class Drivers
{
public:
	Drivers();
	TEvent Event;
	longint trialStartFand;
	int trialInterval;
	WORD KbdChar;
	BYTE KbdFlgs;	// pøíznaky klávesnice - pùvodnì 0:$417 (is used to make control to keys(Num, Caps, Scroll, Alt, ShR, ShL, CtrlL, CtrlR)
	
	enum VideoCard { viCga = 0, viHercules = 1, viEga = 2, viVga = 3 };
	integer GraphDriver, GraphMode;
	WORD ScrSeg, ScrGrSeg;
	BYTE NrVFont, BytesPerChar;
	bool ChkSnow;
	bool IsGraphMode;
	BYTE GrBytesPerChar;
	WORD GrBytesPerLine;
	TPoint LastWhere, LastWhereG, DownWhere;
	TPoint WindMin, WindMax; // ø. 137 - je øešeno pøes record X,Y:byte
	BYTE TextAttr, StartAttr, StartMode;
	WORD LastMode;
	void* OldIntr08;
	void* FontArr;
	void* BGIDriver;
	void* BGILittFont;
	void* BGITripFont;
	BYTE ButtonCount, MouseButtons, LastButtons, DownButtons, LastDouble;
	WORD EventCount, EventQHead, EventQTail;
	/*EventQueue:array[0..EventQSize-1] of record
		Time,Buttons,
		X,Y,
		GX,GY :word {pixel}
		end;
	 EventQLast:record end;*/

	static void Assign(WORD XX, WORD YY);
	char CurrToKamen(char C);
	static void ConvKamenToCurr(WORD Buf, WORD L);
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
	static void ScrClr(WORD X, WORD Y, WORD SizeX, WORD SizeY, char C, BYTE Color);
	static void ScrWrChar(WORD X, WORD Y, char C, BYTE Color);
	static void ScrWrStr(WORD X, WORD Y, string S, BYTE Color);
	static void ScrWrFrameLn(WORD X, WORD Y, BYTE Typ, BYTE Width, BYTE Color);
	void ScrWrBuf(WORD X, WORD Y, WORD Buf, WORD L);
	void ScrRdBuf(WORD X, WORD Y, WORD Buf, WORD L);
	void* ScrPush(WORD X, WORD Y, WORD SizeX, WORD SizeY);
	void ScrPop(WORD X, WORD Y, void* P);
	void ScrPopToGraph(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P, WORD DOfs);
	void ScrMove(WORD X, WORD Y, WORD ToX, WORD ToY, WORD L);
	void ScrColor(WORD X, WORD Y, WORD L, BYTE Color);
	longint CrsGet();
	void CrsSet(longint S);
	void CrsShow();
	static void CrsHide();
	void CrsBig();
	static void CrsNorm();
	void CrsIntrInit();
	void CrsIntrDone();
	static void GotoXY(WORD X, WORD Y);
	BYTE WhereX();
	BYTE WhereY();
	static void Window(BYTE X1, BYTE Y1, BYTE X2, BYTE Y2);
	void ClrScr();
	void ClrEol();
	void TextBackGround(BYTE Color);
	void TextColor(BYTE Color);
	void InsLine();
	void DelLine();
	void beep();
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
	void AssignCrt(string filepath);

private:
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
	void Beep();

	void ScrPush1(WORD X, WORD Y, WORD SizeX, WORD SizeY, void* P);
	void Scroll(WORD X, WORD Y, WORD SizeX, WORD SizeY, bool Up);
	// void GotoXY(BYTE X, BYTE Y);
	void WrDirect();
	void ScrollUp();
	void LineFeed();

	WORD WrOutput(string F);
	WORD DummyCrt(string F);
	WORD OpenCrt(string F);
};
