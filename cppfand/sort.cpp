#include "sort.h"
#include <queue>


#include "FieldDescr.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "KeyFldD.h"
#include "oaccess.h"
#include "obaseww.h"
#include "runfrml.h"
#include "XFile.h"
#include "../Logging/Logging.h"
#include "models/Instr.h"


WRec::WRec(WPage* wp)
{
	N[0] = wp->A[0]; N[1] = wp->A[1]; N[2] = wp->A[2];
	IR[0] = wp->A[3]; IR[1] = wp->A[4]; IR[2] = wp->A[5];
	X.S[0] = wp->A[6];
	memcpy(&X.S[1], &wp->A[7], X.S[0]);
}

longint WRec::GetN()
{
	// asm les di,Self; mov ax,es:[di]; mov dl,es:[di+2]; xor dh,dh ;
	return N[0] + (N[1] << 8) + (N[2] << 16);
}

void WRec::PutN(longint NN)
{
	//asm { asm les di, Self; mov ax, NN.WORD; cld; stosw; mov al, NN[2].BYTE; stosb; }
	N[0] = NN & 0xFF;
	N[1] = (NN >> 8) & 0xFF;
	N[2] = (NN >> 16) & 0xFF;
}

void WRec::PutIR(longint II)
{
	// asm les di,Self; add di,3; mov ax,II.WORD; cld; stosw; mov al,II[2].BYTE; stosb;
	IR[0] = II & 0xFF;
	IR[1] = (II >> 8) & 0xFF;
	IR[2] = (II >> 16) & 0xFF;
}

WORD WRec::Comp(WRec* R)
{
	//asm push ds; cld; lds si, Self; mov al, [si + 6]; add si, 7;
	//les di, R; mov ah, es: [di + 6] ; add di, 7;
	//xor ch, ch; mov cl, al; cmp ah, al; ja @1; mov cl, ah;
	//@jcxz  1@11; repe cmpsb; jb @2; ja @3;
	//@cmp al 11, ah; jb @2; ja @3;   /*compare X*/
	//mov si, Self.WORD; mov di, R.WORD; mov al, ds: [si + 5] ; cmp al, es: [di + 5] ; /*compare IR*/
	//jb @2; ja @3; mov ax, ds: [si + 3] ; cmp ax, es: [di + 3] ; jb @2; ja @3;
	//mov ax, 1; jmp @4;
	//@mov ax 2, 2; jmp @4;
	//@mov ax 3, 4;
	//@pop ds 4;

	WORD offThis = 0;
	WORD offR = 0;
	BYTE lenThis = X.S[0]; // AL
	BYTE lenR = R->X.S[0]; // AH
	if (lenThis != 0 && lenR != 0) {
		// porovnani retezcu VETSI delkou - nechapu ale proc ...
		for (size_t i = 0; i < max(lenThis, lenR); i++) {
			if (X.S[i + 1] == R->X.S[i + 1]) continue;
			if (X.S[i + 1] < R->X.S[i + 1]) return _lt;
			else return _gt;
		}
	}
	if (lenThis != lenR) { // compare X
		if (lenThis < lenR) return _lt;
		else return _gt;
	}

	int irThis = IR[0] + (IR[1] << 8) + (IR[2] << 16);
	int irR = R->IR[0] + (R->IR[1] << 8) + (R->IR[2] << 16);
	if (irThis != irR) { // compare IR
		if (irThis < irR) return _lt;
		else return _gt;
	}

	return _equ;
}

