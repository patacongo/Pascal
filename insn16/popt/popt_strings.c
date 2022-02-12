/****************************************************************************
 *  popt_strings.c
 *  String Stack Optimizaitons
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
 ****************************************************************************/

/* The statement generation logic generates a PUSHS and POPS around
 * every statement.  These instructions save and restore the string
 * stack pointer registers.  However, only some statements actually
 * modify the string stack.  So the first major step in the optimatization
 * process is to retain only PUSHS and POPS statements that are
 * actually required.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "pas_debug.h"
#include "pas_machine.h"
#include "pas_errcodes.h"
#include "insn16.h"
#include "pas_sysio.h"
#include "pas_library.h"

#include "popt.h"
#include "pas_error.h"
#include "popt_reloc.h"
#include "popt_strings.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* PBUFFER_SIZE:  The size in bytes of one allocated buffer.  This
 * determines the largest span of instructions between a PUSHS and abort
 * POPS instruction.
 *
 * NPBUFFERS:  The number of buffers to allocate.  This determines the
 * number of nested PUSHS/POPS pairs that can be handled.
 */

#define PBUFFER_SIZE 4096
#define NPBUFFERS       8

#ifdef CONFIG_POPT_DEBUG
#  define popt_DebugMessage(fmt, ...)
#    do { printf(fmt, ##__VA_ARGS__); while (0)
#else
#  define popt_DebugMessage(fmt, ...)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Represents one nested PUSHS/POPS sequence */

struct nestLevel_s
{
  uint8_t  *pBuffer;
  int       nBytesInBuffer;
  uint32_t  inSectionOffset;
};

typedef struct nestLevel_s nestLevel_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static nestLevel_t      g_nestLevel[NPBUFFERS];
static poffRelocation_t g_nextRelocation;
static uint32_t         g_inSectionOffset;
static uint32_t         g_outSectionOffset;
static int32_t          g_nextRelocationIndex;
static int              g_currentLevel = -1;
static int              g_inCh;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int  popt_GetByte(poffHandle_t poffHandle);
static void popt_PutByte(poffProgHandle_t poffProgHandle, uint8_t outCh,
              uint32_t inSectionOffset);
static inline void popt_DropOpcode8(void);
static void popt_PutBuffer(int ch, poffProgHandle_t poffProgHandle);
static void popt_PutOpCode(poffHandle_t poffHandle,
              poffProgHandle_t poffProgHandle);
static void popt_FlushCh(int ch, poffProgHandle_t poffProgHandle);
static void popt_FlushBuffer(poffProgHandle_t poffProgHandle);
static void popt_DoPush(poffHandle_t poffHandle,
              poffProgHandle_t poffProgHandle);
static void popt_DoPop(poffHandle_t poffHandle,
              poffProgHandle_t poffProgHandle);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/

static int popt_GetByte(poffHandle_t poffHandle)
{
  /* Bump the section offsets and get the next byte from the input stream */

  g_inSectionOffset++;
  return poffGetProgByte(poffHandle);
}

/****************************************************************************/

static void popt_PutByte(poffProgHandle_t poffProgHandle, uint8_t outCh,
                         uint32_t inSectionOffset)
{
  uint16_t errCode;

  /* Check if the current section offset matches a relocation entry */

  if (g_nextRelocationIndex >= 0 &&
      g_nextRelocation.rl_offset == inSectionOffset)
    {
      uint32_t saveRlIndex = g_nextRelocation.rl_offset;

      /* Adjust the relocation section offset so that it will match the
       * location in the file after this optimization pass.
       */

      g_nextRelocation.rl_offset -= (inSectionOffset - g_outSectionOffset);

      /* Add the modified relocation to the temporary, output file */

      poffAddTmpRelocation(g_tmpRelocationHandle, &g_nextRelocation);

      /* Get the next relocation entry from the previous pass */

      g_nextRelocationIndex =
        poffNextTmpRelocation(g_prevTmpRelocationHandle, &g_nextRelocation);

      /* Sanity check */

      if (g_nextRelocationIndex >= 0 &&
          g_nextRelocation.rl_offset <= saveRlIndex)
        {
          /* There is no requirement of the format that relocations be
           * ordered by section offset, however, this is how they are
           * generated by the compiler and the current logic depends on
           * this fact.
           */

          error(eBADRELOCDATA);
        }
    }

  errCode = poffAddTmpProgByte(poffProgHandle, outCh);
  if (errCode != eNOERROR)
    {
      fprintf(stderr, "ERROR: Error writing to file: %d\n", errCode);
      exit(1);
    }

  g_outSectionOffset++;
}

