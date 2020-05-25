#pragma once

typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef int longint;
typedef short integer;

const BYTE FandFace = 17;
const BYTE FloppyDrives = 3;

// *** VIRTUAL KEYS ***
const WORD VK_LBUTTON = 0x01;
const WORD VK_RBUTTON = 0x02;
const WORD VK_CANCEL = 0x03;
const WORD VK_MBUTTON = 0x04;
//
const WORD VK_XBUTTON1 = 0x05;
const WORD VK_XBUTTON2 = 0x06;
//
const WORD VK_BACK = 0x08;
const WORD VK_TAB = 0x09;
//
const WORD VK_CLEAR = 0x0C;
const WORD VK_RETURN = 0x0D;
//
const WORD VK_SHIFT = 0x10;
const WORD VK_CONTROL = 0x11;
const WORD VK_MENU = 0x12;
const WORD VK_PAUSE = 0x13;
const WORD VK_CAPITAL = 0x14;
//
const WORD VK_KANA = 0x15;
const WORD VK_HANGEUL = 0x15;  /* old name - should be here for compatibility */
const WORD VK_HANGUL = 0x15;
const WORD VK_JUNJA = 0x17;
const WORD VK_FINAL = 0x18;
const WORD VK_HANJA = 0x19;
const WORD VK_KANJI = 0x19;
//
const WORD VK_ESCAPE = 0x1B;
//
const WORD VK_CONVERT = 0x1C;
const WORD VK_NONCONVERT = 0x1D;
const WORD VK_ACCEPT = 0x1E;
const WORD VK_MODECHANGE = 0x1F;
//
const WORD VK_SPACE = 0x20;
const WORD VK_PRIOR = 0x21;
const WORD VK_NEXT = 0x22;
const WORD VK_END = 0x23;
const WORD VK_HOME = 0x24;
const WORD VK_LEFT = 0x25;
const WORD VK_UP = 0x26;
const WORD VK_RIGHT = 0x27;
const WORD VK_DOWN = 0x28;
const WORD VK_SELECT = 0x29;
const WORD VK_PRINT = 0x2A;
const WORD VK_EXECUTE = 0x2B;
const WORD VK_SNAPSHOT = 0x2C;
const WORD VK_INSERT = 0x2D;
const WORD VK_DELETE = 0x2E;
const WORD VK_HELP = 0x2F;
//
const WORD VK_LWIN = 0x5B;
const WORD VK_RWIN = 0x5C;
const WORD VK_APPS = 0x5D;
//
const WORD VK_SLEEP = 0x5F;
//
const WORD VK_NUMPAD0 = 0x60;
const WORD VK_NUMPAD1 = 0x61;
const WORD VK_NUMPAD2 = 0x62;
const WORD VK_NUMPAD3 = 0x63;
const WORD VK_NUMPAD4 = 0x64;
const WORD VK_NUMPAD5 = 0x65;
const WORD VK_NUMPAD6 = 0x66;
const WORD VK_NUMPAD7 = 0x67;
const WORD VK_NUMPAD8 = 0x68;
const WORD VK_NUMPAD9 = 0x69;
const WORD VK_MULTIPLY = 0x6A;
const WORD VK_ADD = 0x6B;
const WORD VK_SEPARATOR = 0x6C;
const WORD VK_SUBTRACT = 0x6D;
const WORD VK_DECIMAL = 0x6E;
const WORD VK_DIVIDE = 0x6F;
const WORD VK_F1 = 0x70;
const WORD VK_F2 = 0x71;
const WORD VK_F3 = 0x72;
const WORD VK_F4 = 0x73;
const WORD VK_F5 = 0x74;
const WORD VK_F6 = 0x75;
const WORD VK_F7 = 0x76;
const WORD VK_F8 = 0x77;
const WORD VK_F9 = 0x78;
const WORD VK_F10 = 0x79;
const WORD VK_F11 = 0x7A;
const WORD VK_F12 = 0x7B;
const WORD VK_F13 = 0x7C;
const WORD VK_F14 = 0x7D;
const WORD VK_F15 = 0x7E;
const WORD VK_F16 = 0x7F;
const WORD VK_F17 = 0x80;
const WORD VK_F18 = 0x81;
const WORD VK_F19 = 0x82;
const WORD VK_F20 = 0x83;
const WORD VK_F21 = 0x84;
const WORD VK_F22 = 0x85;
const WORD VK_F23 = 0x86;
const WORD VK_F24 = 0x87;
//
const WORD VK_NUMLOCK = 0x90;
const WORD VK_SCROLL = 0x91;
//
const WORD VK_OEM_NEC_EQUAL = 0x92;   // '=' key on numpad
						   //