WORD WRec::Compare(const WRec& w) const
{
	WORD offThis = 0;
	WORD offR = 0;
	BYTE lenThis = X.S.length(); // AL
	BYTE lenR = w.X.S.length(); // AH
	if (lenThis != 0 && lenR != 0) {
		// porovnani retezcu VETSI delkou - nechapu ale proc ...
		for (unsigned char i = 0; i < max(lenThis, lenR); i++) {
			if (X.S.at(i + 1) == w.X.S.at(i + 1)) continue;
			if (X.S.at(i + 1) < w.X.S.at(i + 1)) return _lt;
			else return _gt;
		}
	}
	if (lenThis != lenR) { // compare X
		if (lenThis < lenR) return _lt;
		else return _gt;
	}

	int irThis = IR[0] + (IR[1] << 8) + (IR[2] << 16);
	int irR = w.IR[0] + (w.IR[1] << 8) + (w.IR[2] << 16);
	if (irThis != irR) { // compare IR
		if (irThis < irR) return _lt;
		else return _gt;
	}

	return _equ;
}

void WRec::Deserialize(unsigned char* data)
{
	memcpy(N, &data[0], 3);
	memcpy(IR, &data[3], 3);
	size_t len = data[6];
	memcpy(&X.S[0], &data[6], len + 1);
}

size_t WRec::Serialize(unsigned char* buffer)
{
	memcpy(&buffer[0], N, 3);
	memcpy(&buffer[3], IR, 3);
	size_t len = X.S[0];
	memcpy(&buffer[6], &X.S[0], len + 1);
	return 3 + 3 + 1 + len; // N + IR + S[0] + 1 B delka
}

bool WRec::operator==(const WRec& w) const
{
	return Compare(w) == 1;
}

bool WRec::operator<(const WRec& w) const
{
	return Compare(w) == 2;
}

bool WRec::operator>(const WRec& w) const
{
	return Compare(w) == 4;
}

void ExChange(void* X, void* Y, WORD L)
{
	if (L == 0) return;
	printf("sort.cpp ExChange() - not implemented");
}

