/**********************************************************************
 * pgen.c
 * P-Code generation logic
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
#include "pinsn32.h"      /* 32-bit target INSN opcode definitions */
#include "pas_error.h"    /* Error handling logic */

#include "pas_insn.h"     /* (to verify prototypes in this file) */

/**********************************************************************
 * Pre-processor Definitions
 **********************************************************************/

#define INVALID_INCLUDE (-1)

/**********************************************************************
 * Public Data
 **********************************************************************/

extern poffHandle_t g_poffHandle; /* Handle to POFF object */
extern FILE *g_lstFile;           /* LIST file pointer */

/**********************************************************************
 * Private Variables
 **********************************************************************/

/* Indexed by enum pcode_e in pas_pcode.h.  Order must match indexing. */

static const uint8_t opmap[NUM_OPCODES] =
{
  oNOP,    /* opNOP */
  oNEG,    /* opNEG */
  oABS,    /* opABS */
  oINC,    /* opINC */
  oDEC,    /* opDEC */
  oNOT,    /* opNOT */
  oADD,    /* opADD */
  oSUB,    /* opSUB */
  oMUL,    /* opMUL */
  oDIV,    /* opDIV */
  oMOD,    /* opMOD */
  oSLL,    /* opSLL */
  oSRL,    /* opSRL */
  oSRA,    /* opSRA */
  oOR,     /* opOR */
  oNOP,    /* opXOR -- not yet implemented */
  oAND,    /* opAND */
  oEQUZ,   /* opEQUZ */
  oNEQZ,   /* opNEQZ */
  oLTZ,    /* opLTZ */
  oGTEZ,   /* opGTEZ */
  oGTZ,    /* opGTZ */
  oLTEZ,   /* opLTEZ */
  oEQU,    /* opEQU */
  oNEQ,    /* opNEQ */
  oLT,     /* opLT */
  oGTE,    /* opGTE */
  oGT,     /* opGT */
  oLTE,    /* opLTE */
  oLDI,    /* opLDI */
  oLDIB,   /* opLDIB */
  oLDIM,   /* opLDIM */
  oSTI,    /* opSTI */
  oSTIB,   /* opSTIB */
  oSTIM,   /* opSTIM */
  oDUP,    /* opDUP */
  oPUSHS,  /* opPUSHS */
  oPOPS,   /* opPOPS */
  oRET,    /* opRET */
  oEND,    /* opEND */
  oFLOAT,  /* opFLOAT */
  oSETOP,  /* opSETOP */
  oJEQUZ,  /* opJEQUZ */
  oJNEQZ,  /* opJNEQZ */
  oJMP,    /* opJMP */
  oJEQU,   /* opJEQU */
  oJNEQ,   /* opJNEQ */
  oJLT,    /* opJLT */
  oJGTE,   /* opJGTE */
  oJGT,    /* opJGT */
  oJLTE,   /* opJLTE */
  oLD,     /* opLD */
  oLDH,    /* opLDH */
  oLDB,    /* opLDB */
  oLDM,    /* opLDM */
  oST,     /* opST */
  oSTB,    /* opSTB */
  oSTM,    /* opSTM */
  oLDX,    /* opLDX */
  oLDXB,   /* opLDXB */
  oLDXM,   /* opLDXM */
  oSTX,    /* opSTX */
  oSTXB,   /* opSTXB */
  oSTXM,   /* opSTXM */
  oLA,     /* opLA */
  oLAC,    /* opLAC */
  oPUSH,   /* opPUSH */
  oINDS,   /* opINDS */
  oLAX,    /* opLAX */
  oLIB,    /* opLIB */
  oSYSIO,  /* opSYSIO */
  oLABEL,  /* opLABEL */
  oPCAL,   /* opPCAL */
  oLDS,    /* opLDS */
  oLDSH,   /* opLDSH */
  oLDSB,   /* opLDSB */
  oLDSM,   /* opLDSM */
  oSTS,    /* opSTS */
  oSTSB,   /* opSTSB */
  oSTSM,   /* opSTSM */
  oLDSX,   /* opLDSX */
  oLDSXB,  /* opLDSXB */
  oLDSXM,  /* opLDSXM */
  oSTSX,   /* opSTSX */
  oSTSXB,  /* opSTSXB */
  oSTSXM,  /* opSTSXM */
  oLAS,    /* opLAS */
  oLASX,   /* opLASX */
  oLINE,   /* opLINE */
};

static uint16_t g_nCurrentIncludeNumber = INVALID_INCLUDE;

/***********************************************************************
 * Private Function Prototypes
 ***********************************************************************/

/***********************************************************************
 * Private Functions
 ***********************************************************************/

static void
insn32_GenerateSimple(uint8_t opcode);
static void
insn32_GenerateDataOperation(uint8_t opcode, uint32_t data);
static void
insn32_Generate(enum pcode_e opcode, uint32_t arg);

/***********************************************************************
 * Private Functions
 ***********************************************************************/

/***********************************************************************/
/* Disassemble an Op-code */

