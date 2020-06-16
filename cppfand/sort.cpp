#include "sort.h"
#include "oaccess.h"
#include "obaseww.h"
#include "runfrml.h"
#include "models/Instr.h"

class XXPage; // forward declaration

class WRec /* record on WPage */
{
public:
	BYTE N[3] { 0, 0 ,0 };
	BYTE IR[3] { 0, 0, 0 };
	XString X;
	longint GetN(); // ASM
	void PutN(longint NN); // ASM
	void PutIR(longint II); // ASM
	WORD Comp(WRec* R); // ASM
};

class WPage /* ca. 64k pages in work file */
{
public:
	longint NxtChain = 0;
	longint Chain = 0;
	WORD NRecs = 0;
	BYTE A = 0;
	void Sort(WORD N, WORD RecLen);
private:
	void PushWord(WORD W);
	WORD PopWord(WORD N, WORD RecLen);
};

class WorkFile
{
public:
	WorkFile();
	virtual ~WorkFile();
	FILE* Handle = nullptr;
	WORD RecLen = 0, MaxOnWPage = 0, WPageSize = 0;
	longint MaxWPage = 0, WRoot = 0, NChains = 0, PgWritten = 0;
	longint WBaseSize = 0;
	WPage* PW = nullptr; WPage* PW1 = nullptr; WPage* PW2 = nullptr;
	longint FreeNr[5] { 0, 0, 0, 0, 0 };
	WORD NFreeNr = 0;
	longint IRec = 0, RecNr = 0;
	KeyFldD* KFRoot = nullptr;
	void Reset(KeyFldD* KF, longint RestBytes, char Typ, longint NRecs);
	void SortMerge();
	virtual bool GetCRec();
	virtual void Output(WRec* R);
private:
	void TestErr();
	longint GetFreeNr();
	void Merge();
	void Merge2Chains(longint Pg1, longint Pg2, longint Pg, longint Nxt);
	void PutFreeNr(longint N);
	void ReadWPage(WPage* W, longint Pg);
	void WriteWPage(WORD N, longint Pg, longint Nxt, longint Chn);
};

class XWorkFile : public WorkFile
{
public:
	XWorkFile(XScan* AScan, XKey* AK);
	XXPage* PX = nullptr;
	KeyD* KD = nullptr;
	XScan* Scan = nullptr;
	bool MsgWritten = false;
	longint NxtXPage = 0;
	XWFile* XF = nullptr;
	XPage* XPP = nullptr;
	void Main(char Typ);
	void CopyIndex(KeyD* K, KeyFldD* KF, char Typ);
	bool GetCRec() override;
	void Output(WRec* R) override;
private:
	void FinishIndex();
};

class XXPage /* for building XPage */
{
public:
	XXPage* Chain = nullptr;
	XWorkFile* XW = nullptr;
	WORD Off = 0, MaxOff = 0;
	pstring LastIndex;
	longint LastRecNr = 0, Sum = 0;
	bool IsLeaf = false;
	longint GreaterPage = 0;
	WORD NItems = 0;
	BYTE A[XPageSize - XPageOverHead];
	void Reset(XWorkFile* OwnerXW);
	void PutN(void* N); // ASM
	void PutDownPage(longint DownPage); // ASM
	void PutMLX(BYTE M, BYTE L); // ASM
	void ClearRest(); // ASM
	void PageFull();
	void AddToLeaf(WRec* R, KeyDPtr KD);
	void AddToUpper(XXPage* P, longint DownPage);
};

longint WRec::GetN()
{
	// asm les di,Self; mov ax,es:[di]; mov dl,es:[di+2]; xor dh,dh ;
	return 0;
}

void WRec::PutN(longint NN)
{
	//asm { asm les di, Self; mov ax, NN.WORD; cld; stosw; mov al, NN[2].BYTE; stosb; }
}

void WRec::PutIR(longint II)
{
	// asm les di,Self; add di,3; mov ax,II.WORD; cld; stosw; mov al,II[2].BYTE; stosb;
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
	return 0;
}

void WPage::Sort(WORD N, WORD RecLen)
{

}

void WPage::PushWord(WORD W)
{
}

