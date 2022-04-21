#include "legacy.h"
#include <SDKDDKVer.h>
#include "windows.h"
#include <ctime>
#include <direct.h>
#include <filesystem>

#include "base.h"
#include <iostream>
#include <fstream>

#include "GlobalVariables.h"
#include "../textfunc/textfunc.h"
#include "../FileSystem/directory.h"

std::vector<std::string> paramstr;
longint ExitCode = 0; // exit kód -> OS
void* ErrorAddr = nullptr; // adresa chyby
void (*ExitProc)() { }; // ukončovací procedura
BYTE OvrResult = 0; // vždy 0 OvrOK



void val(pstring s, BYTE& b, WORD& err)
{
	size_t sz;
	b = std::stoul(s.c_str(), &sz, 10);
	// prelozil se cely retezec?
	if (sz == s.length() - 1) {	err = 0; }
	else { err = static_cast<WORD>(sz); }
}

void val(pstring s, WORD& b, WORD& err)
{
	if (s.empty()) return;
	size_t idx;
	b = std::stoul(s.c_str(), &idx, 10);
	// prelozil se cely retezec?
	if (idx == s.length()) { err = 0; }
	else { err = static_cast<WORD>(idx); }
}

void val(pstring s, integer& b, integer& err)
{
	if (s.empty()) return;
	size_t idx;
	try {
		b = std::stoi(s.c_str(), &idx, 10);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<integer>(idx); }
	}
	catch (std::invalid_argument& e) {
		b = 0;
		err = 1;
	}
}

void val(pstring s, double& b, integer& err)
{
	size_t idx = 0;
	try {
		b = std::stod(s.c_str(), &idx);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<integer>(idx); }
	}
	catch (std::invalid_argument& e) {
		b = 0;
		err = 1;
	}
}

void val(pstring s, double& b, WORD& err)
{
	if (s.length() == 0) {
		b = 0; err = 0;
		return;
	}
	size_t idx = 0;
	try {
		b = std::stod(s.c_str(), &idx);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<WORD>(idx); }
	}
	catch (std::invalid_argument& e) {
		b = 0;
		err = 1;
	}
}

void val(pstring s, longint& b, WORD& err)
{
	if (s.length() == 0) { err = 1;	return;	}
	size_t idx = 0;
	try {
		b = std::stoi(s.c_str(), &idx);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<WORD>(idx); }
	}
	catch (std::invalid_argument& e) {
		b = 0;
		err = 1;
	}
}

void val(pstring s, longint& b, integer& err)
{
	if (s.length() == 0) { err = 1;	return; }
	size_t idx = 0;
	try {
		b = std::stoi(s.c_str(), &idx);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<WORD>(idx); }
	}
	catch (std::invalid_argument& e) {
		b = 0;
		err = 1;
	}
}

double valDouble(std::string& s, integer& err)
{
	if (s.length() == 0) {
		err = 0;
		return 0;
	}
	double result = 0;
	size_t idx = 0;
	try {
		result = std::stod(s.c_str(), &idx);
		// prelozil se cely retezec?
		if (idx == s.length()) { err = 0; }
		else { err = static_cast<WORD>(idx); }
	}
	catch (std::invalid_argument& e) {
		result = 0;
		err = 1;
	}
	return result;
}

pstring copy(pstring source, size_t index, size_t count)
{
	return source.substr(index - 1, count);
}

void str(int input, pstring& output)
{
	std::string a = std::to_string(input);
	output.replace(a.c_str());
}

void str(double input, int total, int right, pstring& output)
{
	char buffer[255];
	snprintf(buffer, sizeof(buffer), "%*.*f", total, right, input);
	output = buffer;
}

void str(double input, int total, int right, std::string& output)
{
	char buffer[255];
	snprintf(buffer, sizeof(buffer), "%*.*f", total, right, input);
	output = buffer;
}

void str(double input, int right, pstring& output)
{
	char buffer[255];
	snprintf(buffer, sizeof(buffer), "%.*f", right, input);
	output = buffer;
}

void str(double input, int right, std::string& output)
{
	char buffer[255];
	snprintf(buffer, sizeof(buffer), "%.*f", right, input);
	output = buffer;
}

WORD pred(WORD input)
{
	if (input <= 0) return 0;
	return input - 1;
}

WORD succ(WORD input)
{
	if (input == 0xFFFF) return 0xFFFF;
	return input + 1;
}

