#include "WRec.h"

const unsigned char equ = 0x1;
const unsigned char lt = 0x2;
const unsigned char gt = 0x4;

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

int WRec::GetN()
{
	// asm les di,Self; mov ax,es:[di]; mov dl,es:[di+2]; xor dh,dh ;
	return N[0] + (N[1] << 8) + (N[2] << 16);
}

void WRec::PutN(int NN)
{
	//asm { asm les di, Self; mov ax, NN.unsigned short; cld; stosw; mov al, NN[2].unsigned char; stosb; }
	N[0] = NN & 0xFF;
	N[1] = (NN >> 8) & 0xFF;
	N[2] = (NN >> 16) & 0xFF;
}

void WRec::PutIR(int II)
{
	// asm les di,Self; add di,3; mov ax,II.unsigned short; cld; stosw; mov al,II[2].unsigned char; stosb;
	IR[0] = II & 0xFF;
	IR[1] = (II >> 8) & 0xFF;
	IR[2] = (II >> 16) & 0xFF;
}

unsigned short WRec::Comp(WRec* R)
{
	//asm push ds; cld; lds si, Self; mov al, [si + 6]; add si, 7;
	//les di, R; mov ah, es: [di + 6] ; add di, 7;
	//xor ch, ch; mov cl, al; cmp ah, al; ja @1; mov cl, ah;
	//@jcxz  1@11; repe cmpsb; jb @2; ja @3;
	//@cmp al 11, ah; jb @2; ja @3;   /*compare X*/
	//mov si, Self.unsigned short; mov di, R.unsigned short; mov al, ds: [si + 5] ; cmp al, es: [di + 5] ; /*compare IR*/
	//jb @2; ja @3; mov ax, ds: [si + 3] ; cmp ax, es: [di + 3] ; jb @2; ja @3;
	//mov ax, 1; jmp @4;
	//@mov ax 2, 2; jmp @4;
	//@mov ax 3, 4;
	//@pop ds 4;

	unsigned short offThis = 0;
	unsigned short offR = 0;
	unsigned char lenThis = X.S[0]; // AL
	unsigned char lenR = R->X.S[0]; // AH
	if (lenThis != 0 && lenR != 0) {
		// porovnani retezcu VETSI delkou - nechapu ale proc ...
		for (size_t i = 0; i < std::max(lenThis, lenR); i++) {
			if (X.S[i + 1] == R->X.S[i + 1]) continue;
			if (X.S[i + 1] < R->X.S[i + 1]) return lt;
			else return gt;
		}
	}
	if (lenThis != lenR) { // compare X
		if (lenThis < lenR) return lt;
		else return gt;
	}

	int irThis = IR[0] + (IR[1] << 8) + (IR[2] << 16);
	int irR = R->IR[0] + (R->IR[1] << 8) + (R->IR[2] << 16);
	if (irThis != irR) { // compare IR
		if (irThis < irR) return lt;
		else return gt;
	}

	return equ;
}

unsigned short WRec::Compare(const WRec& w) const
{
	unsigned short offThis = 0;
	unsigned short offR = 0;
	unsigned char lenThis = X.S.length(); // AL
	unsigned char lenR = w.X.S.length(); // AH
	if (lenThis != 0 && lenR != 0) {
		// porovnani retezcu VETSI delkou - nechapu ale proc ...
		for (unsigned char i = 0; i < std::min(lenThis, lenR); i++) {
			if (X.S.at(i + 1) == w.X.S.at(i + 1)) continue;
			if (X.S.at(i + 1) < w.X.S.at(i + 1)) return lt;
			return gt;
		}
	}
	if (lenThis != lenR) { // compare X
		if (lenThis < lenR) return lt;
		return gt;
	}

	int irThis = IR[0] + (IR[1] << 8) + (IR[2] << 16);
	int irR = w.IR[0] + (w.IR[1] << 8) + (w.IR[2] << 16);
	if (irThis != irR) { // compare IR
		if (irThis < irR) return lt;
		else return gt;
	}

	return equ;
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
	return Compare(w) == equ;
}

bool WRec::operator<(const WRec& w) const
{
	return Compare(w) == lt;
}

bool WRec::operator>(const WRec& w) const
{
	return Compare(w) == gt;
}