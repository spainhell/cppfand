#pragma once
#include <string>
#include "TextEditor.h"

class TextEditorScreen
{
public:
	TextEditorScreen(TextEditor* editor, size_t TextColumns, Blocks* blocks, std::string ctrlKey);
	~TextEditorScreen();
	void EditWrline(const std::string& text_line, int Row, BYTE ColKey[], BYTE TxtColor, BYTE BlockColor);
	void ScrollWrline(char* P, size_t offsetX, int Row, ColorOrd& CO, BYTE ColKey[], BYTE TxtColor, bool& InsPage);
	BYTE Color(char c, BYTE ColKey[]);
	BYTE Color(ColorOrd CO, BYTE ColKey[], BYTE TxtColor);

private:
	TextEditor* _editor;
	size_t _textColumns;
	Blocks* _blocks;
	std::string _ctrlKey;
};

