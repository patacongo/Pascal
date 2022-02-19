/**********************************************************************
 * insn_disasm.c
 * P-Code Disassembler
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
 **********************************************************************/

/***********************************************************************
 * Included Files
 ***********************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#include "pas_debug.h"
#include "pas_pcode.h"
#include "insn16.h"
#include "pas_fpops.h"
#include "pas_sysio.h"
#include "pas_setops.h"
#include "pas_library.h"
#include "paslib.h"

#include "pas_insn.h"

/***********************************************************************
 * Pre-processor Definitions
 ***********************************************************************/

/* These are all the format codes that apply to opcodes */

#define NOARG8        0
#define SHORTINT      1    /* Show ARG8 as a signed integer */
#define SHORTWORD     2    /* Show ARG8 as an unsigned integer */
#define fpOP          3    /* Show ARG8 as encoded floating point operation */
#define setOP         4    /* Show ARG8 as encoded SET operation */

#define NOARG16       0
#define HEX           1    /* Show ARG16 as hexadecimal */
#define DECIMAL       2    /* Show ARG16 as a signed decimal */
#define UDECIMAL      3    /* Show ARG16 as an unsigned decimal */
#define LABEL_DEC     4    /* Show ARG16 as a label */
#define xOP           5    /* Show ARG16 as encoded SYSIO operation */
#define lbOP          6    /* Show ARG16 as encoded library function call */
#define COMMENT       7    /* This is a comment */

#define MKFMT(a8,a16) (((a16) << 3) | a8)
#define ARG8FMT(n)    ((n) & 0x07)
#define ARG16FMT(n)   ((n) >> 3)

/***********************************************************************
 * Private Data
 ***********************************************************************/

/* The following table defines everything that is needed to disassemble
 * a P-Code.  NOTE:  The order of definition in this table must exactly
 * match the declaration sequence in pas_insn.h. */

