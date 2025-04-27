#pragma once
#include "base.h"

#include "../Common/pstring.h"

extern char* pBlk;
extern WORD iBlk, nBlk, Po;
extern int charrd;
extern bool printBlk, outpsw;
extern WORD prFileNr;
extern pstring Ln;

std::string replaceNo(std::string s, std::string sNew);
void ExecPrintManagerProgram();
HANDLE OpenPrintManagerOutput();
void CopyToPrintManager(const std::string& text);
void PrintTxtFBlk(const std::string& text, int BegPos, bool CtrlL);
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
void PrintArray(const std::string& arr, bool CtrlL);
void PrintFandWork();
