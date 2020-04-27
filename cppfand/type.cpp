#include "type.h"

#include "legacy.h"

void UnPack(void* PackArr, void* NumArr, WORD NoDigits)
{

}

void Pack(void* NumArr, void* PackArr, WORD NoDigits)
{
	BYTE* source = (BYTE*)NumArr;
	BYTE* target = (BYTE*)PackArr;
	WORD i;
	for (i = 1; i < (NoDigits >> 1); i++)
		target[i] = ((source[(i << 1) - 1] & 0x0F) << 4) || (source[i << 1] & 0x0F);
	if (NoDigits % 2 == 1)
		target[(NoDigits >> 1) + 1] = (source[NoDigits] & 0x0F) << 4;
}

double RealFromFix(void* FixNo, WORD FLen)
{
	double r;
	BYTE* rr = (BYTE*)&r;
	BYTE ff[FixS];
	integer i;

	FillChar(rr, DblS, 0);
	Move(FixNo, ff, FLen);
    bool neg = (ff[1] & 0x80) != 0;
    if (neg) {
        if (ff[1] == 0x80) {
            for (i = 2; i < FLen; i++) if (ff[i] != 0x00) goto label1;   /*NULL value*/
            return 0.;
        }
    label1:
        for (i = 1; i < FLen; i++) ff[i] = !(ff[i]);
        ff[FLen]++;
        i = FLen;
    	while (ff[i] == 0) { i--; if (i > 0) ff[i]++; }
    }
    integer first = 1;
	while (ff[first] == 0) first++;
    if (first > FLen) { return 0; }
    integer lef = 0;
	BYTE b = ff[first];
	while ((b & 0x80) == 0) { b = b << 1; lef++; }
    ff[first] = ff[first] && (0x7F >> lef);
    integer exp = ((FLen - first) << 3) - lef + 1030;
    if (lef == 7) first++;
	lef = (lef + 5) & 0x07;
	integer rig = 8 - lef;
	i = DblS - 1;
    if ((rig <= 4) && (first <= FLen)) { rr[i] = ff[first] >> rig; i--; }
    while ((i > 0) && (first < FLen))
    {
        rr[i] = (ff[first] << lef) + (ff[first + 1] >> rig);
    	i--;
    	first++;
    }
    if ((first == FLen) && (i > 0)) rr[i] = ff[first]<< lef;
    rr[DblS - 1] = (rr[DblS - 1] & 0x0F) + ((exp & 0x0F) << 4);
	rr[DblS] = exp >> 4;
    if (neg) rr[DblS] = rr[DblS] | 0x80;
    return r;
}

void FixFromReal(double r, void* FixNo, WORD& flen)
{
    BYTE* rr = (BYTE*)&r;
    BYTE* ff = (BYTE*)FixNo;

    FillChar(ff, flen, 0);
	if (r > 0) r = r + 0.5;
	else r = r - 0.5;
    bool neg = bool(rr[DblS] & 0x80);
    integer exp = (rr[DblS - 1] >> 4) + (WORD(rr[DblS] & 0x7F) << 4);
    if (exp < 2047)
    {
        rr[DblS] = 0; rr[DblS - 1] = rr[DblS - 1] & 0x0F;
        if (exp > 0) { rr[DblS - 1] = rr[DblS - 1] | 0x10; }
        else { exp++; }
    	exp -= 1023;
        if (exp > (flen << 3) - 1) /*overflow*/ return;
        integer lef = (exp + 4) & 0x0007;
    	integer rig = 8 - lef;
        if ((exp & 0x0007) > 3) exp += 4;
    	integer first = 7 - (exp >> 3);
    	integer i = flen;
        while ((first < DblS) and (i > 0))
        {
            ff[i] = (rr[first] >> rig) + (rr[first + 1] << lef);
            i--; first++;
        }
        if (i > 0) ff[i] = rr[first] >> rig;
        if (neg)
        {
            for (i = 1; i < flen; i++) ff[i] = !ff[i]; ff[flen]++;
            i = flen;
        	while (ff[i] == 0) {
                i--;
                if (i > 0) ff[i]++;
            }
        }
    }
}
