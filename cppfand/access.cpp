#include "access.h"

#include "common.h"
#include "fileacc.h"
#include "index.h"
#include "kbdww.h"
#include "legacy.h"
#include "memory.h"
#include "obaseww.h"
#include "rdfrml1.h"
#include "recacc.h"
#include "sort.h"
#include "type.h"


//void RunErrorM(LockMode Md, WORD N)
//{
//	OldLMode(Md);
//	RunError(N);
//}

void XString::Clear()
{
	this->S.clean();
}

void XString::StoreReal(double R, KeyFldD* KF)
{
	BYTE A[20];
	const BYTE TabF[18] = { 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8 };
	auto X = KF->FldD;

	if (X->Typ == 'R' || X->Typ == 'D') {
		bool b = KF->Descend;
		if (R < 0) { b = !b; R = -R; }
		StoreD(&R, b);
		return;
	}
	if (X->Flg && f_Comma == 0) R = R * Power10[X->M];
	WORD n = X->L - 1;
	if (X->M > 0) n--;
	n = TabF[n];
	FixFromReal(R, A, n);
	StoreF(A, n, KF->Descend);
}

void XString::StoreStr(pstring V, KeyFldD* KF)
{
	WORD n;
	auto X = KF->FldD;
	while (V[0] < X->L) {
		if (X->M == LeftJust) V = V + " ";
		else {
			auto oldV = V;
			V = " ";
			V += oldV;
		}
	}
	if (X->Typ == 'N') {
		// Pack(&V[1], &V[0], X->L);
		n = (X->L + 1) / 2;
		StoreN(&V, n, KF->Descend);
	}
	else StoreA(&V[1], X->L, KF->CompLex, KF->Descend);
}

void XString::StoreBool(bool B, KeyFldD* KF)
{
	StoreN(&B, 1, KF->Descend);
}

void XString::StoreKF(KeyFldD* KF)
{
	FieldDPtr F;
	F = KF->FldD;
	switch (F->FrmlTyp) {
	case 'S': StoreStr(_ShortS(F), KF); break;
	case 'R': StoreReal(_R(F), KF); break;
	case 'B': StoreBool(_B(F), KF); break;
	}
}

void XString::PackKF(KeyFldD* KF)
{
	Clear();
	while (KF != nullptr) { StoreKF(KF); KF = KF->Chain; }
}

bool XString::PackFrml(FrmlList FL, KeyFldD* KF)
{
	FrmlPtr Z;
	Clear();
	while (FL != nullptr) {
		Z = FL->Frml;
		switch (KF->FldD->FrmlTyp) {
		case 'S':StoreStr(RunShortStr(Z), KF); break;
		case 'R':StoreReal(RunReal(Z), KF); break;
		case 'B':StoreBool(RunBool(Z), KF); break;
		}
		KF = KF->Chain; FL = FL->Chain;
	}
	return KF != nullptr;
}

WORD XPage::Off()
{
	if (IsLeaf) return oLeaf;
	else return oNotLeaf;
}

XItem* XPage::XI(WORD I)
{
	XItemPtr x; WORD o;
	x = XItemPtr(&A);
	o = Off();
	while (I > 1) { x = x->Next(o); I--; }
	return x;
}

uintptr_t XPage::EndOff()
{
	XItemPtr x;
	WORD* xofs = (WORD*)x; // absolute x
	x = XI(NItems + 1); return uintptr_t(xofs);
}

bool XPage::Underflow()
{
	return (EndOff() - uintptr_t(A)) < (XPageSize - XPageOverHead) / 2;
}

bool XPage::Overflow()
{
	return EndOff() - uintptr_t(this) > XPageSize;
}

pstring XPage::StrI(WORD I)
{
	XItemPtr x;
	WORD* xofs = (WORD*)x; // absolute x
	WORD o;
	pstring* s;

	x = XItemPtr(&A);
	o = Off();
	//TODO: asm les di, @result; mov s.WORD, di; mov s[2].WORD, es;

	if (I > NItems) s[0] = 0;
	else {
		for (WORD j = 1; j < I; j++) { *xofs = x->UpdStr(o, s); }
	}
	//TODO: co a jak to vrací?
	return "";
}

longint XPage::SumN()
{
	if (IsLeaf) { return NItems; }
	longint n = 0;
	XItemPtr x = XItemPtr(&A);
	WORD o = Off();
	for (WORD i = 1; i < NItems; i++) { n += x->GetN(); x = x->Next(o); }
	return n;
}

void XPage::Insert(WORD I, void* SS, XItem* XX)
{
	pstring* S = (pstring*)SS;
	XItemPtr x, x2;
	WORD* xofs = (WORD*)x;
	WORD* x2ofs = (WORD*)x2;
	WORD m, m2, o, oE, l, l2, sz;
	integer d;

	o = Off(); oE = EndOff();
	NItems++; x = XI(I);
	m = 0;
	if (I > 1) m = SLeadEqu(StrI(I - 1), *S);
	l = S->length() - m;
	sz = o + 2 + l;
	if (I < NItems) {
		x2 = x;
		m2 = SLeadEqu(StrI(I), *S);
		d = m2 - x->GetM(o);
		if (d > 0) {
			l2 = x->GetL(o); x2ofs += d;
			Move(x, x2, o);
			x2->PutM(o, m2);
			x2->PutL(o, l2 - d);
			sz -= d;
		}
		Move(x2, uintptr_t(x2) + x2ofs + sz, oE - *x2ofs);
	}
	XX = x;
	x->PutM(o, m); x->PutL(o, l);
	xofs += (o + 2);
	Move(&S[m + 1], x, l);
}

