#include "drivers.h"
#include <iostream>
#include "base.h"

void Beep()
{
	std::cout << '\a';
}

void LockBeep()
{
	if (spec.LockBeepAllowed) std::cout << '\a';
}
