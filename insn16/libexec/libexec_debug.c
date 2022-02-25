/**********************************************************************
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
 **********************************************************************/

/**********************************************************************
 * Included Files
 **********************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <getopt.h>
#include <ctype.h>

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

/**********************************************************************
 * Pre-processor Definitions
 **********************************************************************/

#define TRACE_ARRAY_SIZE       16
#define MAX_BREAK_POINTS        8
#define MAX_WATCH_POINTS        1
#define DISPLAY_STACK_SIZE     16
#define DISPLAY_INST_SIZE      16

/**********************************************************************
 * Private Type Definitions
 **********************************************************************/

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
  pasSize_t  pc;
  pasSize_t  sp;
  ustack_t tos;
  ustack_t wp;
};

typedef struct trace_s trace_t;

/**********************************************************************
 * Private Data
 **********************************************************************/

static enum command_e g_lastcmd = eCMD_NONE;
static uint32_t         g_lastvalue;

/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

static void      pdbg_showcommands(void);
static void      pdbg_execcommand(struct libexec_s *st,
                   enum command_e cmd, uint32_t value);
static int32_t   pdbg_readdecimal(char *ptr);
static int32_t   pdbg_readhex(char *ptr, int32_t defaultvalue);
static void      pdbg_programstatus(struct libexec_s *st);
static pasSize_t pdbg_printpcode(struct libexec_s *st, pasSize_t pc,
                   int16_t nitems);
static pasSize_t pdbg_printstack(struct libexec_s *st, pasSize_t sp,
                   int16_t nitems);
static void      pdbg_printregisters(struct libexec_s *st);
static void      pdbg_printwatchpoint(struct libexec_s *st);
static void      pdbg_printtracearray(struct libexec_s *st);
static void      pdbg_addbreakpoint(pasSize_t pc);
static void      pdbg_deletebreakpoint(int16_t bpno);
static void      pdbg_printbreakpoints(struct libexec_s *st);
static void      pdbg_checkbreakpoint(struct libexec_s *st);
static void      pdbg_addwatchpoint(ustack_t addr);
static void      pdbg_clearwatchpoint(int16_t wpno);
static void      pdbg_initdebugger(void);
static void      pdbg_debugpcode(struct libexec_s *st);

/**********************************************************************
 * Private Data
 **********************************************************************/

/* Debugging variables */

static trace_t  g_tracearray[TRACE_ARRAY_SIZE];
                        /* Holds execution histor */
static uint16_t g_tracendx;
                        /* This is the index into the circular g_tracearray */
static uint16_t g_ntracepoints;
                        /* This is the number of valid enties in g_tracearray */
static pasSize_t  g_breakpoint[MAX_BREAK_POINTS];
                        /* Contains address associated with all active */
                        /* break points. */
static pasSize_t  g_watchpoint[MAX_WATCH_POINTS];
                        /* Contains address associated with all active */
                        /* watch points. */
static pasSize_t  g_untilpoint;
                        /* The 'g_untilpoint' is a temporary breakpoint */
static uint16_t g_nbreakpoints;
                        /* Number of items in g_breakPoints[] */
static uint16_t g_nwatchpoints;
                        /* Number of items in g_watchpoint[] */
static pasSize_t  g_displayloc;
                        /* P-code display location display */
static bool     g_bstopexecution;
                        /* true means to stop program execution */

/* I/O variables */

static char     g_inline[LINE_SIZE+1];
                        /* Command line buffer */

/**********************************************************************
 * Private Functions
 **********************************************************************/
/* Show command characters */

static void pdbg_showcommands(void)
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

