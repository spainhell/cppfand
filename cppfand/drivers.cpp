#include "drivers.h"

#include <iostream>

#include "base.h"

void Drivers::Beep()
{
	std::cout << '\a';
}

void Drivers::LockBeep()
{
	if (spec.LockBeepAllowed) std::cout << '\a';
}
