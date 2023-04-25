#include "sort.h"
#include <queue>

#include "XWorkFile.h"
#include "../cppfand/FieldDescr.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/KeyFldD.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/runfrml.h"
#include "../Logging/Logging.h"
#include "../cppfand/models/Instr.h"


void GetIndex(Instr_getindex* PD)
{
	XString x;
	const LocVar* lv = PD->giLV;
	FileD* file_d = lv->FD;
	XWKey* k = (XWKey*)lv->RecPtr;
	LockMode md = file_d->NewLockMode(RdMode);
	if (PD->giMode == ' ') {
		FrmlElem* cond = nullptr;
		KeyFldD* kf = nullptr;
		LocVar* lv2 = nullptr;
		LinkD* ld = PD->giLD;
		if (ld != nullptr) {
			kf = ld->ToKey->KFlds;
		}
		if (PD != nullptr) {
			lv2 = PD->giLV2;
		}
		XScan* Scan = new XScan(file_d, PD->giKD, PD->giKIRoot, false);
		cond = RunEvalFrml(PD->giCond);
		switch (PD->giOwnerTyp) {
		case 'i': {
			Scan->ResetOwnerIndex(ld, lv2, cond);
			break;
		}
		case 'r': {
			file_d = ld->ToFD;
			BYTE* record = (BYTE*)lv2->RecPtr;
			x.PackKF(kf, record);

			file_d = lv->FD;
			Scan->ResetOwner(&x, cond);
			break;
		}
		case 'F': {
			file_d = ld->ToFD;
			md = file_d->NewLockMode(RdMode);
			BYTE* record = file_d->GetRecSpace();
			file_d->ReadRec(RunInt((FrmlElem*)PD->giLV2), record);
			x.PackKF(kf, record);
			delete[] record; record = nullptr;
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
		XWKey* kNew = new XWKey(file_d);
		kNew->Open(file_d, kf, true, false);
		file_d->FF->CreateWIndex(Scan, kNew, 'X');
		k->Close(file_d);
		*k = *kNew;
	}
	else {
		BYTE* record = file_d->GetRecSpace();
		int nr = RunInt(PD->giCond);
		if ((nr > 0) && (nr <= file_d->FF->NRecs)) {
			file_d->ReadRec(nr, record);
			if (PD->giMode == '+') {
				if (!file_d->DeletedFlag(record)) {
					x.PackKF(k->KFlds, record);
					if (!k->RecNrToPath(file_d, x, nr, CRecPtr)) {
						k->InsertOnPath(file_d, x, nr);
						k->NR++;
					}
				}
			}
			else {
				if (k->Delete(file_d, nr, CRecPtr)) {
					k->NR--;
				}
			}
		}
		delete[] record; record = nullptr;
	}
	file_d->OldLockMode(md);
}