void XPage::InsDownIndex(WORD I, longint Page, XPage* P)
{
	pstring s;
	XItemPtr x = nullptr;
	s = P->StrI(P->NItems);
	Insert(I, &s, x);
	x->PutN(P->SumN());
	x->DownPage = Page;
}

void XPage::Delete(WORD I)
{
	XItemPtr x, x1, x2;
	WORD* xofs = (WORD*)x;
	WORD* x1ofs = (WORD*)x1;
	WORD* x2ofs = (WORD*)x2;
	WORD o = Off(); WORD oE = EndOff(); x = XI(I);
	if (I < NItems) {
		x2 = x->Next(o);
		integer d = x2->GetM(o) - x->GetM(o);
		if (d <= 0) Move(x2, x, oE - *x2ofs);
		else {
			Move(x2, x, o);
			x->PutL(o, x2->GetL(o) + d); x1 = x;
			*x1ofs = *x1ofs + o + 2 + d;
			*x2ofs = *x2ofs + o + 2;
			Move(x2, x1, oE - *x2ofs);
		}
		x = XI(NItems);
	}
	FillChar(x, oE - *xofs, 0);
	NItems--;
}

void XPage::AddPage(XPage* P)
{
	XItemPtr x, x1;
	WORD* xofs = (WORD*)x;

	GreaterPage = P->GreaterPage;
	if (P->NItems == 0) return;
	XItemPtr xE = XI(NItems + 1);
	WORD oE = P->EndOff(); WORD o = Off(); x = XItemPtr(&P->A);
	if (NItems > 0) {
		WORD m = SLeadEqu(StrI(NItems), P->StrI(1));
		if (m > 0) {
			WORD l = x->GetL(o) - m;
			x1 = x;
			xofs += m;
			Move(x1, x, o);
			x->PutM(o, m); x->PutL(o, l);
		}
	}
	Move(x, xE, oE - *xofs);
	NItems += P->NItems;
}

void XPage::SplitPage(XPage* P, longint ThisPage)
{
	// figuruje tady pstring* s, ale výsledek se nikam neukládá, je to zakomentované

	XItemPtr x, x1, x2;
	WORD* xofs = (WORD*)x;
	WORD* x1ofs = (WORD*)x1;
	WORD* x2ofs = (WORD*)x2;
	WORD o, oA, oE, n;
	pstring* s;

	x = XItemPtr(&A); x1 = x; o = Off(); oA = *xofs; oE = EndOff(); n = 0;
	while (*xofs - oA < oE - *xofs + x->GetM(o)) { x = x->Next(o); n++; }
	FillChar(P, XPageSize, 0);
	Move(x1, P->A, *xofs - oA);
	//s = (pstring*)(uintptr_t(x1) + oA + o + 1);;
	//s = &StrI(n + 1);
	Move(x, x1, o);
	x1->PutM(o, 0);
	x1 = x1->Next(o); x = x->Next(o); Move(x, x1, oE - *xofs);
	P->NItems = n; NItems -= n; *xofs = EndOff(); FillChar(x, oE - *xofs, 0);
	if (IsLeaf) P->GreaterPage = ThisPage; else P->GreaterPage = 0;
	P->IsLeaf = IsLeaf;
}

XWFile* XKey::XF()
{
	if (InWork) return &XWork;
	return CFile->XF;
}

longint XKey::NRecs()
{
	if (InWork) return NR;
	return CFile->XF->NRecs;
}

