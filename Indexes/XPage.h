#pragma once
#include <vector>

#include "../cppfand/constants.h"
#include "../cppfand/pstring.h"
#include "XItem.h"
#include "XItemLeaf.h"
#include "XItemNonLeaf.h"

const WORD XPageSize = 1024;
const BYTE oLeaf = 3;
const BYTE oNotLeaf = 7;
const BYTE XPageOverHead = 7;
const BYTE MaxIndexLen = 123; // { min.4 items };

class XPage
{
public:
	XPage();
	XPage(const XPage& orig);
	~XPage();

	// hlavicka spolecna pro Leaf i NonLeaf              (7B)
	bool IsLeaf = false;                             // = 1B
	longint GreaterPage = 0;  // or free pages chaining = 4B
	WORD NItems = 0;                                 // = 2B

	BYTE A[XPageSize * 2]{ '\0' };  // item array - double sized because last added item can exceed page size
	WORD Off();
	XItem* GetItem(WORD I);
	bool Underflow();
	bool Overflow();
	pstring GetKey(WORD i);
	longint SumN();
	void InsertItem(unsigned int recordsCount, unsigned int downPage, WORD I, pstring& SS);
	void InsertItem(unsigned int recNr, size_t I, pstring& SS);
	void InsDownIndex(WORD I, longint Page, XPage* P);
	void Delete(WORD I);
	void AddPage(XPage* P);
	void SplitPage(XPage* P, longint ThisPage);
	void Clean();
	size_t ItemsSize();
	void Serialize();   // generate array from vector
	void Deserialize(); // generate vector from array

private:
	// Leaf section
	std::vector<XItemLeaf*>::iterator _addToLeafItems(XItemLeaf* xi, size_t pos);
	std::vector<XItemLeaf*> _leafItems;

	// Non Leaf section
	std::vector<XItemNonLeaf*>::iterator _addToNonLeafItems(XItemNonLeaf* xi, size_t pos);
	std::vector<XItemNonLeaf*> _nonLeafItems;

	bool _cutItem(size_t iIndex, BYTE length); // zkrati polozku o X Bytu, zaktualizuje M i L
	bool _enhItem(size_t iIndex, BYTE length); // prodlouzi polozku o X Bytu z predchozi polozky, zaktualizuje M i L
};

