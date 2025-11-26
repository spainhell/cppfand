#include "FileD.h"

#include "Coding.h"
#include "../Core/GlobalVariables.h"
#include "../Core/oaccess.h"
#include "../Core/obaseww.h"
#include "../Core/runfrml.h"
#include "../fandio/XKey.h"
#include "../Common/compare.h"
#include "../Logging/Logging.h"
#include "../Common/textfunc.h"


FileD::FileD(DataFileType f_type)
{
	this->FileType = f_type;
	switch (f_type) {
	case DataFileType::FandFile: {
		this->FF = new Fand0File(this);
		break;
	}
	case DataFileType::DBF: {
		this->DbfF = new DbfFile(this);
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
	//delete DbfF;

	for (FieldDescr* field_d : FldD) {
		delete field_d;
	}
}

int FileD::GetNRecs()
{
	int result;

	switch (FileType) {
	case DataFileType::FandFile:
		result = FF->NRecs;
		break;
	case DataFileType::DBF:
		result = DbfF->NRecs;
		break;
	default:
		result = 0;
		break;
	}

	return result;
}

void FileD::SetNRecs(int recs)
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->NRecs = recs;
		break;
	case DataFileType::DBF:
		DbfF->NRecs = recs;
		break;
	default:;
	}
}

long FileD::GetFileSize()
{
	long result;

	switch (FileType) {
	case DataFileType::FandFile: {
		result = SizeF(FF->Handle, HandleError);
		break;
	}
	case DataFileType::DBF: {
		result = SizeF(DbfF->Handle, HandleError);
		break;
	}
	default: {
		result = -1;
		break;
	}
	}

	return result;
}

WORD FileD::GetNrKeys()
{
	return static_cast<WORD>(Keys.size());
}

unsigned short FileD::GetFirstRecPos()
{
	unsigned short result;
	switch (FileType) {
	case DataFileType::FandFile: {
		result = FF->FirstRecPos;
		break;
	}
	case DataFileType::DBF: {
		result = DbfF->FirstRecPos;
		break;
	}
	default: {
		result = 0;
		break;
	}
	}
	return result;
}

uint16_t FileD::GetRecLen()
{
	uint16_t result;
	switch (FileType) {
	case DataFileType::FandFile: {
		result = FF->RecLen;
		break;
	}
	case DataFileType::DBF: {
		result = DbfF->RecLen;
		break;
	}
	default: {
		result = 0;
		break;
	}
	}
	return result;
}

void FileD::SetRecLen(uint16_t length)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		FF->RecLen = length;
		break;
	}
	case DataFileType::DBF: {
		DbfF->RecLen = length;
		break;
	}
	default:;
	}
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
	ViewNames.clear();  //after each string uint8_t string with user codes 
	Keys.clear();
	Add.clear();
}

size_t FileD::GetRecordSize()
{
	size_t result;
	switch (FileType) {
	case DataFileType::FandFile: {
		result = FF->RecLen;
		break;
	}
	case DataFileType::DBF: {
		result = DbfF->RecLen;
		break;
	}
	default: {
		result = 0;
		break;
	}
	}
	return result;
}

/// <summary>
/// Vycte zaznam z datoveho souboru (.000)
/// </summary>
/// <param name="rec_nr">kolikaty zaznam (1 .. N)</param>
/// <param name="record">ukazatel na buffer</param>
size_t FileD::ReadRec(size_t rec_nr, uint8_t* record) const
{
	size_t result;

	switch (FileType) {
	case DataFileType::FandFile: {
		result = FF->ReadRec(rec_nr, record);
		break;
	}
	case DataFileType::DBF: {
		result = DbfF->ReadRec(rec_nr, record);
		break;
	}
	default: {
		result = 0;
		break;
	}
	}

	return result;
}

size_t FileD::WriteRec(size_t rec_nr, uint8_t* record) const
{
	size_t result;

	switch (FileType) {
	case DataFileType::FandFile: {
		result = FF->WriteRec(rec_nr, record);
		break;
	}
	case DataFileType::DBF: {
		result = DbfF->WriteRec(rec_nr, record);
		break;
	}
	default: {
		result = 0;
		break;
	}
	}

	return result;
}

int FileD::UsedFileSize() const
{
	int result = 0;

	switch (FileType) {
	case DataFileType::FandFile: {
		result = FF->UsedFileSize();
		break;
	}
	case DataFileType::DBF: {
		result = DbfF->UsedFileSize();
		break;
	}
	default: {
		break;
	}
	}

	return result;
}

bool FileD::GetWasRdOnly() const
{
	bool result;
	switch (FileType) {
	case DataFileType::FandFile: {
		result = FF->WasRdOnly;
		break;
	}
	case DataFileType::DBF: {
		result = DbfF->WasRdOnly;
		break;
	}
	default: {
		result = false;
		break;
	}
	}
	return result;
}

void FileD::SetWasRdOnly(bool was_read_only) const
{
	switch (FileType) {
	case DataFileType::FandFile: {
		FF->WasRdOnly = was_read_only;
		break;
	}
	case DataFileType::DBF: {
		DbfF->WasRdOnly = was_read_only;
		break;
	}
	default: {
		break;
	}
	}
}

void FileD::SetHandle(HANDLE handle)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		FF->Handle = handle;
		break;
	}
	case DataFileType::DBF: {
		DbfF->Handle = handle;
		break;
	}
	default: {
		break;
	}
	}
}

