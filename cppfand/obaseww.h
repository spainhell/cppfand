#pragma once
#include "constants.h"
#include "OldDrivers.h"
#include "pstring.h"


struct WGrBuf
{
	WORD LL;
	longint ChainPos;
	WORD X, Y;
	BYTE A;
};

const WORD MaxGrBufSz = 0x7fff - 4;

WParam* PushWParam(WORD C1, WORD R1, WORD C2, WORD R2, bool WW);
void PopWParam(WParam* wp);
void* PushScr(WORD C1, WORD R1, WORD C2, WORD R2);
longint PushW(WORD C1, WORD R1, WORD C2, WORD R2, bool push_pixel = false, bool ww = true);
void PopScr(void* p, bool draw);
void PopW(longint pos, bool draw = true);
void WriteWFrame(BYTE WFlags, pstring top, pstring bottom); // r142
void WrHd(pstring Hd, WORD Row, WORD MaxCols);
void CenterWw(BYTE& C1, BYTE& R1, BYTE& C2, BYTE& R2, BYTE WFlags);
longint PushWFramed(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, 
	pstring top, pstring bottom, BYTE WFlags); // r176
longint PushWrLLMsg(WORD N, bool WithESC);
void WrLLMsg(WORD N);
void WrLLMsgTxt();
void WrLLF10MsgLine(); // stejna fce definovana v kbdww.cpp
void WrLLF10Msg(WORD N); // stejna fce definovana v kbdww.cpp
bool PromptYN(WORD NMsg);
extern WORD RunErrNr;
void RunError(WORD N); // podobna fce definovana v kbdww.cpp
void CFileMsg(WORD n, char Typ); // podobna fce definovana v kbdww.cpp
void CFileError(WORD N); // podobna fce definovana v kbdww.cpp

struct RunMsgD
{
	RunMsgD* Last = nullptr;
	longint MsgNN = 0, MsgStep = 0, MsgKum = 0;
	longint W = 0;
};

void RunMsgOn(char C, longint N);
void RunMsgN(longint N);
void RunMsgOff();
void RunMsgClear();
