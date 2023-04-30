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

	if (CatFD == nullptr || CatFD->GetCatalogFile()->FF->Handle == nullptr) {
		return result;
	}
	if (CRdb == nullptr) {
		return result;
	}

	RdbD* R = CRdb;

	while (true) {
		for (int i = 1; i <= CatFD->GetCatalogFile()->FF->NRecs; i++) {
			if (EquUpCase(CatFD->GetRdbName(i), R->FD->Name) && EquUpCase(CatFD->GetFileName(i), name)) {
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

void Catalog::GetPathAndVolume(FileD* file_d, int i, std::string& path, std::string& volume)
{
	std::string d;
	bool isRdb;

	std::string dir;
	std::string name;
	std::string ext;

	volume = CatFD->GetVolume(i);
	path = CatFD->GetPathName(i);
	const bool setContentDir = SetContextDir(file_d, d, isRdb);
	if (setContentDir && path.length() > 1 && path[1] != ':') {
		if (isRdb) {
			FSplit(path, dir, name, ext);
			AddBackSlash(d);
			dir = d;
			path = dir + name + ext;
			return;
		}
		if (path[0] == '\\') {
			path = d.substr(0, 2) + path;
		}
		else {
			AddBackSlash(d);
			path = d + path;
		}
	}
	else {
		path = FExpand(path);
	}
	FSplit(path, dir, name, ext);
}

WORD Catalog::Generation(FileD* file_d)
{
	WORD i, j;
	pstring s(2);
	if (file_d->CatIRec == 0) return 0;

	CVol = CatFD->GetVolume(file_d->CatIRec);
	CPath = FExpand(CatFD->GetPathName(file_d->CatIRec));
	FSplit(CPath, CDir, CName, CExt);

	s = CExt.substr(2, 2);
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
		CloseFile(file_d);
	}
	file_d = CatFD->GetCatalogFile();
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
