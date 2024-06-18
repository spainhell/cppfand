#pragma once
#include "Chained.h"
#include "Rdb.h"
#include "../fandio/FandFile.h"

class FieldDescr;
class AddD;

enum class FType
{
	FandFile,
	DBF,
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
	~FileD();

	std::string Name;
	std::string FullPath;

	FType FileType;

	FandFile* FF = nullptr;   // FandFile reference

	int IRec = 0;
	RdbPos ChptPos;           // zero for Rdb and rdb_file translated from string 
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
	std::vector<std::string> ViewNames;  //after each string BYTE string with user codes 

	WORD GetNrKeys();
	void Reset();

	void ReadRec(size_t rec_nr, void* record) const;
	void WriteRec(size_t rec_nr, void* record) const;

	BYTE* GetRecSpace();

	// delete 'T' from working file if exists
	void ClearRecSpace(void* record);

	void IncNRecs(int n);
	void DecNRecs(int n);
	void SeekRec(int n);
	void CreateRec(int n, void* record) const;
	void PutRec(void* record);
	void DeleteRec(int n, void* record) const;
	void RecallRec(int recNr, void* record);
	void AssignNRecs(bool Add, int N);
		
	bool loadB(FieldDescr* field_d, void* record);
	double loadR(FieldDescr* field_d, void* record);
	std::string loadS(FieldDescr* field_d, void* record);
	LongStr* loadLongS(FieldDescr* field_d, void* record);
	int loadT(FieldDescr* field_d, void* record); // pozice textu v .T00 souboru (ukazatel na zacatek textu)

	void saveB(FieldDescr* field_d, bool b, void* record);
	void saveR(FieldDescr* field_d, double r, void* record);
	void saveS(FieldDescr* field_d, const std::string& s, void* record);
	void saveLongS(FieldDescr* field_d, LongStr* ls, void* record);
	int saveT(FieldDescr* field_d, int pos, void* record);

	void CloseFile();
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

	bool SearchKey(XString& XX, XKey* Key, int& NN, void* record);
	FileD* OpenDuplicateF(bool createTextFile);
	void DeleteDuplicateF(FileD* TempFD);
	void ZeroAllFlds(void* record, bool delTFields);
	void CopyRec(void* record1, void* record2, bool delTFields);

	//void DelTFlds(void* record);
	void DelAllDifTFlds(void* record, void* comp_record);

	bool IsActiveRdb();
};
