/****************************************************************************
 * psysio.c
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

#include "psysio.h"

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
  bool inUse;
  bool text;
  bool eoln;
  uint16_t recordSize;
  int16_t eof;
  FILE *stream;
  openMode_t openMode;
};

typedef struct pexecFileTable_s pexecFileTable_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ustack_t pexec_ConvertInteger(uint16_t fileNumber, uint8_t *ioPtr);
static void     pexec_ConvertReal(uint16_t *dest, uint8_t *ioPtr);
static const char *pexec_GetFormat(char fmtChar, uint8_t fieldWidth,
                                   uint8_t precision);
static void     pexec_CheckEoln(uint16_t fileNumber, char *buffer);
static ustack_t pexec_AllocateFile(void);
static int      pexec_FreeFile(uint16_t fileNumber);
static int      pexec_AssignFile(uint16_t fileNumber, bool text,
                                 const char *fileName, uint16_t size);
static int      pexec_OpenFile(uint16_t fileNumber, openMode_t openMode);
static int      pexec_CloseFile(uint16_t fileNumber);
static int      pexec_RecordSize(uint16_t fileNumber, uint16_t size);
static int      pexec_ReadBinary(uint16_t fileNumber, uint8_t *dest,
                                 uint16_t size);
static int      pexec_ReadInteger(uint16_t fileNumber, ustack_t *dest);
static int      pexec_ReadChar(uint16_t fileNumber, uint8_t *dest);
static int      pexec_ReadString(struct pexec_s *st, uint16_t fileNumber,
                                 uint16_t *stringVarPtr, uint16_t readSize);
static int      pexec_ReadReal(uint16_t fileNumber, uint16_t *dest);
static int      pexec_WriteBinary(uint16_t fileNumber, const uint8_t *src,
                                  uint16_t size);
static int      pexec_WriteInteger(uint16_t fileNumber, int16_t value,
                                   uint16_t fieldWidth);
static int      pexec_WriteChar(uint16_t fileNumber, uint8_t value,
                                uint16_t fieldWidth);
static int      pexec_WriteReal(uint16_t fileNumber, double value,
                                uint16_t fieldWidth);
static int      pexec_WriteString(uint16_t fileNumber, const char *string,
                                  uint16_t size, uint16_t fieldWidth);
static int      pexec_Eof(struct pexec_s *st, uint16_t fileNumber);
static int      pexec_Eoln(struct pexec_s *st, uint16_t fileNumber);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The runtime file table.  Maps a fileNumber to the current state of the
 * file.
 */

static pexecFileTable_t g_fileTable[MAX_OPEN_FILES];

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
          /* errorCode = eINTEGEROVERFLOW; */
          value = UINT16_MAX;
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

static const char *pexec_GetFormat(char fmtChar, uint8_t fieldWidth,
                                   uint8_t precision)
{
  static char fmt[20];

  if (fieldWidth > 0)
    {
      if (precision > 0)
        {
          snprintf(fmt, 20, "%%%u.%u%c", fieldWidth, precision, fmtChar);
        }
      else
        {
          snprintf(fmt, 20, "%%%u%c", fieldWidth, fmtChar);
        }
    }
  else
    {
      snprintf(fmt, 20, "%%%c", fmtChar);
    }

  return fmt;
}

static void pexec_CheckEoln(uint16_t fileNumber, char *buffer)
{
  bool eoln = false;
  int len;

  /* fgets will always consume the terminating newline character unless the
   * line is only that the provided read buffer.  The newline character
   * should be the last character read.
   */

  len = strlen(buffer) - 1;
  if (len > 0)
    {
      if (buffer[len] == '\n')
        {
          buffer[len] = '\0';
          eoln = true;
        }
    }

  g_fileTable[fileNumber].eoln = eoln;
}

