/**********************************************************************
 * plink.c
 * P-Code Linker
 *
 *   Copyright (C) 2008-2009, 2022 Gregory Nutt. All rights reserved.
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
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "pas_debug.h"
#include "pas_machine.h"
#include "pas_pcode.h"
#include "pas_errcodes.h"

#include "paslib.h"
#include "pas_error.h"
#include "plsym.h"
#include "plreloc.h"
#include "pas_insn.h"
#include "plink.h"

/**********************************************************************
 * Pre-processor Definitions
 **********************************************************************/

#define MAX_POFF_FILES  8

/**********************************************************************
 * Private Data
 **********************************************************************/

static const char *g_outFileName;
static const char *g_inFileName[MAX_POFF_FILES];
static int         g_nPoffFiles = 0;

/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

static void     plink_ShowUsage        (const char *progname);
static void     plink_ParseArguments   (int argc, char **argv);
static void     plink_LoadInputFiles   (poffHandle_t outHandle);
static void     plink_CheckFileHeader  (poffHandle_t inHandle, poffHandle_t outHandle,
                                        uint32_t pcOffset, bool *progFound);
static uint32_t plink_MergeRoData      (poffHandle_t inHandle, poffHandle_t outHandle);
static uint32_t plink_MergeProgramData (poffHandle_t inHandle, poffHandle_t outHandle,
                                        uint32_t pcOffset, uint32_t roOffset);
static uint32_t plink_MergeFileNames   (poffHandle_t inHandle, poffHandle_t outHandle);
static uint32_t plink_MergeLineNumbers (poffHandle_t inHandle, poffHandle_t outHandle,
                                        uint32_t pcOffset, uint32_t fnOffset);
static void     plink_WirteOutputFile  (poffHandle_t outHandle);

/**********************************************************************
 * Private Functions
 **********************************************************************/

static void plink_ShowUsage(const char *progname)
{
  fprintf(stderr, "USAGE:\n");
  fprintf(stderr, "  %s <in-file-name> [<in-file-name>] <out-file-name>\n",
          progname);
}

/***********************************************************************/

static void plink_ParseArguments(int argc, char **argv)
{
  int i;

  /* Check for existence of filename argument */

  if (argc < 3)
    {
      fprintf(stderr,
              "ERROR: <in-file-name> and one <out-file-name> required\n");
      plink_ShowUsage(argv[0]);
    }

  /* Get the name of the p-code file(s) from the last argument(s) */

  for (i = 1; i < argc-1; i++)
    {
      g_inFileName[g_nPoffFiles] = argv[i];
      g_nPoffFiles++;
    }

  /* The last thing on the command line is the output file name */

  g_outFileName = argv[argc-1];
}

/***********************************************************************/
/* This function loads each POFF file specified on the command line,
 * merges the input POFF data, and generates intermediate structures
 * to be used in the final link.
 */

