// {$I edscreen.pas}
// {$I edevent.pas}

#pragma once
#include "pstring.h"
#include "constants.h"
#include <set>

const int COL = 80;

typedef char* ArrLine;
typedef ArrLine ArrPtr;
typedef pstring ColorOrd;

struct Character {
	char ch;
	BYTE color;
};

ArrLine Arr;
WORD NextI;
integer LineL, ScrL;
longint RScrL;
bool UpdatedL, CtrlL, HardL;
WORD BCol, Colu, Row;
bool ChangeScr;
ColorOrd ColScr;
bool IsWrScreen;

WORD FirstR, FirstC, LastR, LastC, MinC, MinR, MaxC, MaxR;
WORD MargLL[4];
WORD PageS, LineS;
bool bScroll, FirstScroll, HelpScroll;
longint PredScLn;
WORD PredScPos;
BYTE FrameDir;
WORD WordL;
bool Konec;

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
pstring ShortName(pstring Name);
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


