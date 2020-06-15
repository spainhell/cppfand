#pragma once
class Instr;
class FrmlElem;
struct RdbD;
struct ChoiceD;

class Instr_menubox_menubar : public Instr 
{
public:
	FrmlElem* HdLine = nullptr;
	RdbD* HelpRdb = nullptr;
	bool WasESCBranch = false;
	Instr* ESCInstr = nullptr;
	ChoiceD* Choices = nullptr;
	bool Loop = false, PullDown = false, Shdw = false;
	FrmlElem* X = nullptr; FrmlElem* Y = nullptr; FrmlElem* XSz = nullptr;
	FrmlElem* mAttr[4]{ nullptr };
};

class Instr_ifthenelseP_whiledo_repeatuntil : public Instr
{
public:
	FrmlElem* Bool = nullptr;
	Instr* Instr1 = nullptr;
	Instr* ElseInstr1 = nullptr;  // pùvodnì Instr a ElseInstr -> konflikt názvù
};

class Instr_merge_display : public Instr
{
public:
	RdbPos Pos;
};