WORD WPage::PopWord(WORD N, WORD RecLen)
{
	WRec* X = nullptr; WRec* Y = nullptr; WRec* Z = nullptr; WRec* V = nullptr;
	WORD* oX = (WORD*)X;
	WORD* oY = (WORD*)Y;
	WORD* oZ = (WORD*)Z;
	WORD oA = 0, cx = 0, cy = 0, OldSP = 0, CurSP = 0;
	integer iX, iY, R, L;
	if (N <= 1) return 0; // TODO: co má vracet?
	V = (WRec*)GetStore(sizeof(WRec));
	X = (WRec*)&A;
	// TODO: PtrRec(X).Seg -= 0x10; oX += 0x100; /*prevent negative ofs*/
	Y = X; Z = X; oA = *oX;
	// TODO: asm mov OldSP, sp;
	PushWord(0); PushWord(N - 1);
	do {
		R = PopWord(N, RecLen);
		L = PopWord(N, RecLen);
		do {
			*oZ = oA + ((L + R) >> 1) * RecLen;
			MyMove(Z, V, RecLen);
			*oX = oA + L * RecLen;
			*oY = oA + R * RecLen;
			do {
			label1:
				cx = X->Comp(V);
				if (cx == _lt) { oX += RecLen; goto label1; }
			label2:
				cy = V->Comp(Y);
				if (cy == _lt) { oY -= RecLen; goto label2; }
				if (oX <= oY) {
					if ((cx || cy) != _equ) ExChange(X, Y, RecLen);
					oX += RecLen; oY -= RecLen;
				}
			} while (oX <= oY);
			iX = (*oX - oA) / RecLen;
			if (oX - RecLen > oY) iY = iX - 2; else iY = iX - 1;
			if (iY == L) L = iX;
			else if (iX == R) R = iY;
			else if (iY - L < R - iX) {  /*push longest interval on stack*/
				if (iX < R) { PushWord(iX); PushWord(R); } R = iY;
			}
			else {
				if (L < iY) { PushWord(L); PushWord(iY); } L = iX;
			}
		} while (L < R);
		// TODO: asm mov CurSP, sp;
	} while (OldSP != CurSP);
	ReleaseStore(V);
	// TODO: co má vracet?
	return 0;
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
	longint BYTEs; longint pages;
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
	if (BYTEs < kB60) WPageSize = WORD(BYTEs) & 0xF000;
	else WPageSize = kB60;
	MaxOnWPage = (WPageSize - sizeof(WPage) + 1) / RecLen;
	if (integer(MaxOnWPage) < 4) RunError(624);
	MaxWPage = 0; NFreeNr = 0;
	PW = (WPage*)GetStore(WPageSize);
	WRoot = GetFreeNr();
	pages = (NRecs + MaxOnWPage - 1) / MaxOnWPage;
	//asm mov dx, pages.WORD; mov bx, dx; mov cx, dx; mov ax, 1;
	//@add cx 1, dx; test bx, 1; jz @2; sub cx, ax; shr bx, 1; add bx, 1; jmp @3;
	//@shr bx 2, 1;            /* how many pages must be written ? */
	//@shl ax 3, 1; cmp bx, 1; ja @1; cmp cx, 0; jne @4; mov cx, 1;
	//@mov pages 4.WORD, cx;

	RunMsgOn(Typ, pages);
}

void WorkFile::SortMerge()
{
	WRec* r = nullptr; WORD* rofs = (WORD*)r;
	WORD n; longint pg, nxt;
	PgWritten = 0;
	n = 0; r = (WRec*)(&PW->A); nxt = WRoot; NChains = 1;
	while (GetCRec()) {
		if (n == MaxOnWPage) {
			PW->Sort(n, RecLen); pg = nxt; nxt = GetFreeNr(); NChains++;
			WriteWPage(n, pg, nxt, 0);
			n = 0; r = (WRec*)(&PW->A);
		}
		r->PutN(RecNr); r->PutIR(IRec); r->X.PackKF(KFRoot);
		n++; rofs += RecLen;
	}
	PW->Sort(n, RecLen);
	WriteWPage(n, nxt, 0, 0);
	if (NChains > 1) Merge();
	RunMsgOff();
}

bool WorkFile::GetCRec()
{
	// TODO: v originále nevrací nic
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
	WriteWPage((*rofs - w->A) / l, Pg, Nxt, 0);
}

void WorkFile::PutFreeNr(longint N)
{
	NFreeNr++; FreeNr[NFreeNr] = N;
}

void WorkFile::ReadWPage(WPage* W, longint Pg)
{
	SeekH(Handle, WBaseSize + (Pg - 1) * WPageSize);
	ReadH(Handle, WPageSize, W); TestErr();
}

void WorkFile::WriteWPage(WORD N, longint Pg, longint Nxt, longint Chn)
{
	WRec* r = nullptr;
	WORD* rofs = (WORD*)r;
	PgWritten++;
	RunMsgN(PgWritten);
	if (NChains == 1) {
		r = (WRec*)(&PW->A);
		while (N > 0) {
			Output(r); N--; rofs += RecLen;
		}
	}
	else {
		/* !!! with PW^ do!!! */
		{ PW->NRecs = N; PW->NxtChain = Nxt; PW->Chain = Chn; }
		SeekH(Handle, WBaseSize + (Pg - 1) * WPageSize);
		WriteH(Handle, WPageSize, PW); TestErr();
	}
}

