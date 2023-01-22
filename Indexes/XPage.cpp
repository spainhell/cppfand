#include "XPage.h"

#include "../cppfand/base.h"


XPage::XPage()
{
}

XPage::XPage(const XPage& orig)
{
	this->IsLeaf = orig.IsLeaf;
	this->GreaterPage = orig.GreaterPage;
	this->NItems = orig.NItems;

	// to 01/18/2023 items weren't copied, but same items were inserted to the specific vector
	// there were problems with the method XKey::DeleteOnPath() 
	if (IsLeaf) {
		for (size_t i = 0; i < orig._leafItems.size(); i++) {
			_leafItems.push_back(orig._leafItems[i]);
		}
	}
	else {
		for (size_t i = 0; i < orig._nonLeafItems.size(); i++) {
			_nonLeafItems.push_back(orig._nonLeafItems[i]);
		}
	}

	// since 01/18/2023 it will keep it's own data
	Serialize();
	_leafItems.clear();
	_nonLeafItems.clear();
	Deserialize();
}

XPage::~XPage()
{
	Clean();
}

WORD XPage::Off()
{
	if (IsLeaf) {
		return oLeaf;
	}
	else {
		return oNotLeaf;
	}
}

XItem* XPage::GetItem(WORD I)
{
	if (this->IsLeaf) {
		if (_leafItems.size() < I) {
			return nullptr;
		}
		return _leafItems[I - 1];
	}
	else {
		if (_nonLeafItems.size() < I) {
			return nullptr;
		}
		return _nonLeafItems[I - 1];
	}
}

//WORD XPage::EndOff()
//{
//	return sizeof(A);
//}

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
			XItem* x = this->GetItem(i);
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
			_cutItem(I, d);
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
	const char* s = SS.c_str();
	if (SS.length() >= 5 && s[2] == '7' && s[3] == '1' && s[4] == '8')
	{
		printf("Je to tady");
	}

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
			_cutItem(I, d);
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
	XItem* Xi;

	if (IsLeaf) {
		Xi = _leafItems[I - 1];
		if (I < NItems) {
			// tato polozka (I - 1) neni posledni, bude se asi muset upravovat polozka za ni (I)
			auto nextXi = _leafItems[I];
			integer d = nextXi->M - Xi->M;
			if (d > 0) {
				// nasledujici polozku je nutne upravit
				_enhItem(I, d);
			}
		}
		// polozku smazeme z vektoru
		_leafItems.erase(_leafItems.begin() + (I - 1));
	}
	else
	{
		Xi = _nonLeafItems[I - 1];
		if (I < NItems) {
			// tato polozka (I - 1) neni posledni, bude se asi muset upravovat polozka za ni (I)
			auto nextXi = _nonLeafItems[I];
			integer d = nextXi->M - Xi->M;
			if (d > 0) {
				// nasledujici polozku je nutne upravit
				_enhItem(I, d);
			}
		}
		// polozku smazeme z vektoru
		_nonLeafItems.erase(_nonLeafItems.begin() + (I - 1));
	}

	delete Xi; Xi = nullptr;

	NItems--;
}

void XPage::AddPage(XPage* P)
{
	GreaterPage = P->GreaterPage;
	if (P->NItems == 0) return;

	if (NItems > 0) {
		WORD m = SLeadEqu(GetKey(NItems), P->GetKey(1));
		if (m > 0) {
			P->_cutItem(0, m);
		}
	}

	if (IsLeaf) {
		for (size_t i = 0; i < P->_leafItems.size(); i++) {
			this->_leafItems.push_back(P->_leafItems[i]);
		}
		P->_leafItems.clear(); // must clear it, otherwise after destroy P there will be problem in local vector 
	}
	else {
		for (size_t i = 0; i < P->_nonLeafItems.size(); i++) {
			this->_nonLeafItems.push_back(P->_nonLeafItems[i]);
		}
		P->_nonLeafItems.clear(); // must clear it, otherwise after destroy P there will be problem in local vector 
	}
	NItems += P->NItems;
	P->NItems = 0;
}

