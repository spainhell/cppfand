#pragma once

// Connection of the fandio library to CppFand.

// Connects messages and errors reported by fandio to the CppFand UI
// (RunError, FileMsg, WrLLF10Msg, PromptYN). Call once at startup,
// before any data file is opened.
void InstallFandioMessageHandlers();

// Passes CppFand settings (work directory, ...) to fandio.
// Call whenever they change.
void ApplyFandioSettings();