#if CONFIG_DEBUG
static inline void
insn32_DisassemblePCode(uint8_t opcode, uint32_t arg)
{
  opType_t op;

  op.op  = opcode;
  op.arg = arg;

  insn_DisassemblePCode(g_lstFile, &op);
}
#else
# define insn32_DisassemblePCode(op,a)
#endif

/***********************************************************************/
static inline void
insn32_DisassembleOpcode(uint8_t opcode, uint32_t data)
{
#if CONFIG_DEBUG
  opType_t op;
  op.op  = opcode;
  op.arg = data;
  insn32_DisassemblePCode(opcode, 0);
#endif
}

/***********************************************************************/
/* Generate an Op-Code */

static void
insn32_GenerateSimple(uint8_t opcode)
{
  TRACE(g_lstFile,"[insn32_GenerateSimple:0x%02x]", opcode);

  /* Write the 8-bit opcode */

  poffAddProgByte(g_poffHandle, opcode);

  /* Now, add the disassembled PCode to the list file. */

  insn32_DisassembleOpcode(opcode, 0);
}

/***********************************************************************/

static void
insn32_GenerateDataOperation(uint8_t opcode, uint32_t data)
{
  union
  {
    uint8_t b[4];
    uint32_t w;
  } udata;

  TRACE(g_lstFile,"[insn32_GenerateDataOperation:0x%02x:0x%07x]", opcode, data);

  /* Write the 8-bit opcode */

  poffAddProgByte(g_poffHandle, opcode);

  /* Write the 32-bit opcode */

  udata.w = data;
  (void)poffAddProgByte(g_poffHandle, udata.b[opB1]);
  (void)poffAddProgByte(g_poffHandle, udata.b[opB2]);
  (void)poffAddProgByte(g_poffHandle, udata.b[opB3]);
  (void)poffAddProgByte(g_poffHandle, udata.b[opB4]);

  /* Now, add the disassembled PCode to the list file. */

  insn32_DisassembleOpcode(opcode, data);
}

/***********************************************************************/

static void
insn32_Generate(enum pcode_e opcode, uint32_t arg)
{
  uint8_t insn_opcode = opmap[opcode];

  TRACE(g_lstFile,"[insn32_Generate:0x%02x->0x%02x]", opcode, insn_opcode);

  if (insn_opcode & o32)
    {
      insn32_GenerateDataOperation(insn_opcode, arg);
    }
  else
    {
      insn32_GenerateSimple(insn_opcode);

      /* We ignore the argument... what if one was provided? */

      if (arg != 0)
        {
          warn(eARGIGNORED);
        }
    }
}

/***********************************************************************
 * Public Functions
 ***********************************************************************/

void
insn_GenerateSimple(enum pcode_e opcode)
{
  insn32_Generate(opcode, 0);
}

/***********************************************************************/

void
insn_GenerateDataOperation(enum pcode_e opcode, int32_t data)
{
  insn32_Generate(opcode, (uint32_t)data);
}

/***********************************************************************/
/* Data size for the next multiple register operation (in bytes) is
 * retained in the DC register.
 */

void insn_GenerateDataSize(uint32_t dwDataSize)
{
  insn32_GenerateDataOperation(oSDC, dwDataSize);
}

/***********************************************************************/

void
insn_GenerateFpOperation(uint8_t fpOpcode)
{
  insn32_GenerateDataOperation(oFLOAT, fpOpcode);
}

/***********************************************************************/

void
insn_GenerateSetOperation(uint8_t setOpcode)
{
  insn32_GenerateDataOperation(oSETOP, setOpcode);
}

/***********************************************************************/

void
insn_GenerateIoOperation(uint16_t ioOpcode)
{
  insn32_GenerateDataOperation(oSYSIO, (uint32_t)ioOpcode);
}

/***********************************************************************/

void
insn_StandardFunctionCall(uint16_t libOpcode)
{
  insn32_GenerateDataOperation(oLIB, (uint32_t)libOpcode);
}

/***********************************************************************/

void
insn_GenerateLevelReference(enum pcode_e opcode, uint16_t level, int32_t offset)
{
  /* Note that level is ignored.  We used the level set by the
   * preceding call to insn_SetStaticNestingLevel().
   */

  insn32_Generate(opcode, (uint32_t)offset);
}

/***********************************************************************/

void
insn_GenerateProcedureCall(uint16_t level, int32_t offset)
{
  insn32_GenerateDataOperation(oPCAL, (uint32_t)offset);
}

/***********************************************************************/

void
insn_GenerateLineNumber(uint16_t includeNumber, uint32_t lineNumber)
{
  if (includeNumber != g_nCurrentIncludeNumber)
    {
      g_nCurrentIncludeNumber = includeNumber;
      insn32_GenerateDataOperation(oINCLUDE, includeNumber);
    }
  insn32_GenerateDataOperation(oLINE, lineNumber);
}

/***********************************************************************/

void
insn_SetStaticNestingLevel(uint32_t level)
{
  insn32_GenerateDataOperation(oSLSP, level);
}
