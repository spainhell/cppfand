#pragma once
#include "../Common/typeDef.h"
#include "../Common/pstring.h"

const WORD NoDayInMonth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; // aby mesice byly 1-12

double RDate(WORD Y, WORD M, WORD D, WORD hh, WORD mm, WORD ss, WORD tt);
void SplitDate(double R, WORD& d, WORD& m, WORD& y);
double AddMonth(double R, double RM);
double DifMonth(double R1, double R2);
double ValDate(pstring Txt, pstring Mask);
pstring StrDate(double R, pstring Mask);
std::string CppToday();
double Today(); // r362
double CurrTime();
