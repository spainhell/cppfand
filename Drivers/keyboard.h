#pragma once
#include <queue>
#include <string>
#include <Windows.h>

const __int32 SHIFT = 0x00010000;
const __int32  CTRL = 0x00020000;
const __int32   ALT = 0x00040000;

class Keyboard
{
public:
	Keyboard();
	~Keyboard();
	bool Exists();
	bool Empty();
	size_t BufSize();
	void ClearBuf();
	size_t ActualIndex();
	size_t FreeSpace();
	bool Get(KEY_EVENT_RECORD& key);
	void DeleteKeyBuf(); // erase all items in Prior Key Buffer and System Buffer
	// methods for priority buffer
	std::vector<KEY_EVENT_RECORD> GetKeyBuf(); // return all items form Prior Key Buffer
	std::string GetKeyBufAsString(); // return all items form Prior Key Buffer
	void SetKeyBuf(std::string input); // erase all items in Prior Key Buffer and new from input
	void AddToKeyBuf(std::string input); // add items to the end of Prior Key Buffer
	void AddToKeyBuf(char c); // add items to the end of Prior Key Buffer
	void AddToFrontKeyBuf(std::string input); // add items to the front of Prior Key Buffer
	void AddToFrontKeyBuf(char c); // add items to the front of Prior Key Buffer 
	
private:
	HANDLE _handle;
	PINPUT_RECORD _kbdBuf;
	size_t _actualIndex;
	DWORD _inBuffer;
	std::queue<KEY_EVENT_RECORD> _priorBuffer; // used by SetKeyBuf() method
	void _read();
};

class PressedKey
{
public:
	PressedKey() = default;
	PressedKey(KEY_EVENT_RECORD& key);
	char Char();
	unsigned __int32 KeyDescr();
	unsigned __int32 SimpleKeyDescr();
	unsigned __int32 Function();
	bool isChar();
	bool Shift();
	bool Alt();
	bool Ctrl();
	bool Enhanced();
	bool CapsLock();
	bool ScrollLock();
	bool NumLock();
	bool LeftAlt();
	bool RightAlt();
	bool LeftCtrl();
	bool RightCtrl();
private:
	KEY_EVENT_RECORD _key;
};

