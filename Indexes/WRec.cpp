#include "WRec.h"

WRec::WRec(unsigned char* data)
{
	this->Deserialize(data);
}

WRec::WRec(WPage* wp)
{
	N[0] = wp->A[0]; N[1] = wp->A[1]; N[2] = wp->A[2];
	IR[0] = wp->A[3]; IR[1] = wp->A[4]; IR[2] = wp->A[5];
	X.S[0] = wp->A[6];
	memcpy(&X.S[1], &wp->A[7], X.S[0]);
}

longint WRec::GetN()
{
	// asm les di,Self; mov ax,es:[di]; mov dl,es:[di+2]; xor dh,dh ;
	return N[0] + (N[1] << 8) + (N[2] << 16);
}

void WRec::PutN(longint NN)
{
	//asm { asm les di, Self; mov ax, NN.WORD; cld; stosw; mov al, NN[2].BYTE; stosb; }
	N[0] = NN & 0xFF;
	N[1] = (NN >> 8) & 0xFF;
	N[2] = (NN >> 16) & 0xFF;
}

void WRec::PutIR(longint II)
{
	// asm les di,Self; add di,3; mov ax,II.WORD; cld; stosw; mov al,II[2].BYTE; stosb;
	IR[0] = II & 0xFF;
	IR[1] = (II >> 8) & 0xFF;
	IR[2] = (II >> 16) & 0xFF;
}

WORD WRec::Comp(WRec* R)
{
	//asm push ds; cld; lds si, Self; mov al, [si + 6]; add si, 7;
	//les di, R; mov ah, es: [di + 6] ; add di, 7;
	//xor ch, ch; mov cl, al; cmp ah, al; ja @1; mov cl, ah;
	//@jcxz  1@11; repe cmpsb; jb @2; ja @3;
	//@cmp al 11, ah; jb @2; ja @3;   /*compare X*/
	//mov si, Self.WORD; mov di, R.WORD; mov al, ds: [si + 5] ; cmp al, es: [di + 5] ; /*compare IR*/
	//jb @2; ja @3; mov ax, ds: [si + 3] ; cmp ax, es: [di + 3] ; jb @2; ja @3;
	//mov ax, 1; jmp @4;
	//@mov ax 2, 2; jmp @4;
	//@mov ax 3, 4;
	//@pop ds 4;

	WORD offThis = 0;
	WORD offR = 0;
	BYTE lenThis = X.S[0]; // AL
	BYTE lenR = R->X.S[0]; // AH
	if (lenThis != 0 && lenR != 0) {
		// porovnani retezcu VETSI delkou - nechapu ale proc ...
		for (size_t i = 0; i < std::max(lenThis, lenR); i++) {
			if (X.S[i + 1] == R->X.S[i + 1]) continue;
			if (X.S[i + 1] < R->X.S[i + 1]) return _lt;
			else return _gt;
		}
	}
	if (lenThis != lenR) { // compare X
		if (lenThis < lenR) return _lt;
		else return _gt;
	}

	int irThis = IR[0] + (IR[1] << 8) + (IR[2] << 16);
	int irR = R->IR[0] + (R->IR[1] << 8) + (R->IR[2] << 16);
	if (irThis != irR) { // compare IR
		if (irThis < irR) return _lt;
		else return _gt;
	}

	return _equ;
}

WORD WRec::Compare(const WRec& w) const
{
	WORD offThis = 0;
	WORD offR = 0;
	BYTE lenThis = X.S.length(); // AL
	BYTE lenR = w.X.S.length(); // AH
	if (lenThis != 0 && lenR != 0) {
		// porovnani retezcu VETSI delkou - nechapu ale proc ...
		for (unsigned char i = 0; i < std::min(lenThis, lenR); i++) {
			if (X.S.at(i + 1) == w.X.S.at(i + 1)) continue;
			if (X.S.at(i + 1) < w.X.S.at(i + 1)) return _lt;
			return _gt;
		}
	}
	if (lenThis != lenR) { // compare X
		if (lenThis < lenR) return _lt;
		return _gt;
	}

	int irThis = IR[0] + (IR[1] << 8) + (IR[2] << 16);
	int irR = w.IR[0] + (w.IR[1] << 8) + (w.IR[2] << 16);
	if (irThis != irR) { // compare IR
		if (irThis < irR) return _lt;
		else return _gt;
	}

	return _equ;
}

void WRec::Deserialize(unsigned char* data)
{
	memcpy(N, &data[0], 3);
	memcpy(IR, &data[3], 3);
	size_t len = data[6];
	memcpy(&X.S[0], &data[6], len + 1);
}

size_t WRec::Serialize(unsigned char* buffer)
{
	memcpy(&buffer[0], N, 3);
	memcpy(&buffer[3], IR, 3);
	size_t len = X.S[0];
	memcpy(&buffer[6], &X.S[0], len + 1);
	return 3 + 3 + 1 + len; // N + IR + S[0] + 1 B delka
}

bool WRec::operator==(const WRec& w) const
{
	return Compare(w) == 1;
}

bool WRec::operator<(const WRec& w) const
{
	return Compare(w) == 2;
}

bool WRec::operator>(const WRec& w) const
{
	return Compare(w) == 4;
}