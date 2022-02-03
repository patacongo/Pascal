/**********************************************************************
 * insn_dislongop.c
 * Long opcode Disassembler
 *
 *   Copyright (C) 2022 Gregory Nutt. All rights reserved.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#include "pas_debug.h"
#include "pas_pcode.h"
#include "pas_longops.h"
#include "longops.h"
#include "paslib.h"

#include "pas_insn.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* These are all the format codes that apply to opcodes */

#define NOARG8        0

#define NOARG16       0
#define HEX           1    /* Show ARG16 as hexadecimal */

#define MKFMT(a8,a16) (((a16) << 3) | a8)
#define ARG8FMT(n)    ((n) & 0x07)
#define ARG16FMT(n)   ((n) >> 3)

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The following table defines everything that is needed to disassemble
 * a P-Code.  NOTE:  The order of definition in this table must exactly
 * match the declaration sequence in pas_insn.h. */

static const char invOp[] = "Invalid Opcode";
struct opCodeInfo_s
{
  const char *opName;       /* Opcode mnemonics */
  uint8_t format;           /* arg16 format */
};

/********************** OPCODES WITH NO ARGUMENTS ***************************/

static struct opCodeInfo_s g_noArgOpTable[0x40] =
{
/* Program control (No stack arguments) */

/* 0x00 */ { "DNOP",   MKFMT(NOARG8, NOARG16) },

/* Arithmetic & logical & and integer conversions (One stack argument) */

/* 0x01 */ { "DNEG  ", MKFMT(NOARG8, NOARG16) },
/* 0x02 */ { "DABS  ", MKFMT(NOARG8, NOARG16) },
/* 0x03 */ { "DINC  ", MKFMT(NOARG8, NOARG16) },
/* 0x04 */ { "DDEC  ", MKFMT(NOARG8, NOARG16) },
/* 0x05 */ { "DNOT  ", MKFMT(NOARG8, NOARG16) },

/* Arithmetic & logical (Two stack arguments) */

/* 0x06 */ { "DADD  ", MKFMT(NOARG8, NOARG16) },
/* 0x07 */ { "DSUB  ", MKFMT(NOARG8, NOARG16) },
/* 0x08 */ { "DMUL  ", MKFMT(NOARG8, NOARG16) },
/* 0x09 */ { "DDIV  ", MKFMT(NOARG8, NOARG16) },
/* 0x0a */ { "DMOD  ", MKFMT(NOARG8, NOARG16) },
/* 0x0b */ { "DSLL  ", MKFMT(NOARG8, NOARG16) },
/* 0x0c */ { "DSRL  ", MKFMT(NOARG8, NOARG16) },
/* 0x0d */ { "DSRA  ", MKFMT(NOARG8, NOARG16) },
/* 0x0e */ { "DOR   ", MKFMT(NOARG8, NOARG16) },
/* 0x0f */ { "DAND  ", MKFMT(NOARG8, NOARG16) },

/* Comparisons (One stack argument) */

/* 0x10 */ { "DEQUZ ", MKFMT(NOARG8, NOARG16) },
/* 0x11 */ { "DNEQZ ", MKFMT(NOARG8, NOARG16) },
/* 0x12 */ { "DLTZ  ", MKFMT(NOARG8, NOARG16) },
/* 0x13 */ { "DGTEZ ", MKFMT(NOARG8, NOARG16) },
/* 0x14 */ { "DGTZ  ", MKFMT(NOARG8, NOARG16) },
/* 0x15 */ { "DLTEZ ", MKFMT(NOARG8, NOARG16) },
/* 0x16 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x17 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Comparisons (Two stack arguments) */

/* 0x18 */ { "DEQU  ", MKFMT(NOARG8, NOARG16) },
/* 0x19 */ { "DNEQ  ", MKFMT(NOARG8, NOARG16) },
/* 0x1a */ { "DLT   ", MKFMT(NOARG8, NOARG16) },
/* 0x1b */ { "DGTE  ", MKFMT(NOARG8, NOARG16) },
/* 0x1c */ { "DGT   ", MKFMT(NOARG8, NOARG16) },
/* 0x1d */ { "DLTE  ", MKFMT(NOARG8, NOARG16) },
/* 0x1e */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x1f */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Load (One) or Store (Two stack argument) */

/* 0x20 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x21 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x22 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x23 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x24 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x25 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x26 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x27 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Data stack operations */

/* 0x28 */ { "DDUP  ", MKFMT(NOARG8, NOARG16) },
/* 0x29 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x2a */ { "DXCHG ", MKFMT(NOARG8, NOARG16) },
/* 0x2b */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x2c */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x2d */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x2e */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x2f */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* 0x30 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x31 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x32 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x33 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x34 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x35 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x36 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Unsigned arithmetic */

/* 0x37 */ { "DUMUL ", MKFMT(NOARG8, NOARG16) },
/* 0x38 */ { "DUDIV ", MKFMT(NOARG8, NOARG16) },
/* 0x39 */ { "DUMOD ", MKFMT(NOARG8, NOARG16) },

/* Unsigned comparisons */

/* 0x3a */ { "DULT  ", MKFMT(NOARG8, NOARG16) },
/* 0x3b */ { "DUGTE ", MKFMT(NOARG8, NOARG16) },
/* 0x3c */ { "DUGT  ", MKFMT(NOARG8, NOARG16) },
/* 0x3d */ { "DULTE ", MKFMT(NOARG8, NOARG16) },

/* More bitwise operators */

/* 0x3e */ { "DXOR  ", MKFMT(NOARG8, NOARG16) },

/* System functions */

/* 0x3f */ { invOp,    MKFMT(NOARG8, NOARG16) },
};

