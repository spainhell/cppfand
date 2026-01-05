#pragma once

#include <memory>

#include "DataEditorParams.h"
#include "../Core/RunFrml.h"
#include "../Core/EditOpt.h"
#include "../Core/rdrun.h"
#include "../MergeReport/RprtOpt.h"

class ProjectRunner;
enum class FieldType;
class FieldDescr;
class EditableField;


class DataEditor {
public:
	DataEditor();
    DataEditor(EditD* edit);
    DataEditor(FileD* file_d);
    ~DataEditor();

	FileD* GetFileD();
    void SetEditD(EditD* edit);
	void SetFileD(FileD* file_d);

    Record* GetRecord() const;
    Record* GetOriginalRecord() const;

	void EditDataFile(FileD* FD, EditOpt* EO);
    uint16_t EditTxt(std::string& text, uint16_t pos, uint16_t maxlen, uint16_t maxcol, FieldType typ, bool del,
	             bool star, bool upd, bool ret, unsigned int Delta); // r86
    int CRec();
	void UpdateTextField(std::string& text);
    bool StartExit(EdExitD* X, bool Displ);
    bool PromptB(std::string& S, FrmlElem* Impl, FieldDescr* F);
    std::string PromptS(std::string& S, FrmlElem* Impl, FieldDescr* F);
    double PromptR(std::string& S, FrmlElem* Impl, FieldDescr* F);
    bool TestIsNewRec();
    //EditD* WriteParamsToE();
    void ReadParamsFromE(const EditD* edit);
    bool SelFldsForEO(EditOpt* EO, LinkD* LD);
    void DisplEditWw();
    void GotoPrevRecFld(int NewRec, std::vector<EditableField*>::iterator NewFld);
    void GotoNextRecFld(int NewRec, std::vector<EditableField*>::iterator NewFld);
    void GotoRecFld(int NewRec, const std::vector<EditableField*>::iterator& NewFld);
    void SetNewCRec(int N, bool withRead);
    bool EditFreeTxt(FieldDescr* F, std::string ErrMsg, bool Ed, uint16_t& Brk);
    bool OpenEditWw();
    void RunEdit(XString* PX, uint16_t& Brk);
    void SetSelectFalse();
    //void PopEdit();
    EditD* GetEditD();

    bool TxtEdCtrlUBrk = false;
    bool TxtEdCtrlF4Brk = false;
    std::vector<EditableField*>::iterator CFld;

private:
    FileD* file_d_ = nullptr;
    //uint8_t* record_ = nullptr;
    //uint8_t* original_record_ = nullptr;
    Record* current_rec_ = nullptr;
    Record* original_rec_ = nullptr;
    bool current_rec_is_ref = false;    // current record is owned by another object (will not be deleted in destructor)
	std::unique_ptr<DataEditorParams> params_;
    std::unique_ptr<ProjectRunner> runner_;
    EditD* edit_ = nullptr;

    std::vector<EditableField*>::iterator FirstEmptyFld;

    int CNRecs() const;
    int AbsRecNr(int N);
    int LogRecNo(int N);
    bool IsSelectedRec(uint16_t I, Record* record);
    bool EquOldNewRec();
    void RdRec(int nr, Record* record);
    bool CheckOwner(EditD* E);
    bool CheckKeyIn(EditD* E);
    bool ELockRec(EditD* E, int N, bool IsNewRec, bool Subset);
    uint16_t RecAttr(uint16_t I, Record* record);
    uint16_t FldRow(EditableField* D, uint16_t I);
    bool HasTTWw(FieldDescr* F);
    void DisplEmptyFld(EditableField* D, uint16_t I);
    void Wr1Line(FieldDescr* field, Record* record) const;
    void DisplFld(EditableField* D, uint16_t I, uint8_t Color, Record* record);
    void DisplayRecord(uint16_t screen_data_row_nr);
    bool LockRec(bool Displ);
    void UnLockRec(EditD* E);
    void NewRecExit();
    void SetCPage(uint16_t& c_page, ERecTxtD** rt);
    void DisplRecNr(int N);
    void AdjustCRec();
	
	/* void DuplFld(FileD* src_file, FileD* dst_file, uint8_t* src_rec, uint8_t* dst_rec_new, 
        uint8_t* dst_rec_old, FieldDescr* srd_fld, FieldDescr* dst_fld); */

    void DuplicateField(Record* src_record, FieldDescr* src_field, Record* dst_record, FieldDescr* dst_field);

    bool IsFirstEmptyFld();
    void SetFldAttr(EditableField* D, uint16_t I, uint16_t Attr);
    void HighLightOff();
    void HighLightOn();
    void SetRecAttr(uint16_t I);
    void DisplTabDupl();
    void DisplaySystemLine();
    void DisplBool();
    void DisplAllWwRecs();
    void SetNewWwRecAttr();
    void MoveDispl(WORD From, WORD Where, WORD Number);
    void WriteSL(std::vector<std::string>& SL);
    void DisplRecTxt();
    
