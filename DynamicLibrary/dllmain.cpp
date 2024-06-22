#include <iostream>
#include <ostream>
#include <windows.h>

#include "../Common/textfunc.h"
#include "../Core/CfgFile.h"
#include "../Core/GlobalVariables.h"
#include "../Core/runproj.h"
#include "../Core/runfand.h"

#ifdef _EXPORTS
#define FAND_API __declspec(dllexport)
#else
#define FAND_API __declspec(dllimport)
#endif

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
		printf("WAITING FOR DEBUGGER !!!");
		while (!::IsDebuggerPresent())
			::Sleep(1000);
		printf(" ... DONE\n");
#endif
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

RdbD* rdb = nullptr;
FileD* rdbFile = nullptr;
uint8_t* data = nullptr;
std::string code;


std::string ConvertCP852toUnicode(std::string cp852)
{
	int len = MultiByteToWideChar(852, 0, cp852.c_str(), (int)cp852.length(), NULL, 0);
	uint8_t* uni = new byte[len * 2]{ 0 };
	int result = MultiByteToWideChar(852, 0, cp852.c_str(), (int)cp852.length(), (LPWSTR)uni, len);
	std::string uni_str = std::string((char*)uni, len * 2);
	delete[] uni; uni = nullptr;
	return uni_str;
}

std::string ConvertUnicodetoCP852(char* utf8)
{
	int containsUnsupported;
	int len = WideCharToMultiByte(852, 0, (LPCWCH)utf8, -1, NULL, 0, NULL, &containsUnsupported);
	uint8_t* cp852 = new byte[len]{ 0 };
	int result = WideCharToMultiByte(852, 0, (LPCWCH)utf8, -1, (LPSTR)cp852, len, NULL, &containsUnsupported);
	std::string cp852str = std::string((char*)cp852, len - 1);
	delete[] cp852; cp852 = nullptr;
	return cp852str;
}

extern "C" int FAND_API OpenRDB(char* rdbName)
{
	std::string p = rdbName;
	std::string n;

	CfgFile cfgFile;
	cfgFile.Open("FAND.CFG");
	cfgFile.ReadSpec(spec);
	cfgFile.ReadCodeTables(); // for compare strings

	resFile.Open("FAND.RES");
	resFile.ReadInfo(); // read messages

	IsTestRun = true; // debug mode - open files in rdb/W mode
	CompileHelpCatDcl();
	SetTopDir(p, n);
	CreateOpenChpt(n, true);

	rdb = CRdb;
	rdbFile = rdb->v_files[0];
	data = rdbFile->GetRecSpace();

	return rdbFile->FF->NRecs;
}

extern "C" int FAND_API GetRecordsCount()
{
	if (rdbFile == nullptr) {
		return -1;
	}
	return rdbFile->FF->NRecs;
}

extern "C" int FAND_API LoadRecord(int32_t recNr)
{
	if (recNr == 0 || recNr > rdbFile->FF->NRecs) {
		return -1;
	}
	rdbFile->ReadRec(recNr, data);
	return 0;
}

extern "C" void FAND_API GetChapterType(char* chapterType)
{
	std::string chapter_type = rdbFile->loadS(rdbFile->FldD[3], data);
	memcpy(chapterType, chapter_type.c_str(), chapter_type.length());
}

extern "C" void FAND_API GetChapterName(char* chapterName)
{
	std::string chapter_name = rdbFile->loadS(rdbFile->FldD[4], data);
	chapter_name = ConvertCP852toUnicode(chapter_name);
	memcpy(chapterName, chapter_name.c_str(), chapter_name.length());
}

extern "C" int FAND_API GetChapterCodeLength()
{
	code = rdbFile->loadS(rdbFile->FldD[5], data);
	code = ConvertCP852toUnicode(code);
	return static_cast<int>(code.length());
}

extern "C" void FAND_API GetChapterCode(char* chapterCode)
{
	memcpy(chapterCode, code.c_str(), code.length());
}

extern "C" int FAND_API CloseRecord(int32_t recNr)
{
	return 0;
}

extern "C" int FAND_API ClearRdb()
{
	rdbFile->FF->NRecs = 0;
	return 0;
}

extern "C" int FAND_API SaveChapter(char* chapterType, char* chapterName, char* chapterCode)
{
	rdbFile->ClearRecSpace(data);

	std::string chapter_type = ConvertUnicodetoCP852(chapterType);
	rdbFile->saveS(rdbFile->FldD[3], chapter_type, data);

	std::string chapter_name = ConvertUnicodetoCP852(chapterName);
	rdbFile->saveS(rdbFile->FldD[4], chapter_name, data);

	std::string chapter_code = ConvertUnicodetoCP852(chapterCode);
	rdbFile->saveS(rdbFile->FldD[5], chapter_code, data);
	
	rdbFile->CreateRec(rdbFile->FF->NRecs + 1, data);

	return rdbFile->FF->NRecs;
}

extern "C" int FAND_API CloseRdb()
{
	int result = rdbFile->FF->NRecs;
	rdbFile->FF->SaveFile();
	rdbFile->FF->CloseFile();
	return result;
}