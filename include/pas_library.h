/***********************************************************************
 * pas_library.h
 * Definitions of the arguments of the Pascal run-time library
 *
 *   Copyright (C) 2008-2009, 2021 Gregory Nutt. All rights reserved.
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
 *   function getent(name : string) : cstring;
 * ON INPUT:
 *   TOS(0) = length of string
 *   TOS(1) = pointer to string
 * ON RETURN:  actual parameters released
 *   TOS(0,1) = 32-bit absolute address of string
 */

#define lbGETENV        (0x0003)

/* Copy pascal string to a pascal string
 *
 *   procedure strcpy(src : string; var dest : string)
 *
 * ON INPUT:
 *   TOS(0) = Address of dest string variable
 *   TOS(1) = Pointer to source string buffer
 *   TOS(2) = Length of source string
 *
 * And in the indexed case:
 *
 *   TOS(3)=Dest short string variable address offset
 *
 * ON RETURN: actual parameters released.
 *
 * NOTE:  The alternate versions are equivalent but have the dest address
 * and source string reversed.
 */

#define lbSTRCPY        (0x0004)
#define lbSTRCPY2       (0x0005)

#define lbSTRCPYX       (0x0006)
#define lbSTRCPYX2      (0x0007)

/* Copy a pascal short string to a pascal short string.  Stack
 * on entry must be:
 *
 *   TOS(0)=Address of dest short short string variable
 *   TOS(1)=Short string buffer size
 *   TOS(2)=Pointer to source short string buffer
 *   TOS(3)=Length of source short string
 *
 * And in the indexed case:
 *
 *   TOS(4)=Dest short string variable address offset
 *
 * NOTE:  The alternate versions are equivalent but have the dest address
 * and source string reversed.
 */

#define lbSSTRCPY       (0x0008)
#define lbSSTRCPY2      (0x0009)

#define lbSSTRCPYX      (0x000a)
#define lbSSTRCPYX2     (0x000b)

/* Copy a pascal short string to a standard string.  Stack on entry must be:
 *
 *   TOS(0)=Address of dest standard standard string variable
 *   TOS(1)=Short string buffer size
 *   TOS(2)=Pointer to source short string buffer
 *   TOS(3)=Length of source short string
 *
 * And in the indexed case:
 *
 *   TOS(4)=Dest standard string variable address offset
 *
 * NOTE:  The alternate versions are equivalent but have the dest address
 * and source string reversed.
 */

#define lbSSTR2STR      (0x000c)
#define lbSSTR2STR2     (0x000d)

#define lbSSTR2STRX     (0x000e)
#define lbSSTR2STRX2    (0x000f)

/* Copy a pascal standard string to a short string.  Stack on entry must be:
 *
 *   TOS(0)=Address of dest short short string variable
 *   TOS(1)=Pointer to source standard string buffer
 *   TOS(2)=Length of source standard string
 *
 * And in the indexed case:
 *
 *   TOS(3)=Dest short string variable address offset
 *
 * NOTE:  The alternate versions are equivalent but have the dest address
 * and source string reversed.
 */

#define lbSTR2SSTR      (0x0010)
#define lbSTR2SSTR2     (0x0011)

#define lbSTR2SSTRX     (0x0012)
#define lbSTR2SSTRX2    (0x0013)

/* Copy C string to a pascal standard string.  Stack on entry must be:
 *
 *   procedure cstr2str(src : cstring; var dest : string)
 *
 * ON INPUT:
 *   TOS(0)   = address of dest standard string
 *   TOS(1,2) = 32-bit absolute address of C string (big-endian)
 *
 * And in the indxed case:
 *
 *   TOS(3)   = Dest standard string variable address offset
 *
 * ON RETURN: actual parameters released
 *
 * NOTE:  The alternate versions are equivalent but have the dest address
 * and source string reversed.
 */

#define lbCSTR2STR      (0x0014)
#define lbCSTR2STR2     (0x0015)

#define lbCSTR2STRX     (0x0016)
#define lbCSTR2STRX2    (0x0017)

/* Copy C string to a pascal short string.  Stack on entry must be:
 *
 *   procedure cstr2sstr(src : cstring; var dest : shortstring)
 *
 * ON INPUT:
 *   TOS(0)   = address of dest short string
 *   TOS(1,2) = 32-bit absolute address of C string (big-endian)
 *
 * And in the indxed case:
 *
 *   TOS(3)   = Dest short string variable address offset
 *
 * ON RETURN: actual parameters released
 *
 * NOTE:  The alternate versions are equivalent but have the dest address
 * and source string reversed.
 */

#define lbCSTR2SSTR     (0x0018)
#define lbCSTR2SSTR2    (0x0019)

#define lbCSTR2SSTRX    (0x001a)
#define lbCSTR2SSTRX2   (0x001b)

/* Copy binary file character array to a pascal string.  Used when a non-
 * indexed PACKED ARRAY[] OF CHAR appears as a factor in an RVALUE.
 *
 *   function bstr2str(arraySize : Integer, arrayAddress : Integer) : String;
 *
 * ON INPUT:
 *   TOS(0) = Array address
 *   TOS(1) = Array size
 * ON RETURN:
 *   TOS(0) = String character buffer address
 *   TOS(1) = String size
 */

