#include "sort.h"
#include <queue>

#include "XWKey.h"
#include "../Core/LinkD.h"
#include "../Core/models/Instr.h"
#include "../Core/runfrml.h"
#include "../Logging/Logging.h"


int32_t GetIndex(Instr_getindex* PD)
{
	XString x;
	FileD* lvFD = PD->loc_var1->FD;
	XWKey* k = (XWKey*)PD->loc_var1->record;

	BYTE* record = lvFD->GetRecSpace();

	LockMode md = lvFD->NewLockMode(RdMode);
	if (PD->mode == ' ') {
		LocVar* lv2 = nullptr;
		LinkD* ld = PD->link;
		bool link_exists = PD->link != nullptr;

		if (PD != nullptr) {
			lv2 = PD->loc_var2;
		}

		std::unique_ptr<XScan> Scan = std::make_unique<XScan>(lvFD, PD->keys, PD->key_in_root, false);
		FrmlElem* cond = RunEvalFrml(lvFD, PD->condition, record);

		switch (PD->owner_type) {
		case 'i': {
			int32_t err_no = Scan->ResetOwnerIndex(ld, lv2, cond);
			if (err_no != 0) {
				return err_no;
			}
			break;
		}
		case 'r': {
			if (link_exists) {
				x.PackKF(ld->ToFD, ld->ToKey->KFlds, lv2->record);
			}
			else {
				//x.PackKF(ld->ToFD, nullptr, lv2->record);
				x.Clear();
			}

			Scan->ResetOwner(&x, cond);
			break;
		}
		case 'F': {
			lvFD = ld->ToFD;
			md = ld->ToFD->NewLockMode(RdMode);
			ld->ToFD->ReadRec(RunInt(ld->ToFD, (FrmlElem*)PD->loc_var2, record), record);

			if (link_exists) {
				x.PackKF(ld->ToFD, ld->ToKey->KFlds, lv2->record);
			}
			else {
				//x.PackKF(ld->ToFD, nullptr, lv2->record);
				x.Clear();
			}

			ld->ToFD->OldLockMode(md);
			Scan->ResetOwner(&x, cond);
			break;
		}
		default: {
			Scan->Reset(cond, PD->sql_filter, record);
			break;
		}
		}

		XWKey* kNew = new XWKey(lvFD);

		if (!PD->key_fields.empty()) {
			kNew->Open(lvFD, PD->key_fields, true, false);
		}
		else {
			kNew->Open(lvFD, k->KFlds, true, false);
		}

		lvFD->FF->CreateWIndex(Scan.get(), kNew, 'X');
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

