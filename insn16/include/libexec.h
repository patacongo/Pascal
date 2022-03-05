/**********************************************************************************
 * libexec.h
 *
 *   Copyright (C) 2008, 2021-2022 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are metPU
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
 **********************************************************************************/

#ifndef __LIBEXEC_H
#define __LIBEXEC_H

/**********************************************************************************
 * Included Files
 **********************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include "pas_machine.h"

/**********************************************************************************
 * Pre-processor Definitions
 **********************************************************************************/

/* Sanity Check.  NOTE:  This only check 16-, 32-, and 64-bit pointer sizes. */

#if UINTPTR_MAX == 0xffff && CONFIG_PASCAL_POINTERSIZE != 2
#  error CONFIG_PASCAL_POINTERSIZE must be 2
#elif UINTPTR_MAX == 0xffffffff && CONFIG_PASCAL_POINTERSIZE != 4
#  error CONFIG_PASCAL_POINTERSIZE must be 4
#elif UINTPTR_MAX == 0xffffff && CONFIG_PASCAL_POINTERSIZE != 6
#  error CONFIG_PASCAL_POINTERSIZE must be 6
#elif UINTPTR_MAX == 0xffffffffffffffffu && CONFIG_PASCAL_POINTERSIZE != 8
#  error CONFIG_PASCAL_POINTERSIZE must be 8
#endif

/* Pascal stack macros */

#define BPERI         2
#define ITOBSTACK(i)  ((i) << 1)
#define BTOISTACK(i)  ((i) >> 1)
#define ROUNDBTOI(i)  (((i) + 1) >> 1)

/* INPUT/OUTPUT file numbers */

#define INPUT_FILE_NUMBER   0
#define OUTPUT_FILE_NUMBER  1

/* Remove the value from the top of the stack */

#define POP(st, dest) \
  do { \
    dest = (st)->dstack.i[BTOISTACK((st)->sp)]; \
    (st)->sp -= BPERI; \
  } while (0)

/* Add the value to top of the stack */

#define PUSH(st, src) \
  do { \
    (st)->sp += BPERI; \
    (st)->dstack.i[BTOISTACK((st)->sp)] = src; \
  } while (0)

/* Return an rvalue for the (word) offset from the top of the stack */

#define TOS(st, off) \
  (st)->dstack.i[BTOISTACK((st)->sp)-(off)]

/* Save the src (word) at the dest (word) stack position */

#define PUTSTACK(st, src, dest)  \
  do { \
    (st)->dstack.i[BTOISTACK(dest)] = src; \
  } while (0)

/* Return an rvalue for the (word) from the absolute stack position */

#define GETSTACK(st, src) \
  (st)->dstack.i[BTOISTACK(src)]

/* Store a byte to an absolute (byte) stack position */

#define PUTBSTACK(st, src, dest) \
  do { \
    (st)->dstack.b[dest] = src; \
  } while (0)

/* Return an rvalue for the absolute (byte) stack position */

#define GETBSTACK(st, src) \
  (st)->dstack.b[src]

/* Return the address for an absolute (byte) stack position. */

#define ATSTACK(st, src) \
  &(st)->dstack.b[src]

/* Discard n words from the top of the stack */

#define DISCARD(st, n) \
  do { \
    (st)->sp -= BPERI*(n); \
  } while (0)

/* Debug monitor capacities */

#define TRACE_ARRAY_SIZE       16
#define MAX_BREAK_POINTS        8
#define MAX_WATCH_POINTS        1
#define DISPLAY_STACK_SIZE     16
#define DISPLAY_INST_SIZE      16

/**********************************************************************************
 * Public Type Definitions
 **********************************************************************************/

typedef uint16_t ustack_t;   /* Stack values are 16-bits in length */
typedef int16_t  sstack_t;
typedef uint16_t level_t;    /* Limits to UINT16_MAX levels */

/* Memory management types */

typedef struct memChunk_s  memChunk_t;
typedef struct freeChunk_s freeChunk_t;

union stack_u
{
  ustack_t *i;
  uint8_t  *b;
};
typedef union stack_u stackType_t;

/* Floating point */

union fparg_u
{
  double   f;
  uint16_t hw[4];
};

typedef union fparg_u fparg_t;

/* This enum and structure maintain the state of one file */

enum openMode_e
{
  eOPEN_NONE = 0,
  eOPEN_READ,
  eOPEN_WRITE,
  eOPEN_APPEND
};

typedef enum openMode_e openMode_t;

struct execFileTable_s
{
  char fileName[FNAME_SIZE + 1];
  bool inUse;
  bool text;
  bool eoln;
  uint16_t recordSize;
  FILE *stream;
  openMode_t openMode;
};

typedef struct execFileTable_s execFileTable_t;

#ifdef CONFIG_PASCAL_DEBUGGER
/* The following enums and structures are used to support the run-time debug monitor.
 */

enum command_e
{
  eCMD_NONE = 0,
  eCMD_RESET,
  eCMD_RUN,
  eCMD_STEP,
  eCMD_NEXT,
  eCMD_GO,
  eCMD_BS,
  eCMD_BC,
  eCMD_WS,
  eCMD_WC,
  eCMD_DP,
  eCMD_DT,
  eCMD_DS,
  eCMD_DI,
  eCMD_DB,
  eCMD_HELP,
  eCMD_QUIT
};

