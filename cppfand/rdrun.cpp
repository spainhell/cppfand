#include "rdrun.h"

#include "AddD.h"
#include "ChkD.h"
#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "legacy.h"
#include "oaccess.h"
#include "obaseww.h"
#include "runfrml.h"
#include "models/Instr.h"
#include "../fandio/files.h"

std::vector<ConstListEl> OldMFlds;
std::vector<ConstListEl> NewMFlds;   /* Merge + Report*/
InpD* IDA[30];
short MaxIi;
XString OldMXStr;                  /* Merge */
OutpFD* OutpFDRoot;
OutpRD* OutpRDs;
bool Join;
bool PrintView;                  /* Report */
TextFile Rprt;		// puvodne text - souvisi s text. souborem
BlkD* RprtHd;
BlkD* PageHd;
BlkD* PageFt;
std::vector<FrmlElemSum*> PFZeroLst;
LvDescr* FrstLvM;
LvDescr* LstLvM; /* LstLvM->Ft=RF */
bool SelQuest;
EditD* EditDRoot;
FrmlElem* PgeSizeZ = nullptr, * PgeLimitZ = nullptr;
bool CompileFD, EditRdbMode;
LocVarBlkD LVBD;
std::string CalcTxt = "";
MergOpSt MergOpGroup = { _const, 0.0 };

bool EFldD::Ed(bool IsNewRec)
{
	return ((FldD->Flg & f_Stored) != 0) && (EdU || IsNewRec && EdN);
}

Instr::Instr(PInstrCode kind)
{
	this->Kind = kind;
}

void ResetLVBD()
{
	LVBD.pChain = nullptr;
	LVBD.vLocVar.clear();
	LVBD.NParam = 0;
	LVBD.Size = 2 * 4;
	LVBD.FceName = "";
}

bool Add(FileD* file_d, AddD* add_d, void* record, double value, bool back)
{
	bool result = true;

	if (back) value = -value;

	const double new_value = file_d->loadR(add_d->Field, record) + value;
	file_d->saveR(add_d->Field, new_value, record);

	if (add_d->Chk == nullptr) return result;

	if (!back && !RunBool(file_d, add_d->Chk->Bool, record)) {
		SetMsgPar(RunShortStr(file_d, add_d->Chk->TxtZ, record));
		WrLLF10Msg(110);
		result = false;
	}

	return result;
}