void WPage::Sort(WORD N, WORD RecLen)
{
	if (N <= 1) return;
	// vytvorime vektor zaznamu a vsechny do nej nacteme z 'A'
	std::vector<WRec> recs;
	size_t offset1 = 0;
	for (size_t i = 0; i < N; i++) {
		WRec r;
		r.Deserialize(&A[offset1]);
		recs.push_back(r);
		offset1 += RecLen;
	}

	// vektor setridime
	std::sort(recs.begin(), recs.end());

	// setridene zaznamy vlozime zpet do 'A'
	size_t offset2 = 0;
	BYTE buffer[256]{ 0 };
	for (size_t i = 0; i < N; i++) {
		auto len = recs[i].Serialize(buffer);
		memcpy(&A[offset2], buffer, len);
		offset2 += RecLen;
	}
	
	// zkotrolujeme délky offsetu pri nacitani a pri ukladani
	// mely by byt stejne
	if (offset1 != offset2) throw std::exception("WPage::Sort() error: Offset1 != Offset2");

	//std::stack<integer> stack;
	//WRec* X = nullptr, * Y = nullptr, * Z = nullptr, * V = nullptr;
	//WORD oA = 0, cx = 0, cy = 0, OldSP = 0, CurSP = 0;
	//integer iX, iY, R, L;

	//V = new WRec(); // GetStore(sizeof(WRec));
	//X = new WRec(this); // WRecPtr(A);
	////dec(PtrRec(X).Seg, 0x10);
	//*(WORD*)&X->N[0] += 0x100; /*prevent negative ofs*/
	//Y = X; Z = X;
	//oA = *(WORD*)&X->N[0];
	//stack.push(0);
	//stack.push(N - 1);
	//
	//do {
	//	R = stack.top(); stack.pop();
	//	L = stack.top(); stack.pop();
	//	do {
	//		*(WORD*)&Z->N[0] = oA + ((L + R) >> 1) * RecLen;
	//		memcpy(V, Z, RecLen); // MyMove(Z^, V^, RecLen);
	//		*(WORD*)&X->N[0] = oA + L * RecLen;
	//		*(WORD*)&Y->N[0] = oA + R * RecLen;
	//		do {
	//		label1:
	//			cx = X->Comp(V);
	//			if (cx == _lt) { *(WORD*)&X->N[0] += RecLen; goto label1; }
	//		label2:
	//			cy = V->Comp(Y);
	//			if (cy == _lt) { *(WORD*)&Y->N[0] -= RecLen; goto label2; }
	//			if (*(WORD*)&X->N[0] <= *(WORD*)&Y->N[0]) {
	//				if ((cx || cy) != _equ) ExChange(X, Y, RecLen);
	//				*(WORD*)&X->N[0] += RecLen;
	//				*(WORD*)&Y->N[0] -= RecLen;
	//			}
	//		} while (!(*(WORD*)&X->N[0] > *(WORD*)&Y->N[0]));
	//		iX = (*(WORD*)&X->N[0] - oA) / RecLen;
	//		if (*(WORD*)&X->N[0] - RecLen > *(WORD*)&Y->N[0]) iY = iX - 2;
	//		else iY = iX - 1;
	//		if (iY == L) L = iX;
	//		else if (iX == R) R = iY;
	//		else if (iY - L < R - iX) {  /*push longest interval on stack*/
	//			if (iX < R) {
	//				stack.push(iX);
	//				stack.push(R);
	//			}
	//			R = iY;
	//		}
	//		else {
	//			if (L < iY) {
	//				stack.push(L);
	//				stack.push(iY);
	//			}
	//			L = iX;
	//		}
	//	} while (!(L >= R));
	//} while (!stack.empty());
	//ReleaseStore(V);
}

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
	longint BYTEs = 0; longint pages = 0;
	const WORD kB60 = 0x0F000;
	KFRoot = KF; RecLen = 7;
	while (KF != nullptr) {
		/* !!! with KF->FldD^ do!!! */
		if (Typ == 'D') RecLen += 6;
		else RecLen += KF->FldD->NBytes;
		KF = (KeyFldD*)KF->Chain;
	}
	BYTEs = (StoreAvail() - RestBytes - sizeof(WRec)) / 3;
	if (BYTEs < 4096) RunError(624);
	if (BYTEs < kB60) WPageSize = (WORD)BYTEs & 0xF000;
	else WPageSize = kB60;
	MaxOnWPage = (WPageSize - (sizeof(WPage) - 65535 + 1)) / RecLen; // nebude se do toho pocitat delka pole 'A' (66535)
	if (MaxOnWPage < 4) RunError(624);
	MaxWPage = 0; NFreeNr = 0;
	PW = new WPage(); // (WPage*)GetStore(WPageSize);
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
	
	WRec* r = new WRec();
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
			n = 0;
		}
		r->Deserialize(&PW->A[offsetOfPwA]);
		r->PutN(RecNr);
		r->PutIR(IRec);
		r->X.PackKF(KFRoot);
		auto serLen = r->Serialize(buffer);
		memcpy(&PW->A[offsetOfPwA], buffer, serLen);
		n++;
		offsetOfPwA += RecLen;
	}
	delete r; r = nullptr;
	PW->Sort(n, RecLen);
	WriteWPage(n, nxt, 0, 0);
	if (NChains > 1) Merge();
	RunMsgOff();
}

bool WorkFile::GetCRec()
{
	// vola se pouze ze zdedenych trid
	// tady nema vyznam
	return false;
}

void WorkFile::Output(WRec* R)
{
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
	longint npairs, newli, nxtnew, pg1, pg2, nxt;
	nxt = 0;
	PW1 = (WPage*)GetStore(WPageSize);
	PW2 = (WPage*)GetStore(WPageSize);
label1:
	if (NChains == 1) return;
	npairs = NChains / 2; pg1 = WRoot;
	if (NChains == 2) newli = 0; else { WRoot = GetFreeNr(); newli = WRoot; }
	if (nxt > 0) { pg2 = pg1; pg1 = nxt; ReadWPage(PW1, pg1); goto label2; }
	while (npairs > 0) {
		ReadWPage(PW1, pg1); pg2 = PW1->NxtChain;
	label2:
		ReadWPage(PW2, pg2); nxt = PW2->NxtChain;
		if (npairs == 1) nxtnew = 0; else nxtnew = GetFreeNr();
		NChains--;
		Merge2Chains(pg1, pg2, newli, nxtnew);
		npairs--; pg1 = nxt; newli = nxtnew;
	}
	goto label1;
}

