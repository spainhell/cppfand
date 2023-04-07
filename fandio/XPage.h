#pragma once
#include <vector>
#include "XItem.h"
#include "XItemLeaf.h"
#include "XItemNonLeaf.h"

const unsigned short XPageSize = 1024;
const unsigned char oLeaf = 3;
const unsigned char oNotLeaf = 7;
const unsigned char XPageOverHead = 7;
const unsigned char MaxIndexLen = 123; // { min.4 items };

class XPage
{
public:
	XPage();
	XPage(const XPage& orig);
	~XPage();

	// hlavicka spolecna pro Leaf i NonLeaf              (7B)
	bool IsLeaf = false;                             // = 1B
	int GreaterPage = 0;  // or free pages chaining = 4B
	unsigned short NItems = 0;                                 // = 2B

	unsigned char A[XPageSize * 2]{ '\0' };  // item array - double sized because last added item can exceed page size
	unsigned short Off();
	XItem* GetItem(unsigned short I);
	bool Underflow();
	bool Overflow();
	std::string GetKey(size_t i);
	int SumN();
	void InsertItem(unsigned int recordsCount, unsigned int downPage, unsigned short I, pstring& SS);
	void InsertItem(unsigned int recNr, size_t I, pstring& SS);
	void InsDownIndex(unsigned short I, int Page, XPage* P);
	void Delete(unsigned short I);
	void AddPage(XPage* P);
	void SplitPage(XPage* P, int ThisPage);
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

	bool _cutItem(size_t iIndex, unsigned char length); // zkrati polozku o X Bytu, zaktualizuje M i L
	bool _enhItem(size_t iIndex, unsigned char length); // prodlouzi polozku o X Bytu z predchozi polozky, zaktualizuje M i L
};

