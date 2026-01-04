#pragma once
#include <functional>
#include <string>

// Forward declarations
class FileD;
class Record;
class FrmlElem;

namespace fandio {

// Abstract interface for formula evaluation
// Allows fandio to work without Core/runfrml dependency
// Application provides implementation via callbacks

struct FormulaEvaluator {
    // Evaluate formula and return boolean result
    std::function<bool(FileD*, FrmlElem*, Record*)> eval_bool;

    // Evaluate formula and return real (double) result
    std::function<double(FileD*, FrmlElem*, Record*)> eval_real;

    // Evaluate formula and return integer result
    std::function<int(FileD*, FrmlElem*, Record*)> eval_int;

    // Evaluate formula and return string result
    std::function<std::string(FileD*, FrmlElem*, Record*)> eval_string;

    // Evaluate formula and return formula element (for complex expressions)
    std::function<FrmlElem*(FileD*, FrmlElem*, Record*)> eval_frml;

    // Check if evaluator is fully configured
    bool is_configured() const {
        return eval_bool && eval_real && eval_int && eval_string;
    }

    // Check if specific evaluator is available
    bool has_bool() const { return eval_bool != nullptr; }
    bool has_real() const { return eval_real != nullptr; }
    bool has_int() const { return eval_int != nullptr; }
    bool has_string() const { return eval_string != nullptr; }
    bool has_frml() const { return eval_frml != nullptr; }

    // Default evaluator that returns safe defaults
    // Use this when formula evaluation is not needed
    static FormulaEvaluator Default() {
        FormulaEvaluator e;
        e.eval_bool = [](FileD*, FrmlElem*, Record*) { return true; };
        e.eval_real = [](FileD*, FrmlElem*, Record*) { return 0.0; };
        e.eval_int = [](FileD*, FrmlElem*, Record*) { return 0; };
        e.eval_string = [](FileD*, FrmlElem*, Record*) { return std::string(); };
        e.eval_frml = [](FileD*, FrmlElem* f, Record*) { return f; };
        return e;
    }

    // Create evaluator that always returns true for bool (no filtering)
    static FormulaEvaluator NoFilter() {
        FormulaEvaluator e = Default();
        e.eval_bool = [](FileD*, FrmlElem*, Record*) { return true; };
        return e;
    }
};

// Global formula evaluator instance
// Application should set this before using XScan or XString with formulas
extern FormulaEvaluator g_formula_evaluator;

// Helper function to set the global evaluator
inline void SetFormulaEvaluator(const FormulaEvaluator& evaluator) {
    g_formula_evaluator = evaluator;
}

// Helper function to check if evaluator is ready
inline bool HasFormulaEvaluator() {
    return g_formula_evaluator.is_configured();
}

} // namespace fandio
