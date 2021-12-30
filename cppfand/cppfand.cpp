#include <vector>
#include "OldDrivers.h"
#include "runfand.h"
#include "../Logging/Logging.h"

int main(int argc, char* argv[])
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::INFO, "*** *** *** *** *** *** APPLICATION STARTED *** *** *** *** *** ***");
	
	for (int i = 0; i < argc; i++)
	{
		paramstr.push_back(argv[i]);
	}
	
	InitRunFand();
	DeleteFandFiles();
	log->log(loglevel::INFO, "*** *** *** *** *** ***  APPLICATION ENDED   *** *** *** *** *** ***");
	Logging::finish();
	system("cls");
}
