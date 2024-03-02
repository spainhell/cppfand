#include <windows.h>
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


extern "C" int FAND_API OpenRDB()
{
	std::string p = "C:\\PCFAND\\TEST\\ULOHA.RDB";
	std::string n;

	CfgFile cfgFile;
	cfgFile.Open("FAND.CFG");
	cfgFile.ReadSpec(spec);
	cfgFile.ReadCodeTables(); // for compare strings

	resFile.Open("FAND.RES");
	resFile.ReadInfo(); // read messages

	CompileHelpCatDcl();
	SetTopDir(p, n);
	CreateOpenChpt(n, true);

	//return (int)fd;
	return 999;
}
