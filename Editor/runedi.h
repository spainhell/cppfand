#pragma once
#include "../Core/constants.h"
#include "../Core/EditOpt.h"
#include "../Core/rdrun.h"

enum class FieldType;
class FieldDescr;
struct EFldD;
extern bool TxtEdCtrlUBrk, TxtEdCtrlF4Brk;
extern EFldD* CFld;

WORD EditTxt(std::string& s, WORD pos, WORD maxlen, WORD maxcol, FieldType typ, bool del,
    bool star, bool upd, bool ret, WORD Delta); // r86

int CRec();
bool TestIsNewRec();
void SetSelectFalse();
void PopEdit();
void WrEStatus();
void RdEStatus();
void SetNewCRec(int N, bool withRead);
void GotoRecFld(int NewRec, EFldD* NewFld);
bool EditFreeTxt(FieldDescr* F, std::string ErrMsg, bool Ed, WORD& Brk);
void DisplEditWw();
bool OpenEditWw();
void RunEdit(XString* PX, WORD& Brk);

bool PromptB(std::string& S, FrmlElem* Impl, FieldDescr* F);
std::string PromptS(std::string& S, FrmlElem* Impl, FieldDescr* F);
double PromptR(std::string& S, FrmlElem* Impl, FieldDescr* F);
/*called from Proc && Projmgr */
void EditDataFile(FileD* FD, EditOpt* EO);
bool SelFldsForEO(EditOpt* EO, LinkD* LD);
void UpdateEdTFld(LongStr* S);
bool StartExit(EdExitD* X, bool Displ);
void ViewPrinterTxt();
