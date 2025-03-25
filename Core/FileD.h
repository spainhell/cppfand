#pragma once
#include <memory>

#include "Rdb.h"
#include "../fandio/Fand0File.h"
#include "../fandio/DbfFile.h"
#include "../fandio/locks.h" // to be visible in other parts of code

class FieldDescr;
class Additive;

enum class DataFileType
{
	FandFile,
	DBF,
	SQLite,
	PostgreSQL,
	MSSQL,
	MariaDB
};

class FileD
{
public:
	FileD(DataFileType f_type);
	FileD(const FileD& orig);
	~FileD();

	std::string Name;
	std::string FullPath;

	DataFileType FileType;

	Fand0File* FF = nullptr;	// FandFile reference
	DbfFile* DbfF = nullptr;	// DBF file reference

	int IRec = 0;
	RdbPos ChptPos;           // zero for Rdb and v_files translated from string 
	size_t TxtPosUDLI = 0;    // =0 if not present; urcuje zacatek odstavcu #U #D #L #I
	FileD* OrigFD = nullptr;  // like orig. or nil

	int32_t CatIRec = 0;          // cislo zaznamu v katalogu
	bool IsParFile = false;
	bool IsJournal = false;
	bool IsHlpFile = false;
	bool typSQLFile = false;
	bool IsSQLFile = false;
	bool IsDynFile = false;

	std::vector<FieldDescr*> FldD;
	std::vector<XKey*> Keys;
	std::vector<Additive*> Add;
	std::vector<std::string> ViewNames;  // after each string BYTE string with user codes 

	WORD GetNrKeys();
	void Reset();

	size_t ReadRec(size_t rec_nr, void* record) const;
	size_t WriteRec(size_t rec_nr, void* record) const;

	uint8_t* GetRecSpace() const;
	std::unique_ptr<uint8_t[]> GetRecSpaceUnique() const;

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

	void SortByKey(std::vector<KeyFldD*>& keys) const;
	void IndexesMaintenance(bool remove_deleted);

	bool loadB(FieldDescr* field_d, void* record);
	double loadR(FieldDescr* field_d, void* record);
	std::string loadS(FieldDescr* field_d, void* record);
	int loadT(FieldDescr* field_d, void* record); // pozice textu v .T00 souboru (ukazatel na zacatek textu)

	void saveB(FieldDescr* field_d, bool b, void* record);
	void saveR(FieldDescr* field_d, double r, void* record);
	void saveS(FieldDescr* field_d, const std::string& s, void* record);
	void saveLongS(FieldDescr* field_d, LongStr* ls, void* record);
	int saveT(FieldDescr* field_d, int pos, void* record);

	void SetUpdateFlag();
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

	void SetRecordUpdateFlag(void* record);
	void ClearRecordUpdateFlag(void* record);
	bool HasRecordUpdateFlag(void* record);

	bool DeletedFlag(void* record);
	void ClearDeletedFlag(void* record);
	void SetDeletedFlag(void* record);

	bool SearchKey(XString& XX, XKey* Key, int& NN, void* record);
	bool SearchXKey(XKey* K, XString& X, int& N);

	FileD* OpenDuplicateF(bool createTextFile);
	void DeleteDuplicateF(FileD* TempFD);
	void ZeroAllFlds(void* record, bool delTFields);
	void CopyRec(uint8_t* src_record, uint8_t* dst_record, bool delTFields);
	void DelAllDifTFlds(void* record, void* comp_record);

	std::string CExtToT(const std::string& dir, const std::string& name, std::string ext);
	void SetHCatTyp(FandFileType fand_file_type);

	bool IsActiveRdb();

	static void CloseAllAfter(FileD* first_for_close, std::vector<FileD*>& v_files);
	static void CloseAndRemoveAllAfter(FileD* first_for_remove, std::vector<FileD*>& v_files);
	static void CloseAndRemoveAllAfter(size_t first_index_for_remove, std::vector<FileD*>& v_files);
};
