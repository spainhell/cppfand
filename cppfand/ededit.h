// {$I edscreen.pas}
// {$I edevent.pas}

#pragma once
#include <string>
#include <set>

using namespace std;

const int COL = 80;

typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef char ArrLine[COL];
typedef char(*ArtPtr)[COL];
typedef string ColorOrd;
typedef int longint;
typedef string pathstr;

struct Character {
	char ch;
	BYTE color;
};


void Edit(WORD SuccLineSize);
void DekodLine(); // TODO: ;forward; ??? co to je?

// segmenty
void DelEndT();
void WrEndT();
void MoveIdx(int dir);
void TestUpdFile();
void SetUpdat();
void PredPart();
void NextPart();
void NextPartDek();
void SetPart(longint Idex);

void SetPartLine(longint Ln);
longint LineAbs(int Ln);
bool LineInBlock(int Ln);
bool LineBndBlock(int Ln);

// strankovani ve Scroll
longint NewRL(int Line);
int NewL(longint RLine);
bool ModPage(longint RLine);

void TestLenText(WORD F, longint LL);
void SmallerPart(WORD Ind, WORD FreeSize); // tato je vnoøená v pøedchozí
void DekodLine();
pathstr ShortName(pathstr Name);
WORD CountChar(char C, WORD First, WORD Last);
WORD SetLine(WORD Ind);
WORD SetCurrI(WORD Ind);
void SetDekCurrI(WORD Ind);
void SetDekLnCurrI(WORD Ind);
WORD FindLine(int Num);
void DekFindLine(longint Num);
void PosDekFindLine(longint Num, WORD Pos, bool ChScr);
WORD SetInd(WORD Ind, WORD Pos);
WORD Position(WORD c);
WORD Column(WORD p);
void SetScreen(WORD Ind, WORD ScrXY, WORD Pos);
WORD LastPosLine();
void KodLine();
void TestKod();
void NextLine(bool WrScr);
void MyWriteln(); // vnoøená do pøechozí

// help
bool WordExist();
WORD WordNo(WORD I);
WORD WordNo2();
void ClrWord();
bool WordFind(WORD i, WORD WB, WORD WE, WORD LI);
void SetWord(WORD WB, WORD WE);
void CursorWord();

// BEGIN OF Edit


