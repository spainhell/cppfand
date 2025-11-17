#include "realFix.h"

#include <cstring>

const unsigned char DblS = 8;
unsigned char Dbl[DblS];

void FixFromReal(double r, void* FixNo, unsigned short FLen)
{
	unsigned char ff[9]{ 0 };
	unsigned char rr[9]{ 0 };
	// memcpy(ff + 1, FixNo, FLen); // zacneme na indexu 1 podle Pascal. kodu
	if (r > 0) r += 0.5;
	else r -= 0.5;
	std::memcpy(rr + 1, &r, 8);
	bool neg = ((rr[8] & 0x80) != 0); // zaporne cislo urcuje 1 bit
	short exp = (rr[8 - 1] >> 4) + (unsigned short)((rr[8] & 0x7F) << 4);
	if (exp < 2047)
	{
		rr[8] = 0;
		rr[8 - 1] = rr[8 - 1] & 0x0F;
		if (exp > 0) rr[8 - 1] = rr[8 - 1] | 0x10;
		else exp++;
		exp -= 1023;
		if (exp > (FLen << 3) - 1) return; // OVERFLOW
		int lef = (exp + 4) & 0x0007;
		int rig = 8 - lef;
		if ((exp & 0x0007) > 3) exp += 4;
		int first = 7 - (exp >> 3);
		int i = FLen;
		while ((first < 8) && (i > 0)) {
			ff[i] = (rr[first] >> rig) + (rr[first + 1] << lef);
			i--;
			first++;
		}
		if (i > 0) ff[i] = rr[first] >> rig;
		if (neg) {
			for (i = 1; i <= FLen; i++) ff[i] = ~ff[i];
			ff[FLen]++;
			i = FLen;
			while (ff[i] == 0) {
				i--;
				if (i > 0) ff[i]++;
			}
		}
	}
	memcpy(FixNo, &ff[1], FLen);
}

double RealFromFix(void* FixNo, unsigned short FLen)
{
	unsigned char ff[9]{ 0 };
	unsigned char rr[9]{ 0 };
	memcpy(ff + 1, FixNo, FLen); // zacneme na indexu 1 podle Pascal. kodu

	bool neg = ((ff[1] & 0x80) != 0); // zaporne cislo urcuje 1 bit
	if (neg) {
		if (ff[1] == 0x80) {
			for (size_t i = 2; i <= FLen; i++) {
				if (ff[i] != 0x00) break;
				return 0.0;
			}
		}
		for (size_t i = 1; i <= FLen; i++) ff[i] = ~ff[i];
		ff[FLen]++;
		unsigned short I = FLen;
		while (ff[I] == 0) {
			I--;
			if (I > 0) ff[I]++;
		}
	}
	short first = 1;
	while (ff[first] == 0) first++;
	if (first > FLen) return 0;

	short lef = 0;
	unsigned char b = ff[first];
	while ((b & 0x80) == 0) {
		b = b << 1;
		lef++;
	}
	ff[first] = ff[first] & (0x7F >> lef);
	short exp = ((FLen - first) << 3) - lef + 1030;
	if (lef == 7) first++;
	lef = (lef + 5) & 0x07;
	short rig = 8 - lef;
	short i = 8 - 1; // velikost double - 1
	if ((rig <= 4) && (first <= FLen)) {
		rr[i] = ff[first] >> rig;
		i--;
	}
	while ((i > 0) && (first < FLen)) {
		rr[i] = (ff[first] << lef) + (ff[first + 1] >> rig);
		i--;
		first++;
	}
	if ((first == FLen) && (i > 0)) {
		unsigned char t = ff[first] << lef;
		rr[i] = t;
	}
	rr[DblS - 1] = (rr[DblS - 1] & 0x0F) + ((exp & 0x0F) << 4);
	rr[DblS] = exp >> 4;
	if (neg) rr[DblS] = rr[DblS] | 0x80;
	return *(double*)&rr[1];
}
