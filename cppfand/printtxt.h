#pragma once
#include "base.h"
#include "constants.h"
#include "pstring.h"

extern char* pBlk;
extern WORD iBlk, nBlk, Po;
extern int charrd;
extern bool printBlk, outpsw;
extern WORD prFileNr;
extern pstring Ln;

std::string replaceNo(std::string s, std::string sNew);
void ExecPrintManagerProgram();
FILE* OpenPrintManagerOutput();
void CopyToPrintManager(std::string& text);
void PrintTxtFBlk(std::string& text, int BegPos, bool CtrlL);
void PrintChar_T(char c); // definován v obase -> pøidáno *_T
void PrintStr(pstring s);
void NewLine();
//void PrintHeFo(pstring T);
void GetNum(WORD& NN);
bool EofInp();
void RdLnInp();
void ResetInp();
void PrintTxtFile(int BegPos);
void PrintArray(void* P, WORD N, bool CtrlL);
void PrintFandWork();