bool XKey::Search(XString& XX, bool AfterEqu, longint& RecNr)
{
	bool searchResult = false;
	XPagePtr p;
	WORD iItem = 0;
	char result;
	{
		p = (XPage*)GetStore(XPageSize);
	label1:
		XPathN = 1; longint page = IndexRoot; AfterEqu = AfterEqu && Duplic;
		XPath[XPathN].Page = page;
		XF()->RdPage(p, page);
		XItemPtr x = XItemPtr(p->A);
		WORD o = p->Off();
		WORD nItems = p->NItems;
		if (nItems == 0) {
			RecNr = CFile->NRecs + 1;
			XPath[1].I = 1;
			goto label2;
		}

		//__asm {
		//	push ds;
		//	cld;
		//	les dx, x;
		//	mov iItem, 1;
		//}
		
		//asm
		//	push ds; cld; les bx, x; mov iItem, 1; mov dx, 1;
		//@@add 1 bx, o;xor ax, ax; mov al, es: [bx] ; cmp dx, ax; jna @@5; /*first different <= prefix length?*/
		//mov dx, ax; lds si, XX;xor ax, ax; lodsb; sub ax, dx; add si, dx;
		//mov ah, es: [bx + 1] ; /*pstring length*/ lea di, [bx + 2]; /*pstring addr*/
		//xor cx, cx; mov cl, ah; cmp ah, al; jna @@2; mov cl, al;  /*min length*/
		//@@add 2 dx, cx;xor ch, ch; /*set zero flag*/
		//repe cmpsb; jb @@8; ja @@4; cmp al, ah; jb @@8; ja @@3;
		//cmp AfterEqu, 0; je @@7;
		//@@inc 3 dx;
		//@@sub 4 dx, cx;
		//@@mov 5 ax, iItem; cmp ax, nItems; je @@6; /*last item?*/
		//inc ax; mov iItem, ax;
		//xor ax, ax; mov al, es: [bx + 1] ; add ax, 2; add bx, ax;  /*next item*/
		//jmp @@1;
		//@@mov 6 al, _gt; inc iItem; jmp @@9;
		//@@mov 7 al, _equ; jmp @@9;
		//@@mov 8 al, _lt;
		//@@mov 9 result, al; sub bx, o; mov x.WORD, bx; pop ds; }
		XPath[XPathN].I = iItem;
		if (p->IsLeaf) {
			if (iItem > nItems) RecNr = CFile->NRecs + 1; else RecNr = x->GetN();
			if (searchResult == _equ)
				if
#ifdef FandSQL
					!CFile->IsSQLFile&&
#endif
					(((RecNr == 0) || (RecNr > CFile->NRecs))) XF()->Err(833);
				else searchResult = true;
			else
				label2:
			searchResult = false;
			ReleaseStore(p);
			return searchResult;
		}
		if (iItem > nItems) page = p->GreaterPage;
		else page = x->DownPage;
		XPathN++;
		goto label1;
	}
	return searchResult;
}

bool XKey::SearchIntvl(XString& XX, bool AfterEqu, longint& RecNr)
{
	return Search(XX, AfterEqu, RecNr) || Intervaltest && (RecNr <= CFile->NRecs);
}

longint XKey::PathToNr()
{
	WORD i, j; longint n; XPagePtr p; XItemPtr x;
	p = (XPage*)GetStore(XPageSize); n = 0;
	for (j = 1; j < XPathN - 1; j++)
	{
		XF()->RdPage(p, XPath[j].Page);
		x = XItemPtr(p->A);
		for (i = 1; i < XPath[j].I - 1; i++) { (n += x->GetN()); x = x->Next(oNotLeaf); };
	}
	n += XPath[XPathN].I;
	if (n > NRecs() + 1) XF()->Err(834);
	ReleaseStore(p);
	return n;
}

void XKey::NrToPath(longint I)
{
	XPagePtr p = (XPage*)GetStore(XPageSize);
	longint page = IndexRoot; XPathN = 0;
label1:
	XF()->RdPage(p, page); XPathN++; XPath[XPathN].Page = page;
	if (p->IsLeaf) {
		if (I > p->NItems + 1) XF()->Err(837);
		XPath[XPathN].I = I;
		ReleaseStore(p);
		return;
	}
	XItemPtr x = XItemPtr(p->A);
	for (WORD j = 1; j < p->NItems; j++) {
		if (I <= x->GetN()) { XPath[XPathN].I = j; page = x->DownPage; goto label1; }
		I -= x->GetN();
		x = x->Next(oNotLeaf);
	}
	XPath[XPathN].I = p->NItems + 1;
	page = p->GreaterPage;
	goto label1;
}

longint XKey::PathToRecNr()
{
	auto X = XPath[XPathN];
	XPagePtr p = (XPage*)GetStore(XPageSize); /* !!! with XPath[XPathN] do!!! */
	XF()->RdPage(p, X.Page);
	longint recnr = p->XI(X.I)->GetN();
	longint result = recnr;
	if ((recnr == 0) || (recnr > CFile->NRecs)) XF()->Err(835);
	ReleaseStore(p);
	return result;
}

bool XKey::RecNrToPath(XString& XX, longint RecNr)
{
	bool result;
	XPagePtr p; XItemPtr x; longint n;
	XX.PackKF(KFlds); Search(XX, false, n);
	p = (XPage*)GetStore(XPageSize);
	result = false; /* !!! with XPath[XPathN] do!!! */
	{
		auto X = XPath[XPathN];
	label1:
		XF()->RdPage(p, X.Page);
		x = p->XI(X.I);
		if (!(p->StrI(X.I) == XX.S)) goto label3;
	label2:
		if (x->GetN() == RecNr) { result = true; goto label3; }
		X.I++;
		if (X.I > p->NItems) {
			if (IncPath(XPathN - 1, X.Page)) { X.I = 1; goto label1; }
		}
		else {
			x = x->Next(oLeaf); if (x->GetL(oLeaf) != 0) goto label3; goto label2;
		}; }
label3:
	ReleaseStore(p);
	return result;
}

