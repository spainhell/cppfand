#include "runproc.h"


void WaitProc() // ø. 604
{
	WORD w;
	do
	{
		GetEvent();
		w = Event.What;
		ClrEvent();
	} while (w != evKeyDown && w != evMouseDown);
}
