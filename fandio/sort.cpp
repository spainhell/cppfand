#include "sort.h"
#include <queue>

#include "XWKey.h"
#include "../CppFand/runfrml.h"
#include "../Logging/Logging.h"
#include "../CppFand/models/Instr.h"


void GetIndex(Instr_getindex* PD)
{
	XString x;
	FileD* lvFD = PD->giLV->FD;
	XWKey* k = (XWKey*)PD->giLV->RecPtr;

	BYTE* record = lvFD->GetRecSpace();

	LockMode md = lvFD->NewLockMode(RdMode);
	if (PD->giMode == ' ') {
		KeyFldD* kf = nullptr;
		LocVar* lv2 = nullptr;
		LinkD* ld = PD->giLD;
		if (ld != nullptr) {
			kf = ld->ToKey->KFlds;
		}
		if (PD != nullptr) {
			lv2 = PD->giLV2;
		}
		XScan* Scan = new XScan(lvFD, PD->giKD, PD->giKIRoot, false);
		FrmlElem* cond = RunEvalFrml(lvFD, PD->giCond, record);
		switch (PD->giOwnerTyp) {
		case 'i': {
			Scan->ResetOwnerIndex(ld, lv2, cond);
			break;
		}
		case 'r': {
			x.PackKF(ld->ToFD, kf, lv2->RecPtr);
			Scan->ResetOwner(&x, cond);
			break;
		}
		case 'F': {
			lvFD = ld->ToFD;
			md = ld->ToFD->NewLockMode(RdMode);
			ld->ToFD->ReadRec(RunInt(ld->ToFD, (FrmlElem*)PD->giLV2, record), record);
			x.PackKF(ld->ToFD, kf, record);
			ld->ToFD->OldLockMode(md);
			Scan->ResetOwner(&x, cond);
			break;
		}
		default: {
			Scan->Reset(cond, PD->giSQLFilter, record);
			break;
		}
		}
		kf = PD->giKFlds;
		if (kf == nullptr) kf = k->KFlds;
		XWKey* kNew = new XWKey(lvFD);
		kNew->Open(lvFD, kf, true, false);
		lvFD->FF->CreateWIndex(Scan, kNew, 'X');
		k->Close(lvFD);
		*k = *kNew;
	}
	else {
		int nr = RunInt(lvFD, PD->giCond, record);
		if ((nr > 0) && (nr <= lvFD->FF->NRecs)) {
			lvFD->ReadRec(nr, record);
			if (PD->giMode == '+') {
				if (!lvFD->DeletedFlag(record)) {
					x.PackKF(lvFD, k->KFlds, record);
					if (!k->RecNrToPath(lvFD, x, nr, record)) {
						k->InsertOnPath(lvFD, x, nr);
						k->NR++;
					}
				}
			}
			else {
				if (k->Delete(lvFD, nr, record)) {
					k->NR--;
				}
			}
		}
	}
	delete[] record; record = nullptr;
	lvFD->OldLockMode(md);
}

