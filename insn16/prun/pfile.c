/****************************************************************************
 * pfile.c
 *
 *   Copyright (C) 2021 Gregory Nutt. All rights reserved.
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
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "pas_debug.h"
#include "config.h"
#include "pas_machine.h"
#include "pas_sysio.h"
#include "pas_errcodes.h"
#include "pas_error.h"

#include "pfile.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_FILE_NAME       64

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pexecFileTable_s
{
  char fileName[MAX_FILE_NAME];
  FILE *stream;
  openMode_t openMode;
  uint16_t recordSize;
  int16_t eof;
  bool text;
};

typedef struct pexecFileTable_s pexecFileTable_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ustack_t pexec_ConvertInteger(uint16_t fileNumber, uint8_t *ioPtr);
static void     pexec_ConvertReal(uint16_t *dest, uint8_t *ioPtr);
static void     pexec_AssignFile(uint16_t fileNumber, bool text,
                                 const char *fileName);
static void     pexec_OpenFile(uint16_t fileNumber, openMode_t openMode);
static void     pexec_CloseFile(uint16_t fileNumber);
static void     pexec_RecordSize(uint16_t fileNumber, uint16_t size);
static void     pexec_ReadBinary(uint16_t fileNumber, uint8_t *dest,
                                 uint16_t size);
static void     pexec_ReadInteger(uint16_t fileNumber, ustack_t *dest);
static void     pexec_ReadChar(uint16_t fileNumber, uint8_t *dest);
static void     pexec_ReadString(uint16_t fileNumber, char *strPtr,
                                 uint16_t);
static void     pexec_ReadReal(uint16_t fileNumber, uint16_t *dest);
static void     pexec_WriteBinary(uint16_t fileNumber, const uint8_t *src,
                                 uint16_t size);
static void     pexec_WriteInteger(uint16_t fileNumber, int16_t value);
static void     pexec_WriteChar(uint16_t fileNumber, uint8_t value);
static void     pexec_WriteReal(uint16_t fileNumber, double value);
static void     pexec_WriteString(uint16_t fileNumber,
                                  const char *string,
                                  uint16_t size);
static uint16_t pexec_Eof(uint16_t fileNumber);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The runtime file table.  Maps a fileNumber to the current state of the
 * file.
 */

static pexecFileTable_t g_fileTable[MAX_OPEN_FILES];

/* Common error message formats */

static const char *g_badFileNumber   = "ERROR: %s: bad file number: %u\n";
static const char *g_fileAlreadyOpen = "ERROR: %s: File already open: %u\n";
static const char *g_fileNotOpen     = "ERROR: %s: File not open: %u\n";
static const char *g_badOpenMode     = "ERROR: %s: Bad open mode %d: %u\n";
static const char *g_openFailed      = "ERROR: %s: Bad open mode,\"%s\": %u\n";
static const char *g_integerOverFlow = "ERROR: %s: Integer overflow: %u\n";
static const char *g_notOpenForRead  = "ERROR: %s: Not open for reading: %u\n";
static const char *g_readFailed      = "ERROR: %s: Read failed, \"%s\": %u\n";
static const char *g_notOpenForWrite = "ERROR: %s: Not open for writing: %u\n";
static const char *g_writeFailed     = "ERROR: %s: Write failed, \"%s\": %u\n";

/* Working buffer */

static uint8_t g_ioLine[LINE_SIZE + 1];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pexec_ConvertInteger
 *
 * Description:
 *   This function parses a decimal integer from ioPtr
 ****************************************************************************/

static ustack_t pexec_ConvertInteger(uint16_t fileNumber, uint8_t *ioPtr)
{
  unsigned int value = 0;

  while (isspace(*ioPtr)) ioPtr++;
  while ((*ioPtr >= '0') && (*ioPtr <= '9'))
    {
      value = 10 * value
        + (sstack_t)(*ioPtr)
        - (sstack_t)'0';
      ioPtr++;

      if (value > UINT16_MAX)
        {
          fprintf(stderr, g_integerOverFlow, "pexec_ConvertInteger",
                  fileNumber);
          break;
        }
    }

  return (ustack_t)value;
}

/****************************************************************************
 * Name: pexec_ConvertReal
 *
 * Description:
 *    This function parses a decimal integer from ioPtr.
 *
 ****************************************************************************/

