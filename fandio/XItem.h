#pragma once
#include <string>
#include "../cppfand/pstring.h"

class XItem
{
public:
	unsigned char M = 0;
	unsigned char L = 0;
	unsigned char* data = nullptr;

	virtual ~XItem() = default;

	virtual int GetN() = 0;
	virtual void PutN(int N) = 0;

	virtual unsigned short GetM();
	virtual void PutM(unsigned short M);

	virtual unsigned short GetL();
	virtual void PutL(unsigned short L);

	virtual size_t Serialize(unsigned char* buffer, size_t bufferSize) = 0;

	virtual std::string GetKey(std::string& previous_key);
	virtual size_t size() = 0; // vrati delku zaznamu
	virtual size_t data_len();
};
