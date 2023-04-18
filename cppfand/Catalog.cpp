#include "Catalog.h"
#include "access.h"
#include "../Common/textfunc.h"

Catalog::Catalog()
{
	cat_file_ = nullptr;
	record_ = nullptr;

	cat_rdb_name_ = nullptr;
	cat_file_name_ = nullptr;
	cat_archive_ = nullptr;
	cat_path_name_ = nullptr;
	cat_volume_ = nullptr;
}

Catalog::Catalog(FileD* file_d)
{
	cat_file_ = file_d;
	record_ = cat_file_->GetRecSpace();

	if (cat_file_->FldD.empty()) {
		throw std::exception("CompileHelpCatDcl: CatRdbName is NULL");
	}
	cat_rdb_name_ = cat_file_->FldD[0];

	cat_file_name_ = cat_file_->FldD[1];
	cat_archive_ = cat_file_->FldD[2];
	cat_path_name_ = cat_file_->FldD[3];
	cat_volume_ = cat_file_->FldD[4];
}

Catalog::~Catalog()
{
	delete cat_file_;
	delete[] record_;
}

FileD* Catalog::GetCatalogFile()
{
	return cat_file_;
}

std::string Catalog::GetRdbName(size_t rec_nr)
{
	return getValue(rec_nr, cat_rdb_name_);
}

std::string Catalog::GetFileName(size_t rec_nr)
{
	return getValue(rec_nr, cat_file_name_);
}

std::string Catalog::GetArchive(size_t rec_nr)
{
	return getValue(rec_nr, cat_archive_);
}

std::string Catalog::GetPathName(size_t rec_nr)
{
	return getValue(rec_nr, cat_path_name_);
}

std::string Catalog::GetVolume(size_t rec_nr)
{
	return getValue(rec_nr, cat_volume_);
}

std::string Catalog::GetField(size_t rec_nr, FieldDescr* field)
{
	return getValue(rec_nr, field);
}

void Catalog::SetRdbName(size_t rec_nr, const std::string& rdb_name)
{
	setValue(rec_nr, cat_rdb_name_, rdb_name);
}

void Catalog::SetFileName(size_t rec_nr, const std::string& file_name)
{
	setValue(rec_nr, cat_file_name_, file_name);
}

void Catalog::SetArchive(size_t rec_nr, const std::string& archive)
{
	setValue(rec_nr, cat_archive_, archive);
}

void Catalog::SetPathName(size_t rec_nr, const std::string& path_name)
{
	setValue(rec_nr, cat_path_name_, path_name);
}

void Catalog::SetVolume(size_t rec_nr, const std::string& volume)
{
	setValue(rec_nr, cat_volume_, volume);
}

void Catalog::SetField(size_t rec_nr, FieldDescr* field, const std::string& value)
{
	setValue(rec_nr, field, value);
}

FieldDescr* Catalog::CatalogArchiveField()
{
	return cat_archive_;
}

FieldDescr* Catalog::CatalogPathNameField()
{
	return cat_path_name_;
}

FieldDescr* Catalog::CatalogVolumeField()
{
	return cat_volume_;
}

std::string Catalog::getValue(size_t rec_nr, FieldDescr* field)
{
	cat_file_->ReadRec(rec_nr, record_);
	std::string value = _StdS(field, record_);
	std::string result = TrailChar(value, ' ');
	return result;
}

void Catalog::setValue(size_t rec_nr, FieldDescr* field, const std::string& value)
{
	cat_file_->ReadRec(rec_nr, record_);
	S_(cat_file_, field, value, record_);
	cat_file_->WriteRec(rec_nr, record_);
}
