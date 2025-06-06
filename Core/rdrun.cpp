#include "rdrun.h"

#include "Additive.h"
#include "LogicControl.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "LinkD.h"
#include "obaseww.h"
#include "runfrml.h"
#include "../Core/DateTime.h"


XString OldMXStr;                  /* Merge */
bool Join;
bool PrintView;                  /* Report */
TextFile Rprt;		// puvodne text - souvisi s text. souborem

std::vector<FrmlElemSum*> PFZeroLst;
LvDescr* FrstLvM;
LvDescr* LstLvM; /* LstLvM->Ft=RF */
bool SelQuest;
EditD* EditDRoot;
FrmlElem* PgeSizeZ = nullptr, * PgeLimitZ = nullptr;
bool CompileFD = false;
bool EditRdbMode = false;
LocVarBlock LVBD;
std::string CalcTxt;
MergOpSt MergOpGroup = { _const, 0.0 };

bool EFldD::Ed(bool IsNewRec)
{
	return ((FldD->Flg & f_Stored) != 0) && (EdU || IsNewRec && EdN);
}

void ResetLVBD()
{
	//LVBD.pChain = nullptr;
	LVBD.variables.clear();
	LVBD.NParam = 0;
	LVBD.func_name = "";
}

bool Add(FileD* file_d, Additive* add_d, void* record, double value, bool back)
{
	bool result = true;

	if (back) value = -value;

	const double new_value = file_d->loadR(add_d->Field, record) + value;
	file_d->saveR(add_d->Field, new_value, record);

	if (add_d->Chk == nullptr) return result;

	if (!back && !RunBool(file_d, add_d->Chk->Bool, record)) {
		SetMsgPar(RunString(file_d, add_d->Chk->TxtZ, record));
		WrLLF10Msg(110);
		result = false;
	}

	return result;
}

bool RunAddUpdate(FileD* file_d, char kind, void* old_record, bool back, Additive* stop_add_d, LinkD* not_link_d, void* record)
{
	BYTE* cr2 = nullptr;
	BYTE* cr2_old = nullptr;
	bool result = true;
	Additive* add_d_back = nullptr;

	try {
		char kind2_old = 0;
		char kind2 = 0;
		for (Additive* add : file_d->Add) {
			if (add == stop_add_d) {
				// TODO: tady se nema CFile a CRecPtr vracet zpet, ale asi ma zustat !!!
				//ReleaseStore(&p);
				return result;
			}
			if ((not_link_d != nullptr) && (add->LD == not_link_d)) {
				delete[] cr2; cr2 = nullptr;
				delete[] cr2_old; cr2_old = nullptr;
				continue;
			}
			if (add->Assign) {
				if (Assign(file_d, add, record)) {
					delete[] cr2; cr2 = nullptr;
					delete[] cr2_old; cr2_old = nullptr;
					continue;
				}
				else {
					throw std::exception("fail");
				}
			}

			double r = RunReal(file_d, add->Frml, record);
			if (kind == '-') {
				r = -r;
			}
			double r_old = 0;
			if (kind == 'd') {
				r_old = RunReal(file_d, add->Frml, old_record);
			}
			add_d_back = add;

			int n2 = 0;
			int n2_old = 0;
			if (r != 0.0) {
				if (!Link(file_d, add, n2, kind2, record, &cr2)) {
					throw std::exception("fail");
				}
			}
			if (r_old != 0.0) {
				if (!Link(file_d, add, n2_old, kind2_old, old_record, &cr2_old)) {
					throw std::exception("fail");
				}
				if (n2_old == n2) {
					r = r - r_old;
					if (r == 0.0) {
						delete[] cr2; cr2 = nullptr;
						delete[] cr2_old; cr2_old = nullptr;
						continue;
					}
					n2_old = 0;
				}
			}
			if ((n2 == 0) && (n2_old == 0)) {
				delete[] cr2; cr2 = nullptr;
				delete[] cr2_old; cr2_old = nullptr;
				continue;
			}
			if (n2_old != 0) {
				if (!Add(add->File2, add, cr2_old, -r_old, back)) {
					throw std::exception("fail");
				}
			}
			if (n2 != 0) {
				if (!Add(add->File2, add, cr2, r, back)) {
					throw std::exception("fail");
				}
			}
			if ((n2_old != 0) && !TransAdd(add->File2, add, file_d, record, cr2_old, n2_old, kind2_old, false)) {
				throw std::exception("fail");
			}
			if ((n2 != 0) && !TransAdd(add->File2, add, file_d, record, cr2, n2, kind2, false)) {
				if (n2_old != 0) {
					bool b = TransAdd(add->File2, add, file_d, record, cr2_old, n2_old, kind2_old, true);
				}
				throw std::exception("fail");
			}
			if (n2_old != 0) {
				WrUpdRec(add->File2, add, file_d, record, cr2_old, n2_old);
			}
			if (n2 != 0) {
				WrUpdRec(add->File2, add, file_d, record, cr2, n2);
			}

			delete[] cr2; cr2 = nullptr;
			delete[] cr2_old; cr2_old = nullptr;
		}
	}

	catch (std::exception&) {
		delete[] cr2; cr2 = nullptr;
		delete[] cr2_old; cr2_old = nullptr;
		result = false;
		if (add_d_back != nullptr) {
			bool b = RunAddUpdate(file_d, kind, old_record, true, add_d_back, not_link_d, record);  /* backtracking */
		}
	}

	return result;
}

