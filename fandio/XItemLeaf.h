#pragma once
#include "XItem.h"

/// implementace XItem pro kratky zaznam
class XItemLeaf : public XItem
{
public:
	XItemLeaf(unsigned char* data);
	XItemLeaf(const XItemLeaf& orig);
	XItemLeaf(unsigned int RecNr, unsigned char M, unsigned char L, std::string& s); // cely klic 's', zpracuje se jen cast od 'M' o delce 'L'
	~XItemLeaf() override;

	unsigned int RecNr;

	int GetN() override;
	void PutN(int N) override;

	//size_t UpdStr(pstring* S) override;

	size_t Serialize(unsigned char* buffer, size_t bufferSize) override;

	size_t size() override;

#if _DEBUG
	std::string key;
#endif
};

