#pragma once

#include "rdrun.h"
#include "wwmix.h"

struct RdbRecVars;
class DataEditor;

class ProjectRunner {
public:
	void ReleaseFilesAndLinksAfterChapter(EditD* edit);
	bool ChptDel(FileD* file_d, DataEditor* data_editor);
	WORD ChptWriteCRec(DataEditor* data_editor, EditD* edit); /* 0-O.K., 1-fail, 2-fail && undo*/
	bool PromptHelpName(FileD* file_d, WORD& N);
	void EditHelpOrCat(WORD cc, WORD kind, std::string txt);
	void StoreChptTxt(FieldDescr* F, std::string text, bool Del);
	bool EditExecRdb(const std::string& name, const std::string& proc_name, Instr_proc* proc_call, wwmix* ww);
	void UpdateCat();
	void UpdateUTxt();
	void InstallRdb(std::string n);
	void CreateOpenChpt(std::string Nm, bool create);
	void CloseChpt();
	FileD* FindFD(Record* record);
	void Diagnostics(uint8_t* MaxHp, int Free, FileD* FD);
	bool CompRunChptRec(const std::unique_ptr<DataEditor>& rdb_editor, WORD CC);
	void RdUserId(bool check);
	WORD CompileMsgOn(CHAR_INFO* Buf, int& w);
	void CompileMsgOff(CHAR_INFO* Buf, int& w);
	FileD* RdF(FileD* file_d, std::string FileName, Record* record);
	FileD* RdOldF(FileD* file_d, const std::string& file_name, Record* record);
	bool EquStoredF(std::vector<FieldDescr*>& fields1, std::vector<FieldDescr*>& fields2);
	bool MergeAndReplace(FileD* fd_old, FileD* fd_new);
	bool EquKeys(std::vector<XKey*>& K1, std::vector<XKey*>& K2);
	bool MergeOldNew(FileD* new_file, FileD* old_file);
	bool CompileRdb(FileD* rdb_file, bool displ, bool run, bool from_CtrlF10);
	void GotoErrPos(WORD& Brk, std::unique_ptr<DataEditor>& data_editor);
	void WrErrMsg630(std::string Nm);
	void Finish_EditExecRdb(bool wasGraph, int w);

private:
	RdbD* PrepareRdb(const std::string& name, std::string& name1);
	FandFileType ExtToTyp(const std::string& ext);
	bool NetFileTest(RdbRecVars* X);
	void GetSplitChapterName(FileD* file_d, Record* record, std::string& name, std::string& ext);
	void GetRdbRecVars(const EditD* edit, Record* record, RdbRecVars* X);
	bool ChptDelFor(EditD* edit, RdbRecVars* X);
	bool IsDuplFileName(DataEditor* data_editor, std::string name);
	void RenameWithOldExt(RdbRecVars New, RdbRecVars Old);
	WORD FindHelpRecNr(FileD* FD, std::string& txt);
	void SetChptFldD();
	void SetRdbDir(FileD* file_d, char Typ, std::string* Nm);
	void ResetRdOnly();


	int UserW = 0;
	int sz = 0; 
	WORD nTb = 0; 
	void* Tb = nullptr;
};
