#include "FandioHost.h"
#include "base.h"
#include "GlobalVariables.h"
#include "obaseww.h"
#include "OldDrivers.h"
#include "runfrml.h"
#include "../fandio/Expressions.h"
#include "../fandio/FileIO.h"
#include "../Common/CommonVariables.h"
#include "../fandio/Messages.h"
#include "../fandio/Settings.h"
#include "../fandbase/DateTime.h"
#include "../fandbase/pascal.h"
#include "../fandbase/textfunc.h"
#include "../fandio/FilePath.h"
#include "oaccess.h"

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

// Location of a data file: catalog, project directories, help file
// (formerly FileD::SetPathAndVolume). The result is also kept in the global
// CPath, CDir, CName, CExt and CVol, as the rest of CppFand expects.
static fandio::FilePath resolve_path(FileD* file_d, char pathDelim)
{
	bool isRdb = false;

	CVol = "";
	if (file_d->FileType == DataFileType::FandFile && file_d->FF->file_type == FandFileType::CAT) {
		CDir = GetEnv("FANDCAT");
		if (CDir.empty()) {
			CDir = TopDataDir.empty() ? TopRdbDir : TopDataDir;
		}
		AddBackSlash(CDir);
		CName = CatFDName;
		CExt = ".CAT";
		goto finish;
	}

	if (file_d->CatIRec != 0) {
		catalog->GetPathAndVolume(file_d, file_d->CatIRec, CPath, CVol);
		FSplit(CPath, CDir, CName, CExt);
		if (file_d->Name == "@") {
			CName = file_d->Name;
		}
		goto finish;
	}

	switch (file_d->FileType) {
	case DataFileType::FandFile: {
		switch (file_d->FF->file_type) {
		case FandFileType::RDB: {
			CExt = ".RDB";
			break;
		}
		case FandFileType::FAND8: {
			CExt = ".DTA";
			break;
		}
		default: {
			CExt = ".000";
			break;
		}
		}
		break;
	}
	case DataFileType::DBF: {
		CExt = ".DBF";
		break;
	}
	default:
		// other types don't have an extension
		break;
	}

	if (SetContextDir(file_d, CDir, isRdb)) {
		// do nothing
	}
	else {
		if (file_d == HelpFD) {
			CDir = FandDir;
#ifdef FandRunV
			CName = "UFANDHLP";
#else
			CName = "FANDHLP";
#endif
			goto finish;
		}
		CExt = ".100";
		if (CRdb != nullptr) {
			CDir = CRdb->DataDir;
		}
		else {
			CDir = "";
		}
	}

	AddBackSlash(CDir);
	CName = file_d->Name;

finish:
	if (pathDelim == '/') ReplaceChar(CDir, '\\', '/');
	if (pathDelim == '\\') ReplaceChar(CDir, '/', '\\');
	CPath = CDir + CName + CExt;
	return { CDir, CName, CExt, CVol };
}

// Path of an open file of the current projects, for error messages
// (formerly FileD::SetPathForH)
static std::string path_of_handle(HANDLE handle)
{
	Project* RD = CRdb;
	while (RD != nullptr) {
		if (RD->project_file != nullptr) {
			if (RD->project_file->FF->Handle == handle) {
				return RD->project_file->SetPathAndVolume();
			}
			if (RD->project_file->FF->TF != nullptr && RD->project_file->FF->TF->Handle == handle) {
				RD->project_file->SetPathAndVolume();
				return RD->project_file->CExtToT(CDir, CName, CExt);
			}
		}

		if (RD->help_file != nullptr) {
			if (RD->help_file->FF->Handle == handle) {
				return RD->help_file->SetPathAndVolume();
			}
			if (RD->help_file->FF->TF != nullptr && RD->help_file->FF->TF->Handle == handle) {
				RD->help_file->SetPathAndVolume();
				return RD->help_file->CExtToT(CDir, CName, CExt);
			}
		}

		for (FileD* fd : RD->data_files) {
			if (fd->FF->Handle == handle) {
				fd->SetPathAndVolume();
				return CPath;
			}

			if (fd->FF->XF != nullptr && fd->FF->XF->Handle == handle) {
				fd->SetPathAndVolume();
				CPath = CExtToX(CDir, CName, CExt);
				return CPath;
			}

			if (fd->FF->TF != nullptr && fd->FF->TF->Handle == handle) {
				fd->SetPathAndVolume();
				CPath = fd->CExtToT(CDir, CName, CExt);
				return CPath;
			}
		}
		RD = RD->ChainBack;
	}
	ReadMessage(799);
	CPath = MsgLine;
	return CPath;
}