    void UpdMemberRef(Record* old_record, Record* new_record);
    void WrJournal(char Upd, Record* record, double Time);
    bool LockForMemb(FileD* FD, WORD Kind, LockMode NewMd, LockMode& md);
    bool LockWithDep(LockMode CfMd, LockMode MembMd, LockMode& OldMd);
    void UnLockWithDep(LockMode OldMd);
    void UndoRecord();
    bool CleanUp();
    bool DelIndRec(int I, int N);
    bool DeleteRecProc();
    LogicControl* CompChk(EditableField* D, char Typ);
    void FindExistTest(FrmlElem* Z, LinkD** LD);
    bool TestAccRight(const std::string& acc_rights);
    bool ForNavigate(FileD* FD);
    std::string GetFileViewName(FileD* FD, const std::string& view_name);
    void SetPointTo(LinkD* LD, std::string* s1, std::string* s2);
    void GetSel2S(std::string& s, std::string& s2, char C, WORD wh);
    bool EquRoleName(pstring S, LinkD* LD);
    bool EquFileViewName(FileD* FD, std::string S, EditOpt** EO);
    void UpwEdit(LinkD* LkD);
    void DisplChkErr(LogicControl* logic_control);
    bool OldRecDiffers();
    bool ExitCheck(bool MayDispl);
    int UpdateIndexes();
    bool WriteCRec(bool MayDispl, bool& Displ);
    void DuplFromPrevRec();
    void InsertRecProc(Record* RP);
    void AppendRecord(Record* RP);
    bool GotoXRec(XString* PX, int& N);
    std::vector<EditableField*>::iterator FindEFld(FieldDescr* F);
    void CreateOrErr(bool create, Record* RP, int N);
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
    bool IsSkipFld(EditableField* D);
    bool ExNotSkipFld();
    bool ProcessEnter(uint16_t mode); // orig. name: CtrlMProc
    bool GoPrevNextRec(short Delta, bool Displ);
    bool GetChpt(pstring Heslo, int& NN);
    void SetCRec(int I);
    bool EditItemProc(bool del, bool ed, WORD& brk);
    void SetSwitchProc();
    void PromptSelect();
    void SwitchRecs(short Delta);
    bool FinArgs(LinkD* LD, FieldDescr* F);
    void DisplWwRecsOrPage(WORD& c_page, ERecTxtD** rt);
    void DuplOwnerKey();
    bool TestDuplKey(FileD* file_d, XKey* K);
    void DuplKeyMsg(XKey* K);
    void BuildWork();
    void SetStartRec();
    void RefreshSubset();

    void ImbeddEdit();
    void DownEdit();
    void ShiftF7Proc();
    bool ShiftF7Duplicate();
    bool DuplToPrevEdit();
    void Calculate2();
    void DelNewRec();
    std::vector<EditableField*>::iterator FrstFldOnPage(WORD Page);
    void F6Proc();
    int GetEdRecNo();
    void SetEdRecNoEtc(int RNr);
    bool StartProc(Instr_proc* ExitProc, bool Displ);
    void StartRprt(RprtOpt* RO);
    
    void UpdateTxtPos(WORD TxtPos);
    
    WORD ExitKeyProc();
    void FieldHelp();
    void DisplLASwitches();
    void DisplLL();
    void DisplCtrlAltLL(WORD Flags);
    void CtrlReadKbd();
    void MouseProc();
    void ToggleSelectRec();
    void ToggleSelectAll();
    void GoStartFld(EditableField* SFld);

    std::string decodeField(FieldDescr* F, WORD LWw, Record* record);
    std::string decodeFieldRSB(FieldDescr* F, WORD LWw, double R, std::string& T, bool B);
	static void justifyString(std::string& T, WORD L, WORD M, char C);


    WORD FieldEdit(FieldDescr* F, FrmlElem* Impl, WORD LWw, WORD iPos, std::string& Txt,
                   double& RR, bool del, bool upd, bool ret, unsigned int Delta);
    void SetWasUpdated();
    void AssignFld(FieldDescr* F, FrmlElem* Z);
    bool TestMask(std::string& S, std::string Mask);
    void WrPromptTxt(std::string& S, FrmlElem* Impl, FieldDescr* field, std::string& Txt, double& R);

    XKey* VK = nullptr;
    XWKey* WK = nullptr;
    ERecTxtD* RT = nullptr;

    uint16_t UpdCount = false;
    uint16_t CPage = false;

    int BaseRec = 0;
    uint8_t IRec = 0;
    bool IsNewRec = false;

    bool HasIndex = false;
    bool HasTF = false;
    bool NewDisplLL = false;
};
