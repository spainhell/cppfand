#pragma once
#include <string>
#include "OldEditor.h"

typedef std::string ColorOrd;

class EditorScreen
{
public:
	EditorScreen(size_t TextColumns, Blocks* blocks, std::string ctrlKey);
	~EditorScreen();
	void WriteEditLine(std::string& text_line, size_t row);
	void WriteScrollLine(std::string& text_line, size_t offset, size_t row);
	void EditWrline(char* input_text, size_t text_len, int Row, BYTE ColKey[], BYTE TxtColor, BYTE BlockColor);
	void ScrollWrline(char* P, size_t offsetX, int Row, ColorOrd& CO, BYTE ColKey[], BYTE TxtColor, bool& InsPage);
	BYTE Color(char c, BYTE ColKey[]);
	BYTE Color(ColorOrd CO, BYTE ColKey[], BYTE TxtColor);

private:
	size_t _textColumns;
	Blocks* _blocks;
	std::string _ctrlKey;
};

