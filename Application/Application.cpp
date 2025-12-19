#include <vector>
#include "../Core/OldDrivers.h"
#include "../Core/runfand.h"
#include "../Logging/Logging.h"

int main(int argc, char* argv[])
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::INFO, "*** *** *** *** *** *** APPLICATION STARTED *** *** *** *** *** ***");

	//system("pause");

	for (int i = 0; i < argc; i++)
	{
		paramstr.push_back(argv[i]);
	}

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	printf("WAITING FOR DEBUGGER !!!");
	while (!::IsDebuggerPresent())
		::Sleep(1000);
	printf(" ... DONE\n");
#endif

	try 
	{
		InitRunFand();
	}
	catch (std::exception& ex)
	{
		log->log(loglevel::EXCEPTION, "%s", ex.what());
	}

	// finish
	DeleteFandFiles();
	log->log(loglevel::INFO, "*** *** *** *** *** ***  APPLICATION ENDED   *** *** *** *** *** ***");
	Logging::finish();
	system("cls");
}
