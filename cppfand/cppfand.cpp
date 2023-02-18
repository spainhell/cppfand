#include <vector>
#include "OldDrivers.h"
#include "runfand.h"
#include "../Logging/Logging.h"
#include "../SpdLog/sinks/basic_file_sink.h"

int main(int argc, char* argv[])
{
	// Create basic file logger (not rotated)
	auto my_logger = spdlog::basic_logger_mt("basic_logger", "logs/basic.txt");
	my_logger->info("Some log message");


	Logging* log = Logging::getInstance();
	log->log(loglevel::INFO, "*** *** *** *** *** *** APPLICATION STARTED *** *** *** *** *** ***");

	//system("pause");

	for (int i = 0; i < argc; i++)
	{
		paramstr.push_back(argv[i]);
	}

	try 
	{
		InitRunFand();
	}
	catch (std::exception ex)
	{
		log->log(loglevel::EXCEPTION, "%s", ex.what());
	}

	// finish
	DeleteFandFiles();
	log->log(loglevel::INFO, "*** *** *** *** *** ***  APPLICATION ENDED   *** *** *** *** *** ***");
	Logging::finish();
	system("cls");
}
