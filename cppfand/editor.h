#pragma once

#include "constants.h"
#include "edevent.h"

const char TextM = 'T'; const char ViewM = 'V'; const char HelpM = 'H';
const char SinFM = 'S'; const char DouFM = 'D'; const char DelFM = 'F';
const char NotFM = 'N'; const char FileT = 'F'; const char LocalT = 'V';
const char MemoT = 'M';
bool Insert, Indent, Wrap, Just;

// {**********global param begin for SavePar}
char Mode;
char TypeT;
PathStr NameT;
string ErrMsg;
WORD MaxLenT, LenT, IndT, ScrT;
string Breaks;
EdExitDPtr ExitD;
bool SrchT, UpdatT;
WORD LastNr, CtrlLastNr;
integer LeftMarg, RightMarg;
bool TypeB;
StringPtr  LastS, CtrlLastS, ShiftLastS, AltLastS, HeadS;
longint* LocalPPtr;
bool EditT;

// od ø. 101
BYTE ColKey[CountC + 1];
BYTE TxtColor, BlockColor, SysLColor;
string InsMsg, nInsMsg, IndMsg, WrapMsg, JustMsg, BlockMsg;
string ViewMsg;
char CharPg ;
bool InsPg ;
longint BegBLn, EndBLn ;
WORD BegBPos, EndBPos ;
WORD ScrI, LineI, Posi, BPos; // {screen status}
string FindStr, ReplaceStr;
bool Replace ;
ScreenStr OptionStr ;
bool FirstEvent ;
WORD PHNum, PPageS ; // {strankovani ve Scroll}

struct PartDescr
{
	longint PosP; longint LineP;
	WORD LenP, MovI, MovL;
	bool UpdP;
	ColorOrd ColorP;
} Part;

PartDescr Part;
WORD TxtFH;
PathStr TxtPath;
VolStr TxtVol;
bool AllRd;
longint AbsLenT;
bool ChangePart, UpdPHead;


// ø. 451
void InitTxtEditor();
