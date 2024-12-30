#pragma once
#include "access-structs.h"
#include "base.h"

#include "FileD.h"
#include "LocVar.h"
#include "Rdb.h"
#include "switches.h"
#ifdef FandSQL
#include "channel.h"
#endif

const WORD Alloc = 2048;


struct FuncD
{
	std::string name;
	//FuncD* Chain = nullptr;
	char FTyp = '\0';
	LocVarBlock LVB; // {1.LV is result}
	std::vector<Instr*> v_instr; // {InstrPtr}
};

enum class ForAllFilesOperation {
	close, save, save_l_mode, set_old_lock_mode, close_passive_fd, clear_xf_update_lock, find_fd_for_i
};

void ForAllFDs(ForAllFilesOperation op, FileD** file_d = nullptr, WORD i = 0);

std::string TranslateOrd(std::string text);

bool LinkUpw(FileD* file_d, LinkD* LD, int& N, bool WithT, void* record, BYTE** newRecord);
bool LinkLastRec(FileD* file_d, int& N, bool WithT, BYTE** newRecord);

void AsgnParFldFrml(FileD* file_d, FieldDescr* field_d, FrmlElem* frml, bool add);

//void CloseClearH(FandFile* fand_file);
void TestCPathError();

void CloseGoExit(Fand0File* fand_file);

std::string CExtToT(FandTFile* t_file, const std::string& dir, const std::string& name, std::string ext);
std::string CExtToX(std::string dir, std::string name, std::string ext);
