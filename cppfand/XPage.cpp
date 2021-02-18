#include "XPage.h"

#include "base.h"


XPage::~XPage()
{
	for (size_t i = 0; i < _leafItems.size(); i++) {
		delete _leafItems[i];
	}
}

WORD XPage::Off()
{
	if (IsLeaf) return oLeaf;
	else return oNotLeaf;
}

XItem* XPage::XI(WORD I, bool isLeaf)
{
	_xItem = new XItem(A, isLeaf);
	WORD o = Off();
	while (I > 1) {
		auto oldXitem = _xItem;
		_xItem = _xItem->Next(o, isLeaf);
		delete oldXitem; oldXitem = nullptr;
		I--;
	}
	return _xItem;
}

WORD XPage::EndOff()
{
	return sizeof(A);
	XItem* x = XI(NItems + 1, IsLeaf);
	WORD* xofs = (WORD*)x; // absolute x
	return (uintptr_t)xofs;
}

bool XPage::Underflow()
{
	return (EndOff() - uintptr_t(A)) < (XPageSize - XPageOverHead) / 2;
}

bool XPage::Overflow()
{
	genItems();
	auto size = ItemsSize();
	return size > XPageSize;
}

pstring XPage::StrI(WORD I)
{
	pstring s; // toto bude vystup
	XItem* x = new XItem(A, IsLeaf);
	WORD xofs = 0;
	WORD o = Off();

	if (I > NItems) s[0] = 0;
	else {
		for (WORD j = 1; j <= I; j++) {
			xofs += x->UpdStr(o, &s);
			auto oldX = x;
			x = new XItem(&A[xofs], IsLeaf);
			delete oldX; oldX = nullptr;
		}
	}
	delete x; x = nullptr;
	return s;
}

longint XPage::SumN()
{
	if (IsLeaf) { return NItems; }
	longint n = 0;
	XItem* x = new XItem(A, IsLeaf);
	WORD o = Off();
	for (WORD i = 1; i < NItems; i++) {
		n += x->GetN();
		x = x->Next(o, IsLeaf);
	}
	return n;
}

void XPage::Insert(WORD I, void* SS, XItem** XX)
{
	genItems();
	pstring* S = (pstring*)SS;
	NItems++;
	auto x = &_leafItems[I - 1]; // vytahneme predchozi zaznam
	WORD m = 0;
	// zjistime spolecne casti s predchozim zaznamem
	if (I > 1) m = SLeadEqu(StrI(I - 1), *S);
	WORD l = S->length() - m;
	// vytvorime novou polozku s novym zaznamem
	auto newXi = XItemLeaf((unsigned int)I, m, l, *S);

	if (I < NItems) {
		// vkladany zaznam nebude posledni (nebude na konci)
		// zjistime spolecne casti s nasledujicim zaznamem
		WORD m2 = SLeadEqu(StrI(I), *S);
		integer d = m2 - newXi.M;
		if (d > 0) {
			printf("XPage::Insert() - Nutno doimplementovat!");
		}
	}

	size_t bufLen = newXi.size();
	BYTE* buf = new BYTE[bufLen];
	newXi.Serialize(buf, bufLen);

	// vratime tuto novou polozku
	*XX = new XItem(buf, IsLeaf);

	//pstring* S = (pstring*)SS;
	//
	//WORD xofs = 0; // posun pro x
	//WORD x2ofs = 0; // posun pro x2
	//
	//WORD o = Off();
	//WORD oE = EndOff();
	//NItems++;
	//XItem* x = XI(I, IsLeaf);
	//WORD m = 0;
	//// zjistime spolecne casti s predchozim zaznamem
	//if (I > 1) m = SLeadEqu(StrI(I - 1), *S);
	//WORD l = S->length() - m;
	//WORD sz = o + 2 + l;
	//if (I < NItems) {
	//	// vkladany zaznam nebude posledni (nebude na konci)
	//	XItem* x2 = x;
	//	// zjistime spolecne casti s nasledujicim zaznamem
	//	WORD m2 = SLeadEqu(StrI(I), *S);
	//	integer d = m2 - x->GetM(o);
	//	if (d > 0) {
	//		WORD l2 = x->GetL(o);
	//		x2ofs += d;
	//		Move(x, x2, o);
	//		x2->PutM(o, m2);
	//		x2->PutL(o, l2 - d);
	//		sz -= d;
	//	}
	//	// Move(x2, uintptr_t(x2) + x2ofs + sz, oE - *x2ofs);
	//}
	//*XX = x;
	//x->PutM(o, m);
	//x->PutL(o, l);
	////xofs += (o + 2);
	//memcpy(&x->Nr[0], &(*S)[m + 1], l);
}

