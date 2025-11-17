#pragma once
#include "../fandio/FieldDescr.h"
#include "../Common/FileD.h"

class Catalog
{
public:
	Catalog();
	Catalog(FileD* file_d);
	~Catalog();

	void Close();
	
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

	bool OldToNewCat(int& file_size);

	int GetCatalogIRec(const std::string& name, bool multilevel);
	void GetPathAndVolume(FileD* file_d, int rec_nr, std::string& path, std::string& volume) const;

	uint16_t Generation(FileD* file_d, std::string& path, std::string& volume);
	void TurnCat(FileD* file_d, uint16_t Frst, uint16_t N, short I);

private:
	FileD* cat_file_;
	uint8_t* record_;
	FieldDescr* cat_rdb_name_;
	FieldDescr* cat_file_name_;
	FieldDescr* cat_archive_;
	FieldDescr* cat_path_name_;
	FieldDescr* cat_volume_;
	std::string getValue(size_t rec_nr, FieldDescr* field);
	void setValue(size_t rec_nr, FieldDescr* field, const std::string& value);
};

