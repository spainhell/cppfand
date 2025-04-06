#pragma once
#include "../Drivers/keyboard.h"

class TextEditorEvents;

enum class TextEditorSpecialMode {
	normal,
	CtrlK, CtrlO, CtrlP, CtrlQ,
	SingleFrame, DoubleFrame, DeleteFrame, NoFrame
};

/// <summary>
/// Modes for special keys combinations.
/// </summary>
class TextEditorSpecialModes
{
public:
	TextEditorSpecialModes(TextEditorEvents* events);
	TextEditorSpecialMode GetMode();
	TextEditorSpecialMode HandleKeyPress(PressedKey& key);
	TextEditorSpecialMode HandleMouse();

private:
	TextEditorEvents* _events = nullptr;
	TextEditorSpecialMode _actual_mode = TextEditorSpecialMode::normal;

	void process_Ctrl_K(const PressedKey& key);
	void process_Ctrl_O(const PressedKey& key);
	void process_Ctrl_P(PressedKey& key);
	void process_Ctrl_Q(const PressedKey& key);
	void process_Frame(const PressedKey& key);
};
