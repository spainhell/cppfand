#pragma once
#include  "../cppfand/constants.h"

class WPage /* ca. 64k pages in work file */
{
public:
	longint NxtChain = 0;
	longint Chain = 0;
	WORD NRecs = 0;
	BYTE A[65535]{ 0 };
	void Sort(WORD N, WORD RecLen);
};