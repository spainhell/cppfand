#include "Expressions.h"

#include <stdexcept>
#include <utility>

namespace fandio
{
	namespace
	{
		ExpressionHandlers handlers_;

		[[noreturn]] void no_evaluator()
		{
			throw std::logic_error("fandio: expression cannot be evaluated, no evaluator installed");
		}
	}

	void SetExpressionHandlers(ExpressionHandlers handlers)
	{
		handlers_ = std::move(handlers);
	}

	bool EvalBool(FileD* file, FrmlElem* expr, Record* record)
	{
		if (expr == nullptr) return true;
		if (!handlers_.evalBool) no_evaluator();
		return handlers_.evalBool(file, expr, record);
	}

	double EvalReal(FileD* file, FrmlElem* expr, Record* record)
	{
		if (!handlers_.evalReal) no_evaluator();
		return handlers_.evalReal(file, expr, record);
	}

	std::string EvalString(FileD* file, FrmlElem* expr, Record* record)
	{
		if (!handlers_.evalString) no_evaluator();
		return handlers_.evalString(file, expr, record);
	}
}
