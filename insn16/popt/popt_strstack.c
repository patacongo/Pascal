/****************************************************************************
 *  popt_strstack.c
 *  String Stack Optimizaitons
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
#include "pofflib.h"
#include "pas_insn.h"
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

/* NOPCODES_BUFFER:  The size in opcodes of one allocated code chunk.
 *
 * MAX_NESTING:  This determines the maximum number of nested PUSHS/POPS
 I pairs that can be handled.
 */

#define NOPCODES_BUFFER 256
#define MAX_NESTING     32

#ifdef CONFIG_POPT_DEBUG
#  define popt_DebugMessage(fmt, ...)
#    do { printf(fmt, ##__VA_ARGS__); while (0)
#else
#  define popt_DebugMessage(fmt, ...)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Represents on one buffer of instructions in a doubly linked list */

struct codeChunk_s
{
  struct codeChunk_s *next;           /* Supports a singly linked list */
  int nOpCodes;                       /* Max+1 opcode index */
  int firstOpCode;                    /* Index of first valid opcode */
  opTypeR_t opCode[NOPCODES_BUFFER];  /* Buffered opcodes */
};

typedef struct codeChunk_s codeChunk_t;

/* Represents one nested PUSHS/POPS sequence */

struct nestLevelHead_s
{
  codeChunk_t *head;
  codeChunk_t *tail;
};

typedef struct nestLevelHead_s nestLevelHead_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static nestLevelHead_t  g_nestLevel[MAX_NESTING];
static codeChunk_t     *g_freeCodeChunk;
static opTypeR_t        g_opCode;
static int              g_currentLevel = -1;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void popt_FreeCodeChunks(codeChunk_t *chunkHead);
static codeChunk_t *popt_AllocateCodeChunk(void);
static void popt_AddTailOpCode(nestLevelHead_t *levelHead,
                               const opTypeR_t *opCode);
static const opTypeR_t *popt_PeekHeadOpCode(nestLevelHead_t *levelHead);
static void popt_DiscardHeadOpCode(nestLevelHead_t *levelHead);
static void popt_MergeLevels(nestLevelHead_t *srcHead,
                             nestLevelHead_t *destHead);

static void popt_GetOpCode(poffHandle_t poffHandle);
static void popt_WriteOpCode(poffProgHandle_t poffProgHandle,
              const opTypeR_t *opCode);
static void popt_PutBuffer(poffProgHandle_t poffProgHandle,
              const opTypeR_t *opCode);
static void popt_PutOpCode(poffHandle_t poffHandle,
              poffProgHandle_t poffProgHandle);
static void popt_FlushOpCode(poffProgHandle_t poffProgHandle,
              const opTypeR_t *opCode);
static void popt_FlushBuffer(poffProgHandle_t poffProgHandle);
static void popt_DoPush(poffHandle_t poffHandle,
              poffProgHandle_t poffProgHandle);
static void popt_DoPop(poffHandle_t poffHandle,
              poffProgHandle_t poffProgHandle);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/

static void popt_FreeCodeChunks(codeChunk_t *chunkHead)
{
  codeChunk_t *curr;
  codeChunk_t *next;

  for (curr = chunkHead; curr != NULL; curr = next)
    {
      next = curr->next;
      free(curr);
    }
}

/****************************************************************************/

static codeChunk_t *popt_AllocateCodeChunk(void)
{
  codeChunk_t *chunk;

  /* Check if there is a code chunk in the free list */

  if (g_freeCodeChunk != NULL)
    {
      chunk           = g_freeCodeChunk;
      g_freeCodeChunk = chunk->next;
    }
  else
    {
      chunk = (codeChunk_t *)malloc(sizeof(codeChunk_t));
      if (chunk == NULL)
        {
          fatal(eNOMEMORY);
        }
    }

  return chunk;
}

/****************************************************************************/

