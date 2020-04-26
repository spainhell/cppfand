#pragma once
#include "constants.h"

bool TxtEdCtrlUBrk, TxtEdCtrlF4Brk;
WORD EditTxt(pstring* s, WORD pos, WORD maxlen, WORD maxcol, char typ, bool del,
    bool star, bool upd, bool ret, WORD Delta); // r86
