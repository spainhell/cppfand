#include "kbdww.h"

#include "base.h"
#include "common.h"
#include "drivers.h"
#include "memory.h"

void WrLLF10MsgLine()
{
	WORD Buf[MaxTxtCols];
	WORD col, row, len;

	row = TxtRows - 1;
	Drivers::ScrRdBuf(0, row, &Buf[0], TxtCols);
	Drivers::Beep();
	Drivers::ScrClr(0, row, TxtCols, 1, ' ', colors.zNorm);
	if (F10SpecKey == 0xffff) Drivers::ScrWrStr(0, row, "...!", colors.zNorm | 0x80);
	else Drivers::ScrWrStr(0, row, "F10!", colors.zNorm | 0x80);
	col = MsgLine.length() + 5;
	len = 0;
	if ((F10SpecKey == 0xfffe) || (F10SpecKey == _F1_)) {
		MsgLine = MsgLine + ' ' + "F1";
		len = 2;
	}
	if ((F10SpecKey == 0xfffe) || (F10SpecKey == _ShiftF7_)) {
		MsgLine = MsgLine + ' ' + "ShiftF7"; len += 7;
	}
	if (MsgLine.length() > TxtCols - 5) {
		MsgLine[0] = char(TxtCols - 5);
		len = 0;
	}
	Drivers::ScrWrStr(5, row, MsgLine, colors.zNorm);
label1:
	Drivers::GetEvent();
	/*with Event*/
	switch (Event.What) {
	case evMouse:
		if (MouseInRect(0, row, 3, 1))
		{
			KbdChar = _F10_;
			goto label2;
		}
		if (len > 0 && MouseInRect(col, row, len, 1)) {
			KbdChar = F10SpecKey;
			goto label2;
		}
	case evKeyDown:
		if (Event.KeyCode == _F10_ || Event.KeyCode == F10SpecKey || F10SpecKey == 0xffff
			|| F10SpecKey == 0xfffe && (Event.KeyCode == _ShiftF7_ || Event.KeyCode == _F1_))
		{
			KbdChar = Event.KeyCode;
		label2:
			Drivers::ClrEvent();
			goto label3;
		}
	}
	Drivers::ClrEvent();
	goto label1;
label3:
	F10SpecKey = 0;
	Drivers::ScrWrBuf(0, row, &Buf[0], TxtCols);

}

void WrLLF10Msg(WORD N)
{
	RdMsg(N);
	WrLLF10MsgLine();
}

void RunError(WORD N)
{
	RunErrNr = N;
	Drivers::ClearKbdBuf();
	WrLLF10Msg(RunErrNr);
	GoExit();
}
