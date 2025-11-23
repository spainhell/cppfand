#include "keyboard.h"
#include <exception>

const int buff_size = 128;

Keyboard::Keyboard()
{
	_handle = GetStdHandle(STD_INPUT_HANDLE);
	if (_handle == INVALID_HANDLE_VALUE) { throw std::exception("Cannot open console input handle."); }
	_kbdBuf = new _INPUT_RECORD[buff_size];
	_actualIndex = 0;
	_inBuffer = 0;
	DWORD fdwMode = ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
	bool scm = SetConsoleMode(_handle, fdwMode);
	if (!scm) { throw std::exception("Cannot set console input mode."); }
}

Keyboard::~Keyboard()
{
	delete[] _kbdBuf;
}

bool Keyboard::Exists()
{
	KEY_EVENT_RECORD k;
	return Get(k, true);
}

bool Keyboard::Empty()
{
	return !Exists();
}

size_t Keyboard::BufSize()
{
	return buff_size;
}

void Keyboard::ClearBuf()
{
	_read();
	_inBuffer = 0;
	_actualIndex = 0;
}

size_t Keyboard::ActualIndex()
{
	return _actualIndex;
}

size_t Keyboard::FreeSpace()
{
	return buff_size - _inBuffer;
}

bool Keyboard::Get(KEY_EVENT_RECORD& key, bool only_check)
{
	// nejdrive zkontrolujeme primarni buffer
	if (!_priorBuffer.empty()) {
		key = _priorBuffer.front();
		if (!only_check) {
			_priorBuffer.pop_front();
		}
		return true;
	}

	// pokud jsme na konci bufferu, nacteme jej znovu
	if (_inBuffer == 0 || _actualIndex >= _inBuffer) {
		// pokud nic neprislo, vratime false
		_read();
		if (_inBuffer == 0) return false;
	}

	// pokud udalost neni z klavesnice, jdeme na dalsi
	//while (_kbdBuf[_actualIndex].EventType != KEY_EVENT && _actualIndex < _inBuffer) {
	//	_actualIndex++;
	//}
	while (_actualIndex < _inBuffer) {
		bool key_or_mouse = false;

		switch (_kbdBuf[_actualIndex].EventType) {
		case KEY_EVENT:
			key_or_mouse = true;
			break;
		case MOUSE_EVENT:
			key_or_mouse = true;
			break;
		case WINDOW_BUFFER_SIZE_EVENT:
			break;
		case MENU_EVENT:
			break;
		case FOCUS_EVENT:
			break;
		default:;
		}

		if (key_or_mouse) {
			break;
		}
		else {
			_actualIndex++;
		}
	}
	// narazili jsme na udalost z klavesnice, nebo tam zadna takova neni a jsme na konci?
	if (_actualIndex == _inBuffer) {
		return false;
	}

	PINPUT_RECORD event = &_kbdBuf[_actualIndex];
	if (!only_check) {
		_actualIndex++;
	}

	if (event->EventType == MOUSE_EVENT) {
		return false;
	}
	else {
		key = event->Event.KeyEvent;

#if _DEBUG
		auto a = MapVirtualKey(key.wVirtualKeyCode, MAPVK_VK_TO_VSC);
		auto b = MapVirtualKey(key.wVirtualKeyCode, MAPVK_VSC_TO_VK);
		auto c = MapVirtualKey(key.wVirtualKeyCode, MAPVK_VK_TO_CHAR);
		auto d = MapVirtualKey(key.wVirtualKeyCode, MAPVK_VSC_TO_VK_EX);
		auto e = MapVirtualKey(key.wVirtualKeyCode, MAPVK_VK_TO_VSC_EX);
#endif

		return true;
	}
}

void Keyboard::DeleteKeyBuf()
{
	_priorBuffer = std::deque<KEY_EVENT_RECORD>();
}