void FileD::SetHandleT(HANDLE handle)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		FF->TF->Handle = handle;
		break;
	}
	case DataFileType::DBF: {
		DbfF->TF->Handle = handle;
		break;
	}
	default: {
		break;
	}
	}
}

void FileD::CheckT(int file_size)
{
	if (HasTextFile()) {
		switch (FileType) {
		case DataFileType::FandFile: {
			if (file_size < GetFirstRecPos()) {
				FF->TF->SetEmpty();
			}
			else {
				FF->TF->RdPrefix(true);
				if ((FF->file_type == FandFileType::RDB)
					&& !IsActiveRdb()
					&& !Coding::HasPassword(this, 1, ""))
				{
					FileMsg(this, 616, ' ');
					Close();
					GoExit(MsgLine);
				}
			}
			break;
		}
		case DataFileType::DBF: {
			if (file_size < GetFirstRecPos()) {
				DbfF->TF->SetEmpty();
			}
			else {
				DbfF->TF->RdPrefix(true);
				FileMsg(this, 616, ' ');
				Close();
				GoExit(MsgLine);
			}
			break;
		}
		default: {
			// other types don't have a text file
			break;
		}
		}
	}
}

void FileD::CheckX(int file_size)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		if (HasIndexFile()) {
			if (file_size < FF->FirstRecPos) {
				FF->XF->SetNotValid(FF->NRecs, GetNrKeys());
			}
			else {
				WORD Signum = 0;
				FF->XF->ReadData(0, 2, &Signum);
				FF->XF->RdPrefix();

				if (
					!FF->XF->NotValid && ((Signum != 0x04FF)
						|| (FF->XF->NRecsAbs != FF->NRecs)
						|| (FF->XF->FreeRoot > FF->XF->MaxPage)
						|| ((FF->XF->MaxPage + 1) << XPageShft) > FileSizeH(FF->XF->Handle))
					|| (FF->XF->NrKeys != 0) && (FF->XF->NrKeys != GetNrKeys()))
				{

					if (!EquUpCase(GetEnv("FANDMSG830"), "NO")) {
						FileMsg(this, 830, 'X');
					}

					if (FF->IsShared() && (FF->LMode < ExclMode)) {
						ChangeLockMode(ExclMode, 0, false);
					}

					FF->LMode = ExclMode;
					FF->XF->SetNotValid(FF->NRecs, GetNrKeys());
				}
			}
		}
		else {
			// this data file doesn't have index file
		}
		break;
	}
	default: {
		// other types don't have an index file
		break;
	}
	}
}

uint8_t* FileD::GetRecSpace() const
{
	size_t length;
	// 0. uint8_t in front (.X00) -> Valid Record Flag (it's calculated in RecLen for index file)
	// 1. uint8_t in the end -> Work Flag
	// 2. uint8_t in the end -> Update Flag

	switch (FileType) {
	case DataFileType::FandFile:
		length = FF->RecLen + 2;
		break;
	case DataFileType::DBF:
		length = DbfF->RecLen + 2;
		break;
	default:
		length = 0;
		break;
	}

	uint8_t* result = new uint8_t[length];
	memset(result, '\0', length);
	return result;
}

std::unique_ptr<uint8_t[]> FileD::GetRecSpaceUnique() const
{
	size_t length;
	// 0. uint8_t in front (.X00) -> Valid Record Flag (it's calculated in RecLen for index file)
	// 1. uint8_t in the end -> Work Flag
	// 2. uint8_t in the end -> Update Flag

	switch (FileType) {
	case DataFileType::FandFile:
		length = FF->RecLen + 2;
		break;
	case DataFileType::DBF:
		length = DbfF->RecLen + 2;
		break;
	default:
		length = 0;
		break;
	}

	std::unique_ptr<uint8_t[]> result(new uint8_t[length]);
	memset(result.get(), '\0', length);
	return result;
}

/// <summary>
/// Deletes all text fields in the record from the TWork file
/// </summary>
/// <param name="record">data record pointer</param>
void FileD::ClearRecSpace(uint8_t* record)
{
	if (HasTextFile()) {
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

void FileD::CompileRecLen() const
{
	switch (FileType) {
	case DataFileType::FandFile: {
		FF->CompileRecLen();
		break;
	}
	case DataFileType::DBF: {
		DbfF->CompileRecLen();
		break;
	}
	default: break;
	}
}

void FileD::IncNRecs(int n)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		this->FF->IncNRecs(n);
		break;
	}
	case DataFileType::DBF: {
		DbfF->IncNRecs(n);
		break;
	}
	default: break;
	}
}

void FileD::DecNRecs(int n)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		FF->DecNRecs(n);
		break;
	}
	case DataFileType::DBF: {
		DbfF->DecNRecs(n);
		break;
	}
	default: break;
	}
}

void FileD::SeekRec(int n)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		IRec = n;
		if (FF->XF == nullptr) {
			FF->Eof = (n >= FF->NRecs);
		}
		else {
			FF->Eof = (n >= FF->XF->NRecs);
		}
		break;
	}
	case DataFileType::DBF: {
		IRec = n;
		DbfF->Eof = (n >= DbfF->NRecs);
		break;
	}
	default: break;
	}
}

void FileD::CreateRec(int n, uint8_t* record) const
{
	switch (FileType) {
	case DataFileType::FandFile: {
		FF->CreateRec(n, record);
		break;
	}
	case DataFileType::DBF: {
		DbfF->CreateRec(n, record);
		break;
	}
	default: break;
	}
}

