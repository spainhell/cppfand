#include "legacy.h"
#include "windows.h"
#include <iostream>


void val(string s, BYTE& b, WORD& err)
{
    string::size_type sz;
	auto a = stoul(s, &sz, 10);
    // pøeložil se celý øetìzec?
	if (sz == s.length() - 1)
	{
		err = 0;
		b = static_cast<BYTE>(a);
	}
    else 
    {
		err = static_cast<WORD>(sz);
		b = static_cast<BYTE>(a);
	}
}

string copy(string source, size_t index, size_t count)
{
    return source.substr(index, count);
}

void FSplit(string fullname, string& dir, string& name, string& ext)
{
        size_t found;
        found = fullname.find_last_of("/\\");
        dir = fullname.substr(0, found);
        string filename = fullname.substr(found + 1);
        found = filename.find_last_of('.');
        name = filename.substr(0, found - 1);
        ext = filename.substr(found + 1);
}

void GetDir(BYTE disk, string& cesta)
{
    const unsigned long maxDir = 260;
    char currentDir[maxDir];
    GetCurrentDirectory(maxDir, currentDir);
    cesta = currentDir;
}

BYTE Hi(WORD cislo)
{
    return cislo >> 4;
}

BYTE Lo(WORD cislo)
{
    return cislo & 0x00FF;
}

WORD Swap(WORD cislo)
{
    return ((cislo & 0x00FF) << 4) + (cislo >> 4);
}

string::string(): initLen(256)
{
	len = 255;
	arr = new BYTE[initLen]{'\0'};
}

string::string(BYTE length): initLen(length + 1)
{
	len = length;
	arr = new BYTE[initLen]{'\0'};
}

string::string(const char* text): initLen(256)
{
	size_t input_len = strlen(text);
	if (input_len > 255) input_len = 255;
	this->len = (BYTE)input_len - 1;
	arr = new BYTE[initLen]{'\0'};	
	memcpy((void*)arr, (void*)text, len);
}

string::string(const string& ps): initLen(ps.initLen)
{
	this->len = ps.len;
	arr = new BYTE[len]{ '\0' };
	memcpy((void*)arr, (void*)ps.arr, len);
}

string::~string()
{
	delete[] arr;
}

const char* string::c_str()
{
	arr[len] = '\0'; // 'arr' je o 1 větší než 'len'
	return (const char*)arr;
}

BYTE& string::operator[](size_t i)
{
	if (i == 0) return len;
	if (i > len) { throw std::exception("Index out of range."); }
	i--;
	return arr[i];
}

void string::operator=(const char* newvalue)
{
	size_t newLen = strlen(newvalue);
	if (newLen > initLen - 1) { throw std::exception("Index out of range."); }
	memcpy((void*)arr, (void*)newvalue, newLen);
	len = newLen;
	arr[len - 1] = '\0';
}

void string::operator=(const string& newvalue)
{
	if (newvalue.initLen >= this->initLen) { throw std::exception("Index out of range."); }
	memcpy((void*)arr, (void*)newvalue.arr[0], newvalue.len);
	arr[len - 1] = '\0';
}

string::operator std::basic_string<char>() const
{
	arr[len] = '\0'; // 'arr' je o 1 větší než 'len'
	const char* exp = (const char*)arr;
	return std::string(exp);
}