/// <summary>
/// Moves 1st half of this XPage to page P
/// </summary>
/// <param name="P">destination page - have to be empty</param>
/// <param name="ThisPage">this page number</param>
void XPage::SplitPage(XPage* P, longint ThisPage)
{
	// 1st half of this XPage will be moved into P
	P->IsLeaf = IsLeaf;
	size_t origSize = this->ItemsSize();
	size_t actualSize;
	size_t index; // last index that will be moved into P

	if (IsLeaf) {
		P->GreaterPage = ThisPage;

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
	}
	else {
		P->GreaterPage = 0;

		for (index = 0, actualSize = 0; index < _nonLeafItems.size(); index++) {
			actualSize += _nonLeafItems[index]->size();
			if (actualSize > origSize / 2) break;
		}

		// get new first key for this page
		auto firstNewKey = GetKey(index + 1 + 1);
		XItemNonLeaf* firstXItem = this->_nonLeafItems[index + 1];

		// move first items into page P and remove them from this page
		for (size_t i = 0; i <= index; i++) {
			P->_nonLeafItems.push_back(this->_nonLeafItems.front()); // add 1st element to P
			P->NItems++;
			this->_nonLeafItems.erase(_nonLeafItems.begin()); // erase 1st element from original
			this->NItems--;
		}

		// replace 1st item of this page with full key
		auto newFirstXItem = new XItemNonLeaf(firstXItem->RecordsCount, firstXItem->DownPage, 0, firstNewKey.length(), firstNewKey);
		this->_nonLeafItems.erase(_nonLeafItems.begin());
		this->_nonLeafItems.insert(_nonLeafItems.begin(), newFirstXItem);
	}
}

void XPage::Clean()
{
	IsLeaf = false;
	GreaterPage = 0;
	NItems = 0;
	memset(A, 0, sizeof(A));
	for (size_t i = 0; i < _leafItems.size(); i++) {
		delete _leafItems[i];
	}
	for (size_t i = 0; i < _nonLeafItems.size(); i++) {
		delete _nonLeafItems[i];
	}
	_leafItems.clear();
	_nonLeafItems.clear();
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
			_leafItems.push_back(x);
		}
	}
	else {
		size_t offset = 0;
		for (WORD i = 0; i < NItems; i++) {
			auto x = new XItemNonLeaf(&A[offset]);
			offset += x->size();
			_nonLeafItems.push_back(x);
		}
	}
#if _DEBUG
	// vygeneruj cely klic v polozkach
	if (IsLeaf && !_leafItems.empty()) {
		_leafItems[0]->key = std::string((char*)_leafItems[0]->data, _leafItems[0]->L);
		for (size_t i = 1; i < NItems; i++) {
			// part of previous key
			std::string first_part = _leafItems[i - 1]->key.substr(0, _leafItems[i]->M);
			std::string second_part = std::string((char*)_leafItems[i]->data, _leafItems[i]->L);
			_leafItems[i]->key = first_part + second_part;
		}
	}
	else if (!IsLeaf && !_nonLeafItems.empty()) {
		_nonLeafItems[0]->key = std::string((char*)_nonLeafItems[0]->data, _nonLeafItems[0]->L);
		for (size_t i = 1; i < NItems; i++) {
			// part of previous key
			std::string first_part = _nonLeafItems[i - 1]->key.substr(0, _nonLeafItems[i]->M);
			std::string second_part = std::string((char*)_nonLeafItems[i]->data, _nonLeafItems[i]->L);
			_nonLeafItems[i]->key = first_part + second_part;
		}
	}
#endif
}

void XPage::Serialize()
{
	memset(A, 0, sizeof(A));
	size_t offset = 0;
	BYTE buffer[256];
	if (IsLeaf) {
		for (auto&& item : _leafItems) {
			size_t len = item->Serialize(buffer, sizeof(buffer));
			if (offset + len > sizeof(A)) {
				throw std::exception("XPage::Serialize() buffer overflow.");
			}
			memcpy(&A[offset], buffer, len);
			offset += len;
		}
		NItems = _leafItems.size();
	}
	else {
		for (auto&& item : _nonLeafItems) {
			size_t len = item->Serialize(buffer, sizeof(buffer));
			if (offset + len > sizeof(A)) {
				throw std::exception("XPage::Serialize() buffer overflow.");
			}
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

bool XPage::_cutItem(size_t iIndex, BYTE length)
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

bool XPage::_enhItem(size_t iIndex, BYTE length)
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
	memcpy(Xi->data, prevXi->data, length); // z predchoziho zaznamu zkopirujeme prvni Byty
	memcpy(&Xi->data[length], origData, Xi->L - length); // a doplnime je puvodnimi daty
	delete origData; origData = nullptr;
	return true;
}