bool XKey::IncPath(WORD J, longint& Pg)
{
	bool result;
	XPagePtr p;
	p = (XPage*)GetStore(XPageSize);
	result = false;
	auto X = XPath[J];
	if (J == 0) { goto label2; } /* !!! with XPath[J] do!!! */
	{
	label1:
		XF()->RdPage(p, X.Page);
		if (X.I > p->NItems)
			if (IncPath(J - 1, X.Page)) { X.I = 0; goto label1; }
			else goto label2;
		X.I++;
		if (X.I > p->NItems)
			if (p->GreaterPage == 0) {
				X.I = 0; if (IncPath(J - 1, X.Page)) goto label1; goto label2;
			}
			else Pg = p->GreaterPage;
		else Pg = p->XI(X.I)->DownPage;
	}
	result = true;
label2:
	ReleaseStore(p);
	return result;
}

longint XKey::NrToRecNr(longint I)
{
	NrToPath(I);
	return PathToRecNr();
}

pstring XKey::NrToStr(longint I)
{
	pstring result;
	XPagePtr p = (XPage*)GetStore(XPageSize); NrToPath(I); /* !!! with XPath[XPathN] do!!! */
	XF()->RdPage(p, XPath[XPathN].Page);
	result = p->StrI(I);
	ReleaseStore(p);
	return result;
}

longint XKey::RecNrToNr(longint RecNr)
{
	XString x;
	if (RecNrToPath(x, RecNr)) return PathToNr(); else return 0;
}

bool XKey::FindNr(XString& X, longint& IndexNr)
{
	longint n;
	auto result = Search(X, false, n);
	IndexNr = PathToNr();
	return result;
}

void XKey::InsertOnPath(XString& XX, longint RecNr)
{
	WORD i, j;
	longint page, page1, uppage, downpage;
	XItemPtr x;
	longint n, upsum;
	XPagePtr p, p1, upp;

	p = (XPage*)GetStore(2 * XPageSize);
	p1 = (XPage*)GetStore(2 * XPageSize);
	upp = (XPage*)GetStore(2 * XPageSize);
	for (j = XPathN; j > 1; j--) {
		page = XPath[j].Page; XF()->RdPage(p, page); i = XPath[j].I;
		if (p->IsLeaf) {
			InsertItem(XX, p, upp, page, i, x, uppage); x->PutN(RecNr);
		}
		else {
			if (i <= p->NItems) {
				x = p->XI(i); n = x->GetN() + 1; if (uppage != 0) n -= upsum;
				x->PutN(n);
			}
			if (uppage != 0) {
				downpage = uppage; InsertItem(XX, p, upp, page, i, x, uppage);
				x->DownPage = downpage; x->PutN(upsum);
			};
		}
		XF()->WrPage(p, page); if (uppage != 0) {
			XF()->WrPage(upp, uppage); upsum = upp->SumN();
			if (upp->IsLeaf) ChainPrevLeaf(p1, uppage);
		}
	}
	if (uppage != 0) {
		page1 = XF()->NewPage(p1); p1->GreaterPage = page1;
		p1->InsDownIndex(1, uppage, upp); XF()->WrPage(p, page1); XF()->WrPage(p1, page);
		if (upp->IsLeaf) {
			upp->GreaterPage = page1; XF()->WrPage(upp, uppage);
		}
	}
	ReleaseStore(p);
}

void XKey::InsertItem(XString& XX, XPage* P, XPage* UpP, longint Page, WORD I, XItemPtr& X, longint& UpPage)
{
	P->Insert(I, &XX.S, X);
	UpPage = 0;
	if (P->Overflow()) {
		UpPage = XF()->NewPage(UpP);
		P->SplitPage(UpP, Page);
		if (I <= UpP->NItems) X = UpP->XI(I);
		else X = P->XI(I - UpP->NItems);
		XX.S = UpP->StrI(UpP->NItems);
	}
}

void XKey::ChainPrevLeaf(XPagePtr P, longint N)
{
	longint page; WORD i, j;
	for (j = XPathN - 1; j > 1; j--)
		if (XPath[j].I > 1) {
			XF()->RdPage(P, XPath[j].Page); i = XPath[j].I - 1;
		label1:
			page = P->XI(i)->DownPage; XF()->RdPage(P, page);
			if (P->IsLeaf) { P->GreaterPage = N; XF()->WrPage(P, page); return; }
			i = P->NItems; goto label1;
		}
}

bool XKey::Insert(longint RecNr, bool Try)
{
	longint N, XNr; XString x;
	x.PackKF(KFlds);
	if (Search(x, true, N)) {
		if (Try) { return false; }
	}
	else
	{
		XFNotValid();
		CFileError(822);
	}
	InsertOnPath(x, RecNr);
	return true;
}

