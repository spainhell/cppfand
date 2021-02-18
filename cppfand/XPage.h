#pragma once
#include <vector>

#include "constants.h"
#include "pstring.h"
#include "XItem.h"
#include "XItemLeaf.h"

const WORD XPageSize = 1024;
const BYTE oLeaf = 3; const BYTE oNotLeaf = 7;
const BYTE XPageOverHead = 7; const BYTE MaxIndexLen = 123; //{ min.4 items };

class XPage // r289
{
public:
	XPage() {}
	~XPage();
	bool IsLeaf = false;
	longint GreaterPage = 0;  // or free pages chaining
	WORD NItems = 0;
	BYTE A[XPageSize - 4]{ '\0' };  // item array
	WORD Off();
	XItem* XI(WORD I, bool isLeaf);
	WORD EndOff();
	bool Underflow();
	bool Overflow();
	pstring StrI(WORD I);
	longint SumN();
	void Insert(WORD I, void* SS, XItem** XX);
	void InsertLeaf(unsigned int RecNr, size_t I, pstring& SS);
	void InsDownIndex(WORD I, longint Page, XPage* P);
	void Delete(WORD I);
	void AddPage(XPage* P);
	void SplitPage(XPage* P, longint ThisPage);
	void Clean();
	size_t ItemsSize();
	void GenArrayFromVectorItems();
private:
	XItem* _xItem = nullptr;
	void genItems();
	std::vector<XItemLeaf*>::iterator _addToItems(XItemLeaf* xi, size_t pos);
	std::vector<XItemLeaf*> _leafItems;
	bool _cutLeafItem(size_t iIndex, BYTE length); // zkrati polozku o X Bytu, zaktualizuje M i L
	bool _enhLeafItem(size_t iIndex, BYTE length); // prodlouzi polozku o X Bytu z predchozi polozky, zaktualizuje M i L
};
typedef XPage* XPagePtr;

