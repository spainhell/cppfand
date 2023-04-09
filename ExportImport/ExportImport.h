#pragma once
#include "CodingRdb.h"
#include "../cppfand/constants.h"
#include "../cppfand/rdrun.h"
#include "../cppfand/models/Instr.h"


bool OldToNewCat(int& FilSz);

void FileCopy(CopyD* CD);
void MakeMerge(CopyD* CD);

void Backup(bool IsBackup, bool NoCompress, WORD Ir, bool NoCancel);
void BackupM(Instr_backup* PD);

void CheckFile(FileD* FD);

void CodingCRdb(bool Rotate);


bool PromptCodeRdb();

void XEncode(LongStr* S1, LongStr* S2)
{
	/*unsigned char Flags, Mask, RMask, t;
	unsigned short FlagPos, SequPos, SequLen, Len, NewLen, Displ;
	unsigned char* s1 = static_cast<unsigned char*>(S1);
	unsigned char* s2 = static_cast<unsigned char*>(S2) + 2;
	unsigned short bx = s1[0] + (s1[1] << 8);

	t = Timer.byte;
	t &= 0x03;
	RMask = 0x9c;
	RMask = (RMask << t) | (RMask >> (8 - t));

	Len = bx;
	Displ = 0;

	while (true) {
		FlagPos = s2 - static_cast<unsigned char*>(S2) - 1;
		Mask = 1;
		Flags = 0;

		while (Len != 0) {
			unsigned char* si = s1;
			unsigned char* di = s2;
			SequLen = 0;

			while (true) {
				unsigned short cx = si - s1;
				cx = Len < cx ? Len : cx;

				if (cx >= 2) {
					const auto cmpres = std::memcmp(si, di, cx);
					if (cmpres == 0) {
						si += cx;
						di += cx;
						Len -= cx;
						continue;
					}
					++cx;
				}
				si = s1 + (di - s2) + SequLen;
				unsigned short dx = si - s1;
				dx = Len < dx ? Len : dx;

				if (dx > SequLen && dx <= 255) {
					SequPos = *di;
					SequLen = dx;
				}
				break;
			}

			if (SequLen < 2) {
				unsigned char val = *si++;
				RMask = (RMask << 1) | (RMask >> 7);
				val ^= RMask;
				*di++ = val;
				--Len;
			}
			else {
				*s2++ = SequLen;
				*(unsigned short*)s2 = static_cast<unsigned short>(SequPos - s1);
				s2 += 2;
				Flags |= Mask;
				Len -= SequLen;
				s1 += SequLen;
			}

			Mask <<= 1;
			if (Mask == 0) {
				*reinterpret_cast<unsigned char*>(S2) = Flags;
				s2 = static_cast<unsigned char*>(S2) + FlagPos + 1;
				Mask = 1;
				Flags = 0;
			}
		}

		if (Mask == 1) {
			--s2;
		}
		else {
			*reinterpret_cast<unsigned char*>(S2) = Flags;
		}

		s2 = static_cast<unsigned char*>(S2) + 2;
		NewLen = s2 - static_cast<unsigned char*>(S2) + Displ - 3;
		*reinterpret_cast<unsigned short*>(S2) = NewLen;
		++s2;
		unsigned char* si = s2 + NewLen - 1;
		*si-- = t;
		*reinterpret_cast<unsigned short*>(si - 1) = Displ ^ 0xcccc;
		break;
	}*/
}