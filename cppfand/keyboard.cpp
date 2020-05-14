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