static const char invOp[] = "Invalid Opcode";
static const struct
{
  const char *opName;       /* Opcode mnemonics */
  uint8_t format;           /* arg16 format */
} opTable[256] =
{

/******************* OPCODES WITH NO ARGUMENTS ************************/
/* Program control (No stack arguments) */

/* 0x00 */ { "NOP  ",  MKFMT(NOARG8, NOARG16) },

/* Arithmetic & logical & and integer conversions (One stack argument) */

/* 0x01 */ { "NEG  ",  MKFMT(NOARG8, NOARG16) },
/* 0x02 */ { "ABS  ",  MKFMT(NOARG8, NOARG16) },
/* 0x03 */ { "INC  ",  MKFMT(NOARG8, NOARG16) },
/* 0x04 */ { "DEC  ",  MKFMT(NOARG8, NOARG16) },
/* 0x05 */ { "NOT  ",  MKFMT(NOARG8, NOARG16) },

/* Arithmetic & logical (Two stack arguments) */

/* 0x06 */ { "ADD  ",  MKFMT(NOARG8, NOARG16) },
/* 0x07 */ { "SUB  ",  MKFMT(NOARG8, NOARG16) },
/* 0x08 */ { "MUL  ",  MKFMT(NOARG8, NOARG16) },
/* 0x09 */ { "DIV  ",  MKFMT(NOARG8, NOARG16) },
/* 0x0a */ { "MOD  ",  MKFMT(NOARG8, NOARG16) },
/* 0x0b */ { "SLL  ",  MKFMT(NOARG8, NOARG16) },
/* 0x0c */ { "SRL  ",  MKFMT(NOARG8, NOARG16) },
/* 0x0d */ { "SRA  ",  MKFMT(NOARG8, NOARG16) },
/* 0x0e */ { "OR   ",  MKFMT(NOARG8, NOARG16) },
/* 0x0f */ { "AND  ",  MKFMT(NOARG8, NOARG16) },

/* Comparisons (One stack argument) */

/* 0x10 */ { "EQUZ ",  MKFMT(NOARG8, NOARG16) },
/* 0x11 */ { "NEQZ ",  MKFMT(NOARG8, NOARG16) },
/* 0x12 */ { "LTZ  ",  MKFMT(NOARG8, NOARG16) },
/* 0x13 */ { "GTEZ ",  MKFMT(NOARG8, NOARG16) },
/* 0x14 */ { "GTZ  ",  MKFMT(NOARG8, NOARG16) },
/* 0x15 */ { "LTEZ ",  MKFMT(NOARG8, NOARG16) },
/* 0x16 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x17 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Comparisons (Two stack arguments) */

/* 0x18 */ { "EQU  ",  MKFMT(NOARG8, NOARG16) },
/* 0x19 */ { "NEQ  ",  MKFMT(NOARG8, NOARG16) },
/* 0x1a */ { "LT   ",  MKFMT(NOARG8, NOARG16) },
/* 0x1b */ { "GTE  ",  MKFMT(NOARG8, NOARG16) },
/* 0x1c */ { "GT   ",  MKFMT(NOARG8, NOARG16) },
/* 0x1d */ { "LTE  ",  MKFMT(NOARG8, NOARG16) },
/* 0x1e */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x1f */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Load (One) or Store (Two stack argument) */

/* 0x20 */ { "LDI  ",  MKFMT(NOARG8, NOARG16) },
/* 0x21 */ { "LDIB ",  MKFMT(NOARG8, NOARG16) },
/* 0x22 */ { "ULDIB",  MKFMT(NOARG8, NOARG16) },
/* 0x23 */ { "LDIM ",  MKFMT(NOARG8, NOARG16) },
/* 0x24 */ { "STI  ",  MKFMT(NOARG8, NOARG16) },
/* 0x25 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x26 */ { "STIB ",  MKFMT(NOARG8, NOARG16) },
/* 0x27 */ { "STIM ",  MKFMT(NOARG8, NOARG16) },

/* Data stack operations */

/* 0x28 */ { "DUP  ",  MKFMT(NOARG8, NOARG16) },
/* 0x29 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x2a */ { "XCHG ",  MKFMT(NOARG8, NOARG16) },
/* 0x2b */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x2c */ { "PUSHS",  MKFMT(NOARG8, NOARG16) },
/* 0x2d */ { "POPS",   MKFMT(NOARG8, NOARG16) },
/* 0x2e */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x2f */ { "RET  ",  MKFMT(NOARG8, NOARG16) },

/* 0x30 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x31 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x32 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x33 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x34 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x35 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x36 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Unsigned arithmetic */

/* 0x37 */ { "UMUL ",  MKFMT(NOARG8, NOARG16) },
/* 0x38 */ { "UDIV ",  MKFMT(NOARG8, NOARG16) },
/* 0x39 */ { "UMOD ",  MKFMT(NOARG8, NOARG16) },

/* Unsigned comparisons */

/* 0x3a */ { "ULT  ",  MKFMT(NOARG8, NOARG16) },
/* 0x3b */ { "UGTE ",  MKFMT(NOARG8, NOARG16) },
/* 0x3c */ { "UGT  ",  MKFMT(NOARG8, NOARG16) },
/* 0x3d */ { "ULTE ",  MKFMT(NOARG8, NOARG16) },

/* More bitwise operators */

/* 0x3e */ { "XOR  ",  MKFMT(NOARG8, NOARG16) },

/* System functions */

/* 0x3f */ { "EXIT ",  MKFMT(NOARG8, NOARG16) },

/************** OPCODES WITH SINGLE BYTE ARGUMENT (arg8) ***************/

/* 0x40 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x41 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x42 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x43 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x44 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x45 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x46 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x47 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x48 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x49 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x4a */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x4b */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x4c */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x4d */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x4e */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x4f */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* 0x50 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x51 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x52 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x53 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x54 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x55 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x56 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x57 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x58 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x59 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x5a */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x5b */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x5c */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x5d */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x5e */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x5f */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Data stack:  arg8 = 8 bit unsigned data (no stack arguments) */

/* 0x60 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x61 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x62 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x63 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x64 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x65 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x66 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x67 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x68 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x69 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x6a */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x6b */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x6c */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x6d */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x6e */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x6f */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Floating Point Operations:  arg8 = FP op-code */

/* 0x70 */ { "FLOAT",  MKFMT(fpOP,   NOARG16) },
/* 0x71 */ { "SETOP",  MKFMT(setOP,  NOARG16) },
/* 0x72 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x73 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x74 */ { "PUSHB",  MKFMT(SHORTINT, NOARG16) },
/* 0x75 */ { "UPUSHB", MKFMT(SHORTWORD, NOARG16) },
/* 0x76 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x77 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* 0x78 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x79 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x7a */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x7b */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x7c */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x7d */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x7e */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x7f */ { invOp,    MKFMT(NOARG8, NOARG16) },

/************ OPCODES WITH SINGLE 16-BIT ARGUMENT (arg16) ************/

/* 0x80 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x81 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x82 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x83 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x84 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x85 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x86 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x87 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x88 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x89 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x8a */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x8b */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x8c */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x8d */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x8e */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x8f */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Program control:  arg16 = unsigned label (One stack argument) */

/* 0x90 */ { "JEQUZ",  MKFMT(NOARG8, HEX) },
/* 0x91 */ { "JNEQZ",  MKFMT(NOARG8, HEX) },
/* 0x92 */ { "JLTZ ",  MKFMT(NOARG8, HEX) },
/* 0x93 */ { "JGTEZ",  MKFMT(NOARG8, HEX) },
/* 0x94 */ { "JGTZ ",  MKFMT(NOARG8, HEX) },
/* 0x95 */ { "JLTEZ",  MKFMT(NOARG8, HEX) },

/* Program control:  arg16 = unsigned label (no stack arguments) */

/* 0x96 */ { "JMP  ",  MKFMT(NOARG8, HEX) },
/* 0x97 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Program control:  arg16 = unsigned label (One stack argument) */

/* 0x98 */ { "JEQU ",  MKFMT(NOARG8, HEX) },
/* 0x99 */ { "JNEQ ",  MKFMT(NOARG8, HEX) },
/* 0x9a */ { "JLT  ",  MKFMT(NOARG8, HEX) },
/* 0x9b */ { "JGTE ",  MKFMT(NOARG8, HEX) },
/* 0x9c */ { "JGT  ",  MKFMT(NOARG8, HEX) },
/* 0x9d */ { "JLTE ",  MKFMT(NOARG8, HEX) },
/* 0x9e */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x9f */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Data stack:  arg16 = 16 bit signed data (no stack arguments) */

/* Load:  arg16 = unsigned base offset (no stack arguments) */

/* 0xa0 */ { "LD   ",  MKFMT(NOARG8, UDECIMAL) },
/* 0xa1 */ { "LDB  ",  MKFMT(NOARG8, UDECIMAL) },
/* 0xa2 */ { "ULDB ",  MKFMT(NOARG8, UDECIMAL) },
/* 0xa3 */ { "LDM  ",  MKFMT(NOARG8, UDECIMAL) },

/* Store: arg16 = unsigned base offset (One stack arguments) */

/* 0xa4 */ { "ST   ",  MKFMT(NOARG8, UDECIMAL) },
/* 0xa5 */ { invOp,    MKFMT(NOARG8, NOARG16)  },
/* 0xa6 */ { "STB  ",  MKFMT(NOARG8, UDECIMAL) },
/* 0xa7 */ { "STM  ",  MKFMT(NOARG8, UDECIMAL) },

/* Load Indexed: arg16 = unsigned base offset (One stack arguments) */

/* 0xa8 */ { "LDX  ",  MKFMT(NOARG8, UDECIMAL) },
/* 0xa9 */ { "LDXB ",  MKFMT(NOARG8, UDECIMAL) },
/* 0xaa */ { "ULDXB",  MKFMT(NOARG8, UDECIMAL) },
/* 0xab */ { "LDXM ",  MKFMT(NOARG8, UDECIMAL) },

/* Store Indexed: arg16 = unsigned base offset (Two stack arguments) */

/* 0xac */ { "STX  ",  MKFMT(NOARG8, UDECIMAL) },
/* 0xad */ { invOp,    MKFMT(NOARG8, NOARG16)  },
/* 0xae */ { "STXB ",  MKFMT(NOARG8, UDECIMAL) },
/* 0xaf */ { "STXM ",  MKFMT(NOARG8, UDECIMAL) },

/* 0xb0 */ { "LA   ",  MKFMT(NOARG8, UDECIMAL) },
/* 0xb1 */ { "LAC  ",  MKFMT(NOARG8, HEX)      },
/* 0xb2 */ { invOp,    MKFMT(NOARG8, NOARG16)  },
/* 0xb3 */ { invOp,    MKFMT(NOARG8, NOARG16)  },
/* 0xb4 */ { "PUSH ",  MKFMT(NOARG8, DECIMAL)  },
/* 0xb5 */ { "INDS ",  MKFMT(NOARG8, DECIMAL)  },
/* 0xb6 */ { "LIB  ",  MKFMT(NOARG8, lbOP)     },
/* 0xb7 */ { "SYSIO",  MKFMT(NOARG8, xOP)      },
/* 0xb8 */ { "LAX  ",  MKFMT(NOARG8, UDECIMAL) },
/* 0xb9 */ { invOp,    MKFMT(NOARG8, NOARG16)  },

/* Unsigned compare and branch */

/* 0xba */ { "JULT ",  MKFMT(NOARG8, HEX) },
/* 0xbb */ { "JUGTE",  MKFMT(NOARG8, HEX) },
/* 0xbc */ { "JUGT ",  MKFMT(NOARG8, HEX) },
/* 0xbd */ { "JULTE",  MKFMT(NOARG8, HEX) },
/* 0xbe */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Program control:  arg16 = unsigned label (no stack arguments) */

/* 0xbf */ { "LABEL",  MKFMT(NOARG8, LABEL_DEC) },

/**** OPCODES WITH BYTE ARGUMENT (arg8) AND 16-BIT ARGUMENT (arg16) ****/

/* 0xc0 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xc1 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xc2 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xc3 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xc4 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xc5 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xc6 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xc7 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Program Control:  arg8 = level; arg16 = unsigned label (No stack
 * arguments
 */

/* 0xc8 */ { "PCAL ",  MKFMT(SHORTINT, HEX)   },
/* 0xc9 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xca */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xcb */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xcc */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xcd */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xce */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xcf */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* 0xd0 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xd1 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xd2 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xd3 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xd4 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xd5 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xd6 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xd7 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xd8 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xd9 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xda */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xdb */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xdc */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xdd */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xde */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xdf */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Load:  arg8 = level; arg16 = signed frame offset (no stack arguments) */

/* 0xe0 */ { "LDS  ",  MKFMT(SHORTINT, DECIMAL) },
/* 0xe1 */ { "LDSB ",  MKFMT(SHORTINT, DECIMAL) },
/* 0xe2 */ { "ULDSB",  MKFMT(SHORTINT, DECIMAL) },
/* 0xe3 */ { "LDSM ",  MKFMT(SHORTINT, DECIMAL) },

/* Store: arg8 = level; arg16 = signed frame offset (One stack arguments) */

/* 0xe4 */ { "STS  ",  MKFMT(SHORTINT, DECIMAL) },
/* 0xe5 */ { invOp,    MKFMT(NOARG8, NOARG16)   },
/* 0xe6 */ { "STSB ",  MKFMT(SHORTINT, DECIMAL) },
/* 0xe7 */ { "STSM ",  MKFMT(SHORTINT, DECIMAL) },

/* Load Indexed: arg8 = level; arg16 = signed frame offset (One stack arguments) */

/* 0xe8 */ { "LDSX ",  MKFMT(SHORTINT, DECIMAL) },
/* 0xe9 */ { "LDSXB",  MKFMT(SHORTINT, DECIMAL) },
/* 0xea */ { "ULDSXB", MKFMT(SHORTINT, DECIMAL) },
/* 0xeb */ { "LDSXM",  MKFMT(SHORTINT, DECIMAL) },

/* Store Indexed: arg8 = level; arg16 = signed frame offset (Two stack arguments) */

/* 0xec */ { "STSX ",  MKFMT(SHORTINT, DECIMAL) },
/* 0xed */ { invOp,    MKFMT(NOARG8, NOARG16)   },
/* 0xee */ { "STSXB",  MKFMT(SHORTINT, DECIMAL) },
/* 0xef */ { "STSXM",  MKFMT(SHORTINT, DECIMAL) },

/* Load Address:  arg8 = level; arg16 = signed frame offset (no stack arguments) */

/* 0xf0 */ { "LAS  ",  MKFMT(SHORTINT, DECIMAL) },
/* 0xf1 */ { invOp,    MKFMT(NOARG8, NOARG16)   },
/* 0xf2 */ { invOp,    MKFMT(NOARG8, NOARG16)   },
/* 0xf3 */ { invOp,    MKFMT(NOARG8, NOARG16)   },
/* 0xf4 */ { invOp,    MKFMT(NOARG8, NOARG16)   },
/* 0xf5 */ { invOp,    MKFMT(NOARG8, NOARG16)   },
/* 0xf6 */ { invOp,    MKFMT(NOARG8, NOARG16)   },
/* 0xf7 */ { invOp,    MKFMT(NOARG8, NOARG16)   },
/* 0xf8 */ { "LASX ",  MKFMT(SHORTINT, DECIMAL) },

/* System Functions:  (No stack arguments)
 * For SYSIO:        arg16 = sub-function code
 *                   TOS   = File number
 */

/* 0xf9 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xfa */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xfb */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xfc */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xfd */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xfe */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Pseudo-operations:
 * For LINE:         arg8 = file number; arg16 = line number
 */

/* 0xff */ { "LINE ",  MKFMT(SHORTWORD, COMMENT) },
};

