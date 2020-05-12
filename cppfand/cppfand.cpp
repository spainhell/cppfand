#include <vector>
#include "base.h"
#include "drivers.h"
#include "runfand.h"

int main(int argc, char* argv[])
{
	for (int i = 0; i < argc; i++)
	{
		paramstr.push_back(argv[i]);
	}
	//globconf::paramstr.push_back("pokus.rdb");
	//paramstr2.push_back("pokus.rdb");


	//pstring ahoj = "AHOJ";
	//pstring alena = copy(ahoj, 0, 2);
	
	
	InitRunFand();
}