static void plink_LoadInputFiles(poffHandle_t outHandle)
{
  poffHandle_t inHandle;
  FILE        *instream;
  char         fileName[PATH_MAX];  /* Object file path */
  uint32_t     pcOffset = 0;
  uint32_t     fnOffset = 0;
  uint32_t     symOffset = 0;
  uint32_t     roOffset = 0;
  uint32_t     pcEnd = 0;
  uint32_t     fnEnd = 0;
  uint32_t     symEnd = 0;
  uint16_t     errCode;
  bool         progFound = false;
  int          i;

  /* Load the POFF files specified on the command line */

  for (i = 0; i < g_nPoffFiles; i++)
    {
      /* Create a handle to hold the input file data */

      inHandle = poffCreateHandle();
      if (inHandle == NULL) fatal(eNOMEMORY);

      /* Use .o or command line extension, if supplied, to get the
       * input file name.
       */

      (void)extension(g_inFileName[i], "o", fileName, PATH_MAX, 0);

      /* Open the input file */

      instream = fopen(fileName, "rb");
      if (instream == NULL)
        {
          fprintf(stderr, "ERROR: Could not open %s: %s\n",
                  fileName, strerror(errno));
          exit(1);
        }

      /* Load the POFF file */

      errCode = poffReadFile(inHandle, instream);
      if (errCode != eNOERROR)
        {
          fprintf(stderr, "ERROR: Could not read %s (%d)\n",
                  fileName, errCode);
          exit(1);
        }

      /* Check file header for critical settings */

      plink_CheckFileHeader(inHandle, outHandle, pcOffset, &progFound);

      /* Merge the read-only data sections */

      roOffset = plink_MergeRoData(inHandle, outHandle);

      /* Merge program section data from the new input file into the
       * output file container.
       */

      pcEnd = plink_MergeProgramData(inHandle, outHandle, pcOffset,
                                     roOffset);

      /* Merge the file name data from the new input file into the
       * output file container.
       */

      fnEnd = plink_MergeFileNames(inHandle, outHandle);

      /* Merge the line number data from the new input file into the
       * output file container.
       */

      (void)plink_MergeLineNumbers(inHandle, outHandle, pcOffset, fnOffset);

      /* On this pass, we just want to collect all symbol table in a
       * local list where we can resolve all undefined symbols (later)
       */

      symEnd = mergeSymbols(inHandle, pcOffset, symOffset);

      /* On this pass, we will also want to buffer all relocation data,
       * adjusting only the program section offset and sym table
       * offsets.
       */

      mergeRelocations(inHandle, pcOffset, symOffset);

      /* Release the input file data */

      insn_ResetOpCodeRead(inHandle);
      poffDestroyHandle(inHandle);

      /* Close the input file */

      fclose(instream);

      /* Set the offsest to be used for the next file equal
       * to the end values found from processing this file
       */

      pcOffset  = pcEnd;
      fnOffset  = fnEnd;
      symOffset = symEnd;
    }

  /* Did we find exactly one program file? */

  if (!progFound)
    {
      /* No! We have to have a program file to generate an executable */

      fprintf(stderr, "ERROR: No program file found in input files\n");
      exit(1);
    }
}

/***********************************************************************/

static void plink_CheckFileHeader(poffHandle_t inHandle,
                                  poffHandle_t outHandle,
                                  uint32_t pcOffset, bool *progFound)
{
  uint8_t fileType;

  /* What kind of file are we processing? */

  fileType = poffGetFileType(inHandle);
  if (fileType == FHT_PROGRAM)
    {
      /* We can handle only one pascal program file */

      if (*progFound)
        {
          fprintf(stderr,
                  "ERROR: Only one compiled pascal program file "
                  "may appear in input file list\n");
          exit(1);
        }
      else
        {
          /* Get the entry point from the pascal file, apply any
           * necessary offsets, and store the entry point in the
           * linked output file's file header.
           */

          poffSetEntryPoint(outHandle,
                            poffGetEntryPoint(inHandle) + pcOffset);

          /* Copy the program name from the pascal file to the linked
           * output file's file header and mark the output file as
           * a pascal executable.
           */

          poffSetFileType(outHandle, FHT_EXEC, 0,
                          poffGetFileHdrName(inHandle));

          /* Indicate that we have found the program file */

          *progFound = true;
        }
    }
  else if (fileType != FHT_UNIT)
    {
      /* It is something other than a compiled pascal program or unit
       * file.
       */

      fprintf(stderr,
              "ERROR: Only compiled pascal program and unit files "
              "may appear in input file list\n");
      exit(1);
    }
}

/***********************************************************************/

static uint32_t plink_MergeRoData(poffHandle_t inHandle,
                                  poffHandle_t outHandle)
{
  uint8_t *newRoData;
  uint32_t oldRoDataSize;
  uint32_t newRoDataSize;

  /* Get the size of the read-only data section before we add the
   * new data.  This is the offset that must be applied to any
   * references to the new data.
   */

  oldRoDataSize = poffGetRoDataSize(outHandle);

  /* Remove the read-only data from new input file */

  newRoDataSize = poffExtractRoData(inHandle, &newRoData);

  /* And append the new read-only data to output file */

  poffAppendRoData(outHandle, newRoData, newRoDataSize);

  return oldRoDataSize;
}

/***********************************************************************/
/* This function merges the program data section of a new file into the
 * program data section of the output file, relocating simple program
 * section references as they are encountered.
 */

