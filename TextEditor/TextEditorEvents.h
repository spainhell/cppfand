#pragma once

#include "../Core/rdrun.h"
#include <string>

class TextEditor;

class TextEditorEvents
{
public:
	void HandleEvent(TextEditor* editor, char& Mode, bool& IsWrScreen, BYTE SysLColor, std::string& LastS, WORD LastNr, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
	void CtrlShiftAlt(TextEditor* editor, char mode, std::string& LastS, WORD LastNr, bool IsWrScreen);
	bool My2GetEvent();

private:
	bool HelpEvent(std::vector<WORD>& breakKeys);
	void Wr(std::string s, std::string& OrigS, char Mode, BYTE SysLColor);
	bool ScrollEvent(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
	bool ViewEvent(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
	bool MyGetEvent(TextEditor* editor, char& Mode, BYTE SysLColor, std::string& LastS, WORD LastNr, bool IsWrScreen, bool bScroll, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
};

