#pragma once

class WPage /* ca. 64k pages in work file */
{
public:
	int NxtChain = 0;
	int Chain = 0;
	unsigned short NRecs = 0;
	unsigned char A[65535]{ 0 };
	void Sort(unsigned short N, unsigned short RecLen);
};