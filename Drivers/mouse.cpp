#include "mouse.h"
#include "host.h"
#include <chrono>

Mouse mouse;

namespace
{
	uint64_t nowMs()
	{
		return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	// V textovem rezimu puvodni ovladac prepocitaval bunky na pixely pomerem 8:1.
	const WORD PixelsPerCell = 8;
}

bool Mouse::Available() const
{
	if (FandHost::IsEnabled()) return true;
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	if (h == nullptr || h == INVALID_HANDLE_VALUE) return false;
	DWORD mode = 0;
	if (!GetConsoleMode(h, &mode)) return false;
	return (mode & ENABLE_MOUSE_INPUT) != 0;
}

uint8_t Mouse::ButtonCount() const
{
	int n = GetSystemMetrics(SM_CMOUSEBUTTONS);
	if (n < 2) n = 2;
	if (n > 255) n = 255;
	return (uint8_t)n;
}

void Mouse::Push(const MOUSE_EVENT_RECORD& record)
{
	// kolecko puvodni PC-FAND neznal
	if ((record.dwEventFlags & (MOUSE_WHEELED | MOUSE_HWHEELED)) != 0) return;

	std::lock_guard<std::mutex> lock(_mutex);

	WORD buttons = 0;
	if ((record.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0) buttons |= msLeftButton;
	if ((record.dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0) buttons |= msRightButton;

	const WORD x = (WORD)record.dwMousePosition.X;
	const WORD y = (WORD)record.dwMousePosition.Y;
	const bool buttonsChanged = buttons != _buttons;

	_buttons = buttons;
	_x = x; _y = y;
	_gx = x * PixelsPerCell; _gy = y * PixelsPerCell;

	// do fronty jde jen stisk/uvolneni tlacitka, pohyb se pozna z aktualni polohy
	if (!buttonsChanged) return;
	if (_queue.size() >= MouseQueueSize) return;

	MouseRawEvent e;
	e.Time = nowMs();
	e.Buttons = buttons;
	e.X = x; e.Y = y;
	e.GX = _gx; e.GY = _gy;
	_queue.push_back(e);
}

bool Mouse::Pop(MouseRawEvent& e)
{
	std::lock_guard<std::mutex> lock(_mutex);
	if (_queue.empty()) return false;
	e = _queue.front();
	_queue.pop_front();
	return true;
}

bool Mouse::Empty()
{
	std::lock_guard<std::mutex> lock(_mutex);
	return _queue.empty();
}

void Mouse::Clear()
{
	std::lock_guard<std::mutex> lock(_mutex);
	_queue.clear();
}

void Mouse::GetState(WORD& buttons, WORD& x, WORD& y, WORD& gx, WORD& gy)
{
	std::lock_guard<std::mutex> lock(_mutex);
	buttons = _buttons;
	x = _x; y = _y;
	gx = _gx; gy = _gy;
}

void Mouse::SetPosition(WORD x, WORD y)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_x = x; _y = y;
	_gx = x * PixelsPerCell; _gy = y * PixelsPerCell;
}

bool Mouse::WarpTo(WORD x, WORD y)
{
	// hostitel si ukazatel kresli sam, presouvat systemovy kurzor nema smysl
	if (FandHost::IsEnabled()) return false;

	HWND wnd = GetConsoleWindow();
	if (wnd == nullptr) return false;
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	if (out == nullptr || out == INVALID_HANDLE_VALUE) return false;

	CONSOLE_FONT_INFO font{};
	CONSOLE_SCREEN_BUFFER_INFO info{};
	if (!GetCurrentConsoleFont(out, FALSE, &font)) return false;
	if (!GetConsoleScreenBufferInfo(out, &info)) return false;
	if (font.dwFontSize.X <= 0 || font.dwFontSize.Y <= 0) return false;

	POINT p;
	p.x = (LONG)(x - info.srWindow.Left) * font.dwFontSize.X + font.dwFontSize.X / 2;
	p.y = (LONG)(y - info.srWindow.Top) * font.dwFontSize.Y + font.dwFontSize.Y / 2;
	if (!ClientToScreen(wnd, &p)) return false;
	return SetCursorPos(p.x, p.y) != FALSE;
}
