#pragma once

//{$I edevinpt.pas}
//{$I edevproc.pas}

#include "ededit.h"

#include <string>

using namespace std;

typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef int longint;
typedef string* LongStrPtr;

typedef void* EdExitDPtr;

void HandleEvent();
