#pragma once
#include <cstdint>
#include <deque>
#include <mutex>
#include <Windows.h>

// Tlacitka v kodovani puvodniho PC-FANDu (int 33H): bit 0 = leve, bit 1 = prave.
const WORD msLeftButton = 0x0001;
const WORD msRightButton = 0x0002;

// Puvodni fronta udalosti mela 16 polozek (EventQSize v DRIVERS.PAS).
const size_t MouseQueueSize = 16;

/// Jedna polozka fronty udalosti mysi - odpovida zaznamu EventQueue z DRIVERS.PAS.
/// Cas je v milisekundach, puvodne to byly tiky BIOSu (55 ms).
struct MouseRawEvent
{
	uint64_t Time = 0;
	WORD Buttons = 0;
	WORD X = 0, Y = 0;   // znakove souradnice, 0-based
	WORD GX = 0, GY = 0; // pixely (v textovem rezimu 8 px na bunku)
};

/// Prijem udalosti mysi z konzole (nebo od hostitele). Nahrazuje obsluhu preruseni
/// int 33H (MouseEvHandler v KEYBD.PAS): pohyb se jen pamatuje, do fronty jdou
/// zmeny stavu tlacitek. Zaznamy sem sype Keyboard pri cteni vstupu konzole.
class Mouse
{
public:
	/// Je mys k dispozici? (konzole s povolenym ENABLE_MOUSE_INPUT nebo hostitelsky rezim)
	bool Available() const;
	uint8_t ButtonCount() const;

	void Push(const MOUSE_EVENT_RECORD& record);
	bool Pop(MouseRawEvent& e);
	bool Empty();
	void Clear();

	/// Posledni znama poloha a stav tlacitek.
	void GetState(WORD& buttons, WORD& x, WORD& y, WORD& gx, WORD& gy);
	/// Nastavi logickou polohu (SETMOUSE) bez generovani udalosti.
	void SetPosition(WORD x, WORD y);
	/// Presune ukazatel mysi na znakovou pozici; v hostitelskem rezimu vraci false.
	bool WarpTo(WORD x, WORD y);

private:
	std::mutex _mutex;
	std::deque<MouseRawEvent> _queue;
	WORD _buttons = 0;
	WORD _x = 0, _y = 0;
	WORD _gx = 0, _gy = 0;
};

extern Mouse mouse;
