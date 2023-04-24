#include "sort.h"
#include <queue>

#include "XWorkFile.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"
#include "../cppfand/runfrml.h"
#include "../Logging/Logging.h"
#include "../cppfand/models/Instr.h"

void ScanSubstWIndex(FileD* file_d, XScan* Scan, KeyFldD* SK, char Typ)
{
	unsigned short n = 0;
	XWKey* k2 = new XWKey(file_d);
	if (Scan->FD->IsSQLFile && (Scan->Kind == 3)) /*F6-autoreport & sort*/ {
		XKey* k = Scan->Key;
		n = k->IndexLen;
		KeyFldD* kf = SK;
		while (kf != nullptr) {
			n += kf->FldD->NBytes;
			kf = kf->pChain;
		}
		if (n > 255) {
			WrLLF10Msg(155);
			ReleaseStore(k2);
			return;
		}
		kf = k->KFlds;
		KeyFldD* kfroot = nullptr;
		KeyFldD* kf2 = nullptr;
		while (kf != nullptr) {
			kf2 = new KeyFldD();
			*kf2 = *kf;
			ChainLast(kfroot, kf2);
			kf = kf->pChain;
		}
		if (kf2 != nullptr)	kf2->pChain = SK;
		SK = kfroot;
	}
	k2->Open(CFile, SK, true, false);
	file_d->FF->CreateWIndex(Scan, k2, Typ);

	Scan->SubstWIndex(k2);
}

void SortAndSubst(FileD* file_d, KeyFldD* SK)
{
	BYTE* record = file_d->GetRecSpace();

	XScan* Scan = new XScan(file_d, nullptr, nullptr, false);
	Scan->Reset(nullptr, false);
	ScanSubstWIndex(file_d, Scan, SK, 'S');
	FileD* FD2 = OpenDuplicateF(file_d, false);
	RunMsgOn('S', Scan->NRecs);
	Scan->GetRec(record);

	// zapiseme data do souboru .100
	FD2->FF->GenerateNew000File(Scan, record);

	SubstDuplF(FD2, false);
	Scan->Close();
	RunMsgOff();

	delete[] record; record = nullptr;
}

void GetIndexSort(Instr_getindex* PD)
{
	XScan* Scan = nullptr;
	void* p = nullptr;
	LocVar* lv2 = nullptr;
	XWKey* kNew = nullptr;
	int nr = 0;
	LockMode md1;
	FrmlElem* cond = nullptr;
	LinkD* ld = nullptr;
	KeyFldD* kf = nullptr;
	XString x;
	MarkStore(p);
	LocVar* lv = PD->giLV;
	FileD* file_d = lv->FD;
	XWKey* k = (XWKey*)lv->RecPtr;
	LockMode md = file_d->NewLockMode(RdMode);
	if (PD->giMode == ' ') {
		ld = PD->giLD;
		if (ld != nullptr) kf = ld->ToKey->KFlds;
		if (PD != nullptr) lv2 = PD->giLV2;
		Scan = new XScan(file_d, PD->giKD, PD->giKIRoot, false);
		cond = RunEvalFrml(PD->giCond);
		switch (PD->giOwnerTyp) {
		case 'i': {
			Scan->ResetOwnerIndex(ld, lv2, cond);
			break;
		}
		case 'r': {
			file_d = ld->ToFD;
			CRecPtr = lv2->RecPtr;
			x.PackKF(kf);

			file_d = lv->FD;
			Scan->ResetOwner(&x, cond);
			break;
		}
		case 'F': {
			file_d = ld->ToFD;
			md = file_d->NewLockMode(RdMode);
			CRecPtr = file_d->GetRecSpace();
			file_d->ReadRec(RunInt((FrmlElem*)PD->giLV2), CRecPtr);
			x.PackKF(kf);
			ReleaseStore(CRecPtr);
			file_d->OldLockMode(md);

			file_d = lv->FD;
			Scan->ResetOwner(&x, cond);
			break;
		}
		default: {
			Scan->Reset(cond, PD->giSQLFilter);
			break;
		}
		}
		kf = PD->giKFlds;
		if (kf == nullptr) kf = k->KFlds;
		kNew = new XWKey(file_d);
		kNew->Open(file_d, kf, true, false);
		file_d->FF->CreateWIndex(Scan, kNew, 'X');
		k->Close(file_d);
		*k = *kNew;
	}
	else {
		CRecPtr = file_d->GetRecSpace();
		nr = RunInt(PD->giCond);
		if ((nr > 0) && (nr <= file_d->FF->NRecs)) {
			file_d->ReadRec(nr, CRecPtr);
			if (PD->giMode == '+') {
				if (!file_d->DeletedFlag(CRecPtr)) {
					x.PackKF(k->KFlds);
					if (!k->RecNrToPath(file_d, x, nr)) {
						k->InsertOnPath(file_d, x, nr);
						k->NR++;
					}
				}
			}
			else {
				if (k->Delete(file_d, nr)) {
					k->NR--;
				}
			}
		}
	}
	file_d->OldLockMode(md);
	ReleaseStore(p);
}

void CopyIndex(FileD* file_d, XWKey* K, XKey* FromK)
{
	XScan* Scan = nullptr;
	K->Release(file_d);
	LockMode md = file_d->NewLockMode(RdMode);
	Scan = new XScan(file_d, FromK, nullptr, false);
	Scan->Reset(nullptr, false);
	file_d->FF->CreateWIndex(Scan, K, 'W');
	file_d->OldLockMode(md);
}
