#include "pstring.h"

pstring::pstring() : initLen(256)
{
	len = 255;
	arr = new unsigned char[initLen] { '\0' };
}

pstring::pstring(unsigned char length) : initLen(length + 1)
{
	len = length;
	arr = new unsigned char[initLen] { '\0' };
}

pstring::pstring(const char* text) : initLen(256)
{
	size_t input_len = strlen(text);
	if (input_len > 255) input_len = 255;
	this->len = (unsigned char)input_len - 1;
	arr = new unsigned char[initLen] { '\0' };
	memcpy((void*)arr, (void*)text, len);
}

pstring::pstring(const pstring& ps) : initLen(ps.initLen)
{
	this->len = ps.len;
	arr = new unsigned char[len] { '\0' };
	memcpy((void*)arr, (void*)ps.arr, len);
}

pstring::~pstring()
{
	delete[] arr;
}

unsigned char pstring::length()
{
	return len;
}

unsigned short pstring::initLength()
{
	return initLen;
}

const char* pstring::c_str()
{
	arr[len] = '\0'; // 'arr' je o 1 vìtší než 'len'
	return (const char*)arr;
}

bool pstring::empty()
{
	return (len == 0);
}

unsigned char& pstring::operator[](size_t i)
{
	if (i == 0) return len;
	if (i > len) { throw std::exception("Index out of range."); }
	i--;
	return arr[i];
}

pstring& pstring::operator=(const char* newvalue)
{
	int newLen = strlen(newvalue);
	if (newLen > initLen - 1) { throw std::exception("Index out of range."); }
	memcpy((void*)arr, (void*)newvalue, newLen);
	len = (unsigned char)newLen;
	arr[len - 1] = '\0';
	return *this;
}

pstring& pstring::operator=(std::basic_string<char> newvalue)
{
	return pstring::operator=(newvalue.c_str());
}

pstring& pstring::operator=(const pstring& newvalue)
{
	if (newvalue.initLen >= this->initLen) { throw std::exception("Index out of range."); }
	memcpy((void*)arr, (void*)newvalue.arr, newvalue.len);
	arr[len - 1] = '\0';
	return *this;
}

pstring::operator std::string() const
{
	arr[len] = '\0'; // 'arr' je o 1 vìtší než 'len'
	const char* exp = (const char*)arr;
	return std::string(exp);
}

void pstring::operator+=(const pstring& npstring)
{
	unsigned char nL = npstring.len;
	unsigned char sL = len;
	int free = this->initLen - sL - 1;
	int copycount = nL;
	if (free < nL) nL = free;
	memcpy((void*)arr, (void*)npstring.arr, copycount);
}

pstring& pstring::operator+(const pstring& npstring)
{
	unsigned char nL = npstring.len;
	unsigned char sL = len;
	int free = this->initLen - sL - 1;
	int copycount = nL;
	if (free < nL) nL = free;
	memcpy((void*)arr, (void*)npstring.arr, copycount);
	len = sL + copycount;
	return *this;
}

bool pstring::operator==(const pstring& eqpstring)
{
	return strcmp((char*)arr, (char*)eqpstring.arr) == 0;
}

