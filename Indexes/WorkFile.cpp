#include "WorkFile.h"
#include <memory>
#include "../cppfand/base.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/obaseww.h"

WorkFile::WorkFile()
{
	WBaseSize = MaxWSize;
	Handle = WorkHandle;
}

WorkFile::~WorkFile()
{
	MaxWSize = WBaseSize; TruncH(Handle, MaxWSize); FlushH(Handle);
}

void WorkFile::Reset(KeyFldD* KF, longint RestBytes, char Typ, longint NRecs)
{
	longint BYTEs = 0;
	longint pages = 0;
	const WORD kB60 = 0x0F000;
	KFRoot = KF;
	RecLen = 7;
	while (KF != nullptr) {
		if (Typ == 'D') {
			RecLen += 6;
		}
		else {
			if (KF->FldD != nullptr) {
				RecLen += KF->FldD->NBytes;
			}
		}
		KF = KF->pChain;
	}
	BYTEs = (StoreAvail() - RestBytes - sizeof(WRec)) / 3;
	if (BYTEs < 4096) RunError(624);
	if (BYTEs < kB60) WPageSize = (WORD)BYTEs & 0xF000;
	else WPageSize = kB60;
	// MaxOnWPage = (WPageSize - (sizeof(WPage) - 65535 + 1)) / RecLen; // nebude se do toho pocitat delka pole 'A' (66535)
	MaxOnWPage = (WPageSize - 10 + 1) / RecLen; // 10B is size of WPage without array
	if (MaxOnWPage < 4) RunError(624);
	MaxWPage = 0; NFreeNr = 0;
	PW = new WPage();
	WRoot = GetFreeNr();
	pages = (NRecs + MaxOnWPage - 1) / MaxOnWPage;

	// nasledujici usek v ASM zrejme jen udela nejaky prepocet kvuli zobrazeni zpravy
	// v RunMsgOn()
	//asm mov dx, pages.WORD; mov bx, dx; mov cx, dx; mov ax, 1;
	//@add cx 1, dx; test bx, 1; jz @2; sub cx, ax; shr bx, 1; add bx, 1; jmp @3;
	//@shr bx 2, 1;            /* how many pages must be written ? */
	//@shl ax 3, 1; cmp bx, 1; ja @1; cmp cx, 0; jne @4; mov cx, 1;
	//@mov pages 4.WORD, cx;

	RunMsgOn(Typ, pages);
}

/// precte zaznamy, vytvori uplnou delku klice, setridi zaznamy
void WorkFile::SortMerge()
{
	// z 'PW->A' se postupne berou jednotlive WRec;
	// ty pak projdou upravou;
	// a zpatky se vraci na stejne misto do PW->A;

	//WRec* r = new WRec();
	BYTE buffer[512]{ 0 };
	size_t offsetOfPwA = 0;
	WORD n = 0; longint pg = 0, nxt = 0;
	PgWritten = 0;
	nxt = WRoot;
	NChains = 1;
	while (GetCRec()) {
		if (n == MaxOnWPage) {
			PW->Sort(n, RecLen);
			pg = nxt;
			nxt = GetFreeNr();
			NChains++;
			WriteWPage(n, pg, nxt, 0);
			memset(PW->A, '0', sizeof(PW->A));
			offsetOfPwA = 0;
			n = 0;
		}
		auto r = std::make_unique<WRec>();
		r->PutN(RecNr);
		r->PutIR(IRec);
		r->X.PackKF(KFRoot);
		size_t serLen = r->Serialize(buffer);
		memcpy(&PW->A[offsetOfPwA], buffer, serLen);
		n++;
		offsetOfPwA += RecLen;
	}
	PW->Sort(n, RecLen);
	WriteWPage(n, nxt, 0, 0);
	if (NChains > 1) {
		Merge();
	}
	RunMsgOff();
}

void WorkFile::TestErr()
{
	if (HandleError != 0) {
		SetMsgPar(FandWorkName);
		RunError(700 + HandleError);
	}
}

longint WorkFile::GetFreeNr()
{
	longint result = 0;
	if (NFreeNr > 0) { result = FreeNr[NFreeNr]; NFreeNr--; }
	else { MaxWPage++; MaxWSize += WPageSize; result = MaxWPage; }
	return result;
}

void WorkFile::Merge()
{
	longint npairs, newli, nxtnew, pg1, pg2;
	longint nxt = 0;
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
		if (npairs == 1) nxtnew = 0;
		else nxtnew = GetFreeNr();
		NChains--;
		Merge2Chains(pg1, pg2, newli, nxtnew);
		npairs--;
		pg1 = nxt;
		newli = nxtnew;
	}
	goto label1;
}

void WorkFile::Merge2Chains(longint Pg1, longint Pg2, longint Pg, longint Nxt)
{
	bool eof1 = false;
	bool eof2 = false;

	size_t r1ofs = 0;
	size_t r2ofs = 0;
	size_t rofs = 0;

	WRec* r1 = nullptr;
	WRec* r2 = nullptr;
	WRec* r = new WRec(&PW->A[rofs]);

	WORD max1ofs = PW1->NRecs * RecLen;
	WORD max2ofs = PW2->NRecs * RecLen;
	const WORD maxofs = MaxOnWPage * RecLen;

	while (true) {
		if (rofs == maxofs) {
			longint chn = GetFreeNr();
			WriteWPage(MaxOnWPage, Pg, Nxt, chn);
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

	WriteWPage(rofs / RecLen, Pg, Nxt, 0);
	delete r; delete r1; delete r2;
}

void WorkFile::PutFreeNr(longint N)
{
	NFreeNr++;
	FreeNr[NFreeNr] = N;
}

void WorkFile::ReadWPage(WPage* W, longint Pg)
{
	SeekH(Handle, WBaseSize + (Pg - 1) * WPageSize);
	ReadH(Handle, 4, &W->NxtChain);
	ReadH(Handle, 4, &W->Chain);
	ReadH(Handle, 2, &W->NRecs);
	ReadH(Handle, WPageSize - 10, &W->A);
	TestErr();
}

void WorkFile::WriteWPage(WORD N, longint Pg, longint Nxt, longint Chn)
{
	size_t offset = 0;
	PgWritten++;
	RunMsgN(PgWritten);
	if (NChains == 1) {
		for (size_t i = 0; i < N; i++) {
			WRec r;
			r.Deserialize(&PW->A[offset]);
			Output(&r);
			BYTE buffer[512]{ 0 };
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