void WorkFile::Merge2Chains(longint Pg1, longint Pg2, longint Pg, longint Nxt)
{
	WRec* r1 = nullptr; WRec* r2 = nullptr; WRec* r = nullptr;
	WORD* r1ofs = (WORD*)r1;
	WORD* r2ofs = (WORD*)r2;
	WORD* rofs = (WORD*)r;
	WORD max1ofs, max2ofs, maxofs;
	bool eof1, eof2;
	longint chn; WORD l;
	WPage* w1 = nullptr; WPage* w2 = nullptr; WPage* w = nullptr;
	w1 = PW1; w2 = PW2; w = PW; l = RecLen; eof1 = false; eof2 = false;
	r1 = (WRec*)(&w1->A); r2 = (WRec*)(&w2->A); r = (WRec*)(&w->A);
	max1ofs = *r1ofs + w1->NRecs * l; max2ofs = *r2ofs + w2->NRecs * l;
	maxofs = *rofs + MaxOnWPage * l;
label1:
	if (*rofs == maxofs) {
		chn = GetFreeNr(); WriteWPage(MaxOnWPage, Pg, Nxt, chn); Pg = chn; Nxt = 0;
		r = (WRec*)(&w->A);
	}
	if (eof1) goto label3;
	if (eof2) goto label2;
	if (r1->Comp(r2) == _gt) goto label3;
label2:
	MyMove(r1, r, l); rofs += l; r1ofs += l;
	if (*r1ofs == max1ofs) {
		PutFreeNr(Pg1); Pg1 = w1->Chain;
		if (Pg1 != 0) {
			ReadWPage(w1, Pg1); r1 = (WRec*)(&w1->A); max1ofs = *r1ofs + w1->NRecs * l;
		}
		else if (eof2) goto label4; else eof1 = true;
	}
	goto label1;
label3:
	MyMove(r2, r, l); rofs += l; r2ofs += l;
	if (*r2ofs == max2ofs) {
		PutFreeNr(Pg2); Pg2 = w2->Chain;
		if (Pg2 != 0) {
			ReadWPage(w2, Pg2); r2 = (WRec*)(&w2->A); max2ofs = *r2ofs + w2->NRecs * l;
		}
		else if (eof1) goto label4; else eof2 = true;
	}
	goto label1;
label4:
	WriteWPage((*rofs - w->A[0]) / l, Pg, Nxt, 0);
}

void WorkFile::PutFreeNr(longint N)
{
	NFreeNr++;
	FreeNr[NFreeNr] = N;
}

void WorkFile::ReadWPage(WPage* W, longint Pg)
{
	SeekH(Handle, WBaseSize + (Pg - 1) * WPageSize);
	ReadH(Handle, WPageSize, W); TestErr();
}

void WorkFile::WriteWPage(WORD N, longint Pg, longint Nxt, longint Chn)
{
	size_t offset = 0;
	PgWritten++;
	RunMsgN(PgWritten);
	if (NChains == 1) {
		//r = (WRec*)(&PW->A);
		for (size_t i = 0; i < N; i++) {
			WRec r;
			r.Deserialize(&PW->A[offset]);
			Output(&r);
			BYTE buffer[512]{ 0 };
			auto len = r.Serialize(buffer);
			memcpy(&PW->A[offset], buffer, len);
			offset += RecLen;
		}
	}
	else {
		/* !!! with PW^ do!!! */
		{ PW->NRecs = N; PW->NxtChain = Nxt; PW->Chain = Chn; }
		SeekH(Handle, WBaseSize + (Pg - 1) * WPageSize);
		WriteH(Handle, WPageSize, PW);
		TestErr();
	}
}

XWorkFile::XWorkFile(XScan* AScan, XKey* AK)
{
	Scan = AScan;
	CFile = Scan->FD;
	KD = AK;
	XF = AK->XF();
}

