#pragma once
#include "constants.h"
#include "pstring.h"
#include "XItem.h"

/// implementace XItem pro delsi zaznam
class XItemNonLeaf: public XItem
{
public:
	XItemNonLeaf(BYTE* data);
	XItemNonLeaf(const XItemNonLeaf& orig);
	XItemNonLeaf(unsigned int recordsCount, unsigned int downPage, BYTE M, BYTE L, pstring& s); // kompletni 's', zpracuje se jen pozadovana cast
	~XItemNonLeaf() override;

	longint RecordsCount = 0;
	longint DownPage = 0;

	longint GetN() override;
	void PutN(longint N) override;

	WORD GetM() override;
	void PutM(WORD M) override;

	WORD GetL() override;
	void PutL(WORD L) override;

	XItem* Next() override;
	WORD UpdStr(pstring* S) override;

	size_t Serialize(BYTE* buffer, size_t bufferSize) override;

	size_t size();
	size_t data_len(); // bez 2B L + M
};

