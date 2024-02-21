//#include <windows.h>
#include "../fandio/FandFile.h"
#include "../Core/FileD.h"

#ifdef FANDLIBRARY_EXPORTS
#define FANDLIBRARY_API __declspec(dllexport)
#else
#define FANDLIBRARY_API __declspec(dllimport)
#endif

//BOOL APIENTRY DllMain(HMODULE hModule,
//	DWORD  ul_reason_for_call,
//	LPVOID lpReserved
//)
//{
//	switch (ul_reason_for_call)
//	{
//	case DLL_PROCESS_ATTACH:
//	case DLL_THREAD_ATTACH:
//	case DLL_THREAD_DETACH:
//	case DLL_PROCESS_DETACH:
//		break;
//	}
//	return TRUE;
//}


//extern "C" class FANDLIBRARY_API FandFile;
//extern "C" class FANDLIBRARY_API FandXFile;
//extern "C" class FANDLIBRARY_API FandTFile;
//extern "C" class FANDLIBRARY_API FileD;

extern "C" void FANDLIBRARY_API NaserSi()
{
	FileD* fd = new FileD(FType::FandFile);
	FandFile* f = new FandFile(fd);
}
