/****************************************************************************
 * pas_stdfunc.c
 * Standard Functions
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <strings.h>

#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_pcode.h"
#include "pas_fpops.h"
#include "pas_errcodes.h"
#include "pas_sysio.h"
#include "pas_stringlib.h"
#include "pas_oslib.h"

#include "pas_main.h"
#include "pas_expression.h"
#include "pas_procedure.h"
#include "pas_initializer.h" /* for finalizer functions */
#include "pas_function.h"    /* for pas_StandardFunction() */
#include "pas_setops.h"      /* Set operation codes */
#include "pas_codegen.h"     /* for pas_Generate*() */
#include "pas_token.h"
#include "pas_symtable.h"
#include "pas_insn.h"
#include "pas_error.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Standard Pascal Functions */

static exprType_t pas_AbsFunc(void);      /* Integer absolute value */
static exprType_t pas_AddrFunc(void);     /* Address of variable, proc, func */
static exprType_t pas_PredFunc(void);
static void       pas_OrdFunc(void);      /* Convert scalar to integer */
static exprType_t pas_SqrFunc(void);
static void       pas_RealFunc(uint8_t fpCode);
static exprType_t pas_SuccFunc(void);
static void       pas_OddFunc(void);
static void       pas_ChrFunc(void);
static void       pas_FileFunc(uint16_t opCode);
static void       pas_FilePosFunc(uint16_t opCode);
static void       pas_SetFunc(uint16_t setOpcode);
static void       pas_CardFunc(void);

/* Borland style string operations */

static void       pas_LengthFunc(void);
static void       pas_CopyFunc(void);
static void       pas_PosFunc(void);
static void       pas_ConcatFunc(void);

/* Borland style directory operations */

static void       pas_DirectoryFunc(uint16_t opCode);
static void       pas_GetDirFunc(void);

static void       pas_ParseDirEntry(void);
static void       pas_ParseDirSearchRec(void);
static void       pas_OpenDirFunc(void);
static void       pas_ReadDirFunc(void);
static void       pas_ReadDirectoryFunc(uint16_t opCode);

/* Non-standard C-library interface functions */

static void       pas_CharAtFunc(void);
static void       pas_SpawnFunc(void);
static exprType_t pas_GetEnvFunc (void);  /* Get environment string value */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/

static exprType_t pas_AbsFunc(void)
{
  exprType_t absType;

  /* FORM:  ABS (<simple integer/real expression>) */

  pas_CheckLParen();

  absType = pas_Expression(exprUnknown, NULL);
  if (absType == exprInteger)
    {
      pas_GenerateSimple(opABS);
    }
  else if (absType == exprReal)
    {
      pas_GenerateFpOperation(fpABS);
    }
  else
    {
      error(eINVARG);
    }

  pas_CheckRParen();
  return absType;
}

/****************************************************************************/

static exprType_t pas_AddrFunc(void)
{
  exprType_t addrExprType = exprUnknown;

  /* FORM:  'addr' '(' variable-name | procedure-name | function-name ')'
   *
   * Non-standard.  Like the @ except that the retuned value is an untyped
   * address (exprAnyPointer).
   */

  pas_CheckLParen();

  /* Check for variable-name | procedure-name | function-name */

  switch (g_token)
    {
      /* Procedures and Functions (and labels) */

      case sPROC :
      case sFUNC :
      case sLABEL :
        error(eNOTYET);
        break;

      /* Variables */

      case sFILE :
      case sTEXTFILE :
      case sINT :
      case sWORD :
      case sSHORTINT :
      case sSHORTWORD :
      case sLONGINT :
      case sLONGWORD :
      case sBOOLEAN :
      case sCHAR :
      case sREAL :
      case sSTRING :
      case sPOINTER :
      case sSCALAR :
      case sSUBRANGE :
      case sSET :
      case sARRAY :
      case sRECORD :
      case sVAR_PARM :
        pas_GenerateStackReference(opLAS, g_tknPtr);
        addrExprType = exprAnyPointer;
        break;

      default :
        error(eINVARG);
        break;
    }

  getToken();
  pas_CheckRParen();
  return addrExprType;
}

/****************************************************************************/

static void pas_OrdFunc(void)
{
  /* FORM:  ORD (<scalar type>) */

  pas_CheckLParen();
  pas_Expression(exprAnyOrdinal, NULL);     /* Get any ordinal type */
  pas_CheckRParen();
}

