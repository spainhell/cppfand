#include "globconf.h"


globconf* globconf::GetInstance()
{
	if (_instance != nullptr) return _instance;
	_instance = new globconf();
	return _instance;
}

globconf* globconf::_instance = nullptr;

std::vector<std::string> globconf::paramstr;
pstring globconf::OldDir;
pstring globconf::FandDir;
pstring globconf::WrkDir;
pstring globconf::FandOvrName;
pstring globconf::FandResName;
pstring globconf::FandWorkName;
pstring globconf::FandWorkXName;
pstring globconf::FandWorkTName;
pstring globconf::CPath;
pstring globconf::CDir;
pstring globconf::CName;
pstring globconf::CExt;
pstring globconf::CVol;

TResFile globconf::ResFile;
TMsgIdxItem* globconf::MsgIdx;// = TMsgIdx;

WORD globconf::HandleError;

WORD globconf::MsgIdxN;
longint globconf::FrstMsgPos;

char globconf::AbbrYes = 'Y';
char globconf::AbbrNo = 'N';

WORD globconf::F10SpecKey;
BYTE globconf::ProcAttr;
pstring globconf::MsgLine;
pstring globconf::MsgPar[4];

wdaystt globconf::WDaysTabType;
WORD globconf::NWDaysTab;
float globconf::WDaysFirst;
float globconf::WDaysLast;
wdaystt* globconf::WDaysTab;