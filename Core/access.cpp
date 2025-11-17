#include "access.h"

#include "../pascal/asm.h"
#include "Compiler.h"
#include "../fandio/FieldDescr.h"
#include "../Common/FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "../Common/LinkD.h"
#include "obaseww.h"
#include "runfrml.h"
#include "../fandio/Fand0File.h"
#include "../Logging/Logging.h"
#include "../Common/compare.h"


void TestCPathError()
{
	if (HandleError != 0) {
		WORD n = 700 + HandleError;
		if ((n == 705) && (CPath[CPath.length()] == '\\')) n = 840;
		SetMsgPar(CPath);
		RunError(n);
	}
}

bool LinkLastRec(FileD* file_d, int& N, bool WithT, uint8_t** newRecord)
{
	*newRecord = file_d->GetRecSpace();
	LockMode md = file_d->NewLockMode(RdMode);
	auto result = true;
#ifdef FandSQL
	if (file_d->IsSQLFile) {
		if (Strm1->SelectXRec(nullptr, nullptr, _equ, WithT)) N = 1;
		else {
			file_d->ZeroAllFlds(*newRecord, false);
			result = false;
			N = 1;
		}
	}
	else
#endif
	{
		N = file_d->FF->NRecs;
		if (N == 0) {
			file_d->ZeroAllFlds(*newRecord, false);
			result = false;
			N = 1;
		}
		else {
			file_d->ReadRec(N, *newRecord);
		}
	}
	file_d->OldLockMode(md);
	return result;
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
		uint8_t* rec = nullptr;

		if (!LinkLastRec(file_d, n, true, &rec)) {
			file_d->IncNRecs(1);
			file_d->WriteRec(n, rec);
		}

		AssgnFrml(file_d, rec, field_d, frml, true, add);
		file_d->WriteRec(n, rec);
		file_d->OldLockMode(md);
		delete[] rec; rec = nullptr;
	}
}

// zrejme zajistuje pristup do jine tabulky (cizi klic)
bool LinkUpw(LinkD* LD, int& N, bool WithT, void* record, uint8_t** newRecord)
{
	FileD* from_FD = LD->FromFD;
	FileD* to_FD = LD->ToFD;
	*newRecord = to_FD->GetRecSpace();

	XKey* K = LD->ToKey;
	XString x;
	x.PackKF(from_FD, LD->Args, record);


#ifdef FandSQL
	if (ToFD->IsSQLFile) {
		LU = Strm1->SelectXRec(K, @X, _equ, WithT); N = 1;
		if (LU) goto label2; else goto label1;
	}
#endif
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
		lu = to_FD->FF->SearchKey(x, K, N, *newRecord);
	}

	if (lu) {
		to_FD->ReadRec(N, *newRecord);
	}
	else {
		to_FD->ZeroAllFlds(*newRecord, false);
		for (size_t i = 0; i < LD->Args.size(); i++) {
			FieldDescr* F = LD->Args[i]->FldD;
			FieldDescr* F2 = K->KFlds[i]->FldD;
			if (F2->isStored()) {
				switch (F->frml_type) {
				case 'S': {
					x.S = from_FD->loadS(F, record);
					to_FD->saveS(F2, x.S, *newRecord);
					break;
				}
				case 'R': {
					const double r = from_FD->loadR(F, record);
					to_FD->saveR(F2, r, *newRecord);
					break;
				}
				case 'B': {
					const bool b = from_FD->loadB(F, record);
					to_FD->saveB(F2, b, *newRecord);
					break;
				}
				}
			}
		}
	}

#ifdef FandSQL
	if (!ToFD->IsSQLFile)
#endif
		to_FD->OldLockMode(md);

	return lu;
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

void ForAllFDs(ForAllFilesOperation op, FileD** file_d, WORD i)
{
	//FileD* cf = CFile;
	RdbD* R = CRdb;
	while (R != nullptr) {
		//FileD* f = R->v_files;
		//while (f != nullptr) {
		for (FileD* f : R->v_files) {
			switch (op) {
			case ForAllFilesOperation::close: {
				f->CloseFile();
				break;
			}
			case ForAllFilesOperation::save: {
				f->Save();
				break;
			}
			case ForAllFilesOperation::clear_xf_update_lock: {
				f->FF->ClearXFUpdLock();
				break;
			}
			case ForAllFilesOperation::save_l_mode: {
				f->FF->ExLMode = f->FF->LMode;
				break;
			}
			case ForAllFilesOperation::set_old_lock_mode: {
				f->OldLockMode(f->FF->ExLMode);
				break;
			}
			case ForAllFilesOperation::close_passive_fd: {
				if ((f->FF->file_type != FandFileType::RDB) && (f->FF->LMode == NullMode)) {
					f->CloseFile();
				}
				break;
			}
			case ForAllFilesOperation::find_fd_for_i: {
				if ((*file_d == nullptr) && (f->CatIRec == i)) {
					*file_d = f;
				}
				break;
			}
			default:;
			}
			//f = f->pChain;
		}
		R = R->ChainBack;
	}
	//CFile = cf;
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