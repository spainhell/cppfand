#include "sort.h"
#include <queue>

#include "XWKey.h"
#include "../Core/runfrml.h"
#include "../Logging/Logging.h"
#include "../Core/models/Instr.h"


void GetIndex(Instr_getindex* PD)
{
	XString x;
	FileD* lvFD = PD->loc_var1->FD;
	XWKey* k = (XWKey*)PD->loc_var1->RecPtr;

	BYTE* record = lvFD->GetRecSpace();

	LockMode md = lvFD->NewLockMode(RdMode);
	if (PD->mode == ' ') {
		KeyFldD* kf = nullptr;
		LocVar* lv2 = nullptr;
		LinkD* ld = PD->link;
		if (ld != nullptr) {
			kf = ld->ToKey->KFlds;
		}
		if (PD != nullptr) {
			lv2 = PD->loc_var2;
		}
		XScan* Scan = new XScan(lvFD, PD->keys, PD->key_in_root, false);
		FrmlElem* cond = RunEvalFrml(lvFD, PD->condition, record);
		switch (PD->owner_type) {
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
			ld->ToFD->ReadRec(RunInt(ld->ToFD, (FrmlElem*)PD->loc_var2, record), record);
			x.PackKF(ld->ToFD, kf, record);
			ld->ToFD->OldLockMode(md);
			Scan->ResetOwner(&x, cond);
			break;
		}
		default: {
			Scan->Reset(cond, PD->sql_filter, record);
			break;
		}
		}
		kf = PD->key_fields;
		if (kf == nullptr) {
			kf = k->KFlds;
		}
		XWKey* kNew = new XWKey(lvFD);
		kNew->Open(lvFD, kf, true, false);
		lvFD->FF->CreateWIndex(Scan, kNew, 'X');
		k->Close(lvFD);
		*k = *kNew;
	}
	else {
		int nr = RunInt(lvFD, PD->condition, record);
		if ((nr > 0) && (nr <= lvFD->FF->NRecs)) {
			lvFD->ReadRec(nr, record);
			if (PD->mode == '+') {
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

