#pragma once
#include "../cppfand/constants.h"
#include "../cppfand/rdrun.h"
#include <string>

void HandleEvent(char Mode, bool& IsWrScreen, BYTE SysLColor, std::string& LastS, WORD LastNr);
bool My2GetEvent();
bool MyGetEvent(char Mode, BYTE SysLColor, std::string& LastS, WORD LastNr, bool IsWrScreen, bool bScroll, EdExitD* ExitD);