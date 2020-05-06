#pragma once
#include "constants.h"
#include "pstring.h"
#include "rdrun.h"

bool TxtEdCtrlUBrk, TxtEdCtrlF4Brk;
EFldD* CFld;

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
bool ELockRec(EditD* E, longint N, bool IsNewRec, bool Subset);
bool LockRec(bool Displ);
void UnLockRec(EditD* E);
void NewRecExit();
void SetWasUpdated();
void SetCPage();
void AdjustCRec();
void WrEStatus();
void RdEStatus();
void AssignFld(FieldDPtr F, FrmlPtr Z);
void DuplFld(FileD* FD1, FileD* FD2, void* RP1, void* RP2, void* RPt, FieldDPtr F1, FieldDPtr F2);
bool TestMask(pstring* S, pstring* Mask, bool TypeN);
WORD FieldEdit(FieldDPtr F, FrmlPtr Impl, WORD LWw, WORD iPos,
    pstring* Txt, double& RR, bool del, bool upd, bool ret, WORD Delta);
bool IsFirstEmptyFld();
WORD FldRow(EFldD* D, WORD I);
void SetFldAttr(EFldD* D, WORD I, WORD Attr);
WORD RecAttr(WORD I);
void IVoff();
void IVon();
bool HasTTWw(FieldDPtr F);
void DisplFld(EFldD* D, WORD I);
void Wr1Line(FieldDPtr F);
void DisplEmptyFld(EFldD* D, WORD I);
void SetRecAttr(WORD I);
void DisplRec(WORD I);
void DisplTabDupl();
void DisplRecNr(longint N);
void DisplSysLine();
void DisplBool();
void DisplAllWwRecs();
void SetNewWwRecAttr();
void MoveDispl(WORD From, WORD Where, WORD Number);
void SetNewCRec(longint N, bool withRead);
void WriteSL(StringList SL);
void DisplRecTxt();
void DisplEditWw();
void DisplWwRecsOrPage();
void DuplOwnerKey();
bool CheckOwner(EditD* E);
bool CheckKeyIn(EditD* E);
bool TestDuplKey(KeyDPtr K);
void DuplKeyMsg(KeyDPtr K);
#ifdef FandSQL
KeyDPtr OnlyKeyArgFlds(KeyDPtr K);
#endif
void BuildWork();
void SetStartRec();
bool OpenEditWw();
void RefreshSubset();
void GotoRecFld(longint NewRec, EFldD* NewFld);
void UpdMemberRef(void* POld, void* PNew);
void WrJournal(char Upd, void* RP, double Time);
bool LockForMemb(FileDPtr FD, WORD Kind, LockMode NewMd, LockMode& md);
bool LockWithDep(LockMode CfMd, LockMode MembMd, LockMode& OldMd);
void UnLockWithDep(LockMode OldMd);
bool DeleteRecProc();
bool CleanUp();
bool DelIndRec(longint I, longint N);

bool WriteCRec(bool MayDispl, bool& Displ);
bool ExitCheck();
longint UpdateIndexes();
#ifdef FandSQL
bool UpdSQLFile();
#endif
bool OldRecDiffers();
void UndoRecord();
void DuplFromPrevRec();

// *** RUNEDIT2 ***
bool TestAccRight(StringList S);
bool ForNavigate(FileDPtr FD);
void InsertRecProc(void* RP);
void AppendRecord(void* RP);
bool GotoXRec(XString* PX, longint& N);
bool PromptAndSearch(bool Create);
void CreateOrErr(bool Create, void* RP, longint N);
EFldD* FindEFld(FieldDPtr F);
bool PromptSearch(bool Create);
void PromptGotoRecNr();
ChkDPtr CompChk(EFldD* D, char Typ);
void DisplChkErr(ChkDPtr C);
void FindExistTest(FrmlPtr Z, LinkD* LD);
void CheckFromHere();
void Sorting();
void AutoReport();
void AutoGraph();
bool IsDependItem();
void SetDependItem();
void SwitchToAppend();
bool CtrlMProc(WORD Mode);
bool FldInModeF3Key(FieldDPtr F);
bool IsSkipFld(EFldD* D);
bool ExNotSkipFld();
bool CheckForExit(bool& Quit);
bool GoPrevNextRec(integer Delta, bool Displ);
bool GetChpt(pstring Heslo, longint& NN);
void SetCRec(longint I);
void UpdateEdTFld(LongStr* S);
void UpdateTxtPos(WORD TxtPos);
bool EditFreeTxt(FieldDPtr F, pstring ErrMsg, bool Ed, WORD& Brk);
bool EditItemProc(bool del, bool ed, WORD& Brk);
void SetSwitchProc();
void PromptSelect();
void SwitchRecs(integer Delta);

// *** RUNEDIT3 ***
bool SelFldsForEO(EditOpt* EO, LinkD* LD);
bool FinArgs(LinkD* LD, FieldDPtr F);
pstring GetFileViewName(FileD* FD, StringList SL);
bool EquFileViewName(FileD* FD, pstring S, EditOpt* EO);
bool EquRoleName(pstring S, LinkD* LD);
void GetSel2S(pstring* s, pstring* s2, char C, WORD wh);
void UpwEdit(LinkDPtr LkD);
void SetPointTo(LinkDPtr LD, pstring* s1, pstring* s2);
void ImbeddEdit();
void DownEdit();
void ShiftF7Proc();
bool ShiftF7Duplicate();
bool DuplToPrevEdit();
void Calculate(); // existuje i v EDEVPROC
void DelNewRec();
EFldD* FrstFldOnPage(WORD Page);
void F6Proc();
longint GetEdRecNo();
void SetEdRecNoEtc(longint RNr);
bool StartExit(EdExitD* X, bool Displ);
bool StartProc(Instr* ExitProc, bool Displ);
void StartRprt(RprtOpt* RO);
WORD ExitKeyProc();
void FieldHelp();
void DisplLASwitches();
void DisplLL();
void DisplCtrlAltLL(WORD Flags);
void DisplLLHlp();
void CtrlReadKbd();
void MouseProc();
void ToggleSelectRec();
void ToggleSelectAll();
void GoStartFld(EFldD* SFld);
void RunEdit(XString* PX, WORD& Brk);
/*called from Proc && Projmgr */
void EditDataFile(FileDPtr FD, EditOpt* EO);

