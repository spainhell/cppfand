#include "pstring.h"

pstring::pstring() : initLen(256)
{
	arr = new unsigned char[initLen + 1] { '\0' };
}

pstring::pstring(unsigned char length) : initLen(length + 1)
{
	arr = new unsigned char[initLen + 1] { '\0' };
}

pstring::pstring(const char* text) : initLen(256)
{
	size_t input_len = strlen(text);
	if (input_len > 255) input_len = 255;
	arr = new unsigned char[initLen + 1] { '\0' };
	arr[0] = (unsigned char)input_len;
	memcpy((void*)&arr[1], (void*)text, arr[0]);
}

pstring::pstring(const pstring& ps) : initLen(ps.initLen)
{
	arr = new unsigned char[ps.initLen + 1] { '\0' };
	memcpy((void*)arr, (void*)ps.arr, ps.arr[0] + 1);
}

pstring::pstring(std::string cs) : initLen(cs.length())
{
	size_t origLen = cs.length();
	arr = new unsigned char[origLen + 1]{ '\0' };
	arr[0] = origLen;
	memcpy(&arr[1], cs.c_str(), origLen);
}

pstring::~pstring()
{
	delete[] arr;
}

unsigned char pstring::length()
{
	return arr[0];
}

unsigned short pstring::initLength()
{
	return initLen;
}

void pstring::cut(unsigned char length)
{
	if (length < arr[0])
	{
		arr[0] = length;
	}
}

void pstring::clean()
{
	this->cut(0);
}

int pstring::first(char c)
{
	for (int i = 1; i <= arr[0]; i++)
	{
		if (arr[i] == c) return i;
	}
	return 0;
}

void pstring::Delete(int index, int size)
{
	if (index < 1 || index > arr[0]) return;
	int end = index + size;
	if (end > arr[0]) end = arr[0];
	memcpy(&arr[index], &arr[index + size], arr[0] - size + 1);
}

pstring pstring::substr(unsigned char index)
{
	return substr(index, arr[0] - index);
}

pstring pstring::substr(unsigned char index, unsigned char count)
{
	pstring psbst = pstring();
	if (index < arr[0])
	{
		if (count > arr[0] - index) count = arr[0] - index;
		memcpy((void*)psbst.arr, &arr[index], count);
	}
	return psbst;
}

void pstring::replace(const char* value)
{
	for (int i = 1; i <= initLen; i++) { arr[i] = '\0'; }
	int len = strlen(value);
	if (len > initLen) len = initLen;
	arr[0] = len;
	memcpy((void*)&arr[1], (void*)value, len);
}

void pstring::insert(const char* value, unsigned char position)
{
}

const char* pstring::c_str()
{
	int len = arr[0];
	arr[len + 1] = '\0';
	return (const char*)(&arr[1]);
}

bool pstring::empty()
{
	return (arr[0] == 0);
}

unsigned char& pstring::operator[](size_t i)
{
	return arr[i];
}

pstring& pstring::operator=(const char* a)
{
	int newLen = strlen(a);
	if (newLen > initLen - 1) { throw std::exception("Index out of range."); }
	arr[0] = newLen;
	memcpy((void*)&arr[1], (void*)a, newLen);
	return *this;
}

pstring& pstring::operator=(std::basic_string<char> newvalue)
{
	return pstring::operator=(newvalue.c_str());
}

pstring& pstring::operator=(const pstring& newvalue)
{
	if (this == &newvalue) return *this;
	
	if (newvalue.initLen > this->initLen) { throw std::exception("Index out of range."); }
	memcpy((void*)arr, (void*)newvalue.arr, initLen);
	return *this;
}

pstring::operator std::string() const
{
	int len = arr[0];
	arr[len + 1] = '\0';
	const char* exp = (const char*)&arr[1];
	return std::string(exp);
}

pstring& pstring::operator+=(const pstring& second)
{
	unsigned char secLen = second.arr[0];
	unsigned char firLen = arr[0];
	unsigned short newLen = firLen + secLen;
	if (newLen > 255) newLen = 255;
	arr[0] = newLen;
	memcpy((void*)&arr[firLen + 1], (void*)&second.arr[1] , newLen - firLen);
	return *this;
}

pstring pstring::operator+(const pstring& second)
{
	unsigned char secLen = second.arr[0];
	unsigned char firLen = arr[0];
	unsigned short newLen = firLen + secLen;
	if (newLen > 255) newLen = 255;
	pstring nps(*this);
	nps.arr[0] = newLen;
	memcpy((void*)&nps.arr[firLen + 1], (void*)&second.arr[1], newLen - firLen);
	return nps;
}

bool pstring::operator==(const pstring& eqpstring)
{
	if (arr[0] != eqpstring.arr[0]) return false;
	return strcmp((char*)&arr[1], (char*)&eqpstring.arr[1]) == 0;
}

bool pstring::operator!=(const pstring& eqpstring)
{
	return !pstring::operator==(eqpstring);
}

void pstring::Append(unsigned char c)
{
	unsigned char firLen = arr[0];
	unsigned short newLen = firLen + 1;
	if (newLen > initLen - 1) return;
	arr[newLen] = c;
	arr[newLen + 1] = '\0';
	arr[0] = newLen;
}

