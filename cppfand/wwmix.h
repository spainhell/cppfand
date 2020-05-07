#pragma once
#include "constants.h"
#include "fileacc.h"
#include "pstring.h"


class wwmix
{
	
};

struct SS
{
	pstring* Pointto = nullptr; /*(nullptr)  at {ning point to this item*/
	bool Abcd = false; /*(false) alphabetical order in window*/
	bool AscDesc = false; /*(false) > ascending, < descending */
	bool Subset = false; /*(false)*/
	bool ImplAll = false; /*(false)  implic. the whole set */
	bool Empty = false; /* returned, test before calling SelectStr*/
	WORD Size = false; /* returned, subset size  after SelectStr */
	char Tag = false;  /* returned for each GetSelect */
} ss;

const BYTE SelMark = 0xF0;

struct Item
{
	Item* Chain;
	char Tag;
	pstring S;
};

struct
{
	Item* ItemRoot;
	void* markp;
	integer NItems, MaxItemLen, Tabs, TabSize, WwSize, Base, iItem;
} sv;

void PutSelect(pstring s); // r57
Item* GetItem(WORD N);
void SelectStr(integer C1, integer R1, WORD NMsg, pstring LowTxt);
void WriteItem(WORD N);
void SetAttr(WORD Attr);
void IVOn();
void IVOff();
void DisplWw();
void Right();
void Left();
void Down();
void Up();
void SetTag(char c);
void SetAllTags(char c);
void Switch(WORD I1, WORD I2);
void GraspAndMove(char schar);
void AbcdSort();
void SetFirstiItem();
bool MouseInItem(integer& I);
pstring GetSelect();
bool SelFieldList(WORD Nmsg, bool ImplAll, FieldList FLRoot);
pstring SelectDiskFile(pstring Path, WORD HdMsg, bool OnFace);
bool PromptFilter(pstring Txt, FrmlPtr Bool, pstring* BoolTxt);
void PromptLL(WORD N, pstring* Txt, WORD I, bool Del);
pstring PassWord(bool TwoTimes);
void SetPassWord(FileDPtr FD, WORD Nr, pstring Pw);
bool HasPassWord(FileDPtr FD, WORD Nr, pstring Pw);
