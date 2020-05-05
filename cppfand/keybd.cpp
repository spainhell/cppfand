#include "keybd.h"

#include "fileacc.h"
#include "kbdww.h"
#include "legacy.h"

void BreakIntHandler()
{
}

void BreakIntrInit()
{
}

void BreakIntrDone()
{
}

void BreakCheck()
{
	if (BreakFlag) {
		BreakFlag = false; ClearKeyBuf(); Halt(-1);
	}
}

unsigned char CurrToKamen(unsigned char C)
{
	return C;
}

BYTE ConvKamenToCurr(unsigned char* Buf, WORD L)
{
	return ' ';
}

void ConvKamenLatin(unsigned char* Buf, WORD L, bool ToLatin)
{
}

unsigned char ToggleCS(unsigned char C)
{
	return C;
}

unsigned char NoDiakr(unsigned char C)
{
	return C;
}

void ConvToNoDiakr(unsigned char* Buf, WORD L, TVideoFont FromFont)
{
}

void ClearKeyBuf()
{
}

void AddToKbdBuf(WORD KeyCode)
{
}

bool KeyPressed()
{
	return true;
}

WORD ReadKey()
{
	return 0;
}

WORD ConvHCU()
{
	return 0;
}

void GetKeyEvent()
{
}

bool KbdTimer(WORD Delta, BYTE Kind)
{
	//longint EndTime;
	auto result = false;
	//EndTime = Timer + Delta;
	//result = false;
	//label1:
	//switch (Kind) {          /* 0 - wait, 1 - wait || ESC, 2 - wait || any key */
	//case 1: if (KeyPressed() && (ReadKey() == _ESC_)) return result;
	//case 2: if (KbdPressed()) { ReadKbd(); return result; }
	//}
	//if (Timer < EndTime) goto label1;
	//result = true;
	return result;
}

void ShowMouse()
{
}

void HideMouse()
{
}

void ResetMouse()
{
}

void MouseEvHandler()
{
}

void InitMouseEvents()
{
}

void SetMouse(WORD X, WORD Y, bool Visible)
{
}

void DoneMouseEvents()
{
}

void HideMaus()
{
}

void ShowMaus()
{
}

void GetRedKeyName()
{
}

void GetMouseEvent()
{
}

void GetMouseKeyEvent()
{
}

void TestGlobalKey()
{
}

WORD AddCtrlAltShift(BYTE Flgs)
{
	return 0;
}

bool TestEvent()
{
	return false;
}


#ifdef Trial
longint getSec()
{
	WORD h, m, s, ss;
	getTime(h, m, s, ss);
	return h * 3600 + m * 60 + s;
}

void TestTrial()
{
	longint now;
	if ((trialStartFand = 0)) { trialStartFand = getSec(); trialInterval = 900; }
	else {
		now = getSec();
		if (now > trialStartFand + trialInterval) {
			trialStartFand = now;
			trialInterval = trialInterval / 3;
			if (trialInterval < 10) trialInterval = 10;
			WrLLF10Msg(71);
		}
	}
}
#endif