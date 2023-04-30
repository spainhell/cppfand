#pragma once
#include "FieldDescr.h"
#include "FileD.h"

class Catalog
{
public:
	Catalog();
	Catalog(FileD* file_d);
	~Catalog();
	
	FileD* GetCatalogFile();

	std::string GetRdbName(size_t rec_nr);
	std::string GetFileName(size_t rec_nr);
	std::string GetArchive(size_t rec_nr);
	std::string GetPathName(size_t rec_nr);
	std::string GetVolume(size_t rec_nr);
	std::string GetField(size_t rec_nr, FieldDescr* field);

	void SetRdbName(size_t rec_nr, const std::string& rdb_name);
	void SetFileName(size_t rec_nr, const std::string& file_name);
	void SetArchive(size_t rec_nr, const std::string& archive);
	void SetPathName(size_t rec_nr, const std::string& path_name);
	void SetVolume(size_t rec_nr, const std::string& volume);
	void SetField(size_t rec_nr, FieldDescr* field, const std::string& value);

	FieldDescr* CatalogArchiveField();
	FieldDescr* CatalogPathNameField();
	FieldDescr* CatalogVolumeField();

	bool OldToNewCat(int& FilSz);

	int GetCatalogIRec(const std::string& name, bool multilevel);
	void GetCPathForCat(FileD* file_d, int i, std::string& path, std::string& volume);

	WORD Generation(FileD* file_d);
	void TurnCat(FileD* file_d, WORD Frst, WORD N, short I);

private:
	FileD* cat_file_;
	BYTE* record_;
	FieldDescr* cat_rdb_name_;
	FieldDescr* cat_file_name_;
	FieldDescr* cat_archive_;
	FieldDescr* cat_path_name_;
	FieldDescr* cat_volume_;
	std::string getValue(size_t rec_nr, FieldDescr* field);
	void setValue(size_t rec_nr, FieldDescr* field, const std::string& value);
};