static ustack_t pexec_AllocateFile(void)
{
  uint16_t fileNumber;

  for (fileNumber = 0; fileNumber < MAX_OPEN_FILES; fileNumber++)
    {
      if (!g_fileTable[fileNumber].inUse)
        {
          g_fileTable[fileNumber].inUse = true;
          return fileNumber;
        }
    }

  return fileNumber;  /* Return the out-of-range file number */
}

static int pexec_FreeFile(uint16_t fileNumber)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (!g_fileTable[fileNumber].inUse)
    {
      errorCode = eFILENOTINUSE;
    }
  else
    {
      /* If the file was opened, then close it */

      if (g_fileTable[fileNumber].stream != NULL)
        {
          pexec_CloseFile(fileNumber);
        }

      /* Reset the entire file entry */

      memset(&g_fileTable[fileNumber], 0, sizeof(pexecFileTable_t));
    }

  return errorCode;
}

static int pexec_AssignFile(uint16_t fileNumber, bool text, const char *fileName,
                             uint16_t size)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else
    {
      strncpy(g_fileTable[fileNumber].fileName, fileName, MAX_FILE_NAME);
      g_fileTable[fileNumber].text = text;
    }

  return errorCode;
}

static int pexec_OpenFile(uint16_t fileNumber, openMode_t openMode)
{
  int errorCode = eNOERROR;

  const char *modeString;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream != NULL)
    {
      errorCode = eFILEALREADYOPEN;
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
            return eBADOPENMODE;
        }

      g_fileTable[fileNumber].stream = fopen(g_fileTable[fileNumber].fileName,
                                             modeString);
      if (g_fileTable[fileNumber].stream == NULL)
        {
          errorCode = eOPENFAILED;
        }
      else
        {
          g_fileTable[fileNumber].openMode = openMode;
        }
    }

  return errorCode;
}

static int pexec_CloseFile(uint16_t fileNumber)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream == NULL)
    {
      errorCode = eFILENOTOPEN;
    }
  else
    {
      (void)fclose(g_fileTable[fileNumber].stream);
      g_fileTable[fileNumber].stream = NULL;
    }

  return errorCode;
}

static int pexec_RecordSize(uint16_t fileNumber, uint16_t size)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else
    {
      g_fileTable[fileNumber].recordSize = size;
    }

  return errorCode;
}

static int pexec_ReadBinary(uint16_t fileNumber, uint8_t *dest, uint16_t size)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_READ)
    {
      errorCode = eNOTOPENFORREAD;
    }
  else
    {
      size_t nitems = fread(dest, 1, size, g_fileTable[fileNumber].stream);
      if (nitems < size && ferror(g_fileTable[fileNumber].stream))
        {
          errorCode = eREADFAILED;
          clearerr(g_fileTable[fileNumber].stream);
        }
    }

  return errorCode;
}

static int pexec_ReadInteger(uint16_t fileNumber, ustack_t *dest)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_READ)
    {
      errorCode = eNOTOPENFORREAD;
    }
  else
    {
      char *ptr = fgets((char *)g_ioLine, LINE_SIZE,
                         g_fileTable[fileNumber].stream);

      if (ptr == NULL && ferror(g_fileTable[fileNumber].stream))
        {
          errorCode = eREADFAILED;
          clearerr(g_fileTable[fileNumber].stream);
        }
      else
        {
          pexec_CheckEoln(fileNumber, (char *)g_ioLine);
          *dest = pexec_ConvertInteger(fileNumber, g_ioLine);
        }
    }

  return errorCode;
}

static int pexec_ReadChar(uint16_t fileNumber, uint8_t *dest)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_READ)
    {
      errorCode = eNOTOPENFORREAD;
    }
  else
    {
      char *ptr = fgets((char *)g_ioLine, LINE_SIZE,
                        g_fileTable[fileNumber].stream);

      if (ptr == NULL && ferror(g_fileTable[fileNumber].stream))
        {
          errorCode = eREADFAILED;
          clearerr(g_fileTable[fileNumber].stream);
        }
      else
        {
          pexec_CheckEoln(fileNumber, (char *)g_ioLine);
          *dest = g_ioLine[0];
        }
    }

  return errorCode;
}

