#pragma once
#include <functional>
#include <string>

class FileD;
class Record;
class FrmlElem;	// expression of the host application, opaque for fandio

namespace fandio
{
	// How fandio evaluates expressions (scan filters, key interval bounds).
	// fandio only passes FrmlElem pointers around; the host application
	// that created them evaluates them.
	struct ExpressionHandlers
	{
		std::function<bool(FileD* file, FrmlElem* expr, Record* record)> evalBool;
		std::function<double(FileD* file, FrmlElem* expr, Record* record)> evalReal;
		std::function<std::string(FileD* file, FrmlElem* expr, Record* record)> evalString;
	};

	void SetExpressionHandlers(ExpressionHandlers handlers);

	// nullptr is an empty condition and evaluates to true.
	// A non-empty expression without an installed handler throws std::logic_error.
	bool EvalBool(FileD* file, FrmlElem* expr, Record* record);
	double EvalReal(FileD* file, FrmlElem* expr, Record* record);
	std::string EvalString(FileD* file, FrmlElem* expr, Record* record);
}
