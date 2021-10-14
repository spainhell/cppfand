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
	~XItemLeaf() override;

	unsigned int RecNr;

	longint GetN() override;
	void PutN(longint N) override;

	WORD GetM() override;
	void PutM(WORD M) override;

	WORD GetL() override;
	void PutL(WORD L) override;

	XItem* Next() override;
	WORD UpdStr(pstring* S) override;

	size_t Serialize(BYTE* buffer, size_t bufferSize) override;

	size_t size() override;
	size_t dataLen(); // bez 2B L + M
};