void FileD::PutRec(uint8_t* record)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		FF->PutRec(record, IRec);
		break;
	}
	case DataFileType::DBF: {
		DbfF->PutRec(record, IRec);
		break;
	}
	default: break;
	}
}

void FileD::DeleteRec(int n, uint8_t* record) const
{
	switch (FileType) {
	case DataFileType::FandFile: {
		FF->DeleteRec(n, record);
		break;
	}
	case DataFileType::DBF: {
		DbfF->DeleteRec(n, record);
		break;
	}
	default: break;
	}
}

void FileD::RecallRec(int recNr, uint8_t* record)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		FF->TestXFExist();
		FF->XF->NRecs++;
		for (auto& K : Keys) {
			K->Insert(this, recNr, false, record);
		}
		FF->ClearDeletedFlag(record);
		WriteRec(recNr, record);
		break;
	}
	case DataFileType::DBF: {
		DbfF->ClearDeletedFlag(record);
		WriteRec(recNr, record);
		break;
	}
	default: break;
	}
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

	if (FF->file_type == FandFileType::INDEX) {
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

	uint8_t* record = GetRecSpace();
	ZeroAllFlds(record, false);
	SetDeletedFlag(record);
	IncNRecs(N - OldNRecs);

	for (int i = OldNRecs + 1; i <= N; i++) {
		WriteRec(i, record);
	}

	delete[] record; record = nullptr;

	OldLockMode(md);
}

void FileD::SortByKey(std::vector<KeyFldD*>& keys, void (*msgFuncOn)(int8_t, int32_t), void (*msgFuncUpdate)(int32_t), void (*msgFuncOff)()) const
{
	if (FF != nullptr) {
		FF->SortAndSubst(keys, msgFuncOn, msgFuncUpdate, msgFuncOff);
	}
}

void FileD::IndexesMaintenance(bool remove_deleted)
{
	if (FF != nullptr) {
		FF->IndexFileProc(remove_deleted);
	}
}

bool FileD::loadB(FieldDescr* field_d, uint8_t* record)
{
	bool result;

	if (field_d->isStored()) {
		switch (FileType) {
		case DataFileType::FandFile:
			result = FF->loadB(field_d, record);
			break;
		case DataFileType::DBF:
			result = DbfF->loadB(field_d, record);
			break;
		default:
			result = false;
			break;
		}
	}
	else {
		result = RunBool(this, field_d->Frml, record);
	}

	return result;
}

double FileD::loadR(FieldDescr* field_d, uint8_t* record)
{
	double result;

	if (field_d->isStored()) {
		switch (FileType) {
		case DataFileType::FandFile:
			result = FF->loadR(field_d, record);
			break;
		case DataFileType::DBF:
			result = DbfF->loadR(field_d, record);
			break;
		default:
			result = 0.0;
			break;
		}
	}
	else {
		result = RunReal(this, field_d->Frml, record);
	}

	return result;
}

std::string FileD::loadS(FieldDescr* field_d, uint8_t* record)
{
	std::string result;

	if (field_d->isStored()) {
		switch (FileType) {
		case DataFileType::FandFile:
			result = FF->loadS(field_d, record);
			break;
		case DataFileType::DBF:
			result = DbfF->loadS(field_d, record);
			break;
		default:
			result = "";
			break;
		}
	}
	else {
		result = RunString(this, field_d->Frml, record);
	}

	return result;
}

int FileD::loadT(FieldDescr* field_d, uint8_t* record)
{
	int result;

	switch (FileType) {
	case DataFileType::FandFile:
		result = FF->loadT(field_d, record);
		break;
	case DataFileType::DBF:
		result = DbfF->loadT(field_d, record);
		break;
	default:
		result = 0;
		break;
	}

	return result;
}

void FileD::saveB(FieldDescr* field_d, bool b, uint8_t* record)
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->saveB(field_d, b, record);
		break;
	case DataFileType::DBF:
		DbfF->saveB(field_d, b, record);
		break;
	default:
		break;
	}
}

void FileD::saveR(FieldDescr* field_d, double r, uint8_t* record)
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->saveR(field_d, r, record);
		break;
	case DataFileType::DBF:
		DbfF->saveR(field_d, r, record);
		break;
	default:
		break;
	}
}

void FileD::saveS(FieldDescr* field_d, const std::string& s, uint8_t* record)
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->saveS(this, field_d, s, record);
		break;
	case DataFileType::DBF:
		DbfF->saveS(this, field_d, s, record);
		break;
	default:
		break;
	}
}

int FileD::saveT(FieldDescr* field_d, int pos, uint8_t* record) const
{
	int result;

	switch (FileType) {
	case DataFileType::FandFile:
		result = FF->saveT(field_d, pos, record);
		break;
	case DataFileType::DBF:
		result = DbfF->saveT(field_d, pos, record);
		break;
	default:
		result = 0;
		break;
	}

	return result;
}

void FileD::SetDrive(uint8_t drive) const
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->Drive = drive;
		break;
	case DataFileType::DBF:
		DbfF->Drive = drive;
		break;
	default:
		break;
	}
}

void FileD::SetUpdateFlag() const
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->SetUpdateFlag();
		break;
	case DataFileType::DBF:
		DbfF->SetUpdateFlag();
		break;
	default:
		break;
	}
}

void FileD::Close() const
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->Close();
		break;
	case DataFileType::DBF:
		DbfF->Close();
		break;
	default:
		break;
	}
}

void FileD::CloseFile() const
{
	switch (FileType) {
	case DataFileType::FandFile:
		if (FF->Handle != nullptr) FF->CloseFile();
		break;
	case DataFileType::DBF:
		if (DbfF->Handle != nullptr) DbfF->CloseFile();
		break;
	default:
		break;
	}
}

