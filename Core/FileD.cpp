#include "FileD.h"

#include "GlobalVariables.h"
#include "obaseww.h"
#include "runfrml.h"
#include "../fandio/files.h"
#include "../fandio/XKey.h"
#include "../Logging/Logging.h"


FileD::FileD(FType f_type)
{
	this->FileType = f_type;
	switch (f_type) {
	case FType::FandFile: {
		this->FF = new Fand0File(this);
		break;
	}
	default:;
	}
}

FileD::FileD(const FileD& orig)
{
	Name = orig.Name;
	FileType = orig.FileType;
	ChptPos = orig.ChptPos;
	IsParFile = orig.IsParFile;
	IsJournal = orig.IsJournal;
	IsHlpFile = orig.IsHlpFile;
	typSQLFile = orig.typSQLFile;
	IsSQLFile = orig.IsSQLFile;
	IsDynFile = orig.IsDynFile;

	for (FieldDescr* f : orig.FldD) {
		FieldDescr* new_field_d = new FieldDescr(*f);
		FldD.push_back(new_field_d);
	}

	if (orig.FF != nullptr) {
		FF = new Fand0File(*orig.FF, this);
	}

	if (!orig.Keys.empty()) {
		for (auto& k : orig.Keys) {
			XKey* newKey = new XKey(*k);
			Keys.push_back(newKey);
		}
	}

	Add = orig.Add;
}

FileD::~FileD()
{
	delete FF;

	for (FieldDescr* field_d : FldD) {
		delete field_d;
	}
}

WORD FileD::GetNrKeys()
{
	return static_cast<WORD>(Keys.size());
}

void FileD::Reset()
{
	Name = "";
	FullPath = "";
	IRec = 0;
	ChptPos.i_rec = 0;
	ChptPos.rdb = 0;
	TxtPosUDLI = 0;    // =0 if not present; urcuje zacatek odstavcu #U #D #L #I
	OrigFD = nullptr;    // like orig. or nil
	CatIRec = 0;
	FldD.clear();
	IsParFile = false; IsJournal = false; IsHlpFile = false;
	typSQLFile = false; IsSQLFile = false; IsDynFile = false;
	ViewNames.clear();  //after each string BYTE string with user codes 
	Keys.clear();
	Add.clear();
}

/// <summary>
/// Vycte zaznam z datoveho souboru (.000)
/// </summary>
/// <param name="rec_nr">kolikaty zaznam (1 .. N)</param>
/// <param name="record">ukazatel na buffer</param>
size_t FileD::ReadRec(size_t rec_nr, void* record) const
{
	return this->FF->ReadRec(rec_nr, record);
}

size_t FileD::WriteRec(size_t rec_nr, void* record) const
{
	return this->FF->WriteRec(rec_nr, record);
}

uint8_t* FileD::GetRecSpace() const
{
	size_t length = FF->RecLen + 2;
	// 0. BYTE in front (.X00) -> Valid Record Flag (but it's calculated in RecLen for index file)
	// 1. BYTE in the end -> Work Flag
	// 2. BYTE in the end -> Update Flag
	uint8_t* result = new uint8_t[length];
	memset(result, '\0', length);
	return result;
}

std::unique_ptr<uint8_t[]> FileD::GetRecSpaceUnique() const
{
	size_t length = FF->RecLen + 2;
	// 0. BYTE in front (.X00) -> Valid Record Flag (but it's calculated in RecLen for index file)
	// 1. BYTE in the end -> Work Flag
	// 2. BYTE in the end -> Update Flag
	std::unique_ptr<uint8_t[]> result(new uint8_t[length]);
	memset(result.get(), '\0', length);
	return result;
}

/// <summary>
/// Deletes all text fields in the record from the TWork file
/// </summary>
/// <param name="record">data record pointer</param>
void FileD::ClearRecSpace(void* record)
{
	if (FF->TF != nullptr) {
		if (HasTWorkFlag(record)) {
			for (FieldDescr* f : FldD) {
				if ((f->isStored()) && (f->field_type == FieldType::TEXT)) {
					TWork.Delete(loadT(f, record));
					saveT(f, 0, record);
				}
			}
		}
	}
}

void FileD::IncNRecs(int n)
{
	this->FF->IncNRecs(n);
}

void FileD::DecNRecs(int n)
{
	this->FF->DecNRecs(n);
}

void FileD::SeekRec(int n)
{
	IRec = n;
	if (FF->XF == nullptr) {
		FF->Eof = (n >= FF->NRecs);
	}
	else {
		FF->Eof = (n >= FF->XF->NRecs);
	}
}

