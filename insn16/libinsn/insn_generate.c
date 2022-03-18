/**********************************************************************
 * insn_generate.c
 * P-Code generation logic
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
#include "pas_pcode.h"    /* Logical opcode definitions */
#include "pas_errcodes.h" /* Error codes */
#include "insn16.h"       /* 16-bit target INSN opcode definitions */
#include "pas_error.h"    /* Error handling logic */

#include "pas_insn.h"     /* (to verify prototypes in this file) */

/**********************************************************************
 * Public Data
 **********************************************************************/

extern poffHandle_t g_poffHandle; /* Handle to POFF object */
extern FILE *g_lstFile;           /* LIST file pointer */

/**********************************************************************
 * Private Variables
 **********************************************************************/

/* Indexed by enum pcode_e in pas_pcode.h.  Order must match indexing. */

static const uint16_t g_opCodeMap[NUM_OPCODES] =
{
  oNOP,     /* opNOP */
  oNEG,     /* opNEG */
  oABS,     /* opABS */
  oINC,     /* opINC */
  oDEC,     /* opDEC */
  oNOT,     /* opNOT */
  oADD,     /* opADD */
  oSUB,     /* opSUB */
  oMUL,     /* opMUL */
  oUMUL,    /* opUMUL */
  oDIV,     /* opDIV */
  oUDIV,    /* opUDIV */
  oMOD,     /* opMOD */
  oUMOD,    /* opUMOD */
  oSLL,     /* opSLL */
  oSRL,     /* opSRL */
  oSRA,     /* opSRA */
  oOR,      /* opOR */
  oXOR,     /* opXOR */
  oAND,     /* opAND */
  oEQUZ,    /* opEQUZ */
  oNEQZ,    /* opNEQZ */
  oLTZ,     /* opLTZ */
  oGTEZ,    /* opGTEZ */
  oGTZ,     /* opGTZ */
  oLTEZ,    /* opLTEZ */
  oEQU,     /* opEQU */
  oNEQ,     /* opNEQ */
  oLT,      /* opLT */
  oGTE,     /* opGTE */
  oGT,      /* opGT */
  oLTE,     /* opLTE */
  oULT,     /* opULT */
  oUGTE,    /* opUGTE */
  oUGT,     /* opUGT */
  oULTE,    /* opULTE */
  oLDI,     /* opLDI */
  oLDIB,    /* opLDIB */
  oULDIB,   /* opULDIB */
  oLDIM,    /* opLDIM */
  oSTI,     /* opSTI */
  oSTIB,    /* opSTIB */
  oSTIM,    /* opSTIM */
  oDUP,     /* opDUP */
  oXCHG,    /* opXCHG */
  oRET,     /* opRET */
  oEND,     /* opEND */
  oFLOAT,   /* opFLOAT */
  oSETOP,   /* opSETOP */
  oOSOP,    /* opOSOP */
  oJEQUZ,   /* opJEQUZ */
  oJNEQZ,   /* opJNEQZ */
  oJMP,     /* opJMP */
  oJEQU,    /* opJEQU */
  oJNEQ,    /* opJNEQ */
  oJLT,     /* opJLT */
  oJGTE,    /* opJGTE */
  oJGT,     /* opJGT */
  oJLTE,    /* opJLTE */
  oLD,      /* opLD */
  oLDB,     /* opLDB */
  oULDB,    /* opULDB */
  oLDM,     /* opLDM */
  oST,      /* opST */
  oSTB,     /* opSTB */
  oSTM,     /* opSTM */
  oLDX,     /* opLDX */
  oLDXB,    /* opLDXB */
  oULDXB,   /* opULDXB */
  oLDXM,    /* opLDXM */
  oSTX,     /* opSTX */
  oSTXB,    /* opSTXB */
  oSTXM,    /* opSTXM */
  oLA,      /* opLA */
  oLAC,     /* opLAC */
  oLAR,     /* opLAR */
  oPUSH,    /* opPUSH */
  oINDS,    /* opINDS */
  oINCS,    /* opINCS */
  oLAX,     /* opLAX */
  oSTRLIB,  /* opSTRLIB */
  oSYSIO,   /* opSYSIO */
  oLABEL,   /* opLABEL */
  oPCAL,    /* opPCAL */
  oLDS,     /* opLDS */
  oLDSB,    /* opLDSB */
  oULDSB,   /* opULDSB */
  oLDSM,    /* opLDSM */
  oSTS,     /* opSTS */
  oSTSB,    /* opSTSB */
  oSTSM,    /* opSTSM */
  oLDSX,    /* opLDS*/
  oLDSXB,   /* opLDSXB */
  oULDSXB,  /* opULDSXB */
  oLDSXM,   /* opLDSXM */
  oSTSX,    /* opSTSX */
  oSTSXB,   /* opSTSXB */
  oSTSXM,   /* opSTSXM */
  oLAS,     /* opLAS */
  oLASX,    /* opLASX */
  oLINE     /* opLINE */
};