static void popt_AddTailOpCode(nestLevelHead_t *levelHead,
                               const opTypeR_t *opCode)
{
  codeChunk_t *oldTail = levelHead->tail;

  /* Is the list empty?  No? Is there space for another opcode in the last
   * chunk?
   */

  if (oldTail == NULL || oldTail->nOpCodes >= NOPCODES_BUFFER)
    {
      codeChunk_t *newTail;

      /* No, allocate a new tail chunk */

      newTail              = popt_AllocateCodeChunk();
      newTail->next        = NULL;
      newTail->nOpCodes    = 0;
      newTail->firstOpCode = 0;

      /* And attach it to the end of the list */

      levelHead->tail      = newTail;

      /* Was the list previously empty? */

      if (oldTail == NULL)
        {
          /* Yes, then this is the new head too */

          levelHead->head  = newTail;

        }
      else
        {
          /* No, then it belongs right after the old tail */

          oldTail->next    = newTail;
        }

      oldTail              = newTail;
    }

  /* Add the new opcode at the end of the tail chunk */

  oldTail->opCode[oldTail->nOpCodes] = *opCode;
  oldTail->nOpCodes++;
}

/****************************************************************************/

static const opTypeR_t *popt_PeekHeadOpCode(nestLevelHead_t *levelHead)
{
  codeChunk_t *head = levelHead->head;

  /* Remove and return the opcode from the chunk */

  return &head->opCode[head->firstOpCode];
}

/****************************************************************************/

static void popt_DiscardHeadOpCode(nestLevelHead_t *levelHead)
{
  codeChunk_t *oldHead = levelHead->head;

  /* Bump the index to the first valid opcode in the chunk. */

  oldHead->firstOpCode++;

  /* Was that the last opcode in the chunk? */

  if (oldHead->firstOpCode >= oldHead->nOpCodes)
    {
      /* Yes, remove and free the head chunk */

      levelHead->head   = oldHead->next;
      oldHead->next     = g_freeCodeChunk;
      g_freeCodeChunk   = oldHead;

      /* Is the chunk list now empty? */

      if (levelHead->head == NULL)
        {
          /* Yes, then there is no tail either */

          levelHead->tail = NULL;
        }
    }
}

/****************************************************************************/

static void popt_MergeLevels(nestLevelHead_t *srcHead,
                             nestLevelHead_t *destHead)
{
  if (srcHead->head != NULL)
    {
      destHead->tail->next = srcHead->head;
      destHead->tail       = srcHead->tail;

      srcHead->head        = NULL;
      srcHead->tail        = NULL;
    }
}

/****************************************************************************/

static void popt_GetOpCode(poffHandle_t poffHandle)
{
  /* Get the next opcode from the input file */

  uint32_t opSize    = insn_GetOpCode(poffHandle, (opType_t *)&g_opCode);

  /* Save the input program section offset.  We need this to handle moving
   * the relocation when instructions are deleted.
   */

  g_opCode.offset    = g_inSectionOffset;
  g_inSectionOffset += opSize;
}

/****************************************************************************/

