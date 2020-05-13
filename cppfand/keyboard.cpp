#include "keyboard.h"
#include <exception>

Keyboard::Keyboard()
{
	_handle = GetStdHandle(STD_INPUT_HANDLE);
	if (_handle == INVALID_HANDLE_VALUE) { throw std::exception("Cannot open console input handle."); }
	_kbdBuf = new _INPUT_RECORD[128];
	_actualIndex = 0;
	_lastIndex = 0;
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
	return _actualIndex == _lastIndex;
}

size_t Keyboard::BufSize()
{
	return 128;
}

void Keyboard::ClearBuf()
{
	_actualIndex = 0;
	_lastIndex = 0;
}

size_t Keyboard::ActualIndex()
{
	return _actualIndex;
}

size_t Keyboard::FreeSpace()
{
	return 128 - _lastIndex - 1;
}

bool Keyboard::Get(KEY_EVENT_RECORD& key)
{
	// pokud jsme na konci bufferu, naèteme jej znovu
	if (_actualIndex == _lastIndex)
	{
		// pokud nic nepøišlo, vrátíme false
		DWORD count = _read();
		if (count == 0) return false;
	}
	// pokud událost není z klávesnice, jdeme na další
	while (_kbdBuf[_actualIndex].EventType != KEY_EVENT && _actualIndex < _lastIndex)
	{
		_actualIndex++;
	}

	// narazili jsme na událost z klávesnice, nebo tam žádná taková není a jsme na konci?
	if (_actualIndex == _lastIndex) return false;
		
	key = _kbdBuf[_actualIndex++].Event.KeyEvent;
	return true;
}

DWORD Keyboard::_read()
{
	DWORD nrEvents;
	ReadConsoleInput(_handle, _kbdBuf, 128, &nrEvents);
	_actualIndex = 0;
	_lastIndex = nrEvents + 1;
	return nrEvents;
}
