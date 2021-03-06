#include "keyboard.h"
#include <exception>

Keyboard::Keyboard()
{
	_handle = GetStdHandle(STD_INPUT_HANDLE);
	if (_handle == INVALID_HANDLE_VALUE) { throw std::exception("Cannot open console input handle."); }
	_kbdBuf = new _INPUT_RECORD[128];
	_actualIndex = 0;
	_inBuffer = 0;
	DWORD fdwMode = ENABLE_WINDOW_INPUT; // | ENABLE_MOUSE_INPUT;
	bool scm = SetConsoleMode(_handle, fdwMode);
	if (!scm) { throw std::exception("Cannot set console input mode."); }
}

Keyboard::~Keyboard()
{
	delete[] _kbdBuf;
}

bool Keyboard::Exists()
{
	return !Empty();
	
}

bool Keyboard::Empty()
{
	if (_inBuffer > 0) return true;
	_read();
	return _inBuffer > 0;
}

size_t Keyboard::BufSize()
{
	return 128;
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
	return 128 - _inBuffer;
}

bool Keyboard::Get(KEY_EVENT_RECORD& key)
{
	// nejdrive zkontrolujeme primarni buffer
	if (!_priorBuffer.empty())
	{
		key = _priorBuffer.front();
		_priorBuffer.pop();
		return true;
	}
	// pokud jsme na konci bufferu, nacteme jej znovu
	if (_inBuffer == 0 || _actualIndex >= _inBuffer)
	{
		// pokud nic neprislo, vratime false
		_read();
		if (_inBuffer == 0) return false;
	}
	// pokud udalost neni z klavesnice, jdeme na dalsi
	while (_kbdBuf[_actualIndex].EventType != KEY_EVENT && _actualIndex < _inBuffer)
	{
		_actualIndex++;
	}

	// narazili jsme na udalost z klavesnice, nebo tam zadna takova neni a jsme na konci?
	if (_actualIndex == _inBuffer) return false;
		
	key = _kbdBuf[_actualIndex++].Event.KeyEvent;
	return true;
}

void Keyboard::DeleteKeyBuf()
{
	_priorBuffer = std::queue<KEY_EVENT_RECORD>();
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
		_priorBuffer.push(key);
		index++;
	}
}

void Keyboard::AddToKeyBuf(std::string input)
{
	
}

void Keyboard::AddToKeyBuf(char c)
{
	
}

void Keyboard::AddToFrontKeyBuf(std::string input)
{
	
}

void Keyboard::AddToFrontKeyBuf(char c)
{
	
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
	ReadConsoleInput(_handle, _kbdBuf, 128, &_inBuffer);
	_actualIndex = 0;
}

PressedKey::PressedKey(KEY_EVENT_RECORD& key)
{
	_key = key;
	if (_key.wVirtualKeyCode == VK_DECIMAL) {
		_key.uChar.AsciiChar = '.';
		_key.uChar.UnicodeChar = '.';
	}
}

char PressedKey::Char()
{
	return _key.uChar.AsciiChar;
}

unsigned __int32 PressedKey::KeyDescr()
{
	// 1. a 2. B - ControlKeyState  https://docs.microsoft.com/en-us/windows/console/key-event-record-str
	// 3. B      - Virtual Key Code https://docs.microsoft.com/cs-cz/windows/win32/inputdev/virtual-key-codes?redirectedfrom=MSDN
	// 4. B      - znak CHAR
	return (_key.dwControlKeyState << 16) + ((_key.wVirtualKeyCode & 0xFF) << 8) + _key.uChar.AsciiChar;
}

unsigned __int32 PressedKey::SimpleKeyDescr()
{
	// zjednodusena varianta bez rozliseni leve a prave strany
	// 3. B (0x00000ACS) - SHIFT, CONTROL, ALT
	// 2. B Virtual Key Code
	// 1. B znak CHAR
	unsigned char ControlKey = 0;
	if (Alt()) ControlKey += 4;
	if (Ctrl()) ControlKey += 2;
	if (Shift()) ControlKey += 1;
	return (ControlKey << 16) + ((_key.wVirtualKeyCode & 0xFF) << 8) + _key.uChar.AsciiChar;
}

unsigned __int32 PressedKey::Function()
{
	unsigned __int32 result = _key.wVirtualKeyCode;
	if (Shift()) result += SHIFT;
	if (Alt()) result += ALT;
	if (Ctrl()) result += CTRL;
	return result;
}

/// jedna se o tisknutelny znak?
bool PressedKey::isChar()
{
	BYTE c = (BYTE)Char();
	// muze byt se SHIFT, to znaci velke pismeno ...
	return c >= 0x20 && c <= 0xFE && !Alt() && !Ctrl();
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