void XWorkFile::Main(char Typ)
{
	WRec* R = nullptr; KeyD* k = nullptr; KeyFldD* kf = nullptr;
	XPage* p = nullptr; bool frst = false;
	XPP = new XPage(); // (XPage*)GetStore(XPageSize);
	NxtXPage = XF->NewPage(XPP);
	MsgWritten = false;
	frst = true;
	while (KD != nullptr) {
		PX = new XXPage(); // (XXPage*)GetZStore(sizeof(XXPage));
		PX->Reset(this);
		PX->IsLeaf = true;
		k = Scan->Key;
		kf = KD->KFlds;
		if (Scan->Kind == 1 &&
#ifdef FandSQL
			!Scan->FD->IsSQLFile &&
#endif
			(Scan->Bool == nullptr
				&& (EquKFlds(k->KFlds, kf) || kf == nullptr))) CopyIndex(k, kf, Typ);
		else {
			if (frst) frst = false;
			else Scan->SeekRec(0);
			Reset(KD->KFlds, sizeof(XXPage) * 9, Typ, Scan->NRecs);
			SortMerge();
		}
		FinishIndex();
		ReleaseStore(PX);
		KD = KD->Chain;
	}
	XF->ReleasePage(XPP, NxtXPage);
	ReleaseStore(XPP);
}

void XWorkFile::CopyIndex(KeyD* K, KeyFldD* KF, char Typ)
{
	WRec* r = nullptr; XPagePtr p = nullptr; longint page; WORD n; longint i;
	XItem* x = nullptr;
	WORD* xofs = (WORD*)x;

	r = (WRec*)GetStore(sizeof(WRec));
	r->X.S = "";
	p = (XPage*)GetStore(XPageSize);
	K->NrToPath(1);
	page = XPath[XPathN].Page;
	RunMsgOn(Typ, K->NRecs()); i = 0;
	while (page != 0) {
		K->XF()->RdPage(p, page);
		x = (XItem*)(&p->A);
		n = p->NItems;
		while (n > 0) {
			r->PutN(x->GetN());
			if (KF == nullptr) x = x->Next(oLeaf, p->IsLeaf);
			else *xofs = x->UpdStr(oLeaf, (pstring*)(&r->X.S));
			Output(r); n--;
		}
		i += p->NItems; RunMsgN(i); page = p->GreaterPage;
	}
	RunMsgOff();
}

bool XWorkFile::GetCRec()
{
	auto result = false;
	Scan->GetRec();
	result = !Scan->eof;
	RecNr = Scan->RecNr;
	IRec = Scan->IRec;
	return result;
}

void XWorkFile::Output(WRec* R)
{
	PX->AddToLeaf(R, KD);
}

void XWorkFile::FinishIndex()
{
	longint sum = 0, n = 0; XXPage* p = PX; XXPage* p1 = nullptr;
label1:
	sum = sum + p->Sum;
	p->ClearRest();
	p->GreaterPage = n;
	p1 = p->Chain;
	if (p1 == nullptr) n = KD->IndexRoot;
	else n = NxtXPage;

	// kopie XXPage do XPage a jeji zapis;
	auto xp = new XPage();
	xp->IsLeaf = p->IsLeaf;
	xp->GreaterPage = p->GreaterPage;
	xp->NItems = p->NItems;
	memcpy(xp->A, p->A, sizeof(p->A)); // p->A ma 1017B, xp->A ma 1024B
	XF->WrPage(xp, n);
	delete xp; xp = nullptr;
	
	p = p1;
	if (p != nullptr) {
		NxtXPage = XF->NewPage(XPP);
		goto label1;
	}
	if (KD->InWork) ((XWKey*)KD)->NR = sum;
	else ((XFile*)XF)->NRecs = sum;
}

void XXPage::Reset(XWorkFile* OwnerXW)
{
	XW = OwnerXW; Sum = 0; NItems = 0;
	// Offset a Max. offset objektu
	MaxOff = XPageSize - XPageOverHead - 1;
	Off = 0;
}