/****************************************************************************/

static inline void popt_DropOpcode8(void)
{
  g_outSectionOffset--;
}

/****************************************************************************/

static void popt_PutBuffer(int ch, poffProgHandle_t poffProgHandle)
{
  int dlvl = g_currentLevel;

  if (dlvl < 0)
    {
      /* No PUSHS encountered.  Write byte directly to output */

      popt_PutByte(poffProgHandle, (uint8_t)ch, g_inSectionOffset);
    }
  else
    {
      /* PUSHS encountered.  Write byte into buffer associated with
       * nesting level.
       */

      int idx = g_nestLevel[dlvl].nBytesInBuffer;

      if (idx >= PBUFFER_SIZE)
        {
          fatal(eBUFTOOSMALL);
        }
      else
        {
          uint8_t *dest = g_nestLevel[dlvl].pBuffer + idx;

          *dest = ch;
          g_nestLevel[dlvl].nBytesInBuffer = idx + 1;
        }
    }
}

/****************************************************************************/

static void popt_PutOpCode(poffHandle_t poffHandle,
                           poffProgHandle_t poffProgHandle)
{
  int opCode;

  /* Put the opCode in the buffer */

  popt_PutBuffer(g_inCh, poffProgHandle);

  /* Get the next byte from the input stream */

  opCode = g_inCh;
  g_inCh = popt_GetByte(poffHandle);

  /* Check for an 8-bit argument */

  if ((opCode & o8) != 0)
    {
      /* Buffer the 8-bit argument */

      popt_PutBuffer(g_inCh, poffProgHandle);
      g_inCh = popt_GetByte(poffHandle);
    }

  /* Check for a 16-bit argument */

  if ((opCode & o16) != 0)
    {
      /* Buffer the 16-bit argument */

      popt_PutBuffer(g_inCh, poffProgHandle);
      g_inCh = popt_GetByte(poffHandle);
      popt_PutBuffer(g_inCh, poffProgHandle);
      g_inCh = popt_GetByte(poffHandle);
    }
}

/****************************************************************************/

static void popt_FlushCh(int ch, poffProgHandle_t poffProgHandle)
{
  if (g_currentLevel > 0)
    {
      /* Nested PUSHS encountered. Write byte into buffer associated
       * with the previous nesting level.
       */

      int dlvl               = g_currentLevel - 1;
      int idx                = g_nestLevel[dlvl].nBytesInBuffer;

      if (idx >= PBUFFER_SIZE)
        {
          fatal(eBUFTOOSMALL);
        }
      else
        {
          uint8_t *dest          = g_nestLevel[dlvl].pBuffer + idx;

          *dest                  = ch;
          g_nestLevel[dlvl].nBytesInBuffer = idx + 1;
        }
    }
  else
    {
      /* Only one PUSHS encountered.  Write directly to the output
       * buffer
       */

      popt_PutByte(poffProgHandle, (uint8_t)ch, g_inSectionOffset);
    }
}

/****************************************************************************/

static void popt_FlushBuffer(poffProgHandle_t poffProgHandle)
{
  int slvl = g_currentLevel;

  if (g_nestLevel[slvl].nBytesInBuffer > 0)
    {
      if (g_currentLevel > 0)
        {
          /* Nested PUSHS encountered. Flush buffer into buffer associated
           * with the previous nesting level.
           *
           * REVISIT:  What happens to the inSectionOffset at the higher
           * nesting level?
           */

          int dlvl = slvl - 1;

          if (g_nestLevel[dlvl].nBytesInBuffer +
              g_nestLevel[slvl].nBytesInBuffer >= PBUFFER_SIZE)
            {
              fatal(eBUFTOOSMALL);
            }
          else
            {
              uint8_t *src  = g_nestLevel[slvl].pBuffer;
              uint8_t *dest = g_nestLevel[dlvl].pBuffer + g_nestLevel[dlvl].nBytesInBuffer;

              memcpy(dest, src, g_nestLevel[slvl].nBytesInBuffer);
              g_nestLevel[dlvl].nBytesInBuffer += g_nestLevel[slvl].nBytesInBuffer;
            }
        }
      else
        {
          uint32_t inSectionOffset;
          int i;

          /* Only one PUSHS encountered.  Flush directly to the output
           * buffer.  poffWriteTmpProgBytes() could do this as a single
           * operation.  However, we need to check byte-by-byte for
           * relocation data.
           */

          for (i = 0, inSectionOffset = g_nestLevel[0].inSectionOffset;
               i < g_nestLevel[0].nBytesInBuffer;
               i++, inSectionOffset++)
            {
              popt_PutByte(poffProgHandle, g_nestLevel[0].pBuffer[i],
                           inSectionOffset);
            }
        }
    }

  g_nestLevel[slvl].nBytesInBuffer = 0;
}