/****************************************************************************/

static exprType_t pas_PredFunc(void)
{
  exprType_t predType;

  /* FORM:  PRED (<simple integer expression>) */

  pas_CheckLParen();

  /* Process any ordinal expression */

  predType = pas_Expression(exprAnyOrdinal, NULL);
  pas_CheckRParen();
  pas_GenerateSimple(opDEC);
  return predType;
}

/****************************************************************************/

static exprType_t pas_SqrFunc(void)
{
  exprType_t sqrType;

  /* FORM:  SQR (<simple integer OR real expression>) */

  pas_CheckLParen();

  sqrType = pas_Expression(exprUnknown, NULL); /* Process any expression */
  if (sqrType == exprInteger || sqrType == exprShortInteger)
    {
      pas_GenerateSimple(opDUP);
      pas_GenerateSimple(opMUL);
     }
  else if (sqrType == exprWord || sqrType == exprShortWord)
    {
      pas_GenerateSimple(opDUP);
      pas_GenerateSimple(opUMUL);
     }
  else if (sqrType == exprReal)
    {
      pas_GenerateFpOperation(fpSQR);
    }
  else
    {
      error(eINVARG);
    }

  pas_CheckRParen();
  return sqrType;
}

/****************************************************************************/

static void pas_RealFunc (uint8_t fpOpCode)
{
   exprType_t realType;

   /* FORM:  <function identifier> (<real/integer expression>) */

   pas_CheckLParen();

   realType = pas_Expression(exprUnknown, NULL); /* Process any expression */
   if (realType == exprInteger)
     {
       pas_GenerateFpOperation((fpOpCode | fpARG1));
     }
   else if (realType == exprReal)
     {
       pas_GenerateFpOperation(fpOpCode);
     }
   else
     {
       error(eINVARG);
     }

   pas_CheckRParen();
}

/****************************************************************************/

static exprType_t pas_SuccFunc(void)
{
  exprType_t succType;

  /* FORM:  SUCC (<simple integer expression>) */

  pas_CheckLParen();

  /* Process any ordinal expression */

  succType = pas_Expression(exprAnyOrdinal, NULL);

  pas_CheckRParen();
  pas_GenerateSimple(opINC);
  return succType;
}

/****************************************************************************/

static void pas_OddFunc(void)
{
  /* FORM:  ODD (<simple integer expression>) */

  pas_CheckLParen();

  /* Process any ordinal expression */

  pas_Expression(exprAnyOrdinal, NULL);
  pas_CheckRParen();
  pas_GenerateDataOperation(opPUSH, 1);
  pas_GenerateSimple(opAND);
  pas_GenerateSimple(opNEQZ);
}

/****************************************************************************/
/* Process the standard chr function */

static void pas_ChrFunc(void)
{
  /* Form:  chr(integer expression).
   *
   * char(val) is only defined if there exists a character ch such
   * that ord(ch) = val.  If this is not the case, we will simply
   * let the returned value exceed the range of type char. */

  pas_CheckLParen();
  pas_Expression(exprInteger, NULL);
  pas_CheckRParen();
}

/****************************************************************************/
/* EOF/EOLN function */

static void pas_FileFunc(uint16_t opCode)
{
  /* FORM: function Eof(var t : TextFile) : Boolean;
   *       function Eof : Boolean;
   * FORM: function Eoln(var t : TextFile) : Boolean;
   *       function Eoln(var t : File of <type>) : Boolean;
   *       function Eoln : Boolean;
   * FORM: function SeekEOF(var t : TextFile) : Boolean;
   *       function SeekEOF : Boolean;
   * FORM: function SeekEOLn(var t : TextFile) : Boolean;
   *       function SeekEOLn : Boolean;
   *
   * The optional <file number> parameter is a reference to a file variable.
   * If the optional parameter is supplied then the eof function tests the
   * file associated with the parameter. If the optional parameter is not
   * supplied then the file associated with the built-in variable input is
   * used.
   */

  getToken();          /* Skip over function name */
  if (g_token == '(')  /* Check for '(' */
    {
      /* Push the file number argument on the stack */

      getToken();
      (void)pas_GenerateFileNumber(NULL, g_inputFile);

      /* FORM: EOF|EOLN ({<file number>}) */
      /* Generate the file operation */

      pas_GenerateIoOperation(opCode);
      pas_CheckRParen();
    }
  else
    {
      /* FORM: EOF|EOLN
       * Use default INPUT file
       */

      pas_GenerateStackReference(opLDS, g_inputFile);
      pas_GenerateIoOperation(opCode);
    }
}

