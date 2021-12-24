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

/* Halt processing.
 *   procedure halt;
 * ON INPUT:
 *   Takes no inputs
 * ON RETURN:
 *   Does not return
 */

#define lbHALT          (0x0000)

/* Get an environment string.
 *   function getent(name : string) : cstring;
 * ON INPUT:
 *   TOS(0)=length of string
 *   TOS(1)=pointer to string
 * ON RETURN:  actual parameters released
 *   TOS(0,1)=32-bit absolute address of string
 */

#define lbGETENV        (0x0001)

/* Copy pascal string to a pascal string
 *
 *   procedure strcpy(src : string; var dest : string)
 *
 * ON INPUT:
 *   TOS(0)=Address of dest string variable
 *   TOS(1)=Pointer to source string buffer
 *   TOS(2)=Length of source string
 * ON RETURN: actual parameters released.
 */

#define lbSTRCPY        (0x0002)

/* Copy pascal string to a element of a pascal string array
 *
 *   procedure strcpy(src : string; var dest : string; offset : integer)
 *
 * ON INPUT:
 *   TOS(0)=Address of dest string variable
 *   TOS(1)=Pointer to source string buffer
 *   TOS(2)=Length of source string
 *   TOS(3)=Dest string variable address offset
 * ON RETURN: actual parameters released.
 *
 * REVISIT:  This is awkward.  Life would be much easier if the
 * array index could be made to be emitted later in the stack and
 * so could be added to the dest sting variable address easily.
 */

#define lbSTRCPYX       (0x0003)

/* Copy C string to a pascal string
 *
 *   procedure cstr2str(src : cstring; var dest : string)
 *
 * ON INPUT:
 *   TOS(0)=address of dest string
 *   TOS(1,2)=32-bit absolute address of C string
 * ON RETURN: actual parameters released
 */

#define lbCSTR2STR      (0x0004)

/* Copy C string to an element of a pascal string array
 *
 *   procedure cstr2str(src : cstring; var dest : string)
 *
 * ON INPUT:
 *   TOS(0)=Address of dest string variable
 *   TOS(1,2)=32-bit absolute address of C string
 *   TOS(3)=Dest string variable address offset
 * ON RETURN: actual parameters released
 *
 * REVISIT:  This is awkward.  Life would be much easier if the
 * array index could be made to be emitted later in the stack and
 * so could be added to the dest sting variable address easily.
 */

#define lbCSTR2STRX     (0x0005)

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
 *   TOS(0)=address of Code
 *   TOS(1)=address of v
 *   TOS(2)=length of source string
 *   TOS(3)=pointer to source string
 * ON RETURN: actual parameters released
 */

#define lbVAL           (0x0006)

/* Initialize a new string variable. Create a string buffer.  This is called
 * only at entrance into a new Pascal block.
 *
 *   procedure strinit(VAR str : string);
 *
 * ON INPUT
 *   TOS(0)=address of the newly string variable to be initialized
 * ON RETURN
 */

#define lbSTRINIT       (0x0007)

/* Initialize a temporary string variable on the stack. It is similar to
 * lbSTRINIT except that the form of its arguments are different.  This
 * is currently used only when calling a function that returns a string in
 * order to catch the returned string value in an initialized container.
 *
 *   function strtmp : string;
 *
 * ON INPUT
 * ON RETURN
 *   TOS(0)=Pointer to the string buffer on the stack.
 *   TOS(1)=String size (zero)
 */

#define lbSTRTMP        (0x0008)

/* Replace a string with a duplicate string residing in allocated
 * string stack.
 *
 *   function strdup(name : string) : string;
 *
 * ON INPUT
 *   TOS(0)=pointer to original string
 *   TOS(1)=length of original string
 * ON RETURN
 *   TOS(9)=pointer to new string
 *   TOS(1)=length of new string
 */

#define lbSTRDUP        (0x0009)

/* Replace a character with a string residing in allocated string stack.
 *   function mkstkc(c : char) : string;
 * ON INPUT
 *   TOS(0)=Character value
 * ON RETURN
 *   TOS(0)=pointer to new string
 *   TOS(1)=length of new string
 */

#define lbMKSTKC        (0x000a)

/* Concatenate a string to the end of a string.
 *
 *   function strcat(name : string1, c : char) : string;
 *
 * ON INPUT
 *   TOS(st, 0)=pointer to source string1 data
 *   TOS(st, 1)=length of source string1
 *   TOS(st, 2)=pointer to dest string2 data
 *   TOS(st, 3)=length of dest string2
 * ON OUTPUT
 *   TOS(st, 0)=pointer to dest string2 (unchanged)
 *   TOS(st, 1)=new length of dest string2
 */

#define lbSTRCAT        (0x000b)

/* Concatenate a character to the end of a string.
 *
 *   function strcatc(name : string, c : char) : string;
 *
 * ON INPUT
 *   TOS(0)=character to concatenate
 *   TOS(1)=pointer to string
 *   TOS(2)=length of string
 * ON OUTPUT
 *   TOS(0)=pointer to string
 *   TOS(1)=new length of string
 */

#define lbSTRCATC       (0x000c)

/* Compare two pascal strings
 *
 *   function strcmp(name1 : string, name2 : string) : integer;
 *
 * ON INPUT
 *   TOS(2)=address of string2 data
 *   TOS(1)=length of string2
 *   TOS(4)=address of string1 data
 *   TOS(3)=length of string1
 * ON OUTPUT
 *   TOS(0)=(-1=less than, 0=equal, 1=greater than}
 */

#define lbSTRCMP        (0x000d)

#define MAX_LBOP        (0x000e)

#endif /* __PAS_LIBRARY_H */
