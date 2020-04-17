#include "legacy.h"
#include "windows.h"
#include <filesystem>
#include <fstream>
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