/****************************************************************************/
/* Get position in file */

static void pas_FilePosFunc(uint16_t opCode)
{
  uint32_t fileType;

  /* FORM: function FileSize(var f : file) : Int64;
   * FORM: function FilePos(var f : file) : Int64;
   *
   * If the parameter 'f' is not optional.
   */

  /* Verify that the argument list is enclosed in parentheses */

  pas_CheckLParen();

  /* Push the file number argument on the stack */

  getToken();
  fileType = pas_GenerateFileNumber(NULL, NULL);
  if (fileType == sFILE)
    {
      /* These should not be used with TEXTFILE types */

      if (fileType == sTEXTFILE) warn(eINVFILE);
      else error(eINVFILE);
    }

  /* Then generate the I/O operation */

  pas_GenerateIoOperation(opCode);

  /* Assure that the parameter list terminates with a right parenthesis. */

  pas_CheckRParen();
}

/****************************************************************************/

static void pas_SetFunc(uint16_t setOpcode)
{
  exprType_t memberExprType;
  symbol_t  *baseTypePtr;
  uint16_t   baseType;

  /* FORM: 'include' | 'exclude' '(' set-expression, set-member ')' */

  /* Verify that the argument list is enclosed in parentheses */

  pas_CheckLParen();

  /* Get the SET expression */

  pas_Expression(exprSet, NULL);

  /* Verify the presence of the comma separating the parameters */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Successful parsing of a SET expression should have the side-effect of
   * setting g_abstractTypePtr, the type of the SET expression (the full type,
   * not the base type).
   *
   * The base type is probably a SET.  So we will need the child subrange
   * which will tell us the "Subrange of what?"
   */

  baseTypePtr     = pas_GetBaseTypePointer(g_abstractTypePtr);
  baseType        = baseTypePtr->sParm.t.tType;

  if (baseType == sSET)
    {
      baseTypePtr = baseTypePtr->sParm.t.tParent;
      baseType    = baseTypePtr->sParm.t.tType;
    }

  if (baseType == sSUBRANGE)
    {
      baseType   = baseTypePtr->sParm.t.tSubType;
    }

  memberExprType = pas_MapVariable2ExprType(baseType, true);

  /* The set-member argument should then be a value of that type */

  pas_Expression(memberExprType, g_abstractTypePtr);

  /* Make the set-member value zero base */

  if (baseTypePtr->sParm.t.tMinValue != 0)
    {
      pas_GenerateDataOperation(opPUSH, baseTypePtr->sParm.t.tMinValue);
      pas_GenerateSimple(opSUB);
    }

  /* Now we can generate the set operation */

  pas_GenerateSetOperation(setOpcode);

  /* Assure that the parameter list terminates with a right parenthesis. */

  pas_CheckRParen();
}

/****************************************************************************/

static void pas_CardFunc(void)
{
  /* FORM: 'card' '(' set-expression ')' */

  /* Verify that the argument list is enclosed in parentheses */

  pas_CheckLParen();

  /* Get the SET expression */

  pas_Expression(exprSet, NULL);

  /* Now we can generate the set operation */

  pas_GenerateSetOperation(setCARD);

  /* Assure that the parameter list terminates with a right parenthesis. */

  pas_CheckRParen();
}

/****************************************************************************/

static void pas_LengthFunc(void)
{
  exprType_t exprType;

  /* FORM:  length '(' string-expression ')' */

  pas_CheckLParen();

  /* Process the string-expression */

  exprType = pas_Expression(exprString, NULL);
  if (exprType == exprString)
    {
      /* The top of the stack now holds:
       *
       *   TOS(0) - String buffer allocation
       *   TOS(1) - String buffer address
       *   TOS(2) - String length
       *
       * We need to discard the buffer address and buffer allocation, leaving
       * the string length on the stack.  But we also need to free any possible heap allocations used by the string.  The STRLEN string library function will have to handle that.
       */

      pas_StringLibraryCall(lbSTRLEN);
    }
  else
    {
      error(eSTRING);
    }

  pas_CheckRParen();
}

/****************************************************************************/
/* Copy(from : string, from, howmuch: integer) : string - Get a substring
 * from a string.
 */

