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

#include "insn16.h"
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

/* This table is indexed via enum longOp8_e & ~o8 */

static const struct opCodeInfo_s g_longOpTable8[] =
{
  /* Program control (No stack arguments) */

  { "DNOP",   MKFMT(NOARG8, NOARG16) },

  /* Arithmetic & logical & and integer conversions (One stack argument) */

  { "DNEG  ", MKFMT(NOARG8, NOARG16) },
  { "DABS  ", MKFMT(NOARG8, NOARG16) },
  { "DINC  ", MKFMT(NOARG8, NOARG16) },
  { "DDEC  ", MKFMT(NOARG8, NOARG16) },
  { "DNOT  ", MKFMT(NOARG8, NOARG16) },

  /* Arithmetic & logical (Two stack arguments) */

  { "DADD  ", MKFMT(NOARG8, NOARG16) },
  { "DSUB  ", MKFMT(NOARG8, NOARG16) },
  { "DMUL  ", MKFMT(NOARG8, NOARG16) },
  { "DDIV  ", MKFMT(NOARG8, NOARG16) },
  { "DMOD  ", MKFMT(NOARG8, NOARG16) },
  { "DSLL  ", MKFMT(NOARG8, NOARG16) },
  { "DSRL  ", MKFMT(NOARG8, NOARG16) },
  { "DSRA  ", MKFMT(NOARG8, NOARG16) },
  { "DOR   ", MKFMT(NOARG8, NOARG16) },
  { "DAND  ", MKFMT(NOARG8, NOARG16) },

  /* Comparisons (One stack argument) */

  { "DEQUZ ", MKFMT(NOARG8, NOARG16) },
  { "DNEQZ ", MKFMT(NOARG8, NOARG16) },
  { "DLTZ  ", MKFMT(NOARG8, NOARG16) },
  { "DGTEZ ", MKFMT(NOARG8, NOARG16) },
  { "DGTZ  ", MKFMT(NOARG8, NOARG16) },
  { "DLTEZ ", MKFMT(NOARG8, NOARG16) },

  /* Comparisons (Two stack arguments) */

  { "DEQU  ", MKFMT(NOARG8, NOARG16) },
  { "DNEQ  ", MKFMT(NOARG8, NOARG16) },
  { "DLT   ", MKFMT(NOARG8, NOARG16) },
  { "DGTE  ", MKFMT(NOARG8, NOARG16) },
  { "DGT   ", MKFMT(NOARG8, NOARG16) },
  { "DLTE  ", MKFMT(NOARG8, NOARG16) },

  /* Data stack operations */

  { "DDUP  ", MKFMT(NOARG8, NOARG16) },
  { "DXCHG ", MKFMT(NOARG8, NOARG16) },
  { "CNVD  ", MKFMT(NOARG8, NOARG16) },
  { "UCNVD ", MKFMT(NOARG8, NOARG16) },
  { "DCNV  ", MKFMT(NOARG8, NOARG16) },

  /* Unsigned arithmetic */

  { "DUMUL ", MKFMT(NOARG8, NOARG16) },
  { "DUDIV ", MKFMT(NOARG8, NOARG16) },
  { "DUMOD ", MKFMT(NOARG8, NOARG16) },

  /* Unsigned comparisons */

  { "DULT  ", MKFMT(NOARG8, NOARG16) },
  { "DUGTE ", MKFMT(NOARG8, NOARG16) },
  { "DUGT  ", MKFMT(NOARG8, NOARG16) },
  { "DULTE ", MKFMT(NOARG8, NOARG16) },

  /* More bitwise operators */

  { "DXOR  ", MKFMT(NOARG8, NOARG16) },
};

#define NUM_LONGOP8 (sizeof(g_longOpTable8) / sizeof(struct opCodeInfo_s))

/**************** OPCODES WITH SINGLE BYTE ARGUMENT (arg8) ******************/
/* NONE */

/*************** OPCODES WITH SINGLE 16-BIT ARGUMENT (arg16) ****************/

/* This table is indexed via enum longOp24_e & ~o8 */

static const struct opCodeInfo_s g_longOpTable24[] =
{
/* Program control:  arg16 = unsigned label (One stack argument) */

  { "DJEQUZ", MKFMT(NOARG8, HEX) },
  { "DJNEQZ", MKFMT(NOARG8, HEX) },
  { "DJLTZ ", MKFMT(NOARG8, HEX) },
  { "DJGTEZ", MKFMT(NOARG8, HEX) },
  { "DJGTZ ", MKFMT(NOARG8, HEX) },
  { "DJLTEZ", MKFMT(NOARG8, HEX) },

  /* Program control:  arg16 = unsigned label (One stack argument) */

  { "DJEQU ", MKFMT(NOARG8, HEX) },
  { "DJNEQ ", MKFMT(NOARG8, HEX) },

  { "DJLT  ", MKFMT(NOARG8, HEX) },
  { "DJGTE ", MKFMT(NOARG8, HEX) },
  { "DJGT  ", MKFMT(NOARG8, HEX) },
  { "DJLTE ", MKFMT(NOARG8, HEX) },

  { "DJULT ", MKFMT(NOARG8, HEX) },
  { "DJUGTE", MKFMT(NOARG8, HEX) },
  { "DUJGT ", MKFMT(NOARG8, HEX) },
  { "DUJLTE", MKFMT(NOARG8, HEX) },
};

#define NUM_LONGOP24 (sizeof(g_longOpTable24) / sizeof(struct opCodeInfo_s))

/**** OPCODES WITH BYTE ARGUMENT (arg8) AND 16-BIT ARGUMENT (arg16) ****/
/* NONE */

/****************************************************************************/

void insn_DisassembleLongOpCode(FILE* lfile, opType_t *pop)
{
  const struct opCodeInfo_s *opCodeInfo;
  uint8_t fmt16;
  int index;

  /* Print normal long opCode mnemonic */

  index = pop->arg1 & 0x3f;
  switch (pop->op & (o8 | o16))
    {
      case o8 :
        if (index < NUM_LONGOP8)
          {
            opCodeInfo = &g_longOpTable8[index];
            break;
          }

      case o8 | o16 :
        if (index < NUM_LONGOP24)
          {
            opCodeInfo = &g_longOpTable24[index];
            break;
          }

      case 0 :
      case o16 :
        fprintf(lfile, "        %s\n", invOp);
        return;
    }

  fprintf(lfile, "        %s ", opCodeInfo->opName);

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