void XXPage::PutN(longint* N)
{
	/*asm push ds; cld; les bx, Self; mov di, es: [bx] .XXPage.Off;
	lds si, N; lodsw; stosw; lodsb; stosb;
	mov es : [bx] .XXPage.Off, di; pop ds;*/
	memcpy(&A[Off], N, 3); // kopirujeme 3 nejnizsi Byty, posledni se ignoruje
	Off += 3;
}

void XXPage::PutDownPage(longint DownPage)
{
	/*asm cld; les bx, Self; mov di, es: [bx] .XXPage.Off;
	mov ax, DownPage.WORD; stosw; mov ax, DownPage[2].WORD; stosw;
	mov es : [bx] .XXPage.Off, di;*/
	memcpy(&A[Off], &DownPage, 4);
	Off += 4;
}

void XXPage::PutMLX(BYTE M, BYTE L)
{
	/*asm push ds; cld; les bx, Self; mov di, es: [bx] .XXPage.Off;
	mov al, M; stosb; mov al, L; stosb;
	mov ax, es; mov ds, ax; lea si, es: [bx] .XXPage.LastIndex;
	inc si;xor ch, ch; mov cl, M; add si, cx; mov cl, L; rep movsb;
	mov es : [bx] .XXPage.Off, di; pop ds;*/

	A[Off++] = M;
	A[Off++] = L;
	memcpy(&A[Off], &LastIndex[M + 1], L);
	Off += L;
}

void XXPage::ClearRest()
{
	/*asm les bx, Self; mov di, es: [bx] .XXPage.Off; mov cx, es: [bx] .XXPage.MaxOff;
	sub cx, di; jcxz @1; cld; mov al, 0; rep stosb; @  1;*/
	// max. offset je 1 mensi nez delka pole
	// aktualni offset vcetne -> maximalni offset vcetne (proto +1)
	size_t count = MaxOff - Off + 1;
	memset(&A[Off], 0, sizeof(A) - Off);
}

void XXPage::PageFull()
{
	longint n = 0;
	ClearRest();
	if (Chain == nullptr) {
		Chain = new XXPage(); // (XXPage*)GetZStore(sizeof(XXPage));
		Chain->Reset(XW);
	}
	if (IsLeaf) n = XW->NxtXPage;
	else n = XW->XF->NewPage(XW->XPP);
	Chain->AddToUpper(this, n);
	if (IsLeaf) {
		XW->NxtXPage = XW->XF->NewPage(XW->XPP);
		GreaterPage = XW->NxtXPage;
	}
	XW->XF->WrPage((XPage*)(&IsLeaf), n);
}

void XXPage::AddToLeaf(WRec* R, KeyDPtr KD)
{
	BYTE m, l; longint n;
label1:
	m = 0;
	l = R->X.S.length();
	n = R->GetN();
	if ((l > 0) && (NItems > 0)) {
		m = SLeadEqu(R->X.S, LastIndex);
		if ((m == l) && (m == LastIndex.length())) {
			if (n == LastRecNr) return; /* overlapping intervals from  key in .. */
			if (!KD->InWork && !KD->Duplic) {
				if (!XW->MsgWritten) {
					SetMsgPar(CFile->Name);
					if (IsTestRun) { if (!PromptYN(832)) GoExit(); }
					else WrLLF10Msg(828);
					XW->MsgWritten = true;
				}
				ReadRec(CFile, n, CRecPtr);
				XKey* k = CFile->Keys;
				while ((k != KD)) {
					k->Delete(n);
					k = k->Chain;
				}
				SetDeletedFlag();
				WriteRec(n);
				return;
			}
		}
		l = l - m;
	}
	if (Off + 5 + l > MaxOff + 1) { // pristi offset muze skoncit +1, protoze pak uz se nebude zapisovat ...
		PageFull();
		Reset(XW);
		goto label1;
	}
	LastIndex = R->X.S;
	LastRecNr = n;
	Sum++;
	NItems++;
	PutN(&n);
	PutMLX(m, l);
}