void CrIndRec(FileD* file_d, void* record)
{
	file_d->CreateRec(file_d->FF->NRecs + 1, record);
	file_d->RecallRec(file_d->FF->NRecs, record);
}

bool Link(FileD* file_d, Additive* add_d, int& n, char& kind2, void* record, BYTE** linkedRecord)
{
	// TODO: is param file_d needed?

	bool result = true;
	LinkD* ld = add_d->LD;
	kind2 = 'd';

	if (ld != nullptr) {
		if (LinkUpw(ld, n, false, record, linkedRecord)) {
			// TODO: file_d should change after calling LinkUpw to 'ld->ToFD'
			return result;
		}
		SetMsgPar(ld->RoleName);
	}
	else {
		if (!LinkLastRec(add_d->File2, n, false, linkedRecord)
#ifdef FandSQL
			&& !file_d->IsSQLFile
#endif
			) {
			file_d->IncNRecs(1);
			file_d->WriteRec(1, *linkedRecord);
		}
		return result;
	}
	kind2 = '+';
	if ((add_d->Create == 2) || (add_d->Create == 1) && PromptYN(132)) {
#ifdef FandSQL
		if (file_d->IsSQLFile) Strm1->InsertRec(false, true) else
#endif
		{
			file_d->ClearDeletedFlag(*linkedRecord);
			if ((ld != nullptr) && (file_d->FF->file_type == FandFileType::INDEX)) {
				CrIndRec(file_d, *linkedRecord);
				n = file_d->FF->NRecs;
			}
			else {
				file_d->CreateRec(n, *linkedRecord);
			}
		}
		return result;
	}
	WrLLF10Msg(119);
	result = false;
	return result;
}

bool TransAdd(FileD* file_d, Additive* AD, FileD* FD, void* RP, void* new_record, int N, char Kind2, bool Back)
{
	if (file_d->Add.empty()) {
		return true;
	}
	if (Kind2 == '+') {
		return RunAddUpdate(file_d, '+', nullptr, Back, nullptr, nullptr, new_record);
	}
	BYTE* rec = file_d->GetRecSpace();
#ifdef FandSQL
	// TODO: cele pripadne predelat, po refactoringu uz to nesedi
	if (CFile->IsSQLFile) {
		ld = AD->LD; if (ld == nullptr) Strm1->SelectXRec(nullptr, nullptr, _equ, false)
		else {
			CFile = v_files; CRecPtr = RP; x.PackKF(ld->Args);
			CFile = ld->ToFD; CRecPtr = CRold; Strm1->SelectXRec(ld->ToKey, @x, _equ, false)
		}
	}
	else
#endif
		file_d->ReadRec(N, rec);

	bool result = RunAddUpdate(file_d, 'd', rec, Back, nullptr, nullptr, new_record);

	delete[] rec; rec = nullptr;
	return result;
}

void WrUpdRec(FileD* file_d, Additive* add_d, FileD* fd, void* rp, void* new_record, int n)
{
	//XString x;
	//LinkD* ld;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		ld = add_d->LD; if (ld = nullptr) Strm1->UpdateXFld(nullptr, nullptr, add_d->Field)
		else {
			CFile = fd; CRecPtr = rp; x.PackKF(ld->Args);
			CFile = ld->ToFD; CRecPtr = new_record; Strm1->UpdateXFld(ld->ToKey, @x, add_d->Field)
		}
	}
	else
