#pragma once
#include "../cppfand/constants.h"
#include "../cppfand/pstring.h"
#include "XItem.h"

/// implementace XItem pro kratky zaznam
class XItemLeaf : public XItem
{
public:
	XItemLeaf(BYTE* data);
	XItemLeaf(const XItemLeaf& orig);
	XItemLeaf(unsigned int RecNr, BYTE M, BYTE L, pstring& s); // cely klic 's', zpracuje se jen cast od 'M' o delce 'L'
	XItemLeaf(unsigned int RecNr, BYTE M, BYTE L, std::string& s); // cely klic 's', zpracuje se jen cast od 'M' o delce 'L'
	~XItemLeaf() override;

	unsigned int RecNr;

	longint GetN() override;
	void PutN(longint N) override;

	size_t UpdStr(pstring* S) override;

	size_t Serialize(BYTE* buffer, size_t bufferSize) override;

	size_t size() override;

#if _DEBUG
	std::string key;
#endif
};

