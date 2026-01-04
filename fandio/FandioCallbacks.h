#pragma once
#include <functional>
#include <string>
#include "FandioError.h"
#include "ProgressCallbacks.h"

namespace fandio {

// Decision callback return values
enum class UserDecision {
    Continue,    // User chose to continue despite error
    Abort,       // User chose to abort operation
    Retry        // User wants to retry the operation
};

// Decision prompt types
enum class PromptType {
    IndexCorrupted,      // "Index file corrupted, rebuild?"
    DuplicateKey,        // "Duplicate key found during indexing"
    FileAccessError,     // "Cannot access file, retry?"
    LockTimeout,         // "File locked by another user"
    FileCorrupted,       // "File appears corrupted"
    KeyLengthExceeded    // "Key length exceeds maximum"
};

// Callback for user decisions
// Parameters: prompt type, message, file name
// Returns: user's decision
using DecisionCallback = std::function<UserDecision(
    PromptType prompt_type,
    const std::string& message,
    const std::string& file_name
)>;

// Callback for error notifications (non-fatal errors that need user notification)
using ErrorNotifyCallback = std::function<void(const Error& error)>;

// Combined callbacks structure
struct FandioCallbacks {
    ProgressCallbacks progress;       // Existing progress callbacks
    DecisionCallback on_decision;     // For user prompts (replaces PromptYN, WrLLF10Msg)
    ErrorNotifyCallback on_error;     // For error notifications (replaces FileMsg without exit)

    // Check if decision callback is configured
    bool HasDecisionCallback() const { return on_decision != nullptr; }

    // Check if error callback is configured
    bool HasErrorCallback() const { return on_error != nullptr; }

    // Default callbacks that do nothing / return safe defaults
    static FandioCallbacks Default() {
        FandioCallbacks cb;
        cb.on_decision = [](PromptType, const std::string&, const std::string&) {
            return UserDecision::Abort; // Safe default - abort on any decision required
        };
        cb.on_error = [](const Error&) {}; // Silent - do nothing
        return cb;
    }

    // Strict callbacks - always abort, report errors
    static FandioCallbacks Strict() {
        FandioCallbacks cb;
        cb.on_decision = [](PromptType, const std::string&, const std::string&) {
            return UserDecision::Abort;
        };
        cb.on_error = [](const Error& err) {
            // In strict mode, you might want to log or throw
            // For now, just a placeholder
        };
        return cb;
    }

    // Tolerant callbacks - continue when possible
    static FandioCallbacks Tolerant() {
        FandioCallbacks cb;
        cb.on_decision = [](PromptType type, const std::string&, const std::string&) {
            // Continue for warnings, abort for critical errors
            switch (type) {
                case PromptType::DuplicateKey:
                case PromptType::KeyLengthExceeded:
                    return UserDecision::Continue;
                default:
                    return UserDecision::Abort;
            }
        };
        cb.on_error = [](const Error&) {};
        return cb;
    }
};

} // namespace fandio
