// fandtest - runs a PC-FAND test task in CppFand and reports its results.
//
//   fandtest [--fand-dir DIR] [--timeout SEC] [--only TEST[,TEST...]] [--keep] <task-dir>
//
// 1. Packs the chapter files of <task-dir> (NNNN_<name>_<type>.txt) into
//    FANDTEST.RDB in a new temporary work directory (fandrdb pack).
// 2. Runs the task in cppfandlib.dll (FandStart) without a user; when it does
//    not end in time, prints the screen (e.g. a message waiting for a key)
//    and stops it.
// 3. Reads the results the task wrote to VYSLEDKY.000:
//       Test:A,20  Stav:A,5  Popis:A,60
//    Stav is OK or CHYBA; the last record of a complete run is Test = *KONEC*.
//
// DIR is the directory with FAND.CFG and FAND.RES (default: %FANDDIR%).
// Exit code: 0 = all tests passed, 1 = a test failed or the task did not
// finish, 2 = wrong usage or environment, 3 = the interpreter crashed (the
// stack is printed).
//
// Concurrent access: chapters NNNN_<Test>2_P.txt are the second user of the
// test <Test>. They form a second task FANDTST2 (its MAIN calls them in order),
// which runs in another process over the same data (own FANDWORK, LANNODE 2;
// the first task has LANNODE 1). It starts when the first task creates the
// synchronization file TSYNC.000; the tasks then wait for each other there.
// Files of chapters F 0050-0099 are shared (catalog volume #), see write_catalog.

#include <windows.h>
#include <dbghelp.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../fandio/FileD.h"
#include "../fandio/FileIO.h"
#include "../fandio/Messages.h"
#include "../fandio/Record.h"

#pragma comment(lib, "dbghelp.lib")

namespace fs = std::filesystem;

namespace
{
	const char* TaskName = "FANDTEST";
	const char* PartnerTaskName = "FANDTST2";
	const char* ResultsFile = "VYSLEDKY.000";
	const char* SyncFile = "TSYNC.000";
	const char* EndMark = "*KONEC*";

	// ---------------------------------------------------------------- cppfandlib.dll

	struct ScreenInfo
	{
		int32_t Cols, Rows, CursorX, CursorY, CursorVisible, CursorSize, Running, FieldX, FieldY, FieldLen;
	};

	struct Fand
	{
		HMODULE dll = nullptr;
		int (*Start)(const char*, const char*, const char*, const char*) = nullptr;
		void (*SetScreenSize)(int, int) = nullptr;
		void (*SetFieldEditHost)(int) = nullptr;
		int (*Wait)(int) = nullptr;
		void (*Stop)() = nullptr;
		int (*ExitCode)() = nullptr;
		int (*LastError)(char*, int) = nullptr;
		uint64_t (*GetScreen)(uint16_t*, int, ScreenInfo*) = nullptr;

		template <typename T>
		bool get(T& fn, const char* name)
		{
			fn = reinterpret_cast<T>(GetProcAddress(dll, name));
			if (fn == nullptr) fprintf(stderr, "cppfandlib.dll: missing %s\n", name);
			return fn != nullptr;
		}

		bool load(const fs::path& path)
		{
			dll = LoadLibraryW(path.c_str());
			if (dll == nullptr) {
				fprintf(stderr, "cannot load %s (error %lu)\n", path.string().c_str(), GetLastError());
				return false;
			}
			return get(Start, "FandStart") && get(SetScreenSize, "FandSetScreenSize")
				&& get(SetFieldEditHost, "FandSetFieldEditHost") && get(Wait, "FandWait")
				&& get(Stop, "FandStop") && get(ExitCode, "FandExitCode")
				&& get(LastError, "FandLastError") && get(GetScreen, "FandGetScreen");
		}
	};

