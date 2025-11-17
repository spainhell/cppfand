#pragma once

#include "FileD.h"
#include "OldDrivers.h"


struct WGrBuf
{
	WORD LL;
	int ChainPos;
	WORD X, Y;
	uint8_t A;
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
void CenterWw(uint8_t& C1, uint8_t& R1, uint8_t& C2, uint8_t& R2, uint8_t WFlags);
int PushWFramed(uint8_t C1, uint8_t R1, uint8_t C2, uint8_t R2, WORD Attr, 
	std::string top, std::string bottom, uint8_t WFlags); // r176
int PushWrLLMsg(WORD N, bool WithESC);
void WrLLMsg(WORD N);
void WrLLMsgTxt(std::string& message);
void WrLLF10MsgLine(std::string& message); // stejna fce definovana v kbdww.cpp
void WrLLF10Msg(int msgNr); // stejna fce definovana v kbdww.cpp
bool PromptYN(WORD NMsg);
extern WORD RunErrNr;
void RunError(WORD N); // podobna fce definovana v kbdww.cpp
void FileMsg(FileD* file_d, int n, char Typ); // podobna fce definovana v kbdww.cpp
