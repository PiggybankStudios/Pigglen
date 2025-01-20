/*
File:   os_virtual_mem.h
Author: Taylor Robbins
Date:   01\03\2025
Description:
	** Contains aliases that help us work with virtual memory on each platform (VirtualAlloc on Windows, mmap on Linux, etc.)
*/

#ifndef _OS_VIRTUAL_MEM_H
#define _OS_VIRTUAL_MEM_H

#include "base/base_compiler_check.h"
#include "base/base_defines_check.h"
#include "base/base_typedefs.h"
#include "base/base_macros.h"
#include "base/base_assert.h"
#include "std/std_includes.h"
#include "os/os_error.h"

#if (TARGET_IS_WASM && !USING_CUSTOM_STDLIB)
#define WASM_MEMORY_PAGE_SIZE      (64*1024ULL) //64kB or 65,536b
#define WASM_MEMORY_MAX_NUM_PAGES  (64*1024ULL) //65,536 pages * 64 kB/page = 4GB
#define WASM_MEMORY_MAX_SIZE       ((u64)WASM_MEMORY_MAX_NUM_PAGES * (u64)WASM_MEMORY_PAGE_SIZE)
#define WASM_PROTECTED_SIZE        1024       //1kB at start of wasm memory should be unused and should never be written to
#endif

// +--------------------------------------------------------------+
// |                 Header Function Declarations                 |
// +--------------------------------------------------------------+
#if !PIG_CORE_IMPLEMENTATION
	uxx OsGetMemoryPageSize();
	void* OsReserveMemory(uxx numBytes);
	void OsCommitReservedMemory(void* memoryPntr, uxx numBytes);
	void OsFreeReservedMemory(void* memoryPntr, uxx reservedSize);
#endif //!PIG_CORE_IMPLEMENTATION

// +--------------------------------------------------------------+
// |                   Function Implementations                   |
// +--------------------------------------------------------------+
#if PIG_CORE_IMPLEMENTATION

PEXP uxx OsGetMemoryPageSize()
{
	#if TARGET_IS_WINDOWS
	{
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		Assert(systemInfo.dwPageSize > 0);
		return (uxx)systemInfo.dwPageSize;
	}
	#elif (TARGET_IS_LINUX || TARGET_IS_OSX)
	{
		//NOTE: getpagesize() was not available on Ubunutu in WSL
		uxx result = sysconf(_SC_PAGESIZE);
		Assert(result > 0);
		return (uxx)result;
	}
	#elif TARGET_IS_PLAYDATE
	{
		//This is the size of the L1 Cache (Since Playdate doesn't have virtual memory we'll just use this as a standin)
		return Kilobytes(8);
	}
	#elif TARGET_IS_WASM
	{
		//This is the size of the allocation pages in the WASM memory model, and WASM doesn't support virtual memory afaik
		return WASM_MEMORY_PAGE_SIZE;
	}
	#else
	AssertMsg(false, "OsGetMemoryPageSize does not support the current platform yet!");
	return 0;
	#endif
}

//NOTE: numBytes MUST be a multiple of memory page size!
PEXP void* OsReserveMemory(uxx numBytes)
{
	void* result = nullptr;
	uxx pageSize = OsGetMemoryPageSize();
	Assert(numBytes % pageSize == 0);
	
	#if TARGET_IS_WINDOWS
	{
		result = VirtualAlloc(
			nullptr, //lpAddress
			numBytes, //dwSize
			MEM_RESERVE, //flAllocationType
			PAGE_READWRITE //flProtect
		);
		if (result != nullptr) { Assert((size_t)result % pageSize == 0); }
	}
	#elif (TARGET_IS_LINUX || TARGET_IS_OSX)
	{
		// WSL Ubuntu Problems with mmap? https://github.com/microsoft/WSL/issues/658
		// https://unix.stackexchange.com/questions/405883/can-an-application-explicitly-commit-and-decommit-memory
		// It seems like we can't force the application to commit any of this memory. Writing to pages automatically commits them.
		// That's not the end of the world but it is a difference in behavior
		// TODO: Maybe we can use mprotect? Do we need to calling code to manage the protection since it knows the size of the reserved memory?
		//NOTE: If you get MAP_ANONYMOUS is undefined, try using -std=gnu11 instead of something like -std=c11
		result = mmap(
			nullptr, //addr
			numBytes, //length
			PROT_READ|PROT_WRITE, //prot
			MAP_ANONYMOUS | MAP_PRIVATE, //flags
			-1, //fd,
			0 //offset
		);
		Assert(result != MAP_FAILED);
		if (result != nullptr && result != MAP_FAILED) { Assert((uxx)result % pageSize == 0); }
	}
	#else
	AssertMsg(false, "OsReserveMemory is not implemented for the current TARGET!");
	#endif
	
	return result;
}

//NOTE: numBytes must be a multiple of memory page size, and memoryPntr must be aligned to the beginning of a page
PEXP void OsCommitReservedMemory(void* memoryPntr, uxx numBytes)
{
	if (numBytes == 0) { return; }
	uxx pageSize = OsGetMemoryPageSize();
	NotNull(memoryPntr);
	Assert((uxx)memoryPntr % pageSize == 0);
	Assert(numBytes % pageSize == 0);
	
	#if TARGET_IS_WINDOWS
	{
		void* commitResult = VirtualAlloc(
			memoryPntr, //lpAddress
			numBytes, //dwSize
			MEM_COMMIT, //flAllocationType
			PAGE_READWRITE //flProtect
		);
		Assert(commitResult == memoryPntr); //TODO: Handle errors, call GetLastError and return an OsError_t
	}
	#elif (TARGET_IS_LINUX || TARGET_IS_OSX)
	{
		int protectResult = mprotect(
			memoryPntr,
			numBytes,
			PROT_READ|PROT_WRITE
		);
		Assert(protectResult == 0); //TODO: Handle errors, check errno and return an OsError_t
	}
	// #elif TARGET_IS_OSX
	// {
	// 	SetOptionalOutPntr(errorOut, OsError_UnsupportedPlatform);
	// }
	#else
	AssertMsg(false, "OsCommitReservedMemory is not supported on the current TARGET!");
	#endif
}

PEXP void OsFreeReservedMemory(void* memoryPntr, uxx reservedSize)
{
	Assert((memoryPntr == nullptr) == (reservedSize == 0));
	if (memoryPntr == nullptr) { return; }
	
	uxx pageSize = OsGetMemoryPageSize();
	Assert((uxx)memoryPntr % pageSize == 0);
	Assert(reservedSize % pageSize == 0);
	
	#if TARGET_IS_WINDOWS
	{
		BOOL freeResult = VirtualFree(
			memoryPntr, //lpAddress
			reservedSize, //dwSize
			MEM_RELEASE //dwFreeType
		);
		Assert(freeResult != 0); //TODO: Handle errors, call GetLastError and return an OsError_t
	}
	#elif (TARGET_IS_LINUX || TARGET_IS_OSX)
	{
		int unmapResult = munmap(memoryPntr, reservedSize);
		Assert(unmapResult == 0); //TODO: Handle errors, check errno and return an OsError_t
	}
	// #elif OSX_COMPILATION
	// {
	// 	SetOptionalOutPntr(errorOut, OsError_UnsupportedPlatform);
	// }
	#else
	AssertMsg(false, "OsFreeReservedMemory is not supported on the current TARGET!");
	#endif
}

#endif //PIG_CORE_IMPLEMENTATION

#endif //  _OS_VIRTUAL_MEM_H
