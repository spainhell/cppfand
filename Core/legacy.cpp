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
#include "../Common/textfunc.h"
#include "../Common/CommonVariables.h"
#include "../fandio/directory.h"

std::vector<std::string> paramstr;
int ExitCode = 0; // exit kód -> OS
void* ErrorAddr = nullptr; // adresa chyby
void (*ExitProc)() { }; // ukonèovací procedura


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

void GetDir(uint8_t disk, pstring* cesta)
{
	char buf[MAX_PATH];
	if (_getcwd(buf, MAX_PATH) == nullptr)
	{
		HandleError = errno;
	}
	*cesta = buf;
}

std::string GetDir(uint8_t disk)
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

std::string GetEnv(const char* name)
{
	std::string result;
	size_t requiredSize = 0;
	getenv_s(&requiredSize, NULL, 0, name);
	if (requiredSize == 0) {
		result = "";
	}
	else {
		unique_ptr<char[]> buffer = std::make_unique<char[]>(requiredSize * sizeof(char));
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
	/*
	// ulozi data do souboru a zavre jej
	// ukladame po 1 znaku, aby bylo mozno ukladat misto '\n' cely '\r\n'
	char charCR = '\r';
	for (size_t i = 0; i < strlen(data); i++) {
		if (data[i] == '\n') {
			fwrite(&charCR, 1, 1, Handle);
		}
		fwrite(&data[i], 1, 1, Handle);
	}
	*/
	
	fwrite(data, 1, strlen(data), Handle);
	HandleError = ferror(Handle);
	fclose(Handle);
	Handle = nullptr;
	//WriteF(Handle, (void*)data, strlen(data), HandleError);
	//CloseF(Handle, HandleError);
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
	//buffer = new uint8_t[pos];
	//fseek(Handle, 0, SEEK_SET);
	//fread_s(buffer, pos, sizeof(uint8_t), pos, Handle);
	//fclose(Handle);
	//Handle = nullptr;
	return true;
}