void XKey::DeleteOnPath()
{
	longint page, page1, page2;
	longint uppage = 0;
	void* pp;
	XItem* x = nullptr;
	bool released;
	longint n;

	MarkStore(pp);
	XPage* p = (XPage*)GetStore(2 * XPageSize);
	XPage* p1 = (XPage*)GetStore(2 * XPageSize);
	XPage* p2 = (XPage*)GetStore(2 * XPageSize);
	XPage* upp = p2;
	for (WORD j = XPathN; j > 1; j--) {
		page = XPath[j].Page; XF()->RdPage(p, page); WORD i = XPath[j].I;
		if (p->IsLeaf) p->Delete(i);
		else if (upp->Underflow()) {
			XF()->WrPage(upp, uppage);
			WORD i1 = i - 1;
			WORD i2 = i;
			if (i1 == 0) { i1 = 1; i2 = 2; }
			XIDown(p, p1, i1, page1); XIDown(p, p2, i2, page2);
			BalancePages(p1, p2, released);
			XF()->WrPage(p1, page1); p->Delete(i1);
			if (released) {
				XF()->ReleasePage(p2, page2);
				if (i1 > p->NItems) p->GreaterPage = page1;
				else { p->InsDownIndex(i1, page1, p1); p->Delete(i2); }
			}
			else {
				XF()->WrPage(p2, page2); p->InsDownIndex(i1, page1, p1);
				if (i2 <= p->NItems) {
					p->Delete(i2); p->InsDownIndex(i2, page2, p2);
				}
			}
		}
		else {
			if (upp->Overflow()) {
				page1 = XF()->NewPage(p1); upp->SplitPage(p1, uppage);
				XF()->WrPage(p1, page1); p->InsDownIndex(i, page1, p1); i++;
			}
			XF()->WrPage(upp, uppage);
			if (i <= p->NItems) {
				p->Delete(i); p->InsDownIndex(i, uppage, upp);
			}
		}
		uppage = page;
		XPage* px = upp;
		upp = p;
		p = px;
	}
	if (upp->Overflow()) {
		page1 = XF()->NewPage(p1); upp->SplitPage(p1, uppage); page = XF()->NewPage(p);
		p->GreaterPage = page; p->InsDownIndex(1, page1, p1);
		XF()->WrPage(p1, page1); XF()->WrPage(p, uppage); XF()->WrPage(upp, page);
	}
	else {
		page1 = upp->GreaterPage;
		if ((upp->NItems == 0) && (page1 > 0)) {
			XF()->RdPage(p1, page1); Move(p1, upp, XPageSize);
			XF()->ReleasePage(p1, page1);
		}
		XF()->WrPage(upp, uppage);
	}
	ReleaseStore(pp);
}

void XKey::BalancePages(XPage* P1, XPage* P2, bool& Released)
{
	longint n; WORD sz;
	n = P1->GreaterPage;
	P1->AddPage(P2);
	sz = P1->EndOff() - uintptr_t(P1);
	if (sz <= XPageSize) Released = true;
	else {
		Released = false; Move(P1, P2, sz);
		P2->SplitPage(P1, n);
	}
}

void XKey::XIDown(XPage* P, XPage* P1, WORD I, longint& Page1)
{
	if (I > P->NItems) Page1 = P->GreaterPage;
	else Page1 = P->XI(I)->DownPage;
	XF()->RdPage(P1, Page1);
}

bool XKey::Delete(longint RecNr)
{
	XString xx;
	bool b = RecNrToPath(xx, RecNr);
	if (b) DeleteOnPath();
	return b;
}

void XWKey::Open(KeyFldD* KF, bool Dupl, bool Intvl)
{
	KFlds = KF; Duplic = Dupl; InWork = true; Intervaltest = Intvl; NR = 0;
	XPagePtr p = (XPage*)GetStore(sizeof(p)); IndexRoot = XF()->NewPage(p);
	p->IsLeaf = true; XF()->WrPage(p, IndexRoot); ReleaseStore(p); IndexLen = 0;
	while (KF != nullptr) {
		IndexLen += KF->FldD->NBytes;
		KF = KF->Chain;
	}
}

void XWKey::Close()
{
	ReleaseTree(IndexRoot, true); IndexRoot = 0;
}

void XWKey::Release()
{
	ReleaseTree(IndexRoot, false); NR = 0;
}

void XWKey::ReleaseTree(longint Page, bool IsClose)
{
	if ((Page == 0) || (Page > XF()->MaxPage)) return;
	XPagePtr p = (XPage*)GetStore(XPageSize);
	XF()->RdPage(p, Page);
	if (not p->IsLeaf) {
		WORD n = p->NItems;
		for (WORD i = 1; i < n; i++) {
			ReleaseTree(p->XI(i)->DownPage, IsClose);
			XF()->RdPage(p, Page);
		}
		if (p->GreaterPage != 0) ReleaseTree(p->GreaterPage, IsClose);
	}
	if ((Page != IndexRoot) || IsClose)
		XF()->ReleasePage(p, Page);
	else {
		FillChar(p, XPageSize, 0); p->IsLeaf = true; XF()->WrPage(p, Page);
	}
	ReleaseStore(p);
}

void XWKey::OneRecIdx(KeyFldD* KF, longint N)
{
	Open(KF, true, false); Insert(N, true); NR++;
}

void XWKey::InsertAtNr(longint I, longint RecNr)
{
	XString x;
	x.PackKF(KFlds); NR++; NrToPath(I); InsertOnPath(x, RecNr);
}

longint XWKey::InsertGetNr(longint RecNr)
{
	XString x; longint n;
	NR++; x.PackKF(KFlds); Search(x, true, n);
	auto result = PathToNr();
	InsertOnPath(x, RecNr);
	return result;
}

void XWKey::DeleteAtNr(longint I)
{
	NrToPath(I); DeleteOnPath(); NR--;
}

