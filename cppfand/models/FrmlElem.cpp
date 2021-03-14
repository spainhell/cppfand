#include "FrmlElem.h"

FrmlElem0::FrmlElem0(instr_type Op, size_t buff_size): FrmlElem(Op, buff_size)
{
}

FrmlElem1::FrmlElem1(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem2::FrmlElem2(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem2::FrmlElem2(instr_type Op, size_t buff_size, double value) : FrmlElem(Op, buff_size)
{
	this->R = value;
}

FrmlElem4::FrmlElem4(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem4::FrmlElem4(instr_type Op, size_t buff_size, pstring value) : FrmlElem(Op, buff_size)
{
	this->S = value;
}

FrmlElem4::FrmlElem4(instr_type Op, size_t buff_size, pstring* value) : FrmlElem(Op, buff_size)
{
	this->S = *value;
}

FrmlElem5::FrmlElem5(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem5::FrmlElem5(instr_type Op, size_t buff_size, bool value) : FrmlElem(Op, buff_size)
{
	this->B = value;
}

FrmlElem6::FrmlElem6(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem7::FrmlElem7(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem8::FrmlElem8(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem9::FrmlElem9(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem10::FrmlElem10(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem11::FrmlElem11(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem12::FrmlElem12(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem13::FrmlElem13(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

void FrmlElem13::SaveArgs(FrmlElem* arguments[], size_t count)
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

FrmlElem15::FrmlElem15(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem16::FrmlElem16(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem18::FrmlElem18(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem18::FrmlElem18(instr_type Op, LocVar* lv) : FrmlElem(Op, 0)
{
	locvar = lv;
}

FrmlElem19::FrmlElem19(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem20::FrmlElem20(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem21::FrmlElem21(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem22::FrmlElem22(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem23::FrmlElem23(instr_type Op, size_t buff_size) : FrmlElem(Op, buff_size)
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


FrmlElem* CopyFrmlElem(const FrmlElem* orig)
{
	if (orig == nullptr) return nullptr;
	return nullptr;
}
