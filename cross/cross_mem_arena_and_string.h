/*
File:   cross_mem_arena_and_string.h
Author: Taylor Robbins
Date:   01\06\2025
Description:
	** Extensions to the Arena and String APIs that use both together
*/

#ifndef _CROSS_MEM_ARENA_AND_STRING_H
#define _CROSS_MEM_ARENA_AND_STRING_H

//NOTE: Intentionally no includes here!

//NOTE: When addNullTerm is true we will allocate one more byte than numChars to hold a null-terminating character
//      As such, when Freeing these strings, we will pass an innacurate size value to the Arena.
//      If the string is being allocated from an arena that doesn't track sizes, then you might
//      want to forego allocating the nullterm character so sizes are accurate during Free.
char* AllocAndCopyChars(Arena* arena, uxx numChars, const char* charsToCopy, bool addNullTerm)
{
	DebugNotNull(arena);
	Assert(charsToCopy != nullptr || numChars == 0);
	if (numChars == 0 && !addNullTerm) { return nullptr; }
	char* result = (char*)AllocMem(arena, numChars + (addNullTerm ? 1 : 0));
	if (result == nullptr) { return nullptr; }
	if (numChars > 0)
	{
		MyMemCopy(result, charsToCopy, numChars);
	}
	if (addNullTerm)
	{
		result[numChars] = '\0';
	}
	return result;
}
char* AllocAndCopyCharsNt(Arena* arena, const char* nullTermStr, bool addNullTerm)
{
	DebugNotNull(arena);
	DebugNotNull(nullTermStr);
	uxx numChars = MyStrLength64(nullTermStr);
	return AllocAndCopyChars(arena, numChars, nullTermStr, addNullTerm);
}
Str8 AllocStrAndCopy(Arena* arena, uxx numChars, const char* charsToCopy, bool addNullTerm)
{
	DebugNotNull(arena);
	DebugNotNull(charsToCopy);
	char* allocatedChars = AllocAndCopyChars(arena, numChars, charsToCopy, addNullTerm);
	return (allocatedChars != nullptr) ? NewStr8(numChars, allocatedChars) : Str8_Empty;
}
Str8 AllocStrAndCopyNt(Arena* arena, const char* nullTermStr, bool addNullTerm)
{
	DebugNotNull(arena);
	DebugNotNull(nullTermStr);
	uxx numChars = MyStrLength64(nullTermStr);
	char* allocatedChars = AllocAndCopyChars(arena, numChars, nullTermStr, addNullTerm);
	return (allocatedChars != nullptr) ? NewStr8(numChars, allocatedChars) : Str8_Empty;
}

//NOTE: These all implicitly assume addNullTerm=false
#define AllocStr8(arenaPntr, string) AllocStrAndCopy((arenaPntr), (string).length, (string).chars, false)
#define AllocStr8Nt(arenaPntr, nullTermStr) AllocStrAndCopyNt((arenaPntr), (nullTermStr), false)
#define AllocStr8Length(arenaPntr, length, charPntr) AllocStrAndCopy((arenaPntr), (length), (charPntr), false)

void FreeStr8(Arena* arena, Str8* stringPntr)
{
	NotNull(stringPntr);
	if (stringPntr->length > 0)
	{
		FreeMem(arena, stringPntr->chars, stringPntr->length);
	}
	ClearPointer(stringPntr);
}
void FreeStr8WithNt(Arena* arena, Str8* stringPntr)
{
	NotNull(stringPntr);
	if (stringPntr->length > 0 || stringPntr->chars != nullptr)
	{
		FreeMem(arena, stringPntr->chars, stringPntr->length+1);
	}
	ClearPointer(stringPntr);
}

Str8 JoinStringsInArena(Arena* arena, Str8 left, Str8 right, bool addNullTerm)
{
	Str8 result;
	result.length = left.length + right.length;
	if (result.length == 0 && !addNullTerm) { return Str8_Empty; }
	result.chars = AllocArray(char, arena, result.length + (addNullTerm ? 1 : 0));
	if (result.chars == nullptr) { return Str8_Empty; }
	if (left.length  > 0) { MyMemCopy(result.chars + 0,           left.chars,  left.length);  }
	if (right.length > 0) { MyMemCopy(result.chars + left.length, right.chars, right.length); }
	if (addNullTerm) { result.chars[result.length] = '\0'; }
	return result;
}

Str8 StrReplace(Arena* arena, Str8 str, Str8 target, Str8 replacement, bool addNullTerm)
{
	NotNullStr(str);
	NotNullStr(target);
	NotNullStr(replacement);
	if (target.length == 0) { return AllocStr8(arena, str); } //nothing to target means nothing to replace
	Str8 result = Str8_Empty;
	
	for (u8 pass = 0; pass < 2; pass++)
	{
		uxx numBytesNeeded = 0;
		
		for (uxx bIndex = 0; bIndex < str.length; bIndex++)
		{
			if (bIndex + target.length <= str.length)
			{
				if (MyMemEquals(str.chars + bIndex, target.chars, target.length))
				{
					if (result.chars != nullptr)
					{
						Assert(numBytesNeeded + replacement.length <= result.length);
						MyMemCopy(&result.chars[numBytesNeeded], replacement.chars, replacement.length);
					}
					numBytesNeeded += replacement.length;
					bIndex += target.length-1;
				}
				else
				{
					if (result.chars != nullptr)
					{
						Assert(numBytesNeeded+1 <= result.length);
						result.chars[numBytesNeeded] = str.chars[bIndex];
					}
					numBytesNeeded += 1;
				}
			}
			else
			{
				if (result.chars != nullptr)
				{
					Assert(numBytesNeeded+1 <= result.length);
					result.chars[numBytesNeeded] = str.chars[bIndex];
				}
				numBytesNeeded += 1;
			}
		}
		
		if (pass == 0)
		{
			result.length = numBytesNeeded;
			if (arena == nullptr) { return result; }
			result.chars = AllocArray(char, arena, result.length + (addNullTerm ? 1 : 0));
			NotNull(result.chars);
		}
		else
		{
			Assert(numBytesNeeded == result.length);
			if (addNullTerm) { result.chars[result.length] = '\0'; }
		}
	}
	
	return result;
}

#endif //  _CROSS_MEM_ARENA_AND_STRING_H
