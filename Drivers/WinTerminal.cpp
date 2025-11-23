#include "WinTerminal.h"
#include <windows.h>
#include <vector>
#include <algorithm>
#include <string>

// Implementation of CharInfo
CharInfo::CharInfo() : Char(L' '), Attributes(0x07) {}
CharInfo::CharInfo(CHAR c, WORD attr) : Char(c), Attributes(attr) {}

// Constructor
WinTerminal::WinTerminal() : cursorPos{ 0, 0 }, hwnd(nullptr), hFont(nullptr), charWidth(8), charHeight(16) {
    buffer.resize(BUFFER_HEIGHT);
    for (auto& row : buffer) {
        row.resize(BUFFER_WIDTH);
    }

    // Create monospace font
    hFont = CreateFont(
        16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN,
        "Consolas"
    );

    // Calculate character size
    HDC hdc = GetDC(NULL);
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    charHeight = tm.tmHeight;

    SIZE size;
    GetTextExtentPoint32(hdc, "W", 1, &size);
    charWidth = size.cx;

    SelectObject(hdc, oldFont);
    ReleaseDC(NULL, hdc);
}

WinTerminal::~WinTerminal() {
    if (hFont) DeleteObject(hFont);
}

void WinTerminal::SetWindow(HWND hwnd) {
    this->hwnd = hwnd;
}

SIZE WinTerminal::GetWindowSize() const {
    SIZE size;
    size.cx = BUFFER_WIDTH * charWidth;
    size.cy = BUFFER_HEIGHT * charHeight;
    return size;
}

void WinTerminal::WriteConsoleOutput(const CHAR_INFO* data, COORD bufferSize,
    COORD bufferCoord, SMALL_RECT* writeRegion) {
    int srcIndex = 0;

    for (int y = writeRegion->Top; y <= writeRegion->Bottom && y < BUFFER_HEIGHT; y++) {
        for (int x = writeRegion->Left; x <= writeRegion->Right && x < BUFFER_WIDTH; x++) {
            int srcX = x - writeRegion->Left + bufferCoord.X;
            int srcY = y - writeRegion->Top + bufferCoord.Y;

            if (srcX < bufferSize.X && srcY < bufferSize.Y) {
                int idx = srcY * bufferSize.X + srcX;
                buffer[y][x].Char = data[idx].Char.AsciiChar;
                buffer[y][x].Attributes = data[idx].Attributes;
            }
        }
    }

    InvalidateRect(hwnd, NULL, FALSE);
}

void WinTerminal::ReadConsoleOutput(CHAR_INFO* data, COORD bufferSize,
    COORD bufferCoord, SMALL_RECT* readRegion) {
    int destIndex = 0;

    for (int y = readRegion->Top; y <= readRegion->Bottom && y < BUFFER_HEIGHT; y++) {
        for (int x = readRegion->Left; x <= readRegion->Right && x < BUFFER_WIDTH; x++) {
            int destX = x - readRegion->Left + bufferCoord.X;
            int destY = y - readRegion->Top + bufferCoord.Y;

            if (destX < bufferSize.X && destY < bufferSize.Y) {
                int idx = destY * bufferSize.X + destX;
                data[idx].Char.UnicodeChar = buffer[y][x].Char;
                data[idx].Attributes = buffer[y][x].Attributes;
            }
        }
    }
}

void WinTerminal::SetConsoleCursorPosition(COORD pos) {
    cursorPos.X = std::max<SHORT>(0, std::min<SHORT>(pos.X, BUFFER_WIDTH - 1));
    cursorPos.Y = std::max<SHORT>(0, std::min<SHORT>(pos.Y, BUFFER_HEIGHT - 1));
    InvalidateRect(hwnd, NULL, FALSE);
}

COORD WinTerminal::GetConsoleCursorPosition() const {
    return cursorPos;
}

void WinTerminal::WriteString(const char* str, WORD attributes) {
    for (const char* p = str; *p; p++) {
        if (*p == '\n') {
            cursorPos.X = 0;
            cursorPos.Y++;
        }
        else if (*p == '\r') {
            cursorPos.X = 0;
        }
        else {
            if (cursorPos.Y < BUFFER_HEIGHT && cursorPos.X < BUFFER_WIDTH) {
                buffer[cursorPos.Y][cursorPos.X].Char = *p;
                buffer[cursorPos.Y][cursorPos.X].Attributes = attributes;
                cursorPos.X++;

                if (cursorPos.X >= BUFFER_WIDTH) {
                    cursorPos.X = 0;
                    cursorPos.Y++;
                }
            }
        }
    }
    InvalidateRect(hwnd, NULL, FALSE);
}

