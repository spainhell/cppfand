#pragma once
#include "constants.h"

class string;
void val(string s, BYTE& b, WORD& err);
string copy(string source, size_t index, size_t count);

void FSplit(string fullname, string& dir, string& name, string& ext);
string FExpand(string path);

void ChDir(string cesta);
void GetDir(BYTE disk, string& cesta);
void MkDir(string cesta);
void RmDir(string cesta);
void Rename(string soubor, string novejmeno);
void Erase(string soubor);

float Random();
WORD Random(WORD rozsah);

WORD ParamCount(); // vrací poèet parametrù pøíkaz. øádky
string ParamStr(integer index);

inline void Exit() { return;; }

inline void RunError(WORD c) { exit(c); }
inline void Halt(WORD c) { exit(c); }

void FillChar(char* cil, WORD delka, char vypln);
void Move(void* zdroj, void* cil, WORD delka);
BYTE Hi(WORD cislo);
BYTE Lo(WORD cislo);
WORD Swap(WORD cislo);

void UnPack(void* PackArr, WORD& NumArr, WORD& NoDigits);
void Pack(void* NumArr, WORD& PackArr, WORD& NoDigits);

/// Class of standard Borland Pascal string
class string
{
public:
    string() : initLen(255) { arr = new BYTE[len]{ '\0' }; }
    string(BYTE length) : initLen(length) { len = length; arr = new BYTE[len] { '\0' }; }
    ~string() { delete[] arr; }

private:
    const BYTE initLen;
    BYTE len = 0xFF;
    BYTE* arr;

    BYTE length() { return len; }
    BYTE initLength() { return initLen; }
	
    BYTE* operator[] (size_t i)
    {
        if (i == 0) return &len;
        if (i > len) { throw std::exception("Index out of range."); }
        i--;
        return &arr[i];
    }

    void operator = (const char* newvalue) {
        size_t newLen = strlen(newvalue);
        if (newLen > len) { throw std::exception("Index out of range."); }
        memcpy((void*)arr[0], (void*)newvalue, newLen);
        len = newLen;
    }

    void operator = (string& newvalue)
    {
    	if (newvalue.initLen >= this->initLen) { throw std::exception("Index out of range."); }
        memcpy((void*)arr[0], (void*)newvalue.arr[0], newvalue.len);
    }

	
};
