#include "DateTime.h"

#include <ctime>
#include "base.h"
#include "legacy.h"

void EncodeMask(pstring& Mask, WORD& Min, WORD& Max)
{
	char Code[] = "YMDhmst";
	WORD i = 0, j = 0;
	Min = 9; Max = 0;
	for (i = 1; i <= Mask.length(); i++) {
		for (j = 0; j < 7; j++) {
			if ((char)Mask[i] == Code[j]) {
				Mask[i] = (char)j;
				if (Min > j) Min = j;
				if (Max < j) Max = j;
			}
		}
	}
}

void AnalDateMask(pstring& Mask, WORD& I, WORD& IDate, WORD& N)
{
	N = 0;
	if (Mask[I] <= 6) {
		IDate = Mask[I];
		do { I++; N++; } while (!((I > Mask.length()) || (Mask[I] != IDate)));
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
	int l = 0, n = 0;
	double r = 0;

	if ((D > NoDayInMonth[M]) && ((M != 2) || (D != 29) || !(IsLeapYear(Y)))) {
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
	n = tt + 100 * ss + 6000 * mm;
	r = (n + 360000.0 * hh) / (8640000.0);
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

double ValDate(pstring Txt, pstring Mask)
{
	struct Z { int Y = 0, M = 0, D = 0, hh = 0, mm = 0, ss = 0, tt = 0; } z;
	int* Date = &z.Y;
	WORD i = 0, j = 0, k = 0, min = 0, max = 0, iDate = 0,
		n = 0, Ylength = 0, Year = 0, y = 0, Month = 0, Day = 0;
	pstring s; bool WasYMD = false, WasMinus = false; double R = 0; int nl = 0;

	double result = 0.0;
	Ylength = 0; z.Y = -1; z.M = -1; z.D = -1;
	for (i = 3; i <= 6; i++) Date[i] = 0;
	WasYMD = false; WasMinus = false;
	EncodeMask(Mask, min, max);
	i = 1; j = 1;
label1:
	if (j > Txt.length()) {
		// goto label2;
	}
	else {
		if (i > Mask.length()) {
			return result;
		}
		AnalDateMask(Mask, i, iDate, n);
		if (n == 0) {
			if (Mask[i] != Txt[j]) {
				return result;
			}
			i++;
			j++;
		} /* delimiter */
		else { /* YMDhmst */
			s = "";
			if (iDate < 3) WasYMD = true;

			while ((Txt[j] == ' ') && (n > 1)) {
				j++;
				n--;
			}

			if ((Txt[j] == '-') && (n > 1) && (iDate == min) && (iDate > 2)) {
				WasMinus = true;
				j++;
				n--;
			}

			if (!(Txt[j] >= '0' && Txt[j] <= '9')) {
				return result;
			}

			while ((j <= Txt.length()) && (isdigit(Txt[j])) && (n > 0)) {
				s.Append(Txt[j]);
				j++;
				n--;
			}
			val(s, Date[iDate], k);
			if (iDate == 0) {
				Ylength = s.length();
			}
		}
		goto label1;
	}
//label2:
	if ((min == 2) && (max >= 3)) {
		if (z.D < 0) z.D = 0;
		R = z.D + (z.tt + 100 * z.ss + 6000 * z.mm + 360000.0 * z.hh) / 8640000.0;
		//goto label3;
	}
	else {
		if (WasYMD) {
			SplitDate(Today(), Day, Month, Year);
			/*if ((max<3) && (z.D=-1) && (z.M=-1) && (z.Y=-1)) return;*/
			if (z.D == -1) z.D = 1;
			else {
				if ((z.D == 0) || (z.D > 31)) return result;
				else if (z.M == -1) z.M = Month;
			}

			if (z.M == -1) z.M = 1;
			else if ((z.M == 0) || (z.M > 12)) return result;

			if (Ylength == 0) z.Y = Year;
			else if (z.Y > 9999) return result;
			else
				if (Ylength <= 2) {
					if (spec.OffDefaultYear == 0) {
						z.Y = (Year / 100) * 100 + z.Y;
					}
					else {
						y = (Year + spec.OffDefaultYear) % 100;
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

pstring StrDate(double R, pstring Mask)
{
	struct stD { WORD Y = 0; WORD M = 0; WORD D = 0; } d;
	struct stT { int hh = 0, mm = 0, ss = 0, tt = 0; } t;
	pstring x;
	WORD i = 0, iDate = 0, n = 0, m = 0, min = 0, max = 0;
	int f = 0, l = 0; bool First = false, WasMinus = false; char c = 0;

	double MultX[7]{ 0, 0, 0, 24, 1440, 86400, 8640000 };
	int DivX[12]{ 0, 0, 0, 1, 60, 3600, 360000, 1, 60, 6000, 1, 100 };

	pstring s = "";
	EncodeMask(Mask, min, max);
	WasMinus = false;
	if ((R == 0.0) || (R < 0) && (min < 3)) {
		for (i = 1; i <= Mask.length(); i++) {
			if (Mask[i] <= 6) s.Append(' ');
			else s.Append(Mask[i]);
		}
		goto label1;
	}
	else if (R < 0) { WasMinus = true; R = -R; }
	if (min < 3) {
		if ((min == 2) && (max >= 3)) d.D = trunc(R);
		else SplitDate(R, d.D, d.M, d.Y);
		double intpart;
		R = modf(R, &intpart);
	}
	if (max >= 3)
	{
		l = round(R * MultX[max]);
		if (ExCode(3, Mask))
		{
			f = DivX[max]; t.hh = l / f; l = l % f;
		}
		if (ExCode(4, Mask))
		{
			f = DivX[max + 3]; t.mm = l / f; l = l % f;
		}
		if (ExCode(5, Mask))
		{
			f = DivX[max + 5]; t.ss = l / f; l = l % f;
		}
		t.tt = l;
	}
	i = 1;
	First = true;
	while (i <= Mask.length())
	{
		AnalDateMask(Mask, i, iDate, n);
		if (n == 0) { s.Append(Mask[i]); i++; }
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
				if ((iDate == min) && WasMinus)
				{
					pstring oldX = x;
					x = "-";
					x += oldX;
				}
			}
			if (First && (iDate > 2) && (iDate == min)) c = ' ';
			else c = '0';
			First = false;
			while (x.length() < n) {
				pstring oldX = x;
				x = "";
				x.Append(c);
				x += oldX;
			}
			if (iDate < 3) x = copy(x, x.length() - n + 1, n);
			s = s + x;
		}
	}
label1:
	return s;
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