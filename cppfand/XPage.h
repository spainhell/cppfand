#pragma once
#include <vector>

#include "constants.h"
#include "pstring.h"
#include "XItem.h"
#include "XItemLeaf.h"
#include "XItemNonLeaf.h"

const WORD XPageSize = 1024;
const BYTE oLeaf = 3;
const BYTE oNotLeaf = 7;
const BYTE XPageOverHead = 7;
const BYTE MaxIndexLen = 123; // { min.4 items };

class XPage // r289
{
public:
	XPage() {}
	~XPage();

	// hlavicka spolecna pro Leaf i NonLeaf              (7B)
	bool IsLeaf = false;                             // = 1B
	longint GreaterPage = 0;  // or free pages chaining = 4B
	WORD NItems = 0;                                 // = 2B

	BYTE A[XPageSize]{ '\0' };  // item array
	WORD Off();
	XItem* XI(WORD I, bool isLeaf);
	XItem* XI(WORD I);
	WORD EndOff();
	bool Underflow();
	bool Overflow();
	pstring GetKey(WORD i);
	longint SumN();
	void InsertNonLeaf(WORD I, void* SS, XItem** XX, size_t& XXLen);
	void InsertLeaf(unsigned int RecNr, size_t I, pstring& SS);
	void InsDownIndex(WORD I, longint Page, XPage* P);
	void Delete(WORD I);
	void AddPage(XPage* P);
	void SplitPage(XPage* P, longint ThisPage);
	void Clean();
	size_t ItemsSize();
	void Serialize(); // generate array from vector
	void Deserialize(); // generate vector from array
private:
	XItem* _xItem = nullptr;

	// Leaf section
	std::vector<XItemLeaf*>::iterator _addToLeafItems(XItemLeaf* xi, size_t pos);
	std::vector<XItemLeaf*> _leafItems;
	bool _cutLeafItem(size_t iIndex, BYTE length); // zkrati polozku o X Bytu, zaktualizuje M i L
	bool _enhLeafItem(size_t iIndex, BYTE length); // prodlouzi polozku o X Bytu z predchozi polozky, zaktualizuje M i L

	// Non Leaf section
	std::vector<XItemNonLeaf*>::iterator _addToNonLeafItems(XItemNonLeaf* xi, size_t pos);
	std::vector<XItemNonLeaf*> _nonLeafItems;
};

