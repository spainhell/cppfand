#pragma once
#include "constants.h"
#include "olongstr.h"
#include "pstring.h"
#include "runfrml.h"

struct SS
{
	std::string* Pointto = nullptr; /*(nullptr)  at {ning point to this item*/
	bool Abcd = false; /*(false) alphabetical order in window*/
	bool AscDesc = false; /*(false) > ascending, < descending */
	bool Subset = false; /*(false)*/
	bool ImplAll = false; /*(false)  implic. the whole set */
	bool Empty = false; /* returned, test before calling SelectStr*/
	WORD Size = false; /* returned, subset size  after SelectStr */
	char Tag = false;  /* returned for each GetSelect */
};
extern SS ss;

const BYTE SelMark = 0xF0;

class wwmix
{
public :
	wwmix();
	void PutSelect(std::string s); // r57
	void SelectStr(integer C1, integer R1, WORD NMsg, std::string LowTxt);
	pstring GetSelect();
	bool SelFieldList(WORD Nmsg, bool ImplAll, FieldList FLRoot);
	std::string SelectDiskFile(std::string Path, WORD HdMsg, bool OnFace);
	bool PromptFilter(std::string Txt, FrmlElem* Bool, std::string* BoolTxt);
	void PromptLL(WORD N, std::string* Txt, WORD I, bool Del);
	pstring PassWord(bool TwoTimes);
	void SetPassWord(FileD* FD, WORD Nr, pstring Pw);
	bool HasPassWord(FileD* FD, WORD Nr, pstring Pw);

private:
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
};
