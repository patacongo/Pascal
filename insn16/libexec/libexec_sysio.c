/****************************************************************************
 * libexec_sysio.c
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
#include <inttypes.h>
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

#include "libexec_longops.h"
#include "libexec_sysio.h"

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
  FILE *stream;
  openMode_t openMode;
};

typedef struct pexecFileTable_s pexecFileTable_t;

/* Used for dealing with LongInteger and LongWord types */

union uWord_u
{
  int32_t  sData;
  uint32_t uData;
  uint16_t word[2];
};

typedef union uWord_u uWord_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static ustack_t libexec_ConvertInteger(uint16_t fileNumber, uint8_t *ioPtr);
static void     libexec_ConvertReal(uint16_t *dest, uint8_t *ioPtr);
static void     libexec_CheckEoln(uint16_t fileNumber, char *buffer);
static ustack_t libexec_AllocateFile(void);
static int      libexec_FreeFile(uint16_t fileNumber);
static int      libexec_AssignFile(uint16_t fileNumber, bool text,
                  const char *fileName, uint16_t size);
static int      libexec_OpenFile(uint16_t fileNumber, openMode_t openMode);
static int      libexec_CloseFile(uint16_t fileNumber);
static int      libexec_RecordSize(uint16_t fileNumber, uint16_t size);
static int      libexec_ReadLn(uint16_t fileNumber);
static int      libexec_ReadBinary(uint16_t fileNumber, uint8_t *dest,
                  uint16_t size);
static int      libexec_ReadInteger(uint16_t fileNumber, ustack_t *dest);
static int      libexec_ReadChar(uint16_t fileNumber, uint8_t *dest);
static int      libexec_ReadString(struct libexec_s *st, uint16_t fileNumber,
                  uint16_t *stringVarPtr, uint16_t readSize);
static int      libexec_ReadReal(uint16_t fileNumber, uint16_t *dest);
static int      libexec_WriteBinary(uint16_t fileNumber, const uint8_t *src,
                  uint16_t size);
static int      libexec_WriteInteger(uint16_t fileNumber, int16_t value,
                  uint16_t fieldWidth);
static int      libexec_WriteLongInteger(uint16_t fileNumber, int32_t value,
                  uint16_t fieldWidth);
static int      libexec_WriteWord(uint16_t fileNumber, uint16_t value,
                  uint16_t fieldWidth);
static int      libexec_WriteChar(uint16_t fileNumber, uint8_t value,
                  uint16_t fieldWidth);
static int      libexec_WriteReal(uint16_t fileNumber, double value,
                  uint16_t fieldWidth);
static int      libexec_WriteString(uint16_t fileNumber, const char *string,
                  uint16_t size, uint16_t fieldWidth);
static int      libexec_GetFileSize(FILE *stream, off_t *fileSize);
static int      libexec_Eof(struct libexec_s *st, uint16_t fileNumber);
static int      libexec_Eoln(struct libexec_s *st, uint16_t fileNumber);
static int      libexec_FilePos(struct libexec_s *st, uint16_t fileNumber);
static int      libexec_FileSize(struct libexec_s *st, uint16_t fileNumber);
static int      libexec_Seek(struct libexec_s *st, uint16_t fileNumber,
                  uint32_t filePos);
static int      libexec_SeekEof(struct libexec_s *st, uint16_t fileNumber);
static int      libexec_SeekEoln(struct libexec_s *st, uint16_t fileNumber);

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
 * Name: libexec_ConvertInteger
 *
 * Description:
 *   This function parses a decimal integer from ioPtr
 ****************************************************************************/

static ustack_t libexec_ConvertInteger(uint16_t fileNumber, uint8_t *ioPtr)
{
  long int value = 0;
  bool negative = false;

  /* Skip over leading spaces */

  while (isspace(*ioPtr))
    {
      ioPtr++;
    }

  /* Check for a sign */

  if (*ioPtr == '+' || *ioPtr == '-')
    {
      negative = (*ioPtr == '-');
      ioPtr++;
    }


  while (*ioPtr >= '0' && *ioPtr <= '9')
    {
      value = 10 * value + (sstack_t)(*ioPtr) - (sstack_t)'0';
      ioPtr++;

      if (value > INT16_MAX)
        {
          /* errorCode = eINTEGEROVERFLOW; */

          value = negative ? INT16_MAX : INT16_MAX + 1;
          break;
        }
    }

  if (negative)
    {
      value = -value;
    }

  return (ustack_t)value;
}