void FSplit(pstring fullname, pstring& dir, pstring& name, pstring& ext)
{
	std::string s = fullname;
	size_t foundBackslash = s.find_last_of("/\\");
	dir = s.substr(0, foundBackslash + 1);

	if (foundBackslash < s.length() - 1) // ještě nejsme na konci -> existuje název nebo název s příponou
	{
		std::string filename = s.substr(foundBackslash + 1);
		
		size_t foundDot = filename.find_last_of('.');
		if (foundDot < filename.length()) // přípona existuje
		{
			name = filename.substr(0, foundDot);
			ext = filename.substr(foundDot);
		}
		else
		{
			name = filename;
			ext = "";
		}
	}
	else
	{	
		if (foundBackslash != std::string::npos) {
			// lomitko je na konci, existuje jen adresar
			name = ""; ext = "";
			return;
		}

		// lomitko nebylo nalezeno, bude exitovat jen nazev a mozna pripona
		size_t foundDot = s.find_last_of('.');
		if (foundDot < s.length()) // pripona existuje
		{
			name = s.substr(0, foundDot);
			ext = s.substr(foundDot);
		}
		else
		{ // přípona neexistuje, celé je to název souboru
			name = s;
			ext = "";
		}
	}
}

void FSplit(const std::string& fullname, std::string& dir, std::string& name, std::string& ext, char pathDelim)
{
	std::filesystem::path pth = fullname;
	bool isDir = is_directory(pth);

	if (isDir) {
		dir = pth.generic_string();
		name = "";
		ext = "";
	}
	else
	{
		name = pth.stem().string();
		ext = pth.extension().string();
		dir = pth.generic_string().substr(0, pth.generic_string().length() - name.length() - ext.length());
	}
	
	if (pathDelim == '/') ReplaceChar(dir, '\\', '/');
	if (pathDelim == '\\') ReplaceChar(dir, '/', '\\');

	return;

	size_t foundBackslash = fullname.find_last_of("/\\");
	dir = fullname.substr(0, foundBackslash + 1);

	if (foundBackslash < fullname.length() - 1) // jeste nejsme na konci -> existuje nazev nebo nazev s priponou
	{
		std::string filename = fullname.substr(foundBackslash + 1);

		size_t foundDot = filename.find_last_of('.');
		if (foundDot < filename.length()) // pripona existuje
		{
			name = filename.substr(0, foundDot);
			ext = filename.substr(foundDot);
		}
		else
		{
			name = filename;
			ext = "";
		}
	}
	else
	{
		if (foundBackslash != std::string::npos) {
			// lomitko je na konci, existuje jen adresar
			name = ""; ext = "";
			return;
		}

		// lomitko nebylo nalezeno, bude exitovat jen nazev a mozna pripona
		size_t foundDot = fullname.find_last_of('.');
		if (foundDot < fullname.length()) // pripona existuje
		{
			name = fullname.substr(0, foundDot);
			ext = fullname.substr(foundDot);
		}
		else
		{ // pripona neexistuje, cele je to nazev souboru
			name = fullname;
			ext = "";
		}
	}
}

pstring FSearch(pstring& path, pstring& dirlist)
{
	//std::vector<std::string> vDirs;
	//int lastIndex = 0;
	//int actualIndex = 0;

	//std::string slist = dirlist;
	//for (actualIndex = 0; actualIndex < dirlist.length(); actualIndex++)
	//{
	//	if (actualIndex > 0 && dirlist[actualIndex] == ';')
	//	{
	//		std::string dir = dirlist.substr(lastIndex, actualIndex - lastIndex).c_str();
	//		if (dir[dir.length() - 1] != '\\') dir+= '\\';
	//		vDirs.push_back(dir);
	//		lastIndex = actualIndex + 1;
	//	}
	//}
	//for (auto & dir : vDirs)
	//{
	//	std::string fullname = dir + path.c_str();
	//	FILE* file;
	//	if (!fopen_s(&file, fullname.c_str(), "r")) {
	//		fclose(file);
	//		return fullname;
	//	}
	//}
	return path;
}

std::string FSearch(const std::string path, const std::string dirlist)
{
	std::string result;

	auto dirs = SplitString(dirlist, ';');
	for (auto& d : dirs) {
		AddBackSlash(d);
		auto fullpath = d + path;
		if (fileExists(fullpath) == 0) {
			result = fullpath;
			break;
		}
	}
	return result;
}

