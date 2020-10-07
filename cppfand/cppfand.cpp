#include <vector>
#include "base.h"
#include "drivers.h"
#include "runfand.h"
#include <memory>
//#include "../spdlog/spdlog.h"
//#include "../spdlog/spdlog-inl.h"

int main(int argc, char* argv[])
{
	//auto logger = spdlog::rotating_logger_mt("file_logger", "LOG.log", 1024 * 1024 * 5, 3);
	//logger->set_level(spdlog::level::debug);
	//logger->debug("APP STARTING");
	//spdlog::register_logger(logger);
	
	for (int i = 0; i < argc; i++)
	{
		paramstr.push_back(argv[i]);
	}
	
	InitRunFand();
	system("cls");
}