/****************************************************************************
 * Name: libexec_ConvertReal
 *
 * Description:
 *    This function parses a decimal integer from ioPtr.
 *
 ****************************************************************************/

static void libexec_ConvertReal(uint16_t *dest, uint8_t *inPtr)
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

/****************************************************************************/

static void libexec_CheckEoln(uint16_t fileNumber, char *buffer)
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

/****************************************************************************/

static ustack_t libexec_AllocateFile(void)
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

/****************************************************************************/

static int libexec_FreeFile(uint16_t fileNumber)
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
          libexec_CloseFile(fileNumber);
        }

      /* Reset the entire file entry */

      memset(&g_fileTable[fileNumber], 0, sizeof(pexecFileTable_t));
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_AssignFile(uint16_t fileNumber, bool text, const char *fileName,
                              uint16_t size)
{
  int errorCode = eNOERROR;

  /* Verify the fileNumber */

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (size >= MAX_FILE_NAME) /* include NUL terminator */
    {
      errorCode = eBADFILENAME;
    }
  else
    {
      /* Copy the file name into the file table */

      memcpy(g_fileTable[fileNumber].fileName, fileName, size);

      /* NUL terminate the fileName so that we can use it like a C string */

      g_fileTable[fileNumber].fileName[size] = '\0';

      /* The set the file type */

      g_fileTable[fileNumber].text = text;
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_OpenFile(uint16_t fileNumber, openMode_t openMode)
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

/****************************************************************************/

static int libexec_CloseFile(uint16_t fileNumber)
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

/****************************************************************************/

static int libexec_RecordSize(uint16_t fileNumber, uint16_t size)
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

/****************************************************************************/

static int libexec_ReadLn(uint16_t fileNumber)
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
  else if (g_fileTable[fileNumber].eoln)
    {
      g_fileTable[fileNumber].eoln = false;
    }
  else
    {
      int ch;

      do
        {
          ch = fgetc(g_fileTable[fileNumber].stream);
        }
      while (ch != EOF && ch != '\n');
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_ReadBinary(uint16_t fileNumber, uint8_t *dest, uint16_t size)
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

/****************************************************************************/

static int libexec_ReadInteger(uint16_t fileNumber, ustack_t *dest)
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
          libexec_CheckEoln(fileNumber, (char *)g_ioLine);
          *dest = libexec_ConvertInteger(fileNumber, g_ioLine);
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_ReadChar(uint16_t fileNumber, uint8_t *dest)
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
          libexec_CheckEoln(fileNumber, (char *)g_ioLine);
          *dest = g_ioLine[0];
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_ReadString(struct libexec_s *st, uint16_t fileNumber,
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
          libexec_CheckEoln(fileNumber, stringBufferPtr);
          stringVarPtr[sSTRING_SIZE_OFFSET / sINT_SIZE] =
            strlen(stringBufferPtr);
        }
    }

  return errorCode;
}

static int libexec_ReadReal(uint16_t fileNumber, uint16_t *dest)
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
          libexec_CheckEoln(fileNumber, (char *)g_ioLine);
          libexec_ConvertReal(dest, g_ioLine);
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteBinary(uint16_t fileNumber, const uint8_t *src,
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

/****************************************************************************/

static int libexec_WriteInteger(uint16_t fileNumber, int16_t value,
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
      const char *fmt = libexec_GetFormat("d", fieldWidth >> 8, 0);
      int nbytes = fprintf(g_fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteLongInteger(uint16_t fileNumber, int32_t value,
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
      const char *fmt = libexec_GetFormat(PRId32, fieldWidth >> 8, 0);
      int nbytes = fprintf(g_fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteWord(uint16_t fileNumber, uint16_t value,
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
      const char *fmt = libexec_GetFormat("u", fieldWidth >> 8, 0);
      int nbytes = fprintf(g_fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteLongWord(uint16_t fileNumber, uint32_t value,
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
      const char *fmt = libexec_GetFormat(PRIu32, fieldWidth >> 8, 0);
      int nbytes = fprintf(g_fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteChar(uint16_t fileNumber, uint8_t value,
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
      const char *fmt = libexec_GetFormat("c", fieldWidth >> 8, 0);
      int nbytes = fprintf(g_fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
          clearerr(g_fileTable[fileNumber].stream);
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteReal(uint16_t fileNumber, double value,
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
      const char *fmt = libexec_GetFormat("f", fieldWidth >> 8,
                                          fieldWidth & 0x00ff);
      int nbytes = fprintf(g_fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteString(uint16_t fileNumber, const char *stringDataPtr,
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

/****************************************************************************/

static int libexec_GetFileSize(FILE *stream, off_t *fileSize)
{
  int errorCode = eNOERROR;
  off_t oldPos = 0;
  off_t endPos;
  int ret;

  /* Get the current file position */

  oldPos = ftell(stream);
  if (oldPos < 0)
    {
      errorCode = eFTELLFAILED;
    }
  else
    {
      /* Seek to the end of the file */

      ret = fseek(stream, 0, SEEK_END);
      if (ret < 0)
        {
          errorCode = eFSEEKFAILED;
        }
      else
        {
          endPos = ftell(stream);
          if (endPos < 0)
            {
              errorCode = eFTELLFAILED;
            }

          /* Restore the original position */

          ret = fseek(stream, oldPos, SEEK_SET);
          if (ret < 0)
            {
              errorCode = eFSEEKFAILED;
            }
        }
    }

  *fileSize = endPos;
  return errorCode;
}

/****************************************************************************/

static int libexec_Eof(struct libexec_s *st, uint16_t fileNumber)
{
  int errorCode = eNOERROR;
  uint16_t eof = PASCAL_FALSE;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
      eof       = PASCAL_TRUE;
    }
  else if (g_fileTable[fileNumber].stream == NULL)
    {
      errorCode = eFILENOTOPEN;
      eof       = PASCAL_TRUE;
    }
  else if (feof(g_fileTable[fileNumber].stream))
    {
      eof = PASCAL_TRUE;
    }
  else
    {
      off_t fileSize;
      off_t filePos;

      /* The C feof() function does not return true until we actually attempt
       * to read past the end-of-file.  We need to check if we are at the end
       * of file even if feof() returns false.
       */

      filePos = ftell(g_fileTable[fileNumber].stream);
      if (filePos < 0)
        {
          errorCode = eFTELLFAILED;
          eof       = PASCAL_TRUE;
        }
      else
        {
          errorCode = libexec_GetFileSize(g_fileTable[fileNumber].stream, &fileSize);
          if (errorCode != eNOERROR || filePos >= fileSize)
            {
              eof = PASCAL_TRUE;
            }
        }
    }

  PUSH(st, eof);
  return errorCode;
}

/****************************************************************************/

static int libexec_Eoln(struct libexec_s *st, uint16_t fileNumber)
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

/****************************************************************************/
/* Return the current position in the file */

static int libexec_FilePos(struct libexec_s *st, uint16_t fileNumber)
{
  int errorCode = eNOERROR;

  /* FORM: function FilePos(var f : file ): Int64;
   *
   * Entry:
   *   TOS(0)   - fileNumber
   */

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
      off_t pos = ftell(g_fileTable[fileNumber].stream);
      if (pos < 0)
        {
          errorCode = eFTELLFAILED;
        }

      /* REVISIT:  Int64 not yet implemented; substituting LongInteger. */

       libexec_UPush32(st, (uint32_t)pos);
    }

  return errorCode;
}

/****************************************************************************/
/* Return the file size */

static int libexec_FileSize(struct libexec_s *st, uint16_t fileNumber)
{
  int errorCode = eNOERROR;

  /* FORM: function FileSize(var f : file ): Int64;
   *
   * Entry:
   *   TOS(0)   - fileNumber
   */

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
      off_t fileSize;

      errorCode = libexec_GetFileSize(g_fileTable[fileNumber].stream, &fileSize);

      /* REVISIT:  Int64 not yet implemented; substituting LongInteger. */

      libexec_UPush32(st, (uint32_t)fileSize);
    }

  return errorCode;
}

/****************************************************************************/
/* Seek to a position in the file */

static int libexec_Seek(struct libexec_s *st, uint16_t fileNumber, uint32_t filePos)
{
  int errorCode = eNOERROR;

  /* FORM: procedure Seek(var f : file; Pos : Int64);
   *
   * Entry:
   *   TOS(0)   - fileNumber
   *   TOS(1-3) - filePos
   *
   * REVISIT:  Int64 not yet implemented; substituting LongInteger.
   */

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
      int ret = fseek(g_fileTable[fileNumber].stream, filePos, SEEK_SET);
      if (ret < 0)
        {
          errorCode = eFSEEKFAILED;
        }
    }

  return errorCode;
}

/****************************************************************************/
/* Set file position to end of file */

static int libexec_SeekEof(struct libexec_s *st, uint16_t fileNumber)
{
  int errorCode = eNOERROR;
  uint16_t result = PASCAL_FALSE;

  /* FORM: function SeekEOF(var t : TextFile) : Boolean;
   *       function SeekEOF : Boolean;
   */

  /* Entry:
   *   TOS(0) - fileNumber
   * On Return:
   *   TOS(0) - True:  EOF found
   *            False: Non-white space character found before EOF.
   */

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
      for (; ; )
        {
          int ch = fgetc(g_fileTable[fileNumber].stream);
          if (ch == EOF)
            {
              result = PASCAL_TRUE;
              break;
            }
          else if (!isspace(ch))
            {
              result = PASCAL_FALSE;
              break;
            }
        }
    }

  PUSH(st, result);
  return errorCode;
}

/****************************************************************************/
/* Set file position to end of line */

static int libexec_SeekEoln(struct libexec_s *st, uint16_t fileNumber)
{
  int errorCode = eNOERROR;
  uint16_t result = PASCAL_FALSE;


  /* FORM: function SeekEOLn(var t : TextFile) : Boolean;
   *       function SeekEOLn : Boolean;
   */

  /* Entry:
   *   TOS(0) - fileNumber
   * On Return:
   *   TOS(0) - True:  EOLN found
   *            False: Non-white space character found before EOLN.
   */

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
      for (; ; )
        {
          int ch = fgetc(g_fileTable[fileNumber].stream);
          if (ch == '\n')
            {
              result = PASCAL_TRUE;
              break;
            }
          else if (!isspace(ch))
            {
              result = PASCAL_FALSE;
              break;
            }
        }
    }

  PUSH(st, result);
  return errorCode;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void libexec_InitializeFile(void)
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
 * Name: libexec_sysio
 *
 * Description:
 *   This function process a system I/O operation.
 *
 ****************************************************************************/

int libexec_sysio(struct libexec_s *st, uint16_t subfunc)
{
  fparg_t  fp;
  uint16_t fileNumber;
  uint16_t fieldWidth;
  uint16_t size;
  uint16_t address;
  uint16_t uValue;
  int16_t  sValue;
  int      errorCode = eNOERROR;

  switch (subfunc)
    {
    /* ALLOCFILE: No stack arguments */

    case xALLOCFILE :
       fileNumber = libexec_AllocateFile();
       if (fileNumber >= MAX_OPEN_FILES)
         {
           errorCode = eTOOMANYFILES;
         }

       PUSH(st, fileNumber);
       break;

    /* FREEFILE: TOS(0) = File number */

    case xFREEFILE :
       POP(st, fileNumber); /* File number */
       errorCode = libexec_FreeFile(fileNumber);
       break;

    /* EOF: TOS(0) = File number */

    case xEOF :
      POP(st, fileNumber); /* File number */
      errorCode = libexec_Eof(st, fileNumber);
      break;

    /* EOLN: TOS(0) = File number */

    case xEOLN :
      POP(st, fileNumber); /* File number */
      errorCode = libexec_Eoln(st, fileNumber);
      break;

    /* FILEPOS: TOS(0) = File number. */

    case xFILEPOS :
      POP(st, fileNumber); /* File number */
      errorCode = libexec_FilePos(st, fileNumber);
      break;

    /* FILESIZE: TOS(0) = File number */

    case xFILESIZE :
      POP(st, fileNumber); /* File number */
      errorCode = libexec_FileSize(st, fileNumber);
      break;

    /* SEEK: TOS(0) = File number
     *       TOS(2-3) = Int64 file position
     *
     * REVISIT:  Int64 not yet implemented; substituting LongInteger.
     */

    case xSEEK :
      {
        uint32_t filePos;

        POP(st, fileNumber); /* File number */
        filePos = libexec_UPop32(st);
        errorCode = libexec_Seek(st, fileNumber, filePos);
      }
      break;

    /* SEEKEOF: TOS(0) = File number */

    case xSEEKEOF :
      POP(st, fileNumber); /* File number */
      errorCode = libexec_SeekEof(st, fileNumber);
      break;

    /* SEEKEOLN: TOS(0) = File number */

    case xSEEKEOLN :
      POP(st, fileNumber); /* File number */
      errorCode = libexec_SeekEoln(st, fileNumber);
      break;

    /* ASSIGNFILE: TOS(0) = File name pointer
     *             TOS(1) = 0:binary 1:textfile
     *             TOS(2) = File number
     */

    case xASSIGNFILE :
      POP(st, address);     /* File name string address */
      POP(st, size);        /* File name string size */
      POP(st, uValue);      /* Binary/text boolean from stack */
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_AssignFile(fileNumber, (uValue != 0),
                                     (const char *)&st->dstack.b[address],
                                     size);
      break;

    /* RESET: TOS(0) = File number */

    case xRESET :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_OpenFile(fileNumber, eOPEN_READ);
      break;

    /* RESETR: TOS(0) = New record size
     *         TOS(1) = File number
     */

    case xRESETR :
      POP(st, size);  /* File number from stack */
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_OpenFile(fileNumber, eOPEN_READ);
      errorCode = libexec_RecordSize(fileNumber, size);
      break;

    /* REWRITE: TOS(0) = File number */

    case xREWRITE :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_OpenFile(fileNumber, eOPEN_WRITE);
      break;

    /* RESETR: TOS(0) = New record size
     *         TOS(1) = File number
     */

    case xREWRITER :
      POP(st, size);  /* File number from stack */
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_OpenFile(fileNumber, eOPEN_WRITE);
      errorCode = libexec_RecordSize(fileNumber, size);
      break;

    /* APPEND: TOS(0) = File number */

    case xAPPEND :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_OpenFile(fileNumber, eOPEN_APPEND);
      break;

    /* CLOSEFILE: TOS(0) = File number */

    case xCLOSEFILE :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_CloseFile(fileNumber);
      break;

    /* READLN: TOS(0) = File number */

    case xREADLN :
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_ReadLn(fileNumber);
      break;

    /* READ_BINARY: TOS(0) = Read address
     *              TOS(1) = Read size
     *              TOS(2) = File number
     */

    case xREAD_BINARY :
      POP(st, address);     /* Read address */
      POP(st, size);        /* Read size */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_ReadBinary(fileNumber,
                                     (uint8_t *)&st->dstack.b[address],
                                     size);
      break;

    /* READ_INT: TOS(0) = Read address
     *           TOS(1) = File number
     */

    case xREAD_INT :

      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_ReadInteger(fileNumber,
                                      (ustack_t *)&st->dstack.b[address]);
      break;

    /* READ_CHAR: TOS(0) = Read address
     *            TOS(1) = File number
     */

    case xREAD_CHAR:
      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_ReadChar(fileNumber,
                                   (uint8_t *)&st->dstack.b[address]);
      break;

    /* READ_STRING: TOS(0) = String variable address
     *              TOS(1) = File number
     */

    case xREAD_STRING :
      POP(st, address);     /* Standard string variable address */
      POP(st, fileNumber);  /* File number */

      errorCode = libexec_ReadString(st, fileNumber,
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

        errorCode = libexec_ReadString(st, fileNumber, strPtr,strAlloc);
      }
      break;

    /* READ_REAL: TOS(0) = Read address
     *            TOS(1) = File number
     */

    case xREAD_REAL :
      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_ReadReal(fileNumber,
                                   (uint16_t *)&st->dstack.b[address]);
      break;

    /* WRITELN: TOS(0) = File number */

    case xWRITELN :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_WriteChar(fileNumber, '\n', 0);
      break;

    /* WRITE_PAGE: TOS(0) = File number */

    case xWRITE_PAGE :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_WriteChar(fileNumber, '\f', 0);
      break;

    /* WRITE_BINARY: TOS(0) = Write size
     *               TOS(1) = Write address
     *               TOS(2) = File number
     */

    case xWRITE_BINARY :
      POP(st, address);     /* Write address */
      POP(st, size);        /* Write size */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_WriteBinary(fileNumber,
                                      (const uint8_t *)&st->dstack.b[address],
                                      size);
      break;

    /* WRITE_INT: TOS(0) = Field width
     *            TOS(1) = Write integer value
     *            TOS(2) = File number
     */

    case xWRITE_INT :
      POP(st, fieldWidth);  /* Field width */
      POP(st, sValue);      /* Write integer value */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_WriteInteger(fileNumber, sValue, fieldWidth);
      break;

    /* WRITE_LONGINT: TOS(0)   = Field width
     *                TOS(1-2) = Write integer value
     *                TOS(3)   = File number
     */

    case xWRITE_LONGINT :
      {
        uWord_t lword;

        POP(st, fieldWidth);    /* Field width */
        POP(st, lword.word[1]); /* Write integer value[1] */
        POP(st, lword.word[0]); /* Write integer value[0] */
        POP(st, fileNumber);    /* File number from stack */

        errorCode = libexec_WriteLongInteger(fileNumber, lword.sData, fieldWidth);
      }
      break;

    /* WRITE_LONGWORD: TOS(0)   = Field width
     *                 TOS(1-2) = Write unsigned integer value
     *                 TOS(3)   = File number
     */

    case xWRITE_LONGWORD :
      {
        uWord_t lword;

        POP(st, fieldWidth);    /* Field width */
        POP(st, lword.word[1]); /* Write unsigned integer value[1] */
        POP(st, lword.word[0]); /* Write unsigned integer value[0] */
        POP(st, fileNumber);    /* File number from stack */

        errorCode = libexec_WriteLongWord(fileNumber, lword.uData, fieldWidth);
      }
      break;

    /* xWRITE_WORD: TOS(0) = Field width
     *              TOS(1) = Write integer value
     *              TOS(2) = File number
     */

    case xWRITE_WORD :
      POP(st, fieldWidth);  /* Field width */
      POP(st, uValue);      /* Write integer value */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_WriteWord(fileNumber, uValue, fieldWidth);
      break;

    /* WRITE_CHAR: TOS(0) = Field width
     *             TOS(1) = Write value
     *             TOS(2) = File number
     */

    case xWRITE_CHAR :
      POP(st, fieldWidth);  /* Field width */
      POP(st, uValue);      /* Write value */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_WriteChar(fileNumber, (uint8_t)uValue, fieldWidth);
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

      errorCode = libexec_WriteString(fileNumber,
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

      errorCode = libexec_WriteString(fileNumber,
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

      errorCode = libexec_WriteReal(fileNumber, fp.f, fieldWidth);
      break;

    default :
      errorCode = eBADSYSIOFUNC;
      break;
    }

  return errorCode;
}

/****************************************************************************/

const char *libexec_GetFormat(const char *baseFormat, uint8_t fieldWidth,
                              uint8_t precision)
{
  static char fmt[20];

  if (fieldWidth > 0)
    {
      if (precision > 0)
        {
          snprintf(fmt, 20, "%%%u.%u%s", fieldWidth, precision, baseFormat);
        }
      else
        {
          snprintf(fmt, 20, "%%%u%s", fieldWidth, baseFormat);
        }
    }
  else
    {
      snprintf(fmt, 20, "%%%s", baseFormat);
    }

  return fmt;
}
