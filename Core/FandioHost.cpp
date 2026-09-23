#include "FandioHost.h"
#include "base.h"
#include "GlobalVariables.h"
#include "obaseww.h"
#include "../fandio/Messages.h"
#include "../fandio/Settings.h"

static void set_msg_par(const fandio::Message& message)
{
	const std::vector<std::string>& p = message.params;
	switch (p.size()) {
	case 0: break; // keep parameters set by the caller, as before
	case 1: SetMsgPar(p[0]); break;
	case 2: SetMsgPar(p[0], p[1]); break;
	case 3: SetMsgPar(p[0], p[1], p[2]); break;
	default: SetMsgPar(p[0], p[1], p[2], p[3]); break;
	}
}

static char file_part_type(fandio::FilePart part)
{
	switch (part) {
	case fandio::FilePart::Text: return 'T';
	case fandio::FilePart::Index: return 'X';
	default: return '0';
	}
}

void InstallFandioMessageHandlers()
{
	fandio::SetMessageHandlers({
		.error = [](const fandio::Message& message) {
			set_msg_par(message);
			RunError(static_cast<WORD>(message.code));
		},
		.abort = [](const fandio::Message&) {
			// the message has already been shown and is in MsgLine
			GoExit(MsgLine);
		},
		.message = [](const fandio::Message& message) {
			set_msg_par(message);
			WrLLF10Msg(message.code);
		},
		.fileMessage = [](FileD* file, fandio::FilePart part, int code) {
			FileMsg(file, code, file_part_type(part));
		},
		.confirm = [](const fandio::Message& message) {
			set_msg_par(message);
			return PromptYN(static_cast<WORD>(message.code));
		},
	});
}

void ApplyFandioSettings()
{
	fandio::SetSettings({
		.workDir = WrkDir,
	});
}
