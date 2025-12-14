#include "XWorkFile.h"

#include "../fandio/FandXFile.h"
#include "../Core/access.h"
#include "KeyFldD.h"
#include "../Common/Record.h"
#include "../Core/GlobalVariables.h"
#include "../Core/obaseww.h"
#include "../Core/RunMessage.h"


XWorkFile::XWorkFile(FileD* parent, XScan* AScan, std::vector<XKey*>& AK)
{
	_parent = parent;
	WBaseSize = MaxWSize;
	Handle = WorkHandle;
	xScan = AScan;
	x_keys_ = AK;
	xwFile = AK[0]->GetXFile(parent);
}

XWorkFile::~XWorkFile()
{
	MaxWSize = WBaseSize;
	TruncF(Handle, HandleError, MaxWSize);
	FlushF(Handle, HandleError);
}

void XWorkFile::Main(OperationType oper_type, Record* record)
{
	xPage = new XPage();
	nextXPage = xwFile->NewPage(xPage);
	msgWritten = false;
	bool frst = true;

	// for all keys defined in #K
	for (XKey* xKey : x_keys_) {
		xxPage = new XXPage();
		xxPage->Reset(this);
		xxPage->IsLeaf = true;
		XKey* k = xScan->Key;

		if (xScan->Kind == ScanMode::Index &&
#ifdef FandSQL
			!xScan->v_files->IsSQLFile &&
#endif
			(xScan->Bool == nullptr && (xKey->KFlds.empty() || KeyFldD::EquKFlds(k->KFlds, xKey->KFlds)))) {
			CopyIndex(k, xKey->KFlds, oper_type, record);
		}
		else {
			if (frst) {
				frst = false;
			}
			else {
				xScan->SeekRec(0);
			}
			Reset(xKey->KFlds, sizeof(XXPage) * 9, oper_type, xScan->NRecs);
			SortMerge(xKey, record);
		}

		FinishIndex(xKey);
		delete xxPage; xxPage = nullptr;
	}
	xwFile->ReleasePage(xPage, nextXPage);
	delete xPage; xPage = nullptr;
}

void XWorkFile::CopyIndex(XKey* K, std::vector<KeyFldD*>& KF, OperationType oper_type, Record* record)
{

	WRec* r = new WRec();
	// r->X.S = ""; pstring is always "" at the beginning
	XPage* p = new XPage();

	K->NrToPath(_parent, 1);
	int page = XPath[XPathN].Page;
	RunMsgOn(static_cast<char>(oper_type), K->NRecs());
	int count = 0;

	while (page != 0) {
		K->GetXFile(_parent)->RdPage(p, page);
		for (size_t i = 1; i <= p->NItems; i++) {
			XItem* x = p->GetItem(i);
			r->PutN(x->GetN());
			if (KF.empty()) {
				// x = x->Next();
			}
			else {
				std::string s = r->X.S;
				//x->UpdStr(&r->X.S);
				s = x->GetKey(s);
				r->X.S = s;
			}
			Output(K, r, record);
		}
		count += p->NItems;
		RunMsgN(count);
		page = p->GreaterPage;
	}
	RunMsgOff();
}

bool XWorkFile::GetCRec(Record* record)
{
	bool result = false;
	xScan->GetRec(record);
	result = !xScan->eof;
	RecNr = xScan->RecNr;
	IRec = xScan->IRec;
	return result;
}

void XWorkFile::Output(XKey* xKey, WRec* R, Record* record)
{
	xxPage->AddToLeaf(_parent, R, xKey, record);
}

