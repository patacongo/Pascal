/***********************************************************************
 * pas_machine.h
 * Characteristics of the PCODE machine
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

#ifndef __PAS_MACHINE_H
#define __PAS_MACHINE_H

/***********************************************************************
 * Included Files
 ***********************************************************************/

#include <stdint.h>
#include <stdio.h>   /* for FILE */

#include "config.h"

/***********************************************************************
 * Pre-processor Definitions
 ***********************************************************************/

/* Common Sizing Parameters */

#define FNAME_SIZE          40           /* Max size file name */
#define LINE_SIZE           256          /* Max size of input line buffer */
#define MAX_OPEN_FILES      8            /* Max number of open file */

/* Target P-Machine Data Storage Sizes.  Currently assumes 16-bit machine */

/* Sizes of integer types */

#define sINT_SIZE           2
#define MAXINT              32767
#define MININT             -32768
#define BITS_IN_INTEGER     16

#define sWORD_SIZE          2
#define MAXWORD             0xffff
#define MINWORD             0

/* Align to integer stack boundaries */

#define INT_ALIGNUP(n)      (((n) + sINT_SIZE - 1) & ~(sINT_SIZE - 1))
#define INT_ALIGNDOWN(n)    ((n) & ~sINT_SIZE)
#define INT_ISALIGNED(n)    (((n) & (sINT_SIZE - 1)) == 0)

#define sSHORTINT_SIZE      1
#define MAXSHORTINT         127
#define MINSHORTINT         -128

#define sSHORTWORD_SIZE     1
#define MAXSHORTWORD        255
#define MINSHORTWORD        0

#define sLONGINT_SIZE       4
#define MAXLONGINT          2147483647L
#define MINLONGINT          -2147483648L

#define sLONGWORD_SIZE      4
#define MAXLONGWORD         4294967295UL
#define MINLONGWORD         0

/* Sizes of other Pascal types */

#define sCHAR_SIZE          1
#define sBOOLEAN_SIZE       sINT_SIZE
#define sREAL_SIZE          8
#define sPTR_SIZE           sINT_SIZE

#define sSET_SIZE           8
#define sSET_WORDS         (sSET_SIZE / sINT_SIZE)
#define sSET_MAXELEM       (8 * sSET_SIZE)

/* Pascal string variables consist of:
 *
 * - A variable size, string buffer, and
 * - A small string variable that includes the size of the string and a
 *   pointer to the string buffer.  It must always appear on the stack
 *   in this order.
 *
 *      TOS(n)     = Maximum string size.  Allocated size of string data.
 *      TOS(n + 1) = 16-bit pointer to the string data.
 *      TOS(n + 2) = Current string size
 *
 * The string is usually access via opLDSM or opSTSM instructions.  The
 * relation ship between the ordering is confusing.  It is a push up stack.
 * when the memory is copied onto the stack:
 *
 *    STORAGE LOCATION         -> STACK
 *      Size       (Offset 0)  -> TOS(2)
 *      Pointer    (Offset 2)  -> TOS(1)
 *      Allocation (Offset 4)  -> TOS(0)
 *
 * The allocation size is pushed, then the string size is pushed and finally
 * the pointer is pushed.  The order is retained in memory, but the notation
 * is confusing.
 */

#define sSTRING_SIZE         (sINT_SIZE + sPTR_SIZE + sINT_SIZE)
#define sSTRING_SIZE_OFFSET  (0)         /* Byte offset to string size */
#define sSTRING_DATA_OFFSET  (sINT_SIZE) /* Byte offset to buffer pointer */
#define sSTRING_ALLOC_OFFSET (sINT_SIZE + sPTR_SIZE)

/* Default size of the string buffer allocation.  Used only when a string
 * variable is declared with no size.
 */

#define STRING_BUFFER_SIZE   INT_ALIGNUP(CONFIG_PASCAL_DEFAULT_STRALLOC)

/* BOOLEAN constant values */

#define PASCAL_TRUE        (-1)
#define PASCAL_FALSE        0

/* Range of unsigned character type */

#define MAXCHAR             255
#define MINCHAR             0

/* Size of a target hardware pointer in units of 16-bit stack words */

#define PASCAL_POINTERWORDS \
  (INT_ALIGNUP(CONFIG_PASCAL_POINTERSIZE) / sINT_SIZE)

/***********************************************************************
 * Public Types
 ***********************************************************************/

/* Type big enough to hold a the largest memory object. */

typedef uint16_t pasSize_t;  /* Addresses are 16-bits in length */

/* Representation of one P-Code.  Depends on INSN */

struct opType_s
{
  uint8_t  op;    /* Instruction opcode */
  uint8_t  arg1;  /* 8-bit instruction argument */
  uint16_t arg2;  /* 16-bit instruction argument */
};

typedef struct opType_s opType_t;

/* The tgtPtr_t structure simply represents a target machine pointer to
 * allocated memory.  It is represented by a structure only due to alignment
 * issues.
 */

union tgtPtr_u
{
  void    *ptr;
  uint16_t b[PASCAL_POINTERWORDS];
};

typedef union tgtPtr_u tgtPtr_t;

#endif /* __PAS_MACHINE_H */
