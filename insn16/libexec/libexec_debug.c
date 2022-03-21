/**************************************************************************
 * libexec_debug.c
 * P-Code Debugger
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
 ***************************************************************************/

/**************************************************************************
 * Included Files
 ***************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <getopt.h>
#include <ctype.h>

#if defined(CONFIG_PASCAL_BUILD_NUTTX) && defined(CONFIG_SYSTEM_READLINE)
#include "system/readline.h"
#endif

#include "pofflib.h"
#include "paslib.h"
#include "execlib.h"

#include "pas_debug.h"
#include "pas_machine.h"
#include "pas_pcode.h"
#include "pas_sysio.h"
#include "pas_errcodes.h"
#include "pas_insn.h"

#include "insn16.h"
#include "libexec.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_PASCAL_BUILD_NUTTX) && defined(CONFIG_SYSTEM_READLINE)
#  define GETS(b,s,f) ((readline(b,s,stdin,f) >= 0) ? (b) : NULL)
#else
#  define GETS(b,s,f) fgets(b,s,f)
#endif

/**************************************************************************
 * Private Function Prototypes
 ***************************************************************************/

static void      pdbg_ShowCommands(void);
static void      pdbg_ExecCommand(struct libexec_s *st,
                   enum command_e cmd, uint32_t value);
static int32_t   pdbg_ReadDecimal(char *ptr);
static int32_t   pdbg_ReadHex(char *ptr, int32_t defaultvalue);
static bool      pdbg_ValidAddress(struct libexec_s *st, uint16_t addr);
static void      pdbg_ProgramStatus(struct libexec_s *st);
static pasSize_t pdbg_PrintPCode(struct libexec_s *st, pasSize_t pc,
                   int16_t nitems);
static pasSize_t pdbg_PrintStack(struct libexec_s *st, pasSize_t sp,
                   int16_t nitems);
static void      pdbg_PrintRegisters(struct libexec_s *st);
static void      pdbg_PrintWatchpoint(struct libexec_s *st);
static void      pdbg_PrintTraceArray(struct libexec_s *st);
static void      pdbg_AddBreakPoint(struct libexec_s *st, pasSize_t pc);
static void      pdbg_DeleteBreakPoint(struct libexec_s *st, int16_t bpno);
static void      pdbg_PrintBreakPoints(struct libexec_s *st);
static void      pdbg_CheckBreakPoint(struct libexec_s *st);
static void      pdbg_AddWatchPoint(struct libexec_s *st, ustack_t addr);
static void      pdbg_ClearWatchPoint(struct libexec_s *st, int16_t wpno);
static void      pdbg_InitDebugger(struct libexec_s *st);
static void      pdbg_DebugPCode(struct libexec_s *st);

/**************************************************************************
 * Private Functions
 ***************************************************************************/
/* Show command characters */

static void pdbg_ShowCommands(void)
{
   printf("Commands:\n");
   printf("  RE[set]   - Reset\n");
   printf("  RU[n]     - Run\n");
   printf("  S[tep]    - Single Step (Into)\n");
   printf("  N[ext]    - Single Step (Over)\n");
   printf("  G[o]      - Go\n");
   printf("  BS xxxx   - Set Breakpoint\n");
   printf("  BC n      - Clear Breakpoint\n");
   printf("  WS xxxx   - [Re]set Watchpoint\n");
   printf("  WF xxxx   - Level 0 Frame Watchpoint\n");
   printf("  WC        - Clear Watchpoint\n");
   printf("  DP        - Display Program Status\n");
   printf("  DT        - Display Program Trace\n");
   printf("  DS [xxxx] - Display Stack\n");
   printf("  DI [xxxx] - Display Instructions\n");
   printf("  DB        - Display Breakpoints\n");
   printf("  H or ?    - Shows this list\n");
   printf("  Q[uit]    - Quit\n");
}

