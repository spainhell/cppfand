#pragma once
#include "constants.h"
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
	void SelectStr(short C1, short R1, WORD NMsg, std::string LowTxt);
	std::string GetSelect();
	bool SelFieldList(WORD Nmsg, bool ImplAll, FieldListEl** FLRoot);
	bool SelFieldList(WORD Nmsg, bool ImplAll, std::vector<FieldDescr*>& FLRoot);
	std::string SelectDiskFile(std::string Path, WORD HdMsg, bool OnFace);
	bool PromptFilter(std::string Txt, FrmlElem** Bool, std::string* BoolTxt);
	void PromptLL(WORD N, std::string& Txt, WORD I, bool Del);
	std::string PassWord(bool TwoTimes);

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
	bool MouseInItem(short& I);
	size_t lastItemIndex = 0;
};
