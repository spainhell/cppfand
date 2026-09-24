#pragma once
#include <cstdint>
#include <string>

#include "typeDef.h"
#include "pstring.h"

// Two-digit years: pivot relative to the current year (OffDefaultYear in FAND.CFG),
// 0 = current century. Set by the host application.
extern uint8_t OffDefaultYear;

const WORD NoDayInMonth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; // aby mesice byly 1-12

double RDate(WORD Y, WORD M, WORD D, WORD hh, WORD mm, WORD ss, WORD tt);
void SplitDate(double R, WORD& d, WORD& m, WORD& y);
double AddMonth(double R, double RM);
double DifMonth(double R1, double R2);
double ValDate(const std::string& text, std::string mask);
std::string StrDate(double R, std::string mask);
std::string CppToday();
double Today(); // r362
double CurrTime();
