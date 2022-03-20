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

#include <sys/types.h>
#include <sys/stat.h>

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "pas_debug.h"
#include "config.h"
#include "pas_machine.h"
#include "pas_sysio.h"
#include "pas_errcodes.h"
#include "pas_error.h"

#include "libexec_heap.h"
#include "libexec_longops.h"
#include "libexec_stringlib.h"
#include "libexec_sysio.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

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
static void     libexec_CheckEoln(struct libexec_s *st, uint16_t fileNumber,
                  char *buffer);
static ustack_t libexec_AllocateFile(struct libexec_s *st);
static int      libexec_FreeFile(struct libexec_s *st, uint16_t fileNumber);
static int      libexec_AssignFile(struct libexec_s *st, uint16_t fileNumber,
                  bool text, const char *fileName, uint16_t size);
static int      libexec_OpenFile(struct libexec_s *st, uint16_t fileNumber,
                  openMode_t openMode);
static int      libexec_CloseFile(struct libexec_s *st, uint16_t fileNumber);
static int      libexec_RecordSize(struct libexec_s *st, uint16_t fileNumber,
                  uint16_t size);

static int      libexec_CheckReadAccess(struct libexec_s *st,
                   uint16_t fileNumber);
static int      libexec_ReadLn(struct libexec_s *st, uint16_t fileNumber);
static int      libexec_ReadBinary(struct libexec_s *st, uint16_t fileNumber,
                  uint8_t *dest, uint16_t size);
static int      libexec_ReadInteger(struct libexec_s *st, uint16_t fileNumber,
                  ustack_t *dest);
static int      libexec_ReadChar(struct libexec_s *st, uint16_t fileNumber,
                  uint8_t *dest);
static int      libexec_ReadString(struct libexec_s *st, uint16_t fileNumber,
                  uint16_t *stringVarPtr);
static int      libexec_ReadReal(struct libexec_s *st, uint16_t fileNumber,
                  uint16_t *dest);

static int      libexec_CheckWriteAccess(struct libexec_s *st,
                  uint16_t fileNumber);
static int      libexec_WriteBinary(struct libexec_s *st, uint16_t fileNumber,
                  const uint8_t *src, uint16_t size);
static int      libexec_WriteInteger(struct libexec_s *st, uint16_t fileNumber,
                  int16_t value, uint16_t fieldWidth);
static int      libexec_WriteLongInteger(struct libexec_s *st,
                  uint16_t fileNumber, int32_t value, uint16_t fieldWidth);
static int      libexec_WriteWord(struct libexec_s *st, uint16_t fileNumber,
                  uint16_t value, uint16_t fieldWidth);
static int      libexec_WriteLongWord(struct libexec_s *st,
                  uint16_t fileNumber, uint32_t value, uint16_t fieldWidth);
static int      libexec_WriteChar(struct libexec_s *st, uint16_t fileNumber,
                  uint8_t value, uint16_t fieldWidth);
static int      libexec_WriteReal(struct libexec_s *st, uint16_t fileNumber,
                  double value, uint16_t fieldWidth);
static int      libexec_WriteString(struct libexec_s *st, uint16_t fileNumber,
                  uint16_t allocAddr, uint16_t strSize, uint16_t allocSize,
                  uint16_t fieldWidth);
static int      libexec_Flush(struct libexec_s *st, uint16_t fileNumber);

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

static void libexec_CheckEoln(struct libexec_s *st, uint16_t fileNumber,
                              char *buffer)
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

  st->fileTable[fileNumber].eoln = eoln;
}

/****************************************************************************/

static ustack_t libexec_AllocateFile(struct libexec_s *st)
{
  uint16_t fileNumber;

  for (fileNumber = 0; fileNumber < MAX_OPEN_FILES; fileNumber++)
    {
      if (!st->fileTable[fileNumber].inUse)
        {
          st->fileTable[fileNumber].inUse = true;
          return fileNumber;
        }
    }

  return fileNumber;  /* Return the out-of-range file number */
}

/****************************************************************************/