/***************************************************************************/
static void pdbg_ExecCommand(struct libexec_s *st, enum command_e cmd, uint32_t value)
{
  /* Save the command to resuse if the user enters nothing */

  st->lastCmd   = cmd;
  st->lastValue = value;

  switch (cmd)
    {
    case eCMD_NONE:   /* Do nothing */
      break;

    case eCMD_RESET:  /* Reset */
      libexec_Reset(st);
      pdbg_InitDebugger(st);
      pdbg_ProgramStatus(st);
      st->lastCmd = eCMD_NONE;
      break;

    case eCMD_RUN:    /* Run */
      libexec_Reset(st);
      pdbg_InitDebugger(st);
      pdbg_DebugPCode(st);
      pdbg_ProgramStatus(st);
      break;

    case eCMD_STEP:   /* Single Step (into)*/
      st->bExecStop = true;
      pdbg_DebugPCode(st);
      pdbg_ProgramStatus(st);
      break;

    case eCMD_NEXT:   /* Single Step (over) */
      if (st->ispace[st->pc] == oPCAL)
        {
          st->bExecStop = false;
          st->untilPoint = st->pc + 4;
        }
      else
        {
          st->bExecStop = true;
        }

      pdbg_DebugPCode(st);
      st->untilPoint = 0;
      pdbg_ProgramStatus(st);
      break;

    case eCMD_GO:     /* Go */
      st->bExecStop = false;
      pdbg_DebugPCode(st);
      pdbg_ProgramStatus(st);
      break;

    case eCMD_BS:     /* Set Breakpoint */
      if (st->nBreakPoints >= MAX_BREAK_POINTS)
        {
          printf("Too many breakpoints\n");
          st->lastCmd = eCMD_NONE;
        }
      else if (value >= st->maxpc)
        {
          printf("Invalid address for breakpoint\n");
          st->lastCmd = eCMD_NONE;
        }
      else
        {
          pdbg_AddBreakPoint(st, value);
          pdbg_PrintBreakPoints(st);
        }
      break;

    case eCMD_BC:     /* Clear Breakpoint */
      if (value >= 1 && value <= st->nBreakPoints)
        {
          pdbg_DeleteBreakPoint(st, value);
        }
      else
        {
          printf("Invalid breakpoint number\n");
          st->lastCmd = eCMD_NONE;
        }

      pdbg_PrintBreakPoints(st);
      break;

    case eCMD_WS:     /* Set Breakpoint */
      if (st->nWatchPoints >= MAX_WATCH_POINTS)
        {
          printf("Too many watchpoints\n");
          st->lastCmd = eCMD_NONE;
        }
      else if (value >= st->stackSize)
        {
          printf("Invalid address for watchpoint\n");
          st->lastCmd = eCMD_NONE;
        }
      else
        {
          pdbg_AddWatchPoint(st, value);
        }
      break;

    case eCMD_WC:     /* Clear Watchpoint */
      pdbg_ClearWatchPoint(st, value);
      break;

    case eCMD_DP:     /* Display Program Status */
      pdbg_ProgramStatus(st);
      break;

    case eCMD_DT:     /* Display Program Trace */
      pdbg_PrintTraceArray(st);
      break;

    case eCMD_DS:     /* Display Stack (or heap) */
      if (!pdbg_ValidAddress(st, value))
        {
          printf("Invalid stack address\n");
          st->lastCmd = eCMD_NONE;
        }
      else
        {
          st->lastValue = pdbg_PrintStack(st, value, DISPLAY_STACK_SIZE);
        }
      break;

    case eCMD_DI:     /* Display Instructions */
      if (value >= st->maxpc)
        {
          printf("Invalid instruction address\n");
          st->lastCmd = eCMD_NONE;
        }
      else
        {
          st->lastValue = pdbg_PrintPCode(st, value, DISPLAY_INST_SIZE);
        }
      break;

    case eCMD_DB:     /* Display Breakpoints */
      pdbg_PrintBreakPoints(st);
      break;

    case eCMD_QUIT:   /* Quit */
      printf("Goodbye\n");
      exit(0);
      break;

    case eCMD_HELP:   /* Help */
    default:          /* Internal error */
      pdbg_ShowCommands();
      st->lastCmd = eCMD_NONE;
      break;
    }
}

