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
	// pokud jsme na konci bufferu, naèteme jej znovu
	if (_inBuffer == 0 || _actualIndex >= _inBuffer)
	{
		// pokud nic nepøišlo, vrátíme false
		_read();
		if (_inBuffer == 0) return false;
	}
	// pokud událost není z klávesnice, jdeme na další
	while (_kbdBuf[_actualIndex].EventType != KEY_EVENT && _actualIndex < _inBuffer)
	{
		_actualIndex++;
	}

	// narazili jsme na událost z klávesnice, nebo tam žádná taková není a jsme na konci?
	if (_actualIndex == _inBuffer) return false;
		
	key = _kbdBuf[_actualIndex++].Event.KeyEvent;
	return true;
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
	// 3. B (0x00000SCA) - SHIFT, CONTROL, ALT
	// 2. B Virtual Key Code
	// 1. B znak CHAR
	unsigned char ControlKey = 0;
	if (Alt()) ControlKey += 1;
	if (Ctrl()) ControlKey += 2;
	if (Shift()) ControlKey += 4;
	return (ControlKey << 16) + ((_key.wVirtualKeyCode & 0xFF) << 8) + _key.uChar.AsciiChar;
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