void FileD::Save() const
{
	switch (FileType) {
	case DataFileType::FandFile:
		if (FF != nullptr) FF->SaveFile();
		break;
	case DataFileType::DBF:
		if (DbfF != nullptr) DbfF->SaveFile();
		break;
	default:
		break;
	}
}

void FileD::CreateT(const std::string& path) const
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->TF->Create(path);
		break;
	case DataFileType::DBF:
		DbfF->TF->Create(path);
		break;
	default:
		break;
	}
}

FileUseMode FileD::GetUMode() const
{
	FileUseMode mode;

	switch (FileType) {
	case DataFileType::FandFile:
		mode = FF->UMode;
		break;
	case DataFileType::DBF:
		mode = DbfF->UMode;
		break;
	default:
		mode = Exclusive;
		break;
	}

	return mode;
}

LockMode FileD::GetLMode() const
{
	if (FileType == DataFileType::FandFile) return FF->LMode;
	else return NullMode;
}

LockMode FileD::GetExLMode() const
{
	if (FileType == DataFileType::FandFile) return FF->ExLMode;
	else return NullMode;
}

LockMode FileD::GetTaLMode() const
{
	if (FileType == DataFileType::FandFile) return FF->TaLMode;
	else return NullMode;
}

void FileD::SetUMode(FileUseMode mode) const
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->UMode = mode;
		break;
	case DataFileType::DBF:
		DbfF->UMode = mode;
		break;
	default:
		break;
	}
}

void FileD::SetLMode(LockMode mode) const
{
	if (FileType == DataFileType::FandFile) {
		FF->LMode = mode;
	}
	else {
		// locks are not supported in other file types
	}
}

void FileD::SetExLMode(LockMode mode) const
{
	if (FileType == DataFileType::FandFile) {
		FF->ExLMode = mode;
	}
	else {
		// locks are not supported in other file types
	}
}

void FileD::SetTaLMode(LockMode mode) const
{
	if (FileType == DataFileType::FandFile) {
		FF->TaLMode = mode;
	}
	else {
		// locks are not supported in other file types
	}
}

void FileD::OldLockMode(LockMode mode)
{
	if (FileType == DataFileType::FandFile) {
		OldLMode(this, CPath, mode, LANNode);
	}
	else {
		// locks are not supported in other file types
	}
}

LockMode FileD::NewLockMode(LockMode mode)
{
	if (FileType == DataFileType::FandFile) {
		return NewLMode(this, CPath, mode, LANNode);
	}
	else {
		return mode;
	}
}

bool FileD::TryLockMode(LockMode mode, LockMode& old_mode, WORD kind)
{
	if (FileType == DataFileType::FandFile) {
		return TryLMode(this, CPath, mode, old_mode, kind, LANNode);
	}
	else {
		return true;
	}
}

bool FileD::ChangeLockMode(LockMode mode, WORD kind, bool rd_pref)
{
	if (FileType == DataFileType::FandFile) {
		return ChangeLMode(this, CPath, mode, kind, rd_pref, LANNode);
	}
	else {
		return true;
	}
}

bool FileD::Lock(int n, WORD kind) const
{
	if (FileType == DataFileType::FandFile) {
		WORD m;
		std::string XTxt = "CrX";
		bool result = true;

#ifdef FandSQL
		if (fand_file->_parent->IsSQLFile) return result;
#endif

#ifdef FandNetV
		if (!FF->IsShared()) return result;
		int w = 0;
		while (true) {
			if (!TryLockH(FF->Handle, RecLock + n, 1)) {
				if (kind != 2) {   /*0 Kind-wait, 1-wait until ESC, 2-no wait*/
					m = 826;
					if (n == 0) {
						FF->GetFileD()->SetPathAndVolume();
						SetMsgPar(CPath, XTxt);
						m = 825;
					}
					int w1 = PushWrLLMsg(m, kind == 1);
					if (w == 0) {
						w = w1;
					}
					else {
						PopW(w1, false);
					}
					/*beep; don't disturb*/
					if (KbdTimer(spec.NetDelay, kind)) {
						continue;
					}
				}
				result = false;
			}
			if (w != 0) {
				PopW(w);
			}
			break;
		}
#endif

		return result;
	}
	else {
		// locks are not supported in other file types
		return true;
	}
}

void FileD::Unlock(int n)
{
	if (FileType == DataFileType::FandFile) {
		UnLockN(this->FF, n);
	}
	else {
		// locks are not supported in other file types
	}
}

void FileD::RunErrorM(LockMode mode)
{
	OldLockMode(mode);
}

void FileD::SetTWorkFlag(uint8_t* record)
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->SetTWorkFlag(record);
		break;
	case DataFileType::DBF:
		DbfF->SetTWorkFlag(record);
		break;
	default:
		break;
	}
}

bool FileD::HasTWorkFlag(uint8_t* record)
{
	bool result;

	switch (FileType) {
	case DataFileType::FandFile:
		result = FF->HasTWorkFlag(record);
		break;
	case DataFileType::DBF:
		result = DbfF->HasTWorkFlag(record);
		break;
	default:
		result = false;
		break;
	}

	return result;
}

void FileD::SetRecordUpdateFlag(uint8_t* record)
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->SetRecordUpdateFlag(record);
		break;
	case DataFileType::DBF:
		DbfF->SetRecordUpdateFlag(record);
		break;
	default:
		break;
	}
}

