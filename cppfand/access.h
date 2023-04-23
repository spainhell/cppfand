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

void* LocVarAd(LocVar* LV);
bool EquKFlds(KeyFldD* KF1, KeyFldD* KF2);
int StoreInTWork(LongStr* S);
LongStr* ReadDelInTWork(int Pos);
void ForAllFDs(void (*procedure)());
bool IsActiveRdb(FileD* FD);
void ResetCompilePars(); // r953 - posledni fce

std::string TranslateOrd(std::string text); // r804 ASM

// * NACITANI ZE SOUBORU / Z FRMLELEM *
double _R(FieldDescr* F);
pstring _ShortS(FieldDescr* F);
std::string _StdS(FieldDescr* F, void* record);
LongStr* _LongS(FieldDescr* F);

// * UKLADANI DO SOUBORU * / DO FRMLELEM *

void S_(FileD* file_d, FieldDescr* F, std::string S, void* record = nullptr);
void LongS_(FileD* file_d, FieldDescr* F, LongStr* S);

bool LinkUpw(LinkD* LD, int& N, bool WithT);
bool LinkLastRec(FileD* FD, int& N, bool WithT);

void ClearRecSpace(void* p);
void ZeroAllFlds(FileD* file_d, void* record);

void DelTFld(FieldDescr* F);
void DelTFlds();
void DelDifTFld(void* Rec, void* CompRec, FieldDescr* F);

void AsgnParFldFrml(FileD* FD, FieldDescr* F, FrmlElem* Z, bool Ad);

void CopyRecWithT(void* p1, void* p2);
void CloseClearHCFile(FandFile* fand_file);
void TestCPathError();
void AssignNRecs(bool Add, int N);

void CloseGoExit(FandFile* fand_file);

void ClearCacheCFile();
void ResetCFileUpdH();

void FixFromReal(double r, void* FixNo, WORD FLen);

std::string CExtToT(std::string dir, std::string name, std::string ext);
std::string CExtToX(std::string dir, std::string name, std::string ext);