void WinTerminal::Clear() {
    for (auto& row : buffer) {
        for (auto& cell : row) {
            cell.Char = L' ';
            cell.Attributes = 0x07;
        }
    }
    cursorPos.X = cursorPos.Y = 0;
    InvalidateRect(hwnd, NULL, FALSE);
}

COORD WinTerminal::ScreenToBuffer(int x, int y) const {
    COORD pos;
    pos.X = static_cast<SHORT>(x / charWidth);
    pos.Y = static_cast<SHORT>(y / charHeight);
    return pos;
}

void WinTerminal::Paint(HDC hdc) {
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

    // Draw all characters
    for (int y = 0; y < BUFFER_HEIGHT; y++) {
        for (int x = 0; x < BUFFER_WIDTH; x++) {
            const CharInfo& cell = buffer[y][x];

            // Background
            RECT rect;
            rect.left = x * charWidth;
            rect.top = y * charHeight;
            rect.right = rect.left + charWidth;
            rect.bottom = rect.top + charHeight;

            COLORREF bgColor = GetColorFromAttribute(cell.Attributes, true);
            HBRUSH hBrush = CreateSolidBrush(bgColor);
            FillRect(hdc, &rect, hBrush);
            DeleteObject(hBrush);

            // Text
            COLORREF fgColor = GetColorFromAttribute(cell.Attributes, false);
            SetTextColor(hdc, fgColor);
            SetBkMode(hdc, TRANSPARENT);

            TextOut(hdc, x * charWidth, y * charHeight, &cell.Char, 1);
        }
    }

    // Cursor
    RECT cursorRect;
    cursorRect.left = cursorPos.X * charWidth;
    cursorRect.top = cursorPos.Y * charHeight + charHeight - 2;
    cursorRect.right = cursorRect.left + charWidth;
    cursorRect.bottom = cursorRect.top + 2;

    HBRUSH hCursorBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &cursorRect, hCursorBrush);
    DeleteObject(hCursorBrush);

    SelectObject(hdc, oldFont);
}

COLORREF WinTerminal::GetColorFromAttribute(WORD attr, bool background) {
    int colorIndex = background ? ((attr >> 4) & 0x0F) : (attr & 0x0F);

    static const COLORREF colors[16] = {
        RGB(0, 0, 0),       // Black
        RGB(0, 0, 128),     // Dark Blue
        RGB(0, 128, 0),     // Dark Green
        RGB(0, 128, 128),   // Dark Cyan
        RGB(128, 0, 0),     // Dark Red
        RGB(128, 0, 128),   // Dark Magenta
        RGB(128, 128, 0),   // Dark Yellow/Brown
        RGB(192, 192, 192), // Light Gray
        RGB(128, 128, 128), // Dark Gray
        RGB(0, 0, 255),     // Blue
        RGB(0, 255, 0),     // Green
        RGB(0, 255, 255),   // Cyan
        RGB(255, 0, 0),     // Red
        RGB(255, 0, 255),   // Magenta
        RGB(255, 255, 0),   // Yellow
        RGB(255, 255, 255)  // White
    };

    return colors[colorIndex];
}

// Global terminal instance
WinTerminal* g_terminal = nullptr;

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_terminal = new WinTerminal();
        g_terminal->SetWindow(hwnd);

        // Example usage
        g_terminal->WriteString("Terminal Emulator v1.0\n", 0x0A);
        g_terminal->WriteString("Ready for your application!\n", 0x0E);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (g_terminal) {
            g_terminal->Paint(hdc);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        if (g_terminal) {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            COORD pos = g_terminal->ScreenToBuffer(x, y);

            // You can call your application logic here
            char msg[100];
            wsprintf(msg, "Clicked: X=%d, Y=%d        ", pos.X, pos.Y);
            g_terminal->SetConsoleCursorPosition({ 0, 3 });
            g_terminal->WriteString(msg, 0x0F);
        }
        return 0;
    }

    case WM_DESTROY:
        delete g_terminal;
        g_terminal = nullptr;
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}