static void pas_CopyFunc(void)
{
  /* FORM: 'copy' '(' string-expression ',' integer-expression ','
   *        integer-expression ')'
   */

  /* Verify that the argument list is enclosed in parentheses */

  pas_CheckLParen();

  /* Get the string expression */

  pas_Expression(exprString, NULL);

  /* A comma should separate the arguments */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Get the first integer expression */

  pas_Expression(exprInteger, NULL);

  /* A comma should separate the arguments */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Get the second integer expression */

  pas_Expression(exprInteger, NULL);

  /* Now we can generate the string operation */

  pas_StringLibraryCall(lbCOPYSUBSTR);

  /* Assure that the parameter list terminates with a right parenthesis. */

  pas_CheckRParen();
}

/****************************************************************************/
/* Pos(substr, s : string) : integer - Get the position of a substring from
 * a string. If the substring is not found, it returns 0.
 */

static void pas_PosFunc(void)
{
  /* FORM: 'pos' '(' string-expression ',' string-expression [',' integer-expression ')' */

  /* Verify that the argument list is enclosed in parentheses */

  pas_CheckLParen();

  /* Get the first string expression */

  pas_Expression(exprString, NULL);

  /* A comma should separate the arguments */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Get the second string expression */

  pas_Expression(exprString, NULL);

  /* The second string may be followed by an optional start offset */

  if (g_token == ',')
    {
      getToken();
      pas_Expression(exprInteger, NULL);
    }
  else
    {
      /* Otherwise, start at positioni 1 */

      pas_GenerateDataOperation(opPUSH, 1);
    }

  /* Now we can generate the string operation */

  pas_StringLibraryCall(lbFINDSUBSTR);

  /* Assure that the parameter list terminates with a right parenthesis. */

  pas_CheckRParen();
}

/****************************************************************************/
/* concatconcat(s1,s2,...,sn : string) : string ??? Concatenate two or more strings. */

static void pas_ConcatFunc(void)
{
  exprType_t exprType;
  uint16_t opCode;

  /* FORM: 'concat' '(' string-list')'
   *       string-list = string-expression [',' string-list ]
   */

  /* Verify that the argument list is enclosed in parentheses */

  pas_CheckLParen();

  /* Create an empty standard string to catch the result of the concatenation */

  pas_StringLibraryCall(lbSTRTMP);

  for (; ; )
    {
      /* Get the next string expression */

      exprType = pas_Expression(exprString, NULL);

      /* Concatenate this string to the temporary stack string */

      if (exprType == exprString)
        {
          opCode = lbSTRCAT;
        }
      else
        {
          error(eEXPRTYPE);
        }

      pas_StringLibraryCall(opCode);

      /* A comma following the string means that there is another string to
       * be concatenated.  Continue looping.
       */

      if (g_token != ',') break;
      else getToken();
    }

  /* Assure that the parameter list terminates with a right parenthesis. */

  pas_CheckRParen();
}

/****************************************************************************/
/* Set current working directory */

static void pas_DirectoryFunc(uint16_t opCode)
{
  /* FORM: 'setcurrentdir | createdir | removedir' '(' string-expression ')' */

  /* Verify that the argument list is enclosed in parentheses */

  pas_CheckLParen();

  /* Get the string expression */

  pas_Expression(exprString, NULL);

  /* Now we can generate the directory operation */

  pas_GenerateIoOperation(opCode);

  /* Assure that the parameter list terminates with a right parenthesis. */

  pas_CheckRParen();
}

/****************************************************************************/
/* Get the current working directory */

static void pas_GetDirFunc(void)
{
  bool lparen;

  /* FORM: 'getdir' [ '()' ] : string */

  /* Left parenthesis is optional */

  getToken();

  lparen = (g_token == '(');
  if (lparen) getToken();

  /* Generate the directory operation */

  pas_GenerateIoOperation(xGETDIR);

  /* A right parenthis is required only if a left parenthesis was provided. */

  if (lparen)
    {
      if (g_token != ')') error(eRPAREN);
      else getToken();
    }
}

/****************************************************************************/
/* Shared routine to parse a TDir VAR argument */

