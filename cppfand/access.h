#pragma once
#include "access-structs.h"
#include "base.h"
#include "constants.h"
#include "FileD.h"
#include "LocVar.h"
#include "Rdb.h"
#include "../Indexes/XString.h"
#include "switches.h"
#ifdef FandSQL
#include "channel.h"
#endif

class FileD;
class KeyFldD;
class FandTFile;
class FandXFile;
struct FrmlListEl;
class FrmlElem;
class LocVar;
struct FuncD;
class XWKey;
class XWFile;
class XKey;
class LinkD;
class FieldDescr;

// ********** CONST **********
const BYTE LeftJust = 1; // {RightJust=0  coded in M for Typ='N','A'}
const BYTE Ascend = 0;
const BYTE Descend = 6; // {used in SortKey}
const BYTE f_Stored = 1;
const BYTE f_Encryp = 2; // {FieldD flags}
const BYTE f_Mask = 4;
const BYTE f_Comma = 8; // {FieldD flags}

struct DepD : Chained<DepD> // r122
{
	FrmlElem* Bool = nullptr;
	FrmlElem* Frml = nullptr;
};

using DepDPtr = DepD*;

struct FuncD
{
	FuncD* Chain = nullptr;
	char FTyp = '\0';
	LocVarBlkD LVB; // {1.LV is result}
	Instr* pInstr = nullptr; // {InstrPtr}
	pstring Name;
};

integer CompLongStr(LongStr* S1, LongStr* S2); // r529 ASM
integer CompLongShortStr(LongStr* S1, pstring* S2); // r551 ASM
integer CompArea(void* A, void* B, integer L); // r575 ASM
integer CompStr(pstring& S1, pstring& S2); // r792 ASM
int CompStr(std::string& S1, std::string& S2);

WORD CompLexLongStr(LongStr* S1, LongStr* S2);
WORD CompLexLongShortStr(LongStr* S1, pstring& S2);
WORD CompLexStr(pstring& S1, pstring& S2);
WORD CompLexStr(const pstring& S1, const pstring& S2);
WORD CompLexStrings(const std::string& S1, const std::string& S2);

void RunErrorM(LockMode Md, WORD N);
void* GetRecSpace(FandFile* fand_file);
size_t CFileRecSize(FandFile* fand_file);
void SetTWorkFlag(FandFile* fand_file, void* record);
bool HasTWorkFlag(FandFile* fand_file, void* record);
void SetUpdFlag(FandFile* fand_file, void* record);
void ClearUpdFlag(FandFile* fand_file, void* record);
bool HasUpdFlag(FandFile* fand_file, void* record);
void* LocVarAd(LocVar* LV);
bool DeletedFlag(FandFile* fand_file, void* record);
void ClearDeletedFlag(FandFile* fand_file, void* record);
void SetDeletedFlag(FandFile* fand_file, void* record);

bool EquKFlds(KeyFldD* KF1, KeyFldD* KF2);
void Code(std::string& data);
void Code(void* A, WORD L);
void CodingLongStr(LongStr* S);
longint StoreInTWork(LongStr* S);
LongStr* ReadDelInTWork(longint Pos);
void ForAllFDs(void (*procedure)());
bool IsActiveRdb(FileD* FD);
void ResetCompilePars(); // r953 - posledni fce

std::string TranslateOrd(std::string text); // r804 ASM

// * NACITANI ZE SOUBORU / Z FRMLELEM *
bool _B(FieldDescr* F);
double _R(FieldDescr* F);
pstring _ShortS(FieldDescr* F);
std::string _StdS(FieldDescr* F);
LongStr* _LongS(FieldDescr* F);
longint _T(FieldDescr* F);
longint _T(FieldDescr* F, unsigned char* data, FileType file_type);

// * UKLADANI DO SOUBORU * / DO FRMLELEM *
void B_(FieldDescr* F, bool B);
void R_(FieldDescr* F, double R, void* record = nullptr);
void S_(FieldDescr* F, std::string S, void* record = nullptr);
void LongS_(FieldDescr* F, LongStr* S);
void T_(FieldDescr* F, longint Pos);

void CreateRec(FileD* file_d, longint n);
void RecallRec(longint RecNr);
bool LinkUpw(LinkD* LD, longint& N, bool WithT);
bool LinkLastRec(FileD* FD, longint& N, bool WithT);
void IncNRecs(FileD* file_d, longint n);
bool TryLMode(FileD* fileD, LockMode Mode, LockMode& OldMode, WORD Kind);
void OldLMode(FileD* fileD, LockMode Mode);
LockMode NewLMode(FileD* fileD, LockMode Mode);
bool TryLockN(FandFile* fand_file, longint N, WORD Kind);
void UnLockN(FandFile* fand_file, longint N);
void ClearRecSpace(void* p);
void ZeroAllFlds(FileD* file_d, void* record);
void DelTFld(FieldDescr* F);
void DelDifTFld(void* Rec, void* CompRec, FieldDescr* F);

void DelAllDifTFlds(void* Rec, void* CompRec);
void DecNRecs(longint N);
void DeleteRec(longint N);
void AsgnParFldFrml(FileD* FD, FieldDescr* F, FrmlElem* Z, bool Ad);
void PutRec(FileD* dataFile, void* recordData);

void DelTFlds();
void CopyRecWithT(void* p1, void* p2);
void CloseClearHCFile();
void TestCPathError();
void AssignNRecs(bool Add, longint N);

std::string CExtToT(std::string dir, std::string name, std::string ext);
std::string CExtToX(std::string dir, std::string name, std::string ext);

void CloseGoExit();

bool ChangeLMode(FileD* fileD, LockMode Mode, WORD Kind, bool RdPref);
void SeekRec(FileD* fileD, longint N);

void FixFromReal(double r, void* FixNo, WORD FLen);
