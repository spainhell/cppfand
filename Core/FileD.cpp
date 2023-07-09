#include "FileD.h"

#include "GlobalVariables.h"
#include "obaseww.h"
#include "olongstr.h"
#include "runfrml.h"
#include "../Core/access.h"
#include "../fandio/files.h"
#include "../fandio/XKey.h"
#include "../Logging/Logging.h"
#include "../fandio/locks.h"


FileD::FileD(FType f_type)
{
	this->FileType = f_type;
	switch (f_type) {
	case FType::FandFile: {
		this->FF = new FandFile(this);
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
		FF = new FandFile(*orig.FF, this);
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
void FileD::ReadRec(size_t rec_nr, void* record) const
{
	this->FF->ReadRec(rec_nr, record);
}

void FileD::WriteRec(size_t rec_nr, void* record) const
{
	this->FF->WriteRec(rec_nr, record);
}

BYTE* FileD::GetRecSpace()
{
	size_t length = FF->RecLen + 2;
	// 0. BYTE in front (.X00) -> Valid Record Flag (but it's calculated in RecLen for index file)
	// 1. BYTE in the end -> Work Flag
	// 2. BYTE in the end -> Update Flag
	BYTE* result = new BYTE[length];
	memset(result, '\0', length);
	return result;
}

void FileD::ClearRecSpace(void* record)
{
	if (FF->TF != nullptr) {
		if (HasTWorkFlag(record)) {
			for (FieldDescr* f : FldD) {
				if (((f->Flg & f_Stored) != 0) && (f->field_type == FieldType::TEXT)) {
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
			SetUpdHandle(FF->Handle);
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

bool FileD::loadB(FieldDescr* field_d, void* record)
{
	bool result;
	if ((field_d->Flg & f_Stored) != 0) {
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
	if ((field_d->Flg & f_Stored) != 0) {
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
	if ((field_d->Flg & f_Stored) != 0) {
		result = FF->loadS(this, field_d, record);
	}
	else {
		result = RunStdStr(this, field_d->Frml, record);
	}
	return result;
}

pstring FileD::loadOldS(FieldDescr* field_d, void* record)
{
	pstring s;
	if ((field_d->Flg & f_Stored) != 0) {
		s = FF->loadOldS(field_d, record);
	}
	else {
		s = RunShortStr(this, field_d->Frml, record);
	}
	return s;
}

LongStr* FileD::loadLongS(FieldDescr* field_d, void* record)
{
	return FF->loadLongS(field_d, record);
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

void FileD::saveS(FieldDescr* field_d, std::string s, void* record)
{
	FF->saveS(this, field_d, s, record);
}

void FileD::saveLongS(FieldDescr* field_d, LongStr* ls, void* record)
{
	FF->saveLongS(this, field_d, ls, record);
}

int FileD::saveT(FieldDescr* field_d, int pos, void* record)
{
	return FF->saveT(field_d, pos, record);
}

void FileD::Close()
{
	if (FF != nullptr) {
		FF->SaveFile();
	}
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

bool FileD::SearchKey(XString& XX, XKey* Key, int& NN, void* record)
{
	return FF->SearchKey(XX, Key, NN, record);
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

	// create index file
	if (newFile->FF->file_type == FileType::INDEX) {
		if (newFile->FF->XF != nullptr) {
			delete newFile->FF->XF;
			newFile->FF->XF = nullptr;
		}
		newFile->FF->XF = new FandXFile(newFile->FF);
		newFile->FF->XF->Handle = nullptr;
		newFile->FF->XF->NoCreate = true;
		/*else xfile name identical with orig file*/
	}

	// create text file
	if (createTextFile && (newFile->FF->TF != nullptr)) {
		newFile->FF->TF = new FandTFile(newFile->FF);
		*newFile->FF->TF = *FF->TF;
		std::string path_t = SetTempCExt(this, 'T', net);
		newFile->FF->TF->Handle = OpenH(path_t, _isOverwriteFile, Exclusive);
		newFile->FF->TF->TestErr();
		newFile->FF->TF->CompileAll = true;
		newFile->FF->TF->SetEmpty();

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
		if (((F->Flg & f_Stored) != 0) && (F->field_type == FieldType::ALFANUM)) {
			saveS(F, "", record);
		}
	}
}

void FileD::CopyRecWithT(void* record1, void* record2, bool delTFields)
{
	if (delTFields)	{
		this->FF->DelTFlds(record2);
	}

	memcpy(record2, record1, FF->RecLen);
	for (auto& F : FldD) {
		if ((F->field_type == FieldType::TEXT) && ((F->Flg & f_Stored) != 0)) {
			FandTFile* tf1 = FF->TF;
			FandTFile* tf2 = tf1;
			if ((tf1->Format != FandTFile::T00Format)) {
				LongStr* s = loadLongS(F, record1);
				saveLongS(F, s, record2);
				delete s; s = nullptr;
			}
			else {
				if (HasTWorkFlag(record1)) {
					tf1 = &TWork;
				}
				int pos = loadT(F, record1);
				if (HasTWorkFlag(record2)) {
					tf2 = &TWork;
				}
				pos = CopyTFString(this, tf2, this, tf1, pos);
				saveT(F, pos, record2);
			}
		}
	}
}

//void FileD::DelTFlds(void* record)
//{
//	this->FF->DelTFlds(record);
//}

void FileD::DelAllDifTFlds(void* record, void* comp_record)
{
	this->FF->DelAllDifTFlds(record, comp_record);
}

bool FileD::IsActiveRdb()
{
	RdbD* R = CRdb;
	while (R != nullptr) {
		if (this == R->FD) return true;
		R = R->ChainBack;
	}
	return false;
}
