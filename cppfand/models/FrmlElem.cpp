#include "FrmlElem.h"

FrmlElem0::FrmlElem0(BYTE Op, size_t buff_size): FrmlElem(Op, buff_size)
{
}

FrmlElem1::FrmlElem1(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem2::FrmlElem2(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem2::FrmlElem2(BYTE Op, size_t buff_size, double value) : FrmlElem(Op, buff_size)
{
	this->R = value;
}

FrmlElem4::FrmlElem4(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem4::FrmlElem4(BYTE Op, size_t buff_size, pstring value) : FrmlElem(Op, buff_size)
{
	this->S = value;
}

FrmlElem4::FrmlElem4(BYTE Op, size_t buff_size, pstring* value) : FrmlElem(Op, buff_size)
{
	this->S = *value;
}

FrmlElem5::FrmlElem5(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem5::FrmlElem5(BYTE Op, size_t buff_size, bool value) : FrmlElem(Op, buff_size)
{
	this->B = value;
}

FrmlElem6::FrmlElem6(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem7::FrmlElem7(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem8::FrmlElem8(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem9::FrmlElem9(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem10::FrmlElem10(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem11::FrmlElem11(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem12::FrmlElem12(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem13::FrmlElem13(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
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

FrmlElem14::FrmlElem14(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem15::FrmlElem15(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem16::FrmlElem16(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem18::FrmlElem18(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem18::FrmlElem18(BYTE Op, LocVar* lv) : FrmlElem(Op, 0)
{
	locvar = lv;
}

FrmlElem19::FrmlElem19(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem20::FrmlElem20(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem21::FrmlElem21(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem22::FrmlElem22(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElem23::FrmlElem23(BYTE Op, size_t buff_size) : FrmlElem(Op, buff_size)
{
}

FrmlElemIn::FrmlElemIn(BYTE Op) : FrmlElem(Op, 0)
{
}

FrmlElem* CopyFrmlElem(const FrmlElem* orig)
{
	if (orig == nullptr) return nullptr;
	return nullptr;
}