/***************************************************************************/
/* Read a decimal value from the input string */

static int32_t pdbg_ReadDecimal(char *ptr)
{
  int32_t decimal = 0;

  while (!isspace((int)*ptr)) ptr++;
  while (isspace((int)*ptr))  ptr++;
  for (; ((*ptr >= '0') && (*ptr <= '9')); ptr++)
    {
      decimal = 10*decimal + (int32_t)*ptr - (int32_t)'0';
    }

  return decimal;
}

/***************************************************************************/
/* Read a hexadecimal value from the  input string */

static int32_t pdbg_ReadHex(char *ptr, int32_t defaultvalue)
{
   char    c;
   int32_t hex = 0;
   bool    found = false;

   while (!isspace((int)*ptr)) ptr++;
   while (isspace((int)*ptr))  ptr++;
   while (true) {

      c = toupper(*ptr);
      if ((c >= '0') && (c <= '9')) {
         hex = ((hex << 4) | ((int32_t)c - (int32_t)'0'));
         found = true;
      }
      else if ((c >= 'A') && (c <= 'F')) {
         hex = ((hex << 4) | ((int32_t)c - (int32_t)'A' + 10));
         found = true;
      }
      else {
         if (found)
            return hex;
         else
            return defaultvalue;
      }
      ptr++;
   }
}

/***************************************************************************/

static bool pdbg_ValidAddress(struct libexec_s *st, uint16_t addr)
{
  return ((st->sp < st->stackSize && addr <= st->sp) ||
          (addr >= st->hpb && addr < st->hpb + st->hpSize));
}

/***************************************************************************/
/* Print the disassembled P-Code at PC */

static void pdbg_ProgramStatus(struct libexec_s *st)
{
  (void)pdbg_PrintPCode(st, st->pc, 1);
  (void)pdbg_PrintStack(st, st->sp, 2);
  pdbg_PrintRegisters(st);
  pdbg_PrintWatchpoint(st);
}

/***************************************************************************/
/* Print the disassembled P-Code at PC */

static pasSize_t pdbg_PrintPCode(struct libexec_s *st, pasSize_t pc, int16_t nitems)
{
  opType_t op;
  pasSize_t  opsize;
  uint8_t *address;

  for (; pc < st->maxpc && nitems > 0; nitems--)
    {
      address  = &st->ispace[pc];

      op.op    = *address++;
      op.arg1  = 0;
      op.arg2  = 0;
      opsize   = 1;

      printf("PC:%04" PRIx16 "  %02x", pc, op.op);

      if ((op.op & o8) != 0)
        {
          op.arg1 = *address++;
          printf("%02x", op.arg1);
          opsize++;
        }
      else
        {
          printf("..");
        }

      if ((op.op & o16) != 0)
        {
          op.arg2  = ((*address++) << 8);
          op.arg2 |= *address++;
          printf("%04" PRIx16 "", op.arg2);
          opsize += 2;
        }
      else
        {
          printf("....");
        }

      /* The disassemble it to stdout */

      printf("  ");

      /* Treat long operations as a transparent extension to the instruction set */

      if (op.op == oLONGOP8 || op.op == oLONGOP24)
        {
          insn_DisassembleLongOpCode(stdout, &op);
        }
      else
        {
          insn_DisassemblePCode(stdout, &op);
        }

      /* Get the address of the next P-Code */

      pc += opsize;
    }

  return pc;
}

/***************************************************************************/
/* Print the stack value at SP */