void FileD::CreateRec(int n, void* record) const
{
	this->FF->CreateRec(n, record);
}

void FileD::PutRec(void* record)
{
	this->FF->PutRec(record, IRec);
}

void FileD::DeleteRec(int n, void* record) const
{
	this->FF->DeleteRec(n, record);
}

void FileD::RecallRec(int recNr, void* record)
{
	FF->TestXFExist();
	FF->XF->NRecs++;
	for (auto& K : Keys) {
		K->Insert(this, recNr, false, record);
	}
	FF->ClearDeletedFlag(record);
	WriteRec(recNr, record);
}

void FileD::AssignNRecs(bool Add, int N)
{
	int OldNRecs; LockMode md;
#ifdef FandSQL
	if (IsSQLFile) {
		if ((N = 0) && !Add) Strm1->DeleteXRec(nullptr, nullptr, false); return;
	}
#endif
	md = NewLockMode(DelMode);
	OldNRecs = FF->NRecs;

	if (Add) {
		N = N + OldNRecs;
	}

	if ((N < 0) || (N == OldNRecs)) {
		OldLockMode(md);
		return;
	}

	if ((N == 0) && (FF->TF != nullptr)) {
		FF->TF->SetEmpty();
	}

	if (FF->file_type == FileType::INDEX) {
		if (N == 0) {
			FF->NRecs = 0;
			FF->SetUpdateFlag(); //SetUpdHandle(FF->Handle);
			int result = FF->XFNotValid();
			if (result != 0) {
				RunError(result);
			}
			OldLockMode(md);
			return;
		}
		else {
			SetMsgPar(Name);
			RunErrorM(md);
			RunError(821);
		}
	}

	if (N < OldNRecs) {
		DecNRecs(OldNRecs - N);
		OldLockMode(md);
		return;
	}

	BYTE* record = GetRecSpace();
	ZeroAllFlds(record, false);
	SetDeletedFlag(record);
	IncNRecs(N - OldNRecs);

	for (int i = OldNRecs + 1; i <= N; i++) {
		WriteRec(i, record);
	}

	delete[] record; record = nullptr;

	OldLockMode(md);
}

void FileD::SortByKey(std::vector<KeyFldD*>& keys) const
{
	if (FF != nullptr) {
		FF->SortAndSubst(keys);
	}
}

void FileD::IndexesMaintenance(bool remove_deleted)
{
	if (FF != nullptr) {
		FF->IndexFileProc(remove_deleted);
	}
}

bool FileD::loadB(FieldDescr* field_d, void* record)
{
	bool result;
	if (field_d->isStored()) {
		result = FF->loadB(field_d, record);
	}
	else
	{
		result = RunBool(this, field_d->Frml, record);
	}
	return result;
}

double FileD::loadR(FieldDescr* field_d, void* record)
{
	double result;
	if (field_d->isStored()) {
		result = FF->loadR(field_d, record);
	}
	else {
		result = RunReal(this, field_d->Frml, record);
	}
	return result;
}

std::string FileD::loadS(FieldDescr* field_d, void* record)
{
	std::string result;
	if (field_d->isStored()) {
		result = FF->loadS(field_d, record);
	}
	else {
		result = RunString(this, field_d->Frml, record);
	}
	return result;
}

LongStr* FileD::loadLongS(FieldDescr* field_d, void* record)
{
	std::string s = loadS(field_d, record);

	LongStr* result = new LongStr(s.length());
	result->LL = s.length();
	memcpy(result->A, s.c_str(), s.length());

	return result;
}

int FileD::loadT(FieldDescr* field_d, void* record)
{
	return FF->loadT(field_d, record);
}

void FileD::saveB(FieldDescr* field_d, bool b, void* record)
{
	FF->saveB(field_d, b, record);
}

void FileD::saveR(FieldDescr* field_d, double r, void* record)
{
	FF->saveR(field_d, r, record);
}

void FileD::saveS(FieldDescr* field_d, const std::string& s, void* record)
{
	FF->saveS(this, field_d, s, record);
}

void FileD::saveLongS(FieldDescr* field_d, LongStr* ls, void* record)
{
	const std::string s(ls->A, ls->LL);
	FF->saveS(this, field_d, s, record);
}

int FileD::saveT(FieldDescr* field_d, int pos, void* record)
{
	return FF->saveT(field_d, pos, record);
}

