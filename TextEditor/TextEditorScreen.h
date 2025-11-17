#pragma once
#include <string>
#include "TextEditor.h"

class TextEditorScreen
{
public:
	TextEditorScreen(TextEditor* editor, size_t TextColumns, std::set<char> ctrlKey);
	~TextEditorScreen();
	void EditWrline(const std::string& text_line, int Row, uint8_t ColKey[], uint8_t TxtColor, uint8_t BlockColor);
	void ScrollWrline(char* P, size_t offsetX, int Row, ColorOrd& CO, uint8_t ColKey[], uint8_t TxtColor, bool& InsPage);
	uint8_t Color(char c, uint8_t ColKey[]);
	uint8_t Color(ColorOrd CO, uint8_t ColKey[], uint8_t TxtColor);

private:
	TextEditor* _editor;
	size_t _textColumns;
	std::set<char>  _ctrlKey;
};