XWorkFile::XWorkFile(XScan* AScan, XKey* AK)
{
	Scan = AScan; CFile = Scan->FD; KD = AK; XF = AK->XF();
}

void XWorkFile::Main(char Typ)
{
	WRec* R; KeyDPtr k; KeyFldDPtr kf; XPagePtr p; bool frst;
	XPP = (XPage*)GetStore(XPageSize); NxtXPage = XF->NewPage(XPP);
	MsgWritten = false; frst = true;
	while (KD != nullptr) {
		PX = (XXPage*)GetZStore(sizeof(XXPage));
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
			if (frst) frst = false; else Scan->SeekRec(0);
			Reset(KD->KFlds, sizeof(XXPage) * 9, Typ, Scan->NRecs); SortMerge();
		}
		FinishIndex();
		ReleaseStore(PX); KD = KD->Chain;
	}
	XF->ReleasePage(XPP, NxtXPage); ReleaseStore(XPP);
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
			if (KF == nullptr) x = x->Next(oLeaf);
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
	longint sum, n; XXPage* p; XXPage* p1;
	n = 0; sum = 0; p = PX;
label1:
	sum = sum + p->Sum;
	p->ClearRest(); p->GreaterPage = n; p1 = p->Chain;
	if (p1 == nullptr) n = KD->IndexRoot; else n = NxtXPage;
	XF->WrPage(XPagePtr(&p->IsLeaf), n); p = p1;
	if (p != nullptr) { NxtXPage = XF->NewPage(XPP); goto label1; }
	if (KD->InWork) WKeyDPtr(KD)->NR = sum;
	else ((XFile*)XF)->NRecs = sum;
}

void XXPage::Reset(XWorkFile* OwnerXW)
{
	XW = OwnerXW; Sum = 0; NItems = 0;
	// MaxOff = ofs(Chain) + sizeof(XXPage); Off = ofs(A);
}

void XXPage::PutN(void* N)
{
	/*asm push ds; cld; les bx, Self; mov di, es: [bx] .XXPage.Off;
	lds si, N; lodsw; stosw; lodsb; stosb;
	mov es : [bx] .XXPage.Off, di; pop ds;*/
}

void XXPage::PutDownPage(longint DownPage)
{
	/*asm cld; les bx, Self; mov di, es: [bx] .XXPage.Off;
	mov ax, DownPage.WORD; stosw; mov ax, DownPage[2].WORD; stosw;
	mov es : [bx] .XXPage.Off, di;*/
}

void XXPage::PutMLX(BYTE M, BYTE L)
{
	/*asm push ds; cld; les bx, Self; mov di, es: [bx] .XXPage.Off;
	mov al, M; stosb; mov al, L; stosb;
	mov ax, es; mov ds, ax; lea si, es: [bx] .XXPage.LastIndex;
	inc si;xor ch, ch; mov cl, M; add si, cx; mov cl, L; rep movsb;
	mov es : [bx] .XXPage.Off, di; pop ds;*/
}

void XXPage::ClearRest()
{
	/*asm les bx, Self; mov di, es: [bx] .XXPage.Off; mov cx, es: [bx] .XXPage.MaxOff;
	sub cx, di; jcxz @1; cld; mov al, 0; rep stosb; @  1;*/
}

void XXPage::PageFull()
{
	longint n;
	ClearRest();
	if (Chain == nullptr) {
		Chain = (XXPage*)GetZStore(sizeof(XXPage));
		Chain->Reset(XW);
	}
	if (IsLeaf) n = XW->NxtXPage;
	else n = XW->XF->NewPage(XW->XPP);
	Chain->AddToUpper(this, n);
	if (IsLeaf) { XW->NxtXPage = XW->XF->NewPage(XW->XPP); GreaterPage = XW->NxtXPage; }
	XW->XF->WrPage((XPage*)(&IsLeaf), n);
}

void XXPage::AddToLeaf(WRec* R, KeyDPtr KD)
{
	BYTE m, l; XKey* k; longint n;
label1:
	m = 0; l = R->X.S.length(); n = R->GetN();
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
				ReadRec(n);
				k = CFile->Keys; while ((k != KD)) {
					k->Delete(n); k = k->Chain;
				}
				SetDeletedFlag(); WriteRec(n); return;
			}
		}
		l = l - m;
	}
	if (Off + 5 + l > MaxOff) {
		PageFull(); Reset(XW); goto label1;
	}
	LastIndex = R->X.S; LastRecNr = n; Sum++;
	NItems++; PutN(&n); PutMLX(m, l);
}