void FileD::SetUpdateFlag()
{
	FF->SetUpdateFlag();
}

void FileD::CloseFile()
{
	if (FF->Handle == nullptr) return;

	FF->CloseFile();

}

void FileD::Save()
{
	if (FF != nullptr) {
		FF->SaveFile();
	}
}

void FileD::OldLockMode(LockMode mode)
{
	OldLMode(this, CPath, mode, LANNode);
}

LockMode FileD::NewLockMode(LockMode mode)
{
	return NewLMode(this, CPath, mode, LANNode);
}

bool FileD::TryLockMode(LockMode mode, LockMode& old_mode, WORD kind)
{
	return TryLMode(this, CPath, mode, old_mode, kind, LANNode);
}

bool FileD::ChangeLockMode(LockMode mode, WORD kind, bool rd_pref)
{
	return ChangeLMode(this, CPath, mode, kind, rd_pref, LANNode);
}

bool FileD::Lock(int n, WORD kind)
{
	return TryLockN(this->FF, CPath, n, kind);
}

void FileD::Unlock(int n)
{
	UnLockN(this->FF, n);
}

void FileD::RunErrorM(LockMode mode)
{
	OldLockMode(mode);
}

void FileD::SetTWorkFlag(void* record)
{
	FF->SetTWorkFlag(record);
}

bool FileD::HasTWorkFlag(void* record)
{
	return FF->HasTWorkFlag(record);
}

void FileD::SetRecordUpdateFlag(void* record)
{
	FF->SetRecordUpdateFlag(record);
}

void FileD::ClearRecordUpdateFlag(void* record)
{
	FF->ClearRecordUpdateFlag(record);
}

bool FileD::HasRecordUpdateFlag(void* record)
{
	return FF->HasRecordUpdateFlag(record);
}

bool FileD::DeletedFlag(void* record)
{
	return FF->DeletedFlag(record);
}

void FileD::ClearDeletedFlag(void* record)
{
	FF->ClearDeletedFlag(record);
}

void FileD::SetDeletedFlag(void* record)
{
	FF->SetDeletedFlag(record);
}

bool FileD::SearchKey(XString& XX, XKey* Key, int& NN, void* record)
{
	return FF->SearchKey(XX, Key, NN, record);
}

bool FileD::SearchXKey(XKey* K, XString& X, int& N)
{
	if (FF->file_type == FileType::INDEX) {
		FF->TestXFExist();
		return K->SearchInterval(this, X, false, N);
	}
	else {
		std::unique_ptr<uint8_t[]> record = GetRecSpaceUnique();
		return SearchKey(X, K, N, record.get());
	}
}

FileD* FileD::OpenDuplicateF(bool createTextFile)
{
	short Len = 0;
	SetPathAndVolume(this);
	bool net = IsNetCVol();
	FileD* newFile = new FileD(*this);

	std::string path = SetTempCExt(this, '0', net);
	CVol = "";
	newFile->FullPath = path;
	newFile->FF->Handle = OpenH(path, _isOverwriteFile, Exclusive);
	TestCFileError(newFile);
	newFile->FF->NRecs = 0;
	newFile->IRec = 0;
	newFile->FF->Eof = true;
	newFile->FF->UMode = Exclusive;
	newFile->FF->SetUpdateFlag();

	// create index file
	if (newFile->FF->file_type == FileType::INDEX) {
		if (newFile->FF->XF != nullptr) {
			delete newFile->FF->XF;
			newFile->FF->XF = nullptr;
		}
		newFile->FF->XF = new FandXFile(newFile->FF);
		newFile->FF->XF->Handle = nullptr;
		newFile->FF->XF->NoCreate = true;
		newFile->FF->XF->SetUpdateFlag();
		/*else xfile name identical with orig file*/
	}

	// create text file
	if (createTextFile && (newFile->FF->TF != nullptr)) {
		newFile->FF->TF = new FandTFile(newFile->FF);
		//*newFile->FF->TF = *FF->TF;
		std::string path_t = SetTempCExt(this, 'T', net);
		//newFile->FF->TF->Handle = OpenH(path_t, _isOverwriteFile, Exclusive);
		newFile->FF->TF->Create(path_t);
		newFile->FF->TF->TestErr();
		newFile->FF->TF->CompileAll = true;
		newFile->FF->TF->SetEmpty();
		newFile->FF->TF->SetUpdateFlag();
	}
	return newFile;
}

