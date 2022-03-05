/***********************************************************************
 * pas_library.h
 * Definitions of the arguments of the Pascal run-time library
 *
 *   Copyright (C) 2008-2009, 2021-2022 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ***********************************************************************/

#ifndef __PAS_LIBRARY_H
#define __PAS_LIBRARY_H

/***********************************************************************/
/* Codes for runtime library interfaces.  These must be confined to the
 * range 0x0000 through 0xffff.
 */

/* Exit processing.
 *
 *   procedure hist(exitCode : integer);
 *
 * ON INPUT:
 *   TOS(0) - Exit code
 * ON RETURN:
 *   Does not return
 */

#define lbEXIT          (0x0000)

/* Heap allocation:
 *
 *   function new(size : integer) : integer;
 *
 * ON INPUT:
 *   TOS(0) - Size of the heap region to create
 *
 * ON RETURN:
 *   TOS(0) - The allocated heap region
 */

#define lbNEW           (0x0001)

/* Dispose of a previous heap allocation:
 *
 *   procedure despose(VAR alloc : integer);
 *
 * ON INPUT:
 *   TOS(0) - Address of the heap region to dispose of
 *
 * ON RETURN:
 *   No value is returned
 */

#define lbDISPOSE       (0x0002)

/* Get an environment string.
 *
 *   function getenv(name : string) : string;
 *
 * ON INPUT:
 *   TOS(0) = Address of variable name string
 *   TOS(1) = Length of variable name string
 * ON RETURN:
 *   TOS(0) = Address of variable value string
 *   TOS(1) = Length of variable value string
 */

#define lbGETENV        (0x0003)

/* Copy pascal string to a pascal string
 *
 *   procedure strcpy(src : string; var dest : string)
 *
 * ON INPUT:
 *   TOS(0) = Address of dest string variable
 *   TOS(1) = String buffer size
 *   TOS(2) = Pointer to source string buffer
 *   TOS(3) = Length of source string
 *
 * And in the indexed case:
 *
 *   TOS(4) = Dest string variable address offset
 *
 * NOTE:  The alternate versions are equivalent but have the dest address
 * and source string reversed.
 */

#define lbSTRCPY        (0x0004)
#define lbSTRCPY2       (0x0005)

#define lbSTRCPYX       (0x0006)
#define lbSTRCPYX2      (0x0007)

/* Copy binary file character array to a pascal string.  Used when a non-
 * indexed PACKED ARRAY[] OF CHAR appears as a factor in an RVALUE.
 *
 *   function bstr2str(arraySize : Integer, arrayAddress : Integer) : String;
 *
 * ON INPUT:
 *   TOS(0) = Array address
 *   TOS(1) = Array size
 * ON RETURN:
 *   TOS(0) = String character buffer size
 *   TOS(1) = String character buffer address
 *   TOS(2) = String size
 */

#define lbBSTR2STR      (0x0008)

/* Copy a pascal string into a binary file character array.  Use when a non-
 * indexed PACKED ARRAY[] OF CHAR appears as the LVALUE in an assignment.
 *
 *   procedure str2bstr(arraySize : Integer, arrayAddress : Integer,
 *                      source : String);
 *
 * ON INPUT:
 *   TOS(0) = Address of the array (destination)
 *   TOS(1) = Size of the array
 *   TOS(2) = Size of the alloated string buffer (source)
 *   TOS(3) = Address of the string buffer address
 *   TOS(4) = Size of the string
 * ON RETURN:
 *   All inputs consumbed
 */

#define lbSTR2BSTR      (0x0009)

/* Copy a pascal string into a binary file character array.  Use when a non-
 * indexed PACKED ARRAY[] OF CHAR appears within an array element (using as
 * a field of an array of records) as the LVALUE in an assignment.
 *
 *   function str2bstr(arraySize : Integer, arrayAddress : Integer,
 *                     source : String, offset : Integer);
 *
 * ON INPUT:
 *   TOS(0) = Address of the array (destination)
 *   TOS(1) = Size of the array
 *   TOS(2) = Size of the string buffer (source)
 *   TOS(3) = Address of the string buffer
 *   TOS(4) = Size of the string
 *   TOS(5) = Array address offset
 * ON RETURN:
 *   All inputs consumed
 */

