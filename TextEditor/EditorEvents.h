#pragma once

#include "../Core/rdrun.h"
#include <string>

class TextEditor;
void HandleEvent(TextEditor* editor, char& Mode, bool& IsWrScreen, BYTE SysLColor, std::string& LastS, WORD LastNr, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
bool My2GetEvent();
bool MyGetEvent(char& Mode, BYTE SysLColor, std::string& LastS, WORD LastNr, bool IsWrScreen, bool bScroll, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
