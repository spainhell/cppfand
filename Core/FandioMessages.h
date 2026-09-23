#pragma once

// Connects messages and errors reported by fandio to the CppFand UI
// (RunError, FileMsg, WrLLF10Msg, PromptYN). Call once at startup,
// before any data file is opened.
void InstallFandioMessageHandlers();
