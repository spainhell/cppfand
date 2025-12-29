#include "access.h"

#include "Compiler.h"
#include "../fandio/FieldDescr.h"
#include "../Common/FileD.h"
#include "GlobalVariables.h"
#include "../fandio/KeyFldD.h"
#include "../Common/LinkD.h"
#include "../Common/Record.h"
#include "obaseww.h"
#include "runfrml.h"
#include "../fandio/Fand0File.h"
#include "../Logging/Logging.h"
#include "../Common/compare.h"
#include "../Common/CommonVariables.h"


void TestCPathError()
{
	if (HandleError != 0) {
		WORD n = 700 + HandleError;
		if ((n == 705) && (CPath[CPath.length()] == '\\')) n = 840;
		SetMsgPar(CPath);
		RunError(n);
	}
}

//bool LinkLastRec(FileD* file_d, int& N, bool WithT, uint8_t** newRecord)
//{
//	*newRecord = file_d->GetRecSpace();
//	LockMode md = file_d->NewLockMode(RdMode);
//	auto result = true;
//#ifdef FandSQL
//	if (file_d->IsSQLFile) {
//		if (Strm1->SelectXRec(nullptr, nullptr, _equ, WithT)) N = 1;
//		else {
//			file_d->ZeroAllFlds(*newRecord, false);
//			result = false;
//			N = 1;
//		}
//	}
//	else
//#endif
//	{
//		N = file_d->FF->NRecs;
//		if (N == 0) {
//			file_d->ZeroAllFlds(*newRecord, false);
//			result = false;
//			N = 1;
//		}
//		else {
//			file_d->FF->ReadRec(N, *newRecord);
//		}
//	}
//	file_d->OldLockMode(md);
//	return result;
//}

// ulozi hodnotu parametru do souboru
void AsgnParFldFrml(FileD* file_d, FieldDescr* field_d, FrmlElem* frml, bool add)
{
	//#ifdef _DEBUG
	std::string FileName = file_d->FullPath;
	std::string Varible = field_d->Name;
	//#endif

#ifdef FandSQL
	if (file_d->IsSQLFile) {
		CRecPtr = GetRecSpace; ZeroAllFlds; AssgnFrml(field_d, frml, true, add);
		Strm1->UpdateXFld(nullptr, nullptr, field_d); ClearRecSpace(CRecPtr)
	}
	else
#endif
	{
		int n = 0;
		LockMode md = file_d->NewLockMode(WrMode);
		
		Record* rec = file_d->LinkLastRec(n);

		if (rec == nullptr) {
			// add a new empty record to the file
			file_d->IncNRecs(1);
			rec = new Record(file_d);
			file_d->WriteRec(n, rec);
		}

		AssgnFrml(rec, field_d, frml, add);
		file_d->WriteRec(n, rec);
		file_d->OldLockMode(md);
		delete rec; rec = nullptr;
	}
}