static pasSize_t pdbg_PrintStack(struct libexec_s *st, pasSize_t sp,
                                 int16_t nitems)
{
  int32_t isp;

  if (pdbg_ValidAddress(st, sp))
    {
      isp = BTOISTACK(sp);
      printf("SP:%04" PRIx16 "  %04" PRIx16 "\n",
             sp, st->dstack.i[isp]);

      for (isp--, sp -= BPERI, nitems--;
           ((isp >= 0) && (nitems > 0));
           isp--, sp -= BPERI, nitems--)
        {
          printf("   %04" PRIx16 "  %04" PRIx16 "\n",
                 sp, st->dstack.i[isp] & 0xffff);
        }
    }
  else
    {
      printf("SP:%04" PRIx16 "  BAD\n", sp);
    }

  return sp;
}

/***************************************************************************/
/* Print the base register */

static void pdbg_PrintRegisters(struct libexec_s *st)
{
  if (st->fp <= st->sp)
    {
      printf("FP:%04" PRIx16 " ", st->fp);
    }

  printf("CSP:%04" PRIx16 "\n", st->csp);
}

/***************************************************************************/
/* Print the watchpoint SP */

static void pdbg_PrintWatchpoint(struct libexec_s *st)
{
  if (st->nWatchPoints > 0)
    {
      uint16_t isp = BTOISTACK(st->watchPoint[0]);

      if (pdbg_ValidAddress(st, isp))
        {
          printf("WP:%04" PRIx16 "  %04" PRIx16 "\n",
                 st->watchPoint[0], st->dstack.i[isp]);
        }
      else
        {
          printf("WP:%04" PRIx16 "  xxxx\n", st->watchPoint[0]);
        }
    }
}

/***************************************************************************/
/* Print the traceArray */

static void pdbg_PrintTraceArray(struct libexec_s *st)
{
  int nprinted;
  int index;

  index = st->traceIndex + TRACE_ARRAY_SIZE - st->nTracePoints;
  if (index >= TRACE_ARRAY_SIZE)
    {
      index -= TRACE_ARRAY_SIZE;
    }

  for (nprinted = 0; nprinted < st->nTracePoints; nprinted++)
    {
      printf("SP:%04" PRIx16 "  %04" PRIx16 "  ",
             st->traceArray[index].sp, st->traceArray[index].tos);

      /* Print the instruction executed at this traced address */

      (void)pdbg_PrintPCode(st, st->traceArray[index].pc, 1);

      if (st->nWatchPoints > 0)
        {
          printf("WP:%04" PRIx16 "  %04" PRIx16 "\n",
                 st->watchPoint[0], st->traceArray[index].wp);
        }

      /* Index to the next trace entry */

      if (++index >= TRACE_ARRAY_SIZE)
        {
          index = 0;
        }
    }
}

/***************************************************************************/
/* Add a breakpoint to the breakpoint array */

static void pdbg_AddBreakPoint(struct libexec_s *st, pasSize_t pc)
{
  int i;

  /* Is there room for another breakpoint? */

  if (st->nBreakPoints < MAX_BREAK_POINTS)
    {
      /* Yes..Check if the breakpoint already exists */

      for (i = 0; i < st->nBreakPoints; i++)
        {
          if (st->breakPoint[i] == pc)
            {
              /* It is already set.  Return without doing anything */

              return;
            }
        }

      /* The breakpoint is not already set -- set it */

      st->breakPoint[st->nBreakPoints++] = pc;
    }
}

/***************************************************************************/
/* Remove a breakpoint from the breakpoint array */

static void pdbg_DeleteBreakPoint(struct libexec_s *st, int16_t bpno)
{
  if ((bpno >= 1) && (bpno <= st->nBreakPoints))
    {
      for (; (bpno < st->nBreakPoints); bpno++)
        {
          st->breakPoint[bpno - 1] = st->breakPoint[bpno];
        }

      st->nBreakPoints--;

    }
}

/***************************************************************************/
/* Print the breakpoint array */

static void pdbg_PrintBreakPoints(struct libexec_s *st)
{
  int i;

  printf("BP:#  Address  P-Code\n");
  for (i = 0; i < st->nBreakPoints; i++)
    {
      printf("BP:%d  ", (i+1));
      (void)pdbg_PrintPCode(st, st->breakPoint[i], 1);
    }
}

