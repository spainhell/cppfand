#pragma once
#include "access-structs.h"
#include "base.h"

#include "../Common/FileD.h"
#include "../Common/LocVar.h"
#include "Project.h"
#include "switches.h"
#ifdef FandSQL
#include "channel.h"
#endif

const WORD Alloc = 2048;


struct FuncD
{
	std::string name;
	char FTyp = '\0';
	LocVarBlock LVB; // {1.LV is result}
	std::vector<Instr*> v_instr; // {InstrPtr}
};

enum class ForAllFilesOperation {
	close, save, save_l_mode, set_old_lock_mode, close_passive_fd, clear_xf_update_lock, find_fd_for_i
};

void ForAllFDs(ForAllFilesOperation op, FileD** file_d = nullptr, WORD i = 0);

std::string TranslateOrd(std::string text);

Record* LinkUpw(LinkD* LD, int& N, bool WithT, Record* record);
void AsgnParFldFrml(FileD* file_d, FieldDescr* field_d, FrmlElem* frml, bool add);
void TestCPathError();
std::string CExtToT(const std::string& dir, const std::string& name, std::string ext);
std::string CExtToX(std::string dir, std::string name, std::string ext);