static void pexec_ConvertReal(uint16_t *dest, uint8_t *inPtr)
{
  int32_t  intpart;
  fparg_t  result;
  double   fraction;
  uint8_t  unaryop;

  intpart = 0;
  unaryop = '+';

  /* Check for a leading unary - */

  if (*inPtr == '-' || *inPtr == '+')
    {
      unaryop = *inPtr++;
    }

  /* Get the integer part of the real */

  while (*inPtr >= '0' && *inPtr <= '9')
    {
      intpart = 10*intpart + ((int32_t)*inPtr++) - ((int32_t)'0');
    }

  result.f = ((double)intpart);

  /* Check for the a fractional part */

  if (*inPtr == '.')
    {
      inPtr++;
      fraction = 0.1;
      while (*inPtr >= '0' && *inPtr <= '9')
        {
          result.f += fraction * (double)(((int32_t)*inPtr++) - ((int32_t)'0'));
          fraction /= 10.0;
        }
    }

  /* Correct the sign of the result */

  if (unaryop == '-')
    {
      result.f = -result.f;
    }

  /* Return the value into the P-Machine stack */

  *dest++ = result.hw[0];
  *dest++ = result.hw[1];
  *dest++ = result.hw[2];
  *dest   = result.hw[3];
}

static void pexec_AssignFile(uint16_t fileNumber, bool text, const char *fileName)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_AssignFile", fileNumber);
    }
  else
    {
      strncpy(g_fileTable[fileNumber].fileName, fileName, MAX_FILE_NAME);
      g_fileTable[fileNumber].text = text;
    }
}

static void pexec_OpenFile(uint16_t fileNumber, openMode_t openMode)
{
  const char *modeString;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_OpenFile", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream == NULL)
    {
      fprintf(stderr, g_fileAlreadyOpen, "pexec_OpenFile", fileNumber);
    }
  else
    {
      switch (openMode)
        {
          case eOPEN_READ :
            modeString = "r";
            break;

          case eOPEN_WRITE :
            modeString = "w";
            break;

          case eOPEN_APPEND :
            modeString = "a";
            break;

          default :
            fprintf(stderr, g_badOpenMode, "pexec_OpenFile", (int)openMode,
                    fileNumber);
            return;
        }

      g_fileTable[fileNumber].stream = fopen(g_fileTable[fileNumber].fileName,
                                             modeString);
      if (g_fileTable[fileNumber].stream == NULL)
        {
          fprintf(stderr, g_openFailed, "pexec_OpenFile",
                  strerror(errno), g_fileTable[fileNumber].fileName,
                  fileNumber);
        }
      else
        {
          g_fileTable[fileNumber].openMode = openMode;
        }
    }
}

static void pexec_CloseFile(uint16_t fileNumber)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_CloseFile", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream == NULL)
    {
      fprintf(stderr, g_fileNotOpen, "pexec_CloseFile", fileNumber);
    }
  else
    {
      (void)fclose(g_fileTable[fileNumber].stream);
      g_fileTable[fileNumber].stream = NULL;
    }
}

static void pexec_RecordSize(uint16_t fileNumber, uint16_t size)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_RecordSize", fileNumber);
    }
  else
    {
      g_fileTable[fileNumber].recordSize = size;
    }
}

static void pexec_ReadBinary(uint16_t fileNumber, uint8_t *dest, uint16_t size)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_ReadBinary", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_READ)
    {
      fprintf(stderr, g_notOpenForRead, "pexec_ReadBinary", fileNumber);
    }
  else
    {
      size_t nitems = fread(dest, 1, size, g_fileTable[fileNumber].stream);
      if (nitems < size && ferror(g_fileTable[fileNumber].stream))
        {
          fprintf(stderr, g_readFailed, "pexec_ReadBinary", strerror(errno),
                  fileNumber);
          clearerr(g_fileTable[fileNumber].stream);
        }
    }
}

static void pexec_ReadInteger(uint16_t fileNumber, ustack_t *dest)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_ReadInteger", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_READ)
    {
      fprintf(stderr, g_notOpenForRead, "pexec_ReadInteger", fileNumber);
    }
  else
    {
      char *ptr = fgets((char *)g_ioLine, LINE_SIZE,
                         g_fileTable[fileNumber].stream);
      if (ptr == NULL && ferror(g_fileTable[fileNumber].stream))
        {
          fprintf(stderr, g_readFailed, "pexec_ReadInteger", strerror(errno),
                  fileNumber);
          clearerr(g_fileTable[fileNumber].stream);
        }

      *dest = pexec_ConvertInteger(fileNumber, g_ioLine);
    }
}

