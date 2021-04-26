#include "Editor.h"

Editor::Editor()
{
	windMin_ = Wind{ 1, 1 };
	windMax_ = Wind{ 25, 80 };
	screen_ = new Screen(80, 25, &windMin_, &windMax_, &cursor_);
	keyboard_ = Keyboard();
}

Editor::~Editor()
{
	delete screen_;
}