static int libexec_FreeFile(struct libexec_s *st, uint16_t fileNumber)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (!st->fileTable[fileNumber].inUse)
    {
      errorCode = eFILENOTINUSE;
    }
  else
    {
      /* If the file was opened, then close it */

      if (st->fileTable[fileNumber].stream != NULL)
        {
          libexec_CloseFile(st, fileNumber);
        }

      /* Reset the entire file entry */

      memset(&st->fileTable[fileNumber], 0, sizeof(execFileTable_t));
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_AssignFile(struct libexec_s *st, uint16_t fileNumber,
                              bool text, const char *fileName, uint16_t size)
{
  int errorCode = eNOERROR;

  /* Verify the fileNumber */

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (size > FNAME_SIZE) /* include NUL terminator */
    {
      errorCode = eBADFILENAME;
    }
  else
    {
      /* Copy the file name into the file table */

      memcpy(st->fileTable[fileNumber].fileName, fileName, size);

      /* NUL terminate the fileName so that we can use it like a C string */

      st->fileTable[fileNumber].fileName[size] = '\0';

      /* The set the file type */

      st->fileTable[fileNumber].text = text;
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_OpenFile(struct libexec_s *st, uint16_t fileNumber,
                            openMode_t openMode)
{
  int errorCode = eNOERROR;

  const char *modeString;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (st->fileTable[fileNumber].stream != NULL)
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

      st->fileTable[fileNumber].stream = fopen(st->fileTable[fileNumber].fileName,
                                               modeString);
      if (st->fileTable[fileNumber].stream == NULL)
        {
          errorCode = eOPENFAILED;
        }
      else
        {
          st->fileTable[fileNumber].openMode = openMode;
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_CloseFile(struct libexec_s *st, uint16_t fileNumber)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (st->fileTable[fileNumber].stream == NULL)
    {
      errorCode = eFILENOTOPEN;
    }
  else
    {
      (void)fclose(st->fileTable[fileNumber].stream);
      st->fileTable[fileNumber].stream = NULL;
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_RecordSize(struct libexec_s *st, uint16_t fileNumber,
                              uint16_t size)
{
  int errorCode = eNOERROR;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else
    {
      st->fileTable[fileNumber].recordSize = size;
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_CheckReadAccess(struct libexec_s *st, uint16_t fileNumber)
{
  int errorCode;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (st->fileTable[fileNumber].stream    == NULL ||
           st->fileTable[fileNumber].openMode != eOPEN_READ)
    {
      errorCode = eNOTOPENFORREAD;
    }
  else
    {
      errorCode = eNOERROR;
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_ReadLn(struct libexec_s *st, uint16_t fileNumber)
{
  int errorCode = libexec_CheckReadAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      if (st->fileTable[fileNumber].eoln)
        {
          st->fileTable[fileNumber].eoln = false;
        }
      else
        {
          int ch;

          do
            {
              ch = fgetc(st->fileTable[fileNumber].stream);
            }
          while (ch != EOF && ch != '\n');
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_ReadBinary(struct libexec_s *st, uint16_t fileNumber,
                              uint8_t *dest, uint16_t size)
{
  int errorCode = libexec_CheckReadAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      size_t nitems = fread(dest, 1, size, st->fileTable[fileNumber].stream);
      if (nitems < size && ferror(st->fileTable[fileNumber].stream))
        {
          errorCode = eREADFAILED;
          clearerr(st->fileTable[fileNumber].stream);
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_ReadInteger(struct libexec_s *st, uint16_t fileNumber,
                               ustack_t *dest)
{
  int errorCode = libexec_CheckReadAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      char *ptr = fgets((char *)st->ioBuffer, LINE_SIZE,
                         st->fileTable[fileNumber].stream);

      if (ptr == NULL && ferror(st->fileTable[fileNumber].stream))
        {
          errorCode = eREADFAILED;
          clearerr(st->fileTable[fileNumber].stream);
        }
      else
        {
          libexec_CheckEoln(st, fileNumber, (char *)st->ioBuffer);
          *dest = libexec_ConvertInteger(fileNumber, st->ioBuffer);
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_ReadChar(struct libexec_s *st, uint16_t fileNumber,
                            uint8_t *dest)
{
  int errorCode = libexec_CheckReadAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      char *ptr = fgets((char *)st->ioBuffer, LINE_SIZE,
                        st->fileTable[fileNumber].stream);

      if (ptr == NULL && ferror(st->fileTable[fileNumber].stream))
        {
          errorCode = eREADFAILED;
          clearerr(st->fileTable[fileNumber].stream);
        }
      else
        {
          libexec_CheckEoln(st, fileNumber, (char *)st->ioBuffer);
          *dest = st->ioBuffer[0];
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_ReadString(struct libexec_s *st, uint16_t fileNumber,
                              uint16_t *stringVarPtr)
{
  int errorCode = libexec_CheckReadAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      uint16_t strData;
      uint16_t strAlloc;
      uint16_t strReadSize;
      char    *strPtr;
      char    *ptr;

      strAlloc    = stringVarPtr[BTOISTACK(sSTRING_ALLOC_OFFSET)];
      strReadSize = strAlloc & HEAP_SIZE_MASK;

      strData     = stringVarPtr[BTOISTACK(sSTRING_DATA_OFFSET)];
      strPtr      = (char *)&st->dstack.b[strData];

      ptr         = fgets(strPtr, strReadSize,
                          st->fileTable[fileNumber].stream);

      if (ptr == NULL && ferror(st->fileTable[fileNumber].stream))
        {
          errorCode = eREADFAILED;
          clearerr(st->fileTable[fileNumber].stream);
        }
      else
        {
          libexec_CheckEoln(st, fileNumber, strPtr);
          stringVarPtr[BTOISTACK(sSTRING_SIZE_OFFSET)] = strlen(strPtr);
        }
    }

  return errorCode;
}

static int libexec_ReadReal(struct libexec_s *st, uint16_t fileNumber,
                            uint16_t *dest)
{
  int errorCode = libexec_CheckReadAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      char *ptr = fgets((char*)st->ioBuffer, LINE_SIZE,
                        st->fileTable[fileNumber].stream);

      if (ptr == NULL && ferror(st->fileTable[fileNumber].stream))
        {
          errorCode = eREADFAILED;
          clearerr(st->fileTable[fileNumber].stream);
        }
      else
        {
          libexec_CheckEoln(st, fileNumber, (char *)st->ioBuffer);
          libexec_ConvertReal(dest, st->ioBuffer);
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_CheckWriteAccess(struct libexec_s *st, uint16_t fileNumber)
{
  int errorCode;

  if (fileNumber >= MAX_OPEN_FILES)
    {
      errorCode = eBADFILE;
    }
  else if (st->fileTable[fileNumber].stream    == NULL ||
           (st->fileTable[fileNumber].openMode != eOPEN_WRITE &&
            st->fileTable[fileNumber].openMode != eOPEN_APPEND))
    {
      errorCode = eNOTOPENFORWRITE;
    }
  else
    {
      errorCode = eNOERROR;
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteBinary(struct libexec_s *st, uint16_t fileNumber,
                               const uint8_t *src, uint16_t size)
{
  int errorCode = libexec_CheckWriteAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      ssize_t nitems = fwrite(src, 1, size, st->fileTable[fileNumber].stream);
      if (nitems < 0 && ferror(st->fileTable[fileNumber].stream))
        {
          errorCode = eWRITEFAILED;
          clearerr(st->fileTable[fileNumber].stream);
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteInteger(struct libexec_s *st, uint16_t fileNumber,
                                int16_t value, uint16_t fieldWidth)
{
  int errorCode = libexec_CheckWriteAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      const char *fmt = libexec_GetFormat("d", fieldWidth >> 8, 0);
      int nbytes = fprintf(st->fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteLongInteger(struct libexec_s *st,
                                    uint16_t fileNumber, int32_t value,
                                    uint16_t fieldWidth)
{
  int errorCode = libexec_CheckWriteAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      const char *fmt = libexec_GetFormat(PRId32, fieldWidth >> 8, 0);
      int nbytes = fprintf(st->fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteWord(struct libexec_s *st, uint16_t fileNumber,
                             uint16_t value, uint16_t fieldWidth)
{
  int errorCode = libexec_CheckWriteAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      const char *fmt = libexec_GetFormat("u", fieldWidth >> 8, 0);
      int nbytes = fprintf(st->fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteLongWord(struct libexec_s *st, uint16_t fileNumber,
                                 uint32_t value, uint16_t fieldWidth)
{
  int errorCode = libexec_CheckWriteAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      const char *fmt = libexec_GetFormat(PRIu32, fieldWidth >> 8, 0);
      int nbytes = fprintf(st->fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteChar(struct libexec_s *st, uint16_t fileNumber,
                             uint8_t value, uint16_t fieldWidth)
{
  int errorCode = libexec_CheckWriteAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      const char *fmt = libexec_GetFormat("c", fieldWidth >> 8, 0);
      int nbytes = fprintf(st->fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
          clearerr(st->fileTable[fileNumber].stream);
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteReal(struct libexec_s *st, uint16_t fileNumber,
                             double value, uint16_t fieldWidth)
{
  int errorCode = libexec_CheckWriteAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      const char *fmt = libexec_GetFormat("f", fieldWidth >> 8,
                                          fieldWidth & 0x00ff);
      int nbytes = fprintf(st->fileTable[fileNumber].stream, fmt, value);
      if (nbytes < 0)
        {
          errorCode = eWRITEFAILED;
        }
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_WriteString(struct libexec_s *st, uint16_t fileNumber,
                               uint16_t allocAddr, uint16_t strSize,
                               uint16_t allocSize, uint16_t fieldWidth)

{
  const char *strDataPtr = (const char *)&st->dstack.b[allocAddr];
  int errorCode = libexec_CheckWriteAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      size_t nItems;

      /* Right justify */

      for (fieldWidth >>= 8; fieldWidth > strSize; fieldWidth--)
        {
          fputc(' ', st->fileTable[fileNumber].stream);
        }

      /* Then write the string */

      nItems = fwrite(strDataPtr, 1, strSize,
                      st->fileTable[fileNumber].stream);
      if (nItems < 0 && ferror(st->fileTable[fileNumber].stream))
        {
          errorCode = eWRITEFAILED;
        }

      /* We have consumed the name string container, check if we need to free
       * its string buffer allocation as well.
       */

      libexec_FreeTmpString(st, allocAddr, allocSize);
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_Flush(struct libexec_s *st, uint16_t fileNumber)
{
  int errorCode = libexec_CheckWriteAccess(st, fileNumber);
  if (errorCode == eNOERROR)
    {
      /* Flush the write data */

      fflush(st->fileTable[fileNumber].stream);
    }

  return errorCode;
}

/****************************************************************************/

static int libexec_GetFileSize(FILE *stream, off_t *fileSize)
{
  int errorCode = eNOERROR;
  off_t oldPos  = 0;
  off_t endPos  = 0;
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
  else if (st->fileTable[fileNumber].stream == NULL)
    {
      errorCode = eFILENOTOPEN;
      eof       = PASCAL_TRUE;
    }
  else if (feof(st->fileTable[fileNumber].stream))
    {
      eof = PASCAL_TRUE;
    }

  /* ftell not meaningful on stdin (or pipes or sockets, etc.) */

  else if (fileNumber != 0)
    {
      off_t fileSize;
      off_t filePos;

      /* The C feof() function does not return true until we actually attempt
       * to read past the end-of-file.  We need to check if we are at the end
       * of file even if feof() returns false.
       */

      filePos = ftell(st->fileTable[fileNumber].stream);
      if (filePos < 0 && errno != ESPIPE)
        {
          errorCode = eFTELLFAILED;
          eof       = PASCAL_TRUE;
        }
      else
        {
          errorCode = libexec_GetFileSize(st->fileTable[fileNumber].stream,
                                          &fileSize);
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
      eoln      = st->fileTable[fileNumber].eoln
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
  else if (st->fileTable[fileNumber].stream == NULL)
    {
      errorCode = eFILENOTOPEN;
    }
  else
    {
      off_t pos = ftell(st->fileTable[fileNumber].stream);
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
  else if (st->fileTable[fileNumber].stream == NULL)
    {
      errorCode = eFILENOTOPEN;
    }
  else
    {
      off_t fileSize;

      errorCode = libexec_GetFileSize(st->fileTable[fileNumber].stream,
                                      &fileSize);

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
  else if (st->fileTable[fileNumber].stream == NULL)
    {
      errorCode = eFILENOTOPEN;
    }
  else
    {
      int ret = fseek(st->fileTable[fileNumber].stream, filePos, SEEK_SET);
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
  else if (st->fileTable[fileNumber].stream == NULL)
    {
      errorCode = eFILENOTOPEN;
    }
  else
    {
      for (; ; )
        {
          int ch = fgetc(st->fileTable[fileNumber].stream);
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
  else if (st->fileTable[fileNumber].stream == NULL)
    {
      errorCode = eFILENOTOPEN;
    }
  else
    {
      for (; ; )
        {
          int ch = fgetc(st->fileTable[fileNumber].stream);
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

void libexec_InitializeFile(struct libexec_s *st)
{
  int fileNumber;

  /* Close all open files (except INPUT and OUTPUT). */

  for (fileNumber = 2; fileNumber < MAX_OPEN_FILES; fileNumber++)
    {
      if (st->fileTable[fileNumber].stream != NULL)
        {
          (void)fclose(st->fileTable[fileNumber].stream);
        }
    }

  /* Then Re-initalize INPUT and OUTPUT */

  memset(st->fileTable, 0, MAX_OPEN_FILES * sizeof(execFileTable_t));

  strcpy(st->fileTable[INPUT_FILE_NUMBER].fileName, "INPUT");
  st->fileTable[INPUT_FILE_NUMBER].inUse       = true;
  st->fileTable[INPUT_FILE_NUMBER].text        = true;
  st->fileTable[INPUT_FILE_NUMBER].recordSize  = 1;
  st->fileTable[INPUT_FILE_NUMBER].stream      = stdin;
  st->fileTable[INPUT_FILE_NUMBER].openMode    = eOPEN_READ;

  strcpy(st->fileTable[OUTPUT_FILE_NUMBER].fileName, "OUTPUT");
  st->fileTable[OUTPUT_FILE_NUMBER].inUse      = true;
  st->fileTable[OUTPUT_FILE_NUMBER].text       = true;
  st->fileTable[OUTPUT_FILE_NUMBER].recordSize = 1;
  st->fileTable[OUTPUT_FILE_NUMBER].stream     = stdout;
  st->fileTable[OUTPUT_FILE_NUMBER].openMode   = eOPEN_WRITE;
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
  uint16_t dataSize;
  uint16_t address;
  uint16_t uValue;
  int16_t  sValue;
  int      errorCode = eNOERROR;

  switch (subfunc)
    {
    /* ALLOCFILE: No stack arguments */

    case xALLOCFILE :
       fileNumber = libexec_AllocateFile(st);
       if (fileNumber >= MAX_OPEN_FILES)
         {
           errorCode = eTOOMANYFILES;
         }

       PUSH(st, fileNumber);
       break;

    /* FREEFILE: TOS(0) = File number */

    case xFREEFILE :
       POP(st, fileNumber); /* File number */
       errorCode = libexec_FreeFile(st, fileNumber);
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

    /* ASSIGNFILE: TOS(0) = File name string buffer allocation size
     *             TOS(1) = File name string pointer
     *             TOS(2) = File name string size
     *             TOS(3) = 0:binary 1:textfile
     *             TOS(4) = File number
     */

    case xASSIGNFILE :
      DISCARD(st, 1);       /* Discard the strung buffer allocation size */
      POP(st, address);     /* File name string address */
      POP(st, dataSize);    /* File name string size */
      POP(st, uValue);      /* Binary/text boolean from stack */
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_AssignFile(st, fileNumber, (uValue != 0),
                                     (const char *)&st->dstack.b[address],
                                     dataSize);
      break;

    /* RESET: TOS(0) = File number */

    case xRESET :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_OpenFile(st, fileNumber, eOPEN_READ);
      break;

    /* RESETR: TOS(0) = New record size
     *         TOS(1) = File number
     */

    case xRESETR :
      POP(st, dataSize);    /* New record size */
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_OpenFile(st, fileNumber, eOPEN_READ);
      errorCode = libexec_RecordSize(st, fileNumber, dataSize);
      break;

    /* REWRITE: TOS(0) = File number */

    case xREWRITE :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_OpenFile(st, fileNumber, eOPEN_WRITE);
      break;

    /* RESETR: TOS(0) = New record size
     *         TOS(1) = File number
     */

    case xREWRITER :
      POP(st, dataSize);    /* New record size */
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_OpenFile(st, fileNumber, eOPEN_WRITE);
      errorCode = libexec_RecordSize(st, fileNumber, dataSize);
      break;

    /* APPEND: TOS(0) = File number */

    case xAPPEND :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_OpenFile(st, fileNumber, eOPEN_APPEND);
      break;

    /* CLOSEFILE: TOS(0) = File number */

    case xCLOSEFILE :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_CloseFile(st, fileNumber);
      break;

    /* READLN: TOS(0) = File number */

    case xREADLN :
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_ReadLn(st, fileNumber);
      break;

    /* READ_BINARY: TOS(0) = Read address
     *              TOS(1) = Read size
     *              TOS(2) = File number
     */

    case xREAD_BINARY :
      POP(st, address);     /* Read address */
      POP(st, dataSize);    /* Read size */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_ReadBinary(st, fileNumber,
                                     (uint8_t *)&st->dstack.b[address],
                                     dataSize);
      break;

    /* READ_INT: TOS(0) = Read address
     *           TOS(1) = File number
     */

    case xREAD_INT :

      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_ReadInteger(st, fileNumber,
                                      (ustack_t *)&st->dstack.b[address]);
      break;

    /* READ_CHAR: TOS(0) = Read address
     *            TOS(1) = File number
     */

    case xREAD_CHAR:
      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_ReadChar(st, fileNumber,
                                   (uint8_t *)&st->dstack.b[address]);
      break;

    /* READ_STRING: TOS(0) = String variable address
     *              TOS(1) = File number
     */

    case xREAD_STRING :
      {
        uint16_t *strPtr;

        POP(st, address);     /* String variable address */
        POP(st, fileNumber);  /* File number */

        strPtr    = (uint16_t *)&st->dstack.b[address];
        errorCode = libexec_ReadString(st, fileNumber, strPtr);
      }
      break;

    /* READ_REAL: TOS(0) = Read address
     *            TOS(1) = File number
     */

    case xREAD_REAL :
      POP(st, address);     /* Read address */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_ReadReal(st, fileNumber,
                                   (uint16_t *)&st->dstack.b[address]);
      break;

    /* WRITELN: TOS(0) = File number */

    case xWRITELN :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_WriteChar(st, fileNumber, '\n', 0);
      break;

    /* WRITE_PAGE: TOS(0) = File number */

    case xWRITE_PAGE :
      POP(st, fileNumber);  /* File number from stack */
      errorCode = libexec_WriteChar(st, fileNumber, '\f', 0);
      break;

    /* WRITE_BINARY: TOS(0) = Write size
     *               TOS(1) = Write address
     *               TOS(2) = File number
     */

    case xWRITE_BINARY :
      POP(st, address);     /* Write address */
      POP(st, dataSize);    /* Write size */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_WriteBinary(st, fileNumber,
                                      (const uint8_t *)&st->dstack.b[address],
                                      dataSize);
      break;

    /* WRITE_INT: TOS(0) = Field width
     *            TOS(1) = Write integer value
     *            TOS(2) = File number
     */

    case xWRITE_INT :
      POP(st, fieldWidth);  /* Field width */
      POP(st, sValue);      /* Write integer value */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_WriteInteger(st, fileNumber, sValue, fieldWidth);
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

        errorCode = libexec_WriteLongInteger(st, fileNumber, lword.sData,
                                             fieldWidth);
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

        errorCode = libexec_WriteLongWord(st, fileNumber, lword.uData,
                                          fieldWidth);
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

      errorCode = libexec_WriteWord(st, fileNumber, uValue, fieldWidth);
      break;

    /* WRITE_CHAR: TOS(0) = Field width
     *             TOS(1) = Write value
     *             TOS(2) = File number
     */

    case xWRITE_CHAR :
      POP(st, fieldWidth);  /* Field width */
      POP(st, uValue);      /* Write value */
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_WriteChar(st, fileNumber, (uint8_t)uValue,
                                    fieldWidth);
      break;

    /* WRITE_STRING: TOS(0) = Field width
     *               TOS(1) = Write string allocation (not used)
     *               TOS(2) = Write string buffer address
     *               TOS(3) = Write string size
     *               TOS(4) = File number
     */

    case xWRITE_STRING :
      {
        uint16_t allocSize;

        POP(st, fieldWidth);  /* Field width */
        POP(st, allocSize);   /* String allocation size */
        POP(st, address);     /* String address */
        POP(st, dataSize);    /* String size */
        POP(st, fileNumber);  /* File number */

        errorCode = libexec_WriteString(st, fileNumber, address,
                                        dataSize, allocSize, fieldWidth);
      }
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

      errorCode = libexec_WriteReal(st, fileNumber, fp.f, fieldWidth);
      break;

    /* FLUSH: TOS(0) = File number */

    case xFLUSH :
      POP(st, fileNumber);  /* File number from stack */

      errorCode = libexec_Flush(st, fileNumber);
      break;

    /* CHDIR : TOS(0) = Directory string buffer allocation size
     *         TOS(1) = Directory name string address
     *         TOS(2) = Directory name string size
     * MKDIR : TOS(0) = Directory string buffer allocation size
     *         TOS(1) = Directory name string address
     *         TOS(2) = Directory name string size
     * RMDIR : TOS(0) = Directory string buffer allocation size
     *         TOS(1) = Directory name string address
     *         TOS(2) = Directory name string size
     *
     * All return a boolean value on the stack.
     */

    case xCHDIR :
    case xMKDIR :
    case xRMDIR :
      {
        char *strBuffer;
        char *dirPath;
        uint16_t allocSize;
        uint16_t result;

        /* Get the string argument */

        POP(st, allocSize); /* Buffer allocation size */
        POP(st, address);   /* Directory name string address */
        POP(st, dataSize);  /* Directory name string size */

        /* Convert to a NUL terminated C string */

        strBuffer = (char *)ATSTACK(st, address);

        /* Append A NUL terminator to the string.  Assure that there is
         * a free byte at the end of the string buffer.  If not, then use
         * libexec_MkCString to duplicate the string with the NUL terminator.
         */

        if (dataSize < (allocSize & HEAP_SIZE_MASK))
          {
            strBuffer[dataSize] = '\0';
            dirPath             = strBuffer;
          }
        else
          {
            dirPath = libexec_MkCString(st, strBuffer, dataSize, false);
          }

        if (dirPath == NULL)
          {
            errorCode = eSTRSTKOVERFLOW;
            result = PASCAL_FALSE;
          }
        else if (subfunc == xCHDIR)
          {
            /* Then change directories */

            result = (chdir(dirPath) != 0) ? PASCAL_FALSE : PASCAL_TRUE;
          }
        else if (subfunc == xMKDIR)
          {
            /* Then create the directory */

            result = (mkdir(dirPath, 0755) != 0) ? PASCAL_FALSE : PASCAL_TRUE;
          }
        else /* if (subfunc == xRMDIR) */
          {
            /* Then remove the directory */

            result = (rmdir(dirPath) != 0) ? PASCAL_FALSE : PASCAL_TRUE;
          }

        /* We have consumed the name string container, check if we need to
         * free its string buffer allocation as well.
         */

        libexec_FreeTmpString(st, address, allocSize);

        /* Return the result of the directory operation */

        PUSH(st, result);
      }
      break;

    /* GETDIR : TOS(0) = Address of string variable */

    case xGETDIR :
      /* Get the current working directory  */

      if (getcwd((char *)st->ioBuffer, LINE_SIZE) == NULL)
        {
          errorCode = eGETCWDFAILED;
        }
      else
        {
          char *sptr;
          uint16_t allocAddr;
          uint16_t allocSize;

          /* Convert the C-string into a Pascal string */

          st->ioBuffer[LINE_SIZE] = '\0';
          dataSize = strlen((char *)st->ioBuffer);
          if (dataSize > STRING_BUFFER_SIZE)
            {
              dataSize = STRING_BUFFER_SIZE;
            }

          /* Allocate storage for the temporary string in the string heap */

          allocAddr = libexec_AllocTmpString(st, STRING_BUFFER_SIZE,
                                             &allocSize);
          if (allocAddr == 0)
            {
              errorCode = eNOMEMORY;
            }
          else
            {
              /* Copy the string into the allocated string buffer */

              sptr    = (char *)ATSTACK(st, allocAddr);
              memcpy(sptr, st->ioBuffer, dataSize);

              /* And push the newly create string */

              PUSH(st, dataSize);
              PUSH(st, allocAddr);
              PUSH(st, allocSize);
            }
        }
      break;

    /* OPENDIR : Open a directory for reading.
     *
     *   function OpenDir(DirPath : string; VAR dirInfo: TDir) : boolean
     *
     * ON INPUT:
     *   TOS(0) = Address of DirPath
     *   TOS(1) = Dirpath string buffer allocation size
     *   TOS(2) = DirPath string memory address
     *   TOS(3) = The length of the DirPath string
     * ON RETURN:
     *   TOS(0) = Boolean result of the OpenDir operation
     */

    case xOPENDIR :
      {
        const char *strBuffer;
        char       *dirPath;
        DIR        *dirp;
        uint16_t    dirAddr;
        uint16_t    strAlloc;
        uint16_t    strAddr;
        uint16_t    strSize;
        uint16_t    result;

        /* Get the stack arguments */

        POP(st, dirAddr);
        POP(st, strAlloc);
        POP(st, strAddr);
        POP(st, strSize);

        /* Convert the Pascal dirPath string to a NUL-terminated C-string */

        strBuffer = (const char *)ATSTACK(st, strAddr);
        dirPath   = libexec_MkCString(st, strBuffer, strSize, false);
        dirp      = opendir(dirPath);
        if (dirp == NULL)
          {
            result = PASCAL_FALSE;
          }
        else
          {
            tgtPtr_t  dir;
            uint16_t *dest;
            int i;

            /* Copy the DIR pointer into the TDir containing on the Pascal
             * stack.
             */

            dir.ptr = dirp;
            dest = (uint16_t *)ATSTACK(st, dirAddr);

            for (i = 0; i < PASCAL_POINTERWORDS; i++)
              {
                *dest++ = dir.b[i];
              }

            result = PASCAL_TRUE;
          }

        /* We have consumed the name string container, check if we need to
         * free its string buffer allocation as well.
         */

        libexec_FreeTmpString(st, strAddr, strAlloc);

        /* And return the result of the operation on the stack */

        PUSH(st, result);
      }
      break;

    /* READDIR : Read the next directory entry.
     *
     *   function ReadDir(VAR DirPath : TDir, VAR SearchRec : TSearchRec) : boolean
     *
     * ON INPUT:
     *   TOS(0) = Address of SearchRec
     *   TOS(1) = Address of DirPath
     * ON RETURN:
     *   TOS(0) = Boolean result of the ReadDir operation
     */

    case xREADDIR :
      {
        struct dirent *dirent;
        uint16_t      *stkDir;
        tgtPtr_t       dir;
        uint16_t       searchAddr;
        uint16_t       dirAddr;
        uint16_t       result;
        int            i;

        /* Get the stack addresses */

        POP(st, searchAddr);
        POP(st, dirAddr);

        /* Get the pointer from the stack (it may not be aligned) */

        stkDir = (uint16_t *)ATSTACK(st, dirAddr);
        for (i = 0; i < PASCAL_POINTERWORDS; i++)
          {
            dir.b[i] = *stkDir++;
          }

        /* Do the readdir operation */

        dirent = readdir(dir.ptr);
        if (dirent == NULL)
          {
            result = PASCAL_FALSE;
          }
        else
          {
            searchRec_t *searchRec; /* Search result record pointer */
            char        *aStrPtr;   /* File name string allocation pointer */
            int          allocSize;
            int          copySize;
            int          index;

            /* Get a C pointer to the TSearchRec instance on the Pascal stack */

            searchRec = (searchRec_t *)ATSTACK(st, searchAddr);

            /* Copy the file name and type from the dirent structure */

            copySize  = strnlen(dirent->d_name, NAME_MAX);
            index     = sSTRING_ALLOC_OFFSET / sINT_SIZE;
            allocSize = searchRec->name[index];

            if (copySize > allocSize)
              {
                copySize = allocSize;
              }

            index     = sSTRING_DATA_OFFSET / sINT_SIZE;
            aStrPtr   = (char *)ATSTACK(st, searchRec->name[index]);
            memcpy(aStrPtr, dirent->d_name, copySize);

            index     = sSTRING_SIZE_OFFSET / sINT_SIZE;
            searchRec->name[index] = copySize;

            /* Convert the dirent file type into a FileUtils file type */

            if (dirent->d_type == DT_REG)
              {
                searchRec->attr = 0;
              }
            else if (dirent->d_type == DT_DIR)
              {
                searchRec->attr = faDirectory;
              }
            else
              {
                searchRec->attr = faSysFile;
              }

            /* Under a POSIX file system, faHidden means that the filename
             * begins with a dot.
             */

            if (dirent->d_name[0] == '.')
              {
                searchRec->attr |= faHidden;
              }

            result = PASCAL_TRUE;
          }

        PUSH(st, result);
      }
      break;

    /* Get information about a file.  This function will populate the 'size'
     * and 'time'  fields of the `TSearchRec` record.
     *
     *   function FileInfo(FilePath : string; VAR SearchRec : TSearchRec) : boolean
     *
     * ON INPUT:
     *   TOS(0) = Address of SearchRec
     *   TOS(1) = Size of FilePath allocation
     *   TOS(2) = Address of FilePath string
     *   TOS(3) = Length of the FilePath string
     * ON RETURN:
     *   TOS(0) = Boolean result of the FileInfo operation
     */

    case xFILEINFO :
      {
        struct stat  statBuf;
        char        *filePathPtr;
        uint16_t     searchRecAddr;
        uint16_t     filePathAlloc;
        uint16_t     filePathAddr;
        uint16_t     filePathLength;
        uint16_t     result;
        int          ret;

        /* Get the stack addresses */

        POP(st, searchRecAddr);
        POP(st, filePathAlloc);
        POP(st, filePathAddr);
        POP(st, filePathLength);

        /* Convert to a NUL terminated C string by appending A NUL terminator
         * to the end of string.  Assure that there is a free byte at the end
         * of the string buffer.  If not, then use libexec_MkCString to
         * duplicate the string with the NUL terminator.
         */

        filePathPtr = (char *)ATSTACK(st, filePathAddr);

        if (filePathLength < (filePathAlloc & HEAP_SIZE_MASK))
          {
            filePathPtr[filePathLength] = '\0';
          }
        else
          {
            filePathPtr = libexec_MkCString(st, filePathPtr, filePathLength,
                                            false);
          }

        /* Do the stat operation */

        ret = stat(filePathPtr, &statBuf);
        if (ret < 0)
          {
            result = PASCAL_FALSE;
          }
        else
          {
            searchRec_t *resultPtr; /* Search result record pointer */
            mapInt32_t   map;

            /* Get a C pointer to the TSearchRec instance on the Pascal stack */

            resultPtr = (searchRec_t *)ATSTACK(st, searchRecAddr);

            /* Copy the last modification date/time and file size from the
             * statBuf structure.
             */

            map.u32            = (uint32_t)statBuf.st_mtime;
            resultPtr->time[0] = map.u16[0];
            resultPtr->time[1] = map.u16[1];

            /* Size only makes sense for regular files */

            if (S_ISREG(statBuf.st_mode))
              {
                map.u32            = (uint32_t)statBuf.st_size;
                resultPtr->size[0] = map.u16[0];
                resultPtr->size[1] = map.u16[1];
              }

            result = PASCAL_TRUE;
          }

        /* We have consumed the name string container, check if we need to
         * free its string buffer allocation as well.
         */

        libexec_FreeTmpString(st, filePathAddr, filePathAlloc);

        /* And return the result of the operation on the stack */

        PUSH(st, result);
      }
      break;

    /* REWINDDIR : Reset the read position of the beginning of the directory.
     * CLOSEDIR : Close the directory and release any resources.
     *
     *   function RewindDir(VAR dirInfo : TDir) : boolean
     *   function CloseDir(VAR dirInfo : TDir) : boolean
     *
     * ON INPUT:
     *   TOS(0) = Address of dirInfo
     * ON RETURN:
     *   TOS(0) = Boolean result of the RewindDir operation
     */

    case xCLOSEDIR :
    case xREWINDDIR :
      {
        tgtPtr_t       dir;
        uint16_t      *stkDir;
        uint16_t       dirAddr;
        uint16_t       result;
        int            i;

        /* Get the stack address */

        POP(st, dirAddr);

        /* Get the pointer from the stack (it may not be aligend) */

        stkDir = (uint16_t *)ATSTACK(st, dirAddr);
        for (i = 0; i < PASCAL_POINTERWORDS; i++)
          {
            dir.b[i] = *stkDir++;
          }

        if (subfunc == xCLOSEDIR)
          {
            int ret = closedir(dir.ptr);
            result = (ret == 0) ? PASCAL_TRUE : PASCAL_FALSE;
          }
        else /* if (subfunc == xCLOSEDIR) */
          {
            rewinddir(dir.ptr);
            result = PASCAL_TRUE;
          }

        PUSH(st, result);
      }
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
