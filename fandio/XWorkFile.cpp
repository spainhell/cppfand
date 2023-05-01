#include "XWorkFile.h"

#include "../fandio/FandXFile.h"
#include "../cppfand/access.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/obaseww.h"


XWorkFile::XWorkFile(FileD* parent, XScan* AScan, XKey* AK): WorkFile(parent)
{
	xScan = AScan;
	// CFile = xScan->FD;
	xKey = AK;
	xwFile = AK->GetXFile(parent);
}

void XWorkFile::Main(char Typ, void* record)
{
	xPage = new XPage();
	nextXPage = xwFile->NewPage(xPage);
	msgWritten = false;
	bool frst = true;
	// for all keys defined in #K
	while (xKey != nullptr) {
		xxPage = new XXPage();
		xxPage->Reset(this);
		xxPage->IsLeaf = true;
		XKey* k = xScan->Key;
		KeyFldD* kf = xKey->KFlds;
		if (xScan->Kind == 1 &&
#ifdef FandSQL
			!xScan->FD->IsSQLFile &&
#endif
			(xScan->Bool == nullptr && (kf == nullptr || KeyFldD::EquKFlds(k->KFlds, kf)))) {
			CopyIndex(k, kf, Typ, record);
		}
		else {
			if (frst) {
				frst = false;
			}
			else {
				xScan->SeekRec(0);
			}
			Reset(xKey->KFlds, sizeof(XXPage) * 9, Typ, xScan->NRecs);
			SortMerge(record);
		}
		FinishIndex();
		delete xxPage; xxPage = nullptr;
		xKey = xKey->Chain;
	}
	xwFile->ReleasePage(xPage, nextXPage);
	delete xPage; xPage = nullptr;
}

void XWorkFile::CopyIndex(XKey* K, KeyFldD* KF, char Typ, void* record)
{

	WRec* r = new WRec();
	// r->X.S = ""; pstring is always "" at the beginning
	XPage* p = new XPage();

	K->NrToPath(_parent, 1);
	int page = XPath[XPathN].Page;
	RunMsgOn(Typ, K->NRecs());
	int count = 0;

	while (page != 0) {
		K->GetXFile(_parent)->RdPage(p, page);
		for (size_t i = 1; i <= p->NItems; i++) {
			XItem* x = p->GetItem(i);
			r->PutN(x->GetN());
			if (KF == nullptr) {
				// x = x->Next();
			}
			else {
				std::string s = r->X.S;
				//x->UpdStr(&r->X.S);
				s = x->GetKey(s);
				r->X.S = s;
			}
			Output(r, record);
		}
		count += p->NItems;
		RunMsgN(count);
		page = p->GreaterPage;
	}
	RunMsgOff();
}

bool XWorkFile::GetCRec(void* record)
{
	bool result = false;
	xScan->GetRec(record);
	result = !xScan->eof;
	RecNr = xScan->RecNr;
	IRec = xScan->IRec;
	return result;
}

void XWorkFile::Output(WRec* R, void* record)
{
	xxPage->AddToLeaf(_parent, R, xKey, record);
}

void XWorkFile::FinishIndex()
{
	int sum = 0;
	int n = 0;
	XXPage* p = xxPage;

	while (true) {
		sum += p->Sum;
		p->ClearRest();
		p->GreaterPage = n;
		XXPage* p1 = p->Chain;
		if (p1 == nullptr) {
			n = xKey->IndexRoot;
		}
		else {
			n = nextXPage;
		}

		xwFile->WrPage(p, n);

		p = p1;
		if (p != nullptr) {
			nextXPage = xwFile->NewPage(xPage);
			continue;
		}

		break;
	}

	if (xKey->InWork) {
		xKey->NR = sum;
	}
	else {
		((FandXFile*)xwFile)->NRecs = sum;
	}
}