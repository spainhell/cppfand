#include "drivers.h"
#include <iostream>
#include "base.h"

void TPoint::Assign(WORD XX, WORD YY)
{
	// asm les di,Self; mov ax,XX; mov es:[di].TPoint.X,ax;
	// mov ax, YY; mov es : [di] .TPoint.Y, ax end;
}

void Beep()
{
	std::cout << '\a';
}

void LockBeep()
{
	if (spec.LockBeepAllowed) std::cout << '\a';
}
