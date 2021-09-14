#pragma once
#include "constants.h"
#include "pstring.h"

/// implementace XItem pro kratky zaznam
class XItemLeaf
{
public:
	XItemLeaf(BYTE* data);
	XItemLeaf(const XItemLeaf& orig);
	XItemLeaf(unsigned int RecNr, BYTE M, BYTE L, pstring& s); // cely klic 's', zpracuje se jen cast od 'M' o delce 'L'
	~XItemLeaf();
	unsigned int RecNr;
	BYTE M;
	BYTE L;
	BYTE* data;
	size_t size();
	size_t dataLen(); // bez 2B L + M
	size_t Serialize(BYTE* buffer, size_t bufferSize);
};

