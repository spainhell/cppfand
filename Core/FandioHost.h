#pragma once

// Connection of the fandio library to CppFand.

// Connects fandio to CppFand: messages and errors go to the CppFand UI
// (RunError, FileMsg, WrLLF10Msg, PromptYN, lock waiting) and expressions
// are evaluated by the interpreter (RunBool, RunReal, RunString).
// Call once at startup, before any data file is opened.
void InstallFandioHandlers();

// Passes CppFand settings (work directory, version, OffDefaultYear from FAND.CFG, ...)
// to fandio and fandbase. Call after FAND.CFG is read and whenever they change.
void ApplyFandioSettings();