void XWKey::AddToRecNr(longint RecNr, integer Dif)
{
	if (NRecs() == 0) return;
	NrToPath(1);
	XPagePtr p = (XPage*)GetStore(sizeof(*p)); /* !!! with XPath[XPathN] do!!! */
	longint pg = XPath[XPathN].Page;
	integer j = XPath[XPathN].I;
	do {
		XF()->RdPage(p, pg);
		integer n = p->NItems - j + 1;
		XItemPtr x = p->XI(j);
		while (n > 0) {
			longint nn = x->GetN();
			if (nn >= RecNr) x->PutN(nn + Dif);
			x = x->Next(oLeaf); n--;
		}
		XF()->WrPage(p, pg); pg = p->GreaterPage; j = 1;
	} while (pg != 0);
	ReleaseStore(p);
}

void XWFile::Err(WORD N)
{
	if (this == &XWork) { SetMsgPar(FandWorkXName); RunError(N); }
	else {
		CFile->XF->SetNotValid();
		CFileMsg(N, 'X');
		CloseGoExit();
	}
}

void XWFile::TestErr()
{
	if (HandleError != 0) Err(700 + HandleError);
}

longint XWFile::UsedFileSize()
{
	return longint(MaxPage + 1) << XPageShft;
}

bool XWFile::NotCached()
{
	return (this != &XWork) && CFile->NotCached();
}

void XWFile::RdPage(XPagePtr P, longint N)
{
	if ((N == 0) || (N > MaxPage)) Err(831);
	RdWrCache(true, Handle, NotCached(), N << XPageShft, XPageSize, P);
}

void XWFile::WrPage(XPagePtr P, longint N)
{
	if (UpdLockCnt > 0) Err(645);
	RdWrCache(false, Handle, NotCached(), N << XPageShft, XPageSize, P);
}

longint XWFile::NewPage(XPagePtr P)
{
	longint result = 0;
	if (FreeRoot != 0) {
		result = FreeRoot;
		RdPage(P, FreeRoot);
		FreeRoot = P->GreaterPage;
	}
	else {
		MaxPage++;
		if (MaxPage > 0x1fffff) Err(887);
		result = MaxPage;
	}
	FillChar(P, XPageSize, 0);
	return result;
}

void XWFile::ReleasePage(XPagePtr P, longint N)
{
	FillChar(P, XPageSize, 0);
	P->GreaterPage = FreeRoot;
	FreeRoot = N;
	WrPage(P, N);
}

void XFile::SetEmpty()
{
	auto p = (XPage*)GetZStore(XPageSize);
	WrPage(p, 0);
	p->IsLeaf = true; FreeRoot = 0; NRecs = 0;
	KeyDPtr k = CFile->Keys;
	while (k != nullptr) {
		longint n = k->IndexRoot;
		MaxPage = n;
		WrPage(p, n);
		k = k->Chain;
	}
	ReleaseStore(p);
	WrPrefix();
}

void XFile::RdPrefix()
{
	RdWrCache(true, Handle, NotCached(), 2, 18, &FreeRoot);
}

void XFile::WrPrefix()
{
	WORD Signum = 0x04FF;
	RdWrCache(false, Handle, NotCached(), 0, 2, &Signum);
	NRecsAbs = CFile->NRecs; NrKeys = CFile->GetNrKeys();
	RdWrCache(false, Handle, NotCached(), 2, 18, &FreeRoot);
}

void XFile::SetNotValid()
{
	NotValid = true; MaxPage = 0; WrPrefix(); SaveCache(0);
}

XScan::XScan(FileD* aFD, KeyD* aKey, KeyInD* aKIRoot, bool aWithT)
{
	FD = aFD; Key = aKey; KIRoot = aKIRoot; withT = aWithT;
#ifdef FandSQL
	if (aFD->IsSQLFile) {
		if ((aKey != nullptr) && aKey->InWork) { P = (XPage*)GetStore(XPageSize); Kind = 3; }
		else Kind = 4;
	}
	else
#endif
	{
		P = (XPage*)GetStore(XPageSize);
		Kind = 1;
		if (aKIRoot != nullptr) Kind = 2;
	}
}

void XScan::Reset(FrmlPtr ABool, bool SQLFilter)
{
	KeyInD* k; longint n; XString xx; bool b;
	CFile = FD;
	Bool = ABool;
	if (SQLFilter) {
		if (CFile->IsSQLFile) hasSQLFilter = true;
		else Bool = nullptr;
	}
	switch (Kind) {
	case 0: NRecs = CFile->NRecs; break;
	case 1:
	case 3: { if (!Key->InWork) TestXFExist(); NRecs = Key->NRecs(); break; }
	case 2: {
		if (!Key->InWork) TestXFExist();
		CompKIFrml(Key, KIRoot, true); NRecs = 0; k = KIRoot;
		while (k != nullptr) {
			XString a1;
			XString b2;
			a1.S = *k->X1;
			b2.S = *k->X2;
			Key->FindNr(a1, k->XNrBeg);
			b = Key->FindNr(b2, n);
			k->N = 0;
			if (n >= k->XNrBeg) k->N = n - k->XNrBeg + b;
			NRecs += k->N;
			k = k->Chain;
		}
		break; }
#ifdef FandSQL
	case 4: { CompKIFrml(Key, KIRoot, false); New(SQLStreamPtr(Strm), Init); IRec = 1; break; }
#endif
	}
	SeekRec(0);
}

