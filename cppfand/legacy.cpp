#include "legacy.h"
#include <SDKDDKVer.h>
#include "windows.h"
#include <ctime>
#include <iostream>
#include <string>
#include <direct.h>
#include "base.h"


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

void str(int input, pstring& output)
{
	std::string a = std::to_string(input);
	output.replace(a.c_str());
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

pstring FSearch(pstring& path, pstring& dirlist)
{
	std::vector<std::string> vDirs;
	int lastIndex = 0;
	int actualIndex = 0;

	std::string slist = dirlist;
	for (actualIndex = 0; actualIndex < dirlist.length(); actualIndex++)
	{
		if (actualIndex > 0 && dirlist[actualIndex] == ';')
		{
			std::string dir = dirlist.substr(lastIndex, actualIndex - lastIndex);
			if (dir[dir.length() - 1] != '\\') dir+= '\\';
			vDirs.push_back(dir);
			lastIndex = actualIndex + 1;
		}
	}
	for (auto & dir : vDirs)
	{
		std::string fullname = dir + path.c_str();
		FILE* file;
		if (!fopen_s(&file, fullname.c_str(), "r")) {
			fclose(file);
			return fullname;
		}
	}
	return pstring();
}

pstring FExpand(pstring path)
{
	pstring fullpath = pstring(255);
	GetDir(0, path);
	fullpath += "\\";
	fullpath += path;
	return fullpath;
}

void ChDir(pstring cesta)
{
	if (_chdir(cesta.c_str())) {
		HandleError = errno;
	}
}

void GetDir(BYTE disk, pstring& cesta)
{
	char buf[MAX_PATH];
	if (_getcwd(buf, MAX_PATH) == nullptr)
	{
		HandleError = errno;
	}
	cesta = buf;
}

void MkDir(pstring cesta)
{
	if (_mkdir(cesta.c_str()))
	{
		HandleError = errno;
	}
}

void RmDir(pstring cesta)
{
	if (_rmdir(cesta.c_str()) == -1)
	{
		HandleError = errno;
	}
}

void Rename(pstring soubor, pstring novejmeno)
{
	if (rename(soubor.c_str(), novejmeno.c_str()) != 0)
	{
		HandleError = errno;
	}
}

void Erase(pstring soubor)
{
	if (remove(soubor.c_str()) == -1)
	{
		HandleError = errno;
	}
}

void InitGraph(short GraphDriver, short GraphMode, pstring PathToDriver)
{
	return;
}

void CloseGraph()
{
	return;
}

double Random()
{
	srand(time(0));
	double randnr = rand();
	while (randnr >= 1)
	{
		randnr = randnr / 10;
	}
	return randnr;
}

WORD Random(WORD rozsah)
{
	srand(time(0));
	double randnr = rand();
	while (randnr >= rozsah)
	{
		randnr = randnr / 2;
	}
	return (WORD)randnr;
}

WORD ParamCount()
{
	return (WORD)paramstr.size();
}

pstring ParamStr(integer index)
{
	return paramstr[index];
}

void FillChar(char* cil, WORD delka, char vypln)
{
	memset((void*)cil, vypln, delka);
}

void Move(void* zdroj, void* cil, WORD delka)
{
	memmove(cil, zdroj, delka);
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

void UnPack(void* PackArr, WORD& NumArr, WORD& NoDigits)
{
	return;
}

void Pack(void* NumArr, WORD& PackArr, WORD& NoDigits)
{
	return;
}

pstring GetEnv(const char* name)
{
	size_t value = 0;
	char buffer[256];
	getenv_s(&value, buffer, 256, name);
	return pstring(buffer);
}




