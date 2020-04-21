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
//typedef pstring VolStr; // base.pas, �22, string[11]

const WORD _F1 = 59;
const WORD _F6 = 64;
const WORD _F9 = 67;
const WORD _F10 = 68;
const WORD _CtrlF1 = 94;
const WORD _CtrlF2 = 95;
const WORD _CtrlF3 = 96;
const WORD _CtrlF4 = 97;
const WORD _CtrlF5 = 98;
const WORD _CtrlF6 = 99;
const WORD _CtrlF7 = 100;
const WORD _CtrlF8 = 101;
const WORD _CtrlF9 = 102;
const WORD _CtrlF10 = 103;
const WORD _AltF1 = 104;
const WORD _AltF2 = 105;
const WORD _AltF3 = 106;
const WORD _AltF4 = 107;
const WORD _AltF5 = 108;
const WORD _AltF6 = 109;
const WORD _AltF7 = 110;
const WORD _AltF8 = 111;
const WORD _AltF9 = 112;
const WORD _AltF10 = 113;
const WORD _ShiftF1 = 0x54;
const WORD _CtrlHome = 0x77;
const WORD _CtrlEnd = 0x75;
const WORD _EOF = 0x04;
const WORD _CR = 0x13;
const WORD _LF = 0x10;
const WORD _ESC = 0x1B;

const WORD _F1_ = 0x3B00;
const WORD _F2_ = 0x3C00;
const WORD _F3_ = 0x3D00;
const WORD _F4_ = 0x3E00;
const WORD _F5_ = 0x3F00;
const WORD _F6_ = 0x4000;
const WORD _F7_ = 0x4100;
const WORD _F8_ = 0x4200;
const WORD _F9_ = 0x4300;
const WORD _F10_ = 0x4400;
const WORD _ShiftF1_ = 0x5400;
const WORD _ShiftF2_ = 0x5500;
const WORD _ShiftF3_ = 0x5600;
const WORD _ShiftF4_ = 0x5700;
const WORD _ShiftF5_ = 0x5800;
const WORD _ShiftF6_ = 0x5900;
const WORD _ShiftF7_ = 0x5A00;
const WORD _ShiftF8_ = 0x5B00;
const WORD _ShiftF9_ = 0x5C00;
const WORD _ShiftF10_ = 0x5D00;
const WORD _CtrlF1_ = 0x5E00;
const WORD _CtrlF2_ = 0x5F00;
const WORD _CtrlF3_ = 0x6000;
const WORD _CtrlF4_ = 0x6100;
const WORD _CtrlF5_ = 0x6200;
const WORD _CtrlF6_ = 0x6300;
const WORD _CtrlF7_ = 0x6400;
const WORD _CtrlF8_ = 0x6500;
const WORD _CtrlF9_ = 0x6600;
const WORD _CtrlF10_ = 0x6700;
const WORD _AltF1_ = 0x6800;
const WORD _AltF2_ = 0x6900;
const WORD _AltF3_ = 0x6A00;
const WORD _AltF4_ = 0x6B00;
const WORD _AltF5_ = 0x6C00;
const WORD _AltF6_ = 0x6D00;
const WORD _AltF7_ = 0x6E00;
const WORD _AltF8_ = 0x6F00;
const WORD _AltF9_ = 0x7000;
const WORD _AltF10_ = 0x7100;

const WORD _left_ = 0x4B00;
const WORD _right_ = 0x4D00;
const WORD _up_ = 0x4800;
const WORD _down_ = 0x5000;
const WORD _Home_ = 0x4700;
const WORD _End_ = 0x4F00;
const WORD _PgUp_ = 0x4900;
const WORD _PgDn_ = 0x5100;
const WORD _CtrlLeft_ = 0x7300;
const WORD _CtrlRight_ = 0x7400;
const WORD _CtrlPgUp_ = 0x0300; // !differs
const WORD _CtrlPgDn_ = 0x7600;
const WORD _CtrlHome_ = 0x7700;
const WORD _CtrlEnd_ = 0x7500;
const WORD _Ins_ = 0x5200;
const WORD _Del_ = 0x5300;
const WORD _Tab_ = 0x9;
const WORD _ShiftTab_ = 0x0F00;
const WORD _AltEqual_ = 0x8300;

const WORD _ESC_ = 0x1B;
const WORD _A_ = 1;
const WORD _B_ = 2;
const WORD _C_ = 3;
const WORD _D_ = 4;
const WORD _E_ = 5;
const WORD _F_ = 6;
const WORD _G_ = 7;
const WORD _H_ = 8;
const WORD _I_ = 9;
const WORD _J_ = 10;
const WORD _K_ = 11;
const WORD _L_ = 12;
const WORD _M_ = 13;
const WORD _N_ = 14;
const WORD _O_ = 15;
const WORD _P_ = 16;
const WORD _Q_ = 17;
const WORD _R_ = 18;
const WORD _S_ = 19;
const WORD _T_ = 20;
const WORD _U_ = 21;
const WORD _V_ = 22;
const WORD _W_ = 23;
const WORD _X_ = 24;
const WORD _Y_ = 25;
const WORD _Z_ = 26;

std::vector<std::string> paramstr(5);

// ********** DRIVERS.PAS ********** - �. 225 - 231
const WORD Gr640x350 = 0x10;
const WORD Gr640x480 = 0x12;
const WORD Txt80x25 = 0x03;
const WORD GDC_port = 0x3ce;
const WORD SetRes_reg = 0;
const WORD Enable_reg = 1;
const WORD Func_reg = 3;
const WORD Map_reg = 4;
const WORD Mode_reg = 5;
const WORD ColorCare_reg = 7;
const WORD Mask_reg = 8;
const WORD SEQ_port = 0x3c4;
const WORD Seq2_reg = 2;
const WORD Seq4_reg = 4;
const WORD CrsTimeOn = 0x0003;
const WORD CrsTimeOff = 0x0005;
