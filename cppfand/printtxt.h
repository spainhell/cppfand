#pragma once
#include "constants.h"
#include "editor.h"
#include "pstring.h"

CharArr* pBlk; WORD iBlk, nBlk, Po;  longint charrd;
bool printBlk, outpsw;
WORD prFileNr;
pstring Ln;

pstring replaceNo(pstring s, pstring sNew);
void ExecMgrPgm();
FILE* OpenMgrOutput();
void CopyToMgr();
void PrintTxtFBlk(longint BegPos, bool CtrlL);
void PrintChar_T(char c); // definován v obase -> pøidáno *_T
void PrintStr(pstring s);
void NewLine();
void PrintHeFo(pstring T);
void GetNum(WORD& NN);
bool EofInp();
void RdLnInp();
void ResetInp();
void PrintTxtFile(longint BegPos);
void PrintArray(void* P, WORD N, bool CtrlL);
void PrintFandWork();
