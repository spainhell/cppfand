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

	longint IRec = 0;
	RdbPos ChptPos;           // zero for Rdb and FD translated from string 
	size_t TxtPosUDLI = 0;      // =0 if not present; urcuje zacatek odstavcu #U #D #L #I
	FileD* OrigFD = nullptr;  // like orig. or nil
	
	WORD CatIRec = 0;
	std::vector<FieldDescr*> FldD;
	bool IsParFile = false;
	bool IsJournal = false;
	bool IsHlpFile = false;
	bool typSQLFile = false;
	bool IsSQLFile = false;
	bool IsDynFile = false;
	
	StringListEl* ViewNames = nullptr;  //after each string BYTE string with user codes 
	
	std::vector<XKey*> Keys;
	std::vector<AddD*> Add;

	WORD GetNrKeys();
	void Reset();

	void ReadRec(size_t rec_nr, void* record);
	void WriteRec(size_t rec_nr, void* record);
};
