#pragma once
#include <string>
#include "Cfg.h"
#include "../Drivers/screen.h"


class CfgFile
{
public:
	HANDLE Handle = nullptr;
	std::string FullName;

	void Open(std::string path);
	void ReadSpec(Spec& spec);
	void ReadVideoAndColors(Video& video, uint8_t start_mode, enVideoCard video_card, Screen& screen, WORD& TxtCols, WORD& TxtRows);
	void ReadFonts(Fonts& fonts);
	void ReadCodeTables();
	void RdPrinter(short& prMax, Printer printer[10]);
	void RdWDaysTab(wdaystt** wDaysTab, WORD& NWDaysTab, double& WDaysFirst, double& WDaysLast);
	void Close();
};
