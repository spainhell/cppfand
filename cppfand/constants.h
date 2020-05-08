#pragma once
#include <vector>
#include <string>


typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef int longint;
typedef short integer;

//typedef pstring ScreenStr;
//typedef pstring* StringPtr;
//typedef pstring DirStr;
//typedef pstring PathStr;
//typedef pstring NameStr;
//typedef pstring ExtStr;
//typedef pstring VolStr; // base.pas, ø22, string[11]

static const WORD _F1 = 59;
static const WORD _F6 = 64;
static const WORD _F9 = 67;
static const WORD _F10 = 68;
static const WORD _CtrlF1 = 94;
static const WORD _CtrlF2 = 95;
static const WORD _CtrlF3 = 96;
static const WORD _CtrlF4 = 97;
static const WORD _CtrlF5 = 98;
static const WORD _CtrlF6 = 99;
static const WORD _CtrlF7 = 100;
static const WORD _CtrlF8 = 101;
static const WORD _CtrlF9 = 102;
static const WORD _CtrlF10 = 103;
static const WORD _AltF1 = 104;
static const WORD _AltF2 = 105;
static const WORD _AltF3 = 106;
static const WORD _AltF4 = 107;
static const WORD _AltF5 = 108;
static const WORD _AltF6 = 109;
static const WORD _AltF7 = 110;
static const WORD _AltF8 = 111;
static const WORD _AltF9 = 112;
static const WORD _AltF10 = 113;
static const WORD _ShiftF1 = 0x54;
static const WORD _CtrlHome = 0x77;
static const WORD _CtrlEnd = 0x75;
static const WORD _EOF = 0x04;
static const WORD _CR = 0x13;
static const WORD _LF = 0x10;
static const WORD _ESC = 0x1B;

static const WORD _F1_ = 0x3B00;
static const WORD _F2_ = 0x3C00;
static const WORD _F3_ = 0x3D00;
static const WORD _F4_ = 0x3E00;
static const WORD _F5_ = 0x3F00;
static const WORD _F6_ = 0x4000;
static const WORD _F7_ = 0x4100;
static const WORD _F8_ = 0x4200;
static const WORD _F9_ = 0x4300;
static const WORD _F10_ = 0x4400;
static const WORD _ShiftF1_ = 0x5400;
static const WORD _ShiftF2_ = 0x5500;
static const WORD _ShiftF3_ = 0x5600;
static const WORD _ShiftF4_ = 0x5700;
static const WORD _ShiftF5_ = 0x5800;
static const WORD _ShiftF6_ = 0x5900;
static const WORD _ShiftF7_ = 0x5A00;
static const WORD _ShiftF8_ = 0x5B00;
static const WORD _ShiftF9_ = 0x5C00;
static const WORD _ShiftF10_ = 0x5D00;
static const WORD _CtrlF1_ = 0x5E00;
static const WORD _CtrlF2_ = 0x5F00;
static const WORD _CtrlF3_ = 0x6000;
static const WORD _CtrlF4_ = 0x6100;
static const WORD _CtrlF5_ = 0x6200;
static const WORD _CtrlF6_ = 0x6300;
static const WORD _CtrlF7_ = 0x6400;
static const WORD _CtrlF8_ = 0x6500;
static const WORD _CtrlF9_ = 0x6600;
static const WORD _CtrlF10_ = 0x6700;
static const WORD _AltF1_ = 0x6800;
static const WORD _AltF2_ = 0x6900;
static const WORD _AltF3_ = 0x6A00;
static const WORD _AltF4_ = 0x6B00;
static const WORD _AltF5_ = 0x6C00;
static const WORD _AltF6_ = 0x6D00;
static const WORD _AltF7_ = 0x6E00;
static const WORD _AltF8_ = 0x6F00;
static const WORD _AltF9_ = 0x7000;
static const WORD _AltF10_ = 0x7100;

static const WORD _left_ = 0x4B00;
static const WORD _right_ = 0x4D00;
static const WORD _up_ = 0x4800;
static const WORD _down_ = 0x5000;
static const WORD _Home_ = 0x4700;
static const WORD _End_ = 0x4F00;
static const WORD _PgUp_ = 0x4900;
static const WORD _PgDn_ = 0x5100;
static const WORD _CtrlLeft_ = 0x7300;
static const WORD _CtrlRight_ = 0x7400;
static const WORD _CtrlPgUp_ = 0x0300; // !differs
static const WORD _CtrlPgDn_ = 0x7600;
static const WORD _CtrlHome_ = 0x7700;
static const WORD _CtrlEnd_ = 0x7500;
static const WORD _Ins_ = 0x5200;
static const WORD _Del_ = 0x5300;
static const WORD _Tab_ = 0x9;
static const WORD _ShiftTab_ = 0x0F00;
static const WORD _AltEqual_ = 0x8300;

static const WORD _ESC_ = 0x1B;
static const WORD _A_ = 1;
static const WORD _B_ = 2;
static const WORD _C_ = 3;
static const WORD _D_ = 4;
static const WORD _E_ = 5;
static const WORD _F_ = 6;
static const WORD _G_ = 7;
static const WORD _H_ = 8;
static const WORD _I_ = 9;
static const WORD _J_ = 10;
static const WORD _K_ = 11;
static const WORD _L_ = 12;
static const WORD _M_ = 13;
static const WORD _N_ = 14;
static const WORD _O_ = 15;
static const WORD _P_ = 16;
static const WORD _Q_ = 17;
static const WORD _R_ = 18;
static const WORD _S_ = 19;
static const WORD _T_ = 20;
static const WORD _U_ = 21;
static const WORD _V_ = 22;
static const WORD _W_ = 23;
static const WORD _X_ = 24;
static const WORD _Y_ = 25;
static const WORD _Z_ = 26;

std::vector<std::string> paramstr(5);

// ********** DRIVERS.PAS ********** - ø. 225 - 231
static const WORD Gr640x350 = 0x10;
static const WORD Gr640x480 = 0x12;
static const WORD Txt80x25 = 0x03;
static const WORD GDC_port = 0x3ce;
static const WORD SetRes_reg = 0;
static const WORD Enable_reg = 1;
static const WORD Func_reg = 3;
static const WORD Map_reg = 4;
static const WORD Mode_reg = 5;
static const WORD ColorCare_reg = 7;
static const WORD Mask_reg = 8;
static const WORD SEQ_port = 0x3c4;
static const WORD Seq2_reg = 2;
static const WORD Seq4_reg = 4;
static const WORD CrsTimeOn = 0x0003;
static const WORD CrsTimeOff = 0x0005;
