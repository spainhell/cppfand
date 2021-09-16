#pragma once
#include "constants.h"
#include "pstring.h"

class XItem // r274
{
public:
	virtual ~XItem() = default;

	BYTE M;
	BYTE L;
	BYTE* data;

	virtual longint GetN() = 0;
	virtual void PutN(longint N) = 0;

	virtual WORD GetM() = 0;
	virtual void PutM(WORD M) = 0;

	virtual WORD GetL() = 0;
	virtual void PutL(WORD L) = 0;

	virtual size_t Serialize(BYTE* buffer, size_t bufferSize) = 0;

	virtual XItem* Next() = 0;
	virtual WORD UpdStr(pstring* S) = 0;
	virtual size_t size() = 0; // vrati delku zaznamu
};
