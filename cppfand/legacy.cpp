#include "legacy.h"
#include "windows.h"
#include <iostream>
#include <string>




void val(pstring s, BYTE& b, WORD& err)
{
	unsigned int sz;
	auto a = std::stoul(s.c_str(), &sz, 10);
	// přeložil se celý řetězec?
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

pstring copy(pstring source, size_t index, size_t count)
{
	std::string temp = source;
	pstring result = temp.substr(index, count).c_str();
	return result;
}

void FSplit(pstring fullname, pstring& dir, pstring& name, pstring& ext)
{
	std::string s = fullname;
	size_t found;
	found = s.find_last_of("/\\");
	dir = s.substr(0, found);
	std::string filename = s.substr(found + 1);
	found = filename.find_last_of('.');
	name = filename.substr(0, found - 1);
	ext = filename.substr(found);
}

void GetDir(BYTE disk, pstring& cesta)
{
	const unsigned long maxDir = 260;
	char currentDir[maxDir];
	GetCurrentDirectory(maxDir, currentDir);
	cesta = currentDir;
}

WORD ParamCount()
{
	return (WORD)paramstr.size();
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

char* GetEnv(const char* name)
{
	size_t value = 0;
	char buffer[256];
	getenv_s(&value, buffer, 256, name);
	return &buffer[0];
}




