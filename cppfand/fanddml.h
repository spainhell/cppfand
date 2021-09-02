#pragma once
#include <string>
#include "constants.h"

using namespace std;

//struct KeyFldD {
//	KeyFldD* Chain;
//	ItemPtr FldD;
//	bool CompLex;
//};
//typedef KeyFldD* KeyFldDPtr;
//
//struct XKey {
//	XKey* KeyDPtr;
//	KeyFldDPtr KFlds;
//	bool Intervaltest, Duplic;
//	WORD IndexRoot;
//	BYTE IndexLen;
//	string Alias;
//};
//typedef XKey* KeyDPtr;

//
//
//
//
//typedef string ScreenStr;
//typedef void* FrmlPtr;
//
//enum FileUseMode { Closed, RdOnly, RdShared, Shared, Exclusive };
//enum LockMode {	NullMode, NoExclMode, NoDelMode, NoCrMode, RdMode,WrMode, CrMode, DelMode, ExclMode };
//
//struct _FileDescr
//{
//	_FileDescr* Chain;
//	char Typ, FrmlTyp; // {Typ = 'F', 'A', 'N', 'D', 'R', 'T', 'B'  FrmlTyp = 'S', 'R', 'B'}
//	BYTE L, M, NBytes, Flgs; // {NBytes = length in record
//		// M digits after decimal point / Typ = 'F,n.m' /
//		// 1 if left(L), 0 if right(R) / Typ = 'A', 'N' /
//		// L = length in data editor }
//	bool stored;
//	integer Displ; // { displacement in record }
//			/*{ ..Name }*/
//	FrmlPtr Frml; //{formula for computing the item / #C / }
//	string Name; //{after last character of the name
//		//follows the mask string / Typ = 'D' / }
//};
//typedef _FileDescr* ItemPtr;
//
//struct _FileD
//{
//	_FileD* Chain;
//	WORD RecLen;
//	void* RecPtr;
//	longint NRecs;
//	bool WasWrRec, WasRdOnly, eof;
//	char Typ;  // { 8=Fand 86=Fand 160=RDB C=CAT }
//	WORD Handle;
//	longint IRec;
//	WORD FrstDispl;
//	void* TF;
//	BYTE ChptPos[6];
//	WORD TxtPosUDLI;
//	_FileD* OrigFD;  // { like orig. or nil }
//	BYTE Drive; // { 1 = A ,2 = B ,else 0 }
//	WORD CatIRec;
//	_FileDescr* FldD;
//	bool IsParFile, IsJournal, IsHlpFile, typSQLFile, IsSQLFile, IsDynFile;
//	FileUseMode UMode;
//	LockMode LMode, ExLMode, TaLMode;
//	void* ViewNames; void* XF;
//	void* Keys;
//	void* Add;
//	WORD nLDs, DliOfs;
//	string Name;
//};
//typedef _FileD* FilePtr;
//
///// ukazatel: RdbD*
//struct _RdbD
//{
//	_RdbD* ChainBack;
//	_FileD* FD;
//	_FileD* HelpFD;
//	void* X1;
//	void* X2;
//	bool B;
//	string Dir; // TODO: pùvodnì DirStr - ø. 63
//};
//typedef _RdbD* RdbD*;