static uint32_t plink_MergeProgramData(poffHandle_t inHandle,
                                       poffHandle_t outHandle,
                                       uint32_t pcOffset, uint32_t roOffset)
{
  opType_t op;
  uint32_t pc;
  uint32_t opSize;
  int endOp;

  /* Read each opcode from the input file, add pcOffset to each program
   * section address, and add each opcode to the output file.
   */

  pc = pcOffset;
  do
    {
      /* Read the next opcode (with its size) */

      opSize = insn_GetOpCode(inHandle, &op);

      /* Perform any necessary relocations */

      endOp = insn_Relocate(&op, pcOffset, roOffset);

      /* Save the potentially modified opcode in the temporary
       * program data container.
       */

      insn_AddOpCode(outHandle, &op);
      pc += opSize;
    }
  while (endOp == 0);

  return pc;
}

/***********************************************************************/
/* This function merges the file name section of a new file into the
 * file name section of the output file, relocating simple program
 * section references as they are encountered.
 */

static uint32_t plink_MergeFileNames(poffHandle_t inHandle,
                                     poffHandle_t outHandle)
{
  int32_t inOffset;
  uint32_t outOffset;
  const char *fname;

  do
    {
      /* Read each file name from the input File */

      inOffset = poffGetFileName(inHandle, &fname);
      if (inOffset >= 0)
        {
          /* And write it to the output file */

          outOffset = poffAddFileName(outHandle, fname);
        }
    }
  while (inOffset >= 0);

  /* Return the offset to the last file name written to the
   * output file
   */

  return outOffset + 1;
}

/***********************************************************************/
/* This function merges the line number section of a new file into the
 * line number section of the output file, relocating simple program
 * section references as they are encountered.
 */

static uint32_t plink_MergeLineNumbers(poffHandle_t inHandle,
                                       poffHandle_t outHandle,
                                       uint32_t pcOffset,
                                       uint32_t fnOffset)
{
  poffLineNumber_t lineno;
  int32_t inOffset;
  uint32_t outOffset;

  do
    {
      /* Read each line number from the input File */

      inOffset = poffGetRawLineNumber(inHandle, &lineno);
      if (inOffset >= 0)
        {
          /* And write it to the output file */

          outOffset = poffAddLineNumber(outHandle, lineno.ln_lineno,
                                        lineno.ln_fileno + fnOffset,
                                        lineno.ln_poffset + pcOffset);
        }
    }
  while (inOffset >= 0);

  /* Return the offset to the last line number written to the
   * output file
   */

  return outOffset;
}

/***********************************************************************/

static void plink_WirteOutputFile(poffHandle_t outHandle)
{
  FILE *outstream;
  char  fileName[PATH_MAX];  /* Output file name */

  /* Use .pex or command line extension, if supplied, to get the
   * input file name.
   */

  (void)extension(g_outFileName, "pex", fileName, PATH_MAX, 0);

  /* Open the output file */

  outstream = fopen(fileName, "wb");
  if (outstream == NULL)
    {
      fprintf(stderr, "ERROR: Could not open %s: %s\n",
              fileName, strerror(errno));
      exit(1);
    }

  /* Write the POFF file */

  (void)poffWriteFile(outHandle, outstream);

  /* Close the output file */

  fclose(outstream);
}
/**********************************************************************
 * Public Functions
 **********************************************************************/

int main(int argc, char *argv[])
{
  poffHandle_t outHandle;

  /* Parse the command line arguments */

  plink_ParseArguments(argc, argv);

  /* Create a handle to hold the output file data */

  outHandle = poffCreateHandle();
  if (outHandle == NULL) fatal(eNOMEMORY);

  /* Load the POFF files specified on the command line */

  plink_LoadInputFiles(outHandle);

  /* Verify that all symbols were processed correctly */

  verifySymbols();

  /* Apply the relocation data to the program data */

  applyRelocations(outHandle);

  /* Write the symbol table information to the output file */

  writeSymbols(outHandle);

  /* Write the output file */

  plink_WirteOutputFile(outHandle);

  /* Release bufferred symbol/relocation informtion */

  releaseSymbols();
  releaseRelocations();

  /* Release the input file data */

  poffDestroyHandle(outHandle);

  return 0;
}
