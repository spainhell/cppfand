#pragma once

#include "compile.h"
#include "constants.h"
#include "pstring.h"

bool IsCurrChpt();
char ExtToTyp(pstring Ext);
void ReleaseFDLDAfterChpt();

struct RdbRecVars
{
	char Typ; pstring Name = pstring(12); pstring Ext; longint Txt, OldTxt;
	char FTyp; WORD CatIRec; bool  isSQL;
};

bool NetFileTest(RdbRecVars* X);
void GetSplitChptName(pstring* Name, pstring* Ext);
void GetRdbRecVars(void* RecPtr, RdbRecVars* X);
bool ChptDelFor(RdbRecVars* X);
bool ChptDel();
WORD ChptWriteCRec(); /* 0-O.K., 1-fail, 2-fail && undo*/
void RenameWithOldExt();
bool IsDuplFileName(pstring name);

FileD* CFileF;
longint sz; WORD nTb; void* Tb;

void WrFDSegment(longint RecNr); // ASM
void* O(void* p); // ASM
void* OCF(void* p); // ASM
void* OTb(pstring Nm);
void* OLinkD(LinkD* Ld);
void OFrml(FrmlPtr Z);
void OKF(KeyFldDPtr kf);

bool RdFDSegment(WORD FromI, longint Pos);
FileD* GetFD(void* p, bool WithSelf);
FuncD* GetFC(void* p);
LinkD* GetLinkD(void* P);
void SgFrml(FrmlPtr Z);
void SgKF(KeyFldDPtr kf);
WORD FindHelpRecNr(FileDPtr FD, pstring txt);
bool PromptHelpName(WORD& N);
void EditHelpOrCat(WORD cc, WORD kind, pstring txt);
void StoreChptTxt(FieldDPtr F, LongStr* S, bool Del);
