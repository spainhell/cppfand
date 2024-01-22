#pragma once
#include "../Drivers/keyboard.h"

class TextEditorEvents;

enum class TextEditorMode {
	normal,
	CtrlK, CtrlO, CtrlP, CtrlQ,
	SingleFrame, DoubleFrame, DeleteFrame, NoFrame
};

class TextEditorModes
{
public:
	TextEditorModes(TextEditorEvents* events);
	TextEditorMode GetMode();
	TextEditorMode HandleKeyPress(PressedKey key);
	TextEditorMode HandleMouse();

private:
	TextEditorEvents* _events = nullptr;
	TextEditorMode _actual_mode = TextEditorMode::normal;

	void process_Ctrl_K(const PressedKey& key);
	void process_Ctrl_O(const PressedKey& key);
	void process_Ctrl_P(PressedKey& key);
	void process_Ctrl_Q(const PressedKey& key);
	void process_Frame(const PressedKey& key);
};