static void pas_ParseDirEntry(void)
{
  symbol_t *wordTypePtr;
  exprType_t exprType;

  /* Parse the VAR TDir parameter.
   *
   * Verify that the current token is instance of a Array named TDir.  This
   * may also be a pointer to TDir or VAR parameter of type TDir.  The TDir
   * instance could also be an element of an array or a field of a record.
   */

  /* Get the TDir type symbol */

  wordTypePtr = pas_FindSymbol("TDir", 0, NULL);
  if (wordTypePtr == NULL || wordTypePtr->sKind != sTYPE)
    {
      error(eHUH);
    }

  /* Then parse the VAR TDir parameter.  We expect to have a pointer to an
   * array of WORD.
   */

  exprType = pas_VarParameter(exprWordPtr, wordTypePtr);

  /* If pas_Expression was successful, we can be assured that the address
   * of an instance of type tDir was pushed.
   */

  if (exprType != exprWordPtr) error(eARRAYTYPE);
}

/****************************************************************************/
/* Shared routine to parse a TSearchRec VAR argument */

static void pas_ParseDirSearchRec(void)
{
  symbol_t *srecTypePtr;
  exprType_t exprType;

  /* Verify that the current token is instance of a RECORD named TSearchRec.
   * This may also be a pointer to TSearchRec or VAR parameter of type
   * TSearchRec.  The TDir instance could also be an element of an array or
   * a field of a record.
   */

  /* Get the TDir type symbol */

  srecTypePtr = pas_FindSymbol("TSearchRec", 0, NULL);
  if (srecTypePtr == NULL || srecTypePtr->sKind != sTYPE)
    {
      error(eHUH);
    }

  /* Then parse the VAR TSearchRec parameter.  We expect to have a pointer to an
   * array of WORD.
   */

  exprType = pas_VarParameter(exprRecordPtr, srecTypePtr);

  /* If pas_Expression was successful, we can be assured that the address
   * of an instance of type TSearchRec was pushed.
   */

  if (exprType != exprRecordPtr) error(eRECORDTYPE);
}

/****************************************************************************/
/* Open a directory for reading. */

static void pas_OpenDirFunc(void)
{
  /* FORM: 'opendir' '(' string-expression ',' direntry-variable ')' */

  /* Verify that the argument list is enclosed in parentheses */

  pas_CheckLParen();

  /* Get the string expression */

  pas_Expression(exprString, NULL);

  /* Verify that the two arguments are separated with a comma */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Parse the VAR TDir parameter. */

  pas_ParseDirEntry();

  /* Generate the directory operation */

  pas_GenerateIoOperation(xOPENDIR);

  /* Assure that the parameter list terminates with a right parenthesis. */

  pas_CheckRParen();
}

/****************************************************************************/
/*  Read the next directory entry. */

static void pas_ReadDirFunc(void)
{
  /* FORM: 'readdir' '(' direntry-variable ',' search-result-variable ')' */

  /* Verify that the argument list is enclosed in parentheses */

  pas_CheckLParen();

  /* Parse the VAR TDir parameter. */

  pas_ParseDirEntry();

  /* Verify that the two arguments are separated with a comma */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Parse the VAR TSearchRec parameter */

  pas_ParseDirSearchRec();

  /* Generate the directory operation */

  pas_GenerateIoOperation(xREADDIR);

  /* Assure that the parameter list terminates with a right parenthesis. */

  pas_CheckRParen();
}

/****************************************************************************/
/*  Get information about a file. */

static void pas_FileInfoFunc(void)
{
  /* FORM: 'fileinfo' '(' string-expression ',' search-result-variable ')' */

  /* Verify that the argument list is enclosed in parentheses */

  pas_CheckLParen();

  /* Get the directory path string. */

  pas_Expression(exprString, NULL);

  /* Verify that the two arguments are separated with a comma */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Parse the VAR TSearchRec parameter */

  pas_ParseDirSearchRec();

  /* Generate the directory operation */

  pas_GenerateIoOperation(xFILEINFO);

  /* Assure that the parameter list terminates with a right parenthesis. */

  pas_CheckRParen();
}

/****************************************************************************/
/* Reset the read position of the beginning of the directory. */

static void pas_ReadDirectoryFunc(uint16_t opCode)
{
  /* FORM: 'rewinddir|closedir' '(' direntry-variable ')' */

  /* Verify that the argument list is enclosed in parentheses */

  pas_CheckLParen();

  /* Parse the VAR TDir parameter. */

  pas_ParseDirEntry();

  /* Generate the directory operation */

  pas_GenerateIoOperation(opCode);

  /* Assure that the parameter list terminates with a right parenthesis. */

  pas_CheckRParen();
}

/****************************************************************************/
/* Extract a character from a string */

