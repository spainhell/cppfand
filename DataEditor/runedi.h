#pragma once

#include "../Core/EditOpt.h"
#include "../Core/rdrun.h"

enum class FieldType;
class FieldDescr;
struct EFldD;
extern bool TxtEdCtrlUBrk, TxtEdCtrlF4Brk;
extern EFldD* CFld;


class DataEditor {
public:
	DataEditor() = default;
    WORD EditTxt(std::string& s, WORD pos, WORD maxlen, WORD maxcol, FieldType typ, bool del,
	             bool star, bool upd, bool ret, unsigned int Delta); // r86

private:
	static int CRec();
    int CNRecs() const;
    int AbsRecNr(int N);
    int LogRecNo(int N);
    bool IsSelectedRec(WORD I);
    bool EquOldNewRec();
    void RdRec(int N);
    bool CheckOwner(EditD* E);
    bool CheckKeyIn(EditD* E);
    bool ELockRec(EditD* E, int N, bool IsNewRec, bool Subset);
    WORD RecAttr(WORD I);
    WORD FldRow(EFldD* D, WORD I);
    bool HasTTWw(FieldDescr* F);
    void DisplEmptyFld(EFldD* D, WORD I);
    void Wr1Line(FieldDescr* F);
    void DisplFld(EFldD* D, WORD I, BYTE Color);
    void DisplRec(WORD I);
    bool LockRec(bool Displ);
    void UnLockRec(EditD* E);
    void NewRecExit();
    void SetCPage(WORD* c_page, ERecTxtD** rt);
    void DisplRecNr(int N);
    void AdjustCRec();
    bool TestIsNewRec();
    void SetSelectFalse();
    void PopEdit();
    void WrEStatus();
    void RdEStatus();
    void DuplFld(FileD* file_d1, FileD* file_d2, void* record1, void* record2, void* RPt, FieldDescr* field_d1,
                 FieldDescr* field_d2);
    bool IsFirstEmptyFld();
    void SetFldAttr(EFldD* D, WORD I, WORD Attr);
    void IVoff();
    void IVon();
    void SetRecAttr(WORD I);
    void DisplTabDupl();
    void DisplSysLine();
    void DisplBool();
    void DisplAllWwRecs();
    void SetNewWwRecAttr();
    void MoveDispl(WORD From, WORD Where, WORD Number);
    void SetNewCRec(int N, bool withRead);
    void WriteSL(StringListEl* SL);
    void DisplRecTxt();
    void GotoRecFld(int NewRec, EFldD* NewFld);
    void UpdMemberRef(void* POld, void* PNew);
    void WrJournal(char Upd, void* RP, double Time);
    bool LockForMemb(FileD* FD, WORD Kind, LockMode NewMd, LockMode& md);
    bool LockWithDep(LockMode CfMd, LockMode MembMd, LockMode& OldMd);
    void UnLockWithDep(LockMode OldMd);
    void UndoRecord();
    bool CleanUp();
    bool DelIndRec(int I, int N);
    bool DeleteRecProc();
    ChkD* CompChk(EFldD* D, char Typ);
    void FindExistTest(FrmlElem* Z, LinkD** LD);
    bool TestAccRight(StringList S);
    bool ForNavigate(FileD* FD);
    std::string GetFileViewName(FileD* FD, StringListEl** SL);
    void SetPointTo(LinkD* LD, std::string* s1, std::string* s2);
    void GetSel2S(std::string& s, std::string& s2, char C, WORD wh);
    bool EquRoleName(pstring S, LinkD* LD);
    bool EquFileViewName(FileD* FD, std::string S, EditOpt** EO);
    void UpwEdit(LinkD* LkD);
    void DisplChkErr(ChkD* C);
    bool OldRecDiffers();
    bool ExitCheck(bool MayDispl);
    int UpdateIndexes();
    bool WriteCRec(bool MayDispl, bool& Displ);
    void DuplFromPrevRec();
    void InsertRecProc(void* RP);
    void AppendRecord(void* RP);
    bool GotoXRec(XString* PX, int& N);
    EFldD* FindEFld(FieldDescr* F);
    void CreateOrErr(bool create, void* RP, int N);
    bool PromptSearch(bool create);
    bool PromptAndSearch(bool create);
    void PromptGotoRecNr();
    void CheckFromHere();
    void Sorting();
    void AutoReport();
    void AutoGraph();
    bool IsDependItem();
    void SetDependItem();
    void SwitchToAppend();
    bool CheckForExit(bool& Quit);
    bool FldInModeF3Key(FieldDescr* F);
    bool IsSkipFld(EFldD* D);
    bool ExNotSkipFld();
    bool CtrlMProc(WORD Mode);
    bool GoPrevNextRec(short Delta, bool Displ);
    bool GetChpt(pstring Heslo, int& NN);
    void SetCRec(int I);
    bool EditFreeTxt(FieldDescr* F, std::string ErrMsg, bool Ed, WORD& Brk);
    bool EditItemProc(bool del, bool ed, WORD& Brk);
    void SetSwitchProc();
    void PromptSelect();
    void SwitchRecs(short Delta);
    bool FinArgs(LinkD* LD, FieldDescr* F);
    void DisplEditWw();
    void DisplWwRecsOrPage(WORD* c_page, ERecTxtD** rt);
    void DuplOwnerKey();
    bool TestDuplKey(FileD* file_d, XKey* K);
    void DuplKeyMsg(XKey* K);
    void BuildWork();
    void SetStartRec();
    bool OpenEditWw();
    void RefreshSubset();
    void RunEdit(XString* PX, WORD& Brk);

