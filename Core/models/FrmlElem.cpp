#include "FrmlElem.h"

FrmlElemFunction::FrmlElemFunction(instr_type Op, size_t buff_size): FrmlElem(Op, buff_size)
{
}

FrmlElem1::FrmlElem1(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemNumber::FrmlElemNumber(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemNumber::FrmlElemNumber(instr_type Op, size_t buff_size, double value) : FrmlElem(Op, buff_size)
{
	this->R = value;
}

FrmlElemString::FrmlElemString(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemString::FrmlElemString(instr_type Op, size_t buff_size, std::string& value) : FrmlElem(Op, buff_size)
{
	this->S = value;
}

FrmlElemBool::FrmlElemBool(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemBool::FrmlElemBool(instr_type Op, size_t buff_size, bool value) : FrmlElem(Op, buff_size)
{
	this->B = value;
}

FrmlElemDateMask::FrmlElemDateMask(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem7::FrmlElem7(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemNewFile::FrmlElemNewFile(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem9::FrmlElem9(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemCatalogField::FrmlElemCatalogField(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemPrompt::FrmlElemPrompt(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemPosReplace::FrmlElemPosReplace(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemRecNo::FrmlElemRecNo(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

void FrmlElemRecNo::SaveArgs(FrmlElem* arguments[], size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		auto ai = arguments[i];
		this->Arg.push_back(arguments[i]);
	}
}

FrmlElem14::FrmlElem14(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemLink::FrmlElemLink(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem16::FrmlElem16(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemLocVar::FrmlElemLocVar(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemLocVar::FrmlElemLocVar(instr_type Op, LocVar* lv) : FrmlElem(Op, 0)
{
	locvar = lv;
}

FrmlElemUserFunc::FrmlElemUserFunc(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem20::FrmlElem20(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemEval::FrmlElemEval(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem22::FrmlElem22(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemOwned::FrmlElemOwned(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemIn::FrmlElemIn(instr_type Op) : FrmlElem(Op, 0)
{
}

FrmlElemSum::FrmlElemSum(instr_type Op) : FrmlElem(Op, 0)
{
	
}

FrmlElemSum::FrmlElemSum(instr_type op, double r, FrmlElem* frml) : FrmlElem(op, 0)
{
	R = r;
	Frml = frml;
}


FrmlElemInp::FrmlElemInp(instr_type op, InpD* inp) : FrmlElem(op, 0)
{
	this->inp = inp;
}

FrmlElemMerge::FrmlElemMerge(instr_type op, MergOpSt* merge) : FrmlElem(op, 0)
{
	this->merge = merge;
}

FrmlElem* CopyFrmlElem(const FrmlElem* orig)
{
	if (orig == nullptr) return nullptr;
	return nullptr;
}
