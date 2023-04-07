#pragma once
#include "../cppfand/pstring.h"
#include "XItem.h"

/// implementace XItem pro delsi zaznam
class XItemNonLeaf: public XItem
{
public:
	XItemNonLeaf(unsigned char* data);
	XItemNonLeaf(const XItemNonLeaf& orig);
	XItemNonLeaf(unsigned int recordsCount, unsigned int downPage, unsigned char M, unsigned char L, pstring& s); // kompletni 's', zpracuje se jen pozadovana cast
	XItemNonLeaf(unsigned int recordsCount, unsigned int downPage, unsigned char M, unsigned char L, std::string& s);
	~XItemNonLeaf() override;

	int RecordsCount = 0;
	int DownPage = 0;

	int GetN() override;
	void PutN(int N) override;

	//size_t UpdStr(pstring* S) override;

	size_t Serialize(unsigned char* buffer, size_t bufferSize) override;

	size_t size() override;

#if _DEBUG
	std::string key;
#endif
};