    bool PromptB(std::string& S, FrmlElem* Impl, FieldDescr* F);
    std::string PromptS(std::string& S, FrmlElem* Impl, FieldDescr* F);
    double PromptR(std::string& S, FrmlElem* Impl, FieldDescr* F);
    /*called from Proc && Projmgr */
    void EditDataFile(FileD* FD, EditOpt* EO);
    bool SelFldsForEO(EditOpt* EO, LinkD* LD);
    void ImbeddEdit();
    void DownEdit();
    void ShiftF7Proc();
    bool ShiftF7Duplicate();
    bool DuplToPrevEdit();
    void Calculate2();
    void DelNewRec();
    EFldD* FrstFldOnPage(WORD Page);
    void F6Proc();
    int GetEdRecNo();
    void SetEdRecNoEtc(int RNr);
    bool StartProc(Instr_proc* ExitProc, bool Displ);
    void StartRprt(RprtOpt* RO);
    void UpdateEdTFld(LongStr* S);
    void UpdateTxtPos(WORD TxtPos);
    bool StartExit(EdExitD* X, bool Displ);
    WORD ExitKeyProc();
    void FieldHelp();
    void DisplLASwitches();
    void DisplLL();
    void DisplCtrlAltLL(WORD Flags);
    void CtrlReadKbd();
    void MouseProc();
    void ToggleSelectRec();
    void ToggleSelectAll();
    void GoStartFld(EFldD* SFld);


    WORD FieldEdit(FieldDescr* F, FrmlElem* Impl, WORD LWw, WORD iPos, std::string& Txt,
                   double& RR, bool del, bool upd, bool ret, unsigned int Delta);
    void SetWasUpdated(FandFile* fand_file, void* record);
    void AssignFld(FieldDescr* F, FrmlElem* Z);
    bool TestMask(std::string& S, std::string Mask);
    void WrPromptTxt(std::string& S, FrmlElem* Impl, FieldDescr* F, std::string& Txt, double& R);


    bool TxtEdCtrlUBrk, TxtEdCtrlF4Brk;
    EFldD* CFld;

    //EditD* E = EditDRoot;
    EFldD* FirstEmptyFld;
    XKey* VK;
    XWKey* WK;
    static int BaseRec;
    static BYTE IRec;
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
};