void XScan::ResetSort(KeyFldDPtr aSK, FrmlPtr& BoolZ, LockMode OldMd, bool SQLFilter)
{
	LockMode m;
	if (Kind == 4) {
		SK = aSK;
		if (SQLFilter) { Reset(BoolZ, true); BoolZ = nullptr; }
		else Reset(nullptr, false);
		return;
	}
	if (aSK != nullptr) {
		Reset(BoolZ, false);
		ScanSubstWIndex(this, aSK, 'S');
		BoolZ = nullptr;
	}
	else Reset(nullptr, false);
	/* !!! with CFile^ do!!! */
	if (CFile->NotCached()) {
		switch (Kind) {
		case 0: { m = NoCrMode; if (CFile->XF != nullptr) m = NoExclMode; break; }
		case 1: { m = OldMd; if (Key->InWork) m = NoExclMode; break; }
		default: return;
		}
		m = LockMode(MaxW(m, OldMd));
		if (m != OldMd) ChangeLMode(m, 0, true);
	}
}

void XScan::SubstWIndex(WKeyDPtr WK)
{
	Key = WK; if (Kind != 3) Kind = 1;
	if (P == nullptr) P = (XPage*)GetStore(XPageSize);
	NRecs = Key->NRecs();
	Bool = nullptr; SeekRec(0); TempWX = true;
}

void XScan::ResetOwner(XString* XX, FrmlPtr aBool)
{
	longint n; bool b;
	CFile = FD; Bool = aBool;
#ifdef FandSQL
	if (Kind = 4) {           /* !on .SQL with Workindex */
		KIRoot = GetZStore(sizeof(KIRoot^));
		KIRoot->X1 = StoreStr(XX->S); KIRoot->X2 = StoreStr(XX->S);
		New(SQLStreamPtr(Strm), Init); IRec = 1
	}
	else
#endif
	{
		TestXFExist();
		KIRoot = (KeyInD*)GetZStore(sizeof(*KIRoot));
		Key->FindNr(*XX, KIRoot->XNrBeg);
		AddFFs(Key, XX->S);
		b = Key->FindNr(*XX, n); NRecs = n - KIRoot->XNrBeg + b;
		KIRoot->N = NRecs; Kind = 2;
	}
	SeekRec(0);
}

void XScan::ResetOwnerIndex(LinkDPtr LD, LocVar* LV, FrmlPtr aBool)
{
	WKeyDPtr k;
	CFile = FD; TestXFExist(); Bool = aBool; OwnerLV = LV; Kind = 2;
	if (!EquKFlds(WKeyDPtr(LV->RecPtr)->KFlds, LD->ToKey->KFlds)) RunError(1181);
	SeekRec(0);
}

#ifdef FandSQL
void XScan::ResetSQLTxt(FrmlPtr Z)
{
	LongStrPtr s;
	New(SQLStreamPtr(Strm), Init); s = RunLongStr(Z);
	SQLStreamPtr(Strm)->InpResetTxt(s); ReleaseStore(s);
	eof = false;
}
#endif

void XScan::ResetLV(void* aRP)
{
	Strm = aRP; Kind = 5; NRecs = 1;
}

void XScan::Close()
{
	CFile = FD;
#ifdef FandSQL
	if (Kind = 4) /* !!! with SQLStreamPtr(Strm)^ do!!! */ { InpClose; Done; }
#endif
	if (TempWX) WKeyDPtr(Key)->Close();
}

void XScan::SeekRec(longint I)
{
	KeyInD* k; FrmlPtr z;
	CFile = FD;

#ifdef FandSQL
	if (Kind == 4) {
		if (I != IRec) /* !!! with SQLStreamPtr(Strm)^ do!!! */
		{
			if (NotFrst) InpClose; NotFrst = true;
			if (hasSQLFilter) z = Bool else z = nullptr;
			InpReset(Key, SK, KIRoot, z, withT);
			EOF = AtEnd; IRec = 0; NRecs = 0x20000000;
		}
		return;
	}
#endif
	if ((Kind == 2) && (OwnerLV != nullptr)) {
		IRec = 0;
		NRecs = 0x20000000;
		iOKey = 0;
		NextIntvl();
		eof = I >= NRecs;
		return;
	}
	IRec = I;
	eof = I >= NRecs;
	if (!eof)
		switch (Kind) {
		case 1:
		case 3: {
			Key->NrToPath(I + 1); /* !!! with XPath[XPathN] do!!! */
			SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
			break;
		}
		case 2: {
			k = KIRoot;
			while (I >= k->N) { I -= k->N; k = k->Chain; }
			KI = k; SeekOnKI(I);
			break;
		}
		}
}

