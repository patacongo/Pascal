/**********************************************************************
 * insn_genlongop.c
 * Long integer/word P-Code generation logic
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

/**********************************************************************
 * Included Files
 **********************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "config.h"       /* Configuration */
#include "pas_debug.h"    /* Standard types */
#include "pas_machine.h"  /* Common types */
#include "pofflib.h"      /* POFF library definitions */
#include "pas_insn.h"     /* Logical long interger/word definitions */
#include "pas_longops.h"  /* Logical long interger/word definitions */
#include "pas_errcodes.h" /* Error codes */
#include "insn16.h"       /* 16-bit target INSN opcode definitions */
#include "longops.h"      /* 16-bit target long integer/word opcode definitions */
#include "pas_error.h"    /* Error handling logic */

#include "longops.h"     /* (to verify prototypes in this file) */

/**********************************************************************
 * Public Data
 **********************************************************************/

extern poffHandle_t g_poffHandle; /* Handle to POFF object */
extern FILE *g_lstFile;           /* LIST file pointer */

/**********************************************************************
 * Private Variables
 **********************************************************************/

/* Indexed by enum longops_e in pas_pcode.h.  Order must match indexing. */

static const uint16_t g_longOpcodeMap[NUM_LONGOPS] =
{
  oDNOP,    /* opDNOP */

  oDNEG,    /* opDNEG */
  oDABS,    /* opDABS */
  oDINC,    /* opDINC */
  oDDEC,    /* opDDEC */
  oDNOT,    /* opDNOT */

  oDADD,    /* opDADD */
  oDSUB,    /* opDSUB */
  oDMUL,    /* opDMUL */
  oDUMUL,   /* opDUMUL */
  oDDIV,    /* opDDIV */
  oDUDIV,   /* opDUDIV */
  oDMOD,    /* opDMOD */
  oDUMOD,   /* opDUMOD */

  oDSLL,    /* opDSLL */
  oDSRL,    /* opDSRL */
  oDSRA,    /* opDSRA */
  oDOR,     /* opDOR */
  oDXOR,    /* opDXOR */
  oDAND,    /* opDAND */

  oDEQUZ,   /* opDEQUZ */
  oDNEQZ,   /* opDNEQZ */
  oDLTZ,    /* opDLTZ */
  oDGTEZ,   /* opDGTEZ */
  oDGTZ,    /* opDGTZ */
  oDLTEZ,   /* opDLTEZ */

  oDEQU,    /* opDEQU */
  oDNEQ,    /* opDNEQ */

  oDLT,     /* opDLT */
  oDGTE,    /* opDGTE */
  oDGT,     /* opDGT */
  oDLTE,    /* opDLTE */

  oDULT,    /* opDULT */
  oDUGTE,   /* opDUGTE */
  oDUGT,    /* opDUGT */
  oDULTE,   /* opDULTE */

  oDDUP,    /* opDDUP */
  oDXCHG,   /* opDXCHG */
  oCNVD,    /* opCNVD */
  oUCNVD,   /* opUCNVD */
  oDCNV,    /* opDCNV */

  oDJEQUZ,  /* opDJEQUZ */
  oDJNEQZ,  /* opDJNEQZ */

  oDJEQU,   /* opDJEQU */
  oDJNEQ,   /* opDJNEQ */
  oDJLT,    /* opDJLT */
  oDJGTE,   /* opDJGTE */
  oDJGT,    /* opDJGT */
  oDJLTE    /* opDJLTE */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void insn16_longOpcodeGenerate(enum longops_e opcode, uint16_t arg1,
                                      int32_t arg2);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/
/* Disassemble an Op-code */

#if CONFIG_DEBUG
static inline void
insn16_DisassembleLongOpCode(uint8_t longOpcode, uint8_t arg1, uint16_t arg2)
{
  opType_t op;

  op.op   = longOpcode;
  op.arg1 = arg1;
  op.arg2 = arg2;

  insn_DisassembleLongOpCode(g_lstFile, &op);
}
#else
# define insn16_DisassembleLongOpCode(op,a1,a2)
#endif

/****************************************************************************/
/* Generate an Op-Code */

static void insn16_longOpcodeGenerate(enum longops_e longOpCode,
                                      uint16_t arg1, int32_t arg2)
{
  uint16_t insn_longOpCode = g_longOpcodeMap[longOpCode];
  uint16_t insn_opCode;
  uint16_t arg16;

  if ((insn_longOpCode & o16) != 0)
    {
      insn_opCode = oLONGOP24;
    }
  else
    {
      insn_opCode = oLONGOP8;
    }

  poffAddProgByte(g_poffHandle, insn_opCode);
  poffAddProgByte(g_poffHandle, insn_longOpCode);

  if ((insn_longOpCode & o16) != 0)
    {
      if (arg2 < -32768 || arg2 > 65535) error(eINTOVF);
      arg16 = (uint16_t)arg2;
      poffAddProgByte(g_poffHandle, (uint8_t)(arg16 >> 8));
      poffAddProgByte(g_poffHandle, (uint8_t)(arg16 & 0xff));
    }

  /* Now, add the disassembled PCode to the list file. */

  insn16_DisassembleLongOpCode(insn_longOpCode, arg1, arg2);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void insn_GenerateSimpleLongOperation(enum longops_e longOpCode)
{
  insn16_longOpcodeGenerate(longOpCode, 0, 0);
}

/****************************************************************************/

void insn_GenerateDataLongOperation(enum longops_e longOpCode, int32_t data)
{
  insn16_longOpcodeGenerate(longOpCode, 0, data);
}