#endif
		file_d->WriteRec(n, new_record);
}

bool Assign(FileD* file_d, Additive* add_d, void* record)
{
	double r = 0.0;
	std::string s;
	bool b = false;
	int n2;
	char kind2;

	if (!RunBool(file_d, add_d->Bool, record)) {
		return true;
	}
	FieldDescr* f = add_d->Field;
	FrmlElem* z = add_d->Frml;

	switch (f->frml_type) {
	case 'R': {
		r = RunReal(file_d, z, record);
		break;
	}
	case 'S': {
		if (f->field_type == FieldType::TEXT) {
			s = RunString(file_d, z, record);
		}
		else {
			s = RunString(file_d, z, record);
		}
		break;
	}
	default: {
		b = RunBool(file_d, z, record);
		break;
	}
	}
	
	BYTE* linkedRecord = nullptr;

	if (!Link(file_d, add_d, n2, kind2, record, &linkedRecord)) {
		delete[] linkedRecord; linkedRecord = nullptr;
		return false;
	}

	switch (f->frml_type) {
	case 'R': {
		add_d->File2->saveR(f, r, linkedRecord);
		break;
	}
	case 'S': {
		if (f->field_type == FieldType::TEXT) {
			add_d->File2->saveS(f, s, linkedRecord);
		}
		else {
			add_d->File2->saveS(f, s, linkedRecord);
		}
		break;
	}
	default: {
		add_d->File2->saveB(f, b, linkedRecord);
		break;
	}
	}
	add_d->File2->WriteRec(n2, linkedRecord);

	delete[] linkedRecord; linkedRecord = nullptr;
	return true;
}

bool LockForAdd(FileD* file_d, WORD kind, bool Ta, LockMode& md)
{
	LockMode md1; /*0-ExLMode,1-lock,2-unlock*/
	bool result = false;

	for (Additive* add_d : file_d->Add) {
		if (file_d != add_d->File2) {
			switch (kind) {
			case 0: {
				if (Ta) {
					add_d->File2->FF->TaLMode = add_d->File2->FF->LMode;
				}
				else {
					add_d->File2->FF->ExLMode = add_d->File2->FF->LMode;
				}
				break;
			}
			case 1: {
				md = WrMode;
				if (add_d->Create > 0) {
					md = CrMode;
				}
				if (!add_d->File2->TryLockMode(md, md1, 2)) {
					return result;
				}
				break;
			}
			case 2: {
				if (Ta) {
					add_d->File2->OldLockMode(add_d->File2->FF->TaLMode);
				}
				else {
					add_d->File2->OldLockMode(add_d->File2->FF->ExLMode);
				}
				break;
			}
			}
			if (!LockForAdd(add_d->File2, kind, Ta, md)) {
				return result;
			}
		}
		else {
			continue;
		}
	}
	result = true;
	return result;
}

bool RunAddUpdate(FileD* file_d, char kind, void* old_record, LinkD* not_link_d, void* record)
{
	LockMode md;
	LockForAdd(file_d, 0, false, md);
	while (!LockForAdd(file_d, 1, false, md)) {
		file_d->SetPathAndVolume();
		SetMsgPar(CPath, LockModeTxt[md]);
		LockForAdd(file_d, 2, false, md);
		int w = PushWrLLMsg(825, false);
		KbdTimer(spec.NetDelay, 0);
		if (w != 0) PopW(w);
	}
	const bool result = RunAddUpdate(file_d, kind, old_record, false, nullptr, not_link_d, record);
	LockForAdd(file_d, 2, false, md);
	return result;
}

bool TestExitKey(WORD KeyCode, EdExitD* X)
{
	for (const EdExKeyD& key : X->Keys) {
		if (KeyCode == key.KeyCode) {
			EdBreak = key.Break;
			return true;
		}
	}
	return false;
}

void SetCompileAll()
{
	ChptTF->CompileAll = true;
	ChptTF->TimeStmp = Today() + CurrTime();
	ChptTF->SetUpdateFlag(); //SetUpdHandle(ChptTF->Handle);
}