static void pas_CharAtFunc(void)
{
  /* FORM:  charat '(' string-expression ',' integer-expression ')' */

  pas_CheckLParen();

  /* Get the string expression */

  pas_Expression(exprString, NULL);

  /* Verify that the two arguments are separated with a comma */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Get the integer expression (position in the string) */

  pas_Expression(exprInteger, NULL);

  pas_StringLibraryCall(lbCHARAT);
  pas_CheckRParen();
}

/****************************************************************************/
/* Spawn a Pascal task */

static void pas_SpawnFunc(void)
{
  /* FORM:  function Spawn(PexFileName : string; StringBufferAlloc,
   *                       HeapAlloc : integer; Wait, Debug : boolean) :
   *                       boolean
   */

  pas_CheckLParen();

  /* Get the string expression representing the .pex filename. */

  pas_Expression(exprString, NULL);

  /* Get the string buffer allocation */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  pas_Expression(exprInteger, NULL);

  /* Get the heap allocation */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  pas_Expression(exprInteger, NULL);

  /* Get the Wait boolean value */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  pas_Expression(exprBoolean, NULL);

  /* Get the Debug boolean value */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  pas_Expression(exprBoolean, NULL);

  /* And generate the OS call */

  pas_OsInterfaceCall(osSPAWN);
  pas_CheckRParen();
}

/****************************************************************************/
/* C library getenv interface */

