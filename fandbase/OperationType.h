#pragma once
#include <cstdint>

/// <summary>
/// Type of operation being performed during index creation, sorting, or other file operations
/// </summary>
enum class OperationType : int8_t
{
	//None = ' ',           // No specific operation
	//Copy = 'C',           // Copy operation (export, file copy)
	Duplicate = 'D',      // Duplicate/Delete operations
	//Print = 'P',          // Print operation
	Sort = 'S',           // Sorting/Scan operation
	Work = 'W',           // Work index operation
	Index = 'X'           // Creating index file
};