void XXPage::AddToUpper(XXPage* P, longint DownPage)
{
	WORD l, m;
label1:
	m = 0; l = P->LastIndex.length();
	if ((l > 0) && (NItems > 0)) {
		m = SLeadEqu(P->LastIndex, LastIndex); l = l - m;
	}
	if (Off + 9 + l > MaxOff) {
		PageFull(); Reset(XW);
		goto label1;
	}
	LastIndex = P->LastIndex; Sum += P->Sum; NItems++; PutN(&P->Sum);
	PutDownPage(DownPage); PutMLX(m, l);
}

void CreateIndexFile()
{
	ExitRecord er;
	void* cr = nullptr; void* p = nullptr;
	LockMode md = NullMode; bool fail = false;
	XWorkFile* XW;
	XScan* Scan;
	XFile* XF = nullptr;
	//NewExit(Ovr(), er);
	goto label1;
	MarkStore(p); fail = true;
	XF = CFile->XF; cr = CRecPtr; CRecPtr = GetRecSpace();
	md = NewLMode(RdMode); TryLockN(0, 0); /*ClearCacheCFile;*/
	if (XF->Handle == nullptr) RunError(903);
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
		XF->NotValid = false; XF->WrPrefix();
		if (!SaveCache(0)) GoExit(); /*FlushHandles;*/;
	}
	fail = false;
label1:
	RestoreExit(er); ReleaseStore(p); CRecPtr = cr;
	if (fail) { XF->SetNotValid(); XF->NoCreate = true; }
	UnLockN(0); OldLMode(md);
	if (fail) GoExit();
}

void CreateWIndex(XScan* Scan, XWKey* K, char Typ)
{
	void* p = nullptr;
	MarkStore(p);
	void* cr = CRecPtr;
	CRecPtr = GetRecSpace();
	//New(XW, Init(Scan, K));
	XWorkFile* XW = new XWorkFile(Scan, K);
	XW->Main(Typ);
	delete XW;
	CRecPtr = cr;
	ReleaseStore(p);
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
	while (!Scan->eof) {
		RunMsgN(Scan->IRec); CFile = FD2; PutRec(); Scan->GetRec();
	}
	if (!SaveCache(0)) GoExit();
	CFile = cf; SubstDuplF(FD2, false); Scan->Close(); RunMsgOff();
	ReleaseStore(p);
}

void GetIndex(Instr_getindex* PD)
{
	XScan* Scan = nullptr; void* p = nullptr; LocVar* lv = nullptr;
	LocVar* lv2 = nullptr; XWKey* kNew = nullptr; XWKey* k = nullptr; longint nr = 0;
	LockMode md, md1; FrmlPtr cond = nullptr; LinkD* ld = nullptr;
	KeyFldD* kf = nullptr; XString x;
	MarkStore(p);
	lv = PD->giLV; CFile = lv->FD; k = WKeyDPtr(lv->RecPtr);
	md = NewLMode(RdMode);
	if (PD->giMode == ' ') {
		ld = PD->giLD; kf = ld->ToKey->KFlds; lv2 = PD->giLV2;
		//New(Scan, Init(CFile, PD->giKD, PD->giKIRoot, false));
		Scan = new XScan(CFile, PD->giKD, PD->giKIRoot, false);
		cond = RunEvalFrml(PD->giCond);
		switch (PD->giOwnerTyp) {
		case 'i': Scan->ResetOwnerIndex(ld, lv2, cond); break;
		case 'r': { CFile = ld->ToFD; CRecPtr = lv2->RecPtr; x.PackKF(kf); goto label1; break; }
		case 'F': { CFile = ld->ToFD; md = NewLMode(RdMode); CRecPtr = GetRecSpace();
			ReadRec(RunInt(FrmlPtr(PD->giLV2))); x.PackKF(kf);
			ReleaseStore(CRecPtr); OldLMode(md);
		label1:
			CFile = lv->FD; Scan->ResetOwner(&x, cond); break; }
		default: Scan->Reset(cond, PD->giSQLFilter); break;
		}
		kf = PD->giKFlds;
		if (kf == nullptr) kf = k->KFlds;
		kNew = (XWKey*)GetZStore(sizeof(*kNew));
		kNew->Open(kf, true, false);
		CreateWIndex(Scan, kNew, 'X'); k->Close(); *k = *kNew;
	}
	else {
		CRecPtr = GetRecSpace(); nr = RunInt(PD->giCond);
		if ((nr > 0) && (nr <= CFile->NRecs)) {
			ReadRec(nr);
			if (PD->giMode == '+') {
				if (!DeletedFlag()) {
					x.PackKF(k->KFlds); if (!k->RecNrToPath(x, nr)) {
						k->InsertOnPath(x, nr); k->NR++;
					}
				}
			}
			else if (k->Delete(nr)) k->NR--;
		}
	}
	OldLMode(md); ReleaseStore(p);
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