static int pexec_ReadString(struct pexec_s *st, uint16_t fileNumber,
                            uint16_t *stringVarPtr, uint16_t readSize)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_READ)
    {
      errorCode = eNOTOPENFORREAD;
    }
  else
    {
      uint16_t stringBufferStack;
      char    *stringBufferPtr;
      char    *ptr;

      stringBufferStack = stringVarPtr[sSTRING_DATA_OFFSET / sINT_SIZE];
      stringBufferPtr   = (char *)&st->dstack.b[stringBufferStack];
      ptr               = fgets(stringBufferPtr, readSize,
                                g_fileTable[fileNumber].stream);

      if (ptr == NULL && ferror(g_fileTable[fileNumber].stream))
        {
          errorCode = eREADFAILED;
          clearerr(g_fileTable[fileNumber].stream);
        }
      else
        {
          pexec_CheckEoln(fileNumber, stringBufferPtr);
          stringVarPtr[sSTRING_SIZE_OFFSET / sINT_SIZE] =
            strlen(stringBufferPtr);
        }
    }

  return errorCode;
}

static int pexec_ReadReal(uint16_t fileNumber, uint16_t *dest)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           g_fileTable[fileNumber].openMode != eOPEN_READ)
    {
      errorCode = eNOTOPENFORREAD;
    }
  else
    {
      char *ptr = fgets((char*)g_ioLine, LINE_SIZE,
                        g_fileTable[fileNumber].stream);

      if (ptr == NULL && ferror(g_fileTable[fileNumber].stream))
        {
          errorCode = eREADFAILED;
          clearerr(g_fileTable[fileNumber].stream);
        }
      else
        {
          pexec_CheckEoln(fileNumber, (char *)g_ioLine);
          pexec_ConvertReal(dest, g_ioLine);
        }
    }

  return errorCode;
}

static int pexec_WriteBinary(uint16_t fileNumber, const uint8_t *src,
                             uint16_t size)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           (g_fileTable[fileNumber].openMode != eOPEN_WRITE &&
            g_fileTable[fileNumber].openMode != eOPEN_APPEND))
    {
      errorCode = eNOTOPENFORWRITE;
    }
  else
    {
      ssize_t nitems = fwrite(src, 1, size, g_fileTable[fileNumber].stream);
      if (nitems < 0 && ferror(g_fileTable[fileNumber].stream))
        {
          errorCode = eWRITEFAILED;
          clearerr(g_fileTable[fileNumber].stream);
        }
    }

  return errorCode;
}

static int pexec_WriteInteger(uint16_t fileNumber, int16_t value,
                              uint16_t fieldWidth)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           (g_fileTable[fileNumber].openMode != eOPEN_WRITE &&
            g_fileTable[fileNumber].openMode != eOPEN_APPEND))
    {
      errorCode = eNOTOPENFORWRITE;
    }
  else
    {
      const char *fmt = pexec_GetFormat('d', fieldWidth >> 8, 0);
      int nbytes = fprintf(g_fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

static int pexec_WriteChar(uint16_t fileNumber, uint8_t value,
                           uint16_t fieldWidth)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           (g_fileTable[fileNumber].openMode != eOPEN_WRITE &&
            g_fileTable[fileNumber].openMode != eOPEN_APPEND))
    {
      errorCode = eNOTOPENFORWRITE;
    }
  else
    {
      const char *fmt = pexec_GetFormat('c', fieldWidth >> 8, 0);
      int nbytes = fprintf(g_fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
          clearerr(g_fileTable[fileNumber].stream);
        }
    }

  return errorCode;
}

