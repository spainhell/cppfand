#pragma once

#include "TextEditorModes.h"
#include "../Core/rdrun.h"
#include <string>

#include "TextEditor.h"

class TextEditor;

enum class TextEditorType { View, Text, Help };

class TextEditorEvents
{
public:
	friend class TextEditorModes;
	TextEditorEvents();
	~TextEditorEvents();
	void HandleEvent(TextEditor* editor, char& Mode, bool& IsWrScreen, BYTE SysLColor, std::string& LastS, WORD LastNr, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
	bool CtrlShiftAlt(TextEditor* editor, char mode, std::string& LastS, WORD LastNr, bool IsWrScreen);
	bool My2GetEvent();

private:
	TextEditorModes* _modes_handler = nullptr; // Ctrl-O, Ctrl-P, Ctrl-Q, ...
	TextEditorType _editor_type = TextEditorType::View;

	bool HelpEvent(std::vector<WORD>& breakKeys);
	void Wr(std::string s, std::string& OrigS, char Mode, BYTE SysLColor);
	bool ScrollEvent(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
	bool ViewEvent(std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
	bool MyGetEvent(TextEditor* editor, char& Mode, BYTE SysLColor, std::string& LastS, WORD LastNr, bool IsWrScreen, bool bScroll, std::vector<EdExitD*>& ExitD, std::vector<WORD>& breakKeys);
	bool TestExitKeys(TextEditor* editor, char& mode, std::vector<EdExitD*>& ExitD, int& fs,
	                  LongStr*& sp, WORD key);
};

