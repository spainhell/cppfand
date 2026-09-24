#pragma once
#include <string>

#include "FandXFile.h"

typedef void* HANDLE;

// Work files used for building indexes and sorting.
// The host application sets the names and opens them.

extern std::string FandWorkName;	// FANDWORK.$$$ - work pages of XWorkFile
extern std::string FandWorkXName;	// FANDWORK.X$$ - work indexes (XWork)
extern HANDLE WorkHandle;			// handle of FandWorkName
extern int MaxWSize;				// currently occupied in FANDWORK.$$$
extern FandXFile XWork;				// index file of work indexes (XWKey)
