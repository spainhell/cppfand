#include "XPage.h"

#include "base.h"


XPage::~XPage()
{
	for (size_t i = 0; i < _leafItems.size(); i++) {
		delete _leafItems[i];
	}
	for (size_t i = 0; i < _nonLeafItems.size(); i++) {
		delete _nonLeafItems[i];
	}
}

WORD XPage::Off()
{
	if (IsLeaf) return oLeaf;
	else return oNotLeaf;
}

XItem* XPage::XI(WORD I)
{
	if (this->IsLeaf) {
		return _leafItems[I - 1];
	}
	else {
		return _nonLeafItems[I - 1];
	}
}

WORD XPage::EndOff()
{
	return sizeof(A);
	XItem* x = XI(NItems + 1);
	WORD* xofs = (WORD*)x; // absolute x
	return (uintptr_t)xofs;
}

bool XPage::Underflow()
{
	const auto size = ItemsSize();
	return size < (XPageSize - XPageOverHead) / 2;
}

bool XPage::Overflow()
{
	const auto size = ItemsSize();
	return size > XPageSize - XPageOverHead;
}


/// Returns calculated complete key value;
/// i = 1 .. N
pstring XPage::GetKey(WORD i)
{
	pstring s; // toto bude vystup

	if (i > NItems) s[0] = 0;
	else {
		if (IsLeaf) {
			for (WORD j = 0; j < i; j++) {
				XItem* x = _leafItems[j];
				x->UpdStr(&s);
			}
		}
		else {
			for (WORD j = 0; j < i; j++) {
				XItem* x = _nonLeafItems[j];
				x->UpdStr(&s);
			}
		}
	}
	return s;
}

longint XPage::SumN()
{
	if (IsLeaf) {
		return NItems;
	}
	else {
		longint n = 0;
		WORD o = Off();
		for (WORD i = 1; i < NItems; i++) {
			XItem* x = this->XI(i);
			n += x->GetN();
		}
		return n;
	}
}

/// <summary>
/// Insert Non Leaf Item
/// </summary>
/// <param name="recordsCount">total records count on the referenced page</param>
/// <param name="downPage">down page number</param>
/// <param name="I">order of the inserted item</param>
/// <param name="SS">key</param>
void XPage::InsertItem(unsigned int recordsCount, unsigned int downPage, WORD I, pstring& SS)
{
	NItems++;
	WORD m = 0;
	// zjistime spolecne casti s predchozim zaznamem
	if (I > 1) m = SLeadEqu(GetKey(I - 1), SS);
	WORD l = SS.length() - m;
	// vytvorime novou polozku s novym zaznamem a vlozime ji do vektoru

	auto newXi = new XItemNonLeaf(recordsCount, downPage, m, l, SS);
	_addToNonLeafItems(newXi, I - 1);

	if (I < NItems) {
		// vkladany zaznam nebude posledni (nebude na konci)
		// zjistime spolecne casti s nasledujicim zaznamem
		WORD m2 = SLeadEqu(GetKey(I + 1), SS);
		integer d = m2 - newXi->GetM();
		if (d > 0) {
			// puvodni polozka je ted na pozici I (nova je na I - 1)
			_cutLeafItem(I, d);
		}
	}
}