void XXPage::AddToUpper(XXPage* P, longint DownPage)
{
	WORD l, m;
label1:
	m = 0;
	l = P->LastIndex.length();
	if ((l > 0) && (NItems > 0)) {
		m = SLeadEqu(P->LastIndex, LastIndex);
		l = l - m;
	}
	if (Off + 9 + l > MaxOff) {
		PageFull();
		Reset(XW);
		goto label1;
	}
	LastIndex = P->LastIndex;
	Sum += P->Sum;
	NItems++;
	PutN(&P->Sum);
	PutDownPage(DownPage);
	PutMLX(m, l);
}

void CreateIndexFile()
{
	Logging* log = Logging::getInstance();
	
	ExitRecord er;
	void* cr = nullptr; //void* p = nullptr;
	LockMode md = NullMode;
	bool fail = false;
	XWorkFile* XW = nullptr;
	XScan* Scan = nullptr;
	XFile* XF = nullptr;
	//NewExit(Ovr(), er);
	//goto label1;
	//MarkStore(p);
	fail = true;
	XF = CFile->XF;
	cr = CRecPtr;
	CRecPtr = GetRecSpace();
	md = NewLMode(RdMode);
	TryLockN(0, 0); /*ClearCacheCFile;*/
	if (XF->Handle == nullptr) RunError(903);
	log->log(loglevel::DEBUG, "CreateIndexFile() file 0x%p name '%s'", XF->Handle, CFile->Name.c_str());
	XF->RdPrefix();
	if (XF->NotValid) {
		XF->SetEmpty();
		//New(Scan, Init(CFile, nullptr, nullptr, false));
		Scan = new XScan(CFile, nullptr, nullptr, false);
		Scan->Reset(nullptr, false);
		//New(XW, Init(Scan, CFile->Keys));
		XW = new XWorkFile(Scan, CFile->Keys);
		XW->Main('X');
		delete XW;
		XF->NotValid = false;
		XF->WrPrefix();
		if (!SaveCache(0, CFile->Handle)) GoExit(); /*FlushHandles;*/;
	}
	fail = false;
label1:
	RestoreExit(er);
	//ReleaseStore(p);
	CRecPtr = cr;
	if (fail) {
		XF->SetNotValid();
		XF->NoCreate = true;
	}
	UnLockN(0);
	OldLMode(md);
	if (fail) GoExit();
}

void CreateWIndex(XScan* Scan, XWKey* K, char Typ)
{
	//void* p = nullptr;
	//MarkStore(p);
	void* cr = CRecPtr;
	CRecPtr = GetRecSpace();
	//New(XW, Init(Scan, K));
	XWorkFile* XW = new XWorkFile(Scan, K);
	XW->Main(Typ);
	delete XW; XW = nullptr;
	CRecPtr = cr;
	//ReleaseStore(p);
}