void InstallFandioHandlers()
{
	fandio::SetPathHandlers({
		.resolve = resolve_path,
		.currentVolume = [] { return CVol; },
		.resetCurrentVolume = [] { CVol = ""; },
		.pathOfHandle = path_of_handle,
		.catalogRecord = [](const std::string& name, bool multilevel) {
			return catalog->GetCatalogIRec(name, multilevel);
		},
		.mountVolume = [](const std::string& path) {
			return static_cast<uint8_t>(TestMountVol(path.empty() ? '\0' : path[0]));
		},
	});
	fandio::SetExpressionHandlers({
		.evalBool = [](FileD* file, FrmlElem* expr, Record* record) { return RunBool(file, expr, record); },
		.evalReal = [](FileD* file, FrmlElem* expr, Record* record) { return RunReal(file, expr, record); },
		.evalString = [](FileD* file, FrmlElem* expr, Record* record) { return RunString(file, expr, record); },
	});

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
		.duplicateKey = [](const std::string& file_name) {
			// in the test run the user decides, otherwise just a message
			SetMsgPar(file_name);
			if (IsTestRun) {
				return PromptYN(832);
			}
			WrLLF10Msg(828);
			return true;
		},
		.lockWait = [](fandio::LockWait& wait) {
			switch (wait.kind) {
			case fandio::LockWaitKind::Mode:
			case fandio::LockWaitKind::Record: {
				// lock mode change: message after spec.LockRetries attempts, with a beep;
				// record lock: message right away, without a beep
				const bool mode_lock = wait.kind == fandio::LockWaitKind::Mode;
				if (!mode_lock || wait.attempt > spec.LockRetries) {
					WORD msg = 826;
					if (mode_lock || wait.record == 0) {
						SetMsgPar(wait.path, wait.mode);
						msg = 825;
					}
					int w = PushWrLLMsg(msg, wait.cancellable);
					if (wait.token == 0) {
						wait.token = w;
					}
					else {
						PopW(w, false);
					}
					if (mode_lock) {
						LockBeep();
					}
				}
				break;
			}
			case fandio::LockWaitKind::Open: {
				// opening a file on a network volume: message once, beep every time
				if (wait.token == 0) {
					SetMsgPar(wait.path, wait.mode);
					wait.token = PushWrLLMsg(825, false);
				}
				LockBeep();
				break;
			}
			}
			return KbdTimer(spec.NetDelay, wait.cancellable ? 1 : 0);
		},
		.lockWaitEnd = [](fandio::LockWait& wait) {
			if (wait.token != 0) {
				PopW(wait.token);
			}
		},
	});

	// settings that are known already; called again once FAND.CFG, WrkDir and LANNODE are read
	ApplyFandioSettings();
}

void ApplyFandioSettings()
{
	fandio::SetSettings({
		.workDir = WrkDir,
		.version = Version,
		.lanNode = LANNode,
		.isCurrentProjectFile = [](FileD* file) { return file == Chpt; },
		.isActiveProjectFile = [](FileD* file) {
			for (Project* R = CRdb; R != nullptr; R = R->ChainBack) {
				if (file == R->project_file) return true;
			}
			return false;
		},
		.isCatalogFile = [](FileD* file) { return catalog != nullptr && file == catalog->GetCatalogFile(); },
		.writableProjectFiles = [] { return IsTestRun || IsInstallRun; },
		.upgradeFile = [](FileD* file, int& file_size) { return catalog != nullptr && catalog->OldToNewCat(file_size); },
	});
	OffDefaultYear = spec.OffDefaultYear;
}