/// <summary>
/// Insert Leaf Item
/// </summary>
/// <param name="recNr">record number in .000 file</param>
/// <param name="I">order of the inserted item</param>
/// <param name="SS">key</param>
void XPage::InsertItem(unsigned int recNr, size_t I, pstring& SS)
{
	NItems++;
	WORD m = 0;
	// zjistime spolecne casti s predchozim zaznamem
	if (I > 1) m = SLeadEqu(GetKey(I - 1), SS);
	WORD l = SS.length() - m;
	// vytvorime novou polozku s novym zaznamem a vlozime ji do vektoru

	auto newXi = new XItemLeaf(recNr, m, l, SS);
	_addToLeafItems(newXi, I - 1);

	if (I < NItems) {
		// vkladany zaznam nebude posledni (nebude na konci)
		// zjistime spolecne casti s nasledujicim zaznamem
		WORD m2 = SLeadEqu(GetKey(I + 1), SS);
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
	size_t xLen = 0;
	InsertItem(P->SumN(), Page, I, s);
	// TODO: memcpy(this->A, &x->RecordsCount, xLen);
}

void XPage::Delete(WORD I)
{
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
	// Serialize();
}

void XPage::AddPage(XPage* P)
{
	XItem* x = nullptr, * x1 = nullptr;
	WORD* xofs = (WORD*)x;

	GreaterPage = P->GreaterPage;
	if (P->NItems == 0) return;
	XItem* xE = XI(NItems + 1);
	WORD oE = P->EndOff(); WORD o = Off(); x = (XItem*)(&P->A);
	if (NItems > 0) {
		WORD m = SLeadEqu(GetKey(NItems), P->GetKey(1));
		if (m > 0) {
			WORD l = x->GetL() - m;
			x1 = x;
			xofs += m;
			Move(x1, x, o);
			x->PutM(m); x->PutL(l);
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
	_nonLeafItems.clear();

	if (IsLeaf) {
		size_t offset = 0;
		for (WORD i = 0; i < NItems; i++) {
			auto x = new XItemLeaf(&A[offset]);
			offset += x->size();
			_leafItems.push_back(std::move(x));
		}
	}
	else {
		size_t offset = 0;
		for (WORD i = 0; i < NItems; i++) {
			auto x = new XItemNonLeaf(&A[offset]);
			offset += x->size();
			_nonLeafItems.push_back(std::move(x));
		}
	}
}

void XPage::Serialize()
{
	memset(A, 0, sizeof(A));
	size_t offset = 0;
	BYTE buffer[256];
	if (IsLeaf) {
		for (auto&& item : _leafItems) {
			size_t len = item->Serialize(buffer, sizeof(buffer));
			memcpy(&A[offset], buffer, len);
			offset += len;
		}
		NItems = _leafItems.size();
	}
	else {
		for (auto&& item : _nonLeafItems) {
			size_t len = item->Serialize(buffer, sizeof(buffer));
			memcpy(&A[offset], buffer, len);
			offset += len;
		}
		NItems = _nonLeafItems.size();
	}

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
	XItem* Xi;
	if (IsLeaf) {
		if (_leafItems.size() < iIndex + 1) return false; // polozka neexistuje
		Xi = _leafItems[iIndex];
	}
	else {
		if (_nonLeafItems.size() < iIndex + 1) return false; // polozka neexistuje
		Xi = _nonLeafItems[iIndex];
	}
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
	XItem* Xi;
	XItem* prevXi;

	if (IsLeaf) {
		if (_leafItems.size() < iIndex + 1) return false; // polozka neexistuje
		prevXi = _leafItems[iIndex - 1]; // mazana polozka, z teto budeme brat data
		Xi = _leafItems[iIndex]; // aktualizovana (rozsirovana) polozka	
	}
	else {
		if (_nonLeafItems.size() < iIndex + 1) return false; // polozka neexistuje
		prevXi = _nonLeafItems[iIndex - 1]; // mazana polozka, z teto budeme brat data
		Xi = _nonLeafItems[iIndex]; // aktualizovana (rozsirovana) polozka	
	}

	Xi->M -= length;
	Xi->L += length;
	auto origData = Xi->data;
	Xi->data = new BYTE[Xi->L];
	memcpy(Xi->data, &prevXi->data[prevXi->L - length - 1], length); // z predchoziho zaznamu zkopirujeme X poslednich Bytu
	memcpy(&Xi->data[length], origData, Xi->L - length); // a doplnime je puvodnimi daty
	delete origData; origData = nullptr;
	return true;
}
