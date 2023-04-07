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


void CreateWIndex(XScan* Scan, XWKey* K, char Typ)
{
	void* cr = CRecPtr;
	CRecPtr = CFile->GetRecSpace();
	XWorkFile* XW = new XWorkFile(Scan, K);
	XW->Main(Typ);
	delete XW; XW = nullptr;
	CRecPtr = cr;
}

void ScanSubstWIndex(XScan* Scan, KeyFldD* SK, char Typ)
{
	unsigned short n = 0;
	XWKey* k2 = new XWKey();
	if (Scan->FD->IsSQLFile && (Scan->Kind == 3)) /*F6-autoreport & sort*/ {
		XKey* k = Scan->Key;
		n = k->IndexLen;
		KeyFldD* kf = SK;
		while (kf != nullptr) {
			n += kf->FldD->NBytes;
			kf = (KeyFldD*)kf->pChain;
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
			//kf2 = (KeyFldD*)GetStore(sizeof(KeyFldD));
			kf2 = new KeyFldD();
			*kf2 = *kf;
			ChainLast(kfroot, kf2);
			kf = (KeyFldD*)kf->pChain;
		}
		if (kf2 != nullptr)	kf2->pChain = SK;
		SK = kfroot;
	}
	k2->Open(SK, true, false);
	CreateWIndex(Scan, k2, Typ);

	Scan->SubstWIndex(k2);
}

void GenerateNew000File(FileD* f, XScan* x)
{
	// vytvorime si novy buffer pro data,
	// ten pak zapiseme do souboru naprimo (bez cache)

	const unsigned short header000len = 6; // 4B pocet zaznamu, 2B delka 1 zaznamu
	// z puvodniho .000 vycteme pocet zaznamu a jejich delku
	const size_t totalLen = x->FD->FF->NRecs * x->FD->FF->RecLen + header000len;
	unsigned char* buffer = new unsigned char[totalLen]{ 0 };
	size_t offset = header000len; // zapisujeme nejdriv data; hlavicku az nakonec
	
	while (!x->eof) {
		RunMsgN(x->IRec);
		f->FF->NRecs++;
		memcpy(&buffer[offset], CRecPtr, f->FF->RecLen);
		offset += f->FF->RecLen;
		f->IRec++;
		f->FF->Eof = true;
		x->GetRec();
	}

	// zapiseme hlavicku
	memcpy(&buffer[0], &f->FF->NRecs, 4);
	memcpy(&buffer[4], &f->FF->RecLen, 2);

	// provedeme primy zapis do souboru
	WriteH(f->FF->Handle, totalLen, buffer);

	delete[] buffer; buffer = nullptr;
}

void SortAndSubst(KeyFldD* SK)
{
	void* p = nullptr;
	MarkStore(p);
	FileD* cf = CFile;
	CRecPtr = CFile->GetRecSpace();
	XScan* Scan = new XScan(CFile, nullptr, nullptr, false);
	Scan->Reset(nullptr, false);
	ScanSubstWIndex(Scan, SK, 'S');
	FileD* FD2 = OpenDuplF(false);
	RunMsgOn('S', Scan->NRecs);
	Scan->GetRec();

	// zapiseme data do souboru .100
	GenerateNew000File(FD2, Scan);
	//while (!xScan->eof) {
	//	RunMsgN(xScan->IRec);
	//	CFile = FD2;
	//	PutRec();
	//	xScan->GetRec();
	//}
	//if (!SaveCache(0, CFile->Handle)) GoExit();

	CFile = cf;
	SubstDuplF(FD2, false);
	Scan->Close();
	RunMsgOff();
	ReleaseStore(p);
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
	CFile = lv->FD;
	XWKey* k = (XWKey*)lv->RecPtr;
	LockMode md = NewLMode(CFile, RdMode);
	if (PD->giMode == ' ') {
		ld = PD->giLD;
		if (ld != nullptr) kf = ld->ToKey->KFlds;
		if (PD != nullptr) lv2 = PD->giLV2;
		Scan = new XScan(CFile, PD->giKD, PD->giKIRoot, false);
		cond = RunEvalFrml(PD->giCond);
		switch (PD->giOwnerTyp) {
		case 'i': {
			Scan->ResetOwnerIndex(ld, lv2, cond);
			break;
		}
		case 'r': {
			CFile = ld->ToFD;
			CRecPtr = lv2->RecPtr;
			x.PackKF(kf);

			CFile = lv->FD;
			Scan->ResetOwner(&x, cond);
			break;
		}
		case 'F': {
			CFile = ld->ToFD;
			md = NewLMode(CFile, RdMode);
			CRecPtr = CFile->GetRecSpace();
			CFile->ReadRec(RunInt((FrmlElem*)PD->giLV2), CRecPtr);
			x.PackKF(kf);
			ReleaseStore(CRecPtr);
			OldLMode(CFile, md);

			CFile = lv->FD;
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
		kNew = new XWKey();
		kNew->Open(kf, true, false);
		CreateWIndex(Scan, kNew, 'X');
		k->Close();
		*k = *kNew;
	}
	else {
		CRecPtr = CFile->GetRecSpace();
		nr = RunInt(PD->giCond);
		if ((nr > 0) && (nr <= CFile->FF->NRecs)) {
			CFile->ReadRec(nr, CRecPtr);
			if (PD->giMode == '+') {
				if (!DeletedFlag(CFile->FF, CRecPtr)) {
					x.PackKF(k->KFlds);
					if (!k->RecNrToPath(x, nr)) {
						k->InsertOnPath(x, nr);
						k->NR++;
					}
				}
			}
			else {
				if (k->Delete(nr)) {
					k->NR--;
				}
			}
		}
	}
	OldLMode(CFile, md);
	ReleaseStore(p);
}

void CopyIndex(XWKey* K, XKey* FromK)
{
	XScan* Scan = nullptr;
	K->Release();
	LockMode md = NewLMode(CFile, RdMode);
		Scan = new XScan(CFile, FromK, nullptr, false);
	Scan->Reset(nullptr, false);
	CreateWIndex(Scan, K, 'W');
	OldLMode(CFile, md);
}