struct trace_s
{
  pasSize_t pc;
  pasSize_t sp;
  ustack_t  tos;
  ustack_t  wp;
};

typedef struct trace_s trace_t;
#endif

/* This structure describes the parameters needed to initialize the p-code
 * interpreter.
 */

struct libexec_attr_s
{
  /* Instruction space (I-Space) */

  uint8_t  *ispace;   /* Allocated I-Space containing p-code data */
  pasSize_t entry;    /* Entry point */
  pasSize_t maxpc;    /* Last valid p-code address */

  /* Read-only data block */

  uint8_t  *rodata;   /* Address of read-only data block */
  pasSize_t roSize;   /* Size of read-only data block */

  /* Allocate for variable storage */

  pasSize_t strSize;  /* String storage size */
  pasSize_t stkSize;  /* Pascal stack size */
  pasSize_t hpSize;   /* Heap storage size */
};

/* This structure defines the current state of the p-code interpreter.  It
 * includes the simulated CPU registers and memory map information.
 *
 * In order to have multiple instances of the pascal-runtime active at once
 * (and, therefore, multi-threaded Pascal), we have to maintain *all* global
 * data in this structure.  An instance of libexec_t is passed to most
 * functions as a parameter in the run-time implementation.
 */

struct libexec_s
{
  /* This is the emulated P-Machine stack (D-Space) */

  stackType_t dstack;

  /* This is the emulated P-Machine instruction space (I-Space) */

  uint8_t  *ispace;

 /* Address of last valid P-Code */

  pasSize_t maxpc;

  /* These are the emulated P-Machine registers:
   *
   * spb: Base of the stack
   * sp:  The Pascal stack pointer
   * csp: The current top of the stack used to manage string
   *      storage
   * hpb: Base of the heap
   * hsp: Heap stack pointer
   * fp:  Base Register of the current stack frame.  Holds the address
   *      of the base of the stack frame of the current block.
   * fop: Pointer to section containing read-only data
   * pc:  Holds the current p-code location
   */

  pasSize_t spb;        /* Pascal stack base */
  pasSize_t sp;         /* Pascal stack pointer */
  pasSize_t csp;        /* Character stack pointer */
  pasSize_t hpb;        /* Base of the heap */
  pasSize_t hsp;        /* Heap stack pointer */
  pasSize_t fp;         /* Base of the current frame */
  pasSize_t rop;        /* Read-only data pointer */
  pasSize_t pc;         /* Program counter */
  pasSize_t lsp;        /* Static nesting level */

  /* Info needed to perform a simulated reset.  Memory organization:
   *
   *  0                                   : String stack
   *  strSize                             : RO-only data
   *  strSize + roSize                    : "Normal" Pascal stack
   *  strSize + roSize + stkSize          : Heap stack
   *  strSize + roSize + stkSize + hpSize : "Normal" Pascal stack
   */

  pasSize_t strSize;    /* String stack size */
  pasSize_t roSize;     /* Read-only stack size */
  pasSize_t stkSize;    /* Pascal stack size */
  pasSize_t hpSize;     /* Heap stack size */
  pasSize_t stackSize;  /* Total memory allocation */

  pasSize_t entry;      /* Entry point */
  int16_t   exitCode;

  /* Memory management */

  freeChunk_t *freeChunks;

  /* File I/O */

  execFileTable_t fileTable[MAX_OPEN_FILES];
  uint8_t ioBuffer[LINE_SIZE + 1];

#ifdef CONFIG_PASCAL_DEBUGGER
  /* Debug monitor */

 enum command_e lastCmd;   /* Used to repeat last command on ENTER */
 uint32_t   lastValue;     /* Value associated with lastCmd */
 trace_t    traceArray[TRACE_ARRAY_SIZE];
                           /* Holds execution history */
 uint16_t   traceIndex;    /* This is the index into the circular traceArray */
 uint16_t   nTracePoints;  /* This is the number of valid enties in traceArray */
 pasSize_t  breakPoint[MAX_BREAK_POINTS];
                           /* Contains address associated with all active
                            * break points. */
 pasSize_t  watchPoint[MAX_WATCH_POINTS];
                           /* Contains address associated with all active
                            * watch points. */
 pasSize_t  untilPoint;    /* The 'untilPoint' is a temporary breakpoint */
 uint16_t   nBreakPoints;  /* Number of items in breakPoint[] */
 uint16_t   nWatchPoints;  /* Number of items in watchPoint[] */
 bool       bExecStop;     /* true means to stop program execution */
 char       cmdLine[LINE_SIZE + 1];
                           /* Command line buffer */
#endif
};

/**********************************************************************************
 * Public Function Prototypes
 **********************************************************************************/

struct libexec_s *libexec_Initialize(struct libexec_attr_s *attr);
int    libexec_Execute(struct libexec_s *st);
void   libexec_Reset(struct libexec_s *st);

#endif /* __LIBEXEC_H */
