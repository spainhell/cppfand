#include "TextEditorModes.h"
#include "../Drivers/constants.h"
#include "TextEditorEvents.h"

TextEditorModes::TextEditorModes(TextEditorEvents* events)
{
	_events = events;
}

TextEditorMode TextEditorModes::GetMode()
{
	return _actual_mode;
}

TextEditorMode TextEditorModes::HandleKeyPress(PressedKey& key)
{
	switch (_actual_mode)
	{
	case TextEditorMode::CtrlK: {
		process_Ctrl_K(key);
		break;
	}
	case TextEditorMode::CtrlO: {
		process_Ctrl_O(key);
		break;
	}
	case TextEditorMode::CtrlP: {
		process_Ctrl_P(key);
		break;
	}
	case TextEditorMode::CtrlQ: {
		process_Ctrl_Q(key);
		break;
	}
	case TextEditorMode::SingleFrame:
	case TextEditorMode::DoubleFrame:
	case TextEditorMode::DeleteFrame:
	case TextEditorMode::NoFrame: {
		process_Frame(key);
		break;
	}
	default:
	{
		switch (key.KeyCombination()) {
		case __CTRL_K: {
			_actual_mode = TextEditorMode::CtrlK;
			break;
		}
		case __CTRL_O: {
			_actual_mode = TextEditorMode::CtrlO;
			break;
		}
		case __CTRL_P: {
			_actual_mode = TextEditorMode::CtrlP;
			break;
		}
		case __CTRL_Q: {
			_actual_mode = TextEditorMode::CtrlQ;
			break;
		}
		default: {}
		}
	}
	}

	return _actual_mode;
}

TextEditorMode TextEditorModes::HandleMouse()
{
	// not implemented yet
	return _actual_mode;
}

void TextEditorModes::process_Ctrl_K(const PressedKey& key)
{
	std::set<char> setKc = { 'B', 'K', 'H', 'S', 'Y', 'C', 'V', 'W', 'R', 'P', 'F', 'U', 'L', 'N' };
	if (setKc.count((char)Event.Pressed.KeyCombination()) > 0) {
		//Event.KeyCode = (ww << 8) | Event.KeyCode;
	}
	else {
		//Event.KeyCode = 0;
	}
	_actual_mode = TextEditorMode::normal;
}

void TextEditorModes::process_Ctrl_O(const PressedKey& key)
{
	switch (Event.Pressed.Char) {
	case 'W': // wrap
	case 'R':
	case 'L':
	case 'J':
	case 'C': {
		//Event.KeyCode = (ww << 8) | Event.KeyCode;
		break;
	}
	default: {
		//Event.KeyCode = 0;
	}
	}
	_actual_mode = TextEditorMode::normal;
}

void TextEditorModes::process_Ctrl_P(PressedKey& key)
{
	if (key.Char == 0) {
		// no key pressed (only Shift, Alt, Ctrl, ...)
	}
	else {
		char upper = toupper(key.Char);
		if (upper >= 'A' && upper <= 'Z') {
			if ((upper == 'Y' || upper == 'Z') && (spec.KbdTyp == CsKbd || spec.KbdTyp == SlKbd)) {
				switch (upper) {
				case 'Z': upper = 'Y'; break;
				case 'Y': upper = 'Z'; break;
				default: break;
				}
			}
			key.UpdateKey(CTRL + upper - '@');
		}
		else {
			// to be compatible with PC FAND 4.2
			// TODO: color of a character isn't right
			key.UpdateKey(key.Char + '@');
		}
		_actual_mode = TextEditorMode::normal;
	}
}

void TextEditorModes::process_Ctrl_Q(const PressedKey& key)
{
	switch (Event.Pressed.KeyCombination()) {
	//case 'S': Event.Pressed.Key()->wVirtualKeyCode = __HOME; break;
	//case 'D': Event.Pressed.Key()->wVirtualKeyCode = __END; break;
	//case 'rdb': Event.Pressed.Key()->wVirtualKeyCode = __CTRL_PAGEUP; break;
	//case 'C': Event.Pressed.Key()->wVirtualKeyCode = __CTRL_PAGEDOWN; break;

	//case 'E': case 'X': case 'Y':
	//case 'L': case 'B': case 'K':
	//case 'I': case 'F': case 'A': {
	//	break;
	//}

	case '-': {
		_actual_mode = TextEditorMode::SingleFrame;
		screen.CrsBig();
		FrameDir = 0;
		// result = true;
		// ClrEvent();
		break;
	}
	case '=': {
		_actual_mode = TextEditorMode::DoubleFrame;
		screen.CrsBig();
		FrameDir = 0;
		// result = true;
		// ClrEvent();
		break;
	}
	case '/': {
		_actual_mode = TextEditorMode::DeleteFrame;
		screen.CrsBig();
		FrameDir = 0;
		// result = true;
		// ClrEvent();
		break;
	}
	default: {
		_actual_mode = TextEditorMode::normal;
		// ClrEvent();
	}
	}
}

void TextEditorModes::process_Frame(const PressedKey& key)
{
	if (key.Char == __ESC) {
		_actual_mode = TextEditorMode::normal;
	}
}
