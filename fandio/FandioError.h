#pragma once
#include <string>
#include <variant>
#include <optional>

namespace fandio {

// Error codes enumeration - values match original error numbers for compatibility
enum class ErrorCode {
    // Success
    Ok = 0,

    // Memory/resource errors
    InsufficientMemory = 624,

    // File operation errors (700-799 range)
    FileReadError = 701,
    FileWriteError = 702,
    FileSeekError = 703,
    FileOpenError = 704,
    FileCreateError = 705,
    FileNotOpen = 706,
    FileLockError = 707,

    // Index errors (800-899 range)
    IndexNotValid = 819,
    DuplicateKey = 822,
    LockFailed = 825,
    DeleteFailed = 827,
    DuplicateKeyWarning = 828,
    IndexCorrupted = 831,
    IndexRecordError = 833,
    IndexPathError = 834,
    IndexPageError = 837,

    // Demo limitation
    DemoRecordLimit = 884,

    // Text file errors
    TextFileCorrupted = 888,
    TextFilePageError = 889,
    TextFileLengthError = 890,
    TextFilePositionError = 891,
    TextFilePrefixError = 893,

    // Index file errors
    IndexFileMissing = 903,
    IndexFileCreateError = 906,
    IndexCreationFailed = 907,

    // Demo mode
    DemoLimit = 884,

    // Index key too long
    IndexKeyTooLong = 155,

    // Lock errors
    LockConflict = 645,
    LockTimeout = 646,

    // Scan errors
    ScanError = 1181,

    // General errors
    UnknownError = 9999
};

// Error structure with context
struct Error {
    ErrorCode code = ErrorCode::UnknownError;
    std::string message;
    std::string file_path;
    char file_type = '\0'; // '0' = data, 'T' = text, 'X' = index

    Error() = default;

    Error(ErrorCode c, const std::string& msg = "",
          const std::string& path = "", char type = '\0')
        : code(c), message(msg), file_path(path), file_type(type) {}

    // Create error from original numeric error code
    static Error FromLegacyCode(int legacy_code, const std::string& path = "", char type = '\0') {
        return Error(static_cast<ErrorCode>(legacy_code), "", path, type);
    }

    // Get numeric code for compatibility
    int GetLegacyCode() const {
        return static_cast<int>(code);
    }

    // Check if this is a specific error
    bool Is(ErrorCode c) const { return code == c; }

    // Check if error is in file operation range
    bool IsFileError() const {
        int c = static_cast<int>(code);
        return c >= 700 && c < 800;
    }

    // Check if error is in index range
    bool IsIndexError() const {
        int c = static_cast<int>(code);
        return (c >= 800 && c < 900) || c == 903 || c == 906;
    }

    // Check if error is in text file range
    bool IsTextFileError() const {
        int c = static_cast<int>(code);
        return c >= 888 && c <= 893;
    }
};

// Result type - similar to Rust's Result<T, E>
template<typename T>
class Result {
public:
    Result() : has_value_(false) {}

    // Success constructor
    static Result<T> Ok(T value) {
        Result<T> r;
        r.value_ = std::move(value);
        r.has_value_ = true;
        return r;
    }

    // Error constructor
    static Result<T> Err(Error error) {
        Result<T> r;
        r.error_ = std::move(error);
        r.has_value_ = false;
        return r;
    }

    // Quick error creation from code
    static Result<T> Err(ErrorCode code, const std::string& msg = "") {
        return Err(Error(code, msg));
    }

    bool is_ok() const { return has_value_; }
    bool is_err() const { return !has_value_; }
    explicit operator bool() const { return has_value_; }

    T& value() { return value_; }
    const T& value() const { return value_; }
    T* operator->() { return &value_; }
    const T* operator->() const { return &value_; }
    T& operator*() { return value_; }
    const T& operator*() const { return value_; }

    Error& error() { return error_; }
    const Error& error() const { return error_; }

    // Unwrap with default
    T value_or(T default_val) const {
        return has_value_ ? value_ : std::move(default_val);
    }

    // Map success value
    template<typename U, typename F>
    Result<U> map(F&& func) const {
        if (has_value_) {
            return Result<U>::Ok(func(value_));
        }
        return Result<U>::Err(error_);
    }

    // Chain operations
    template<typename F>
    auto and_then(F&& func) const -> decltype(func(std::declval<T>())) {
        if (has_value_) {
            return func(value_);
        }
        using ReturnType = decltype(func(std::declval<T>()));
        return ReturnType::Err(error_);
    }

private:
    T value_{};
    Error error_;
    bool has_value_ = false;
};

// Specialization for void return type
template<>
class Result<void> {
public:
    Result() : has_value_(false) {}

    static Result<void> Ok() {
        Result<void> r;
        r.has_value_ = true;
        return r;
    }

    static Result<void> Err(Error error) {
        Result<void> r;
        r.error_ = std::move(error);
        r.has_value_ = false;
        return r;
    }

    static Result<void> Err(ErrorCode code, const std::string& msg = "") {
        return Err(Error(code, msg));
    }

    bool is_ok() const { return has_value_; }
    bool is_err() const { return !has_value_; }
    explicit operator bool() const { return has_value_; }

    Error& error() { return error_; }
    const Error& error() const { return error_; }

    // Chain void results
    template<typename F>
    auto and_then(F&& func) const -> decltype(func()) {
        if (has_value_) {
            return func();
        }
        using ReturnType = decltype(func());
        return ReturnType::Err(error_);
    }

private:
    Error error_;
    bool has_value_ = false;
};

// Helper macros for error propagation (similar to Rust's ? operator)
#define FANDIO_TRY(expr) \
    do { \
        auto _result = (expr); \
        if (_result.is_err()) { \
            return decltype(_result)::Err(_result.error()); \
        } \
    } while(0)

#define FANDIO_TRY_ASSIGN(var, expr) \
    auto _result_##var = (expr); \
    if (_result_##var.is_err()) { \
        return decltype(_result_##var)::Err(_result_##var.error()); \
    } \
    var = std::move(_result_##var.value())

} // namespace fandio