#define lbSTR2BSTRX     (0x000a)

/* Initialize a new string variable. Create a string buffer.  This is
 * called only at entrance into a new Pascal block.
 *
 *   TYPE
 *     string : string[size]
 *
 *   procedure strinit(VAR str : string);
 *
 * ON INPUT
 *   TOS(0) = address of the newly string variable to be initialized
 *   TOS(1) = size of the string memory allocation
 * ON RETURN
 */

#define lbSTRINIT       (0x000b)

/* Initialize a temporary string variable on the stack. This is similar to
 * lbSTRINIT except that the form of its arguments are different.  This
 * is currently used only when calling a function that returns a string in
 * order to catch the returned string value in an initialized container.
 *
 *   function strtmp : string;
 *
 * ON INPUT
 * ON RETURN
 *   TOS(0) = Size of the allocated string buffer
 *   TOS(1) = Pointer to the string buffer
 *   TOS(2) = String size (zero)
 */

#define lbSTRTMP        (0x000c)

/* Replace a string with a duplicate string residing in allocated
 * string stack.
 *
 *   function strdup(name : string) : string;
 *
 * ON INPUT
 *   TOS(0) = Allocation size of original string
 *   TOS(1) = Pointer to original string
 *   TOS(2) = Length of original string
 * ON RETURN
 *   TOS(0) = Allocation size of new string (set to default string size)
 *   TOS(1) = Pointer to new string
 *   TOS(2) = Length of new string
 */

#define lbSTRDUP        (0x000d)

/* Replace a character with a string residing in allocated string stack
 * memory.
 *
 *   function mkstkc(c : char) : string;
 *
 * ON INPUT
 *   TOS(0) = Character value
 * ON RETURN
 *   TOS(0) = Size of new string buffer
 *   TOS(1) = Pointer to new string buffer
 *   TOS(2) = Length of new string
 */

#define lbMKSTKC        (0x000e)

/* Concatenate a string to the end of a string.
 *
 *   function strcat(string1 : string, string2 : string) : string;
 *
 * ON INPUT
 *   TOS(0) = string1 allocation size
 *   TOS(1) = pointer to source string1 data
 *   TOS(2) = length of source string1
 *   TOS(3) = string2 allocation size
 *   TOS(4) = pointer to dest string2 data
 *   TOS(5) = length of dest string2
 * ON OUTPUT
 *   TOS(0) = string2 allocation size (unchanged)
 *   TOS(1) = pointer to dest string2 (unchanged)
 *   TOS(2) = new length of dest string2
 */

#define lbSTRCAT        (0x000f)

/* Concatenate a character to the end of a string.
 *
 *   function strcatc(name : string, c : char) : string;
 *
 * ON INPUT
 *   TOS(0) = character to concatenate
 *   TOS(1) = string allocation (unchanged)
 *   TOS(2) = pointer to string allocation
 *   TOS(3) = length of string
 * ON OUTPUT
 *   TOS(0) = string allocation (unchanged)
 *   TOS(1) = pointer to string allocation (unchanged)
 *   TOS(2) = new length of string
 */

#define lbSTRCATC       (0x0010)

/* Compare two pascal strings
 *
 *   function strcmp(name1 : string, name2 : string) : integer;
 *
 * ON INPUT
 *   TOS(0) = Size of string2 allocation
 *   TOS(1) = Address of string2 data
 *   TOS(2) = Length of string2
 *   TOS(3) = Size of string1 allocation
 *   TOS(4) = Address of string1 data
 *   TOS(5) = Length of string1
 * ON OUTPUT
 *   TOS(0) = (-1=less than, 0=equal, 1=greater than}
 */

#define lbSTRCMP        (0x0011)

/* Borland-style string operations ******************************************/