/***************************************************************************/
/* Check if a breakpoint is set at the current value of program counter.
 * If so, print the instruction and stop execution. */

static void pdbg_CheckBreakPoint(struct libexec_s *st)
{
  uint16_t bpIndex;

  /* Check for a user breakpoint */

  for (bpIndex = 0;
       ((bpIndex < st->nBreakPoints) && (!st->bExecStop));
       bpIndex++)
    {
      if (st->breakPoint[bpIndex] == st->pc)
        {
          printf("Breakpoint #%d -- Execution Stopped\n", (bpIndex+1));
          st->bExecStop = true;
          return;
        }
    }
}

/***************************************************************************/

static void pdbg_AddWatchPoint(struct libexec_s *st, ustack_t addr)
{
  st->watchPoint[0] = addr;
  st->nWatchPoints  = 1;
}

/***************************************************************************/

static void pdbg_ClearWatchPoint(struct libexec_s *st, int16_t wpno)
{
  st->nWatchPoints  = 0;
}

/***************************************************************************/
/* Initialize Debugger variables */

static void pdbg_InitDebugger(struct libexec_s *st)
{
   st->lastCmd      = eCMD_NONE;
   st->bExecStop    = false;
   st->traceIndex   = 0;
   st->nTracePoints = 0;
}

/***************************************************************************/
/* This function executes the P-Code program until a stopping condition
 * is encountered. */

static void pdbg_DebugPCode(struct libexec_s *st)
{
  uint16_t errorCode;

  do
    {
      /* Trace the next instruction execution */

      st->traceArray[st->traceIndex].pc  = st->pc;
      st->traceArray[st->traceIndex].sp  = st->sp;

      if (st->nWatchPoints > 0)
        {
          int32_t isp = BTOISTACK(st->watchPoint[0]);
          st->traceArray[st->traceIndex].wp = st->dstack.i[isp];
        }
      else
        {
          st->traceArray[st->traceIndex].wp = 0;
        }

      if (st->sp < st->stackSize)
        {
          st->traceArray[st->traceIndex].tos = st->dstack.i[BTOISTACK(st->sp)];
        }
      else
        {
          st->traceArray[st->traceIndex].tos = 0;
        }

      if (++st->traceIndex >= TRACE_ARRAY_SIZE)
        {
           st->traceIndex = 0;
        }

      if (st->nTracePoints < TRACE_ARRAY_SIZE)
        {
          st->nTracePoints++;
        }

      /* Execute the instruction */

      errorCode = libexec_Execute(st);

      /* Check for exceptional stopping conditions */

      if (errorCode != eNOERROR)
        {
          if (errorCode == eEXIT)
            {
              printf("Normal Termination\n");
            }
          else
            {
              printf("Runtime error 0x%02x -- Execution Stopped\n",
                     errorCode);
            }

          st->bExecStop = true;
        }

      /* Check for normal stopping conditions */

      if (!st->bExecStop)
        {
          /* Check for attempt to execute code outside of legal range */

          if (st->pc >= st->maxpc)
            {
              st->bExecStop = true;
            }

          /* Check for a temporary breakpoint */

          else if ((st->untilPoint > 0) && (st->untilPoint == st->pc))
            {
              st->bExecStop = true;
            }

          /* Check if there is a breakpoint at the next instruction */

          else if (st->nBreakPoints > 0)
            {
              pdbg_CheckBreakPoint(st);
            }
        }
    }
  while (!st->bExecStop);
}

/**************************************************************************
 * Public Functions
 ***************************************************************************/