//// zrejme zajistuje pristup do jine tabulky (cizi klic)
//bool LinkUpw(LinkD* LD, int& N, bool WithT, uint8_t* record, uint8_t** newRecord)
//{
//	FileD* from_FD = LD->FromFD;
//	FileD* to_FD = LD->ToFD;
//	*newRecord = to_FD->GetRecSpace();
//
//	XKey* K = LD->ToKey;
//	XString x;
//	x.PackKF(from_FD, LD->Args, record);
//
//	// TODO: FandSQL removed
//
//	const LockMode md = to_FD->NewLockMode(RdMode);
//	bool lu;
//	if (to_FD->FF->file_type == FandFileType::INDEX) {
//		to_FD->FF->TestXFExist();
//		lu = K->SearchInterval(to_FD, x, false, N);
//	}
//	else if (to_FD->FF->NRecs == 0) {
//		lu = false;
//		N = 1;
//	}
//	else {
//		lu = to_FD->FF->SearchKey(x, K, N, *newRecord);
//	}
//
//	if (lu) {
//		to_FD->FF->ReadRec(N, *newRecord);
//	}
//	else {
//		to_FD->ZeroAllFlds(*newRecord, false);
//		for (size_t i = 0; i < LD->Args.size(); i++) {
//			FieldDescr* F = LD->Args[i]->FldD;
//			FieldDescr* F2 = K->KFlds[i]->FldD;
//			if (F2->isStored()) {
//				switch (F->frml_type) {
//				case 'S': {
//					x.S = from_FD->loadS(F, record);
//					to_FD->saveS(F2, x.S, *newRecord);
//					break;
//				}
//				case 'R': {
//					const double r = from_FD->loadR(F, record);
//					to_FD->saveR(F2, r, *newRecord);
//					break;
//				}
//				case 'B': {
//					const bool b = from_FD->loadB(F, record);
//					to_FD->saveB(F2, b, *newRecord);
//					break;
//				}
//				}
//			}
//		}
//	}
//
//	// TODO: FandSQL removed
//	to_FD->OldLockMode(md);
//
//	return lu;
//}

// zrejme zajistuje pristup do jine tabulky (cizi klic)
Record* LinkUpw(LinkD* LD, int& N, bool WithT, Record* record)
{
	FileD* from_FD = LD->FromFD;
	FileD* to_FD = LD->ToFD;
	Record* up_rec = new Record(to_FD);

	XKey* K = LD->ToKey;
	XString x;
	x.PackKF(from_FD, LD->Args, record);

	// TODO: FandSQL removed

	const LockMode md = to_FD->NewLockMode(RdMode);
	bool lu;
	if (to_FD->FF->file_type == FandFileType::INDEX) {
		to_FD->FF->TestXFExist();
		lu = K->SearchInterval(to_FD, x, false, N);
	}
	else if (to_FD->FF->NRecs == 0) {
		lu = false;
		N = 1;
	}
	else {
		lu = to_FD->FF->SearchKey(x, K, N, up_rec);
	}

	if (lu) {
		to_FD->FF->ReadRec(N, up_rec);
	}
	else {
		up_rec->Reset(); //to_FD->ZeroAllFlds(up_rec, false);
		for (size_t i = 0; i < LD->Args.size(); i++) {
			FieldDescr* F = LD->Args[i]->FldD;
			FieldDescr* F2 = K->KFlds[i]->FldD;
			if (F2->isStored()) {
				switch (F->frml_type) {
				case 'S': {
					const std::string s = record->LoadS(F);
					up_rec->SaveS(F2, s);
					break;
				}
				case 'R': {
					const double r = record->LoadR(F);
					up_rec->SaveR(F2, r);
					break;
				}
				case 'B': {
					const bool b = record->LoadB(F);
					up_rec->SaveB(F2, b);
					break;
				}
				}
			}
		}
	}

	// TODO: FandSQL removed
	to_FD->OldLockMode(md);

	return up_rec;
}

LocVar* LocVarBlock::GetRoot()
{
	if (this->variables.size() > 0) return this->variables[0];
	return nullptr;
}

LocVar* LocVarBlock::FindByName(std::string Name)
{
	for (LocVar* i : variables) {
		if (EquUpCase(Name, i->name)) return i;
	}
	return nullptr;
}

void DirMinusBackslash(pstring& D)
{
	if ((D.length() > 3) && (D[D.length() - 1] == '\\')) D[0]--;
}

void ProcessFileOperation(ForAllFilesOperation op, FileD* file_d)
{
	if (file_d == nullptr)
	{
		return;
	}

	switch (op) {
	case ForAllFilesOperation::close: {
		file_d->CloseFile();
		break;
	}
	case ForAllFilesOperation::save: {
		file_d->Save();
		break;
	}
	case ForAllFilesOperation::clear_xf_update_lock: {
		file_d->FF->ClearXFUpdLock();
		break;
	}
	case ForAllFilesOperation::save_l_mode: {
		file_d->FF->ExLMode = file_d->FF->LMode;
		break;
	}
	case ForAllFilesOperation::set_old_lock_mode: {
		file_d->OldLockMode(file_d->FF->ExLMode);
		break;
	}
	case ForAllFilesOperation::close_passive_fd: {
		if ((file_d->FF->file_type != FandFileType::RDB) && (file_d->FF->LMode == NullMode)) {
			file_d->CloseFile();
		}
		break;
	}
	default: 
		break;
	}
}

