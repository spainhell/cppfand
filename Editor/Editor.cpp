#include "Editor.h"
#include "../textfunc/textfunc.h"

Editor::Editor(std::string& text)
{
	rawText_ = &text;
	vLines = GetAllRows(text);
	windMin_ = Wind{ 1, 1 };
	windMax_ = Wind{ 80, 25 };
	screen_ = new Screen(80, 25, &windMin_, &windMax_, &cursor_);
	//keyboard_ = Keyboard();
	editorLines_ = windMax_.Y - windMin_.Y - headLines_ - footLines_ + 1;
	editorColumns_ = windMax_.X - windMin_.X + 1;
	displayAllRows();
}

Editor::~Editor()
{
	delete screen_;
}

void Editor::Run()
{
	bool End = false;
	KEY_EVENT_RECORD keyBasic;
	while (!End) {
		bool pressed = keyboard_.Get(keyBasic);
		if (!pressed) {
			Sleep(100);
			continue;
		}
		if (!keyBasic.bKeyDown) continue;
		PressedKey key = PressedKey(keyBasic);

		if (key.isChar()) {

		}
		else {
			switch (key.Function())
			{
			case VK_ESCAPE: { End = true; break; }
			case VK_LEFT: { MoveLeft(); break; }
			case VK_RIGHT: { MoveRight(); break; }
			case VK_UP: { MoveUp(); break; }
			case VK_DOWN: { MoveDown(); break; }
			case VK_HOME + CTRL: { MoveHome(); break; }
			case VK_END + CTRL: { MoveEnd(); break; }
			}
		}

	}
}

void Editor::displayAllRows()
{
	size_t textLines = vLines.size();
	//size_t emptyLines = editorLines_ - textLines;
	std::string line; // 1 line to print

	for (size_t i = 0; i < min(editorLines_, textLines); i++)
	{
		if (shiftX <= 0) { line = vLines[i + shiftY].substr(0, editorColumns_); }
		else {
			if (vLines[i + shiftY].length() > shiftX) {
				line = vLines[i + shiftY].substr(shiftX, editorColumns_);
			}
			else
			{
				line = ' ';
			}
		}
		line = AddTrailChars(line, ' ', editorColumns_); // doplnime mezery vpravo na sirku obrazovky
		screen_->ScrFormatWrText(1, 1 + headLines_ + i, line.c_str());
	}
	screen_->GotoXY(1, 1 + headLines_, relative);
}

void Editor::MoveLeft()
{
	if (shiftX == 0) {
		printf("%c", '\a');
		return;
	}
	shiftX--;
	displayAllRows();
}

void Editor::MoveRight()
{
	shiftX++;
	displayAllRows();
}

void Editor::MoveUp()
{
	if (shiftY == 0) {
		printf("%c", '\a');
		return;
	}
	shiftY--;
	displayAllRows();
}

void Editor::MoveDown()
{
	shiftY++;
	displayAllRows();
}

void Editor::MoveHome()
{
	shiftX = 0;
	shiftY = 0;
	displayAllRows();
}

void Editor::MoveEnd()
{
	shiftX = 0;
	shiftY = 0;
	displayAllRows();
}