#pragma once
#include <queue>
#include <string>
#include <Windows.h>

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
	bool Get(KEY_EVENT_RECORD& key, bool only_check = false, bool ignore_other_events = false);
	bool GetMouse(MOUSE_EVENT_RECORD& mouse, bool only_check = false);
	void DeleteKeyBuf(); // erase all items in Prior Key Buffer and System Buffer
	// methods for priority buffer
	std::vector<KEY_EVENT_RECORD> GetKeyBuf(); // return all items form Prior Key Buffer
	std::string GetKeyBufAsString(); // return all items form Prior Key Buffer
	void SetKeyBuf(std::string input); // erase all items in Prior Key Buffer and new from input
	void AddToKeyBuf(std::string input); // add items to the end of Prior Key Buffer
	void AddToKeyBuf(unsigned short c); // add items to the end of Prior Key Buffer
	void AddToFrontKeyBuf(std::string input); // add items to the front of Prior Key Buffer
	void AddToFrontKeyBuf(unsigned short c); // add items to the front of Prior Key Buffer
	short GetState(int nVirtKey); // get state of the Virtual key
	
private:
	HANDLE _handle;
	PINPUT_RECORD _kbdBuf;
	size_t _actualIndex; // index of the last read event
	DWORD _inBuffer; // number of events in buffer
	std::deque<KEY_EVENT_RECORD> _priorBuffer; // used by SetKeyBuf() method
	void _read();
};

class PressedKey
{
public:
	PressedKey() = default;
	PressedKey(KEY_EVENT_RECORD& key);
	KEY_EVENT_RECORD* Key();
	/// transformed character (for use with Alt, Ctrl or Shift)
	char Char; 
	unsigned __int32 KeyDescr();
	unsigned __int32 SimpleKeyDescr();
	unsigned short KeyCombination();
	void UpdateKey(WORD newKey);
	//unsigned __int32 Function();
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

