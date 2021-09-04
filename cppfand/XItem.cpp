#include "XItem.h"

XItem::XItem(BYTE* data, bool isLeaf)
{
	Nr = data;
	if (isLeaf) {
		DownPage = (longint*)data;
		XPageData = &data[4];
	}
	else {
		DownPage = (longint*)&data[3];
		XPageData = &data[8];
	}
}

longint XItem::GetN()
{
	return *Nr & 0x00ffffff;
}

void XItem::PutN(longint N)
{
	// asm les bx,Self; mov ax,N.word; mov es:[bx],ax; mov al,N[2].byte;
	// mov es : [bx + 2] , al
	memcpy(Nr, &N, 3); // kopirujeme 3 nejnizsi Byty, posledni se ignoruje
}

WORD XItem::GetM(WORD O)
{
	// asm les bx,Self; add bx,O; xor ah,ah; mov al,es:[bx]
	return Nr[O];
}

void XItem::PutM(WORD O, WORD M)
{
	// asm les bx,Self; add bx,O; mov ax,M; mov es:[bx],al
}

WORD XItem::GetL(WORD O)
{
	// asm les bx,Self; add bx,O; xor ah,ah; mov al,es:[bx+1]
	return 0;
}

void XItem::PutL(WORD O, WORD L)
{
	// asm les bx,Self; add bx,O; mov ax,L; mov es:[bx+1],al
}

XItem* XItem::Next(WORD O, bool isLeaf)
{
	// O by melo byt 3 nebo 7 - urcuje delku "hlavicky" zaznamu
	unsigned char recLen = XPageData[0];
	// dalsi zaznam zacina hned za daty o delce recLen
	auto xi = new XItem(&XPageData[recLen + 1], isLeaf);
	return xi;
}

WORD XItem::UpdStr(WORD O, pstring* S)
{
	/*asm  push ds; lds bx,Self; les di,S; cld; add bx,O;
	 mov al,[bx];{M} add al,[bx+1];{L} stosb;
	 mov al,[bx]; xor ah,ah; add di,ax; lea si,[bx+2];
	 xor ch,ch; mov cl,[bx+1]; rep movsb; mov ax,si; pop ds;*/

	 // Nr[0] je ukazatel i na vsechno dalsi
	 // 'O' je index - offset
	 // delka S bude M + L
	BYTE M = Nr[O];
	BYTE L = Nr[O + 1];
	(*S)[0] = M + L; // nova delka retezce
	memcpy(&(*S)[M + 1], &Nr[O + 2], L);
	return O + L + 2;
}

size_t XItem::size(bool isLeaf)
{
	return (isLeaf ? 3 : 7) + 2 + XPageData[0]; // 2 - L + M
}