static const char invXOp[] = "Invalid SYSIO";
static const char *xName[MAX_XOP] =
{ /* SYSIO opcode mnemonics */
/* 0x00 */ invXOp,      "ALLOCFILE",  "FREEFILE",   "EOF",
/* 0x04 */ "EOLN",      "FILEPOS",    "FILESIZE",   "SEEK",
/* 0x08 */ "SEEKEOF",   "SEEKEOLN",   "ASSIGNFILE", "RESET",
/* 0x0c */ "RESETR",    "REWRITE",    "REWRITER",   "APPEND",
/* 0x10 */ "CLOSEFILE", "READLN",     "READPG",     "READBIN",
/* 0x14 */ "READINT",   "READCHR",    "READSTR",    "READSSTR",
/* 0x18 */ "READRL",    "WRITELN",    "WRITEPG",    "WRITEBIN",
/* 0x1c */ "WRITEINT",  "WRITEWORD",  "WRITELONG",  "WRITEULONG",
/* 0x20 */ "WRITECHR",  "WRITESTR",   "WRITESSTR",  "WRITERL"
};

static const char invSetOp[] = "Invalid SETOP";
static const char *sName[MAX_SETOP] =
{ /* Set opcode mnemonics */
/* 0x00 */ invSetOp,     "EMPTY",     "INTERSECTION", "UNION",
/* 0x04 */ "DIFFERENCE", "SYMDIFF",   "EQUAL",        "NEQUAL",
/* 0x08 */ "CONTAINS",   "MEMBER",    "INCLUDE",      "EXCLUDE",
/* 0x0c */ "CARD",       "SINGLETON", "SUBRANGE"
};