void libexec_DebugLoop(EXEC_HANDLE_t handle)
{
  struct libexec_s *st = (struct libexec_s *)handle;
  pasSize_t pc;
  int i;

  pdbg_ShowCommands();
  pdbg_InitDebugger(st);
  pdbg_ProgramStatus(st);

  while (true)
    {
      printf("CMD: ");
      fflush(stdout);
      (void)GETS(st->cmdLine, LINE_SIZE, stdin);
      switch (toupper(st->cmdLine[0]))
        {
        case 'R' :
          switch (toupper(st->cmdLine[1]))
            {
              case 'E' :  /* Reset */
                pdbg_ExecCommand(st, eCMD_RESET, 0);
                break;
              case 'U' :  /* Run */
                pdbg_ExecCommand(st, eCMD_RUN, 0);
                break;
              default :
                printf("Unrecognized Command\n");
                pdbg_ExecCommand(st, eCMD_HELP, 0);
                break;
            }
          break;

        case 'S' :  /* Single Step (into) */
          pdbg_ExecCommand(st, eCMD_STEP, 0);
          break;

        case 'N' :  /* Single Step (over) */
          pdbg_ExecCommand(st, eCMD_NEXT, 0);
          break;

        case 'G' :  /* Go */
          pdbg_ExecCommand(st, eCMD_GO, 0);
          break;

        case 'B' :
          switch (toupper(st->cmdLine[1]))
            {
              case 'S' :  /* Set Breakpoint */
                pc = pdbg_ReadHex(&st->cmdLine[2], st->pc);
                pdbg_ExecCommand(st, eCMD_BS, pc);
                break;

              case 'C' :  /* Clear Breakpoint */
                i =  pdbg_ReadDecimal(&st->cmdLine[2]);
                pdbg_ExecCommand(st, eCMD_BC, i);
                break;

              default :
                printf("Unrecognized Command\n");
                pdbg_ExecCommand(st, eCMD_HELP, 0);
                break;
            }
          break;

        case 'W' :
          switch (toupper(st->cmdLine[1]))
            {
              case 'S' :  /* Set Watchpoint */
                pc = pdbg_ReadHex(&st->cmdLine[2], st->pc);
                pdbg_ExecCommand(st, eCMD_WS, pc);
                break;

              case 'F' :  /* Set Level 0 Frame Watchpoint */
                pc = pdbg_ReadHex(&st->cmdLine[2], st->pc) + st->spb;
                pdbg_ExecCommand(st, eCMD_WS, pc);
                break;

              case 'C' :  /* Clear Watchpoint */
                pdbg_ExecCommand(st, eCMD_WC, 0);
                break;

              default :
                printf("Unrecognized Command\n");
                pdbg_ExecCommand(st, eCMD_HELP, 0);
                break;
            }
          break;

        case 'D' :
          switch (toupper(st->cmdLine[1]))
            {
              case 'P' :  /* Display Program Status */
                pdbg_ExecCommand(st, eCMD_DP, 0);
                break;

              case 'T' :  /* Display Program Trace */
                pdbg_ExecCommand(st, eCMD_DT, 0);
                break;

              case 'S' :  /* Display Stack */
                pc = pdbg_ReadHex(&st->cmdLine[2], st->sp);
                pdbg_ExecCommand(st, eCMD_DS, pc);
                break;

              case 'I' :  /* Display Instructions */
                pc = pdbg_ReadHex(&st->cmdLine[2], st->pc);
                pdbg_ExecCommand(st, eCMD_DI, pc);
                break;

              case 'B' :  /* Display Breakpoints */
                pdbg_ExecCommand(st, eCMD_DB, 0);
                break;

              default :
                printf("Unrecognized Command\n");
                pdbg_ExecCommand(st, eCMD_HELP, 0);
                break;
            }
          break;

        case 'Q' :  /* Quit */
          pdbg_ExecCommand(st, eCMD_QUIT, 0);
          break;

        case 'H' :  /* Help */
        case '?' :
          pdbg_ExecCommand(st, eCMD_HELP, 0);
          break;

        case '\0' : /* Repeat last command */
        case '\n' : /* Repeat last command */
          pdbg_ExecCommand(st, st->lastCmd, st->lastValue);
          break;

        default :
          printf("Unrecognized Command\n");
          pdbg_ExecCommand(st, eCMD_HELP, 0);
          break;
        }
    }
}
