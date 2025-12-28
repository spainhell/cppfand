#include "Catalog.h"
#include "access.h"
#include "GlobalVariables.h"
#include "oaccess.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"
#include "../Common/Record.h"

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
	record_ = new Record(cat_file_);

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
	delete record_;
}

void Catalog::Close()
{
	cat_file_->CloseFile();
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

bool Catalog::OldToNewCat(int& file_size)
{
	struct stX { int32_t NRecs; uint16_t RecLen; } x;
	uint8_t buf[512];

	if (cat_file_->FF->file_type != FandFileType::CAT) return false;

	cat_file_->FF->ReadData(0, 4, &x.NRecs);
	cat_file_->FF->ReadData(4, 2, &x.RecLen);

	if (x.RecLen != 106) return false;

	x.RecLen = 107;
	cat_file_->FF->WriteData(0, 4, &x.NRecs);
	cat_file_->FF->WriteData(4, 2, &x.RecLen);

	for (int32_t i = x.NRecs; i >= 1; i--) {
		int32_t off = 6 + (i - 1) * 106;
		int32_t offNew = off + (i - 1);
		cat_file_->FF->ReadData(off + 16, 90, buf);
		cat_file_->FF->WriteData(offNew + 17, 90, buf);
		buf[17] = 0;
		cat_file_->FF->ReadData(off, 16, buf);
		cat_file_->FF->WriteData(offNew, 17, buf);
	}

	cat_file_->FF->NRecs = x.NRecs;
	file_size = x.NRecs * 107 + 6;

	return true;
}

int Catalog::GetCatalogIRec(const std::string& name, bool multilevel)
{
	int result = 0;

	if (catalog == nullptr || catalog->GetCatalogFile()->FF->Handle == nullptr) {
		return result;
	}
	if (CRdb == nullptr) {
		return result;
	}

	Project* R = CRdb;

	while (true) {
		for (int i = 1; i <= catalog->GetCatalogFile()->FF->NRecs; i++) {
			if (EquUpCase(catalog->GetRdbName(i), R->project_file->Name) && EquUpCase(catalog->GetFileName(i), name)) {
				result = i;
				return result;
			}
		}
		R = R->ChainBack;
		if ((R != nullptr) && multilevel) {
			continue;
		}
		break;
	}

	return result;
}

void Catalog::GetPathAndVolume(FileD* file_d, int rec_nr, std::string& path, std::string& volume) const
{
	//if (file_d->Name == "PARAM3")
	//{
	//	printf("");
	//}

	bool isRdb;

	std::string dir;
	std::string name;
	std::string ext;
	std::string content_dir;

	volume = catalog->GetVolume(rec_nr);
	path = catalog->GetPathName(rec_nr);
	const bool setContentDir = SetContextDir(file_d, content_dir, isRdb);
	if (setContentDir && path.length() > 1 && path[1] != ':') {
		if (isRdb) {
			FSplit(path, dir, name, ext);
			AddBackSlash(content_dir);
			dir = content_dir;
			path = dir + name + ext;
			return;
		}
		if (path[0] == '\\') {
			path = content_dir.substr(0, 2) + path;
		}
		else {
			AddBackSlash(content_dir);
			path = content_dir + path;
		}
	}
	else {
		path = FExpand(path);
	}
}

uint16_t Catalog::Generation(FileD* file_d, std::string& path, std::string& volume)
{
	if (file_d->CatIRec == 0) return 0;

	std::string dir;
	std::string name;
	std::string ext;

	volume = catalog->GetVolume(file_d->CatIRec);
	path = FExpand(catalog->GetPathName(file_d->CatIRec));
	FSplit(path, dir, name, ext);

	WORD i, j;
	pstring s(2);
	s = ext.substr(2, 2);
	val(s, i, j);

	if (j == 0) {
		return i;
	}
	else {
		return 0;
	}
}

void Catalog::TurnCat(FileD* file_d, uint16_t Frst, uint16_t N, short I)
{
	if (file_d != nullptr) {
		file_d->CloseFile();
	}
	file_d = catalog->GetCatalogFile();

	Record* p = new Record(file_d);
	Record* q = new Record(file_d);
	uint16_t last = Frst + N - 1;
	
	if (I > 0)
		while (I > 0) {
			file_d->ReadRec(Frst, q);
			for (uint16_t j = 1; j <= N - 1; j++) {
				file_d->ReadRec(Frst + j, p);
				file_d->WriteRec(Frst + j - 1, p);
			}
			file_d->WriteRec(last, q);
			I--;
		}
	else
		while (I < 0) {
			file_d->ReadRec(last, q);
			for (uint16_t j = 1; j <= N - 1; j++) {
				file_d->ReadRec(last - j, p);
				file_d->WriteRec(last - j + 1, p);
			}
			file_d->WriteRec(Frst, q);
			I++;
		}
	delete p; p = nullptr;
	delete q; q = nullptr;
}

std::string Catalog::getValue(size_t rec_nr, FieldDescr* field)
{
	cat_file_->ReadRec(rec_nr, record_);
	//std::string value = cat_file_->loadS(field, record_);
	std::string value = record_->LoadS(field);
	std::string result = TrailChar(value, ' ');
	return result;
}

void Catalog::setValue(size_t rec_nr, FieldDescr* field, const std::string& value)
{
	cat_file_->ReadRec(rec_nr, record_);
	//cat_file_->saveS(field, value, record_);
	record_->SaveS(field, value);
	cat_file_->WriteRec(rec_nr, record_);
}