void FileD::ClearRecordUpdateFlag(uint8_t* record)
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->ClearRecordUpdateFlag(record);
		break;
	case DataFileType::DBF:
		DbfF->ClearRecordUpdateFlag(record);
		break;
	default:
		break;
	}
}

bool FileD::HasRecordUpdateFlag(uint8_t* record)
{
	bool result;

	switch (FileType) {
	case DataFileType::FandFile:
		result = FF->HasRecordUpdateFlag(record);
		break;
	case DataFileType::DBF:
		result = DbfF->HasRecordUpdateFlag(record);
		break;
	default:
		result = false;
		break;
	}

	return result;
}

bool FileD::DeletedFlag(uint8_t* record)
{
	bool result;

	switch (FileType) {
	case DataFileType::FandFile:
		result = FF->DeletedFlag(record);
		break;
	case DataFileType::DBF:
		result = DbfF->DeletedFlag(record);
		break;
	default:
		result = false;
		break;
	}

	return result;
}

void FileD::ClearDeletedFlag(uint8_t* record) const
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->ClearDeletedFlag(record);
		break;
	case DataFileType::DBF:
		DbfF->ClearDeletedFlag(record);
		break;
	default:
		break;
	}
}

void FileD::SetDeletedFlag(uint8_t* record) const
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->SetDeletedFlag(record);
		break;
	case DataFileType::DBF:
		DbfF->SetDeletedFlag(record);
		break;
	default:
		break;
	}
}

uint16_t FileD::RdPrefix() const
{
	uint16_t result;
	switch (FileType) {
	case DataFileType::FandFile:
		result = FF->RdPrefix();
		break;
	case DataFileType::DBF:
		result = DbfF->RdPrefix();
		break;
	default:
		result = 0;
		break;
	}
	return result;
}

void FileD::WrPrefix() const
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->WrPrefix();
		break;
	case DataFileType::DBF:
		DbfF->WrPrefix();
		break;
	default:
		break;
	}
}

void FileD::WrPrefixes() const
{
	switch (FileType) {
	case DataFileType::FandFile:
		FF->WrPrefixes();
		break;
	case DataFileType::DBF:
		DbfF->WrPrefixes();
		break;
	default:
		break;
	}
}

bool FileD::HasIndexFile() const
{
	bool result;

	if (FileType == DataFileType::FandFile) {
		result = (FF->XF != nullptr);
	}
	else {
		result = false;
	}

	return result;
}

bool FileD::HasTextFile() const
{
	bool result;

	switch (FileType) {
	case DataFileType::FandFile:
		result = (FF->TF != nullptr);
		break;
	case DataFileType::DBF:
		result = (DbfF->TF != nullptr);
		break;
	default:
		result = false;
		break;
	}

	return result;
}

bool FileD::SearchKey(XString& XX, XKey* Key, int& NN, uint8_t* record) const
{
	return FF->SearchKey(XX, Key, NN, record);
}

bool FileD::SearchXKey(XKey* K, XString& X, int& N)
{
	if (FF->file_type == FandFileType::INDEX) {
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
	SetPathAndVolume();
	bool net = IsNetCVol();
	FileD* newFile = new FileD(*this);

	std::string path = SetTempCExt('0', net);
	CVol = "";
	newFile->FullPath = path;
	newFile->FF->Handle = OpenH(path, _isOverwriteFile, Exclusive);
	newFile->TestCFileError();
	newFile->FF->NRecs = 0;
	newFile->IRec = 0;
	newFile->FF->Eof = true;
	newFile->FF->UMode = Exclusive;
	newFile->FF->SetUpdateFlag();

	// create index file
	if (newFile->FF->file_type == FandFileType::INDEX) {
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
		std::string path_t = SetTempCExt('T', net);
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
	SetPathAndVolume();
	SetTempCExt('0', FF->IsShared());
	MyDeleteFile(CPath);
}

void FileD::ZeroAllFlds(uint8_t* record, bool delTFields)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		if (delTFields) {
			this->FF->DelTFlds(record);
		}
		memset(record, 0, FF->RecLen);
		break;
	}
	case DataFileType::DBF: {
		if (delTFields) {
			this->DbfF->DelTFlds(record);
		}
		memset(record, 0, DbfF->RecLen);
		break;
	}
	default:;
	}

	for (FieldDescr* F : FldD) {
		if (F->isStored() && (F->field_type == FieldType::ALFANUM)) {
			saveS(F, "", record);
		}
	}
}

