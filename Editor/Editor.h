#pragma once
#include "../Drivers/screen.h"
#include "../Drivers/keyboard.h"

class Editor
{
public:
	Editor();
	~Editor();

private:
	Screen* screen_;
	Wind windMin_;
	Wind windMax_;
	TCrs cursor_;
	Keyboard keyboard_;
};

