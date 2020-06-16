#pragma once
#include <string>

extern int strcount;
extern int strbytes;

class pstring
{
public:
    pstring();
    pstring(unsigned char length);
    pstring(const char* text);
    pstring(const pstring& ps);
    pstring(std::string cs);
    pstring(char str[], unsigned char i);
    ~pstring();

    unsigned char length();
    unsigned short initLength();
    void cut(unsigned char length);
    void clean();
    int first(char c);
    void Delete(int index, int size);

    pstring substr(unsigned char index);
    pstring substr(unsigned char index, unsigned char count);

    void replace(const char* value); // smaže obsah a nahradí jej novou hodnotou
    void insert(const char* value, unsigned char position); // vloží na pozici novou hodnotu, pùvodní se posune

    const char* c_str();
    bool empty();

    unsigned char& operator[] (size_t i);
    pstring& operator = (const char* newvalue);
    pstring& operator = (std::basic_string<char> newvalue);
    pstring& operator = (const pstring& pstring);
    operator std::string() const;
    pstring& operator += (const pstring& pstring);
    pstring operator + (const pstring& pstring);
	bool operator == (const pstring& pstring);
    bool operator != (const pstring& pstring);
    void Append(unsigned char c);

private:
    const unsigned short initLen;
    unsigned char* arr;
};