static int pexec_WriteReal(uint16_t fileNumber, double value,
                           uint16_t fieldWidth)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           (g_fileTable[fileNumber].openMode != eOPEN_WRITE &&
            g_fileTable[fileNumber].openMode != eOPEN_APPEND))
    {
      errorCode = eNOTOPENFORWRITE;
    }
  else
    {
      const char *fmt = pexec_GetFormat('f', fieldWidth >> 8,
                                        fieldWidth & 0x00ff);
      int nbytes = fprintf(g_fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

static int pexec_WriteString(uint16_t fileNumber, const char *stringDataPtr,
                             uint16_t size, uint16_t fieldWidth)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (g_fileTable[fileNumber].stream    == NULL ||
           (g_fileTable[fileNumber].openMode != eOPEN_WRITE &&
            g_fileTable[fileNumber].openMode != eOPEN_APPEND))
    {
      errorCode = eNOTOPENFORWRITE;
    }
  else
    {
      size_t nItems;

      /* Right justify */

      for (fieldWidth >>= 8; fieldWidth > size; fieldWidth--)
        {
          fputc(' ', g_fileTable[fileNumber].stream);
        }

      /* Then write the string */

      nItems = fwrite(stringDataPtr, 1, size,
                      g_fileTable[fileNumber].stream);
      if (nItems < 0 && ferror(g_fileTable[fileNumber].stream))
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

static int pexec_Eof(struct pexec_s *st, uint16_t fileNumber)
{
  int errorCode = eNOERROR;
  uint16_t eof;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
      eof       = PASCAL_TRUE;
    }
  else
    {
      if (feof(g_fileTable[fileNumber].stream))
        {
          eof = PASCAL_TRUE;
        }
      else
        {
          eof = PASCAL_FALSE;
        }
    }

  PUSH(st, eof);
  return errorCode;
}

static int pexec_Eoln(struct pexec_s *st, uint16_t fileNumber)
{
  int errorCode = eNOERROR;
  uint16_t eoln;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
      eoln      = PASCAL_TRUE;
    }
  else
    {
      eoln      = g_fileTable[fileNumber].eoln
                  ? PASCAL_TRUE : PASCAL_FALSE;
    }

  PUSH(st, eoln);
  return errorCode;
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
  g_fileTable[INPUT_FILE_NUMBER].inUse       = true;
  g_fileTable[INPUT_FILE_NUMBER].text        = true;
  g_fileTable[INPUT_FILE_NUMBER].recordSize  = 1;
  g_fileTable[INPUT_FILE_NUMBER].stream      = stdin;
  g_fileTable[INPUT_FILE_NUMBER].openMode    = eOPEN_READ;

  strcpy(g_fileTable[OUTPUT_FILE_NUMBER].fileName, "OUTPUT");
  g_fileTable[OUTPUT_FILE_NUMBER].inUse      = true;
  g_fileTable[OUTPUT_FILE_NUMBER].text       = true;
  g_fileTable[OUTPUT_FILE_NUMBER].recordSize = 1;
  g_fileTable[OUTPUT_FILE_NUMBER].stream     = stdout;
  g_fileTable[OUTPUT_FILE_NUMBER].openMode   = eOPEN_WRITE;
}

/****************************************************************************
 * Name: pexec_sysio
 *
 * Description:
 *   This function process a system I/O operation.
 *
 ****************************************************************************/

int pexec_sysio(struct pexec_s *st, uint16_t subfunc)
{
  fparg_t  fp;
  uint16_t fileNumber;
  uint16_t fieldWidth;
  uint16_t size;
  uint16_t address;
  uint16_t value;
  int      errorCode = eNOERROR;

  switch (subfunc)
    {
    /* ALLOCFILE: No stack arguments */

    case xALLOCFILE :
       fileNumber = pexec_AllocateFile();
       if (fileNumber >= MAX_OPEN_FILES)
         {
           errorCode = eTOOMANYFILES;
         }

       PUSH(st, fileNumber);
       break;

    /* FREEFILE: TOS = File number */

    case xFREEFILE :
       POP(st, fileNumber); /* File number */
       errorCode = pexec_FreeFile(fileNumber);
       break;

    /* EOF: TOS = File number */

    case xEOF :
      POP(st, fileNumber); /* File number */
      errorCode = pexec_Eof(st, fileNumber);
      break;

    /* EOLN: TOS = File number */

    case xEOLN :
      POP(st, fileNumber); /* File number */
      errorCode = pexec_Eoln(st, fileNumber);
      break;

    /* ASSIGNFILE: TOS(0) = File name pointer
     *             TOS(1) = 0:binary 1:textfile
     *             TOS(2) = File number
     */

    case xASSIGNFILE :
      POP(st, address);     /* File name string address */
      POP(st, size);        /* File name string size */
      POP(st, value);       /* Binary/text boolean from stack */
      POP(st, fileNumber);  /* File number from stack */
      errorCode = pexec_AssignFile(fileNumber, (value != 0),
                                   (const char *)&st->dstack.b[address],
                                   size);
      break;

    /* RESET: TOS = File number */

    case xRESET :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = pexec_OpenFile(fileNumber, eOPEN_READ);
      break;

    /* RESETR: TOS(0) = New record size
     *         TOS(1) = File number
     */

    case xRESETR :
      POP(st, size);  /* File number from stack */
      POP(st, fileNumber);  /* File number from stack */
      errorCode = pexec_OpenFile(fileNumber, eOPEN_READ);
      errorCode = pexec_RecordSize(fileNumber, size);
      break;

    /* REWRITE: TOS = File number */

    case xREWRITE :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = pexec_OpenFile(fileNumber, eOPEN_WRITE);
      break;

    /* RESETR: TOS(0) = New record size
     *         TOS(1) = File number
     */

    case xREWRITER :
      POP(st, size);  /* File number from stack */
      POP(st, fileNumber);  /* File number from stack */
      errorCode = pexec_OpenFile(fileNumber, eOPEN_WRITE);
      errorCode = pexec_RecordSize(fileNumber, size);
      break;

    /* APPEND: TOS = File number */

    case xAPPEND :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = pexec_OpenFile(fileNumber, eOPEN_APPEND);
      break;

    /* CLOSEFILE: TOS = File number */

    case xCLOSEFILE :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = pexec_CloseFile(fileNumber);
      break;

    /* READLN: TOS = File number */

    case xREADLN :
      /* REVISIT:  Not implemented */
      POP(st, fileNumber);  /* File number from stack */
      break;

    /* READ_BINARY: TOS(0) = Read address
     *              TOS(1) = Read size
     *              TOS(2) = File number
     */

    case xREAD_BINARY :
      POP(st, address);     /* Read address */
      POP(st, size);        /* Read size */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = pexec_ReadBinary(fileNumber,
                                   (uint8_t *)&st->dstack.b[address],
                                   size);
      break;

    /* READ_INT: TOS(0) = Read address
     *           TOS(1) = File number
     */

    case xREAD_INT :

      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = pexec_ReadInteger(fileNumber,
                                    (ustack_t *)&st->dstack.b[address]);
      break;

    /* READ_CHAR: TOS(0) = Read address
     *            TOS(1) = File number
     */

    case xREAD_CHAR:
      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = pexec_ReadChar(fileNumber,
                                 (uint8_t *)&st->dstack.b[address]);
      break;

    /* READ_STRING: TOS(0) = String variable address
     *              TOS(1) = File number
     */

    case xREAD_STRING :
      POP(st, address);     /* Standard string variable address */
      POP(st, fileNumber);  /* File number */

      errorCode = pexec_ReadString(st, fileNumber,
                                   (uint16_t *)&st->dstack.b[address],
                                   st->strsize);
      break;

    case xREAD_SHORTSTRING :
      {
        uint16_t *strPtr;
        uint16_t  strAlloc;

        POP(st, address);     /* Short string variable address */
        POP(st, fileNumber);  /* File number */

        /* Get the allocation size of the short string */

        strPtr = (uint16_t *)&st->dstack.b[address];
        strAlloc = strPtr[sSHORTSTRING_ALLOC_OFFSET / sINT_SIZE];

        errorCode = pexec_ReadString(st, fileNumber, strPtr,strAlloc);
      }
      break;

    /* READ_REAL: TOS(0) = Read address
     *            TOS(1) = File number
     */

    case xREAD_REAL :
      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = pexec_ReadReal(fileNumber,
                                 (uint16_t *)&st->dstack.b[address]);
      break;

    /* WRITELN: TOS = File number */

    case xWRITELN :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = pexec_WriteChar(fileNumber, '\n', 0);
      break;

    /* WRITE_PAGE: TOS = File number */

    case xWRITE_PAGE :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = pexec_WriteChar(fileNumber, '\f', 0);
      break;

    /* WRITE_BINARY: TOS(0) = Write size
     *               TOS(1) = Write address
     *               TOS(2) = File number
     */

    case xWRITE_BINARY :
      POP(st, address);     /* Write address */
      POP(st, size);        /* Write size */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = pexec_WriteBinary(fileNumber,
                                    (const uint8_t *)&st->dstack.b[address],
                                    size);
      break;

    /* WRITE_INT: TOS(0) = Field width
     *            TOS(1) = Write integer value
     *            TOS(2) = File number
     */

    case xWRITE_INT :
      POP(st, fieldWidth);  /* Field width */
      POP(st, value);       /* Write integer value */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = pexec_WriteInteger(fileNumber, (int16_t)value, fieldWidth);
      break;

    /* WRITE_CHAR: TOS(0) = Field width
     *             TOS(1) = Write value
     *             TOS(2) = File number
     */

    case xWRITE_CHAR :
      POP(st, fieldWidth);  /* Field width */
      POP(st, value);       /* Write value */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = pexec_WriteChar(fileNumber, (uint8_t)value, fieldWidth);
      break;

    /* WRITE_STRING: TOS(0) = Field width
     *               TOS(1) = Write standard string buffer address
     *               TOS(2) = Write standard string size
     *               TOS(3) = File number
     */

    case xWRITE_STRING :
      POP(st, fieldWidth);  /* Field width */
      POP(st, address);     /* String address */
      POP(st, size);        /* String size */
      POP(st, fileNumber);  /* File number */

      errorCode = pexec_WriteString(fileNumber,
                                    (const char *)&st->dstack.b[address],
                                    size, fieldWidth);
      break;

    /* WRITE_SHORTSTRING: TOS(0) = Field width
     *                    TOS(1) = Write short string allocation (not used)
     *                    TOS(2) = Write short string buffer address
     *                    TOS(3) = Write short string size
     *                    TOS(4) = File number
     */

    case xWRITE_SHORTSTRING :
      POP(st, fieldWidth);  /* Field width */
      DISCARD(st, 1);       /* Discard the unused stack allocation */
      POP(st, address);     /* String address */
      POP(st, size);        /* String size */
      POP(st, fileNumber);  /* File number */

      errorCode = pexec_WriteString(fileNumber,
                                    (const char *)&st->dstack.b[address],
                                    size, fieldWidth);
      break;

    /* WRITE_CHAR: TOS(0)   = Field width/precision
     *             TOS(1-4) = Write value
     *             TOS(5)   = File number
     */

    case xWRITE_REAL :
      POP(st, fieldWidth);  /* Field width */
      POP(st, fp.hw[3]);    /* Write value */
      POP(st, fp.hw[2]);
      POP(st, fp.hw[1]);
      POP(st, fp.hw[0]);
      POP(st, fileNumber);  /* File number from stack */

      errorCode = pexec_WriteReal(fileNumber, fp.f, fieldWidth);
      break;

    default :
      errorCode = eBADSYSIOFUNC;
      break;
    }

  return errorCode;
}