void XPage::InsertLeaf(unsigned int RecNr, size_t I, pstring& SS)
{
	genItems();
	NItems++;
	WORD m = 0;
	// zjistime spolecne casti s predchozim zaznamem
	if (I > 1) m = SLeadEqu(StrI(I - 1), SS);
	WORD l = SS.length() - m;
	// vytvorime novou polozku s novym zaznamem a vlozime ji do vektoru

	auto newXi = new XItemLeaf(RecNr, m, l, SS);
	_addToItems(newXi, I - 1);

	if (I < NItems) {
		// vkladany zaznam nebude posledni (nebude na konci)
		// zjistime spolecne casti s nasledujicim zaznamem
		WORD m2 = SLeadEqu(StrI(I), SS);
		BYTE d = m2 - newXi->M;
		if (d > 0) {
			// puvodni polozka je ted na pozici I (nova je na I - 1)
			_cutLeafItem(I, d);
		}
	}
}

void XPage::InsDownIndex(WORD I, longint Page, XPage* P)
{
	pstring s;
	XItem* x = nullptr;
	s = P->StrI(P->NItems);
	Insert(I, &s, &x);
	x->PutN(P->SumN());
	*(x->DownPage) = Page;
}

void XPage::Delete(WORD I)
{
	genItems();
	// polozka ke smazani
	auto Xi = _leafItems[I - 1];
	if (I < NItems) {
		// tato polozka (I - 1) neni posledni, bude se asi muset upravovat polozka za ni (I)
		auto nextXi = _leafItems[I];
		integer d = nextXi->M - Xi->M;
		if (d > 0) {
			// nasledujici polozku je nutne upravit
			_enhLeafItem(I, d);
		}
	}

	// polozku smazeme z vektoru
	_leafItems.erase(_leafItems.begin() + (I - 1));

	// polozku smazeme z pameti
	delete Xi; Xi = nullptr;

	// pregenerujeme pole
	NItems--;
	GenArrayFromVectorItems();

	//XItemPtr x = nullptr, x1 = nullptr, x2 = nullptr;
	//WORD* xofs = (WORD*)x;
	//WORD* x1ofs = (WORD*)x1;
	//WORD* x2ofs = (WORD*)x2;
	//WORD o = Off(); WORD oE = EndOff(); x = XI(I, IsLeaf);
	//if (I < NItems) {
	//	x2 = x->Next(o, IsLeaf);
	//	integer d = x2->GetM(o) - x->GetM(o);
	//	if (d <= 0) Move(x2, x, oE - *x2ofs);
	//	else {
	//		Move(x2, x, o);
	//		x->PutL(o, x2->GetL(o) + d); x1 = x;
	//		*x1ofs = *x1ofs + o + 2 + d;
	//		*x2ofs = *x2ofs + o + 2;
	//		Move(x2, x1, oE - *x2ofs);
	//	}
	//	x = XI(NItems, IsLeaf);
	//}
	//FillChar(x, oE - *xofs, 0);
	//NItems--;
}

