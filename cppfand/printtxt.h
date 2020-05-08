#pragma once
#include "constants.h"
#include "editor.h"
#include "pstring.h"

static CharArr* pBlk; static WORD iBlk, nBlk, Po;
static longint charrd;
static bool printBlk, outpsw;
static WORD prFileNr;
static pstring Ln;

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
