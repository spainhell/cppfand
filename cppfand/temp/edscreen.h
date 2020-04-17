#pragma once
#include <string>

using namespace std;

const int TXTCOLS = 80;

// TYPY
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef int longint;
typedef void* ArrPtr;
typedef string ColorOrd;

// PROMENNE
bool InsPage;

// METODY
void WrStatusLine();
void WriteMargins();
void WrLLMargMsg(string* s, WORD n);
void InitScr();
void UpdStatLine(int Row, int Col);
void EditWrline(ArrPtr P, int Row);
void ScrollWrline(ArrPtr P, int Row, ColorOrd CO);
BYTE Color(ColorOrd CO); // vnoøená do pøedešlé
bool MyTestEvent();
void UpdScreen();
void WrEndL(bool Hard, int Row); // vnoøená do pøedešlé
void Background();