void Keyboard::SetKeyBuf(std::string input)
{
	DeleteKeyBuf();
	size_t index = 0;
	while (index < input.length())
	{
		KEY_EVENT_RECORD key;
		key.bKeyDown = true;
		key.wRepeatCount = 1;
		key.dwControlKeyState = 0;
		key.wVirtualKeyCode = 0;
		key.wVirtualScanCode = 0;
		unsigned char c = input[index];
		if (c > 0) {
			// jde o bezny znak -> pridame jej do bufferu
			// nebo jde o CTRL+PA (^A) .. CTRL+PZ (^Z)
			key.uChar.AsciiChar = c;
			key.uChar.UnicodeChar = c;
			switch (c) {
			case 13: key.wVirtualKeyCode = VK_RETURN; break;
			case 27: key.wVirtualKeyCode = VK_ESCAPE; break;
			case 9: key.wVirtualKeyCode = VK_TAB; break;
			default: break;
			}
		}
		else {
			c = input[++index];
			key.uChar.AsciiChar = 0;
			key.uChar.UnicodeChar = 0;
			switch (c) {
			case 82: key.wVirtualKeyCode = VK_INSERT; break;
			case 83: key.wVirtualKeyCode = VK_DELETE; break;
			case 71: key.wVirtualKeyCode = VK_HOME; break;
			case 79: key.wVirtualKeyCode = VK_END; break;
			case 73: key.wVirtualKeyCode = VK_PRIOR; break;
			case 81: key.wVirtualKeyCode = VK_NEXT; break;
			case 75: key.wVirtualKeyCode = VK_LEFT; break;
			case 77: key.wVirtualKeyCode = VK_RIGHT; break;
			case 72: key.wVirtualKeyCode = VK_UP; break;
			case 80: key.wVirtualKeyCode = VK_DOWN; break;

			case 131: key.wVirtualKeyCode = key.uChar.AsciiChar = '='; key.uChar.UnicodeChar = '=';  key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 15: key.wVirtualKeyCode = VK_TAB; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 119: key.wVirtualKeyCode = VK_HOME; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 117: key.wVirtualKeyCode = VK_END; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 132: key.wVirtualKeyCode = VK_PRIOR; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 118: key.wVirtualKeyCode = VK_NEXT; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 115: key.wVirtualKeyCode = VK_LEFT; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 116: key.wVirtualKeyCode = VK_RIGHT; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
				
			case 59: key.wVirtualKeyCode = VK_F1; break;
			case 60: key.wVirtualKeyCode = VK_F2; break;
			case 61: key.wVirtualKeyCode = VK_F3; break;
			case 62: key.wVirtualKeyCode = VK_F4; break;
			case 63: key.wVirtualKeyCode = VK_F5; break;
			case 64: key.wVirtualKeyCode = VK_F6; break;
			case 65: key.wVirtualKeyCode = VK_F7; break;
			case 66: key.wVirtualKeyCode = VK_F8; break;
			case 67: key.wVirtualKeyCode = VK_F9; break;
			case 68: key.wVirtualKeyCode = VK_F10; break;
			case 69: key.wVirtualKeyCode = VK_F11; break;
			case 70: key.wVirtualKeyCode = VK_F12; break;
			case 84: key.wVirtualKeyCode = VK_F1; key.dwControlKeyState = SHIFT_PRESSED; break;
			case 85: key.wVirtualKeyCode = VK_F2; key.dwControlKeyState = SHIFT_PRESSED; break;
			case 86: key.wVirtualKeyCode = VK_F3; key.dwControlKeyState = SHIFT_PRESSED; break;
			case 87: key.wVirtualKeyCode = VK_F4; key.dwControlKeyState = SHIFT_PRESSED; break;
			case 88: key.wVirtualKeyCode = VK_F5; key.dwControlKeyState = SHIFT_PRESSED; break;
			case 89: key.wVirtualKeyCode = VK_F6; key.dwControlKeyState = SHIFT_PRESSED; break;
			case 90: key.wVirtualKeyCode = VK_F7; key.dwControlKeyState = SHIFT_PRESSED; break;
			case 91: key.wVirtualKeyCode = VK_F8; key.dwControlKeyState = SHIFT_PRESSED; break;
			case 92: key.wVirtualKeyCode = VK_F9; key.dwControlKeyState = SHIFT_PRESSED; break;
			case 93: key.wVirtualKeyCode = VK_F10; key.dwControlKeyState = SHIFT_PRESSED; break;
			case 94: key.wVirtualKeyCode = VK_F1; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 95: key.wVirtualKeyCode = VK_F2; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 96: key.wVirtualKeyCode = VK_F3; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 97: key.wVirtualKeyCode = VK_F4; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 98: key.wVirtualKeyCode = VK_F5; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 99: key.wVirtualKeyCode = VK_F6; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 100: key.wVirtualKeyCode = VK_F7; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 101: key.wVirtualKeyCode = VK_F8; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 102: key.wVirtualKeyCode = VK_F9; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 103: key.wVirtualKeyCode = VK_F10; key.dwControlKeyState = LEFT_CTRL_PRESSED; break;
			case 104: key.wVirtualKeyCode = VK_F1; key.dwControlKeyState = LEFT_ALT_PRESSED; break;
			case 105: key.wVirtualKeyCode = VK_F2; key.dwControlKeyState = LEFT_ALT_PRESSED; break;
			case 106: key.wVirtualKeyCode = VK_F3; key.dwControlKeyState = LEFT_ALT_PRESSED; break;
			case 107: key.wVirtualKeyCode = VK_F4; key.dwControlKeyState = LEFT_ALT_PRESSED; break;
			case 108: key.wVirtualKeyCode = VK_F5; key.dwControlKeyState = LEFT_ALT_PRESSED; break;
			case 109: key.wVirtualKeyCode = VK_F6; key.dwControlKeyState = LEFT_ALT_PRESSED; break;
			case 110: key.wVirtualKeyCode = VK_F7; key.dwControlKeyState = LEFT_ALT_PRESSED; break;
			case 111: key.wVirtualKeyCode = VK_F8; key.dwControlKeyState = LEFT_ALT_PRESSED; break;
			case 112: key.wVirtualKeyCode = VK_F9; key.dwControlKeyState = LEFT_ALT_PRESSED; break;
			case 113: key.wVirtualKeyCode = VK_F10; key.dwControlKeyState = LEFT_ALT_PRESSED; break;
			default: break;
			}
		}
		_priorBuffer.push_back(key);
		index++;
	}
}

