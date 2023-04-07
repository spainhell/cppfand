#pragma once
#include "constants.h"
#include "FileD.h"
#include "OldDrivers.h"
#include "pstring.h"


struct WGrBuf
{
	WORD LL;
	int ChainPos;
	WORD X, Y;
	BYTE A;
};

const WORD MaxGrBufSz = 0x7fff - 4;

WParam* PushWParam(WORD C1, WORD R1, WORD C2, WORD R2, bool WW);
void PopWParam(WParam* wp);
void* PushScr(WORD C1, WORD R1, WORD C2, WORD R2);
int PushW(WORD C1, WORD R1, WORD C2, WORD R2, bool push_pixel = false, bool ww = true);
void PopScr(void* p, bool draw);
void PopW(int pos, bool draw = true);
void WriteWFrame(BYTE WFlags, pstring top, pstring bottom); // r142
void WrHd(pstring Hd, WORD Row, WORD MaxCols);
void CenterWw(BYTE& C1, BYTE& R1, BYTE& C2, BYTE& R2, BYTE WFlags);
int PushWFramed(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, 
	pstring top, pstring bottom, BYTE WFlags); // r176
int PushWrLLMsg(WORD N, bool WithESC);
void WrLLMsg(WORD N);
void WrLLMsgTxt();
void WrLLF10MsgLine(); // stejna fce definovana v kbdww.cpp
void WrLLF10Msg(int msgNr); // stejna fce definovana v kbdww.cpp
bool PromptYN(WORD NMsg);
extern WORD RunErrNr;
void RunError(WORD N); // podobna fce definovana v kbdww.cpp
void CFileMsg(WORD n, char Typ); // podobna fce definovana v kbdww.cpp
void CFileError(FileD* file_d, WORD N); // podobna fce definovana v kbdww.cpp

struct RunMsgD
{
	RunMsgD* Last = nullptr;
	int MsgNN = 0, MsgStep = 0, MsgKum = 0;
	int W = 0;
};

void RunMsgOn(char C, int N);
void RunMsgN(int N);
void RunMsgOff();
void RunMsgClear();