bool RunAddUpdte1(char kind, void* old_record, bool back, AddD* stop_add_d, LinkD* not_link_d)
{
	int n2, n2_old;
	char kind2, kind2_old;
	FileD* cf2 = nullptr;
	BYTE* cr2 = nullptr;
	BYTE* cr2_old = nullptr;
	void* p = nullptr;
	bool b;
	bool result = true;
	double r;
	double r_old;
	FileD* originalCFile = CFile;
	void* originalCRecPtr = CRecPtr;
	MarkStore(p);
	AddD* ADback = nullptr;

	for (AddD* add : CFile->Add) {
		if (add == stop_add_d) {
			ReleaseStore(&p);
			return result;
		}
		if ((not_link_d != nullptr) && (add->LD == not_link_d)) {
			goto label1;
		}
		if (add->Assign) {
			if (Assign(CFile, add, CRecPtr)) {
				goto label1;
			}
			else {
				goto fail;
			}
		}

		r = RunReal(CFile, add->Frml, CRecPtr);
		if (kind == '-') {
			r = -r;
		}
		r_old = 0;
		if (kind == 'd') {
			CRecPtr = old_record;
			r_old = RunReal(CFile, add->Frml, CRecPtr);
		}
		ADback = add;
		cf2 = add->File2;
		n2 = 0;
		n2_old = 0;
		if (r != 0.0) {
			CRecPtr = originalCRecPtr;
			if (!Link(CFile, add, n2, kind2, originalCRecPtr, &cr2)) {
				goto fail;
			}
			//CR2 = (BYTE*)CRecPtr;
		}
		if (r_old != 0.0) {
			CFile = originalCFile;
			CRecPtr = old_record;
			if (!Link(originalCFile, add, n2_old, kind2_old, old_record, &cr2_old)) {
				goto fail;
			}
			//CR2old = (BYTE*)CRecPtr;
			if (n2_old == n2) {
				r = r - r_old;
				if (r == 0.0) {
					goto label1;
				}
				n2_old = 0;
			}
		}
		if ((n2 == 0) && (n2_old == 0)) {
			goto label1;
		}
		CFile = cf2;
		if (n2_old != 0) {
			if (!Add(CFile, add, cr2_old, -r_old, back)) {
				goto fail;
			}
		}
		if (n2 != 0) {
			if (!Add(CFile, add, cr2, r, back)) {
				goto fail;
			}
		}
		if ((n2_old != 0) && !TransAdd(CFile, add, originalCFile, originalCRecPtr, cr2_old, n2_old, kind2_old, false)) {
			goto fail;
		}
		if ((n2 != 0) && !TransAdd(CFile, add, originalCFile, originalCRecPtr, cr2, n2, kind2, false)) {
			if (n2_old != 0) {
				b = TransAdd(CFile, add, originalCFile, originalCRecPtr, cr2_old, n2_old, kind2_old, true);
			}
			goto fail;
		}
		if (n2_old != 0) {
			WrUpdRec(CFile, add, originalCFile, originalCRecPtr, cr2_old, n2_old);
		}
		if (n2 != 0) {
			WrUpdRec(CFile, add, originalCFile, originalCRecPtr, cr2, n2);
		}
	label1:
		ReleaseStore(&p);
		delete[] cr2; cr2 = nullptr;
		delete[] cr2_old; cr2_old = nullptr;
		CFile = originalCFile;
		CRecPtr = originalCRecPtr;
	}
	return result;
fail:
	ReleaseStore(&p);
	delete[] cr2; cr2 = nullptr;
	delete[] cr2_old; cr2_old = nullptr;
	CFile = originalCFile;
	CRecPtr = originalCRecPtr;
	result = false;
	if (ADback != nullptr) {
		b = RunAddUpdte1(kind, old_record, true, ADback, not_link_d);  /* backtracking */
	}
	return result;
}

void CrIndRec(FileD* file_d, void* record)
{
	file_d->CreateRec(file_d->FF->NRecs + 1, record);
	file_d->RecallRec(file_d->FF->NRecs, record);
}

