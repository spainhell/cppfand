#pragma once
#include "access.h"
#include "constants.h"
#include "edglobal.h"
#include "pstring.h"

//TODO: fake Timer
int Timer = 0;

void MyInsLine();
void MyDelLine();
void PredLine();
void RollNext();
void RollPred();
void Frame();
void direction1(BYTE x, BYTE& zn2);
void CleanFrameM();
void FrameStep(BYTE& odir, WORD EvKeyC);
void direction2(BYTE x, BYTE& zn2); // pozor! 2 alternativy
bool TestLastPos(WORD F, WORD T);
void MoveB(WORD& B, WORD& F, WORD& T);
void DelChar();
void FillBlank();
void DeleteL();
void NewLine(char Mode);
WORD SetPredI();
void WrChar_E(char Ch);
void Format(WORD& i, longint First, longint Last, WORD Posit, bool Rep);
void Calculate_E();
bool BlockExist();
void SetBlockBound(longint& BBPos, longint& EBPos);
bool BlockHandle(longint& fs, FILE* F1, char Oper);
void ResetPrint(char Oper, longint& fs, FILE* F1, longint LenPrint, ColorOrd* co, WORD& I1, bool isPrintFile, CharArr* p);
void LowCase(unsigned char& c);
void LowCase(char& c);
void DelStorClpBd(void* P1, LongStr* sp);
void MarkRdClpBd(void* P1, LongStr* sp);
bool BlockGrasp(char Oper, void* P1, LongStr* sp);
void MovePart(WORD Ind);
void BlockDrop(char Oper, void* P1, LongStr* sp);
bool BlockCGrasp(char Oper, void* P1, LongStr* sp);
void BlockCDrop(char Oper, void* P1, LongStr* sp);
void InsertLine(WORD& i, WORD& I1, WORD& I3, WORD& ww, LongStr* sp);
void BlockCopyMove(char Oper, void* P1, LongStr* sp);
bool ColBlockExist();
void BlockLRShift(WORD I1);
void NewBlock1(WORD& I1, longint& L2);
void BlockUDShift(longint L1);
void NewBlock2(longint& L1, longint& L2);
bool MyPromptLL(WORD n, pstring* s);
void FindReplaceString(longint First, longint Last);
void ChangeP(WORD& fst);
void ReplaceString(WORD& J, WORD& fst, WORD& lst, longint& Last);
char MyVerifyLL(WORD n, pstring s);
void HelpLU(char dir);
void HelpRD(char dir);
