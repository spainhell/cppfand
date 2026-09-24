#pragma once
// Replacements of Turbo Pascal runtime routines used by the ported code

#include <string>

#include "pstring.h"
#include "typeDef.h"

void val(pstring s, uint8_t& b, WORD& err);
void val(pstring s, WORD& b, WORD& err);
void val(pstring s, short& b, short& err);
void val(pstring s, double& b, short& err);
void val(pstring s, double& b, WORD& err);
void val(pstring s, int& b, WORD& err);
void val(pstring s, int& b, short& err);
double valDouble(std::string& s, short& err);
pstring copy(pstring source, size_t index, size_t count);
void str(int input, pstring& output);
void str(double input, pstring& output);
void str(double input, std::string& output);
void str(double input, int total, int right, pstring& output);
void str(double input, int total, int right, std::string& output);
void str(double input, int right, pstring& output);
void str(double input, int right, std::string& output);

WORD pred(WORD input);
WORD succ(WORD input);

void FSplit(const std::string& fullname, std::string& dir, std::string& name, std::string& ext, char pathDelim = '\\');

void FillChar(void* cil, int delka, size_t vypln);
uint8_t Hi(WORD cislo);
uint8_t Lo(WORD cislo);
WORD Swap(WORD cislo);