/***********************************************************************/
static void pdbg_execcommand(struct libexec_s *st, enum command_e cmd, uint32_t value)
{
  /* Save the command to resuse if the user enters nothing */

  g_lastcmd   = cmd;
  g_lastvalue = value;

  switch (cmd)
    {
    case eCMD_NONE:   /* Do nothing */
      break;
    case eCMD_RESET:  /* Reset */
      libexec_Reset(st);
      pdbg_initdebugger();
      pdbg_programstatus(st);
      g_lastcmd = eCMD_NONE;
      break;
    case eCMD_RUN:    /* Run */
      libexec_Reset(st);
      pdbg_initdebugger();
      pdbg_debugpcode(st);
      pdbg_programstatus(st);
      break;
    case eCMD_STEP:   /* Single Step (into)*/
      g_bstopexecution = true;
      pdbg_debugpcode(st);
      pdbg_programstatus(st);
      break;
    case eCMD_NEXT:   /* Single Step (over) */
      if (st->ispace[st->pc] == oPCAL)
        {
          g_bstopexecution = false;
          g_untilpoint = st->pc + 4;
        }
      else
        {
          g_bstopexecution = true;
        }
      pdbg_debugpcode(st);
      g_untilpoint = 0;
      pdbg_programstatus(st);
      break;
    case eCMD_GO:     /* Go */
      g_bstopexecution = false;
      pdbg_debugpcode(st);
      pdbg_programstatus(st);
      break;
    case eCMD_BS:     /* Set Breakpoint */
      if (g_nbreakpoints >= MAX_BREAK_POINTS)
        {
          printf("Too many breakpoints\n");
          g_lastcmd = eCMD_NONE;
        }
      else if (value >= st->maxpc)
        {
          printf("Invalid address for breakpoint\n");
          g_lastcmd = eCMD_NONE;
        }
      else
        {
          pdbg_addbreakpoint(value);
          pdbg_printbreakpoints(st);
        }
      break;
    case eCMD_BC:     /* Clear Breakpoint */
      if (value >= 1 && value <= g_nbreakpoints)
        {
          pdbg_deletebreakpoint(value);
        }
      else
        {
          printf("Invalid breakpoint number\n");
          g_lastcmd = eCMD_NONE;
        }
      pdbg_printbreakpoints(st);
      break;
    case eCMD_WS:     /* Set Breakpoint */
      if (g_nwatchpoints >= MAX_WATCH_POINTS)
        {
          printf("Too many watchpoints\n");
          g_lastcmd = eCMD_NONE;
        }
      else if (value >= st->stacksize)
        {
          printf("Invalid address for watchpoint\n");
          g_lastcmd = eCMD_NONE;
        }
      else
        {
          pdbg_addwatchpoint(value);
        }
      break;
    case eCMD_WC:     /* Clear Watchpoint */
      pdbg_clearwatchpoint(value);
      break;
    case eCMD_DP:     /* Display Program Status */
      pdbg_programstatus(st);
      break;
    case eCMD_DT:     /* Display Program Trace */
      pdbg_printtracearray(st);
      break;
    case eCMD_DS:     /* Display Stack */
      if (value > st->sp)
        {
          printf("Invalid stack address\n");
          g_lastcmd = eCMD_NONE;
        }
      else
        {
          g_lastvalue = pdbg_printstack(st, value, DISPLAY_STACK_SIZE);
        }
      break;
    case eCMD_DI:     /* Display Instructions */
      if (value >= st->maxpc)
        {
          printf("Invalid instruction address\n");
          g_lastcmd = eCMD_NONE;
        }
      else
        {
          g_lastvalue = pdbg_printpcode(st, value, DISPLAY_INST_SIZE);
        }
      break;
    case eCMD_DB:     /* Display Breakpoints */
      pdbg_printbreakpoints(st);
      break;
    case eCMD_QUIT:   /* Quit */
      printf("Goodbye\n");
      exit(0);
      break;
    case eCMD_HELP:   /* Help */
    default:          /* Internal error */
      pdbg_showcommands();
      g_lastcmd = eCMD_NONE;
      break;
    }
}

/***********************************************************************/
/* Read a decimal value from the  input string */

static int32_t pdbg_readdecimal(char *ptr)
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

/***********************************************************************/
/* Read a hexadecimal value from the  input string */

static int32_t pdbg_readhex(char *ptr, int32_t defaultvalue)
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

/***********************************************************************/
/* Print the disassembled P-Code at PC */

static void pdbg_programstatus(struct libexec_s *st)
{
  (void)pdbg_printpcode(st, st->pc, 1);
  (void)pdbg_printstack(st, st->sp, 2);
  pdbg_printregisters(st);
  pdbg_printwatchpoint(st);
}

/***********************************************************************/
/* Print the disassembled P-Code at PC */

static pasSize_t pdbg_printpcode(struct libexec_s *st, pasSize_t pc, int16_t nitems)
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

/***********************************************************************/
/* Print the stack value at SP */

