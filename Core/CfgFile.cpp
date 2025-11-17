#include "CfgFile.h"
#include "constants.h"
#include "../pascal/real48.h"
#include "../Common/codePages.h"
#include "../Drivers/files.h"

void CfgFile::Open(std::string path)
{
	DWORD error;
	Handle = OpenF(path, error, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
	FullName = path;
	if (error != 0) {
		printf("can't open %s\n", path.c_str());
		system("pause");
		exit(-1);
	}

	char ver[5] = { 0,0,0,0,0 };
	ReadF(Handle, ver, 4, error);
	if (strcmp(ver, CfgVersion) != 0) {
		printf("Invalid version of FAND.CFG");
		system("pause");
		exit(-1);
	}
}

void CfgFile::ReadSpec(Spec& spec)
{
	DWORD error;
	// nacteni SPEC
	ReadF(Handle, &spec.UpdCount, sizeof(spec.UpdCount), error);
	ReadF(Handle, &spec.AutoRprtWidth, sizeof(spec.AutoRprtWidth), error);
	ReadF(Handle, &spec.AutoRprtLimit, sizeof(spec.AutoRprtLimit), error);
	ReadF(Handle, &spec.CpLines, sizeof(spec.CpLines), error);
	ReadF(Handle, &spec.AutoRprtPrint, sizeof(spec.AutoRprtPrint), error);
	ReadF(Handle, &spec.ChoosePrMsg, sizeof(spec.ChoosePrMsg), error);
	ReadF(Handle, &spec.TxtInsPg, sizeof(spec.TxtInsPg), error);
	ReadF(Handle, &spec.TxtCharPg, sizeof(spec.TxtCharPg), error);
	ReadF(Handle, &spec.ESCverify, sizeof(spec.ESCverify), error);
	ReadF(Handle, &spec.Prompt158, sizeof(spec.Prompt158), error);
	ReadF(Handle, &spec.F10Enter, sizeof(spec.F10Enter), error);
	ReadF(Handle, &spec.RDBcomment, sizeof(spec.RDBcomment), error);
	ReadF(Handle, &spec.CPMdrive, sizeof(spec.CPMdrive), error);
	ReadF(Handle, &spec.RefreshDelay, sizeof(spec.RefreshDelay), error);
	ReadF(Handle, &spec.NetDelay, sizeof(spec.NetDelay), error);
	ReadF(Handle, &spec.LockDelay, sizeof(spec.LockDelay), error);
	ReadF(Handle, &spec.LockRetries, sizeof(spec.LockRetries), error);
	ReadF(Handle, &spec.Beep, sizeof(spec.Beep), error);
	ReadF(Handle, &spec.LockBeepAllowed, sizeof(spec.LockBeepAllowed), error);
	ReadF(Handle, &spec.XMSMaxKb, sizeof(spec.XMSMaxKb),error);
	ReadF(Handle, &spec.NoCheckBreak, sizeof(spec.NoCheckBreak), error);
	ReadF(Handle, &spec.KbdTyp, 1, error); // v C++ je enum 4B, original 1B
	ReadF(Handle, &spec.NoMouseSupport, sizeof(spec.NoMouseSupport), error);
	ReadF(Handle, &spec.MouseReverse, sizeof(spec.MouseReverse), error);
	ReadF(Handle, &spec.DoubleDelay, sizeof(spec.DoubleDelay), error);
	ReadF(Handle, &spec.RepeatDelay, sizeof(spec.RepeatDelay), error);
	ReadF(Handle, &spec.CtrlDelay, sizeof(spec.CtrlDelay), error);
	ReadF(Handle, &spec.OverwrLabeledDisk, sizeof(spec.OverwrLabeledDisk), error);
	ReadF(Handle, &spec.ScreenDelay, sizeof(spec.ScreenDelay), error);
	ReadF(Handle, &spec.OffDefaultYear, sizeof(spec.OffDefaultYear), error);
	ReadF(Handle, &spec.WithDiskFree, sizeof(spec.WithDiskFree), error);
	// konec SPEC
}

void CfgFile::ReadVideoAndColors(Video& video, uint8_t start_mode, enVideoCard video_card, Screen& screen, WORD& TxtCols, WORD& TxtRows)
{
	WORD typ;
	DWORD error;

	if (start_mode == 7) typ = 1;
	else if (video_card >= enVideoCard::viEga) typ = 3;
	else typ = 2;

	int sizeVideo = sizeof(video); // 10
	int sizeColors = sizeof(screen.colors); // 54

	SeekF(Handle, error, PosF(Handle, error) + (sizeVideo + sizeColors) * (typ - 1));
	ReadF(Handle, &video, sizeof(video), error);
	ReadF(Handle, &screen.colors, sizeof(screen.colors), error);
	SeekF(Handle, error, PosF(Handle, error) + (sizeof(video) + sizeof(screen.colors)) * (3 - typ));

	// Text Rows from CFG
	if (video.TxtRows != 0) {
		TxtRows = video.TxtRows;
	}
	// Text Columns from Video Address CFG
	if (video.address < 0xFF) {
		TxtCols = video.address;
	}
}

void CfgFile::ReadFonts(Fonts& fonts)
{
	DWORD error;

	ReadF(Handle, &fonts.VFont, 1, error); // enum orginal 1B
	ReadF(Handle, &fonts.LoadVideoAllowed, sizeof(fonts.LoadVideoAllowed), error);
	ReadF(Handle, &fonts.NoDiakrSupported, sizeof(fonts.NoDiakrSupported), error);
}

void CfgFile::ReadCodeTables()
{
	DWORD error;

	ReadF(Handle, CharOrdTab, sizeof(CharOrdTab), error);
	ReadF(Handle, UpcCharTab, sizeof(UpcCharTab), error);
}

void CfgFile::RdPrinter(short& prMax, Printer printer[10])
{
	DWORD error;
	uint8_t L;
	const int NPrintStrg = 32;
	uint8_t A[NPrintStrg * 256]{ '\0' };
	ReadF(Handle, &prMax, 1, error);

	for (short j = 0; j < prMax; j++) {
		WORD n = 0;
		size_t index = 0;
		for (int i = 0; i <= NPrintStrg; i++) {
			ReadF(Handle, &L, 1, error);
			if (L == 255) {
				goto label1;
			}
			A[index++] = L;
			ReadF(Handle, &A[index], L, error);
			index += L;
			n += L + 1;
		}
		ReadF(Handle, &L, 1, error);
		if (L != 255) {
		label1:
			printf("Invalid FAND.CFG");
			system("pause");
			exit(-1);
		}
		printer[j].Strg = std::string((char*)A, n);
		ReadF(Handle, &printer[j].Typ, 1, error);
		ReadF(Handle, &printer[j].Kod, 1, error);
		ReadF(Handle, &printer[j].Lpti, 1, error);
		ReadF(Handle, &printer[j].TmOut, 1, error);
		printer[j].OpCls = false;
		printer[j].ToHandle = false;
		printer[j].ToMgr = false;
		switch (printer[j].TmOut) {
		case 255: {
			printer[j].OpCls = true;
			printer[j].TmOut = 0;
			break;

		}
		case 254: {
			printer[j].ToHandle = true;
			printer[j].TmOut = 0;
			break;
		}
		case 253: {
			printer[j].ToMgr = true;
			printer[j].TmOut = 0;
			break;
		}
		default: break;
		}
	}

	// jeste jsou tam nejaka data a datum v nasl. konfiguraci je posunuty -> nutno rucne posunout:
	// SeekH(CfgHandle, PosH(CfgHandle) + 4);
}

void CfgFile::RdWDaysTab(wdaystt** wDaysTab, WORD& NWDaysTab, double& WDaysFirst, double& WDaysLast)
{
	DWORD error;
	ReadF(Handle, &NWDaysTab, 2, error);   // je to WORD - pocet zaznamu v kalendari
	ReadF(Handle, &WDaysFirst, 6, error);  // v Pascalu real 6B
	WDaysFirst = Real48ToDouble(&WDaysFirst);
	ReadF(Handle, &WDaysLast, 6, error);  // v Pascalu real 6B
	WDaysLast = Real48ToDouble(&WDaysLast);

	*wDaysTab = new wdaystt[NWDaysTab];
	for (int i = 0; i < NWDaysTab; i++) {
		ReadF(Handle, &(*wDaysTab)[i].Typ, sizeof((*wDaysTab)[i].Typ), error);
		ReadF(Handle, &(*wDaysTab)[i].Nr, sizeof((*wDaysTab)[i].Nr), error);
	}
}

void CfgFile::Close()
{
	DWORD error;
	CloseF(Handle, error);
}


