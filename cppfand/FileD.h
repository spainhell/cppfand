#pragma once
#include "base.h"
#include "Chained.h"
#include "Rdb.h"
#include "../fandio/FandFile.h"

class FieldDescr;
struct StringListEl;
class AddD;

enum class FType
{
	FandFile,
	SQLite,
	PostgreSQL,
	MSSQL,
	MariaDB
};

class FileD : public Chained<FileD>
{
public:
	FileD(FType f_type);
	FileD(const FileD& orig);
	std::string Name;
	std::string FullPath;

	FType FileType;

	FandFile* FF = nullptr;   // FandFile reference

	int IRec = 0;
	RdbPos ChptPos;           // zero for Rdb and FD translated from string 
	size_t TxtPosUDLI = 0;    // =0 if not present; urcuje zacatek odstavcu #U #D #L #I
	FileD* OrigFD = nullptr;  // like orig. or nil

	int CatIRec = 0;          // cislo zaznamu v katalogu
	bool IsParFile = false;
	bool IsJournal = false;
	bool IsHlpFile = false;
	bool typSQLFile = false;
	bool IsSQLFile = false;
	bool IsDynFile = false;

	std::vector<FieldDescr*> FldD;
	std::vector<XKey*> Keys;
	std::vector<AddD*> Add;
	StringListEl* ViewNames = nullptr;  //after each string BYTE string with user codes 

	WORD GetNrKeys();
	void Reset();

	void ReadRec(size_t rec_nr, void* record);
	void WriteRec(size_t rec_nr, void* record);

	BYTE* GetRecSpace();

	void IncNRecs(int n);
	void DecNRecs(int n);
	void SeekRec(int n);
	void CreateRec(int n, void* record);
	void PutRec(void* record);
	void DeleteRec(int n, void* record);
	void DelAllDifTFlds(void* Rec, void* CompRec);
	void RecallRec(int recNr, void* record);
		
	bool loadB(FieldDescr* field_d, void* record);
	double loadR(FieldDescr* field_d, void* record);
	std::string loadS(FieldDescr* field_d, void* record);
	pstring loadOldS(FieldDescr* field_d, void* record);
	LongStr* loadLongS(FieldDescr* field_d, void* record);
	int loadT(FieldDescr* field_d, void* record); // pozice textu v .T00 souboru (ukazatel na zacatek textu)

	void saveB(FieldDescr* field_d, bool b, void* record);
	void saveR(FieldDescr* field_d, double r, void* record);
	void saveS(FieldDescr* field_d, std::string s, void* record);
	void saveLongS(FieldDescr* field_d, LongStr* ls, void* record);
	int saveT(FieldDescr* field_d, int pos, void* record);

	void Close();
	void Save();

	void OldLockMode(LockMode mode);
	LockMode NewLockMode(LockMode mode);
	bool TryLockMode(LockMode mode, LockMode& old_mode, WORD kind);
	bool ChangeLockMode(LockMode mode, WORD kind, bool rd_pref);
	bool Lock(int n, WORD kind);
	void Unlock(int n);
	void RunErrorM(LockMode mode);

	void SetTWorkFlag(void* record);
	bool HasTWorkFlag(void* record);
	void SetUpdFlag(void* record);
	void ClearUpdFlag(void* record);
	bool HasUpdFlag(void* record);
	bool DeletedFlag(void* record);
	void ClearDeletedFlag(void* record);
	void SetDeletedFlag(void* record);

	bool SearchKey(XString& XX, XKey* Key, int& NN);
};
