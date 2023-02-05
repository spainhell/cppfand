#pragma once
#include "base.h"
#include "Chained.h"
#include "Rdb.h"
#include "TFile.h"
#include "../Indexes/XKey.h"
#include "XFile.h"

class FieldDescr;
struct StringListEl;
class AddD;

//enum FileType
//{
//	fand8 = '8',
//	fand16 = '6',
//	index = 'x',
//	rdb = '0',
//	cat = 'c',
//	dbf = 'd',
//	unknown = -1
//};

class FileD : public Chained<FileD>
{
public:
	FileD();
	FileD(const FileD& orig);
	std::string Name;
	std::string FullName;
	WORD RecLen = 0;
	void* RecPtr = nullptr;
	longint NRecs = 0;
	bool WasWrRec = false, WasRdOnly = false, Eof = false;
	char Typ = '0';           // 8 = Fand 8; 6 = Fand 16; X = .X; 0 = RDB; C = CAT 
	FILE* Handle = nullptr;
	longint IRec = 0;
	WORD FrstDispl = 0;
	TFile* TF = nullptr;
	RdbPos ChptPos;           // zero for Rdb and FD translated from string 
	WORD TxtPosUDLI = 0;      // =0 if not present; urcuje zacatek odstavcu #U #D #L #I
	FileD* OrigFD = nullptr;  // like orig. or nil
	BYTE Drive = 0;           // 1=A, 2=B, else 0
	WORD CatIRec = 0;
	std::vector<FieldDescr*> FldD;
	bool IsParFile = false, IsJournal = false, IsHlpFile = false;
	bool typSQLFile = false, IsSQLFile = false, IsDynFile = false;
	FileUseMode UMode = FileUseMode::Closed;
	LockMode LMode = NullMode, ExLMode = NullMode, TaLMode = NullMode;
	StringListEl* ViewNames = nullptr;  //after each string BYTE string with user codes 
	XFile* XF = nullptr;
	std::vector <XKey*> Keys;
	std::vector<AddD*> Add;
	uintptr_t nLDs = 0, LiOfs = 0;
	longint UsedFileSize();
	bool IsShared();
	bool NotCached();
	bool Cached();
	WORD GetNrKeys();
	void Reset();
};
typedef FileD* FileDPtr;

void ReadRec(FileD* file, longint N, void* record);
void WriteRec(FileD* file, longint N, void* record);
