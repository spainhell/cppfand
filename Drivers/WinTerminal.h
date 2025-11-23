#pragma once

#include <windows.h>
#include <vector>
#include <algorithm>

// Structure for a character in the buffer (compatible with CHAR_INFO)
struct CharInfo {
    CHAR Char;
    WORD Attributes;

    CharInfo();
    CharInfo(CHAR c, WORD attr);
};

// Class for terminal emulation
class WinTerminal {
public:
    WinTerminal();
    ~WinTerminal();

    void SetWindow(HWND hwnd);

    // Get recommended window size
    SIZE GetWindowSize() const;

    // Emulator for WriteConsoleOutput
    void WriteConsoleOutput(const CHAR_INFO* data, COORD bufferSize,
        COORD bufferCoord, SMALL_RECT* writeRegion);

    // Emulator for ReadConsoleOutput
    void ReadConsoleOutput(CHAR_INFO* data, COORD bufferSize,
        COORD bufferCoord, SMALL_RECT* readRegion);

    // Emulator for SetConsoleCursorPosition
    void SetConsoleCursorPosition(COORD pos);

    COORD GetConsoleCursorPosition() const;

    // Write text (helper function)
    void WriteString(const char* str, WORD attributes = 0x07);

    // Clear the buffer
    void Clear();

    // Convert mouse coordinates to buffer position
    COORD ScreenToBuffer(int x, int y) const;

    // Paint the terminal
    void Paint(HDC hdc);

private:
    static const int BUFFER_WIDTH = 80;
    static const int BUFFER_HEIGHT = 25;

    std::vector<std::vector<CharInfo>> buffer;
    COORD cursorPos;
    HWND hwnd;
    HFONT hFont;
    int charWidth;
    int charHeight;

    // Convert attributes to colors
    COLORREF GetColorFromAttribute(WORD attr, bool background);
};