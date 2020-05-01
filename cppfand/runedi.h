#pragma once
#include "constants.h"
#include "pstring.h"
#include "rdrun.h"

bool TxtEdCtrlUBrk, TxtEdCtrlF4Brk;

EditD* E = EditDRoot;
EFldD* FirstEmptyFld;
KeyDPtr VK;
WKeyDPtr WK;
longint BaseRec;
BYTE IRec;
bool IsNewRec, Append, Select, WasUpdated, EdRecVar;
bool AddSwitch, ChkSwitch, WarnSwitch, Subset, NoDelTFlds, WasWK;
bool NoDelete, VerifyDelete, NoCreate, F1Mode, OnlyAppend, OnlySearch;
bool Only1Record, OnlyTabs, NoESCPrompt, MustESCPrompt, Prompt158;
bool NoSrchMsg, WithBoolDispl, Mode24, NoCondCheck, F3LeadIn;
bool LUpRDown, MouseEnter, TTExit;
bool MakeWorkX, NoShiftF7Msg, MustAdd, MustCheck, SelMode;
WORD UpdCount, CPage;
ERecTxtD* RT;
bool HasIndex, HasTF, NewDisplLL;

void PopEdit();
bool TestIsNewRec();
void SetSelectFalse();

WORD EditTxt(pstring* s, WORD pos, WORD maxlen, WORD maxcol, char typ, bool del,
    bool star, bool upd, bool ret, WORD Delta); // r86
void DelBlk(BYTE* sLen, pstring* s, WORD pos);
void WriteStr(WORD& pos, WORD& base, WORD& maxLen, WORD& maxCol, BYTE* sLen, pstring* s, bool star,
	WORD cx, WORD cy, WORD cx1, WORD cy1);
void WrPromptTxt(pstring* S, FrmlPtr Impl, FieldDPtr F, pstring* Txt, double& R);
bool PromptB(pstring* S, FrmlPtr Impl, FieldDPtr F);
pstring PromptS(pstring* S, FrmlPtr Impl, FieldDPtr F);
double PromptR(pstring* S, FrmlPtr Impl, FieldDPtr F);

// *** RUNEDIT1 ***
longint CRec();
longint CNRecs();
longint AbsRecNr(longint N);
longint LogRecNo(longint N);
bool IsSelectedRec(WORD I);
bool EquOldNewRec();
void RdRec(longint N);