	std::string cp852_to_utf8(const std::string& s)
	{
		if (s.empty()) return s;
		const int wlen = MultiByteToWideChar(852, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
		std::wstring w(wlen, L'\0');
		MultiByteToWideChar(852, 0, s.data(), static_cast<int>(s.size()), w.data(), wlen);
		const int len = WideCharToMultiByte(CP_UTF8, 0, w.data(), wlen, nullptr, 0, nullptr, nullptr);
		std::string result(len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, w.data(), wlen, result.data(), len, nullptr, nullptr);
		return result;
	}

	void print_screen(Fand& fand)
	{
		std::vector<uint16_t> cells(200 * 100);
		ScreenInfo info{};
		if (fand.GetScreen(cells.data(), static_cast<int>(cells.size()), &info) == 0) return;
		printf("---- screen %dx%d ----\n", info.Cols, info.Rows);
		for (int y = 0; y < info.Rows; y++) {
			std::string line;
			for (int x = 0; x < info.Cols; x++) {
				const char c = static_cast<char>(cells[y * info.Cols + x] & 0xFF);
				line += (static_cast<unsigned char>(c) < 32) ? ' ' : c;
			}
			while (!line.empty() && line.back() == ' ') line.pop_back();
			printf("%s\n", cp852_to_utf8(line).c_str());
		}
		printf("----\n");
	}

	// ---------------------------------------------------------------- results

	struct Result
	{
		std::string test, state, description;
	};

	FieldDescr* make_alfa(const char* name, uint8_t len)
	{
		FieldDescr* f = new FieldDescr();
		f->Name = name;
		f->field_type = FieldType::ALFANUM;
		f->frml_type = 'S';
		f->L = len;
		f->M = LeftJust;
		f->NBytes = len;
		f->Flg = f_Stored;
		return f;
	}

	std::string trim_right(std::string s)
	{
		while (!s.empty() && (s.back() == ' ' || s.back() == '\0')) s.pop_back();
		return s;
	}

	// records of VYSLEDKY.000 (declared in the test task as Test:A,20; Stav:A,5; Popis:A,60)
	bool read_results(const fs::path& path, std::vector<Result>& results)
	{
		if (!fs::exists(path)) return false;
		FileD file(DataFileType::FandFile, ProgressCallbacks{});
		file.Name = path.stem().string();
		file.FullPath = path.string();
		file.FF->file_type = FandFileType::FAND16;
		FieldDescr* test = make_alfa("Test", 20);
		FieldDescr* state = make_alfa("Stav", 5);
		FieldDescr* description = make_alfa("Popis", 60);
		file.FldD = { test, state, description };
		file.CompileRecLen();
		if (!file.OpenF(file.FullPath, RdOnly, false)) return false;

		Record record(&file);
		for (int i = 1; i <= file.GetNRecs(); i++) {
			file.ReadRec(i, &record);
			results.push_back({
				cp852_to_utf8(trim_right(record.LoadS(test))),
				trim_right(record.LoadS(state)),
				cp852_to_utf8(trim_right(record.LoadS(description))) });
		}
		file.Close();
		return true;
	}

	// A crash of the interpreter (in its thread in cppfandlib.dll) ends the whole
	// process; print where it happened (function names and lines from the .pdb).
	LONG WINAPI crash_handler(EXCEPTION_POINTERS* info)
	{
		fprintf(stderr, "\nCRASH: exception 0x%08lX at %p\n",
			info->ExceptionRecord->ExceptionCode, info->ExceptionRecord->ExceptionAddress);

		HANDLE process = GetCurrentProcess();
		HANDLE thread = GetCurrentThread();
		SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
		SymInitialize(process, nullptr, TRUE);

		CONTEXT context = *info->ContextRecord;
		STACKFRAME64 frame{};
#ifdef _M_X64
		const DWORD machine = IMAGE_FILE_MACHINE_AMD64;
		frame.AddrPC.Offset = context.Rip;
		frame.AddrFrame.Offset = context.Rbp;
		frame.AddrStack.Offset = context.Rsp;
#else
		const DWORD machine = IMAGE_FILE_MACHINE_I386;
		frame.AddrPC.Offset = context.Eip;
		frame.AddrFrame.Offset = context.Ebp;
		frame.AddrStack.Offset = context.Esp;
#endif
		frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;

		alignas(SYMBOL_INFO) char symbol_buffer[sizeof(SYMBOL_INFO) + 256];
		for (int i = 0; i < 40; i++) {
			if (!StackWalk64(machine, process, thread, &frame, &context, nullptr,
				SymFunctionTableAccess64, SymGetModuleBase64, nullptr) || frame.AddrPC.Offset == 0) break;
			SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer);
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = 255;
			DWORD64 displacement = 0;
			const char* name = SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol) ? symbol->Name : "?";
			IMAGEHLP_LINE64 line{ sizeof(IMAGEHLP_LINE64) };
			DWORD line_displacement = 0;
			if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &line_displacement, &line)) {
				fprintf(stderr, "  %s  %s:%lu\n", name, fs::path(line.FileName).filename().string().c_str(), line.LineNumber);
			}
			else {
				fprintf(stderr, "  %s\n", name);
			}
		}
		fflush(stderr);
		TerminateProcess(GetCurrentProcess(), 3);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	fs::path exe_dir()
	{
		wchar_t buf[MAX_PATH];
		GetModuleFileNameW(nullptr, buf, MAX_PATH);
		return fs::path(buf).parent_path();
	}

	std::string env(const char* name)
	{
		char* value = nullptr;
		size_t len = 0;
		if (_dupenv_s(&value, &len, name) != 0 || value == nullptr) return "";
		std::string result = value;
		free(value);
		return result;
	}

	// for this process and for cppfandlib.dll loaded later (it may have its own CRT)
	void set_env(const char* name, const std::string& value)
	{
		_putenv_s(name, value.c_str());
		SetEnvironmentVariableA(name, value.c_str());
	}

	bool write_file(const fs::path& path, const std::string& text)
	{
		FILE* f = nullptr;
		if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || f == nullptr) return false;
		fwrite(text.data(), 1, text.size(), f);
		fclose(f);
		return true;
	}

	// NNNN_<name>_<type>.txt -> number, name and type
	bool split_chapter(const std::string& file_name, int& number, std::string& name, std::string& type)
	{
		const size_t first = file_name.find('_');
		const size_t last = file_name.rfind('_');
		const size_t dot = file_name.rfind('.');
		if (first == std::string::npos || last <= first || dot == std::string::npos || dot < last) return false;
		number = std::atoi(file_name.substr(0, first).c_str());
		name = file_name.substr(first + 1, last - first - 1);
		type = file_name.substr(last + 1, dot - last - 1);
		return true;
	}

	// procedure of the second user of a test (<Test>2)
	bool is_partner_chapter(const std::string& file_name)
	{
		int number; std::string name, type;
		return split_chapter(file_name, number, name, type) && number >= 100
			&& _stricmp(type.c_str(), "P") == 0 && name.size() > 1 && name.back() == '2';
	}

	// Files declared in chapters F 0050-0099 are shared by the users: the catalog
	// <task>.CAT puts them on the network volume '#' (otherwise PC-FAND opens files
	// exclusively). Catalog record (107 B): RdbName A,8; FileName A,8; Archive N,2 (1 B);
	// PathName 79 B; Volume 11 B (as in real catalogs).
	// Further records are in <task-dir>\katalog.csv (lines RdbName;FileName;Archive;PathName;Volume,
	// RdbName * = the task, # = comment); for ARCHIVES records the directory of the archive
	// is created in the work directory.
	bool write_catalog(const fs::path& work, const char* task, const std::vector<std::string>& chapters, const fs::path& task_dir)
	{
		auto field = [](std::string s, size_t len) { s.resize(len, ' '); return s; };
		std::string records;
		int32_t n = 0;
		for (const std::string& chapter : chapters) {
			int number; std::string name, type;
			if (!split_chapter(chapter, number, name, type) || number < 50 || number > 99 || _stricmp(type.c_str(), "F") != 0) continue;
			const size_t dot = name.find('.');
			const bool dbf = dot != std::string::npos && _stricmp(name.substr(dot).c_str(), ".DBF") == 0;
			const std::string file = name.substr(0, dot);
			records += field(task, 8) + field(file, 8) + std::string(1, '\0') + field(file + (dbf ? ".DBF" : ".000"), 79) + field("#", 11);
			n++;
		}

		FILE* f = nullptr;
		if (_wfopen_s(&f, (task_dir / "katalog.csv").c_str(), L"rb") == 0 && f != nullptr) {
			char buf[512];
			while (fgets(buf, sizeof(buf), f) != nullptr) {
				std::string line = buf;
				while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
				if (line.empty() || line[0] == '#') continue;
				std::vector<std::string> items;
				size_t start = 0;
				while (true) {
					const size_t semicolon = line.find(';', start);
					items.push_back(line.substr(start, semicolon == std::string::npos ? std::string::npos : semicolon - start));
					if (semicolon == std::string::npos) break;
					start = semicolon + 1;
				}
				items.resize(5);
				const std::string rdb = items[0] == "*" ? std::string(task) : items[0];
				std::string archive = items[2];
				if (archive.size() == 1) archive = "0" + archive;
				const uint8_t packed = archive.size() == 2 ? static_cast<uint8_t>(((archive[0] - '0') << 4) | (archive[1] - '0')) : 0;
				records += field(rdb, 8) + field(items[1], 8) + std::string(1, static_cast<char>(packed)) + field(items[3], 79) + field(items[4], 11);
				n++;
				if (_stricmp(rdb.c_str(), "ARCHIVES") == 0) {
					// path of the archive (without the list of further archive numbers after a space)
					const fs::path archive_path = work / items[3].substr(0, items[3].find(' '));
					std::error_code ec;
					fs::create_directories(archive_path.parent_path(), ec);
				}
			}
			fclose(f);
		}
		if (n == 0) return true;
		const uint16_t rec_len = 107;
		std::string prefix(6, '\0');
		memcpy(prefix.data(), &n, 4);
		memcpy(prefix.data() + 4, &rec_len, 2);
		return write_file(work / (std::string(task) + ".CAT"), prefix + records);
	}

	int start_and_wait(Fand& fand, const std::string& fand_dir, const fs::path& work, const char* task,
		int timeout_s, const char* who, std::function<void()> tick = nullptr)
	{
		fand.SetScreenSize(132, 25); // wide enough for messages with long paths
		if (fand.Start(fand_dir.c_str(), work.string().c_str(), task, "") != 0) {
			fprintf(stderr, "%sFandStart failed\n", who);
			return -1;
		}
		fand.SetFieldEditHost(0);

		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_s);
		bool finished = false;
		while (!finished && std::chrono::steady_clock::now() < deadline) {
			finished = fand.Wait(100) != 0;
			if (tick) tick();
		}
		if (!finished) {
			printf("%sthe task did not finish in %d s:\n", who, timeout_s);
			print_screen(fand);
			fand.Stop();
			fand.Wait(5000);
		}
		return finished ? 1 : 0;
	}

	// the second user: runs FANDTST2 in the work directory of the first one
	int run_partner(const std::string& fand_dir, const fs::path& work, int timeout_s)
	{
		set_env("FANDWORK", (work / "wB").string() + "\\");
		set_env("LANNODE", "2");
		Fand fand;
		if (!fand.load(exe_dir() / "cppfandlib.dll")) return 2;
		const int finished = start_and_wait(fand, fand_dir, work, PartnerTaskName, timeout_s, "second user: ");
		char error[512]{};
		fand.LastError(error, sizeof(error));
		const int exit_code = fand.ExitCode();
		if (finished > 0 && exit_code == 0 && error[0] == '\0') return 0;
		printf("second user: exit code %d %s\n", exit_code, error);
		return 1;
	}
}

