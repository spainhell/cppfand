#pragma once
#include <memory>

#include "../Common/rdbPos.h"
#include "../fandio/Fand0File.h"
#include "../fandio/DbfFile.h"
#include "../fandio/locks.h" // to be visible in other parts of code

class Record;
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
	FileD(DataFileType f_type, ProgressCallbacks callbacks);
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
	std::vector<std::string> ViewNames;  // after each string uint8_t string with user codes 

	int GetNRecs();
	void SetNRecs(int recs);
	long GetFileSize();
	uint16_t GetNrKeys();
	unsigned short GetFirstRecPos();
	uint16_t GetRecLen();
	void SetRecLen(uint16_t length);
	void Reset();
	size_t GetRecordSize();

	size_t ReadRec(size_t rec_nr, Record* record) const;
	size_t UpdateRec(size_t rec_nr, Record* record) const;
	size_t UpdateRec(size_t rec_nr, Record* old_record, Record* new_record) const;
	void PutRec(Record* record);
	
	
	int UsedFileSize() const;

	bool GetWasRdOnly() const;
	void SetWasRdOnly(bool was_read_only) const;
	void SetHandle(HANDLE handle);
	void SetHandleT(HANDLE handle);
	int32_t CheckT(int file_size);
	int32_t CheckX(int file_size);

	void CompileRecLen() const;

	void IncNRecs(int n);
	void DecNRecs(int n);
	void SeekRec(int n);
	void CreateRec(int n, Record* record) const;
	void DeleteRec(int n, Record* record) const;
	void RecallRec(int recNr, Record* record);
	void AssignNRecs(bool Add, int N);

	void SortByKey(std::vector<KeyFldD*>& keys) const;
	void IndexesMaintenance(bool remove_deleted);

	void SetDrive(uint8_t drive) const;
	void SetUpdateFlag() const;
	void Close() const;
	void CloseFile() const;
	void Save() const;

	void CreateT(const std::string& path) const;

	FileUseMode GetUMode() const;
	LockMode GetLMode() const;
	LockMode GetExLMode() const;
	LockMode GetTaLMode() const;

	void SetUMode(FileUseMode mode) const;
	void SetLMode(LockMode mode) const;
	void SetExLMode(LockMode mode) const;
	void SetTaLMode(LockMode mode) const;

	void OldLockMode(LockMode mode);
	LockMode NewLockMode(LockMode mode);
	bool TryLockMode(LockMode mode, LockMode& old_mode, uint16_t kind);
	bool ChangeLockMode(LockMode mode, uint16_t kind, bool rd_pref);
	bool Lock(int32_t n, uint16_t kind) const;
	void Unlock(int32_t n);
	void RunErrorM(LockMode mode);

	uint16_t RdPrefix() const;
	void WrPrefix() const;
	void WrPrefixes() const;

	bool IsIndexFile() const;
	bool HasIndexFile() const;
	bool HasTextFile() const;

	bool SearchKey(XString& XX, XKey* Key, int& NN, Record* record) const;
	bool SearchXKey(XKey* K, XString& X, int& N);

	FileD* OpenDuplicateF(bool createTextFile);
	void DeleteDuplicateF(FileD* TempFD);

	std::string CExtToT(const std::string& dir, const std::string& name, std::string ext);
	std::string SetTempCExt(char typ, bool isNet);
	void SetHCatTyp(FandFileType fand_file_type);
	void GetTFileD(bool has_tt);
	int32_t GetXFileD();

	bool IsActiveRdb();
	bool IsOpen();
	bool IsShared();
	bool NotCached();
	bool Cached();

	bool OpenF(const std::string& path, FileUseMode UM, bool is_project_file);
	// open file(s) from disk
	bool OpenF1(const std::string& path, FileUseMode UM, bool is_project_file);
	// load prefix(es) from file
	bool OpenF2(const std::string& path, bool is_project_file);
	void CreateF(bool is_project_file);
	bool OpenCreateF(const std::string& path, FileUseMode UM, bool is_project_file);
	void DeleteF();

	void TestCFileError();
	std::string SetPathMountVolumeSetNet(FileUseMode UM, bool is_project_file);
	std::string SetPathAndVolume(char pathDelim = '\\');
	void CFileError(int N);

	static void CloseAllAfter(FileD* first_for_close, std::vector<FileD*>& v_files);
	static void CloseAndRemoveAllAfter(FileD* first_for_remove, std::vector<FileD*>& v_files);
	static void CloseAndRemoveAllAfter(size_t first_index_for_remove, std::vector<FileD*>& v_files);

	static void CopyH(HANDLE h1, HANDLE h2);
	static std::string SetPathForH(HANDLE handle);

	Record* LinkLastRec(int32_t& n);

private:
	//Fand0File* FF = nullptr;	// FandFile reference
	void lock_excl_and_write_prefix();
};
