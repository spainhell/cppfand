#pragma once
#include "editor.h"
#include "rdfrml1.h"

struct Instr
{
	Instr* Chain;
	PInstrCode Kind;
	FrmlPtr HdLine;
	RdbDPtr HelpRdb;
	bool WasESCBranch;
	InstrPtr ESCInstr;
	ChoiceDPtr Choices;
	bool Loop, PullDown, Shdw;
	FrmlPtr X, Y, XSz;
	FrmlPtr mAttr[4];
	FrmlPtr Bool;
	InstrPtr Instr1, ElseInstr1;  // pùvodnì Instr a ElseInstr -> konflikt názvù
	RdbPos Pos;
	RdbPos PPos;
	BYTE N;
	bool ExPar;
	TypAndFrml TArg[2];
	RdbPos lpPos;
	pstring* lpName;
	pstring* RdbNm;
	pstring* ProcNm;
	InstrPtr ProcCall;
	pstring* ProgPath;
	WORD ProgCatIRec;
	bool NoCancel, FreeMm, LdFont, TextMd;
	FrmlPtr Param;
	CopyDPtr CD;
	BYTE LF /*0-write,1-writeln,2-message,3-message+help*/;
	WrLnD WD;
	RdbDPtr mHlpRdb;
	FrmlPtr mHlpFrml;
	FrmlPtr GoX, GoY;
	FrmlPtr Frml;
	bool Add;
	LocVarPtr AssLV;
	FrmlPtr Frml0;
	RdbDPtr HelpRdb0;
	FrmlPtr Frml1;
	bool Add1;
	FileDPtr FD;
	FieldDPtr FldD;
	FrmlPtr RecFrml;
	bool Indexarg;
	FrmlPtr Frml2; bool Add2; LocVarPtr AssLV2; FieldDPtr RecFldD;
	FrmlPtr Frml3; FileDPtr FD3; WORD CatIRec; FieldDPtr CatFld;
	LocVarPtr RecLV1, RecLV2;
	AssignDPtr Ass;
	LinkDPtr LinkLD;
	WKeyDPtr xnrIdx;
	FrmlPtr RecNr; bool AdUpd;
	LocVarPtr LV; bool ByKey; KeyDPtr Key;
	char CompOp;
	FileDPtr RecFD;
	FileDPtr NextGenFD;
	WORD FrstCatIRec, NCatIRecs; FrmlPtr TCFrml;
	FileDPtr SortFD; KeyFldDPtr SK;
	FileDPtr EditFD; EditOptPtr EO;
	RprtOptPtr RO;
	pstring* TxtPath; WORD TxtCatIRec; LocVarPtr TxtLV;
	char EdTxtMode; EdExitDPtr ExD;
	BYTE WFlags; FrmlPtr TxtPos, TxtXY, ErrMsg;
	WRectFrml Ww; FrmlPtr Atr; FrmlPtr Hd;
	FrmlPtr Head, Last, CtrlLast, AltLast, ShiftLast;
	pstring* TxtPath1; WORD TxtCatIRec1;
	FrmlPtr Txt; bool App;
	FrmlPtr Drive;
	WORD MountCatIRec; bool MountNoCancel;
	FileDPtr IndexFD; bool Compress;
	LocVarPtr giLV; char giMode; /*+,-,blank*/
	FrmlPtr giCond; /* || RecNr-Frml */
	KeyDPtr giKD; KeyFldDPtr giKFlds;
	KeyInDPtr giKIRoot; bool giSQLFilter;
	char giOwnerTyp; LinkDPtr giLD; LocVarPtr giLV2;
	WRectFrml W; FrmlPtr Attr; InstrPtr WwInstr; FrmlPtr Top;
	BYTE WithWFlags;
	WRectFrml W2; FrmlPtr Attr2, FillC;
	FileDPtr CFD; KeyDPtr CKey; LocVarPtr CVar, CRecVar;
	KeyInDPtr CKIRoot; FrmlPtr CBool/*or SQLTxt*/; InstrPtr CInstr;
	LinkDPtr CLD; bool CWIdx, inSQL, CSQLFilter, CProcent;
	char COwnerTyp; LocVarPtr CLV;
	InstrPtr WDoInstr, WElseInstr; bool WasElse; LockD WLD;
	GraphDPtr GD;
	FrmlPtr Par1, Par2, Par3, Par4, Par5, Par6, Par7, Par8, Par9, Par10, Par11;
	WORD BrCatIRec; bool IsBackup, NoCompress, BrNoCancel;
	BYTE bmX[5];
	FrmlPtr bmDir; FrmlPtr bmMasks; /*backup only*/
	bool bmSubDir, bmOverwr;
	FileDPtr clFD;
	FrmlPtr Insert, Indent, Wrap, Just, ColBlk, Left, Right;
	FrmlPtr MouseX, MouseY, Show;
	FileDPtr cfFD; pstring* cfPath; WORD cfCatIRec;
	FrmlPtr liName, liPassWord;
	pstring* TxtPath2; WORD TxtCatIRec2; bool IsRead;
	FileDPtr sqlFD; KeyDPtr sqlKey; FieldDPtr sqlFldD; FrmlPtr sqlXStr;
	FrmlPtr IsWord, Port, PortWhat;
};
