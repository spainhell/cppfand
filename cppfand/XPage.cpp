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
	Deserialize();
	auto size = ItemsSize();
	return size > XPageSize;
}


/// i = 1 .. N
pstring XPage::GetKey(WORD i)
{
	pstring s; // toto bude vystup
	XItem* x = new XItem(A, IsLeaf);
	WORD xofs = 0;
	WORD o = Off();

	if (i > NItems) s[0] = 0;
	else {
		for (WORD j = 1; j <= i; j++) {
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

void XPage::InsertNonLeaf(WORD I, void* SS, XItem** XX, size_t& XXLen)
{
	if (_leafItems.empty()) Deserialize();
	pstring* S = (pstring*)SS;
	std::unique_ptr<XItemLeaf> newXi;
	NItems++;
	if (!_leafItems.empty()) {
		// predchozi zaznam existuje -> vytahneme jej
		auto x = &_leafItems[I - 1];
		WORD m = 0;
		// zjistime spolecne casti s predchozim zaznamem
		if (I > 1) m = SLeadEqu(GetKey(I - 1), *S);
		WORD l = S->length() - m;
		// vytvorime novou polozku s novym zaznamem
		newXi = std::make_unique<XItemLeaf>((unsigned int)I, m, l, *S);
	}
	else {
		// vytvorime 1. polozku s novym zaznamem
		newXi = std::make_unique<XItemLeaf>((unsigned int)I, 0, S->length(), *S);
	}

	if (I < NItems) {
		// vkladany zaznam nebude posledni (nebude na konci)
		// zjistime spolecne casti s nasledujicim zaznamem
		WORD m2 = SLeadEqu(GetKey(I), *S);
		integer d = m2 - newXi->M;
		if (d > 0) {
			printf("XPage::InsertNonLeaf() - Nutno doimplementovat!");
		}
	}

	if (IsLeaf) {
		size_t bufLen = newXi->size();
		XXLen = bufLen;
		BYTE* buf = new BYTE[bufLen];
		newXi->Serialize(buf, bufLen);

		// vratime tuto novou polozku
		*XX = new XItem(buf, IsLeaf);
	}
	else {
		size_t bufLen = newXi->size() + 4; // nonLeaf is 2 B greater then Leaf item
		XXLen = bufLen;
		BYTE* buf = new BYTE[bufLen];
		newXi->Serialize(&buf[4], bufLen);

		// vratime tuto novou polozku
		*XX = new XItem(buf, IsLeaf);
	}
}

void XPage::InsertLeaf(unsigned int RecNr, size_t I, pstring& SS)
{
	Deserialize();
	NItems++;
	WORD m = 0;
	// zjistime spolecne casti s predchozim zaznamem
	if (I > 1) m = SLeadEqu(GetKey(I - 1), SS);
	WORD l = SS.length() - m;
	// vytvorime novou polozku s novym zaznamem a vlozime ji do vektoru

	auto newXi = new XItemLeaf(RecNr, m, l, SS);
	_addToLeafItems(newXi, I - 1);

	if (I < NItems) {
		// vkladany zaznam nebude posledni (nebude na konci)
		// zjistime spolecne casti s nasledujicim zaznamem
		WORD m2 = SLeadEqu(GetKey(I), SS);
		BYTE d = m2 - newXi->M;
		if (d > 0) {
			// puvodni polozka je ted na pozici I (nova je na I - 1)
			_cutLeafItem(I, d);
		}
	}
}

void XPage::InsDownIndex(WORD I, longint Page, XPage* P)
{
	pstring s = P->GetKey(P->NItems);
	XItem* x = nullptr;
	size_t xLen = 0;
	InsertNonLeaf(I, &s, &x, xLen);
	x->PutN(P->SumN());
	*(x->DownPage) = Page;
	this->_xItem = x;
	memcpy(this->A, this->_xItem->Nr, xLen);
}

void XPage::Delete(WORD I)
{
	Deserialize();
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
	Serialize();

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
		WORD m = SLeadEqu(GetKey(NItems), P->GetKey(1));
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
	// 1st half of this XPage will be moved into P

	size_t origSize = this->ItemsSize();
	size_t actualSize;
	size_t index; // last index that will be moved into P
	for (index = 0, actualSize = 0; index < _leafItems.size(); index++) {
		actualSize += _leafItems[index]->size();
		if (actualSize > origSize / 2) break;
	}

	// get new first key for this page
	auto firstNewKey = GetKey(index + 1 + 1);
	XItemLeaf* firstXItem = this->_leafItems[index + 1];

	// move first items into page P and remove them from this page
	for (size_t i = 0; i <= index; i++) {
		P->_leafItems.push_back(this->_leafItems.front()); // add 1st element to P
		P->NItems++;
		this->_leafItems.erase(_leafItems.begin()); // erase 1st element from original
		this->NItems--;
	}

	// replace 1st item of this page with full key
	auto newFirstXItem = new XItemLeaf(firstXItem->RecNr, 0, firstNewKey.length(), firstNewKey);
	this->_leafItems.erase(_leafItems.begin());
	this->_leafItems.insert(_leafItems.begin(), newFirstXItem);

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

void XPage::Deserialize()
{
	_leafItems.clear();
	size_t offset = 0;
	for (WORD i = 0; i < NItems; i++) {
		auto x = new XItemLeaf(&A[offset]);
		offset += x->size();
		_leafItems.push_back(std::move(x));
	}
}

void XPage::Serialize()
{
	memset(A, 0, sizeof(A));
	size_t offset = 0;
	BYTE buffer[256];
	for (auto&& item : _leafItems) {
		size_t len = item->Serialize(buffer, sizeof(buffer));
		memcpy(&A[offset], buffer, len);
		offset += len;
	}
	NItems = _leafItems.size();
}

std::vector<XItemLeaf*>::iterator XPage::_addToLeafItems(XItemLeaf* xi, size_t pos)
{
	return _leafItems.insert(_leafItems.begin() + pos, std::move(xi));
}

std::vector<XItemNonLeaf*>::iterator XPage::_addToNonLeafItems(XItemNonLeaf* xi, size_t pos)
{
	return _nonLeafItems.insert(_nonLeafItems.begin() + pos, std::move(xi));
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