static void pexec_ReadChar(uint16_t fileNumber, uint8_t *dest)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_ReadChar", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_READ)
    {
      fprintf(stderr, g_notOpenForRead, "pexec_ReadChar", fileNumber);
    }
  else
    {
      char *ptr = fgets((char *)g_ioLine, LINE_SIZE,
                        g_fileTable[fileNumber].stream);
      if (ptr == NULL && ferror(g_fileTable[fileNumber].stream))
        {
          fprintf(stderr, g_readFailed, "pexec_ReadChar", strerror(errno),
                  fileNumber);
          clearerr(g_fileTable[fileNumber].stream);
        }

      *dest = g_ioLine[0];
    }
}

static void pexec_ReadString(uint16_t fileNumber, char *strPtr,
                             uint16_t size)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_ReadString", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_READ)
    {
      fprintf(stderr, g_notOpenForRead, "pexec_ReadString", fileNumber);
    }
  else
    {
      char *ptr = fgets(strPtr, size, g_fileTable[fileNumber].stream);
      if (ptr == NULL && ferror(g_fileTable[fileNumber].stream))
        {
          fprintf(stderr, g_readFailed, "pexec_ReadString", strerror(errno),
                  fileNumber);
          clearerr(g_fileTable[fileNumber].stream);
        }
    }
}

static void pexec_ReadReal(uint16_t fileNumber, uint16_t *dest)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_ReadReal", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_READ)
    {
      fprintf(stderr, g_notOpenForRead, "pexec_ReadReal", fileNumber);
    }
  else
    {
      char *ptr = fgets((char*)g_ioLine, LINE_SIZE,
                        g_fileTable[fileNumber].stream);
      if (ptr == NULL && ferror(g_fileTable[fileNumber].stream))
        {
          fprintf(stderr, g_readFailed, "pexec_ReadReal", strerror(errno),
                  fileNumber);
          clearerr(g_fileTable[fileNumber].stream);
        }

      pexec_ConvertReal(dest, g_ioLine);
    }
}

static void  pexec_WriteBinary(uint16_t fileNumber, const uint8_t *src,
                               uint16_t size)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_WriteBinary", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_WRITE)
    {
      fprintf(stderr, g_notOpenForWrite, "pexec_WriteBinary", fileNumber);
    }
  else
    {
      ssize_t nitems = fwrite(src, 1, size, g_fileTable[fileNumber].stream);
      if (nitems < 0 && ferror(g_fileTable[fileNumber].stream))
        {
          fprintf(stderr, g_readFailed, "pexec_WriteBinary", strerror(errno),
                  fileNumber);
          clearerr(g_fileTable[fileNumber].stream);
        }
    }
}

static void pexec_WriteInteger(uint16_t fileNumber, int16_t value)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_WriteInteger", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_WRITE)
    {
      fprintf(stderr, g_notOpenForWrite, "pexec_WriteInteger", fileNumber);
    }
  else
    {
      int nbytes = fprintf(g_fileTable[fileNumber].stream, "%d", value);
      if (nbytes < 0)
        {
          fprintf(stderr, g_writeFailed, "pexec_WriteInteger", strerror(errno),
                  fileNumber);
        }
    }
}

static void pexec_WriteChar(uint16_t fileNumber, uint8_t value)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_WriteChar", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_WRITE)
    {
      fprintf(stderr, g_notOpenForWrite, "pexec_WriteChar", fileNumber);
    }
  else
    {
      int result = fputc(value, g_fileTable[fileNumber].stream);
      if (result == EOF && ferror(g_fileTable[fileNumber].stream))
        {
          fprintf(stderr, g_writeFailed, "pexec_WriteChar", strerror(errno),
                  fileNumber);
          clearerr(g_fileTable[fileNumber].stream);
        }
    }
}

static void pexec_WriteReal(uint16_t fileNumber, double value)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_WriteReal", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_WRITE)
    {
      fprintf(stderr, g_notOpenForWrite, "pexec_WriteReal", fileNumber);
    }
  else
    {
      int nbytes = fprintf(g_fileTable[fileNumber].stream, "%f", value);
      if (nbytes < 0)
        {
          fprintf(stderr, g_writeFailed, "pexec_WriteReal", strerror(errno),
                  fileNumber);
        }
    }
}