const WORD VK_OEM_FJ_JISHO = 0x92;   // 'Dictionary' key
const WORD VK_OEM_FJ_MASSHOU = 0x93;   // 'Unregister word' key
const WORD VK_OEM_FJ_TOUROKU = 0x94;   // 'Register word' key
const WORD VK_OEM_FJ_LOYA = 0x95;   // 'Left OYAYUBI' key
const WORD VK_OEM_FJ_ROYA = 0x96;   // 'Right OYAYUBI' key
						 //
const WORD VK_LSHIFT = 0xA0;
const WORD VK_RSHIFT = 0xA1;
const WORD VK_LCONTROL = 0xA2;
const WORD VK_RCONTROL = 0xA3;
const WORD VK_LMENU = 0xA4;
const WORD VK_RMENU = 0xA5;
//
const WORD VK_BROWSER_BACK = 0xA6;
const WORD VK_BROWSER_FORWARD = 0xA7;
const WORD VK_BROWSER_REFRESH = 0xA8;
const WORD VK_BROWSER_STOP = 0xA9;
const WORD VK_BROWSER_SEARCH = 0xAA;
const WORD VK_BROWSER_FAVORITES = 0xAB;
const WORD VK_BROWSER_HOME = 0xAC;
//
const WORD VK_VOLUME_MUTE = 0xAD;
const WORD VK_VOLUME_DOWN = 0xAE;
const WORD VK_VOLUME_UP = 0xAF;
const WORD VK_MEDIA_NEXT_TRACK = 0xB0;
const WORD VK_MEDIA_PREV_TRACK = 0xB1;
const WORD VK_MEDIA_STOP = 0xB2;
const WORD VK_MEDIA_PLAY_PAUSE = 0xB3;
const WORD VK_LAUNCH_MAIL = 0xB4;
const WORD VK_LAUNCH_MEDIA_SELECT = 0xB5;
const WORD VK_LAUNCH_APP1 = 0xB6;
const WORD VK_LAUNCH_APP2 = 0xB7;
//
const WORD VK_OEM_1 = 0xBA;   // ';:' for US
const WORD VK_OEM_PLUS = 0xBB;   // '+' any country
const WORD VK_OEM_COMMA = 0xBC;   // ';' any country
const WORD VK_OEM_MINUS = 0xBD;   // '-' any country
const WORD VK_OEM_PERIOD = 0xBE;   // '.' any country
const WORD VK_OEM_2 = 0xBF;   // '/?' for US
const WORD VK_OEM_3 = 0xC0;   // '`~' for US
				   //
const WORD VK_OEM_4 = 0xDB;  //  '[{' for US
const WORD VK_OEM_5 = 0xDC;  //  '\|' for US
const WORD VK_OEM_6 = 0xDD;  //  ']}' for US
const WORD VK_OEM_7 = 0xDE;  //  ''"' for US
const WORD VK_OEM_8 = 0xDF;
//
const WORD VK_OEM_AX = 0xE1;  //  'AX' key on Japanese AX kbd
const WORD VK_OEM_102 = 0xE2;  //  "<>" or "\|" on RT 102-key kbd.
const WORD VK_ICO_HELP = 0xE3;  //  Help key on ICO
const WORD VK_ICO_00 = 0xE4;  //  00 key on ICO
				   //
const WORD VK_PROCESSKEY = 0xE5;
//
const WORD VK_ICO_CLEAR = 0xE6;
//
const WORD VK_PACKET = 0xE7;
//
const WORD VK_OEM_RESET = 0xE9;
const WORD VK_OEM_JUMP = 0xEA;
const WORD VK_OEM_PA1 = 0xEB;
const WORD VK_OEM_PA2 = 0xEC;
const WORD VK_OEM_PA3 = 0xED;
const WORD VK_OEM_WSCTRL = 0xEE;
const WORD VK_OEM_CUSEL = 0xEF;
const WORD VK_OEM_ATTN = 0xF0;
const WORD VK_OEM_FINISH = 0xF1;
const WORD VK_OEM_COPY = 0xF2;
const WORD VK_OEM_AUTO = 0xF3;
const WORD VK_OEM_ENLW = 0xF4;
const WORD VK_OEM_BACKTAB = 0xF5;
//
const WORD VK_ATTN = 0xF6;
const WORD VK_CRSEL = 0xF7;
const WORD VK_EXSEL = 0xF8;
const WORD VK_EREOF = 0xF9;
const WORD VK_PLAY = 0xFA;
const WORD VK_ZOOM = 0xFB;
const WORD VK_NONAME = 0xFC;
const WORD VK_PA1 = 0xFD;
const WORD VK_OEM_CLEAR = 0xFE;