void ScanSubstWIndex(XScan* Scan, KeyFldD* SK, char Typ)
{
	WORD n = 0;
	//k2 = (XWKey*)GetZStore(sizeof(*k2));
	XWKey* k2 = new XWKey();
	if (Scan->FD->IsSQLFile && (Scan->Kind == 3)) /*F6-autoreport & sort*/ {
		KeyD* k = Scan->Key;
		n = k->IndexLen;
		KeyFldD* kf = SK;
		while (kf != nullptr) {
			n += kf->FldD->NBytes;
			kf = (KeyFldD*)kf->Chain;
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
			kf = (KeyFldD*)kf->Chain;
		}
		if (kf2 != nullptr)	kf2->Chain = SK;
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

	const WORD header000len = 6; // 4B pocet zaznamu, 2B delka 1 zaznamu
	// z puvodniho .000 vycteme pocet zaznamu a jejich delku
	const size_t totalLen = x->FD->NRecs * x->FD->RecLen + header000len;
	BYTE* buffer = new BYTE[totalLen]{ 0 };
	size_t offset = header000len; // zapisujeme nejdriv data; hlavicku az nakonec
	
	while (!x->eof) {
		RunMsgN(x->IRec);
		f->NRecs++;
		memcpy(&buffer[offset], CRecPtr, f->RecLen);
		offset += f->RecLen;
		f->IRec++;
		f->Eof = true;
		x->GetRec();
	}

	// zapiseme hlavicku
	memcpy(&buffer[0], &f->NRecs, 4);
	memcpy(&buffer[4], &f->RecLen, 2);

	// provedeme primy zapis do souboru
	WriteH(f->Handle, totalLen, buffer);

	delete[] buffer; buffer = nullptr;
}

void SortAndSubst(KeyFldD* SK)
{
	void* p = nullptr;
	MarkStore(p);
	FileD* cf = CFile;
	CRecPtr = GetRecSpace();
	//New(Scan, Init(CFile, nullptr, nullptr, false));
	XScan* Scan = new XScan(CFile, nullptr, nullptr, false);
	Scan->Reset(nullptr, false);
	ScanSubstWIndex(Scan, SK, 'S');
	FileD* FD2 = OpenDuplF(false);
	RunMsgOn('S', Scan->NRecs);
	Scan->GetRec();

	// zapiseme data do souboru .100
	GenerateNew000File(FD2, Scan);
	//while (!Scan->eof) {
	//	RunMsgN(Scan->IRec);
	//	CFile = FD2;
	//	PutRec();
	//	Scan->GetRec();
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
	XScan* Scan = nullptr; void* p = nullptr; LocVar* lv = nullptr;
	LocVar* lv2 = nullptr; XWKey* kNew = nullptr; longint nr = 0;
	LockMode md, md1; FrmlElem* cond = nullptr; LinkD* ld = nullptr;
	KeyFldD* kf = nullptr; XString x;
	MarkStore(p);
	lv = PD->giLV;
	CFile = lv->FD;
	XWKey* k = (XWKey*)lv->RecPtr;
	md = NewLMode(RdMode);
	if (PD->giMode == ' ') {
		ld = PD->giLD;
		if (ld != nullptr) kf = ld->ToKey->KFlds;
		if (PD != nullptr) lv2 = PD->giLV2;
		//New(Scan, Init(CFile, PD->giKD, PD->giKIRoot, false));
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
			goto label1;
			break;
		}
		case 'F': {
			CFile = ld->ToFD;
			md = NewLMode(RdMode);
			CRecPtr = GetRecSpace();
			ReadRec(CFile, RunInt((FrmlElem*)PD->giLV2), CRecPtr);
			x.PackKF(kf);
			ReleaseStore(CRecPtr);
			OldLMode(md);
		label1:
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
		kNew = new XWKey(); // (XWKey*)GetZStore(sizeof(*kNew));
		kNew->Open(kf, true, false);
		CreateWIndex(Scan, kNew, 'X');
		k->Close();
		*k = *kNew;
	}
	else {
		CRecPtr = GetRecSpace();
		nr = RunInt(PD->giCond);
		if ((nr > 0) && (nr <= CFile->NRecs)) {
			ReadRec(CFile, nr, CRecPtr);
			if (PD->giMode == '+') {
				if (!DeletedFlag()) {
					x.PackKF(k->KFlds);
					if (!k->RecNrToPath(x, nr)) {
						k->InsertOnPath(x, nr);
						k->NR++;
					}
				}
			}
			else if (k->Delete(nr)) k->NR--;
		}
	}
	OldLMode(md);
	ReleaseStore(p);
}

void CopyIndex(WKeyDPtr K, KeyDPtr FromK)
{
	XScan* Scan = nullptr; void* p = nullptr; LockMode md;
	K->Release(); MarkStore(p); md = NewLMode(RdMode);
	// New(Scan, Init(CFile, FromK, nullptr, false));
	Scan = new XScan(CFile, FromK, nullptr, false);
	Scan->Reset(nullptr, false);
	CreateWIndex(Scan, K, 'W');
	OldLMode(md); ReleaseStore(p);
}