void FileD::DeleteDuplicateF(FileD* TempFD)
{
	CloseClearH(&TempFD->FF->Handle);
	SetPathAndVolume(this);
	SetTempCExt(this, '0', FF->IsShared());
	MyDeleteFile(CPath);
}

void FileD::ZeroAllFlds(void* record, bool delTFields)
{
	if (delTFields) {
		this->FF->DelTFlds(record);
	}

	memset(record, 0, FF->RecLen);
	for (FieldDescr* F : FldD) {
		if (F->isStored() && (F->field_type == FieldType::ALFANUM)) {
			saveS(F, "", record);
		}
	}
}

/// \brief Copy complete record
/// - for T: one of this files is always TWork
/// \param src_record source record
/// \param dst_record destination record
/// \param delTFields deletes the existing destination T first
void FileD::CopyRec(uint8_t* src_record, uint8_t* dst_record, bool delTFields)
{
	if (delTFields) {
		this->FF->DelTFlds(dst_record);
	}

	//memcpy(dst_record, src_record, FF->RecLen);
	for (FieldDescr* const& f : FldD) {
		if (f->isStored()) {
			switch (f->field_type) {
			case FieldType::TEXT: {
				FandTFile* src_t00_file = FF->TF;
				FandTFile* dst_t00_file = FF->TF;

				if (src_t00_file->Format != FandTFile::T00Format) {
					std::string s = loadS(f, src_record);
					saveS(f, s, dst_record);
				}
				else {
					bool src_is_work = HasTWorkFlag(src_record);
					bool dst_is_work = HasTWorkFlag(dst_record);


					if (src_is_work) {
						src_t00_file = &TWork;
					}

					if (dst_is_work) {
						dst_t00_file = &TWork;
					}

					int src_pos = loadT(f, src_record);
					int dst_pos = 0;

					if (src_is_work && dst_is_work) {
						// src and dest are in TWork
						// don't need to lock anything
						std::string s = src_t00_file->Read(src_pos);
						dst_pos = dst_t00_file->Store(s);
					}
					else if (src_is_work) {
						// src is in TWork
						// lock dest for Write
						LockMode md = NewLockMode(WrMode);
						std::string s = src_t00_file->Read(src_pos);
						dst_pos = dst_t00_file->Store(s);
						OldLockMode(md);
					}
					else if (dst_is_work) {
						// dest is in TWork
						// lock src for Read
						LockMode md = NewLockMode(RdMode);
						std::string s = src_t00_file->Read(src_pos);
						dst_pos = dst_t00_file->Store(s);
						OldLockMode(md);
					}
					else {
						// src and dest are single T00 file
						// lock for Write
						LockMode md = NewLockMode(WrMode);
						std::string s = src_t00_file->Read(src_pos);
						dst_pos = dst_t00_file->Store(s);
						OldLockMode(md);
					}

					saveT(f, dst_pos, dst_record);
				}
				break;
			}
			default: 
				memcpy(&dst_record[f->Displ], &src_record[f->Displ], f->NBytes);
				break;
			}
		}
	}
}

void FileD::DelAllDifTFlds(void* record, void* comp_record)
{
	this->FF->DelAllDifTFlds(record, comp_record);
}

bool FileD::IsActiveRdb()
{
	RdbD* R = CRdb;
	while (R != nullptr) {
		if (this == R->v_files[0]) return true;
		R = R->ChainBack;
	}
	return false;
}

void FileD::CloseAllAfter(FileD* first_for_close, std::vector<FileD*>& v_files)
{
	// find first_for_close in v_files
	auto it0 = std::ranges::find(v_files, first_for_close);

	while (it0 != v_files.end()) {
		(*it0)->CloseFile();
		++it0;
	}
}

void FileD::CloseAndRemoveAllAfter(FileD* first_for_remove, std::vector<FileD*>& v_files)
{
	// find first_for_close in v_files
	auto it0 = std::ranges::find(v_files, first_for_remove);

	while (it0 != v_files.end()) {
		(*it0)->CloseFile();
		it0 = v_files.erase(it0);
	}
}

void FileD::CloseAndRemoveAllAfter(size_t first_index_for_remove, std::vector<FileD*>& v_files)
{
	if (first_index_for_remove >= v_files.size()) return;

	for (size_t i = first_index_for_remove; i < v_files.size(); i++) {
		v_files[i]->CloseFile();
		delete v_files[i];
		v_files[i] = nullptr;
	}
	v_files.erase(v_files.begin() + static_cast<int>(first_index_for_remove), v_files.end());
}