bool Link(FileD* file_d, AddD* add_d, int& n, char& kind2, void* record, BYTE** linkedRecord)
{
	bool result = true;
	LinkD* ld = add_d->LD;
	kind2 = 'd';

	if (ld != nullptr) {
		if (LinkUpw(file_d, ld, n, false, record, linkedRecord)) {
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
			if ((ld != nullptr) && (file_d->FF->file_type == FileType::INDEX)) {
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

bool TransAdd(FileD* file_d, AddD* AD, FileD* FD, void* RP, void* new_record, int N, char Kind2, bool Back)
{
	XString x;
	LinkD* ld;
	if (file_d->Add.empty()) {
		return true;
	}
	if (Kind2 == '+') {
		CRecPtr = new_record;
		return RunAddUpdte1('+', nullptr, Back, nullptr, nullptr);
	}
	BYTE* rec = file_d->GetRecSpace();
	CRecPtr = rec;
#ifdef FandSQL
	// TODO: cele pripadne predelat, po refactoringu uz to nesedi
	if (CFile->IsSQLFile) {
		ld = AD->LD; if (ld == nullptr) Strm1->SelectXRec(nullptr, nullptr, _equ, false)
		else {
			CFile = FD; CRecPtr = RP; x.PackKF(ld->Args);
			CFile = ld->ToFD; CRecPtr = CRold; Strm1->SelectXRec(ld->ToKey, @x, _equ, false)
		}
	}
	else
#endif
		file_d->ReadRec(N, rec);
	CRecPtr = new_record;
	bool result = RunAddUpdte1('d', rec, Back, nullptr, nullptr);

	delete[] rec; rec = nullptr;
	return result;
}

void WrUpdRec(FileD* file_d, AddD* add_d, FileD* fd, void* rp, void* new_record, int n)
{
	//XString x;
	//LinkD* ld;
	CRecPtr = new_record; // TODO: pro jistotu, mozno asi odstranit
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

bool Assign(FileD* file_d, AddD* add_d, void* record)
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
			s = RunStdStr(file_d, z, record);
		}
		else {
			s = RunShortStr(file_d, z, record);
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
		file_d->saveR(f, r, linkedRecord);
		break;
	}
	case 'S': {
		if (f->field_type == FieldType::TEXT) {
			file_d->saveS(f, s, linkedRecord);
		}
		else {
			file_d->saveS(f, s, linkedRecord);
		}
		break;
	}
	default: {
		file_d->saveB(f, b, linkedRecord);
		break;
	}
	}
	file_d->WriteRec(n2, linkedRecord);

	delete[] linkedRecord; linkedRecord = nullptr;
	return true;
}

bool LockForAdd(FileD* FD, WORD Kind, bool Ta, LockMode& md)
{
	LockMode md1; /*0-ExLMode,1-lock,2-unlock*/
	auto result = false;
	CFile = FD;
	for (AddD* AD : FD->Add) {
		/* !!! with AD^ do!!! */
		if (CFile != AD->File2) {
			CFile = AD->File2;
			switch (Kind) {
			case 0: {
				if (Ta) CFile->FF->TaLMode = CFile->FF->LMode;
				else CFile->FF->ExLMode = CFile->FF->LMode;
				break;
			}
			case 1: {
				md = WrMode;
				if (AD->Create > 0) md = CrMode;
				if (!CFile->TryLockMode(md, md1, 2)) return result;
				break;
			}
			case 2: {
				if (Ta) {
					CFile->OldLockMode(CFile->FF->TaLMode);
				}
				else {
					CFile->OldLockMode(CFile->FF->ExLMode);
				}
				break;
			}
			}
			if (!LockForAdd(CFile, Kind, Ta, md)) return result;
		}
	}
	result = true;
	return result;
}

bool RunAddUpdte(char Kind, void* CRold, LinkD* notLD)
{
	LockMode md;
	FileD* CF = CFile;
	LockForAdd(CF, 0, false, md);
	while (!LockForAdd(CF, 1, false, md)) {
		SetPathAndVolume(CFile);
		SetMsgPar(CPath, LockModeTxt[md]);
		LockForAdd(CF, 2, false, md);
		int w = PushWrLLMsg(825, false);
		KbdTimer(spec.NetDelay, 0);
		if (w != 0) PopW(w);
	}
	CFile = CF;
	bool result = RunAddUpdte1(Kind, CRold, false, nullptr, notLD);
	LockForAdd(CF, 2, false, md);
	CFile = CF;
	return result;
}

bool TestExitKey(WORD KeyCode, EdExitD* X)
{
	for (auto& key : X->Keys) {
		if (KeyCode == key.KeyCode) {
			EdBreak = key.Break;
			return true;
		}
	}
	return false;

	/*EdExKeyD* E = X->Keys;
	while (E != nullptr) {
		if (KeyCode == E->KeyCode) {
			EdBreak = E->Break;
			return true;
		}
		E = E->pChain;
	}
	return false;*/
}

void SetCompileAll()
{
	/* !!! with ChptTF^ do!!! */
	ChptTF->CompileAll = true;
	ChptTF->TimeStmp = Today() + CurrTime();
	SetUpdHandle(ChptTF->Handle);
}