void Keyboard::AddToKeyBuf(std::string input)
{
	
}

void Keyboard::AddToKeyBuf(unsigned short c)
{
	KEY_EVENT_RECORD key = KEY_EVENT_RECORD();

	// reverse function to KeyCombination
	if (c & 0x0400) { key.dwControlKeyState += 0x0002; } // left Alt
	if (c & 0x0200) { key.dwControlKeyState += 0x0008; } // left Ctrl
	if (c & 0x0100) { key.dwControlKeyState += 0x0010; } // shift

	if (c & 0x8000) {
		// non printable character
		key.wVirtualKeyCode = c & 0xFF;
		key.uChar.AsciiChar = 0;
		key.uChar.UnicodeChar = 0;
	}
	else {
		key.wVirtualKeyCode = c & 0xFF;
		key.uChar.AsciiChar = c & 0xFF;
		key.uChar.UnicodeChar = c & 0xFF;
	}
	
	_priorBuffer.push_back(key);
}

void Keyboard::AddToFrontKeyBuf(std::string input)
{
	
}

void Keyboard::AddToFrontKeyBuf(unsigned short c)
{
	KEY_EVENT_RECORD key = KEY_EVENT_RECORD();
	// set key down
	key.bKeyDown = true;

	// reverse function to KeyCombination
	if (c & 0x0400) { key.dwControlKeyState += 0x0002; } // left Alt
	if (c & 0x0200) { key.dwControlKeyState += 0x0008; } // left Ctrl
	if (c & 0x0100) { key.dwControlKeyState += 0x0010; } // shift

	if (c & 0x8000) {
		// non printable character
		key.wVirtualKeyCode = c & 0xFF;
		key.uChar.AsciiChar = 0;
		key.uChar.UnicodeChar = 0;
	}
	else {
		key.wVirtualKeyCode = c & 0xFF;
		key.uChar.AsciiChar = c & 0xFF;
		key.uChar.UnicodeChar = c & 0xFF;
	}

	_priorBuffer.push_front(key);
}

short Keyboard::GetState(int nVirtKey)
{
	return GetKeyState(nVirtKey);
}

std::vector<KEY_EVENT_RECORD> Keyboard::GetKeyBuf()
{
	return std::vector<KEY_EVENT_RECORD>();
}

std::string Keyboard::GetKeyBufAsString()
{
	return std::string();
}

void Keyboard::_read()
{
	//DWORD waitResult = WaitForSingleObject(_handle, 50/*INFINITE*/);
	//if (waitResult == WAIT_OBJECT_0) {
	//	ReadConsoleInput(_handle, _kbdBuf, buff_size, &_inBuffer);
	//	_actualIndex = 0;
	//}
	
	DWORD events_count;
	GetNumberOfConsoleInputEvents(_handle, &events_count);
	if (events_count > 0) {
		ReadConsoleInput(_handle, _kbdBuf, buff_size, &_inBuffer);
		_actualIndex = 0;
	}
	else {
		_inBuffer = 0;
	}
}

PressedKey::PressedKey(KEY_EVENT_RECORD& key)
{
	_key = key;
	if (_key.wVirtualKeyCode == VK_DECIMAL) {
		_key.uChar.AsciiChar = '.';
		_key.uChar.UnicodeChar = '.';
	}
	// transformed character (for use with Alt, Ctrl or Shift)
	Char = MapVirtualKey(key.wVirtualKeyCode, MAPVK_VK_TO_CHAR);
}

KEY_EVENT_RECORD* PressedKey::Key()
{
	return &_key;
}

//char PressedKey::Char()
//{
//	return _key.uChar.AsciiChar;
//}

uint32_t PressedKey::KeyDescr()
{
	// 1. a 2. B - ControlKeyState  https://docs.microsoft.com/en-us/windows/console/key-event-record-str
	// 3. B      - Virtual Key Code https://docs.microsoft.com/cs-cz/windows/win32/inputdev/virtual-key-codes
	// 4. B      - znak CHAR
	return (_key.dwControlKeyState << 16) + ((_key.wVirtualKeyCode & 0xFF) << 8) + Char;
}