static void popt_WriteOpCode(poffProgHandle_t poffProgHandle,
                             const opTypeR_t *opCode)
{
  /* Check if the current section offset matches a relocation entry */

  if (g_nextRelocationIndex >= 0 &&
      g_nextRelocation.rl_offset == opCode->offset)
    {
      uint32_t saveRlIndex = g_nextRelocation.rl_offset;

      /* Adjust the relocation section offset so that it will match the
       * location in the file after this optimization pass.
       */

      g_nextRelocation.rl_offset -= (opCode->offset - g_outSectionOffset);

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

  /* Write the opcode to the temporary program data */

  g_outSectionOffset += insn_AddTmpOpCode(poffProgHandle, (opType_t *)opCode);
}

/****************************************************************************/

static void popt_PutBuffer(poffProgHandle_t poffProgHandle,
                           const opTypeR_t *opCode)
{
  int currentLevel = g_currentLevel;

  if (currentLevel < 0)
    {
      /* No PUSHS encountered.  Write opcode directly to output */

      popt_WriteOpCode(poffProgHandle, opCode);
    }
  else
    {
      /* PUSHS encountered.  Write byte into buffer associated with
       * nesting level.
       */

      popt_AddTailOpCode(&g_nestLevel[currentLevel], opCode);
    }
}

/****************************************************************************/

static void popt_PutOpCode(poffHandle_t poffHandle,
                           poffProgHandle_t poffProgHandle)
{
  /* Put the opCode into the buffer */

  popt_PutBuffer(poffProgHandle, &g_opCode);

  /* Get the next byte from the input stream */

  popt_GetOpCode(poffHandle);
}

/****************************************************************************/

static void popt_FlushOpCode(poffProgHandle_t poffProgHandle,
                             const opTypeR_t *opCode)
{
  if (g_currentLevel > 0)
    {
      /* Nested PUSHS encountered. Write byte into buffer associated
       * with the previous nesting level.
       */

      popt_AddTailOpCode(&g_nestLevel[g_currentLevel - 1], opCode);
    }
  else
    {
      /* Only one PUSHS encountered.  Write directly to the output
       * buffer
       */

      popt_WriteOpCode(poffProgHandle, opCode);
    }
}

/****************************************************************************/

static void popt_FlushBuffer(poffProgHandle_t poffProgHandle)
{
  int currentLevel = g_currentLevel;

  if (currentLevel > 0)
    {
      /* Nested PUSHS encountered. Flush buffer into buffer associated with
       * the previous nesting level.
       */

      popt_MergeLevels(&g_nestLevel[currentLevel],
                       &g_nestLevel[currentLevel - 1]);
    }
  else
    {
      /* Loop while there are still opcodes at level 0 */

      while (g_nestLevel[0].head != NULL)
        {
          /* Write the opcode directly from the code chunk to the output
           * data.
           */

          const opTypeR_t *opCode = popt_PeekHeadOpCode(&g_nestLevel[0]);
          popt_WriteOpCode(poffProgHandle, opCode);

          /* Then discard the opcode */

          popt_DiscardHeadOpCode(&g_nestLevel[0]);
        }
    }
}

/****************************************************************************/

static void popt_DoPush(poffHandle_t poffHandle,
                        poffProgHandle_t poffProgHandle)
{
  while (g_opCode.op != oEND)
    {
      /* Search for a PUSHS opCode */

      if (g_opCode.op != oPUSHS)
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
          if (g_currentLevel >= MAX_NESTING)
            {
              fatal(eBUFTOOSMALL);
            }

          /* Skip over the PUSHS and look for the POPS. */

          popt_GetOpCode(poffHandle);
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
   * is found.  This requires substantial sizes for NOPCODES_BUFFER (like 4096
   * vs. 1024).
   */

  bool keepPop = false;

  /* We have found PUSHS.  Now search for the next occurrence of either an
   * instruction that increments the string stack or for the matching POPS
   */

  popt_DebugMessage("Consider PUSH at %04x, level %d\n",
                     pushOffset, g_currentLevel);
  while (g_opCode.op != oEND)
    {
      switch (g_opCode.op)
        {
          /* Did we encounter another PUSHS? */

          case oPUSHS :
            {
              /* Yes... recurse to handle it */

              g_currentLevel++;
              if (g_currentLevel >= MAX_NESTING)
                {
                  fatal(eBUFTOOSMALL);
                }

              /* Skip over the PUSH and look for the matching POPS */

              popt_GetOpCode(poffHandle);
              popt_DoPop(poffHandle, poffProgHandle);
              g_currentLevel--;
            }
            break;

          case oPOPS :
            {
              if (keepPop)
                {
                  opTypeR_t pushOpCode;

                  popt_DebugMessage("  Keep PUSH at %04x and POPS at %04x, "
                                    "level %d\n",
                                    pushOffset, g_inSectionOffset - 2,
                                    g_currentLevel);

                  /* Copy the POPS to the buffer */

                  popt_PutOpCode(poffHandle, poffProgHandle);

                  /* Flush the buffered data with the PUSHS */

                  pushOpCode.op     = oPUSHS;
                  pushOpCode.arg1   = 0;
                  pushOpCode.arg2   = 0;
                  pushOpCode.offset = 0;

                  popt_FlushOpCode(poffProgHandle, &pushOpCode);
                  popt_FlushBuffer(poffProgHandle);
                }
              else
                {
                  /* Flush the buffered data without the PUSHS */

                  popt_DebugMessage("  Drop PUSH at %04x and POPS at %04x, "
                                    "level %d\n",
                                    pushOffset, g_inSectionOffset - 2,
                                    g_currentLevel);

                  popt_FlushBuffer(poffProgHandle);

                  /* And discard the matching POPS */

                  popt_GetOpCode(poffHandle);
                }
            }
            return;

          case oLIB :
            {
              /* Is it LIB STRINIT? STRDUP? or other string library functions
               * that we should not optimize?  These functions all allocate
               * new memory from the string stack.
               */

              if (g_opCode.arg2 == lbSTRINIT    ||
                  g_opCode.arg2 == lbSTRTMP     ||
                  g_opCode.arg2 == lbSTRDUP     ||
                  g_opCode.arg2 == lbMKSTKC     ||
                  g_opCode.arg2 == lbBSTR2STR   ||
                  g_opCode.arg2 == lbCOPYSUBSTR)
                {
                  popt_DebugMessage("  Keep PUSH at %04x, level %d\n",
                                    pushOffset, g_currentLevel);

                  /* Remind ourselves to keep both the PUSHS and the POPS
                   * when we find the matching POPS.
                   */

                  keepPop = true;
                }

              /* Put the instruction in the buffer in any event. */

              popt_PutOpCode(poffHandle, poffProgHandle);
            }
            break;

          case oSYSIO :
            {
              /* GetDir also allocates string stack. */

              if (g_opCode.arg2 == xGETDIR)
                {
                  popt_DebugMessage("  Keep PUSH at %04x, level %d\n",
                                    pushOffset, g_currentLevel);

                  /* Remind ourselves to keep both the PUSHS and the POPS
                   * when we find the matching POPS.
                   */

                  keepPop = true;
                }

              /* Put the instruction in the buffer in any event. */

              popt_PutOpCode(poffHandle, poffProgHandle);
            }
            break;

#if 1       /* REVISIT: Increases code size dramatically. */

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

          case oEND:
            /* What?  No matching POPS? */

            fprintf(stderr, "ERROR: No matching POPS!\n");
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

  /* Prime the search logic */

  g_currentLevel         = -1;
  g_inSectionOffset      = 0;
  g_outSectionOffset     = 0;

  /* Get the first relocation entry */

  g_nextRelocationIndex  =
    poffNextTmpRelocation(g_prevTmpRelocationHandle, &g_nextRelocation);

  /* And parse the input file to the output file, removing unnecessary string
   * stack operations.
   */

  popt_GetOpCode(poffHandle);
  popt_DoPush(poffHandle, poffProgHandle);

  /* Release the buffers */

  for (i = 0; i < MAX_NESTING; i++)
    {
      if (g_nestLevel[i].head != NULL)
        {
          fprintf(stderr, "ERROR: Unwritten program data!\n");
          popt_FreeCodeChunks(g_nestLevel[i].head);
        }

      g_nestLevel[i].head = NULL;
      g_nestLevel[i].tail = NULL;
    }

  popt_FreeCodeChunks(g_freeCodeChunk);

  /* All of the relocations should have been adjusted and copied to the
   * optimized output.
   */

  if (g_nextRelocationIndex >= 0)
    {
      error(eEXTRARELOCS);
    }
}