void XPage::AddPage(XPage* P)
{
	XItemPtr x = nullptr, x1 = nullptr;
	WORD* xofs = (WORD*)x;

	GreaterPage = P->GreaterPage;
	if (P->NItems == 0) return;
	XItemPtr xE = XI(NItems + 1, IsLeaf);
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

	XItemPtr x = nullptr, x1 = nullptr, x2 = nullptr;
	WORD* xofs = (WORD*)x;
	WORD* x1ofs = (WORD*)x1;
	WORD* x2ofs = (WORD*)x2;
	WORD o, oA, oE, n;
	pstring* s;

	x = XItemPtr(&A);
	x1 = x; o = Off();
	oA = *xofs;
	oE = EndOff();
	n = 0;
	while (*xofs - oA < oE - *xofs + x->GetM(o)) { x = x->Next(o, IsLeaf); n++; }
	FillChar(P, XPageSize, 0);
	Move(x1, P->A, *xofs - oA);
	//s = (pstring*)(uintptr_t(x1) + oA + o + 1);;
	//s = &StrI(n + 1);
	Move(x, x1, o);
	x1->PutM(o, 0);
	x1 = x1->Next(o, IsLeaf);
	x = x->Next(o, IsLeaf);
	Move(x, x1, oE - *xofs);
	P->NItems = n; NItems -= n;
	*xofs = EndOff();
	FillChar(x, oE - *xofs, 0);
	if (IsLeaf) P->GreaterPage = ThisPage;
	else P->GreaterPage = 0;
	P->IsLeaf = IsLeaf;
}

void XPage::Clean()
{
	IsLeaf = false;
	GreaterPage = 0;
	WORD NItems = 0;
	memset(A, 0, XPageSize - 4);
}

size_t XPage::ItemsSize()
{
	size_t count = 0;
	for (auto xi : _leafItems) {
		count += xi->size();
	}
	return count;
}

void XPage::genItems()
{
	_leafItems.clear();
	size_t offset = 0;
	for (WORD i = 0; i < NItems; i++)
	{
		auto x = new XItemLeaf(&A[offset]);
		offset += x->size();
		_leafItems.push_back(std::move(x));
	}
}

void XPage::GenArrayFromVectorItems()
{
	memset(A, 0, sizeof(A));
	size_t offset = 0;
	BYTE buffer[256];
	for (auto&& item : _leafItems)
	{
		size_t len = item->Serialize(buffer, sizeof(buffer));
		memcpy(&A[offset], buffer, len);
		offset += len;
	}
	NItems = _leafItems.size();
}

std::vector<XItemLeaf*>::iterator XPage::_addToItems(XItemLeaf* xi, size_t pos)
{
	return _leafItems.insert(_leafItems.begin() + pos, std::move(xi));
}

bool XPage::_cutLeafItem(size_t iIndex, BYTE length)
{
	if (_leafItems.size() < iIndex + 1) return false; // polozka neexistuje
	auto Xi = _leafItems[iIndex];
	if (length > Xi->L) return false; // polozka neni tak dlouha, delka by byla zaporna
	Xi->M += length;
	Xi->L -= length;
	auto origData = Xi->data;
	Xi->data = new BYTE[Xi->L];
	memcpy(Xi->data, &origData[length], Xi->L);
	delete origData; origData = nullptr;
	return true;
}

bool XPage::_enhLeafItem(size_t iIndex, BYTE length)
{
	if (_leafItems.size() < iIndex + 1) return false; // polozka neexistuje
	auto prevXi = _leafItems[iIndex - 1]; // mazana polozka, z teto budeme brat data
	auto Xi = _leafItems[iIndex]; // aktualizovana (rozsirovana) polozka	
	Xi->M -= length;
	Xi->L += length;
	auto origData = Xi->data;
	Xi->data = new BYTE[Xi->L];
	memcpy(Xi->data, &prevXi->data[prevXi->L - length - 1], length); // z predchoziho zaznamu zkopirujeme X poslednich Bytu
	memcpy(&Xi->data[length], origData, Xi->L - length); // a doplnime je puvodnimi daty
	delete origData; origData = nullptr;
	return true;
}
