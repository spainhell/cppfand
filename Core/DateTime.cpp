#include "DateTime.h"

#include <ctime>
#include "base.h"
#include "legacy.h"

void EncodeMask(std::string& mask, WORD& min, WORD& max)
{
	char Code[] = "YMDhmst";
	min = 9; max = 0;
	for (size_t i = 1; i <= mask.length(); i++) {
		for (uint8_t j = 0; j < 7; j++) {
			if (mask[i - 1] == Code[j]) {
				mask[i - 1] = static_cast<char>(j);
				if (min > j) min = j;
				if (max < j) max = j;
			}
		}
	}
}

void AnalDateMask(const std::string& mask, WORD& i, WORD& i_date, WORD& n)
{
	n = 0;

	if (static_cast<uint8_t>(mask[i - 1]) <= 6) {
		i_date = static_cast<uint8_t>(mask[i - 1]);
		do {
			i++; n++;
		} while (!((i > mask.length()) || (mask[i - 1] != i_date)));
	}
}

bool IsLeapYear(WORD year)
{
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

WORD LeapYears(WORD year)
{
	if (year < 3) return 0;
	year--;
	return year / 4 - year / 100 + year / 400;
}

double RDate(WORD Y, WORD M, WORD D, WORD hh, WORD mm, WORD ss, WORD tt)
{
	int l;

	if ((D > NoDayInMonth[M]) && (M != 2 || D != 29 || !IsLeapYear(Y))) {
		return 0;
	}

	if (Y + M + D == 0) {
		l = 0;
	}
	else {
		l = (Y - 1) * 365 + LeapYears(Y) + D;
		for (WORD i = 1; i <= M - 1; i++) {
			l = l + NoDayInMonth[i];
		}
		if ((M > 2) && IsLeapYear(Y)) {
			l++;
		}
	}

	const int n = tt + 100 * ss + 6000 * mm;
	const double r = (n + 360000.0 * hh) / 8640000.0;

	return l + r;
}

void SplitDate(double R, WORD& d, WORD& m, WORD& y)
{
	int l = (int)trunc(R);

	if (l == 0) {
		y = 1;
		m = 1;
		d = 1;
	}
	else {
		WORD j;
		y = l / 365;
		y++;
		l = l % 365;
		while (l <= LeapYears(y)) {
			y--;
			l += 365;
		}
		l = l - LeapYears(y);

		for (j = 1; j <= 12; j++) {
			WORD i = NoDayInMonth[j];
			if ((j == 2) && IsLeapYear(y)) {
				i++;
			}
			if (i >= l) {
				break;
			}
			l -= i;
		}

		m = j;
		d = (WORD)l;
	}
}

double Today()
{
	std::time_t t = std::time(0);   // get time now
	struct tm lt;
	errno_t err = localtime_s(&lt, &t);
	return RDate(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, 0, 0, 0, 0);
}

std::string CppToday()
{
	std::time_t t = std::time(0);   // get time now
	struct tm lt;
	char buffer[32]{ '\0' };
	errno_t err = localtime_s(&lt, &t);
	strftime(buffer, sizeof(buffer), "%d.%m.%Y", &lt);
	return buffer;
}

double CurrTime()
{
	std::time_t t = std::time(0);   // get time now
	struct tm lt;
	errno_t err = localtime_s(&lt, &t);
	//return RDate(1 + 1900, 1, 0, lt.tm_hour, lt.tm_min, lt.tm_sec, 0);
	return RDate(1, 1, 0, lt.tm_hour, lt.tm_min, lt.tm_sec, 0);
}

double ValDate(const std::string& text, std::string mask)
{
	struct Z { int Y = 0, M = 0, D = 0, hh = 0, mm = 0, ss = 0, tt = 0; } z;
	int* Date = &z.Y;
	WORD k = 0, min = 0, max = 0, i_date = 0, n = 0;
	WORD year = 0, y = 0, month = 0, day = 0;

	double R = 0;
	int nl = 0;

	double result = 0.0;
	WORD y_length = 0;
	z.Y = -1; z.M = -1; z.D = -1;
	for (size_t i = 3; i <= 6; i++) {
		Date[i] = 0;
	}
	bool WasYMD = false;
	bool WasMinus = false;
	EncodeMask(mask, min, max);
	WORD i = 1;
	WORD j = 1;

	//label1:
	while (true) {
		if (j > text.length()) {
			// goto label2;
			break;
		}
		else {
			if (i > mask.length()) {
				return result;
			}
			AnalDateMask(mask, i, i_date, n);
			if (n == 0) {
				if (mask[i - 1] != text[j - 1]) {
					return result;
				}
				i++;
				j++;
			} /* delimiter */
			else { /* YMDhmst */
				std::string str;
				if (i_date < 3) WasYMD = true;

				while ((text[j - 1] == ' ') && (n > 1)) {
					j++;
					n--;
				}

				if ((text[j - 1] == '-') && (n > 1) && (i_date == min) && (i_date > 2)) {
					WasMinus = true;
					j++;
					n--;
				}

				if (!(text[j - 1] >= '0' && text[j - 1] <= '9')) {
					return result;
				}

				while ((j <= text.length()) && (isdigit(text[j - 1])) && (n > 0)) {
					str += text[j - 1];
					j++;
					n--;
				}
				val(str, Date[i_date], k);
				if (i_date == 0) {
					y_length = str.length();
				}
			}
			// goto label1;
		}
	}

	//label2:
	if ((min == 2) && (max >= 3)) {
		if (z.D < 0) z.D = 0;
		R = z.D + (z.tt + 100 * z.ss + 6000 * z.mm + 360000.0 * z.hh) / 8640000.0;
		//goto label3;
	}
	else {
		if (WasYMD) {
			SplitDate(Today(), day, month, year);
			/*if ((max<3) && (z.D=-1) && (z.M=-1) && (z.Y=-1)) return;*/
			if (z.D == -1) z.D = 1;
			else {
				if ((z.D == 0) || (z.D > 31)) return result;
				else if (z.M == -1) z.M = month;
			}

			if (z.M == -1) z.M = 1;
			else if ((z.M == 0) || (z.M > 12)) return result;

			if (y_length == 0) z.Y = year;
			else if (z.Y > 9999) return result;
			else
				if (y_length <= 2) {
					if (spec.OffDefaultYear == 0) {
						z.Y = (year / 100) * 100 + z.Y;
					}
					else {
						y = (year + spec.OffDefaultYear) % 100;
						if (z.Y < y) {
							z.Y = z.Y + 2000;
						}
						else {
							z.Y = z.Y + 1900;
						}
					}
				}
		}
		else { z.Y = 0; z.M = 0; z.D = 0; }

		if ((min < 3) && (z.hh > 23)) return result;
		if ((min < 4) && (z.mm > 59)) return result;
		if ((min < 5) && (z.ss > 59)) return result;

		R = RDate(z.Y, z.M, z.D, z.hh, z.mm, z.ss, z.tt);
	}
	//label3:
	if (!WasYMD && (R == 0.0)) R = 1E-11;

	if (WasMinus) result = -R;
	else result = R;

	return result;
}

bool ExCode(WORD N, pstring Mask)
{
	for (WORD i = 1; i <= Mask.length(); i++) {
		if ((Mask[i]) == N) return true;
	}
	return false;
}

std::string StrDate(double R, std::string mask)
{
	struct stD { WORD Y = 0; WORD M = 0; WORD D = 0; } d;
	struct stT { int hh = 0, mm = 0, ss = 0, tt = 0; } t;
	pstring x;
	WORD i, iDate = 0, n = 0, m = 0, min = 0, max = 0;
	int f = 0, l = 0; bool First = false, WasMinus = false; char c = 0;

	double MultX[7]{ 0, 0, 0, 24, 1440, 86400, 8640000 };
	int DivX[12]{ 0, 0, 0, 1, 60, 3600, 360000, 1, 60, 6000, 1, 100 };

	std::string res;
	EncodeMask(mask, min, max);
	WasMinus = false;

	if ((R == 0.0) || (R < 0) && (min < 3)) {
		for (size_t i = 1; i <= mask.length(); i++) {
			if (mask[i - 1] <= 6) {
				res += ' ';
			}
			else {
				res += mask[i - 1];
			}
		}
		goto label1;
	}
	else if (R < 0) {
		WasMinus = true;
		R = -R;
	}

	if (min < 3) {
		if ((min == 2) && (max >= 3)) {
			d.D = static_cast<uint16_t>(trunc(R));
		}
		else {
			SplitDate(R, d.D, d.M, d.Y);
		}
		double int_part;
		R = modf(R, &int_part);
	}

	if (max >= 3) {
		l = static_cast<int>(round(R * MultX[max]));
		if (ExCode(3, mask)) {
			f = DivX[max]; t.hh = l / f; l = l % f;
		}
		if (ExCode(4, mask)) {
			f = DivX[max + 3]; t.mm = l / f; l = l % f;
		}
		if (ExCode(5, mask)) {
			f = DivX[max + 5]; t.ss = l / f; l = l % f;
		}
		t.tt = l;
	}

	i = 1;
	First = true;

	while (i <= mask.length()) {
		AnalDateMask(mask, i, iDate, n);
		if (n == 0) {
			res += mask[i - 1];
			i++;
		}
		else {
			if (iDate < 3) {
				if (iDate == 0)	str(d.Y, x);
				if (iDate == 1)	str(d.M, x);
				if (iDate == 2)	str(d.D, x);
			}
			else {
				if (iDate == 3) str(t.hh, x);
				if (iDate == 4) str(t.mm, x);
				if (iDate == 5) str(t.ss, x);
				if (iDate == 6) str(t.tt, x);
				if ((iDate == min) && WasMinus) {
					pstring oldX = x;
					x = "-";
					x += oldX;
				}
			}
			if (First && (iDate > 2) && (iDate == min)) {
				c = ' ';
			}
			else {
				c = '0';
			}
			First = false;
			while (x.length() < n) {
				pstring oldX = x;
				x = "";
				x.Append(c);
				x += oldX;
			}
			if (iDate < 3) {
				x = copy(x, x.length() - n + 1, n);
			}
			res += x;
		}
	}
label1:
	return res;
}

double AddMonth(double R, double RM)
{
	WORD d, m, y;
	SplitDate(R, d, m, y);
	const int l = y * 12 + m - 1 + static_cast<int>(trunc(RM));
	double intpart;
	const double RTime = modf(R, &intpart);
	y = static_cast<WORD>(l / 12);
	m = (l % 12) + 1;
	if (d > NoDayInMonth[m]) {
		d = NoDayInMonth[m];
		if (m == 2 && IsLeapYear(y)) d = 29;
	}
	return RDate(y, m, d, 0, 0, 0, 0) + RTime;
}

double DifMonth(double R1, double R2)
{
	WORD d1, m1, y1, d2, m2, y2;
	SplitDate(R1, d1, m1, y1);
	SplitDate(R2, d2, m2, y2);
	const int y = y2 - y1;
	const int m = m2 - m1;
	return y * 12 + m;
}