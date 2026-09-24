#pragma once
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

class FileD;

namespace fandio
{
	// Part of a data file a message refers to
	enum class FilePart
	{
		Data,	// .000, .DBF
		Text,	// .T00, .DBT, .FPT
		Index	// .X00
	};

	// Message identified by its number in the CppFand message file, with optional parameters
	struct Message
	{
		int code = 0;
		std::vector<std::string> params;
	};

	// Thrown by the default handlers (when the host has not installed its own)
	class Error : public std::runtime_error
	{
	public:
		explicit Error(Message message);
		const Message& message() const { return message_; }
		int code() const { return message_.code; }

	private:
		Message message_;
	};

	// How fandio reports errors and talks to the user. The host application
	// installs its own handlers with SetMessageHandlers(); an empty handler
	// means the default behavior described below.
	enum class LockWaitKind
	{
		Mode,	// change of the lock mode of a file
		Record,	// lock of a record (record 0 = whole file)
		Open	// opening a file on a network volume that another user holds
	};

	// A lock that is held by another user; fandio retries until the handler gives up
	struct LockWait
	{
		LockWaitKind kind = LockWaitKind::Mode;
		std::string path;		// file being locked
		std::string mode;		// requested lock mode (RD, WR, CR, ...)
		int32_t record = 0;		// LockWaitKind::Record: record number (0 = whole file)
		bool cancellable = false;	// the user may give up (e.g. by ESC)
		int attempt = 0;		// number of failed attempts so far (1, 2, ...)
		int token = 0;			// free for the handler (e.g. id of a message window)
	};

	struct MessageHandlers
	{
		// Fatal error, the operation cannot continue. Must not return.
		// Default: throws fandio::Error.
		std::function<void(const Message&)> error;

		// The operation stops; the user has already been told why (by message,
		// fileMessage or confirm). Must not return.
		// Default: throws fandio::Error.
		std::function<void(const Message&)> abort;

		// Non-fatal message, the operation continues.
		// Default: writes a warning to the log.
		std::function<void(const Message&)> message;

		// Non-fatal message about a file (typically an I/O error, the code is
		// 700 + system error). The host builds the path from the file itself.
		// Default: writes a warning to the log.
		std::function<void(FileD* file, FilePart part, int code)> fileMessage;

		// Yes/no question.
		// Default: writes a warning to the log and answers "no".
		std::function<bool(const Message&)> confirm;

		// A duplicate key was found while building a unique index; the record
		// is left out of the index. Returns false to stop the operation.
		// Default: writes a warning to the log and continues.
		std::function<bool(const std::string& file_name)> duplicateKey;

		// Called after each failed attempt to get a lock. Waits before the next
		// attempt and returns true to try again, false to give up (honored only
		// when wait.cancellable).
		// Default: waits 0.5 s; a cancellable wait gives up after 20 attempts.
		std::function<bool(LockWait& wait)> lockWait;

		// Called when waiting for a lock is over (the lock was acquired or the
		// wait was given up), only if lockWait has been called.
		std::function<void(LockWait& wait)> lockWaitEnd;
	};

	void SetMessageHandlers(MessageHandlers handlers);

	[[noreturn]] void RaiseError(int code, std::vector<std::string> params = {});
	[[noreturn]] void Abort(int code, std::vector<std::string> params = {});
	void ShowMessage(int code, std::vector<std::string> params = {});
	void ShowFileMessage(FileD* file, FilePart part, int code);
	bool Confirm(int code, std::vector<std::string> params = {});
	// false = stop the operation
	bool ReportDuplicateKey(const std::string& file_name);

	// Records a failed attempt in wait and lets the handler wait; false = give up
	bool WaitForLock(LockWait& wait);
	// Ends the wait started by WaitForLock (no-op when there has been no failed attempt)
	void EndLockWait(LockWait& wait);
}