static const char invLbOp[] = "Invalid runtime code";
static const char *lbName[MAX_LBOP] =
{ /* LIB opcode mnemonics */
/* 0x00 */ "EXIT",       "NEW",        "DISPOSE",     "GETENV",
/* 0x04 */ "STRCPY",     "STRCPY2",    "STRCPYX",     "STRCPYX2",
/* 0x08 */ "SSTRCPY",    "SSTRCPY2",   "SSTRCPYX",    "SSTRCPYX2",
/* 0x0c */ "SSTR2STR",   "SSTR2STR2",  "SSTR2STRX",   "SSTR2STRX2",
/* 0x10 */ "STR2SSTR",   "STR2SSTR2",  "STR2SSTRX",   "STR2SSTRX2",
/* 0x14 */ "BSTR2STR",   "STR2BSTR",   "STR2BSTRX",   "STRINIT",
/* 0x18 */ "SSTRINIT",   "STRTMP",     "STRDUP",      "SSTRDUP",
/* 0x1c */ "MKSTKC",     "STRCAT",     "SSTRCAT",     "SSTRCATSTR",
/* 0x20 */ "STRCATSSTR", "STRCATC",    "SSTRCATC",    "STRCMP",
/* 0x24 */ "SSTRCMP",    "SSTRCMPSTR", "STRCMPSSTR",  "COPYSUBSTR",
/* 0x28 */ "FINDSUBSTR", "VAL"
};