#define lbBSTR2STR      (0x001c)

/* Copy a pascal string into a binary file character array.  Use when a non-
 * indexed PACKED ARRAY[] OF CHAR appears as the LVALUE in an assignment.
 *
 *   procedure str2bstr(arraySize : Integer, arrayAddress : Integer,
 *                      source : String);
 *
 * ON INPUT:
 *   TOS(0) = Address of the array (destination)
 *   TOS(1) = Size of the array
 *   TOS(2) = Address of the string (source)
 *   TOS(3) = Size of the string
 * ON RETURN:
 *   All inputs consumbed
 */

#define lbSTR2BSTR      (0x001d)

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
 *   TOS(2) = Address of the string (source)
 *   TOS(3) = Size of the string
 *   TOS(4) = Array address offset
 * ON RETURN:
 *   All inputs consumbed
 */

#define lbSTR2BSTRX     (0x001e)

/* Convert a string to a numeric value
 *
 *   procedure val(const s : string; var v; var code : word);
 *
 * Description:
 * val() converts the value represented in the string S to a numerical
 * value, and stores this value in the variable V, which can be of type
 * Longint, Real and Byte. If the conversion isn¡Çt succesfull, then the
 * parameter Code contains the index of the character in S which
 * prevented the conversion. The string S is allowed to contain spaces
 * in the beginning.
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
 *   TOS(2) = length of source string
 *   TOS(3) = pointer to source string
 * ON RETURN: actual parameters released
 */

#define lbVAL           (0x001f)

/* Initialize a new string variable. Create a string buffer.  This is called
 * only at entrance into a new Pascal block.
 *
 *   procedure strinit(VAR str : string);
 *
 * ON INPUT
 *   TOS(0) = address of the newly string variable to be initialized
 * ON RETURN
 */

#define lbSTRINIT       (0x0020)

/* Initialize a new short string variable. Create a string buffer.  This is
 * called only at entrance into a new Pascal block.
 *
 *   TYPE
 *     shortstring : string[size]
 *   procedure sstrinit(VAR str : shortstring);
 *
 * ON INPUT
 *   TOS(0) = address of the newly string variable to be initialized
 *   TOS(1) = size of the short string memory allocation
 * ON RETURN
 */

#define lbSSTRINIT      (0x0021)

/* Initialize a temporary string variable on the stack. This is similar to
 * lbSTRINIT except that the form of its arguments are different.  This
 * is currently used only when calling a function that returns a string in
 * order to catch the returned string value in an initialized container.
 *
 *   function strtmp : string;
 *
 * ON INPUT
 * ON RETURN
 *   TOS(0) = Pointer to the string buffer on the stack.
 *   TOS(1) = String size (zero)
 */

#define lbSTRTMP        (0x0022)

/* Replace a standard string with a duplicate string residing in allocated
 * string stack.
 *
 *   function strdup(name : string) : string;
 *
 * ON INPUT
 *   TOS(0) = pointer to original standard string
 *   TOS(1) = length of original standard string
 * ON RETURN
 *   TOS(0) = pointer to new standard string
 *   TOS(1) = length of new standard string
 */

#define lbSTRDUP        (0x0023)

/* Replace a short string with a duplicate string residing in allocated
 * string stack.
 *
 *   function strdup(name : shortstring) : shortstring;
 *
 * ON INPUT
 *   TOS(0) = allocation of original short string
 *   TOS(1) = pointer to original short string
 *   TOS(2) = length of original short string
 * ON RETURN
 *   TOS(0) = allocation of new short string (unchanged)
 *   TOS(1) = pointer to new short string
 *   TOS(2) = length of new short string
 */

#define lbSSTRDUP       (0x0024)

/* Replace a character with a string residing in allocated string stack.
 *   function mkstkc(c : char) : string;
 * ON INPUT
 *   TOS(0) = Character value
 * ON RETURN
 *   TOS(0) = pointer to new string
 *   TOS(1) = length of new string
 */

#define lbMKSTKC        (0x0025)

/* Concatenate a standard string to the end of a standard string.
 *
 *   function strcat(string1 : string, string2 : string) : string;
 *
 * ON INPUT
 *   TOS(0) = pointer to source standard string1 data
 *   TOS(1) = length of source standard string1
 *   TOS(2) = pointer to dest standard string2 data
 *   TOS(3) = length of dest standard string2
 * ON OUTPUT
 *   TOS(0) = pointer to dest standard string2 (unchanged)
 *   TOS(1) = new length of dest standard string2
 */

#define lbSTRCAT        (0x0026)