// *** end virtual keys ***

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
const WORD _CR = 0x0D;
const WORD _LF = 0x0A;
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
const WORD _A_ = 0x41;
const WORD _B_ = 0x42;
const WORD _C_ = 0x43;
const WORD _D_ = 0x44;
const WORD _E_ = 0x45;
const WORD _F_ = 0x46;
const WORD _G_ = 0x47;
const WORD _H_ = 0x48;
const WORD _I_ = 0x49;
const WORD _J_ = 0x4A;
const WORD _K_ = 0x4B;
const WORD _L_ = 0x4C;
const WORD _M_ = 0x4D;
const WORD _N_ = 0x4E;
const WORD _O_ = 0x4F;
const WORD _P_ = 0x50;
const WORD _Q_ = 0x51;
const WORD _R_ = 0x52;
const WORD _S_ = 0x53;
const WORD _T_ = 0x54;
const WORD _U_ = 0x55;
const WORD _V_ = 0x56;
const WORD _W_ = 0x57;
const WORD _X_ = 0x58;
const WORD _Y_ = 0x59;
const WORD _Z_ = 0x60;

// ********** DRIVERS.PAS ********** - ø. 225 - 231
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

// *** BASE.H ***
longint const UserLicNrShow = 999001; // 160188
const WORD FDVersion = 0x0411;
const WORD ResVersion = 0x0420;
const char CfgVersion[] = { '4', '.', '2', '0', '\0' };
const BYTE DMLVersion = 41;
const WORD NoDayInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const bool HasCoproc = false;

const WORD MaxLStrLen = 65000;
const BYTE WShadow = 0x01; // window flags
const BYTE WNoClrScr = 0x02;
const BYTE WPushPixel = 0x04;
const BYTE WNoPop = 0x08;
const BYTE WHasFrame = 0x10;
const BYTE WDoubleFrame = 0x20;

const BYTE CachePageShft = 12;
const WORD NCachePages = 0;
const WORD XMSCachePages = 0;

// konstanty
const BYTE prName = 0;
const BYTE prUl1 = 1;
const BYTE prUl2 = 2;
const BYTE prKv1 = 3;
const BYTE prKv2 = 4;
const BYTE prBr1 = 5;
const BYTE prBr2 = 6;
const BYTE prDb1 = 7;
const BYTE prDb2 = 8;
const BYTE prBd1 = 9;
const BYTE prBd2 = 10;
const BYTE prKp1 = 11;
const BYTE prKp2 = 12;
const BYTE prEl1 = 13;
const BYTE prEl2 = 14;
const BYTE prReset = 15;
const BYTE prMgrFileNm = 15;
const BYTE prMgrProg = 16;
const BYTE prMgrParam = 17;
const BYTE prPageSizeNN = 16;
const BYTE prPageSizeTrail = 17;
const BYTE prLMarg = 18;
const BYTE prLMargTrail = 19;
const BYTE prUs11 = 20;
const BYTE prUs12 = 21;
const BYTE prUs21 = 22;
const BYTE prUs22 = 23;
const BYTE prUs31 = 24;
const BYTE prUs32 = 25;
const BYTE prLine72 = 26;
const BYTE prLine216 = 27;
const BYTE prDen60 = 28;
const BYTE  prDen120 = 29;
const BYTE prDen240 = 30;
const BYTE prColor = 31;
const BYTE prClose = 32;

const BYTE RMsgIdx = 0;
const BYTE BgiEgaVga = 1;
const BYTE BgiHerc = 2;
const BYTE ChrLittKam = 3;
const BYTE ChrTripKam = 4;
const BYTE Ega8x14K = 5;
const BYTE Vga8x16K = 6;
const BYTE Vga8x19K = 7;
const BYTE Ega8x14L = 8;
const BYTE Vga8x16L = 9;
const BYTE Vga8x19L = 10;
const BYTE ChrLittLat = 11;
const BYTE ChrTripLat = 12;
const BYTE LatToWinCp = 13;
const BYTE KamToWinCp = 14;
const BYTE WinCpToLat = 15;