static const char invFpOp[] = "Invalid FP Operation";
static const char *fpName[MAX_FOP] =
{
/* 0x00 */ invFpOp,    "FLOAT",    "TRUNC",    "ROUND",
/* 0x04 */ "ADD",      "SUB",      "MUL",      "DIV",
/* 0x08 */ "MOD",      invFpOp,    "EQU",      "NEQ",
/* 0x0c */ "LT",       "GTE",      "GT",       "LTE",
/* 0x10 */ "NEG",      "ABS",      "SQR",      "SQRT",
/* 0x14 */ "SIN",      "COS",      "ATAN",     "LN",
/* 0x18 */ "EXP"
};

/***********************************************************************/

void insn_DisassemblePCode(FILE* lfile, opType_t *pop)
{
  uint8_t fmt8;
  uint8_t fmt16;

  /* Indent, comment or label */

  fmt8  = ARG8FMT(opTable[pop->op].format);
  fmt16 = ARG16FMT(opTable[pop->op].format);

  switch (fmt16)
    {
    case LABEL_DEC :
      fprintf(lfile, "L%04x:  ", pop->arg2);
      break;

    case COMMENT   :
      fprintf(lfile, "; ");
      break;

    default   :
      fprintf(lfile, "        ");
      break;
    }

  /* Special Case Comment line format */

  if (fmt16 == COMMENT)
    {
      fprintf(lfile, "%s ", opTable[pop->op].opName);
      if (pop->op & o8)
        {
          fprintf(lfile, "%d", pop->arg1);
          if (pop->op & o16)
            {
              fprintf(lfile, ":%d", pop->arg2);
            }
        }
      else if (pop->op & o16)
        {
          fprintf(lfile, "%d", pop->arg2);
        }
    }

  /* Print normal opCode mnemonic */

  else
    {
      if (fmt16 != LABEL_DEC)
        {
          fprintf(lfile, "%s ", opTable[pop->op].opName);
        }

      /* Print arg8 (if present) */

      if ((pop->op & o8) != 0)
        {
          switch (fmt8)
            {
              case SHORTWORD :    /* Show ARG8 as an unsigned integer */
                fprintf(lfile, "%u", pop->arg1);
                break;

              case SHORTINT :    /* Show ARG8 as a signed integer */
                fprintf(lfile, "%d", signExtend8(pop->arg1));
                break;

              case fpOP :        /* Show ARG8 as encoded floating point operation */
                if ((pop->arg1 & fpMASK) < MAX_FOP)
                  {
                    fprintf(lfile, "%s", fpName[(pop->arg1 & 0x3f)]);
                  }
                else
                  {
                    fprintf(lfile, "%s", invFpOp);
                  }
                break;

              case setOP :       /* Show ARG8 as encoded SET operation */
                if (pop->arg1 < MAX_SETOP)
                  {
                    fprintf(lfile, "%s", sName[pop->arg1]);
                  }
                else
                  {
                    fprintf(lfile, "%s", invSetOp);
                  }
                break;

              case NOARG8 :
              default:
                break;
            }
        }

      /* Print arg16 (if present) */

      if ((pop->op & o16) != 0)
        {
          /* Separate from arg8 (if present) with a comma and space */

          if ((pop->op & o8) != 0)
            {
              fprintf(lfile, ", ");
            }

          switch (fmt16)
            {
              case DECIMAL :     /* Show ARG16 as a signed decimal */
              case COMMENT :     /* This is a comment */
                fprintf(lfile, "%d", (int16_t)pop->arg2);
                break;

              case HEX :         /* Show ARG16 as hexadecimal */
                fprintf(lfile, "0x%04x", pop->arg2);
                break;

              case UDECIMAL :    /* Show ARG16 as an unsigned decimal */
                fprintf(lfile, "%u", pop->arg2);
                break;

              case xOP :         /* Show ARG16 as encoded SYSIO operation */
                if (pop->arg2 < MAX_XOP)
                  {
                    fprintf(lfile, "%s", xName[pop->arg2]);
                  }
                 else
                  {
                    fprintf(lfile, "%s", invXOp);
                  }
                break;

              case lbOP :        /* Show ARG16 as encoded library function call */
                if (pop->arg2 < MAX_LBOP)
                  {
                    fprintf(lfile, "%s", lbName[pop->arg2]);
                  }
                else
                  {
                    fprintf(lfile, "%s", invLbOp);
                  }
                break;

              case NOARG16 :     /* There is no ARG16? */
              case LABEL_DEC :   /* Show ARG16 as a label */
              default        :
                break;
            }
        }
    }

  /* Don't forget the newline! */

  fputc('\n', lfile);
}
