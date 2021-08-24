#pragma once
#include "../cppfand/constants.h"
#include "../cppfand/pstring.h"
#include "../cppfand/rdrun.h"
#include "../cppfand/models/FrmlElem.h"

class FieldDescr;
struct EFldD;
extern bool TxtEdCtrlUBrk, TxtEdCtrlF4Brk;
extern EFldD* CFld;

WORD EditTxt(std::string& s, WORD pos, WORD maxlen, WORD maxcol, char typ, bool del,
    bool star, bool upd, bool ret, WORD Delta); // r86

longint CRec();
bool TestIsNewRec();
void SetSelectFalse();
void PopEdit();
void WrEStatus();
void RdEStatus();
void SetNewCRec(longint N, bool withRead);
void GotoRecFld(longint NewRec, EFldD* NewFld);
bool EditFreeTxt(FieldDescr* F, std::string ErrMsg, bool Ed, WORD& Brk);
void DisplEditWw();
bool OpenEditWw();
void RunEdit(XString* PX, WORD& Brk);

bool PromptB(std::string& S, FrmlPtr Impl, FieldDescr* F);
pstring PromptS(std::string& S, FrmlPtr Impl, FieldDescr* F);
double PromptR(std::string& S, FrmlElem* Impl, FieldDescr* F);
/*called from Proc && Projmgr */
void EditDataFile(FileD* FD, EditOpt* EO);
bool SelFldsForEO(EditOpt* EO, LinkD* LD);
void UpdateEdTFld(LongStr* S);
bool StartExit(EdExitD* X, bool Displ);
void ViewPrinterTxt();
