#pragma once

#include "drivers.h"
#include <iostream>
#include "base.h"
#include "editor.h"
#include "legacy.h"

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

void ConvKamenToCurr(WORD* Buf, WORD L)
{
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
	return 0;
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

void LoadVideoFont()
{
}

void ScrClr(WORD X, WORD Y, WORD SizeX, WORD SizeY, char C, BYTE Color)
{
}

void ScrWrChar(WORD X, WORD Y, char C, BYTE Color)
{
}

void ScrWrStr(WORD X, WORD Y, pstring S, BYTE Color)
{
}

void ScrWrFrameLn(WORD X, WORD Y, BYTE Typ, BYTE Width, BYTE Color)
{
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
}

longint CrsGet()
{
	return 0;
}

void CrsSet(longint S)
{
}

void CrsShow()
{
}

void CrsHide()
{
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
	return 1;
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
	F->Mode = "fmClosed"; F->bufsize = 128; //BufPtr = buffer;
	F->openfunc = OpenCrt; F->name[0] = 0;
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

void HercGetOfs()
{
}

void HercWriteArr(WORD X, WORD Y, WORD L, void* From)
{
}

void CrsDraw()
{
}

void ScrGetPtr()
{
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
