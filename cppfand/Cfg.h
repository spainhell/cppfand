#pragma once
#include "constants.h"
#include "../Common/codePages.h"
#include <string>

enum class enVideoCard { viCga = 0, viHercules = 1, viEga = 2, viVga = 3 };
enum TKbdConv { OrigKbd, CsKbd, CaKbd, SlKbd, DtKbd };


class Spec
{
public:
	BYTE UpdCount = 0;
	BYTE AutoRprtWidth = 0;
	BYTE AutoRprtLimit = 0;
	BYTE CpLines = 0;
	bool AutoRprtPrint = false;
	bool ChoosePrMsg = false;
	bool TxtInsPg = false;
	char TxtCharPg = 0;
	bool ESCverify = false;
	bool Prompt158 = false;
	bool F10Enter = false;
	bool RDBcomment = false;
	char CPMdrive = 0;
	WORD RefreshDelay = 0;
	WORD NetDelay = 0;
	BYTE LockDelay = 0;
	BYTE LockRetries = 0;
	bool Beep = false;
	bool LockBeepAllowed = false;
	WORD XMSMaxKb = 0;
	bool NoCheckBreak = false;
	TKbdConv KbdTyp = OrigKbd;
	bool NoMouseSupport = false;
	bool MouseReverse = false;
	BYTE DoubleDelay = 0;
	BYTE RepeatDelay = 0;
	BYTE CtrlDelay = 0;
	bool OverwrLabeledDisk = false;
	WORD ScreenDelay = 0;
	BYTE OffDefaultYear = 0;
	bool WithDiskFree = false;
};

struct Video // r. 345
{
	WORD address;
	BYTE TxtRows;
	bool ChkSnow;	// {not used }
	WORD CursOn, CursOff, CursBig;
};

struct Fonts // r350
{
	TVideoFont VFont = TVideoFont::foLatin2;
	bool LoadVideoAllowed = false;
	bool NoDiakrSupported = false;
};

struct Printer {
	std::string Strg;
	char Typ, Kod;
	BYTE Lpti, TmOut;
	bool OpCls, ToHandle, ToMgr;
	WORD Handle;
};

struct wdaystt { BYTE Typ = 0; WORD Nr = 0; };

