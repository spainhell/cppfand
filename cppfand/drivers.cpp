#include "drivers.h"

#include <iostream>

#include "base.h"

void Drivers::LockBeep()
{
	if (spec.LockBeepAllowed) std::cout << '\a';
}