int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);
	SetUnhandledExceptionFilter(crash_handler);

	std::string fand_dir = env("FANDDIR");
	int timeout_s = 60;
	bool keep = false;
	bool partner = false;
	std::string only;
	std::string task_dir;
	for (int i = 1; i < argc; i++) {
		const std::string a = argv[i];
		if (a == "--fand-dir" && i + 1 < argc) fand_dir = argv[++i];
		else if (a == "--timeout" && i + 1 < argc) timeout_s = atoi(argv[++i]);
		else if (a == "--only" && i + 1 < argc) only = argv[++i];
		else if (a == "--keep") keep = true;
		else if (a == "--partner") partner = true;
		else task_dir = a;
	}
	if (task_dir.empty()) {
		fprintf(stderr, "usage: fandtest [--fand-dir DIR] [--timeout SEC] [--only TEST[,TEST...]] [--keep] <task-dir>\n");
		return 2;
	}
	if (fand_dir.empty() || !fs::exists(fs::path(fand_dir) / "FAND.RES")) {
		fprintf(stderr, "FAND.RES not found; set --fand-dir or FANDDIR to the directory with FAND.CFG and FAND.RES\n");
		return 2;
	}
	if (partner) {
		// started by the first user, task_dir is its work directory
		return run_partner(fand_dir, task_dir, timeout_s);
	}

	// 1. work directory with the packed task (src) and the task of the second user (src2)
	const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count() % 100000000;
	const fs::path work = fs::temp_directory_path() / ("ft" + std::to_string(stamp));
	const fs::path src = work / "src";
	const fs::path src2 = work / "src2";
	fs::create_directories(src);
	fs::create_directories(src2);
	fs::create_directories(work / "wA");
	fs::create_directories(work / "wB");

	// with --only, test chapters (numbers 0100 and higher) that are not selected are left out,
	// so that a compile error in one of them does not stop the others
	auto selected = [&only](const std::string& file_name) {
		if (only.empty()) return true;
		int number; std::string name, type;
		if (!split_chapter(file_name, number, name, type) || number < 100) return true;
		if (is_partner_chapter(file_name)) name.pop_back(); // <Test>2 belongs to <Test>
		const std::string list = "," + only + ",";
		const std::string item = "," + name + ",";
		for (size_t i = 0; i + item.size() <= list.size(); i++) {
			if (_strnicmp(list.c_str() + i, item.c_str(), item.size()) == 0) return true;
		}
		return false;
	};
	std::vector<std::string> chapters;
	for (const auto& entry : fs::directory_iterator(task_dir)) {
		if (entry.path().extension() != ".txt") continue;
		const std::string name = entry.path().filename().string();
		if (selected(name)) chapters.push_back(name);
	}
	std::sort(chapters.begin(), chapters.end());

	std::string main_chapter;
	std::string partner_main = "begin\r\n";
	bool has_partner = false;
	for (const std::string& name : chapters) {
		fs::copy_file(fs::path(task_dir) / name, src / name);
		fs::copy_file(fs::path(task_dir) / name, src2 / name);
		if (name.size() > 11 && _stricmp(name.substr(name.size() - 11).c_str(), "_MAIN_P.txt") == 0) main_chapter = name;
		if (is_partner_chapter(name)) {
			int number; std::string proc, type;
			split_chapter(name, number, proc, type);
			partner_main += "proc(" + proc + ");\r\n";
			has_partner = true;
		}
	}
	partner_main += "end;\r\n";
	if (main_chapter.empty()) {
		fprintf(stderr, "no MAIN chapter (NNNN_MAIN_P.txt) in '%s'\n", task_dir.c_str());
		return 2;
	}
	if (!only.empty()) {
		// MAIN calls only the selected tests
		std::string main_text = "begin\r\n";
		size_t start = 0;
		while (start <= only.size()) {
			const size_t comma = only.find(',', start);
			const std::string test = only.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
			if (!test.empty()) main_text += "proc(" + test + ");\r\n";
			if (comma == std::string::npos) break;
			start = comma + 1;
		}
		main_text += std::string("proc(Vysl,('") + EndMark + "','OK',''));\r\nend;\r\n";
		if (!write_file(src / main_chapter, main_text)) return 2;
	}
	if (!write_file(src2 / main_chapter, partner_main)) return 2;

	auto pack = [&work](const fs::path& dir, const char* task) {
		const fs::path rdb = work / (std::string(task) + ".RDB");
		const std::string cmd = "\"\"" + (exe_dir() / "fandrdb.exe").string() + "\" pack \"" + dir.string() + "\" \"" + rdb.string() + "\" > nul\"";
		return std::system(cmd.c_str()) == 0;
	};
	if (!pack(src, TaskName) || (has_partner && !pack(src2, PartnerTaskName))) {
		fprintf(stderr, "fandrdb pack failed\n");
		return 2;
	}
	if (!write_catalog(work, TaskName, chapters, task_dir) || (has_partner && !write_catalog(work, PartnerTaskName, chapters, task_dir))) {
		fprintf(stderr, "cannot write the catalog\n");
		return 2;
	}

	// 2. run the task (FandStart switches the current directory of the process to work);
	//    the second user starts when the task creates the synchronization file
	const fs::path original_dir = fs::current_path();
	set_env("FANDWORK", (work / "wA").string() + "\\");
	set_env("LANNODE", "1");
	Fand fand;
	if (!fand.load(exe_dir() / "cppfandlib.dll")) return 2;

	PROCESS_INFORMATION partner_process{};
	bool partner_started = false;
	auto start_partner = [&]() {
		if (!has_partner || partner_started || !fs::exists(work / SyncFile)) return;
		wchar_t exe[MAX_PATH];
		GetModuleFileNameW(nullptr, exe, MAX_PATH);
		std::wstring cmd = L"\"" + std::wstring(exe) + L"\" --partner --fand-dir \"" + fs::path(fand_dir).wstring()
			+ L"\" --timeout " + std::to_wstring(timeout_s) + L" \"" + work.wstring() + L"\"";
		STARTUPINFOW si{ sizeof(si) };
		partner_started = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr,
			work.c_str(), &si, &partner_process) != 0;
		if (!partner_started) fprintf(stderr, "cannot start the second user (error %lu)\n", GetLastError());
	};
	const int started = start_and_wait(fand, fand_dir, work, TaskName, timeout_s, "", start_partner);
	if (started < 0) return 2;
	const bool finished = started > 0;
	char error[512]{};
	fand.LastError(error, sizeof(error));
	const int exit_code = fand.ExitCode();

	bool partner_ok = true;
	if (partner_started) {
		if (WaitForSingleObject(partner_process.hProcess, (timeout_s + 10) * 1000) != WAIT_OBJECT_0) {
			TerminateProcess(partner_process.hProcess, 1);
			printf("second user: did not end\n");
		}
		DWORD code = 1;
		GetExitCodeProcess(partner_process.hProcess, &code);
		partner_ok = code == 0;
		CloseHandle(partner_process.hProcess);
		CloseHandle(partner_process.hThread);
	}

	// 3. results
	std::vector<Result> results;
	const bool have_results = read_results(work / ResultsFile, results);
	int failed = 0, passed = 0;
	bool complete = false;
	for (const Result& r : results) {
		if (r.test == EndMark) {
			complete = true;
			continue;
		}
		const bool ok = r.state == "OK";
		ok ? passed++ : failed++;
		printf("%-5s %-20s %s\n", ok ? "OK" : "CHYBA", r.test.c_str(), r.description.c_str());
	}

	printf("\n%d passed, %d failed", passed, failed);
	if (!have_results) printf(", no results file");
	else if (!complete) printf(", the task did not finish (no %s record)", EndMark);
	if (exit_code != 0 || error[0] != '\0') printf(", exit code %d %s", exit_code, error);
	if (!partner_ok) printf(", the second user failed");
	printf("\n");

	std::error_code ec;
	fs::current_path(original_dir, ec);
	if (!keep) fs::remove_all(work, ec);
	if (keep || ec) printf("work directory: %s\n", work.string().c_str());

	return (finished && complete && failed == 0 && passed > 0 && partner_ok) ? 0 : 1;
}
