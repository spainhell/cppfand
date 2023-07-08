#include "RunMessage.h"
#include "GlobalVariables.h"
#include "obaseww.h"

RunMsgD* CM = nullptr;

void RunMsgOn(char C, int N)
{
	RunMsgD* CM1 = new RunMsgD();
#ifndef norunmsg
	CM1->Last = CM;
	CM = CM1;
	CM->MsgStep = N / 100;
	if (CM->MsgStep == 0) {
		CM->MsgStep = 1;
	}
	CM->MsgKum = CM->MsgStep;
	CM->MsgNN = N;
	CM->W = PushW(1, TxtRows, 8, TxtRows, true, true);
	TextAttr = screen.colors.zNorm;

	screen.ScrFormatWrStyledText(1, 1, TextAttr, "%c%c", /*0xAF*/ 0x10, C);
	if (N == 0) {
		screen.ScrFormatWrStyledText(3, 1, TextAttr, "    %c", /*0xAE*/ 0x11);
	}
	else {
		screen.ScrFormatWrStyledText(3, 1, TextAttr, "  0%c%c", '%', /*0xAE*/ 0x11);
	}
#endif
}

void RunMsgOff()
{
#ifndef norunmsg
	if (CM == nullptr) return;
	PopW(CM->W);
	CM = CM->Last;
#endif
}

void RunMsgN(int n)
{
#ifndef norunmsg
	if (n < CM->MsgKum) return;
	while (n >= CM->MsgKum) {
		CM->MsgKum += CM->MsgStep;
	}
	WORD percent;
	if (CM->MsgNN == 0) {
		// print 100%
		screen.ScrFormatWrText(3, 1, "%*i", 3, 0);
	}
	else {
		screen.ScrFormatWrText(3, 1, "%*i", 3, n * 100 / CM->MsgNN);
	}
#endif
}

void RunMsgClear()
{
	if (CM != nullptr) {
		delete CM;
		CM = nullptr;
	}
	else {
		// object CM not exists
	}
}
