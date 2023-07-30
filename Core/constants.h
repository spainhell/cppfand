#pragma once
#include "../Common/typeDef.h"

const BYTE FloppyDrives = 2;

// *** BASE.H ***
int const UserLicNrShow = 999001; // 160188
const WORD FDVersion = 0x0411;
const WORD ResVersion = 0x0420;
const char CfgVersion[] = { '4', '.', '2', '0', '\0' };
const BYTE DMLVersion = 41;
const bool HasCoproc = true;

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
const BYTE prDen120 = 29;
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


enum instr_type
{
	_notdefined = 0x0,
	_equ = 0x1, _lt = 0x2, _le = 0x3,
	_gt = 0x4, _ge = 0x5, _ne = 0x6,
	_subrange = 0x7, _number = 0x8, _assign = 0x9, _identifier = 0xA,
	_addass = 0xB, _quotedstr = 0xC,
	_const = 0x10, //{float/string/boolean},             {0-ary instructions}
	_field = 0x11, _getlocvar = 0x12, // {fieldD}, {BPOfs},
	_access = 0x13, // {fieldD or nil for exist,newfileD,linkD or nil},
	_recvarfld = 0x14, // {fieldD,fileD,recptr}
	_today = 0x18, _currtime = 0x19, _pi = 0x1A, _random = 0x1B,
	_exitcode = 0x1d, _edrecno = 0x1e, _getWORDvar = 0x1f, // {n:0..}
	_memavail = 0x22, _maxcol = 0x23, _maxrow = 0x24,
	_getmaxx = 0x25, _getmaxy = 0x26,
	_lastupdate = 0x27, _nrecs = 0x28, _nrecsabs = 0x29, // {FD}
	_generation = 0x2a, // {FD}
	_recno = 0x2b, _recnoabs = 0x2c, _recnolog = 0x2d,  // {FD,K,Z1,Z2,...}
	_filesize = 0x2e,  // {txtpath,txtcatirec}
	_txtpos = 0x2f, _cprinter = 0x30, _mousex = 0x31, _mousey = 0x32,
	_txtxy = 0x33, _indexnrecs = 0x34, _owned = 0x35,  // {bool,sum,ld}           // {R}
	_catfield = 0x36, // {CatIRec,CatFld}
	_password = 0x37, _version = 0x38, _username = 0x39, _edfield = 0x3a,
	_accright = 0x3b, _readkey = 0x3c, _edreckey = 0x3d, _edbool = 0x3e,
	_edfile = 0x3f, _edkey = 0x40, _clipbd = 0x41, _keybuf = 0x42,
	_keyof = 0x43,  // {LV,XKey}                                             // {S}
	_edupdated = 0x44, _keypressed = 0x45, _escprompt = 0x46,
	_trust = 0x47, _lvdeleted = 0x48,  // {bytestring}_lvdeleted=#$48,   // {LV}                      // {B}
	_userfunc = 0x49,
	_isnewrec = 0x4a, _mouseevent = 0x4b, _ismouse = 0x4c,
	_testmode = 0x4d,
	_count = 0x4f, // count of records in file in IDA (FrmlElemInp)
	_mergegroup = 0x50, // for (FrmlElemMerge)
	_newfile = 0x60,  // {newfile,newRP},                      // {1-ary instructions}
	_lneg = 0x61,
	_inreal = 0x62, _instr = 0x63,  // {precision,constlst}, {tilda,constlst},
	_isdeleted = 0x64, _setmybp = 0x65,  // {RecFD}
	_modulo = 0x66,  // {length,modulo,weight1,...},                          // {B}
	_getpath = 0x68, _upcase = 0x69, _lowcase = 0x6A,
	_leadchar = 0x6B, _getenv = 0x6C,
	_trailchar = 0x6D, _strdate1 = 0x6E,  // {char}  // {maskstring},
	_nodiakr = 0x6F,  // {S}
	_char = 0x70, _sqlfun = 0x71,  // {SR}
	_unminus = 0x73, _abs = 0x74, _int = 0x75, _frac = 0x76, _sqr = 0x77,
	_sqrt = 0x78, _sin = 0x79, _cos = 0x7A, _arctan = 0x7b, _ln = 0x7c,
	_exp = 0x7d, _typeday = 0x7e, _color = 0x7f,
	_link = 0x90, // {LD}, // {R}
	_val = 0x91, _valdate = 0x92, _length = 0x93, _linecnt = 0x94, // {maskstring}
	_diskfree = 0x95, _ord = 0x96, _eval = 0x97,  // {Typ},  // {RS}
	_accrecno = 0x98,  // {FD,FldD},    // {R,S,B}
	_promptyn = 0x99,  // {BS}
	_conv = 0xa0,   // { used in Prolog}
	_and = 0xb1, _or = 0xb2, _limpl = 0xb3, _lequ = 0xb4,  // {2-ary instructions}
	_compreal = 0xb5, _compstr = 0xb6,    // {compop,precision}  // {compop,tilda},     // {B}
	_concat = 0xc0, // {S}
	_repeatstr = 0xc2, // {SSR}
	_gettxt = 0xc3, // {txtpath,txtcatirec}       // {SRR}
	_plus = 0xc4, _minus = 0xc5, _times = 0xc6, _divide = 0xc7,
	_div = 0xc8, _mod = 0xc9, _round = 0xca,
	_addwdays = 0xcb, _difwdays = 0xcc, // {typday}
	_addmonth = 0xcd, _difmonth = 0xce, _inttsr = 0xcf, // {ptr},
	_min = 0xd0, _max = 0xd1, // {used in Prolog}     // {R}
	_equmask = 0xd2,   // {BSS}
	_prompt = 0xd3, // {fieldD},   // {R,S,B}
	_portin = 0xd4, // {RBR}
	_cond = 0xf0, // {bool or nil,frml,continue or nil},    // {3-ary instructions}
	_copy = 0xf1, _str = 0xf2, // {S}
	_selectstr = 0xf3, _copyline = 0xf4,  // {SSRR}
	_pos = 0xf5, _replace = 0xf6, // {options}, // {options},   // {RSSR}
	_mousein = 0xf7,  // {P4},
};