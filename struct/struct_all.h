/*
File:   struct_all.h
Author: Taylor Robbins
Date:   01\04\2025
Description:
	** Includes all files in this folder, meant as a quick shorthand for applications that just want everything from this folder available
	** This file also serves as a reference for the order of dependencies between files in this folder.
	** This file also includes files from other folders like mem and std because they are required for some of the files in struct
*/

#ifndef _STRUCT_ALL_H
#define _STRUCT_ALL_H

#include "base/base_typedefs.h" //Needed by struct_color.h and struct_var_array.h

#include "struct/struct_color.h"
#include "struct/struct_directions.h"

#include "base/base_macros.h" //Needed by struct_string.h and struct_var_array.h
#include "base/base_assert.h" //Needed by struct_string.h and struct_var_array.h
#include "base/base_char.h" //Needed by struct_string.h

#include "struct/struct_string.h"

#include "mem/mem_arena.h" //Needed by struct_var_array.h
#include "std/std_memset.h" //Needed by struct_var_array.h

#include "struct/struct_var_array.h"

#include "struct/struct_handmade_math_include.h" //Needed by struct_vectors.h, struct_quaternion.h, and struct_matrices.h

#include "struct/struct_vectors.h"
#include "struct/struct_quaternion.h"
#include "struct/struct_matrices.h"

#endif //  _STRUCT_ALL_H