static exprType_t pas_GetEnvFunc(void)
{
  exprType_t stringType;

  /* FORM:  <value-string> = getenv(<name-string>) */

  pas_CheckLParen();

  /* Get the string expression representing the environment variable name. */

  stringType = pas_Expression(exprString, NULL);

  /* Any expression other then 'exprString' would be an error. */

  if (stringType != exprString)
    {
      error(eINVARG);
    }

  pas_OsInterfaceCall(osGETENV);
  pas_CheckRParen();
  return exprString;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/
/* Process a standard Pascal function call */

exprType_t pas_StandardFunction(void)
{
  exprType_t funcType = exprUnknown;

  /* Is the token a function? */

  if (g_token == tSTDFUNC)
    {
      /* Yes, process it procedure according to the extended token type */

      switch (g_tknSubType)
        {
          /* Functions which return the same type as their argument */

        case txABS :
          funcType = pas_AbsFunc();
          break;

        case txADDR :
          funcType = pas_AddrFunc();
          break;

        case txSQR :
          funcType = pas_SqrFunc();
          break;

        case txPRED :
          funcType = pas_PredFunc();
          break;

        case txSUCC :
          funcType = pas_SuccFunc();
          break;

        case txSIZEOF :
          pas_SizeOfFunc();
          funcType = exprInteger;
          break;

          /* Borland-style string operations */

        case txLENGTH :
          pas_LengthFunc();
          funcType = exprInteger;
          break;

        case txCOPY :
          pas_CopyFunc();
          funcType = exprString;
          break;

        case txPOS :
          pas_PosFunc();
          funcType = exprInteger;
          break;

        case txCONCAT :
          pas_ConcatFunc();
          funcType = exprString;
          break;

          /* Borland-style directory operations */

        case txSETCURRDIR :
          pas_DirectoryFunc(xCHDIR);
          funcType = exprBoolean;
          break;

        case txCREATEDIR :
          pas_DirectoryFunc(xMKDIR);
          funcType = exprBoolean;
          break;

        case txREMOVEDIR :
          pas_DirectoryFunc(xRMDIR);
          funcType = exprBoolean;
          break;

        case txGETDIR :
          pas_GetDirFunc();
          funcType = exprString;
          break;

        case txOPENDIR :
          pas_OpenDirFunc();
          funcType = exprBoolean;
          break;

        case txREADDIR :
          pas_ReadDirFunc();
          funcType = exprBoolean;
          break;

        case txFILEINFO :
          pas_FileInfoFunc();
          funcType = exprBoolean;
          break;

        case txREWINDDIR :
          pas_ReadDirectoryFunc(xREWINDDIR);
          funcType = exprBoolean;
          break;

        case txCLOSEDIR :
          pas_ReadDirectoryFunc(xCLOSEDIR);
          funcType = exprBoolean;
          break;

          /* Non-standard C library interfaces */

        case txCHARAT :
          pas_CharAtFunc();
          funcType = exprChar;
          break;

        case txSPAWN :
          pas_SpawnFunc();
          funcType = exprBoolean;
          break;

        case txGETENV :
          funcType = pas_GetEnvFunc();
          break;

          /* Functions returning INTEGER with REAL arguments */

        case txROUND :
          getToken();                          /* Skip over 'round' */
          pas_Expression(exprReal, NULL);
          pas_GenerateFpOperation(fpROUND);
          funcType = exprInteger;
          break;

        case txTRUNC :
          getToken();                          /* Skip over 'trunc' */
          pas_Expression(exprReal, NULL);
          pas_GenerateFpOperation(fpTRUNC);
          funcType = exprInteger;
          break;

          /* Functions returning CHARACTER with INTEGER arguments. */

        case txCHR :
          pas_ChrFunc();
          funcType = exprChar;
          break;

          /* Function returning integer with scalar arguments */

        case txORD :
          pas_OrdFunc();
          funcType = exprInteger;
          break;

          /* Functions returning BOOLEAN */

        case txODD :
          pas_OddFunc();
          funcType = exprBoolean;
          break;

        case txEOF :
          pas_FileFunc(xEOF);
          funcType = exprBoolean;
          break;

        case txEOLN :
          pas_FileFunc(xEOLN);
          funcType = exprBoolean;
          break;

        case txSEEKEOF :
          pas_FileFunc(xSEEKEOF);
          funcType = exprBoolean;
          break;

        case txSEEKEOLN :
          pas_FileFunc(xSEEKEOLN);
          funcType = exprBoolean;
          break;

        case txFILEPOS :
          pas_FilePosFunc(xFILEPOS);
          funcType = exprInt64;
          break;

        case txFILESIZE :
          pas_FilePosFunc(xFILESIZE);
          funcType = exprInt64;
          break;

          /* Functions returning REAL with REAL/INTEGER arguments */

        case txSQRT :
          pas_RealFunc(fpSQRT);
          funcType = exprReal;
          break;

        case txSIN :
          pas_RealFunc(fpSIN);
          funcType = exprReal;
          break;

        case txCOS :
          pas_RealFunc(fpCOS);
          funcType = exprReal;
          break;

        case txARCTAN :
          pas_RealFunc(fpATAN);
          funcType = exprReal;
          break;

        case txLN :
          pas_RealFunc(fpLN);
          funcType = exprReal;
          break;

        case txEXP :
          pas_RealFunc(fpEXP);
          funcType = exprReal;
          break;

          /* Set operations */

        case txINCLUDE :
          pas_SetFunc(setINCLUDE);
          funcType = exprSet;
          break;

        case txEXCLUDE :
          pas_SetFunc(setEXCLUDE);
          funcType = exprSet;
          break;

        case txCARD :
          pas_CardFunc();
          funcType = exprInteger;
          break;

        default :
          error(eINVALIDFUNC);
          break;
        }
    }

  return funcType;
}

/****************************************************************************/

void pas_SizeOfFunc(void)
{
  uint16_t size;

  /* FORM:  sizeof '(' variable | type ')' */

  pas_CheckLParen();
  switch (g_token)
    {
      /* Variables */

      case sFILE :
      case sTEXTFILE :
      case sINT :
      case sWORD :
      case sSHORTINT :
      case sSHORTWORD :
      case sLONGINT :
      case sLONGWORD :
      case sBOOLEAN :
      case sCHAR :
      case sREAL :
      case sSTRING :
      case sSCALAR :
      case sSUBRANGE :
      case sSET :
      case sARRAY :
      case sRECORD :
        size = g_tknPtr->sParm.v.vSize;
        break;

      /* Pointers variables and VAR parameters are always the size of a point */

      case sPOINTER :
      case sVAR_PARM :
        size = sPTR_SIZE;
        break;

      /* Types */

      case sTYPE :
        size = g_tknPtr->sParm.t.tAllocSize;
        break;

      default:
        error(eINVARG);
        size = 0;
        break;;
    }

  /* Push the size on the stack */

  pas_GenerateDataOperation(opPUSH, size);

  getToken();
  pas_CheckRParen();
}

/****************************************************************************/

void pas_CheckLParen(void)
{
   getToken();                          /* Skip over function name */
   if (g_token != '(') error(eLPAREN);  /* Check for '(' */
   else getToken();
}

/****************************************************************************/

void pas_CheckRParen(void)
{
   if (g_token != ')') error(eRPAREN);  /* Check for ')') */
   else getToken();
}
