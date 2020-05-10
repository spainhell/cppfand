#include <vector>
#include "base.h"
#include "drivers.h"
#include "globconf.h"
#include "runfand.h"

int main(int argc, char* argv[])
{
	for (int i = 0; i < argc; i++)
	{
		globconf::GetInstance()->paramstr.push_back(argv[i]);
	}
	//globconf::paramstr.push_back("pokus.rdb");
	//paramstr2.push_back("pokus.rdb");
	InitRunFand();
}

