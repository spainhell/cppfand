#pragma once
#include "constants.h"
#include "pstring.h"
#include "rdrun.h"

extern bool TxtEdCtrlUBrk, TxtEdCtrlF4Brk;
extern EFldD* CFld;

WORD EditTxt(pstring* s, WORD pos, WORD maxlen, WORD maxcol, char typ, bool del,
    bool star, bool upd, bool ret, WORD Delta); // r86
longint CRec();
bool TestIsNewRec();
void SetSelectFalse();
void PopEdit();
void WrEStatus();
void RdEStatus();
void SetNewCRec(longint N, bool withRead);
void GotoRecFld(longint NewRec, EFldD* NewFld);
bool EditFreeTxt(FieldDPtr F, pstring ErrMsg, bool Ed, WORD& Brk);
void DisplEditWw();
bool OpenEditWw();
void RunEdit(XString* PX, WORD& Brk);

bool PromptB(std::string& S, FrmlPtr Impl, FieldDPtr F);
pstring PromptS(std::string& S, FrmlPtr Impl, FieldDPtr F);
double PromptR(std::string& S, FrmlElem* Impl, FieldDPtr F);
/*called from Proc && Projmgr */
void EditDataFile(FileDPtr FD, EditOpt* EO);
bool SelFldsForEO(EditOpt* EO, LinkD* LD);
void UpdateEdTFld(LongStr* S);
bool StartExit(EdExitD* X, bool Displ);
