#include "pstring.h"

int strcount = 0;
int strbytes = 0;

pstring::pstring() : initLen(256)
{
	arr = new unsigned char[initLen + 1] { '\0' };
	strcount++;
	strbytes += initLen + 1;
}

pstring::pstring(unsigned char length) : initLen(length + 1)
{
	arr = new unsigned char[initLen + 1] { '\0' };
	strcount++;
	strbytes += length + 1;
}

pstring::pstring(const char* text) : initLen(256)
{
	size_t input_len = strlen(text);
	if (input_len > 255) input_len = 255;
	arr = new unsigned char[initLen + 1] { '\0' };
	arr[0] = (unsigned char)input_len;
	memcpy((void*)&arr[1], (void*)text, arr[0]);
	strcount++;
	strbytes += initLen + 1;
}

pstring::pstring(const pstring& ps) : initLen(256)
{
	arr = new unsigned char[ps.initLen] { '\0' };
	unsigned short copyLen = initLen < ps.initLen ? initLen : ps.initLen;
	// memcpy((void*)arr, (void*)ps.arr, ps.arr[0] + 1); // kopiruje se 1B a pak delka retezce
	// kopiruje se vetsi delka (nektere retezce za sebou jeste nesou data) - napr. fce FieldDMask je pak vytahuje
	memcpy((void*)arr, (void*)ps.arr, copyLen);
	strcount++;
	strbytes += initLen + 1;
}

pstring::pstring(std::string cs) : initLen(256)
{
	size_t origLen = cs.length();
	arr = new unsigned char[origLen + 1]{ '\0' };
	arr[0] = origLen;
	memcpy(&arr[1], cs.c_str(), origLen);
	strcount++;
	strbytes += initLen + 1;
}

pstring::pstring(char str[], unsigned char i) : initLen(256)
{
	arr = new unsigned char[256]{ '\0' };
	arr[0] = i;
	memcpy(&arr[1], str, i);
	strcount++;
	strbytes += initLen + 1;
}

pstring::~pstring()
{
	delete[] arr;
	arr = nullptr;
	strcount--;
	strbytes--;
	strbytes -= initLen;
}

unsigned char pstring::length() const
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
	if (index > arr[0] || size < 1) return;
	if (index + size + 1 > arr[0])
	{
		// odebíráme od konce
		size = arr[0] - index + 1;
		arr[0] = index - 1;
		arr[index] = '\0';
	}
	else 
	{
		// odebíráme uvnitø øetìzce
		memcpy(&arr[index], &arr[index + size], arr[0] - index - size + 1);
		arr[0] = arr[0] - size;
		arr[arr[0] + 1] = '\0';
	}
}

pstring pstring::substr(unsigned char index)
{
	return substr(index, arr[0] - index);
}

pstring pstring::substr(unsigned char index, unsigned char count)
{
	pstring psbst;
	if (index < arr[0])
	{
		if (count > arr[0] - index) count = arr[0] - index;
		psbst[0] = count;
		memcpy(&psbst.arr[1], &arr[index + 1], count);
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

unsigned char pstring::at(unsigned char index) const
{
	return arr[index];
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
	
	if (newvalue.initLen > this->initLen)
	{
		unsigned char* newArray = new unsigned char[newvalue.initLen];
		delete[] this->arr;
		this->arr = newArray;
	}
	//memset(arr, 0, initLen);
	//memcpy(arr, newvalue.arr, newvalue.arr[0] + 1);
	// musí se zkopírovat i data za retezcem - nekter funkce je vyuzivaji
	memcpy(arr, newvalue.arr, newvalue.initLen); 
	return *this;
}

pstring::operator std::string() const
{
	int len = arr[0];
	//arr[len + 1] = '\0';
	const char* exp = (const char*)&arr[1];
	return std::string(exp, len);
}

pstring& pstring::operator+=(const pstring& second)
{
	unsigned char secLen = second.arr[0];
	unsigned char firLen = arr[0];
	unsigned short newLen = firLen + secLen;
	if (newLen > initLen) throw std::exception("String is too small to add new text into it. (+=)");
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
	return memcmp((char*)&arr[1], (char*)&eqpstring.arr[1], arr[0]) == 0;
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