void XWorkFile::Reset(std::vector<KeyFldD*>& KF, int RestBytes, OperationType oper_type, int NRecs)
{
	int BYTEs = 0;
	int pages = 0;
	const unsigned short kB60 = 0x0F000;
	KFRoot = KF;
	RecLen = 7;

	//while (KF != nullptr) {
	for (KeyFldD* k : KF) {
		if (oper_type == OperationType::Duplicate) {
			RecLen += 6;
		}
		else {
			if (k->FldD != nullptr) {
				RecLen += k->FldD->NBytes;
			}
		}
		//KF = KF->pChain;
	}

	//BYTEs = (MemoryAvailable() - RestBytes - sizeof(WRec)) / 3;

	//if (BYTEs < 4096) {
	//	RunError(624);
	//}

	//if (BYTEs < kB60) {
	//	WPageSize = (unsigned short)BYTEs & 0xF000;
	//}
	//else {
	WPageSize = kB60;
	//}

	// MaxOnWPage = (WPageSize - (sizeof(WPage) - 65535 + 1)) / RecLen; // nebude se do toho pocitat delka pole 'A' (66535)
	MaxOnWPage = (WPageSize - 10 + 1) / RecLen; // 10B is size of WPage without array
	if (MaxOnWPage < 4) {
		RunError(624);
	}
	MaxWPage = 0;
	NFreeNr = 0;
	PW = new WPage();
	WRoot = GetFreeNr();
	pages = (NRecs + MaxOnWPage - 1) / MaxOnWPage;

	// nasledujici usek v ASM zrejme jen udela nejaky prepocet kvuli zobrazeni zpravy
	// v RunMsgOn()
	//asm mov dx, pages.unsigned short; mov bx, dx; mov cx, dx; mov ax, 1;
	//@add cx 1, dx; test bx, 1; jz @2; sub cx, ax; shr bx, 1; add bx, 1; jmp @3;
	//@shr bx 2, 1;            /* how many pages must be written ? */
	//@shl ax 3, 1; cmp bx, 1; ja @1; cmp cx, 0; jne @4; mov cx, 1;
	//@mov pages 4.unsigned short, cx;

	RunMsgOn(static_cast<char>(oper_type), pages);
}

/// precte zaznamy, vytvori uplnou delku klice, setridi zaznamy
void XWorkFile::SortMerge(XKey* xKey, Record* record)
{// z 'PW->A' se postupne berou jednotlive WRec;
	// ty pak projdou upravou;
	// a zpatky se vraci na stejne misto do PW->A;

	//WRec* r = new WRec();
	unsigned char buffer[512]{ 0 };
	size_t offsetOfPwA = 0;
	unsigned short n = 0; int pg = 0, nxt = 0;
	PgWritten = 0;
	nxt = WRoot;
	NChains = 1;
	while (GetCRec(record)) {
		if (n == MaxOnWPage) {
			PW->Sort(n, RecLen);
			pg = nxt;
			nxt = GetFreeNr();
			NChains++;
			WriteWPage(xKey, n, pg, nxt, 0, record);
			memset(PW->A, '0', sizeof(PW->A));
			offsetOfPwA = 0;
			n = 0;
		}
		auto r = std::make_unique<WRec>();
		r->PutN(RecNr);
		r->PutIR(IRec);
		r->X.PackKF(_parent, KFRoot, record);
		size_t serLen = r->Serialize(buffer);
		memcpy(&PW->A[offsetOfPwA], buffer, serLen);
		n++;
		offsetOfPwA += RecLen;
	}
	PW->Sort(n, RecLen);
	WriteWPage(xKey, n, nxt, 0, 0, record);
	if (NChains > 1) {
		Merge(xKey, record);
	}
	RunMsgOff();
}

void XWorkFile::TestErr()
{
	if (HandleError != 0) {
		SetMsgPar(FandWorkName);
		RunError(700 + HandleError);
	}
}

void XWorkFile::FinishIndex(XKey* xKey)
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
		xwFile->NRecs = sum;
	}
}

int XWorkFile::GetFreeNr()
{
	int result = 0;
	if (NFreeNr > 0) {
		result = FreeNr[NFreeNr];
		NFreeNr--;
	}
	else {
		MaxWPage++;
		MaxWSize += WPageSize;
		result = MaxWPage;
	}
	return result;
}

void XWorkFile::ReadWPage(WPage* W, int Pg)
{
	SeekH(Handle, WBaseSize + (Pg - 1) * WPageSize);
	ReadH(Handle, 4, &W->NxtChain);
	ReadH(Handle, 4, &W->Chain);
	ReadH(Handle, 2, &W->NRecs);
	ReadH(Handle, WPageSize - 10, &W->A);
	TestErr();
}

