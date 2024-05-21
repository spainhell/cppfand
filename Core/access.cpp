#include "access.h"

#include "../pascal/asm.h"
#include "Compiler.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "obaseww.h"
#include "runfrml.h"
#include "../fandio/FandFile.h"
#include "../Logging/Logging.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"



void CloseClearH(FandFile* fand_file)
{
	CloseClearH(&fand_file->Handle);
	if (fand_file->file_type == FileType::INDEX) {
		CloseClearH(&fand_file->XF->Handle);
	}
	if (fand_file->TF != nullptr) {
		CloseClearH(&fand_file->TF->Handle);
	}
}

void CloseGoExit(FandFile* fand_file)
{
	CloseClearH(fand_file);
	GoExit();
}

//BYTE ByteMask[_MAX_INT_DIG];

//const BYTE FixS = 8;
//BYTE Fix[FixS];
//BYTE RealMask[DblS + 1];

void TestCPathError()
{
	WORD n;
	if (HandleError != 0) {
		n = 700 + HandleError;
		if ((n == 705) && (CPath[CPath.length()] == '\\')) n = 840;
		SetMsgPar(CPath);
		RunError(n);
	}
}

bool LinkLastRec(FileD* file_d, int& N, bool WithT, BYTE** newRecord)
{
	*newRecord = file_d->GetRecSpace();
	LockMode md = file_d->NewLockMode(RdMode);
	auto result = true;
#ifdef FandSQL
	if (file_d->IsSQLFile)
	{
		if (Strm1->SelectXRec(nullptr, nullptr, _equ, WithT)) N = 1;
		else goto label1;
	}
	else
#endif
	{
		N = file_d->FF->NRecs;
		if (N == 0) {
		label1:
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
		BYTE* rec = nullptr;
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
bool LinkUpw(FileD* file_d, LinkD* LD, int& N, bool WithT, void* record, BYTE** newRecord)
{
	FileD* ToFD = LD->ToFD;
	*newRecord = ToFD->GetRecSpace();

	XKey* K = LD->ToKey;
	XString x;
	x.PackKF(file_d, LD->Args, record);


#ifdef FandSQL
	if (ToFD->IsSQLFile) {
		LU = Strm1->SelectXRec(K, @X, _equ, WithT); N = 1;
		if (LU) goto label2; else goto label1;
	}
#endif
	const LockMode md = ToFD->NewLockMode(RdMode);
	bool lu;
	if (ToFD->FF->file_type == FileType::INDEX) {
		ToFD->FF->TestXFExist();
		lu = K->SearchInterval(ToFD, x, false, N);
	}
	else if (ToFD->FF->NRecs == 0) {
		lu = false;
		N = 1;
	}
	else {
		lu = ToFD->FF->SearchKey(x, K, N, *newRecord);
	}

	if (lu) {
		ToFD->ReadRec(N, *newRecord);
	}
	else {
		ToFD->ZeroAllFlds(*newRecord, false);
		const KeyFldD* KF = K->KFlds;
		for (auto& arg : LD->Args) {
			FieldDescr* F = arg->FldD;
			FieldDescr* F2 = KF->FldD;
			if (F2->isStored()) {
				switch (F->frml_type) {
				case 'S': {
					x.S = file_d->loadS(F, record);
					ToFD->saveS(F2, x.S, *newRecord);
					break;
				}
				case 'R': {
					const double r = file_d->loadR(F, record);
					ToFD->saveR(F2, r, *newRecord);
					break;
				}
				case 'B': {
					const bool b = file_d->loadB(F, record);
					ToFD->saveB(F2, b, *newRecord);
					break;
				}
				}
			}
			KF = KF->pChain;
		}
	}

#ifdef FandSQL
	if (!ToFD->IsSQLFile)
#endif
		ToFD->OldLockMode(md);

	return lu;
}

LocVar* LocVarBlkD::GetRoot()
{
	if (this->vLocVar.size() > 0) return this->vLocVar[0];
	return nullptr;
}

LocVar* LocVarBlkD::FindByName(std::string Name)
{
	for (auto& i : vLocVar) {
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
		FileD* f = R->rdb_file;
		while (f != nullptr) {
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
				if ((f->FF->file_type != FileType::RDB) && (f->FF->LMode == NullMode)) {
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
			f = f->pChain;
		}
		R = R->ChainBack;
	}
	//CFile = cf;
}

void ResetCompilePars()
{
	// ptrRdFldNameFrml = g_compiler->RdFldNameFrmlF;
	g_compiler->rdFldNameType = FieldNameType::F;
	RdFunction = nullptr;
	FileVarsAllowed = true;
	FDLocVarAllowed = false;
	IdxLocVarAllowed = false;
	PrevCompInp.clear();
}

std::string TranslateOrd(std::string text)
{
	std::string trans;
	for (size_t i = 0; i < text.length(); i++) {
		char c = CharOrdTab[(BYTE)text[i]];
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

std::string CExtToT(FandTFile* t_file, const std::string& dir, const std::string& name, std::string ext)
{
	if (EquUpCase(ext, ".RDB")) {
		ext = ".TTT";
	}
	else if (EquUpCase(ext, ".DBF")) {
		if (t_file->Format == FandTFile::FptFormat) {
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