/// \brief Copy complete record
/// - for T: one of these files is always TWork
/// \param src_record source record
/// \param dst_record destination record
/// \param delTFields deletes the existing destination T first
void FileD::CopyRec(uint8_t* src_record, uint8_t* dst_record, bool delTFields)
{
	if (delTFields) {
		this->FF->DelTFlds(dst_record);
	}

	for (FieldDescr* const& f : FldD) {
		if (f->isStored()) {
			switch (f->field_type) {
			case FieldType::TEXT: {
				bool src_is_work = HasTWorkFlag(src_record);
				bool dst_is_work = HasTWorkFlag(dst_record);

				if (FileType == DataFileType::DBF) {
					// if (src_t00_file->Format != FandTFile::T00Format)
					std::string s = loadS(f, src_record);
					saveS(f, s, dst_record);
				}
				else
				{
					FandTFile* src_t00_file = nullptr;
					FandTFile* dst_t00_file = nullptr;

					if (src_is_work) {
						src_t00_file = &TWork;
						dst_t00_file = FF->TF;
					}

					if (dst_is_work) {
						src_t00_file = FF->TF;
						dst_t00_file = &TWork;
					}

					int32_t src_pos = loadT(f, src_record);
					int32_t dst_pos = 0;

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

void FileD::DelAllDifTFlds(uint8_t* record, uint8_t* comp_record)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		FF->DelAllDifTFlds(record, comp_record);
		break;
	}
	case DataFileType::DBF: {
		DbfF->DelAllDifTFlds(record, comp_record);
		break;
	}
	default:
		break;
	}
}

std::string FileD::CExtToT(const std::string& dir, const std::string& name, std::string ext)
{
	if (EquUpCase(ext, ".RDB")) {
		ext = ".TTT";
	}
	else if (EquUpCase(ext, ".DBF")) {
		if (DbfF->TF->Format == DbfTFile::FptFormat) {
			ext = ".FPT";
		}
		else {
			ext = ".DBT";
		}
	}
	else {
		ext[1] = 'T';
	}
	return dir + name + ext;
}

std::string FileD::SetTempCExt(char typ, bool isNet)
{
	std::string result;

	switch (FileType) {
	case DataFileType::FandFile: {
		result = FF->SetTempCExt(typ, isNet);
		break;
	}
	case DataFileType::DBF: {
		result = DbfF->SetTempCExt(typ, isNet);
		break;
	}
	default: break;
		break;
	}

	return result;
}

void FileD::SetHCatTyp(FandFileType fand_file_type)
{
	/// smaze Handle, nastavi typ na FDTyp a ziska CatIRec z GetCatalogIRec() - musi existovat catalog
	if (FileType == DataFileType::FandFile) {
		FF->Handle = nullptr;
		FF->file_type = fand_file_type;
	}

	CatIRec = catalog->GetCatalogIRec(Name, FF != nullptr && FF->file_type == FandFileType::RDB /*multilevel*/);

#ifdef FandSQL
	typSQLFile = isSql;
	SetIsSQLFile();
#endif
}

void FileD::GetTFileD(bool has_tt)
{
	switch (FileType) {
	case DataFileType::FandFile: {
		if (!has_tt && (FF->TF == nullptr)) return;
		if (FF->TF == nullptr) {
			FF->TF = new FandTFile(FF);
		}
		FF->TF->Handle = nullptr;
		break;
	}
	case DataFileType::DBF: {
		if (!has_tt && (DbfF->TF == nullptr)) return;
		if (DbfF->TF == nullptr) {
			DbfF->TF = new DbfTFile(DbfF);
		}
		DbfF->TF->Handle = nullptr;
		DbfF->TF->Format = DbfTFile::DbtFormat;
		break;
	}
	default: break;
	}
}

int32_t FileD::GetXFileD()
{
	int32_t result = 0;

	if (FileType == DataFileType::FandFile) {
		if (FF->file_type != FandFileType::INDEX) {
			if (FF->XF != nullptr) {
				//gc->OldError(104);
				result = 104;
			}
		}
		else {
			if (FF->XF == nullptr) {
				FF->XF = new FandXFile(FF);
			}

			FF->XF->Handle = nullptr;
		}
	}

	return result;
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

bool FileD::IsOpen()
{
	bool result;

	switch (FileType) {
	case DataFileType::FandFile: {
		result = FF->Handle != nullptr;
		break;
	}
	case DataFileType::DBF: {
		result = DbfF->Handle != nullptr;
		break;
	}
	default:
		result = false;
		break;
	}

	return result;
}

bool FileD::IsShared()
{
	if (FileType == DataFileType::FandFile) {
		return FF->IsShared();
	}
	else {
		return false;
	}
}

bool FileD::NotCached()
{
	if (GetUMode() == Shared) goto label1;
	if (GetUMode() != RdShared) return false;
label1:
	if (GetLMode() == ExclMode) return false;
	return true;
}

bool FileD::Cached()
{
	return !NotCached();
}

bool FileD::OpenF(const std::string& path, FileUseMode UM)
{
	bool result = true;
	if (IsOpen()) return result;
	if (OpenF1(path, UM)) {
		if (IsShared()) {
			ChangeLockMode(RdMode, 0, false);
			SetLMode(RdMode);
		}
		result = OpenF2(path);
		OldLockMode(NullMode);
	}
	else {
		result = false;
	}
	return result;
}

bool FileD::OpenF1(const std::string& path, FileUseMode UM)
{
	WORD n;
	bool result = true;
	SetLMode(NullMode);
	SetPathMountVolumeSetNet(UM);
	const bool b = (this == Chpt) || (this == catalog->GetCatalogFile());
	if (b && (IsTestRun || IsInstallRun) && ((GetFileAttr(CPath, HandleError) & 0b00000001/*RdOnly*/) != 0)) {
		SetFileAttr(CPath, HandleError, GetFileAttr(CPath, HandleError) & 0b00100110);
		if (HandleError == 5) HandleError = 79;
		TestCFileError();
		SetWasRdOnly(true);
	}
	while (true) {
		HANDLE h = OpenH(CPath, _isOldFile, GetUMode());
		SetHandle(h);
		if ((HandleError != 0) && GetWasRdOnly()) {
			SetFileAttr(CPath, HandleError, (GetFileAttr(CPath, HandleError) & 0b00100111) | 0b00000001 /*RdONly*/);
			TestCFileError();
		}
		if ((HandleError == 5) && (GetUMode() == Exclusive)) {
			SetUMode(RdOnly);
			continue;
		}
		if (HandleError == 2) {
			result = false;
			return result;
		}
		break;
	}
#ifndef FandNetV
	if ((HandleError == 5 || HandleError == 0x21) &&
		((CVol == '#') || (CVol == "##") || SEquUpcase(CVol, "#R"))) CFileError(842);
#endif
	TestCFileError();

	// open text file (.T__)
	if (HasTextFile()) {
		CPath = CExtToT(CDir, CName, CExt);
		if (GetWasRdOnly()) {
			SetFileAttr(CPath, HandleError, GetFileAttr(CPath, HandleError) & 0b00100110); // 0x26 = archive + hidden + system
		}
		while (true) {
			HANDLE h = OpenH(CPath, _isOldFile, GetUMode());
			SetHandleT(h);
			if (HandleError == 2) {
				if (FileType == DataFileType::DBF && DbfF->TF->Format == DbfTFile::DbtFormat) {
					DbfF->TF->Format = DbfTFile::FptFormat;
					CExt = ".FPT";
					CPath = CDir + CName + CExt;
					continue;
				}
				if (IsDynFile) {
					if (FileType == DataFileType::FandFile) {
						CloseClearH(&FF->Handle);
					}
					else if (FileType == DataFileType::DBF) {
						CloseClearH(&DbfF->Handle);
					}
					result = false;
					return result;
				}
			}
			break;
		}

		if (HandleError != 0) {
			n = HandleError;
			Close();
			HandleError = n;
			TestCPathError();
			return result;
		}
	}

	// open index file (*.X__)
	if (HasIndexFile()) {
		CPath = CExtToX(CDir, CName, CExt);
		while (true) {
			FF->XF->Handle = OpenH(CPath, _isOldFile, FF->UMode);
			if (HandleError == 2) {
				FF->XF->Handle = OpenH(CPath, _isOverwriteFile, Exclusive);
				if (HandleError != 0) {
					n = HandleError;
					FF->Close(); //CloseClearH(file_d->FF);
					HandleError = n;
					TestCPathError();
					return result;
				}
				FF->XF->SetNotValid(FF->NRecs, GetNrKeys());
				CloseH(&FF->XF->Handle);
				continue;
			}
			if (HandleError != 0) {
				n = HandleError;
				FF->Close(); //CloseClearH(file_d->FF);
				HandleError = n;
				TestCPathError();
			}
			if (FF->XF != nullptr && FileSizeH(FF->XF->Handle) < 512) {
				FF->XF->SetNotValid(FF->NRecs, GetNrKeys());
			}
			break;
		}
	}
	return result;
}

bool FileD::OpenF2(const std::string& path)
{
	int file_size = GetFileSize();
	SetNRecs(0);

	if (file_size < GetFirstRecPos()) {
		lock_excl_and_write_prefix();
	}
	else {
		uint16_t rLen = RdPrefix();
		int32_t n = (file_size - GetFirstRecPos()) / GetRecLen();

		if (rLen != 0xffff) {
			if (IsDynFile) {
				Close(); //CloseClearH(file_d->FF);
				return false;
			}
			else {
				if (catalog->OldToNewCat(file_size)) {
					CheckT(file_size);
					CheckX(file_size);
					SeekRec(0);
					return true;
				}

				FileMsg(this, 883, ' ');
				int l = GetNRecs() * rLen + GetFirstRecPos();

				if (l == file_size || !PromptYN(885)) {
					Close();
					GoExit(MsgLine);
				}

				if (GetNRecs() == 0 || l >> CachePageShft != file_size >> CachePageShft) {
					WrLLF10Msg(886);
					SetNRecs(n);
				}

				SetUpdateFlag();
				WrPrefix();
			}
		}
		else {
			if (n < GetNRecs()) {
				SetPathAndVolume();
				SetMsgPar(CPath);
				if (PromptYN(882)) {
					SetNRecs(n);
					lock_excl_and_write_prefix();
				}
				else {
					Close();
					GoExit(MsgLine);
				}
			}
		}
	}

	CheckT(file_size);
	CheckX(file_size);

	SeekRec(0);
	return true;
}

void FileD::CreateF()
{
	std::string path = SetPathMountVolumeSetNet(Exclusive);
	HANDLE h = OpenH(path, _isOverwriteFile, Exclusive);
	SetHandle(h);
	TestCFileError();
	SetNRecs(0);

	if (HasTextFile()) {
		path = CExtToT(CDir, CName, CExt);
		CreateT(path);
	}

	if (HasIndexFile() && FF->file_type == FandFileType::INDEX) {
		path = CExtToX(CDir, CName, CExt);
		FF->XF->Handle = OpenH(path, _isOverwriteFile, Exclusive);
		FF->XF->TestErr(); /*SetNotValid*/
		FF->XF->SetEmpty(FF->NRecs, GetNrKeys());
	}

	SeekRec(0);
	SetUpdateFlag();
}

bool FileD::OpenCreateF(const std::string& path, FileUseMode UM)
{
	if (!OpenF(path, UM)) {
		CreateF();
		if ((UM == Shared) || (UM == RdShared)) {
			WrPrefixes();

			if (FileType == DataFileType::FandFile) {
				SaveCache(0, FF->Handle);
				CloseClearH(&FF->Handle);

				if (FF->file_type == FandFileType::INDEX) {
					CloseClearH(&FF->XF->Handle);
				}

				if (FF->TF != nullptr) {
					CloseClearH(&FF->TF->Handle);
				}
			}
			else if (FileType == DataFileType::DBF) {
				SaveCache(0, DbfF->Handle);
				CloseClearH(&DbfF->Handle);

				if (DbfF->TF != nullptr) {
					CloseClearH(&DbfF->TF->Handle);
				}
			}
			else {
				// other types don't have index or text files
			}

			// TODO: here is probably an issue with file caching
			// wait 100 ms before re-open
			//std::this_thread::sleep_for(std::chrono::milliseconds(100));
			OpenF(path, UM);
		}
	}
	return true;
}

void FileD::DeleteF()
{
	CloseFile();
	SetPathAndVolume();
	MyDeleteFile(CPath);
	CPath = CExtToX(CDir, CName, CExt);
	if (FF->XF != nullptr) {
		MyDeleteFile(CPath);
	}
	if (FF->TF != nullptr) {
		CPath = CExtToT(CDir, CName, CExt);
		MyDeleteFile(CPath);
	}
}

void FileD::TestCFileError()
{
	if (HandleError != 0) {
		CFileError(700 + HandleError);
	}
}

std::string FileD::SetPathMountVolumeSetNet(FileUseMode UM)
{
	std::string path = SetPathAndVolume();
	SetUMode(UM);
	SetDrive((uint8_t)TestMountVol(path[0]));
	if (!IsNetCVol() || (this == Chpt))
		switch (UM) {
		case RdShared: SetUMode(RdOnly); break;
		case Shared: SetUMode(Exclusive); break;

		case Closed:
		case RdOnly:
		case Exclusive: break;
		}
	else if ((UM == Shared) && EquUpCase(CVol, "#R")) {
		SetUMode(RdShared);
	}
	CPath = path;
	return path;
}

std::string FileD::SetPathAndVolume(char pathDelim)
{
	bool isRdb = false;

	CVol = "";
	if (FileType == DataFileType::FandFile && FF->file_type == FandFileType::CAT) {
		CDir = GetEnv("FANDCAT");
		if (CDir.empty()) {
			CDir = TopDataDir.empty() ? TopRdbDir : TopDataDir;
		}
		AddBackSlash(CDir);
		CName = CatFDName;
		CExt = ".CAT";
		goto finish;
	}

	if (CatIRec != 0) {
		catalog->GetPathAndVolume(this, CatIRec, CPath, CVol);
		FSplit(CPath, CDir, CName, CExt);
		if (Name == "@") {
			CName = Name;
			goto finish;
		}
		else {
			goto finish;
		}
	}

	switch (FileType) {
	case DataFileType::FandFile: {
		switch (FF->file_type) {
		case FandFileType::RDB: {
			CExt = ".RDB";
			break;
		}
		case FandFileType::FAND8: {
			CExt = ".DTA";
			break;
		}
		default: {
			CExt = ".000";
			break;
		}
		}
		break;
	}
	case DataFileType::DBF: {
		CExt = ".DBF";
		break;
	}
	default:
		// other types don't have an extension
		break;
	}

	if (SetContextDir(this, CDir, isRdb)) {
		// do nothing
	}
	else {
		if (this == HelpFD) {
			CDir = FandDir;
#ifdef FandRunV 
			CName = "UFANDHLP";
#else
			CName = "FANDHLP";
#endif
			goto finish;
		}
		CExt = ".100";
		if (CRdb != nullptr) {
			CDir = CRdb->DataDir;
		}
		else {
			CDir = "";
		}
	}

	AddBackSlash(CDir);
	CName = Name;

finish:
	if (pathDelim == '/') ReplaceChar(CDir, '\\', '/');
	if (pathDelim == '\\') ReplaceChar(CDir, '/', '\\');
	CPath = CDir + CName + CExt;
	return CPath;
}

void FileD::CFileError(int N)
{
	FileMsg(this, N, '0');
	Close();
	GoExit(MsgLine);
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

void FileD::CopyH(HANDLE h1, HANDLE h2)
{
	const WORD BufSize = 32768;
	uint8_t* p = new uint8_t[BufSize];
	int sz = FileSizeH(h1);
	SeekH(h1, 0);
	SeekH(h2, 0);
	while (sz > BufSize) {
		ReadH(h1, BufSize, p);
		WriteH(h2, BufSize, p);
		sz -= BufSize;
	}
	ReadH(h1, sz, p);
	WriteH(h2, sz, p);
	CloseH(&h1);
	MyDeleteFile(CPath);
	ReleaseStore(&p);
}

std::string FileD::SetPathForH(HANDLE handle)
{
	RdbD* RD = CRdb;
	while (RD != nullptr) {
		for (FileD* fd : RD->v_files) {
			if (fd->FF->Handle == handle) {
				fd->SetPathAndVolume();
				return CPath;
			}

			if (fd->FF->XF != nullptr && fd->FF->XF->Handle == handle) {
				fd->SetPathAndVolume();
				CPath = CExtToX(CDir, CName, CExt);
				return CPath;
			}

			if (fd->FF->TF != nullptr && fd->FF->TF->Handle == handle) {
				fd->SetPathAndVolume();
				CPath = fd->CExtToT(CDir, CName, CExt);
				return CPath;
			}
		}
		RD = RD->ChainBack;
	}
	ReadMessage(799);
	CPath = MsgLine;
	return CPath;
}

void FileD::lock_excl_and_write_prefix()
{
	if (IsShared() && (GetLMode() < ExclMode)) {
		ChangeLockMode(ExclMode, 0, false);
	}
	SetLMode(ExclMode);
	SetUpdateFlag();
	WrPrefix();
}