void XWorkFile::WriteWPage(XKey* xKey, unsigned short N, int Pg, int Nxt, int Chn, Record* record)
{
	size_t offset = 0;
	PgWritten++;
	RunMsgN(PgWritten);
	if (NChains == 1) {
		for (size_t i = 0; i < N; i++) {
			WRec r;
			r.Deserialize(&PW->A[offset]);
			Output(xKey, &r, record);
			unsigned char buffer[512]{ 0 };
			size_t len = r.Serialize(buffer);
			memcpy(&PW->A[offset], buffer, len);
			offset += RecLen;
		}
	}
	else {
		PW->NRecs = N;
		PW->NxtChain = Nxt;
		PW->Chain = Chn;
		SeekH(Handle, WBaseSize + (Pg - 1) * WPageSize);
		WriteH(Handle, 4, &PW->NxtChain);
		WriteH(Handle, 4, &PW->Chain);
		WriteH(Handle, 2, &PW->NRecs);
		WriteH(Handle, WPageSize - 10, &PW->A);
		TestErr();
	}
}

void XWorkFile::Merge(XKey* xKey, Record* record)
{
	int npairs, newli, nxtnew, pg1, pg2;
	int nxt = 0;
	PW1 = new WPage();
	PW2 = new WPage();
label1:
	if (NChains == 1) return;
	npairs = NChains / 2;
	pg1 = WRoot;
	if (NChains == 2) newli = 0;
	else {
		WRoot = GetFreeNr();
		newli = WRoot;
	}
	if (nxt > 0) {
		pg2 = pg1; pg1 = nxt;
		ReadWPage(PW1, pg1);
		goto label2;
	}
	while (npairs > 0) {
		ReadWPage(PW1, pg1);
		pg2 = PW1->NxtChain;
	label2:
		ReadWPage(PW2, pg2);
		nxt = PW2->NxtChain;
		nxtnew = (npairs == 1) ? 0 : GetFreeNr();
		NChains--;
		Merge2Chains(xKey, pg1, pg2, newli, nxtnew, record);
		npairs--;
		pg1 = nxt;
		newli = nxtnew;
	}
	goto label1;
}

void XWorkFile::Merge2Chains(XKey* xKey, int Pg1, int Pg2, int Pg, int Nxt, Record* record)
{
	bool eof1 = false;
	bool eof2 = false;

	size_t r1ofs = 0;
	size_t r2ofs = 0;
	size_t rofs = 0;

	WRec* r1 = nullptr;
	WRec* r2 = nullptr;
	WRec* r = new WRec(&PW->A[rofs]);

	unsigned short max1ofs = PW1->NRecs * RecLen;
	unsigned short max2ofs = PW2->NRecs * RecLen;
	const unsigned short maxofs = MaxOnWPage * RecLen;

	while (true) {
		if (rofs == maxofs) {
			int chn = GetFreeNr();
			WriteWPage(xKey, MaxOnWPage, Pg, Nxt, chn, record);
			Pg = chn;
			Nxt = 0;
			delete r;
			rofs = 0;
			r = new WRec(&PW->A[rofs]);
		}
		if (eof1) goto label3;
		if (eof2) goto label2;

		delete r1; delete r2;
		r1 = new WRec(&PW1->A[r1ofs]);
		r2 = new WRec(&PW2->A[r2ofs]);

		if (r1->Comp(r2) == _gt) goto label3;
	label2:
		memcpy(&PW->A[rofs], &PW1->A[r1ofs], RecLen);
		rofs += RecLen;
		r1ofs += RecLen;
		if (r1ofs == max1ofs) {
			PutFreeNr(Pg1);
			Pg1 = PW1->Chain;
			if (Pg1 != 0) {
				ReadWPage(PW1, Pg1);
				delete r1;
				r1ofs = 0;
				r1 = new WRec(&PW1->A[r1ofs]);
				max1ofs = PW1->NRecs * RecLen;
			}
			else if (eof2) break;
			else eof1 = true;
		}
		continue;
	label3:
		memcpy(&PW->A[rofs], &PW2->A[r2ofs], RecLen);
		rofs += RecLen;
		r2ofs += RecLen;
		if (r2ofs == max2ofs) {
			PutFreeNr(Pg2);
			Pg2 = PW2->Chain;
			if (Pg2 != 0) {
				ReadWPage(PW2, Pg2);
				delete r2;
				r2ofs = 0;
				r2 = new WRec(&PW2->A[r2ofs]);
				max2ofs = PW2->NRecs * RecLen;
			}
			else if (eof1) break;
			else eof2 = true;
		}
		continue;
	}

	WriteWPage(xKey, rofs / RecLen, Pg, Nxt, 0, record);
	delete r; delete r1; delete r2;
}

void XWorkFile::PutFreeNr(int N)
{
	NFreeNr++;
	FreeNr[NFreeNr] = N;
}