static pasSize_t pdbg_printstack(struct libexec_s *st, pasSize_t sp, int16_t nitems)
{
  int32_t isp;

  if ((st->sp < st->stacksize) && (sp <= st->sp))
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

/***********************************************************************/
/* Print the base register */

static void pdbg_printregisters(struct libexec_s *st)
{
  if (st->fp <= st->sp)
    {
      printf("FP:%04" PRIx16 " ", st->fp);
    }

  printf("CSP:%04" PRIx16 "\n", st->csp);
}

/***********************************************************************/
/* Print the watchpoint SP */

static void pdbg_printwatchpoint(struct libexec_s *st)
{
  if (g_nwatchpoints > 0)
    {
      int32_t isp = BTOISTACK(g_watchpoint[0]);
      printf("WP:%04" PRIx16 "  %04" PRIx16 "\n",
             g_watchpoint[0], st->dstack.i[isp]);
    }
}

/***********************************************************************/
/* Print the g_tracearray */

static void pdbg_printtracearray(struct libexec_s *st)
{
  int nprinted;
  int index;

  index = g_tracendx + TRACE_ARRAY_SIZE - g_ntracepoints;
  if (index >= TRACE_ARRAY_SIZE)
    {
      index -= TRACE_ARRAY_SIZE;
    }

  for (nprinted = 0; nprinted < g_ntracepoints; nprinted++)
    {
      printf("SP:%04" PRIx16 "  %04" PRIx16 "  ",
             g_tracearray[index].sp, g_tracearray[index].tos);

      /* Print the instruction executed at this traced address */

      (void)pdbg_printpcode(st, g_tracearray[index].pc, 1);

      if (g_nwatchpoints > 0)
        {
          printf("WP:%04" PRIx16 "  %04" PRIx16 "\n",
                 g_watchpoint[0], g_tracearray[index].wp);
        }

      /* Index to the next trace entry */

      if (++index >= TRACE_ARRAY_SIZE)
        {
          index = 0;
        }
    }
}

/***********************************************************************/
/* Add a breakpoint to the breakpoint array */

static void pdbg_addbreakpoint(pasSize_t pc)
{
  int i;

  /* Is there room for another breakpoint? */

  if (g_nbreakpoints < MAX_BREAK_POINTS)
    {
      /* Yes..Check if the breakpoint already exists */

      for (i = 0; i < g_nbreakpoints; i++)
        {
          if (g_breakpoint[i] == pc)
            {
              /* It is already set.  Return without doing anything */

              return;
            }
        }

      /* The breakpoint is not already set -- set it */

      g_breakpoint[g_nbreakpoints++] = pc;
    }
}

/***********************************************************************/
/* Remove a breakpoint from the breakpoint array */

static void pdbg_deletebreakpoint(int16_t bpno)
{
  if ((bpno >= 1) && (bpno <= g_nbreakpoints))
    {
      for (; (bpno < g_nbreakpoints); bpno++)
        {
          g_breakpoint[bpno-1] = g_breakpoint[bpno];
        }

      g_nbreakpoints--;

    }
}

/***********************************************************************/
/* Print the breakpoint array */

static void pdbg_printbreakpoints(struct libexec_s *st)
{
  int i;

  printf("BP:#  Address  P-Code\n");
  for (i = 0; i < g_nbreakpoints; i++)
    {
      printf("BP:%d  ", (i+1));
      (void)pdbg_printpcode(st, g_breakpoint[i], 1);
    }
}

/***********************************************************************/
/* Check if a breakpoint is set at the current value of program counter.
 * If so, print the instruction and stop execution. */

static void pdbg_checkbreakpoint(struct libexec_s *st)
{
  uint16_t bpIndex;

  /* Check for a user breakpoint */

  for (bpIndex = 0;
       ((bpIndex < g_nbreakpoints) && (!g_bstopexecution));
       bpIndex++)
    {
      if (g_breakpoint[bpIndex] == st->pc)
        {
          printf("Breakpoint #%d -- Execution Stopped\n", (bpIndex+1));
          g_bstopexecution = true;
          return;
        }
    }
}

/***********************************************************************/

static void pdbg_addwatchpoint(ustack_t addr)
{
  g_watchpoint[0] = addr;
  g_nwatchpoints  = 1;
}

/***********************************************************************/

static void pdbg_clearwatchpoint(int16_t wpno)
{
  g_nwatchpoints  = 0;
}

/***********************************************************************/
/* Initialize Debugger variables */

static void pdbg_initdebugger(void)
{
   g_bstopexecution = false;
   g_displayloc     = 0;
   g_tracendx       = 0;
   g_ntracepoints   = 0;
}

/***********************************************************************/
/* This function executes the P-Code program until a stopping condition
 * is encountered. */

static void pdbg_debugpcode(struct libexec_s *st)
{
  uint16_t errorCode;

  do
    {
      /* Trace the next instruction execution */

      g_tracearray[g_tracendx].pc  = st->pc;
      g_tracearray[g_tracendx].sp  = st->sp;

      if (g_nwatchpoints > 0)
        {
          int32_t isp = BTOISTACK(g_watchpoint[0]);
          g_tracearray[g_tracendx].wp = st->dstack.i[isp];
        }
      else
        {
          g_tracearray[g_tracendx].wp = 0;
        }

      if (st->sp < st->stacksize)
        {
          g_tracearray[g_tracendx].tos = st->dstack.i[BTOISTACK(st->sp)];
        }
      else
        {
          g_tracearray[g_tracendx].tos = 0;
        }

      if (++g_tracendx >= TRACE_ARRAY_SIZE)
        {
           g_tracendx = 0;
        }

      if (g_ntracepoints < TRACE_ARRAY_SIZE)
        {
          g_ntracepoints++;
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

          g_bstopexecution = true;
        }

      /* Check for normal stopping conditions */

      if (!g_bstopexecution)
        {
          /* Check for attempt to execute code outside of legal range */

          if (st->pc >= st->maxpc)
            {
              g_bstopexecution = true;
            }

          /* Check for a temporary breakpoint */

          else if ((g_untilpoint > 0) && (g_untilpoint == st->pc))
            {
              g_bstopexecution = true;
            }

          /* Check if there is a breakpoint at the next instruction */

          else if (g_nbreakpoints > 0)
            {
              pdbg_checkbreakpoint(st);
            }
        }
    }
  while (!g_bstopexecution);
}

/**********************************************************************
 * Public Functions
 **********************************************************************/

void libexec_DebugLoop(EXEC_HANDLE_t handle)
{
  struct libexec_s *st = (struct libexec_s *)handle;
  pasSize_t pc;
  int i;

  pdbg_showcommands();
  pdbg_initdebugger();
  pdbg_programstatus(st);

  while (true)
    {
      printf("CMD: ");
      (void) fgets(g_inline, LINE_SIZE, stdin);
      switch (toupper(g_inline[0]))
        {
        case 'R' :
          switch (toupper(g_inline[1]))
            {
              case 'E' :  /* Reset */
                pdbg_execcommand(st, eCMD_RESET, 0);
                break;
              case 'U' :  /* Run */
                pdbg_execcommand(st, eCMD_RUN, 0);
                break;
              default :
                printf("Unrecognized Command\n");
                pdbg_execcommand(st, eCMD_HELP, 0);
                break;
            }
          break;

        case 'S' :  /* Single Step (into) */
          pdbg_execcommand(st, eCMD_STEP, 0);
          break;

        case 'N' :  /* Single Step (over) */
          pdbg_execcommand(st, eCMD_NEXT, 0);
          break;

        case 'G' :  /* Go */
          pdbg_execcommand(st, eCMD_GO, 0);
          break;

        case 'B' :
          switch (toupper(g_inline[1]))
            {
              case 'S' :  /* Set Breakpoint */
                pc = pdbg_readhex(&g_inline[2], st->pc);
                pdbg_execcommand(st, eCMD_BS, pc);
                break;

              case 'C' :  /* Clear Breakpoint */
                i =  pdbg_readdecimal(&g_inline[2]);
                pdbg_execcommand(st, eCMD_BC, i);
                break;

              default :
                printf("Unrecognized Command\n");
                pdbg_execcommand(st, eCMD_HELP, 0);
                break;
            }
          break;

        case 'W' :
          switch (toupper(g_inline[1]))
            {
              case 'S' :  /* Set Watchpoint */
                pc = pdbg_readhex(&g_inline[2], st->pc);
                pdbg_execcommand(st, eCMD_WS, pc);
                break;

              case 'F' :  /* Set Level 0 Frame Watchpoint */
                pc = pdbg_readhex(&g_inline[2], st->pc) + st->spb;
                pdbg_execcommand(st, eCMD_WS, pc);
                break;

              case 'C' :  /* Clear Watchpoint */
                pdbg_execcommand(st, eCMD_WC, 0);
                break;

              default :
                printf("Unrecognized Command\n");
                pdbg_execcommand(st, eCMD_HELP, 0);
                break;
            }
          break;

        case 'D' :
          switch (toupper(g_inline[1]))
            {
              case 'P' :  /* Display Program Status */
                pdbg_execcommand(st, eCMD_DP, 0);
                break;

              case 'T' :  /* Display Program Trace */
                pdbg_execcommand(st, eCMD_DT, 0);
                break;

              case 'S' :  /* Display Stack */
                pc = pdbg_readhex(&g_inline[2], st->sp);
                pdbg_execcommand(st, eCMD_DS, pc);
                break;

              case 'I' :  /* Display Instructions */
                pc = pdbg_readhex(&g_inline[2], st->pc);
                pdbg_execcommand(st, eCMD_DI, pc);
                break;

              case 'B' :  /* Display Breakpoints */
                pdbg_execcommand(st, eCMD_DB, pc);
                break;

              default :
                printf("Unrecognized Command\n");
                pdbg_execcommand(st, eCMD_HELP, 0);
                break;
            }
          break;

        case 'Q' :  /* Quit */
          pdbg_execcommand(st, eCMD_QUIT, pc);
          break;

        case 'H' :  /* Help */
        case '?' :
          pdbg_execcommand(st, eCMD_HELP, 0);
          break;

        case '\0' : /* Repeat last command */
        case '\n' : /* Repeat last command */
          pdbg_execcommand(st, g_lastcmd, g_lastvalue);
          break;

        default :
          printf("Unrecognized Command\n");
          pdbg_execcommand(st, eCMD_HELP, 0);
          break;
        }
    }
}