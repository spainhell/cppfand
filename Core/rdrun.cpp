#include "rdrun.h"

#include "Additive.h"
#include "LogicControl.h"
#include "../fandio/FieldDescr.h"
#include "../Common/FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "../Common/LinkD.h"
#include "obaseww.h"
#include "runfrml.h"
#include "../Common/DateTime.h"
#include "../Common/Record.h"


XString OldMXStr;					/* Merge */
bool Join;
bool PrintView;						/* Report */
TextFile Rprt;		// puvodne text - souvisi s text. souborem

std::vector<FrmlElemSum*> PFZeroLst;

bool SelQuest;
EditD* EditDRoot;
FrmlElem* PgeSizeZ = nullptr, * PgeLimitZ = nullptr;
bool CompileFD = false;
bool EditRdbMode = false;
LocVarBlock LVBD;
std::string CalcTxt;
MergOpSt MergOpGroup = { _const, 0.0 };

void ResetLVBD()
{
	//LVBD.pChain = nullptr;
	LVBD.variables.clear();
	LVBD.NParam = 0;
	LVBD.func_name = "";
}

bool Add(FileD* file_d, Additive* add_d, Record* record, double value, bool back)
{
	bool result = true;

	if (back) value = -value;

	//const double new_value = file_d->loadR(add_d->Field, record) + value;
	const double new_value = record->LoadR(add_d->Field) + value;
	//file_d->saveR(add_d->Field, new_value, record);
	record->SaveR(add_d->Field, new_value);

	if (add_d->Chk == nullptr) return result;

	if (!back && !RunBool(file_d, add_d->Chk->Bool, record)) {
		SetMsgPar(RunString(file_d, add_d->Chk->TxtZ, record));
		WrLLF10Msg(110);
		result = false;
	}

	return result;
}

void CrIndRec(FileD* file_d, Record* record)
{
	file_d->CreateRec(file_d->FF->NRecs + 1, record);
	file_d->RecallRec(file_d->FF->NRecs, record);
}

Record* Link(FileD* file_d, Additive* add_d, int& n, char& kind2, Record* record)
{
	// TODO: is param file_d needed?

	Record* result = nullptr;
	LinkD* ld = add_d->LD;
	kind2 = 'd';

	if (ld != nullptr) {
		Record* rec = LinkUpw(ld, n, false, record);

		if (rec != nullptr) {
			result = new Record(add_d->File2);
			//memcpy(result->GetRecord(), rec->GetRecord(), add_d->File2->GetRecordSize());
			rec->CopyTo(result);
			delete rec; rec = nullptr;
			return result;
		}

		SetMsgPar(ld->RoleName);
	}
	else {
		// cond. for FandSQL removed
		Record* r = add_d->File2->LinkLastRec(n);

		if (r == nullptr) {
			r = new Record(add_d->File2);
			file_d->IncNRecs(1);
			file_d->WriteRec(1, r);
		}

		result = new Record(add_d->File2);
		//memcpy(result->GetRecord(), r->GetRecord(), add_d->File2->GetRecordSize());
		r->CopyTo(result);
		delete r; r = nullptr;

		return result;
	}
	kind2 = '+';
	if ((add_d->Create == 2) || (add_d->Create == 1) && PromptYN(132)) {
		// cond. for FandSQL removed
		result->ClearDeleted();
		if ((ld != nullptr) && (file_d->FF->file_type == FandFileType::INDEX)) {
			CrIndRec(file_d, result);
			n = file_d->FF->NRecs;
		}
		else {
			file_d->CreateRec(n, result);
		}
		return result;
	}
	WrLLF10Msg(119);

	return result;
}

bool TransAdd(FileD* file_d, Additive* AD, FileD* FD, Record* RP, Record* new_record, int N, char Kind2, bool Back)
{
	if (file_d->Add.empty()) {
		return true;
	}
	if (Kind2 == '+') {
		return RunAddUpdate(file_d, '+', nullptr, Back, nullptr, nullptr, new_record);
	}
	Record* rec = new Record(file_d);
	// TODO: FandSQL condition removed
	file_d->ReadRec(N, rec);

	bool result = RunAddUpdate(file_d, 'd', rec, Back, nullptr, nullptr, new_record);

	delete rec; rec = nullptr;
	return result;
}

void WrUpdRec(FileD* file_d, Additive* add_d, FileD* fd, Record* rp, Record* new_record, int n)
{
	//XString x;
	//LinkD* ld;
	// TODO: FandSQL condition removed
	file_d->WriteRec(n, new_record);
}

bool Assign(FileD* file_d, Additive* add_d, Record* record)
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
		s = RunString(file_d, z, record);
		break;
	}
	default: {
		b = RunBool(file_d, z, record);
		break;
	}
	}

	Record* linked = Link(file_d, add_d, n2, kind2, record);

	if (linked == nullptr) {
		//delete[] linkedRecord; linkedRecord = nullptr;
		return false;
	}
	else {
		switch (f->frml_type) {
		case 'R': {
			//add_d->File2->saveR(f->Name, r, linked);
			linked->SaveR(f, r);
			break;
		}
		case 'S': {
			//add_d->File2->saveS(f->Name, s, linked);
			linked->SaveS(f, s);
			break;
		}
		default: {
			//add_d->File2->saveB(f->Name, b, linked);
			linked->SaveB(f, b);
			break;
		}
		}

		add_d->File2->WriteRec(n2, linked);

		delete linked; linked = nullptr;
		return true;
	}
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

bool RunAddUpdate(FileD* file_d, char kind, Record* old_record, LinkD* not_link_d, Record* record)
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

bool RunAddUpdate(FileD* file_d, char kind, Record* old_record, bool back, Additive* stop_add_d, LinkD* not_link_d, Record* record)
{
	Record* cr2 = nullptr;
	Record* cr2_old = nullptr;
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
				delete cr2; cr2 = nullptr;
				delete cr2_old; cr2_old = nullptr;
				continue;
			}
			if (add->Assign) {
				if (Assign(file_d, add, record)) {
					delete cr2; cr2 = nullptr;
					delete cr2_old; cr2_old = nullptr;
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
				Record* linked = Link(file_d, add, n2, kind2, record);
				if (linked == nullptr) {
					throw std::exception("fail");
				}
			}
			if (r_old != 0.0) {
				Record* linked = Link(file_d, add, n2_old, kind2_old, old_record);
				if (linked == nullptr) {
					throw std::exception("fail");
				}
				if (n2_old == n2) {
					r = r - r_old;
					if (r == 0.0) {
						delete cr2; cr2 = nullptr;
						delete cr2_old; cr2_old = nullptr;
						continue;
					}
					n2_old = 0;
				}
			}
			if ((n2 == 0) && (n2_old == 0)) {
				delete cr2; cr2 = nullptr;
				delete cr2_old; cr2_old = nullptr;
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

			delete cr2; cr2 = nullptr;
			delete cr2_old; cr2_old = nullptr;
		}
	}

	catch (std::exception&) {
		delete cr2; cr2 = nullptr;
		delete cr2_old; cr2_old = nullptr;
		result = false;
		if (add_d_back != nullptr) {
			bool b = RunAddUpdate(file_d, kind, old_record, true, add_d_back, not_link_d, record);  /* backtracking */
		}
	}

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