std::string FExpand(std::string path, char pathDelim)
{
	std::string dir, name, ext;
	FSplit(path, dir, name, ext);

	// je cesta kompletni?
	if (dir.length() > 0 && name.length() > 0 && ext.length() > 0) {
		// je to kompletni soubor
		if (pathDelim == '/') ReplaceChar(path, '\\', '/');
		if (pathDelim == '\\') ReplaceChar(path, '/', '\\');
		return path;
	}
	if (dir.length() > 0) {
		// je to jen adresar
		if (pathDelim == '/') ReplaceChar(dir, '\\', '/');
		if (pathDelim == '\\') ReplaceChar(dir, '/', '\\');
		return dir;
	} 

	std::string fullpath = GetDir(0);
	if (name.length() > 0 || ext.length() > 0) {
		fullpath += "\\";
		fullpath += name;
		if (ext.length() > 0) { fullpath += ext; }
	}

	if (pathDelim == '/') ReplaceChar(fullpath, '\\', '/');
	if (pathDelim == '\\') ReplaceChar(fullpath, '/', '\\');
	return fullpath;
}

void ChDir(std::string cesta)
{
	if (_chdir(cesta.c_str())) {
		HandleError = errno;
	}
}

void GetDir(BYTE disk, pstring* cesta)
{
	char buf[MAX_PATH];
	if (_getcwd(buf, MAX_PATH) == nullptr)
	{
		HandleError = errno;
	}
	*cesta = buf;
}

std::string GetDir(BYTE disk)
{
	char buf[MAX_PATH];
	if (_getcwd(buf, MAX_PATH) == nullptr)
	{
		HandleError = errno;
	}
	return buf;
}

void MkDir(std::string cesta)
{
	if (_mkdir(cesta.c_str()))
	{
		HandleError = errno;
	}
}

void RmDir(std::string cesta)
{
	if (_rmdir(cesta.c_str()) == -1)
	{
		HandleError = errno;
	}
}

void Rename(std::string soubor, std::string novejmeno)
{
	if (rename(soubor.c_str(), novejmeno.c_str()) != 0)
	{
		HandleError = errno;
	}
}

void Erase(std::string soubor)
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

WORD ParamCount()
{
	return (WORD)paramstr.size();
}

pstring ParamStr(integer index)
{
	if (index >= paramstr.size()) return "";
	pstring ptmp = paramstr[index].c_str();
	return ptmp;
}

void FillChar(void* cil, int delka, size_t vypln)
{
	memset(cil, vypln, delka);
}

void Move(void* zdroj, void* cil, WORD delka)
{
	memmove(cil, zdroj, delka);
}

BYTE Hi(WORD cislo)
{
	return cislo >> 8;
}

BYTE Lo(WORD cislo)
{
	return cislo & 0x00FF;
}

WORD Swap(WORD cislo)
{
	return ((cislo & 0x00FF) << 4) + (cislo >> 4);
}

void beep()
{
}

std::string GetEnv(const char* name)
{
	std::string result;
	size_t requiredSize = 0;
	getenv_s(&requiredSize, NULL, 0, name);
	if (requiredSize == 0)
	{
		result = "";
	}
	else {
		auto buffer = std::make_unique<char[]>(requiredSize * sizeof(char));
		getenv_s(&requiredSize, buffer.get(), requiredSize, name);
		result = std::string(buffer.get());
	}
	return result;
}

WORD IOResult()
{
	return 0;
}

WORD DosError()
{
	return 0;
}

double DiskFree(char disk)
{
	return 1000000000.0;
}

TextFile::~TextFile()
{
	delete[] buffer;
}

const char* TextFile::c_str()
{
	return (const char*)buffer;
}

void TextFile::Close(const char* data)
{
	// ulozi data do souboru a zavre jej
	// ukladame po 1 znaku, aby bylo mozno ukladat misto '\n' cely '\r\n'
	char charCR = '\r';
	for (size_t i = 0; i < strlen(data); i++) {
		if (data[i] == '\n') {
			fwrite(&charCR, 1, 1, Handle);
		}
		fwrite(&data[i], 1, 1, Handle);
	}
	
	//fwrite(data, 1, strlen(data), Handle);
	HandleError = ferror(Handle);
	fclose(Handle);
	Handle = nullptr;
}

void TextFile::Assign(std::string FullPath)
{
	this->FullPath = FullPath;
}

void TextFile::Reset()
{
}

void TextFile::Rewrite()
{
	// otevre soubor pro zapis
}

bool TextFile::ResetTxt()
{
	auto HandleError = fopen_s(&Handle, FullPath.c_str(), "r");
	//fseek(Handle, 0, SEEK_END);
	//auto pos = ftell(Handle);
	//buffer = new BYTE[pos];
	//fseek(Handle, 0, SEEK_SET);
	//fread_s(buffer, pos, sizeof(BYTE), pos, Handle);
	//fclose(Handle);
	//Handle = nullptr;
	return true;
}
