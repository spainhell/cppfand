// fandtest - runs a PC-FAND test task in CppFand and reports its results.
//
//   fandtest [--fand-dir DIR] [--timeout SEC] [--keep] <task-dir>
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
// finish, 2 = wrong usage or environment.

#include <windows.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "../fandio/FileD.h"
#include "../fandio/FileIO.h"
#include "../fandio/Messages.h"
#include "../fandio/Record.h"

namespace fs = std::filesystem;

namespace
{
	const char* TaskName = "FANDTEST";
	const char* ResultsFile = "VYSLEDKY.000";
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
}

int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);

	std::string fand_dir = env("FANDDIR");
	int timeout_s = 60;
	bool keep = false;
	std::string task_dir;
	for (int i = 1; i < argc; i++) {
		const std::string a = argv[i];
		if (a == "--fand-dir" && i + 1 < argc) fand_dir = argv[++i];
		else if (a == "--timeout" && i + 1 < argc) timeout_s = atoi(argv[++i]);
		else if (a == "--keep") keep = true;
		else task_dir = a;
	}
	if (task_dir.empty()) {
		fprintf(stderr, "usage: fandtest [--fand-dir DIR] [--timeout SEC] [--keep] <task-dir>\n");
		return 2;
	}
	if (fand_dir.empty() || !fs::exists(fs::path(fand_dir) / "FAND.RES")) {
		fprintf(stderr, "FAND.RES not found; set --fand-dir or FANDDIR to the directory with FAND.CFG and FAND.RES\n");
		return 2;
	}

	// 1. work directory with the packed task
	const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count() % 100000000;
	const fs::path work = fs::temp_directory_path() / ("ft" + std::to_string(stamp));
	fs::create_directories(work);
	const fs::path rdb = work / (std::string(TaskName) + ".RDB");
	const std::string pack = "\"\"" + (exe_dir() / "fandrdb.exe").string() + "\" pack \"" + task_dir + "\" \"" + rdb.string() + "\" > nul\"";
	if (std::system(pack.c_str()) != 0) {
		fprintf(stderr, "fandrdb pack failed\n");
		return 2;
	}

	// 2. run the task (FandStart switches the current directory of the process to work)
	const fs::path original_dir = fs::current_path();
	Fand fand;
	if (!fand.load(exe_dir() / "cppfandlib.dll")) return 2;
	fand.SetScreenSize(132, 25); // wide enough for messages with long paths
	if (fand.Start(fand_dir.c_str(), work.string().c_str(), TaskName, "") != 0) {
		fprintf(stderr, "FandStart failed\n");
		return 2;
	}
	fand.SetFieldEditHost(0);

	bool finished = fand.Wait(timeout_s * 1000) != 0;
	if (!finished) {
		printf("the task did not finish in %d s:\n", timeout_s);
		print_screen(fand);
		fand.Stop();
		fand.Wait(5000);
	}
	char error[512]{};
	fand.LastError(error, sizeof(error));
	const int exit_code = fand.ExitCode();

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
	printf("\n");

	std::error_code ec;
	fs::current_path(original_dir, ec);
	if (!keep) fs::remove_all(work, ec);
	if (keep || ec) printf("work directory: %s\n", work.string().c_str());

	return (finished && complete && failed == 0 && passed > 0) ? 0 : 1;
}
