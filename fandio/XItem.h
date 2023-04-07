#pragma once
#include "../cppfand/constants.h"
#include "../cppfand/pstring.h"

class XItem // r274
{
public:
	BYTE M = 0;
	BYTE L = 0;
	BYTE* data = nullptr;

	virtual ~XItem() = default;

	virtual longint GetN() = 0;
	virtual void PutN(longint N) = 0;

	virtual WORD GetM();
	virtual void PutM(WORD M);

	virtual WORD GetL();
	virtual void PutL(WORD L);

	virtual size_t Serialize(BYTE* buffer, size_t bufferSize) = 0;

	virtual size_t UpdStr(pstring* S) = 0;
	virtual std::string GetKey(std::string& previous_key);
	virtual size_t size() = 0; // vrati delku zaznamu
	virtual size_t data_len();
};
