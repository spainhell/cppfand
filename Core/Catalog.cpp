#include "Catalog.h"
#include "access.h"
#include "GlobalVariables.h"
#include "oaccess.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"

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

bool Catalog::OldToNewCat(int& FilSz)
{
	struct stX { int NRecs; WORD  RecLen; } x;
	int off, offNew;
	BYTE a[91]; // budeme cislovat od 1, jako v Pascalu (a:array[1..90] of byte;)

	auto result = false;
	bool cached = cat_file_->FF->NotCached();
	if (cat_file_->FF->file_type != FileType::CAT) return result;
	RdWrCache(READ, cat_file_->FF->Handle, cached, 0, 6, &x);
	if (x.RecLen != 106) return result;
	x.RecLen = 107;
	RdWrCache(WRITE, cat_file_->FF->Handle, cached, 0, 6, &x);
	for (int i = x.NRecs; i >= 1; i--) {
		off = 6 + (i - 1) * 106;
		offNew = off + (i - 1);
		RdWrCache(READ, cat_file_->FF->Handle, cached, off + 16, 90, a);
		RdWrCache(WRITE, cat_file_->FF->Handle, cached, offNew + 17, 90, a);
		a[17] = 0;
		RdWrCache(READ, cat_file_->FF->Handle, cached, off, 16, a);
		RdWrCache(WRITE, cat_file_->FF->Handle, cached, offNew, 17, a);
	}
	cat_file_->FF->NRecs = x.NRecs;
	FilSz = x.NRecs * 107 + 6;
	result = true;
	return result;
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

	RdbD* R = CRdb;

	while (true) {
		for (int i = 1; i <= catalog->GetCatalogFile()->FF->NRecs; i++) {
			if (EquUpCase(catalog->GetRdbName(i), R->rdb_file->Name) && EquUpCase(catalog->GetFileName(i), name)) {
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

WORD Catalog::Generation(FileD* file_d, std::string& path, std::string& volume)
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

void Catalog::TurnCat(FileD* file_d, WORD Frst, WORD N, short I)
{
	if (file_d != nullptr) {
		file_d->CloseFile();
	}
	file_d = catalog->GetCatalogFile();
	BYTE* p = file_d->GetRecSpace();
	BYTE* q = file_d->GetRecSpace();
	WORD last = Frst + N - 1;
	if (I > 0)
		while (I > 0) {
			file_d->ReadRec(Frst, q);
			for (WORD j = 1; j <= N - 1; j++) {
				file_d->ReadRec(Frst + j, p);
				file_d->WriteRec(Frst + j - 1, p);
			}
			file_d->WriteRec(last, q);
			I--;
		}
	else
		while (I < 0) {
			file_d->ReadRec(last, q);
			for (WORD j = 1; j <= N - 1; j++) {
				file_d->ReadRec(last - j, p);
				file_d->WriteRec(last - j + 1, p);
			}
			file_d->WriteRec(Frst, q);
			I++;
		}
	delete[] p; p = nullptr;
	delete[] q; q = nullptr;
}

std::string Catalog::getValue(size_t rec_nr, FieldDescr* field)
{
	cat_file_->ReadRec(rec_nr, record_);
	std::string value = cat_file_->loadS(field, record_);
	std::string result = TrailChar(value, ' ');
	return result;
}

void Catalog::setValue(size_t rec_nr, FieldDescr* field, const std::string& value)
{
	cat_file_->ReadRec(rec_nr, record_);
	cat_file_->saveS(field, value, record_);
	cat_file_->WriteRec(rec_nr, record_);
}