/* Concatenate a short string to the end of a short string.
 *
 *   function sstrcat(string1 : shortstring, string2 : shortstring) : shortstring;
 *
 * ON INPUT
 *   TOS(0) = string1 allocation size
 *   TOS(1) = pointer to source short string1 data
 *   TOS(2) = length of source short string1
 *   TOS(3) = string2 allocation size
 *   TOS(4) = pointer to dest short string2 data
 *   TOS(5) = length of dest short string2
 * ON OUTPUT
 *   TOS(0) = string2 allocation size (unchanged)
 *   TOS(1) = pointer to dest short string2 (unchanged)
 *   TOS(2) = new length of dest short string2
 */

#define lbSSTRCAT       (0x0027)

/* Concatenate a standard string to the end of a short string.
 *
 *   function sstrcat(string1 : shortstring, string2 : standard) : shortstring;
 *
 * ON INPUT
 *   TOS(0) = pointer to source standard string2 data
 *   TOS(1) = length of source standard string2
 *   TOS(2) = string2 allocation size
 *   TOS(3) = pointer to dest short string1 data
 *   TOS(4) = length of dest short string1
 * ON OUTPUT
 *   TOS(0) = string2 allocation size (unchanged)
 *   TOS(1) = pointer to dest short string1 (unchanged)
 *   TOS(2) = new length of dest short string1
 */

#define lbSSTRCATSTR    (0x0028)

/* Concatenate a short string to the end of a standard string.
 *
 *   function sstrcat(string1 : shortstring, string2 : standard) : shortstring;
 *
 * ON INPUT
 *   TOS(0) = string1 allocation size
 *   TOS(1) = pointer to source short string1 data
 *   TOS(2) = length of source short string1
 *   TOS(3) = pointer to dest standard string1 data
 *   TOS(4) = length of dest standard string1
 * ON OUTPUT
 *   TOS(0) = pointer to dest standard string1 (unchanged)
 *   TOS(1) = new length of dest standard string1
 */

#define lbSTRCATSSTR    (0x0029)

/* Concatenate a character to the end of a standard string.
 *
 *   function strcatc(name : string, c : char) : string;
 *
 * ON INPUT
 *   TOS(0) = character to concatenate
 *   TOS(1) = pointer to standard string allocation
 *   TOS(2) = length of standard string
 * ON OUTPUT
 *   TOS(0) = pointer to standard string allocation (unchanged)
 *   TOS(1) = new length of standard string
 */

#define lbSTRCATC       (0x002a)

/* Concatenate a character to the end of a short string.
 *
 *   function strcatc(name : shortstring, c : char) : shortstring;
 *
 * ON INPUT
 *   TOS(0) = character to concatenate
 *   TOS(1) = short string allocation (unchanged)
 *   TOS(2) = pointer to short string allocation
 *   TOS(3) = length of short string
 * ON OUTPUT
 *   TOS(0) = short string allocation (unchanged)
 *   TOS(1) = pointer to short string allocation (unchanged)
 *   TOS(2) = new length of short string
 */

#define lbSSTRCATC      (0x002b)

/* Compare two pascal standard strings
 *
 *   function strcmp(name1 : string, name2 : string) : integer;
 *
 * ON INPUT
 *   TOS(0) = address of standard string2 data
 *   TOS(1) = length of standard string2
 *   TOS(2) = address of standard string1 data
 *   TOS(3) = length of standard string1
 * ON OUTPUT
 *   TOS(0) = (-1=less than, 0=equal, 1=greater than}
 */

#define lbSTRCMP        (0x002c)

/* Compare two pascal short strings
 *
 *   function sstrcmp(name1 : shortstring, name2 : shortstring) : integer;
 *
 * ON INPUT
 *   TOS(0) = size of short string2 allocation
 *   TOS(1) = address of short string2 data
 *   TOS(2) = length of short string2
 *   TOS(3) = size of short string1 allocation
 *   TOS(4) = address of short string1 data
 *   TOS(5) = length of short string1
 * ON OUTPUT
 *   TOS(0) = (-1=less than, 0=equal, 1=greater than}
 */

#define lbSSTRCMP       (0x002d)

/* Compare a pascal short string to a pascal standard string
 *
 *   function sstrcmpstr(name1 : shortstring, name2 : string) : integer;
 *
 * ON INPUT
 *   TOS(0) = address of standard string2 data
 *   TOS(1) = length of standard string2
 *   TOS(2) = size of short string1 allocation
 *   TOS(3) = address of short string1 data
 *   TOS(4) = length of short string1
 * ON OUTPUT
 *   TOS(0) = (-1=less than, 0=equal, 1=greater than}
 */

#define lbSSTRCMPSTR    (0x002e)

/* Compare a pascal standard string to a pascal short string
 *
 *   function sstrcmpstr(name1 : string, name2 : shortstring) : integer;
 *
 * ON INPUT
 *   TOS(0) = size of short string2 allocation
 *   TOS(1) = address of short string2 data
 *   TOS(2) = length of short string2
 *   TOS(3) = address of standard string1 data
 *   TOS(4) = length of standard string1
 * ON OUTPUT
 *   TOS(0) = (-1=less than, 0=equal, 1=greater than}
 */

#define lbSTRCMPSSTR    (0x002f)

#define MAX_LBOP        (0x0030)

#endif /* __PAS_LIBRARY_H */