uint32_t PressedKey::SimpleKeyDescr()
{
	// zjednodusena varianta bez rozliseni leve a prave strany
	// 3. B (0x00000ACS) - SHIFT, CONTROL, ALT
	// 2. B Virtual Key Code
	// 1. B znak CHAR
	uint8_t ControlKey = 0;
	if (Alt()) ControlKey += 4;
	if (Ctrl()) ControlKey += 2;
	if (Shift()) ControlKey += 1;
	return (ControlKey << 16) + ((_key.wVirtualKeyCode & 0xFF) << 8) + Char;
}

uint16_t PressedKey::KeyCombination()
{
	// primitive variant without detection of left or right side
	// 2. B (0xN0000ACS) - NonChar, SHIFT, CONTROL, ALT
	// 1. B NonChar: Virtual Key Code, otherwise printable char
	uint16_t result;
	uint8_t ControlKey = 0;
	if (Alt()) ControlKey += 4;
	if (Ctrl()) ControlKey += 2;
	if (Shift()) ControlKey += 1;
	
	if (Char == 0) {
		// non printable character
		ControlKey += 0x80;
		result = static_cast<uint16_t>((ControlKey << 8) + (_key.wVirtualKeyCode & 0xFF));
	}
	else if (ControlKey == 2 && Char != _key.uChar.AsciiChar) {
		// CTRL + A .. CTRL + N
		ControlKey += 0x80;
		result = static_cast<uint16_t>((ControlKey << 8) + Char);
	}
	else {
		// printable character
		if (ControlKey == 1 && (Char < 1 || Char > 31)) {
			// capital char (with Shift)
			ControlKey = 0;
		}
		else if (ControlKey == 6 && Char != _key.uChar.AsciiChar) {
			// Ctrl + Alt + V (@), ...
			ControlKey = 0;
		}
		result = (ControlKey << 8) + (uint8_t)(_key.uChar.AsciiChar);
	}
	return result;
}

void PressedKey::UpdateKey(WORD newKey)
{
	// reverse function to KeyCombination
	_key.dwControlKeyState = 0;
	if (newKey & 0x0400) { _key.dwControlKeyState += 0x0002; } // left Alt
	if (newKey & 0x0200) { _key.dwControlKeyState += 0x0008; } // left Ctrl
	if (newKey & 0x0100) { _key.dwControlKeyState += 0x0010; } // shift

	if (newKey & 0x8000) {
		// non printable character
		_key.wVirtualKeyCode = newKey & 0xFF;
		_key.uChar.AsciiChar = 0;
		_key.uChar.UnicodeChar = 0;
		Char = 0;
	}
	else {
		_key.wVirtualKeyCode = newKey & 0xFF;
		_key.uChar.AsciiChar = newKey & 0xFF;
		Char = newKey & 0xFF;
	}
}

//unsigned __int32 PressedKey::Function()
//{
//	unsigned __int32 result = _key.wVirtualKeyCode;
//	if (Shift()) result += 0x00010000;
//	if (Alt()) result += 0x00040000;
//	if (Ctrl()) result += 0x00020000;
//	return result;
//}

/// jedna se o tisknutelny znak?
bool PressedKey::isChar()
{
	WORD comb = this->KeyCombination();
	return comb >= 0x20 && comb <= 0xFF;

	/*
	uint8_t c = (uint8_t)_key.uChar.AsciiChar;
	// muze byt se SHIFT, to znaci velke pismeno ...
	// muze byt s CTRL a ALT zaroven, to znaci znaky '@' atp.
	bool alt = Alt();
	bool ctrl = Ctrl();
	bool combi = (!alt && !ctrl) || (alt && ctrl);
	return c >= 0x20 && combi;
	*/
}

bool PressedKey::Shift()
{
	return _key.dwControlKeyState & 0x0010;
}

bool PressedKey::Alt()
{
	return LeftAlt() || RightAlt();
}

bool PressedKey::Ctrl()
{
	return LeftCtrl() || RightCtrl();
}

bool PressedKey::Enhanced()
{
	return _key.dwControlKeyState & 0x0100;
}

bool PressedKey::CapsLock()
{
	return false;
}

bool PressedKey::ScrollLock()
{
	return _key.dwControlKeyState & 0x0080;
}

bool PressedKey::NumLock()
{
	return _key.dwControlKeyState & 0x0020;
}

bool PressedKey::LeftAlt()
{
	return _key.dwControlKeyState & 0x0002;
}

bool PressedKey::RightAlt()
{
	return _key.dwControlKeyState & 0x0001;
}

bool PressedKey::LeftCtrl()
{
	return _key.dwControlKeyState & 0x0008;
}

bool PressedKey::RightCtrl()
{
	return _key.dwControlKeyState & 0x0004;
}