void ForAllFDs(ForAllFilesOperation op, FileD** file_d, WORD i)
{
	Project* R = CRdb;
	while (R != nullptr) {
		ProcessFileOperation(op, R->project_file);
		
		ProcessFileOperation(op, R->help_file);
		
		for (FileD* f : R->data_files) {
			if (op == ForAllFilesOperation::find_fd_for_i) {
				if (*file_d == nullptr && f->CatIRec == i) {
					*file_d = f;
					return;
					break;
				}
			}
			else {
				ProcessFileOperation(op, f);
			}
		}
		R = R->ChainBack;
	}
}

std::string TranslateOrd(std::string text)
{
	std::string trans;
	for (size_t i = 0; i < text.length(); i++) {
		char c = CharOrdTab[(uint8_t)text[i]];
#ifndef FandAng
		if (c == 0x49 && trans.length() > 0) {       // znak 'H'
			if (trans[trans.length() - 1] == 0x43) { // posledni znak ve vystupnim retezci je 'C' ?
				trans[trans.length() - 1] = 0x4A;    // na vstupu bude 'J' jako 'CH'
				continue;
			}
		}
#endif
		trans += c;
	}
	return trans;
}

std::string CExtToX(const std::string dir, const std::string name, std::string ext)
{
	ext[1] = 'X';
	return dir + name + ext;
}

std::string CExtToT(const std::string& dir, const std::string& name, std::string ext)
{
	if (EquUpCase(ext, ".RDB")) {
		ext = ".TTT";
	}
	else {
		ext[1] = 'T';
	}
	return dir + name + ext;
}

//std::string TWork_LoadS(FieldDescr* field, uint8_t* record)
//{
//	std::string result;
//	if (field->field_type == FieldType::TEXT && field->isStored())
//	{
//		char* source = (char*)record + field->Displ;
//		int32_t* pos = reinterpret_cast<int32_t*>(source);
//		if (*pos == 0) {
//			return result;
//		}
//		result = TWork.Read(*pos);
//	}
//	else {
//		// non-text field -> cannot be saved in TWork
//	}
//	return result;
//}
//
//void TWork_SaveS(FieldDescr* field, const std::string& text, uint8_t* record)
//{
//	if (field->field_type == FieldType::TEXT && field->isStored())
//	{
//		char* source = (char*)record + field->Displ;
//		uint32_t* pos = reinterpret_cast<uint32_t*>(source);
//		if (*pos == 0) {
//			// there is nothing saved now
//		}
//		else
//		{
//			// delete previous text
//			TWork.Delete(*pos);
//			*pos = 0;
//		}
//		if (text.empty()) {
//			// nothing to save
//		}
//		else {
//			*pos = TWork.Store(text);
//		}
//	}
//	else {
//		// non-text field -> cannot be saved in TWork
//	}
//}
//
//
//void TWork_DeleteT(FieldDescr* field, uint8_t* record)
//{
//	char* source = (char*)record + field->Displ;
//	int32_t* pos = reinterpret_cast<int32_t*>(source);
//	if (*pos == 0) return;
//	TWork.Delete(*pos);
//	*pos = 0;
//}
//
//void TWork_DeleteAllT(const std::vector<FieldDescr*>& fields, uint8_t* record)
//{
//	for (FieldDescr* field : fields) {
//		if (field->field_type == FieldType::TEXT && field->isStored()) {
//			TWork_DeleteT(field, record);
//		}
//	}
//}