static void pexec_WriteString(uint16_t fileNumber, const char *strPtr,
                              uint16_t size)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
      fprintf(stderr, g_badFileNumber, "pexec_WriteString", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_WRITE)
    {
      fprintf(stderr, g_notOpenForWrite, "pexec_WriteString", fileNumber);
    }
  else
    {
      ssize_t nitems = fwrite(strPtr, 1, size,
                              g_fileTable[fileNumber].stream);
      if (nitems < 0 && ferror(g_fileTable[fileNumber].stream))
        {
          fprintf(stderr, g_readFailed, "pexec_WriteString", strerror(errno),
                  fileNumber);
        }
    }
}

static uint16_t pexec_Eof(uint16_t fileNumber)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
       fprintf(stderr, g_badFileNumber, "pexec_Eof", fileNumber);
       return PASCAL_FALSE;
    }
  else
    {
      if (feof(g_fileTable[fileNumber].stream))
        {
          return PASCAL_TRUE;
        }
      else
        {
          return PASCAL_FALSE;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void pexec_InitializeFile(void)
{
  int fileNumber;

  /* Close all open files (except INPUT and OUTPUT). */

  for (fileNumber = 2; fileNumber < MAX_OPEN_FILES; fileNumber++)
    {
      if (g_fileTable[fileNumber].stream != NULL)
        {
          (void)fclose(g_fileTable[fileNumber].stream);
        }
    }

  /* Then Re-initalize INPUT and OUTPUT */

  memset(g_fileTable, 0, MAX_OPEN_FILES * sizeof(pexecFileTable_t));

  strcpy(g_fileTable[INPUT_FILE_NUMBER].fileName, "INPUT");
  g_fileTable[INPUT_FILE_NUMBER].stream      = stdin;
  g_fileTable[INPUT_FILE_NUMBER].openMode    = eOPEN_READ;
  g_fileTable[INPUT_FILE_NUMBER].recordSize  = 1;
  g_fileTable[INPUT_FILE_NUMBER].text        = true;

  strcpy(g_fileTable[OUTPUT_FILE_NUMBER].fileName, "OUTPUT");
  g_fileTable[OUTPUT_FILE_NUMBER].stream     = stdout;
  g_fileTable[OUTPUT_FILE_NUMBER].openMode   = eOPEN_WRITE;
  g_fileTable[OUTPUT_FILE_NUMBER].recordSize = 1;
  g_fileTable[OUTPUT_FILE_NUMBER].text       = true;
}

/****************************************************************************
 * Name: pexec_sysio
 *
 * Description:
 *   This function process a system I/O operation.
 *
 ****************************************************************************/

uint16_t pexec_sysio(struct pexec_s *st, uint16_t subfunc)
{
  fparg_t  fp;
  uint16_t fileNumber;
  uint16_t size;
  uint16_t address;
  uint16_t value;

  switch (subfunc)
    {
    /* EOF: TOS = File number */

    case xEOF :
      TOS(st, 0) = pexec_Eof(TOS(st, 0));
      break;

    /* EOLN: TOS = File number */

    case xEOLN :
      /* REVISIT:  Not implemented */
      TOS(st, 0) =  0;
      break;

    /* ASSIGNFILE: TOS     = File name pointer
     *             TOS + 1 = 0:binary 1:textfile
     *             TOS + 2 = File number
     */

    case xASSIGNFILE :
      POP(st, address);     /* File name string address */
      POP(st, size);        /* File name string size */
      POP(st, value);       /* Binary/text boolean from stack */
      POP(st, fileNumber);  /* File number from stack */
      pexec_AssignFile(fileNumber, (bool)value,
                       (const char *)&st->dstack.b[address]);
      break;

    /* RESET: TOS = File number */

    case xRESET :
      POP(st, fileNumber);  /* File number from stack */
      pexec_OpenFile(fileNumber, eOPEN_READ);
      break;

    /* RESETR: TOS   = New record size
     *         TOS+1 = File number
     */

    case xRESETR :
      POP(st, size);  /* File number from stack */
      POP(st, fileNumber);  /* File number from stack */
      pexec_OpenFile(fileNumber, eOPEN_READ);
      pexec_RecordSize(fileNumber, size);
      break;

    /* REWRITE: TOS = File number */

    case xREWRITE :
      POP(st, fileNumber);  /* File number from stack */
      pexec_OpenFile(fileNumber, eOPEN_WRITE);
      break;

    /* RESETR: TOS   = New record size
     *         TOS+1 = File number
     */

    case xREWRITER :
      POP(st, size);  /* File number from stack */
      POP(st, fileNumber);  /* File number from stack */
      pexec_OpenFile(fileNumber, eOPEN_WRITE);
      pexec_RecordSize(fileNumber, size);
      break;

    /* APPEND: TOS = File number */

    case xAPPEND :
      POP(st, fileNumber);  /* File number from stack */
      pexec_OpenFile(fileNumber, eOPEN_APPEND);
      break;

    /* CLOSEFILE: TOS = File number */

    case xCLOSEFILE :
      POP(st, fileNumber);  /* File number from stack */
      pexec_CloseFile(fileNumber);
      break;

    /* READLN: TOS = File number */

    case xREADLN :
      /* REVISIT:  Not implemented */
      POP(st, fileNumber);  /* File number from stack */
      break;

    /* READ_BINARY: TOS   = Read size
     *              TOS+1 = Read address
     *              TOS+2 = File number
     */

    case xREAD_BINARY :
      POP(st, size);        /* Read size */
      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      pexec_ReadBinary(fileNumber,
                       (uint8_t *)&st->dstack.b[address],
                       size);
      break;

    /* READ_INT: TOS   = Read address
     *           TOS+1 = File number
     */

    case xREAD_INT :

      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      pexec_ReadInteger(fileNumber,
                        (ustack_t *)&st->dstack.b[address]);
      break;

    /* READ_CHAR: TOS   = Read address
     *            TOS+1 = File number
     */

    case xREAD_CHAR:
      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      pexec_ReadChar(fileNumber,
                     (uint8_t *)&st->dstack.b[address]);
      break;

    /* READ_STRING: TOS   = Read size
     *              TOS+1 = Read address
     *              TOS+2 = File number
     */

    case xREAD_STRING :
      POP(st, size);        /* Read address */
      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      pexec_ReadString(fileNumber,
                       (char *)&st->dstack.b[address],
                       size);
      break;

    /* READ_REAL: TOS   = Read address
     *            TOS+1 = File number
     */

    case xREAD_REAL :
      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      pexec_ReadReal(fileNumber,
                     (uint16_t *)&st->dstack.b[address]);
      break;

    /* WRITELN: TOS = File number */

    case xWRITELN :
      POP(st, fileNumber);  /* File number from stack */
      pexec_WriteChar(fileNumber, '\n');
      break;

    /* WRITE_PAGE: TOS = File number */

    case xWRITE_PAGE :
      POP(st, fileNumber);  /* File number from stack */
      pexec_WriteChar(fileNumber, '\f');
      break;

    /* WRITE_BINARY: TOS   = Write size
     *               TOS+1 = Write address
     *               TOS+2 = File number
     */

    case xWRITE_BINARY :
      POP(st, size);        /* Write size */
      POP(st, address);     /* Write address */
      POP(st, fileNumber);  /* File number from stack */

      pexec_WriteBinary(fileNumber,
                        (const uint8_t *)&st->dstack.b[address],
                        size);
      break;

    /* WRITE_INT: TOS   = Write value
     *            TOS+1 = File number
     */

    case xWRITE_INT :
      POP(st, value);       /* Write address */
      POP(st, fileNumber);  /* File number from stack */

      pexec_WriteInteger(fileNumber, (int16_t)value);
      break;

    /* WRITE_CHAR: TOS   = Write value
     *             TOS+1 = File number
     */

    case xWRITE_CHAR :
      POP(st, value);       /* Write value */
      POP(st, fileNumber);  /* File number from stack */

      pexec_WriteChar(fileNumber, (uint8_t)value);
      break;

    /* WRITE_STRING: TOS   = Write size
     *               TOS+1 = Write address
     *               TOS+2 = File number
     */

    case xWRITE_STRING :
      POP(st, size);        /* String size */
      POP(st, address);     /* String address */
      POP(st, fileNumber);  /* File number from stack */

      pexec_WriteString(fileNumber,
                        (const char *)&st->dstack.b[address],
                        size);
      break;

    /* WRITE_REAL: TOS-TOS+3 = Write value
     *             TOS+4     = File number
     */

    case xWRITE_REAL :
      POP(st, fp.hw[3]);    /* Write value */
      POP(st, fp.hw[2]);
      POP(st, fp.hw[1]);
      POP(st, fp.hw[0]);
      POP(st, fileNumber);  /* File number from stack */

      pexec_WriteReal(fileNumber, fp.f);
      break;

    default :
      return eBADSYSIOFUNC;
    }

  return eNOERROR;
}


