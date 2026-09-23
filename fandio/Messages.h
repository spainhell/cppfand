#pragma once
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
	};

	void SetMessageHandlers(MessageHandlers handlers);

	[[noreturn]] void RaiseError(int code, std::vector<std::string> params = {});
	[[noreturn]] void Abort(int code, std::vector<std::string> params = {});
	void ShowMessage(int code, std::vector<std::string> params = {});
	void ShowFileMessage(FileD* file, FilePart part, int code);
	bool Confirm(int code, std::vector<std::string> params = {});
}
