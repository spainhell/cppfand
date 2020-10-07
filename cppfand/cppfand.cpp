#include <vector>
#include "base.h"
#include "drivers.h"
#include "runfand.h"
#include "../Logging/spdlog.h"
#include <memory>

using namespace ::std;
namespace spd = spdlog;

int main(int argc, char* argv[])
{
	std::shared_ptr<spd::logger> logger;
	
	logger->debug("STARTING APPLICATION");
	
	for (int i = 0; i < argc; i++)
	{
		paramstr.push_back(argv[i]);
	}
	
	InitRunFand();
	system("cls");
}