/* Copy a substring from a string.
 *
 *   Copy(from : string, from, howmuch: integer) : string
 *
 * ON INPUT
 *   TOS(0) = Integer value that provides the length of the substring
 *   TOS(1) = Integer value that provides the (1-based) string position
 *   TOS(2) = Size of string buffer
 *   TOS(3) = Address of string data
 *   TOS(4) = Length of the string
 * ON OUTPUT
 *   TOS(0) = Size of string buffer
 *   TOS(1) = Address of the substring data
 *   TOS(2) = Length of the substring
 */

#define lbCOPYSUBSTR    (0x0012)

/* Find a substring in a string.  Returns the (1-based) character position of
 * the substring or zero if the substring is not found.
 *
 *   Pos(substr, s : string, start : integer) : integer
 *
 * ON INPUT
 *   TOS(0) = Start position
 *   TOS(1) = Size of string buffer
 *   TOS(2) = Address of string buffer
 *   TOS(3) = Length of the string
 *   TOS(4) = Size of substring buffer
 *   TOS(5) = Address of substring data
 *   TOS(6) = Length of the substring
 * ON OUTPUT
 *   TOS(0) = Position of the substring (or zero if not present)
 */

#define lbFINDSUBSTR    (0x0013)

/* Insert a string into another string.
 *
 *   Insert(source : string, VAR target : string, index: integer) : string
 *
 * ON INPUT
 *   TOS(0) = Integer value that provides the (1-based) string position
 *   TOS(1) = Address of the target string to be modified
 *   TOS(2) = Size of source string buffer
 *   TOS(3) = Address of source string buffer
 *   TOS(4) = Length of the source string
 * ON OUTPUT
 */

#define lbINSERTSTR     (0x0014)

/* Delete a substring from a string.
 *
 *   Delete(VAR from : string; from, howmuch: integer) : string
 *
 * ON INPUT
 *   TOS(0) = Integer value that provides the length of the substring
 *   TOS(1) = Integer value that provides the (1-based) string position
 *   TOS(2) = Address of string variable to be modified
 * ON OUTPUT
 */

#define lbDELSUBSTR     (0x0015)

/* Fill string s with character value until s is count-1 char long.
 *
 *   fillchar(s : string; count : integer; value : shortword)
 *
 * ON INPUT
 *   TOS(0) = Integer 'value' value
 *   TOS(1) = Integer 'count' value
 *   TOS(2) = Address of string variable
 * ON OUTPUT
 */

#define lbFILLCHAR      (0x0016)

/* Convert a numeric value to a string
 *
 * ON INPUT
 *   TOS(0)   = Address of the string
 *   TOS(1)   = Field width
 *   TOS(2-n) = Numeric value.  The actual length varies with type.
 * ON OUTPUT
 */

#define lbINTSTR        (0x0017)
#define lbWORDSTR       (0x0018)
#define lbLONGSTR       (0x0019)
#define lbULONGSTR      (0x001a)
#define lbREALSTR       (0x001b)

/* Convert a string to a numeric value
 *
 *   procedure val(const s : string; VAR v : integer; VAR code : word);
 *
 * Description:
 * val() converts the value represented in the string S to a numerical
 * value, and stores this value in the variable V, which can be of type
 * Integer, Longinteger, ShortInteger, or Real. If the conversion isn't
 * succesful, then the parameter Code contains the index of the character
 * in S which prevented the conversion. The string S is allowed to contain
 * spaces in the beginning.
 *
 * The string S can contain a number in decimal, hexadecimal, binary or
 * octal format, as described in the language reference.
 *
 * Errors:
 * If the conversion doesn't succeed, the value of Code indicates the
 * position where the conversion went wrong.
 *
 * ON INPUT
 *   TOS(0) = address of Code
 *   TOS(1) = address of v
 *   TOS(2) = Source string buffer size
 *   TOS(3) = Pointer to source string buffer
 *   TOS(4) = Length of source string
 * ON RETURN: actual parameters released
 */

#define lbVAL           (0x001c)

#define MAX_LBOP        (0x001d)

#endif /* __PAS_LIBRARY_H */