/***********************************************************************
 * Private Function Prototypes
 ***********************************************************************/

/***********************************************************************
 * Private Functions
 ***********************************************************************/

static void
insn16_Generate(enum pcode_e opcode, uint16_t arg1, int32_t arg2);

/***********************************************************************
 * Private Functions
 ***********************************************************************/

/***********************************************************************/
/* Disassemble an Op-code */

#if CONFIG_DEBUG
static inline void insn16_DisassemblePCode(uint8_t opcode, uint8_t arg1,
                                           uint16_t arg2)
{
  opType_t op;

  op.op   = opcode;
  op.arg1 = arg1;
  op.arg2 = arg2;

  insn_DisassemblePCode(g_lstFile, &op);
}
#else
# define insn16_DisassemblePCode(op,a1,a2)
#endif

/***********************************************************************/
/* Generate an Op-Code */

static void insn16_Generate(enum pcode_e opcode, uint16_t arg1,
                            int32_t arg2)
{
  uint16_t insn_opcode = g_opCodeMap[opcode];
  uint16_t arg16;

  poffAddProgByte(g_poffHandle, insn_opcode);
  if (insn_opcode & o8)
    {
      if (arg1 > 0xff) error(eINTOVF);
      poffAddProgByte(g_poffHandle, (uint8_t)arg1);
    }

  if (insn_opcode & o16)
    {
      if (arg2 < -32768 || arg2 > 65535) error(eINTOVF);
      arg16 = (uint16_t)arg2;
      poffAddProgByte(g_poffHandle, (uint8_t)(arg16 >> 8));
      poffAddProgByte(g_poffHandle, (uint8_t)(arg16 & 0xff));
    }

  /* Now, add the disassembled PCode to the list file. */

  insn16_DisassemblePCode(insn_opcode, arg1, arg2);
}

/***********************************************************************
 * Public Functions
 ***********************************************************************/

void insn_GenerateSimple(enum pcode_e opcode)
{
  insn16_Generate(opcode, 0, 0);
}

/***********************************************************************/

void insn_GenerateDataOperation(enum pcode_e opcode, int32_t data)
{
  insn16_Generate(opcode, 0, data);
}

/***********************************************************************/
/* Data size for a multiple register operation (in bytes) is simply
 * represented by that value at the top of the stack.
 */

void insn_GenerateDataSize(uint32_t dwDataSize)
{
  insn16_Generate(opPUSH, 0, dwDataSize);
}

/***********************************************************************/

void insn_GenerateFpOperation(uint8_t fpOpcode)
{
  insn16_Generate(opFLOAT, fpOpcode, 0);
}

/***********************************************************************/

void insn_GenerateSetOperation(uint8_t setOpcode)
{
  insn16_Generate(opSETOP, setOpcode, 0);
}

/***********************************************************************/

void insn_GenerateIoOperation(uint16_t ioOpcode)
{
  insn16_Generate(opSYSIO, 0, (int32_t)ioOpcode);
}

/***********************************************************************/

void insn_StringLibraryCall(uint16_t strOpcode)
{
  insn16_Generate(opSTRLIB, 0, (int32_t)strOpcode);
}

/***********************************************************************/

void insn_OsInterfaceCall(uint16_t osOpcode)
{
  insn16_Generate(opOSOP, osOpcode, 0);
}

/***********************************************************************/

void insn_GenerateLevelReference(enum pcode_e opcode, uint16_t level,
                                 int32_t offset)
{
  insn16_Generate(opcode, level, offset);
}

/***********************************************************************/

void insn_GenerateProcedureCall(uint16_t level, int32_t offset)
{
  insn16_Generate(opPCAL, level, offset);
}

/***********************************************************************/

void insn_GenerateLineNumber(uint16_t includeNumber, uint32_t lineNumber)
{
  insn16_Generate(opLINE, includeNumber, lineNumber);
}
