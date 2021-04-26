#pragma once
#include "../Drivers/screen.h"
#include "../Drivers/keyboard.h"

class Editor
{
public:
	Editor(std::string& text);
	~Editor();
	void Run();

private:
	Screen* screen_;
	Wind windMin_;
	Wind windMax_;
	TCrs cursor_;
	Keyboard keyboard_;
	std::string* rawText_;
	std::vector<std::string> vLines;
	
	short headLines_ = 1; // HEAD lines
	short footLines_ = 1; // FOOT lines
	short editorLines_ = 0; // EDITOR lines
	short editorColumns_ = 0; // EDITOR rows
	short shiftX = 0; // shift to right
	short shiftY = 0; // shift down

	void displayAllRows();

	void MoveLeft();
	void MoveRight();
	void MoveUp();
	void MoveDown();
	void MoveHome();
	void MoveEnd();
};

