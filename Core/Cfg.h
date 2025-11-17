#pragma once

#include "../Common/codePages.h"
#include <string>

enum class enVideoCard { viCga = 0, viHercules = 1, viEga = 2, viVga = 3 };
enum TKbdConv { OrigKbd, CsKbd, CaKbd, SlKbd, DtKbd };


class Spec
{
public:
	uint8_t UpdCount = 0;
	uint8_t AutoRprtWidth = 0;
	uint8_t AutoRprtLimit = 0;
	uint8_t CpLines = 0;
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
	uint8_t LockDelay = 0;
	uint8_t LockRetries = 0;
	bool Beep = false;
	bool LockBeepAllowed = false;
	WORD XMSMaxKb = 0;
	bool NoCheckBreak = false;
	TKbdConv KbdTyp = OrigKbd;
	bool NoMouseSupport = false;
	bool MouseReverse = false;
	uint8_t DoubleDelay = 0;
	uint8_t RepeatDelay = 0;
	uint8_t CtrlDelay = 0;
	bool OverwrLabeledDisk = false;
	WORD ScreenDelay = 0;
	uint8_t OffDefaultYear = 0;
	bool WithDiskFree = false;
};

struct Video // r. 345
{
	WORD address;
	uint8_t TxtRows;
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
	uint8_t Lpti, TmOut;
	bool OpCls, ToHandle, ToMgr;
	WORD Handle;
};

struct wdaystt { uint8_t Typ = 0; WORD Nr = 0; };