/****************************************************************************/

static void popt_DoPush(poffHandle_t poffHandle,
                        poffProgHandle_t poffProgHandle)
{
  while (g_inCh != EOF)
    {
      /* Search for a PUSHS opCode */

      if (g_inCh != oPUSHS)
        {
          /* Its not PUSHS, just echo to the output file/buffer */

          popt_PutOpCode(poffHandle, poffProgHandle);
        }
      else
        {
          /* We have found PUSHS.  Now search for the next occurrence of
           * either an instruction that increments the string stack or for
           * the matching POPS
           */

          g_currentLevel++;
          if (g_currentLevel >= NPBUFFERS)
            {
              fatal(eBUFTOOSMALL);
            }

          /* REVISIT:  This offset is not used (only at level 0) */

          g_nestLevel[g_currentLevel].inSectionOffset = g_inSectionOffset;

          /* Skip over the PUSHS and look for the POPS. */

          g_inCh = popt_GetByte(poffHandle);
          popt_DoPop(poffHandle, poffProgHandle);
          g_currentLevel--;
        }
    }
}

/****************************************************************************/

static void popt_DoPop(poffHandle_t poffHandle,
                       poffProgHandle_t poffProgHandle)
{
#ifdef CONFIG_POPT_DEBUG
  int pushOffset = g_inSectionOffset - 2;
#endif

  /* REVISIT:  KeepOp is set true if we decide to keep the PUSHS/POPS.  If
   * keepOp is set, we still continue buffering data until the matching POPS
   * is found.  This requires substantial sizes for PBUFFER_SIZE (like 4096
   * vs. 1024).
   */

  bool keepPop = false;

  /* We have found PUSHS.  Now search for the next occurrence of either an
   * instruction that increments the string stack or for the matching POPS
   */

  popt_DebugMessage("Consider PUSH at %04x, level %d\n",
                     pushOffset, g_currentLevel);
  while (g_inCh != EOF)
    {
      switch (g_inCh)
        {
          /* Did we encounter another PUSHS? */

          case oPUSHS :
            {
              /* Yes... recurse to handle it */

              g_currentLevel++;
              if (g_currentLevel >= NPBUFFERS)
                {
                  fatal(eBUFTOOSMALL);
                }

              /* REVISIT:  This offset is not used (only at level 0) */

              g_nestLevel[g_currentLevel].inSectionOffset = g_inSectionOffset;

              /* Skip over the PUSH and look for the matching POPS */

              g_inCh = popt_GetByte(poffHandle);
              popt_DoPop(poffHandle, poffProgHandle);
              g_currentLevel--;
            }
            break;

          case oPOPS :
            {
              if (keepPop)
                {
                  popt_DebugMessage("  Keep PUSH at %04x and POPS at %04x, "
                                    "level %d\n",
                                    pushOffset, g_inSectionOffset - 2,
                                    g_currentLevel);

                  /* Copy the POPS to the buffer */

                  popt_PutOpCode(poffHandle, poffProgHandle);

                  /* Flush the buffered data with the PUSHS */

                  popt_FlushCh(oPUSHS, poffProgHandle);
                  popt_FlushBuffer(poffProgHandle);
                }
              else
                {
                  /* Flush the buffered data without the PUSHS */

                  popt_DebugMessage("  Drop PUSH at %04x and POPS at %04x, "
                                    "level %d\n",
                                    pushOffset, g_inSectionOffset - 2,
                                    g_currentLevel);

                  popt_DropOpcode8();
                  popt_FlushBuffer(poffProgHandle);

                  /* And discard the matching POPS */

                  popt_DropOpcode8();
                  g_inCh = popt_GetByte(poffHandle);
                }
            }
            return;

          case oLIB :
            {
              int arg16;
              int arg16a;
              int arg16b;

              /* Get the 16-bit argument */

              popt_PutBuffer(g_inCh, poffProgHandle);
              arg16a = popt_GetByte(poffHandle);
              popt_PutBuffer(arg16a, poffProgHandle);
              arg16b = popt_GetByte(poffHandle);
              popt_PutBuffer(arg16b, poffProgHandle);
              arg16  = (arg16a << 8) | arg16b;
              g_inCh = popt_GetByte(poffHandle);

              /* Is it LIB STRINIT? STRDUP? or other string library functions
               * that we should not optimize?  These functions all allocate
               * new memory from the string stack.
               */

              if (arg16 == lbSTRINIT  ||
                  arg16 == lbSSTRINIT ||
                  arg16 == lbSTRTMP   ||
                  arg16 == lbSTRDUP   ||
                  arg16 == lbSSTRDUP  ||
                  arg16 == lbMKSTKC   ||
                  arg16 == lbBSTR2STR)
                {
                  popt_DebugMessage("  Keep PUSH at %04x, level %d\n",
                                    pushOffset, g_currentLevel);

                  /* Put the instruction in the buffer and remind ourselves
                   * to keep both the PUSHS and the POPS when we find the
                   * matching POPS.
                   */

                  popt_PutOpCode(poffHandle, poffProgHandle);
                  keepPop = true;
                }
            }
            break;

#if 1         /* REVISIT: Increases code size dramatically. */

              /* If we encounter a label or a jump between the PUSHS and the
               * POPS, then keep both.  Labels are known to happen in loops
               * where the top-of-loop label is after the PUSHS but the
               * matching POPS may be much later in the file.
               */

          case oJMP   : /* Unconditional */
          case oJEQUZ : /* Unary comparisons with zero */
          case oJNEQZ :
          case oJLTZ  :
          case oJGTEZ :
          case oJGTZ  :
          case oJLTEZ :
          case oJEQU  : /* Binary comparisons */
          case oJNEQ  :
          case oJLT   :
          case oJGTE  :
          case oJGT   :
          case oJLTE  :
          case oLABEL : /* Label */
            {
              popt_DebugMessage("  Keep PUSH at %04x, level %d\n",
                                pushOffset, g_currentLevel);

              /* Put the instruction in the buffer and remind ourselves
               * to keep both the PUSHS and the POPS when we find the
               * matching POPS.
               */

              popt_PutOpCode(poffHandle, poffProgHandle);
              keepPop = true;
            }
            break;
#endif

          case EOF:
            /* What?  No matching POPS? */

            fatal(eHUH);
            break;

          default:
            {
              /* Something else.  Put it in the buffer */

              popt_PutOpCode(poffHandle, poffProgHandle);
            }
            break;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/

void popt_StringStackOptimize(poffHandle_t poffHandle,
                              poffProgHandle_t poffProgHandle)
{
  int i;

  /* Allocate an array of buffers to hold pcode data */

  for (i = 0; i < NPBUFFERS; i++)
    {
      g_nestLevel[i].pBuffer = (uint8_t*)malloc(PBUFFER_SIZE);
      if (g_nestLevel[i].pBuffer == NULL)
        {
          fatal(eNOMEMORY);
        }

      g_nestLevel[i].nBytesInBuffer = 0;
    }

  /* Prime the search logic */

  g_inCh                 = popt_GetByte(poffHandle);
  g_currentLevel         = -1;

  g_inSectionOffset      = 0;
  g_outSectionOffset     = 0;

  /* Get the next relocation entry (before string optimization) */

  g_nextRelocationIndex  =
    poffNextTmpRelocation(g_prevTmpRelocationHandle, &g_nextRelocation);

  /* And parse the input file to the output file, removing unnecessary string
   * stack operations.
   */

  popt_DoPush(poffHandle, poffProgHandle);

  /* Release the buffers */

  for (i = 0; i < NPBUFFERS; i++)
    {
      free(g_nestLevel[i].pBuffer);
      g_nestLevel[i].pBuffer        = NULL;
      g_nestLevel[i].nBytesInBuffer = 0;
    }

  /* All of the relocations should have been adjusted and copied to the
   * optimized output.
   */

  if (g_nextRelocationIndex >= 0)
    {
      error(eEXTRARELOCS);
    }
}
