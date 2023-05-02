#pragma once
#include "access-structs.h"
#include "base.h"
#include "constants.h"
#include "FileD.h"
#include "LocVar.h"
#include "Rdb.h"
#include "switches.h"
#ifdef FandSQL
#include "channel.h"
#endif

const WORD Alloc = 2048;

struct DepD
{
	FrmlElem* Bool = nullptr;
	FrmlElem* Frml = nullptr;
};

struct FuncD
{
	FuncD* Chain = nullptr;
	char FTyp = '\0';
	LocVarBlkD LVB; // {1.LV is result}
	Instr* pInstr = nullptr; // {InstrPtr}
	pstring Name;
};

enum class ForAllFilesOperation {
	close, save, save_l_mode, set_old_lock_mode, close_passive_fd, clear_xf_update_lock, find_fd_for_i
};

void* LocVarAd(LocVar* LV);

void ForAllFDs(ForAllFilesOperation op, FileD** file_d = nullptr, WORD i = 0);

void ResetCompilePars(); // r953 - posledni fce

std::string TranslateOrd(std::string text);

bool LinkUpw(LinkD* LD, int& N, bool WithT);
bool LinkLastRec(FileD* FD, int& N, bool WithT);



void CloseClearHCFile(FandFile* fand_file);
void TestCPathError();

void CloseGoExit(FandFile* fand_file);

void ClearCacheCFile();
void ResetCFileUpdH();


std::string CExtToT(const std::string& dir, const std::string& name, std::string ext);
std::string CExtToX(std::string dir, std::string name, std::string ext);
