#include <vector>
#include "base.h"
#include "drivers.h"
#include "runfand.h"
#include <memory>


int main(int argc, char* argv[])
{
	for (int i = 0; i < argc; i++)
	{
		paramstr.push_back(argv[i]);
	}
	
	InitRunFand();
	system("cls");
}