void XScan::GetRec()
{
	XString xx;
	CFile = FD;
#ifdef FandSQL
	if (Kind == 4) {
		repeat EOF = !SQLStreamPtr(Strm)->GetRec
			until EOF || hasSQLFilter || RunBool(Bool);
		inc(IRec); return;
	}
#endif
label1:
	eof = IRec >= NRecs;
	if (!eof) {
		IRec++;
		switch (Kind) {
		case 0: { RecNr = IRec; goto label2; break; }
		case 1:
		case 2: { RecNr = X->GetN(); NOnPg--;
			if (NOnPg > 0) X = X->Next(oLeaf);
			else if ((Kind == 2) && (NOfKI == 0)) NextIntvl();
			else if (P->GreaterPage > 0) SeekOnPage(P->GreaterPage, 1);
		label2:
			ReadRec(RecNr); if (DeletedFlag()) goto label1;
		label3:
			if (!RunBool(Bool)) goto label1;
		}
#ifdef FandSQL
		case 3: {
			NOnPg--;
			xx.S = P->StrI(P->NItems - NOnPg);
			if ((NOnPg == 0) && (P->GreaterPage > 0)) SeekOnPage(P->GreaterPage, 1);
			if (!Strm1->SelectXRec(Key, @xx, _equ, withT)) goto label1;
			goto label3;
			break;
		}
#endif
		case 5:
		{
			move(Strm, CRecPtr, CFile->RecLen + 1);
			break;
		}
		}
	}
}

void XScan::SeekOnKI(longint I)
{
	NOfKI = KI->N - I; Key->NrToPath(KI->XNrBeg + I);
	/* !!! with XPath[XPathN] do!!! */
	SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
}

void XScan::SeekOnPage(longint Page, WORD I)
{
	Key->XF()->RdPage(P, Page); NOnPg = P->NItems - I + 1;
	if (Kind == 2)
	{
		if (NOnPg > NOfKI) NOnPg = NOfKI;
		NOfKI -= NOnPg;
	}
	X = P->XI(I);
}

void XScan::NextIntvl()
{
	XString xx; bool b; longint n, nBeg; WKeyDPtr k;
	if (OwnerLV != nullptr) {
		k = WKeyDPtr(OwnerLV->RecPtr);
		while (iOKey < k->NRecs()) {
			iOKey++;
			CFile = OwnerLV->FD; xx.S = k->NrToStr(iOKey);
			CFile = FD;
			Key->FindNr(xx, nBeg); AddFFs(Key, xx.S);
			b = Key->FindNr(xx, n);
			n = n - nBeg + b;
			if (n > 0) {
				NOfKI = n;
				Key->NrToPath(nBeg); /* !!! with XPath[XPathN] do!!! */
				SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
				return;
			};
		}
		NRecs = IRec; /*EOF*/
	}
	else {
		do { KI = KI->Chain; } while ((KI != nullptr) || (KI->N > 0));
		if (KI != nullptr) SeekOnKI(0);
	}
}


pstring* FieldDMask(FieldDPtr F)
{
	return nullptr;
}

void* GetRecSpace()
{
	return GetZStore(CFile->RecLen + 2);
}

void* GetRecSpace2()
{
	return GetZStore2(CFile->RecLen + 2);
}

bool EquKFlds(KeyFldDPtr KF1, KeyFldDPtr KF2)
{
	bool result = false;
	while (KF1 != nullptr) {
		if ((KF2 == nullptr) || (KF1->CompLex != KF2->CompLex) || (KF1->Descend != KF2->Descend)
			|| (KF1->FldD->Name != KF2->FldD->Name)) return result;
		KF1 = KF1->Chain; KF2 = KF2->Chain;
	}
	if (KF2 != nullptr) return false;
	return true;
}

/// nedìlá nic, pùvodnì dìlal XOR 0xAA;
void Code(void* A, WORD L)
{
	return;
}

/// nedìlá nic
void XDecode(LongStrPtr S)
{
	return;
}

void CodingLongStr(LongStrPtr S)
{
	if (CFile->TF->LicenseNr == 0) Code(S->A, S->LL);
	else XDecode(S);
}

void DirMinusBackslash(pstring& D)
{
	if ((D.length() > 3) && (D[D.length() - 1] == '\\')) D[0]--;
}

longint StoreInTWork(LongStrPtr S)
{
	return TWork.Store(S);
}

LongStrPtr ReadDelInTWork(longint Pos)
{
	auto result = TWork.Read(1, Pos);
	TWork.Delete(Pos);
	return result;
}

void ForAllFDs(void(*procedure)())
{
	RdbDPtr R; FileDPtr cf;
	cf = CFile; R = CRdb;
	while (R != nullptr) {
		CFile = R->FD;
		while (CFile != nullptr) { procedure(); CFile = CFile->Chain; };
		R = R->ChainBack;
	}
	CFile = cf;
}

bool IsActiveRdb(FileDPtr FD)
{
	RdbDPtr R;
	R = CRdb; while (R != nullptr) {
		if (FD == R->FD) return true;
		R = R->ChainBack;
	}
	return false;
}

void ResetCompilePars()
{
	char a;
	RdFldNameFrml = RdFldNameFrmlF;
	RdFunction = nullptr;
	ChainSumEl = nullptr;
	FileVarsAllowed = true;
	FDLocVarAllowed = false;
	IdxLocVarAllowed = false;
	PrevCompInp = nullptr;
}


