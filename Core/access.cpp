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
			file_d->UpdateRec(n, rec);
		}

		AssgnFrml(rec, field_d, frml, add);
		file_d->UpdateRec(n, rec);
		file_d->OldLockMode(md);
		delete rec; rec = nullptr;
	}
}

// zrejme zajistuje pristup do jine tabulky (cizi klic)
Record* LinkUpw(LinkD* LD, int& N, bool WithT, Record* record)
{
	FileD* from_FD = LD->FromFile;
	FileD* to_FD = LD->ToFile;
	Record* up_rec = new Record(to_FD);

	XKey* K = LD->ToKey;
	XString x;
	x.PackKF(LD->Args, record);

	// TODO: FandSQL removed

	const LockMode md = to_FD->NewLockMode(RdMode);
	bool lu;
	if (to_FD->FF->file_type == FandFileType::INDEX) {
		to_FD->FF->TestXFExist();
		lu = K->SearchInterval(to_FD, x, false, N);
	}
	else if (to_FD->GetNRecs() == 0) {
		lu = false;
		N = 1;
	}
	else {
		lu = to_FD->FF->SearchKey(x, K, N, up_rec);
	}

	if (lu) {
		to_FD->ReadRec(N, up_rec);
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
				// used only in Export/Import /TbFile.cpp
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
		if (c == 0x49 && !trans.empty()) {           // znak 'H'
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
