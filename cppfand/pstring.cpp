#include "pstring.h"

pstring::pstring() : initLen(256)
{
	len = 0;
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
	this->len = (unsigned char)input_len;
	arr = new unsigned char[initLen] { '\0' };
	memcpy((void*)arr, (void*)text, len);
}

pstring::pstring(const pstring& ps) : initLen(ps.initLen)
{
	this->len = ps.len;
	arr = new unsigned char[ps.initLen] { '\0' };
	memcpy((void*)arr, (void*)ps.arr, len);
}

pstring::~pstring()
{
	delete[] arr;
}

unsigned char pstring::length()
{
	arr[len] = '\0'; // 'arr' je o 1 vìtší než 'len'
	return len;
}

unsigned short pstring::initLength()
{
	return initLen;
}

void pstring::cut(unsigned char length)
{
	if (length < this->len)
	{
		this->len = length;
		arr[this->len] = '\0';
	}
}

void pstring::clean()
{
	this->cut(0);
}

int pstring::first(char c)
{
	for (int i = 0; i < this->len; i++)
	{
		if (arr[0] == c) return i + 1;
	}
	return 0;
}

void pstring::Delete(int index, int size)
{
	if (index < 1) return;
	int i = index - 1;
	int takeFrom = i + size;
	if (takeFrom >= len) return;
	memcpy(&arr[i], &arr[takeFrom], this->len - takeFrom);
	len -= size;
}

pstring pstring::substr(unsigned char index)
{
	return substr(index, len - index);
}

pstring pstring::substr(unsigned char index, unsigned char count)
{
	arr[len] = '\0'; // 'arr' je o 1 vìtší než 'len'
	pstring psbst = pstring();
	if (index < len)
	{
		if (count > len - index) count = len - index;
		memcpy((void*)psbst.arr, &arr[index], count);
		psbst.len = count;
	}
	return psbst;
}

void pstring::replace(const char* value)
{
	for (int i = 0; i <= initLen; i++) { arr[i] = '\0'; }
	int len = strlen(value);
	if (len > initLen) len = initLen;
	memcpy((void*)arr, (void*)value, len);
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
	arr[len] = '\0'; // 'arr' je o 1 vìtší než 'len'
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
	arr[len] = '\0'; // 'arr' je o 1 vìtší než 'len'
	return *this;
}

pstring::operator std::string() const
{
	arr[len] = '\0'; // 'arr' je o 1 vìtší než 'len'
	const char* exp = (const char*)arr;
	return std::string(exp);
}

pstring& pstring::operator+=(const pstring& second)
{
	unsigned char secLen = second.len;
	unsigned char firLen = len;
	unsigned short newLen = firLen + secLen;
	if (newLen > 255) newLen = 255;
	this->len = newLen;
	memcpy((void*)&arr[firLen], (void*)second.arr, newLen - firLen);
	arr[len] = '\0'; // 'arr' je o 1 vìtší než 'len'
	return *this;
}

pstring pstring::operator+(const pstring& second)
{
	unsigned char secLen = second.len;
	unsigned char firLen = len;
	unsigned short newLen = firLen + secLen;
	if (newLen > 255) newLen = 255;
	pstring nps(*this);
	nps.len = newLen;
	memcpy((void*)&nps.arr[firLen], (void*)second.arr, newLen - firLen);
	arr[len] = '\0'; // 'arr' je o 1 vìtší než 'len'
	return nps;
}

bool pstring::operator==(const pstring& eqpstring)
{
	arr[len] = '\0'; // 'arr' je o 1 vìtší než 'len'
	eqpstring.arr[eqpstring.len] = '\0'; // 'arr' je o 1 vìtší než 'len'
	if (len != eqpstring.len) return false;
	return strcmp((char*)arr, (char*)eqpstring.arr) == 0;
}

bool pstring::operator!=(const pstring& eqpstring)
{
	return !pstring::operator==(eqpstring);
}

