#pragma once
#include "constants.h"
#include "pstring.h"

/// implementace XItem pro delsi zaznam
class XItemNonLeaf
{
public:
	XItemNonLeaf(BYTE* data);
	XItemNonLeaf(const XItemNonLeaf& orig);
	//XItemNonLeaf(unsigned int RecNr, BYTE M, BYTE L, pstring& s); // kompletni 's', zpracuje se jen pozadovana cast
	~XItemNonLeaf();
	unsigned int RecNr;
	uint32_t DownPage;
	BYTE M;
	BYTE L;
	BYTE* data;
	size_t size();
	size_t data_len(); // bez 2B L + M
	size_t Serialize(BYTE* buffer, size_t bufferSize);
};

