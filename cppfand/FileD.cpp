#include "FileD.h"
#include "../cppfand/access.h"
#include "../fandio/XKey.h"
#include "../Logging/Logging.h"
#include "../fandio/locks.h"


FileD::FileD(FType f_type)
{
	this->FileType = f_type;
	switch (f_type) {
	case FType::FandFile: {
		this->FF = new FandFile();
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
	FldD = orig.FldD;
	IsParFile = orig.IsParFile;
	IsJournal = orig.IsJournal;
	IsHlpFile = orig.IsHlpFile;
	typSQLFile = orig.typSQLFile;
	IsSQLFile = orig.IsSQLFile;
	IsDynFile = orig.IsDynFile;

	if (orig.FF != nullptr) {
		FF = new FandFile(*orig.FF);
	}

	if (!orig.Keys.empty()) {
		for (auto& k : orig.Keys) {
			XKey* newKey = new XKey(*k);
			Keys.push_back(newKey);
		}
	}

	Add = orig.Add;
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
	ChptPos.IRec = 0;
	ChptPos.R = 0;
	TxtPosUDLI = 0;    // =0 if not present; urcuje zacatek odstavcu #U #D #L #I
	OrigFD = nullptr;    // like orig. or nil
	CatIRec = 0;
	FldD.clear();
	IsParFile = false; IsJournal = false; IsHlpFile = false;
	typSQLFile = false; IsSQLFile = false; IsDynFile = false;
	ViewNames = nullptr;  //after each string BYTE string with user codes 
	Keys.clear();
	Add.clear();
}

/// <summary>
/// Vycte zaznam z datoveho souboru (.000)
/// </summary>
/// <param name="rec_nr">kolikaty zaznam (1 .. N)</param>
/// <param name="record">ukazatel na buffer</param>
void FileD::ReadRec(size_t rec_nr, void* record)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "ReadRec(), file 0x%p, RecNr %i", file, N);
	RdWrCache(READ, this->FF->Handle, this->FF->NotCached(),
		(rec_nr - 1) * this->FF->RecLen + this->FF->FrstDispl, this->FF->RecLen, record);
}

void FileD::WriteRec(size_t rec_nr, void* record)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "WriteRec(%i), CFile 0x%p", N, file->Handle);
	RdWrCache(WRITE, this->FF->Handle, this->FF->NotCached(),
		(rec_nr - 1) * this->FF->RecLen + this->FF->FrstDispl, this->FF->RecLen, record);
	this->FF->WasWrRec = true;
}

BYTE* FileD::GetRecSpace()
{
	size_t length = FF->RecLen + 2;
	BYTE* result = new BYTE[length];
	memset(result, '\0', length);
	return result;
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

void FileD::CreateRec(int n, void* record)
{
	IncNRecs(1);
	void* tmp = GetRecSpace();
	for (int i = FF->NRecs - 1; i >= n; i--) {
		ReadRec(i, tmp);
		WriteRec(i + 1, tmp);
	}
	delete[] tmp;
	tmp = nullptr;
	WriteRec(n, record);
}

void FileD::PutRec(void* record)
{
	this->FF->PutRec(record, IRec);
}

void FileD::DeleteRec(int n, void* record)
{
	DelAllDifTFlds(record, nullptr);
	for (int i = n; i <= FF->NRecs - 1; i++) {
		ReadRec(i + 1, record);
		WriteRec(i, record);
	}
	DecNRecs(1);
}

void FileD::DelAllDifTFlds(void* Rec, void* CompRec)
{
	for (auto& F : FldD) {
		if (F->field_type == FieldType::TEXT && ((F->Flg & f_Stored) != 0)) DelDifTFld(Rec, CompRec, F);
	}
}

void FileD::RecallRec(int recNr, void* record)
{
	TestXFExist();
	FF->XF->NRecs++;
	for (auto& K : Keys) {
		K->Insert(recNr, false);
	}
	FF->ClearDeletedFlag(record);
	WriteRec(recNr, record);
}

int FileD::_T(FieldDescr* F, void* record)
{
	return FF->_T(F, record);
}

void FileD::B_(FieldDescr* F, bool B, void* record)
{
	FF->B_(F, B, record);
}

int FileD::T_(FieldDescr* field_d, int pos, void* record)
{
	return FF->T_(field_d, pos, record);
}

void FileD::Close()
{
	if (FF != nullptr) {
		FF->CloseFile();
	}
}

void FileD::Save()
{
	FF->SaveFile();
}

void FileD::OldLockMode(LockMode mode)
{
	OldLMode(this, mode);
}

LockMode FileD::NewLockMode(LockMode mode)
{
	return NewLMode(this, mode);
}

bool FileD::TryLockMode(LockMode mode, LockMode& old_mode, WORD kind)
{
	return TryLMode(this, mode, old_mode, kind);
}

bool FileD::ChangeLockMode(LockMode mode, WORD kind, bool rd_pref)
{
	return ChangeLMode(this, mode, kind, rd_pref);
}

bool FileD::Lock(int n, WORD kind)
{
	return TryLockN(this->FF, n, kind);
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

void FileD::SetUpdFlag(void* record)
{
	FF->SetUpdFlag(record);
}

void FileD::ClearUpdFlag(void* record)
{
	FF->ClearUpdFlag(record);
}

bool FileD::HasUpdFlag(void* record)
{
	return FF->HasUpdFlag(record);
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
