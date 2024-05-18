#pragma once

#include "FileD.h"
#include "OldDrivers.h"
#include "../Common/pstring.h"


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
void PopScr(bool draw);
void PopW(int pos, bool draw = true);
void WriteWFrame(uint8_t WFlags, const std::string& top, const std::string& bottom, uint8_t color);
void WriteHeader(std::string header, WORD row, WORD maxCols);
void CenterWw(BYTE& C1, BYTE& R1, BYTE& C2, BYTE& R2, BYTE WFlags);
int PushWFramed(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, 
	std::string top, std::string bottom, BYTE WFlags); // r176
int PushWrLLMsg(WORD N, bool WithESC);
void WrLLMsg(WORD N);
void WrLLMsgTxt();
void WrLLF10MsgLine(); // stejna fce definovana v kbdww.cpp
void WrLLF10Msg(int msgNr); // stejna fce definovana v kbdww.cpp
bool PromptYN(WORD NMsg);
extern WORD RunErrNr;
void RunError(WORD N); // podobna fce definovana v kbdww.cpp
void FileMsg(FileD* file_d, int n, char Typ); // podobna fce definovana v kbdww.cpp