/**************** OPCODES WITH SINGLE BYTE ARGUMENT (arg8) ******************/
/* NONE */

/*************** OPCODES WITH SINGLE 16-BIT ARGUMENT (arg16) ****************/

static struct opCodeInfo_s g_arg16OpTable[0x40] =
{
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

/* 0x90 */ { "DJEQUZ", MKFMT(NOARG8, HEX) },
/* 0x91 */ { "DJNEQZ", MKFMT(NOARG8, HEX) },
/* 0x92 */ { "DJLTZ ", MKFMT(NOARG8, HEX) },
/* 0x93 */ { "DJGTEZ", MKFMT(NOARG8, HEX) },
/* 0x94 */ { "DJGTZ ", MKFMT(NOARG8, HEX) },
/* 0x95 */ { "DJLTEZ", MKFMT(NOARG8, HEX) },

/* Program control:  arg16 = unsigned label (no stack arguments) */

/* 0x96 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x97 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Program control:  arg16 = unsigned label (One stack argument) */

/* 0x98 */ { "DJEQU ", MKFMT(NOARG8, HEX) },
/* 0x99 */ { "DJNEQ ", MKFMT(NOARG8, HEX) },
/* 0x9a */ { "DJLT  ", MKFMT(NOARG8, HEX) },
/* 0x9b */ { "DJGTE ", MKFMT(NOARG8, HEX) },
/* 0x9c */ { "DJGT  ", MKFMT(NOARG8, HEX) },
/* 0x9d */ { "DJLTE ", MKFMT(NOARG8, HEX) },
/* 0x9e */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0x9f */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Data stack:  arg16 = 16 bit signed data (no stack arguments) */

/* Load:  arg16 = unsigned base offset (no stack arguments) */

/* 0xa0 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xa1 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xa2 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xa3 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Store: arg16 = unsigned base offset (One stack arguments) */

/* 0xa4 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xa5 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xa6 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xa7 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Load Indexed: arg16 = unsigned base offset (One stack arguments) */

/* 0xa8 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xa9 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xaa */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xab */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Store Indexed: arg16 = unsigned base offset (Two stack arguments) */

/* 0xac */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xad */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xae */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xaf */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* 0xb0 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xb1 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xb2 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xb3 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xb4 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xb5 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xb6 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xb7 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xb8 */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xb9 */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Unsigned compare and branch */

/* 0xba */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xbb */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xbc */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xbd */ { invOp,    MKFMT(NOARG8, NOARG16) },
/* 0xbe */ { invOp,    MKFMT(NOARG8, NOARG16) },

/* Program control:  arg16 = unsigned label (no stack arguments) */

/* 0xbf */ { invOp,    MKFMT(NOARG8, NOARG16) },
};

/**** OPCODES WITH BYTE ARGUMENT (arg8) AND 16-BIT ARGUMENT (arg16) ****/
/* NONE */

/****************************************************************************/

void insn_DisassembleLongOpCode(FILE* lfile, opType_t *pop)
{
  struct opCodeInfo_s *opCodeInfo;
  uint8_t fmt16;

  /* Print normal long opCode mnemonic */

  switch (pop->op & (o8 | o16))
    {
      case 0 :
        opCodeInfo = &g_noArgOpTable[pop->op];
        break;

      case o16 :
        opCodeInfo = &g_arg16OpTable[pop->op & 0x3f];
        break;

      case o8 :
      case o8 | o16 :
        fprintf(lfile, "        %s\n", invOp);
        return;
    }

  fprintf(lfile, "        %s ", opCodeInfo->opName);

  /* Print arg8 (if present) -- There are none. */

  /* Print arg16 (if present) */

  if ((pop->op & o16) != 0)
    {
      fmt16 = ARG16FMT(opCodeInfo->format);

      switch (fmt16)
        {
          case HEX :         /* Show ARG16 as hexadecimal */
            fprintf(lfile, "0x%04x", pop->arg2);
            break;

          case NOARG16 :     /* There is no ARG16? */
          default        :
            break;
        }
    }

  /* Don't forget the newline! */

  fputc('\n', lfile);
}
