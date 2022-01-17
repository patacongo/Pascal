/****************************************************************************
 * pas_stdfunc.c
 * Standard Functions
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdio.h>

#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_pcode.h"
#include "pas_fpops.h"
#include "pas_errcodes.h"
#include "pas_sysio.h"
#include "pas_library.h"

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

static exprType_t pas_NewFunc(void);     /* Memory allocator */
static exprType_t pas_AbsFunc(void);     /* Integer absolute value */
static exprType_t pas_PredFunc(void);
static void       pas_OrdFunc(void);     /* Convert scalar to integer */
static exprType_t pas_SqrFunc(void);
static void       pas_RealFunc(uint8_t fpCode);
static exprType_t pas_SuccFunc(void);
static void       pas_OddFunc(void);
static void       pas_ChrFunc(void);
static void       pas_FileFunc(uint16_t opcode);
static void       pas_SetFunc(uint16_t setOpcode);
static void       pas_CardFunc(void);

/* Enhanced Pascal functions */

/* Non-standard C-library interface functions */

static exprType_t pas_GetEnvFunc (void); /* Get environment string value */

/* Misc. helper functions */

static void       pas_InitializeNewRecord(symbol_t *typePtr);
static void       pas_InitializeNewArray(symbol_t *typePtr);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/
/* Memory allocator */

static exprType_t pas_NewFunc(void)
{
  exprType_t exprType = exprUnknown;

  /* FORM:  'new' '(' type-identifer ')' */

  TRACE(g_lstFile,"[pas_NewFunc]");

  pas_CheckLParen();

  /* Check for type-identifier */

  if (g_token == sTYPE)
    {
      symbol_t *baseTypePtr;
      symbol_t *typePtr;
      uint16_t  varType;

      typePtr = g_tknPtr;
      getToken();

      /* Allocate memory for an object the size of an allocated instance of
       * this type.  A pointer to the allocated memory will lie at the top of
       * the stack at run-time.
       */

      pas_GenerateDataOperation(opPUSH, typePtr->sParm.t.tAllocSize);
      pas_StandardFunctionCall(lbNEW);

      /* If we just allocate a string, shortstring, or file type, then we
       * have to initialize the allocated instance.
       */

      baseTypePtr = pas_GetBaseTypePointer(typePtr);
      varType     = baseTypePtr->sParm.t.tType;

      /* If we just created a string variable, then set up and initializer
       * for the string; memory for the string buffer must be set up at run
       * time.
       */

      if (varType == sSTRING || varType == sSHORTSTRING)
        {
          pas_InitializeNewString(typePtr);
        }

      /* Handle files similarly */

      else if (varType == sFILE || varType == sTEXTFILE)
        {
          pas_InitializeNewFile(typePtr);
        }

      /* A more complex case:  We just created a RECORD variable that may
       * contain string or file fields that need to be initialized.
       */

      else if (varType == sRECORD)
        {
          pas_InitializeNewRecord(typePtr);
        }

      /* Or an array that may contain variables that need initialization.
       * (OR an array or records with fields that are arrays that ... and
       * all need to be initialized).
       */

      else if (varType == sARRAY)
        {
          pas_InitializeNewArray(typePtr);
        }

      exprType = pas_MapVariable2ExprPtrType(varType, false);
    }

   pas_CheckRParen();
   return exprType;
}

/****************************************************************************/

static exprType_t pas_AbsFunc(void)
{
   exprType_t absType;

   TRACE(g_lstFile,"[pas_AbsFunc]");

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

static void pas_OrdFunc(void)
{
   TRACE(g_lstFile,"[pas_OrdFunc]");

   /* FORM:  ORD (<scalar type>) */

   pas_CheckLParen();
   pas_Expression(exprAnyOrdinal, NULL);     /* Get any ordinal type */
   pas_CheckRParen();
}

/****************************************************************************/

static exprType_t pas_PredFunc(void)
{
   exprType_t predType;

   TRACE(g_lstFile,"[pas_PredFunc]");

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

   TRACE(g_lstFile,"[pas_SqrFunc]");

/* FORM:  SQR (<simple integer OR real expression>) */

   pas_CheckLParen();

   sqrType = pas_Expression(exprUnknown, NULL); /* Process any expression */
   if (sqrType == exprInteger) {

     pas_GenerateSimple(opDUP);
     pas_GenerateSimple(opMUL);

   }
   else if (sqrType == exprReal)
     pas_GenerateFpOperation(fpSQR);

   else
     error(eINVARG);

   pas_CheckRParen();
   return sqrType;
}

/****************************************************************************/

static void pas_RealFunc (uint8_t fpOpCode)
{
   exprType_t realType;

   TRACE(g_lstFile,"[pas_RealFunc]");

   /* FORM:  <function identifier> (<real/integer expression>) */

   pas_CheckLParen();

   realType = pas_Expression(exprUnknown, NULL); /* Process any expression */
   if (realType == exprInteger)
     pas_GenerateFpOperation((fpOpCode | fpARG1));
   else if (realType == exprReal)
     pas_GenerateFpOperation(fpOpCode);
   else
     error(eINVARG);

   pas_CheckRParen();
}

/****************************************************************************/

static exprType_t pas_SuccFunc(void)
{
   exprType_t succType;

   TRACE(g_lstFile,"[pas_SuccFunc]");

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
   TRACE(g_lstFile,"[pas_OddFunc]");

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
   TRACE(g_lstFile,"[charFactor]");

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

static void pas_FileFunc(uint16_t opcode)
{
  TRACE(g_lstFile,"[pas_FileFunc]");

  /* FORM: EOF|EOLN {({<file number>})}
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

      pas_GenerateIoOperation(opcode);
      pas_CheckRParen();
    }
  else
    {
      /* FORM: EOF|EOLN
       * Use default INPUT file
       */

      pas_GenerateStackReference(opLDS, g_inputFile);
      pas_GenerateIoOperation(opcode);
    }
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
/* C library getenv interface */

static exprType_t pas_GetEnvFunc(void)
{
  exprType_t stringType;

  TRACE(g_lstFile, "[pas_GetEnvFunc]");

  /* FORM:  <string_var> = getenv(<string>) */

  pas_CheckLParen();

  /* Get the string expression representing the environment variable name. */

  stringType = pas_Expression(exprString, NULL);

  /* Any expression other then 'exprString' would be an error. */

  if (stringType != exprString)
    {
      error(eINVARG);
    }

  pas_StandardFunctionCall(lbGETENV);
  pas_CheckRParen();
  return exprCString;
}

/****************************************************************************/

static void pas_InitializeNewRecord(symbol_t *typePtr)
{
  /* Verify that this is a RECORD type */

  if (typePtr == NULL ||
      typePtr->sKind != sTYPE ||
      typePtr->sParm.t.tType != sRECORD)
    {
      error(eRECORDTYPE);
    }

  /* Looks like a good RECORD type.  On entry, a pointer to the RECORD to
   * be initialized will be at the top of the stack.
   */

  else
    {
      symbol_t *recordObjectPtr;
      int nObjects = typePtr->sParm.t.tMaxValue;
      int objectIndex;

      /* The parent is the RECORD type.  That is followed by the
       * RECORD OBJECT symbols.  The number of following RECORD
       * OBJECT symbols is given by the maxValue field of the
       * RECORD type entry.
       *
       * RECORD OBJECTS may not be contiguous but may be interspersed
       * with spurious (un-named) type symbols.  The first RECORD
       * OBJECT symbol is, however, guaranteed to immediately follow
       * the RECORD type.
       */

      for (objectIndex = 1, recordObjectPtr = &typePtr[1];
           objectIndex <= nObjects && recordObjectPtr != NULL;
           objectIndex++, recordObjectPtr = recordObjectPtr->sParm.r.rNext)
        {
          symbol_t *parentTypePtr;

          if (recordObjectPtr->sKind != sRECORD_OBJECT)
            {
              /* The symbol table must be corrupted */

              error(eHUH);
            }

          /* If this field is a string, then set up to initialize it.
           * At run-time, a pointer to the allocated RECORD will be
           * at the top of the stack.
           */

          parentTypePtr = recordObjectPtr->sParm.r.rParent;

          if (parentTypePtr == NULL || parentTypePtr->sKind != sTYPE)
            {
              error(eHUH);
            }
          else if (parentTypePtr->sParm.t.tType == sSTRING ||
                   parentTypePtr->sParm.t.tType == sSHORTSTRING)
            {
              /* Get the address of the string field to be initialized at the
               * top of the stack.
               */

              pas_GenerateSimple(opDUP);
              pas_GenerateDataOperation(opPUSH,
                                        recordObjectPtr->sParm.r.rOffset);
              pas_GenerateSimple(opADD);
              pas_InitializeNewString(parentTypePtr);
              pas_GenerateDataOperation(opINDS, -sINT_SIZE);
            }
          else if (parentTypePtr->sParm.t.tType == sFILE ||
                   parentTypePtr->sParm.t.tType == sTEXTFILE)
            {
              /* Get the address of the file field to be initialized at the
               * top of the stack.
               */

              pas_GenerateSimple(opDUP);
              pas_GenerateDataOperation(opPUSH,
                                        recordObjectPtr->sParm.r.rOffset);
              pas_GenerateSimple(opADD);
              pas_InitializeNewFile(parentTypePtr);
              pas_GenerateDataOperation(opINDS, -sINT_SIZE);
            }
          else if (parentTypePtr->sParm.t.tType == sRECORD)
            {
              pas_GenerateSimple(opDUP);
              pas_GenerateDataOperation(opPUSH,
                                        recordObjectPtr->sParm.r.rOffset);
              pas_GenerateSimple(opADD);
              pas_InitializeNewRecord(parentTypePtr);
              pas_GenerateDataOperation(opINDS, -sINT_SIZE);
            }
          else if (parentTypePtr->sParm.t.tType == sARRAY)
            {
              /* Get the address of the array field to be initialized at the
               * top of the stack.
               */

              pas_GenerateSimple(opDUP);
              pas_GenerateDataOperation(opPUSH,
                                        recordObjectPtr->sParm.r.rOffset);
              pas_GenerateSimple(opADD);
              pas_InitializeNewArray(parentTypePtr);
              pas_GenerateDataOperation(opINDS, -sINT_SIZE);
            }
        }
    }
}

/****************************************************************************/

static void pas_InitializeNewArray(symbol_t *typePtr)
{
  /* On entry, a pointer to the ARRAY to be initialized will be at the top
   * of the stack.
   */

  symbol_t *baseTypePtr;

  /* Some sanity checks */

  if (typePtr->sKind           != sTYPE  ||
      typePtr->sParm.t.tType   != sARRAY ||
      typePtr->sParm.t.tParent == NULL   ||
      typePtr->sParm.t.tIndex  == NULL)
    {
      error(eHUH);  /* Should never happen */
    }

  /* We are only interested if the parent type is a FILE, STRING, or abort
   * RECORD that may contain file or string fields.
   */

  /* Get a pointer to the underlying base type symbol */

  baseTypePtr = pas_GetBaseTypePointer(typePtr);

  if (baseTypePtr->sParm.t.tType == sFILE        ||
      baseTypePtr->sParm.t.tType == sTEXTFILE    ||
      baseTypePtr->sParm.t.tType == sSTRING      ||
      baseTypePtr->sParm.t.tType == sSHORTSTRING ||
      baseTypePtr->sParm.t.tType == sRECORD      ||
      baseTypePtr->sParm.t.tType == sARRAY)
    {
      symbol_t *indexPtr;
      int       nElements;
      int       index;

      /* The index should be a SUBRANGE or SCALAR type */

      indexPtr = typePtr->sParm.t.tIndex;
      if (indexPtr->sKind != sTYPE ||
          (indexPtr->sParm.t.tType != sSUBRANGE &&
           indexPtr->sParm.t.tType != sSCALAR))
        {
          error(eHUH);  /* Should not happen */
        }

      /* Now loop for each element of the array */

      nElements = (int)indexPtr->sParm.t.tMaxValue -
                  (int)indexPtr->sParm.t.tMinValue + 1;

      for (index = 0; index < nElements; index++)
        {
          /* The address of the beginning of the array is at the TOP of the
           * stack.  Duplicate it and offset it for the index and element
           * size.
           */

          pas_GenerateSimple(opDUP);
          if (index > 0)
            {
              pas_GenerateDataOperation(opPUSH,
                                        baseTypePtr->sParm.t.tAllocSize);
              if (index > 1)
                {
                  pas_GenerateDataOperation(opPUSH, index);
                  pas_GenerateSimple(opMUL);
                }
              else
                {
                  pas_GenerateSimple(opADD);
                }
            }

          /* Generate the initializer */

          switch (baseTypePtr->sParm.t.tType)
            {
              case sFILE :
              case sTEXTFILE :
                pas_InitializeNewFile(baseTypePtr);
                break;

              case sSTRING :
              case sSHORTSTRING :
                pas_InitializeNewString(baseTypePtr);
                break;

              case sRECORD :
                pas_InitializeNewRecord(baseTypePtr);
                break;

              case sARRAY :
                pas_InitializeNewArray(baseTypePtr);
                break;

              default:
                error(eHUH);
                break;
            }
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void pas_PrimeStandardFunctions(void)
{
}

/****************************************************************************/
/* Process a standard Pascal function call */

exprType_t pas_StandardFunction(void)
{
  exprType_t funcType = exprUnknown;

  TRACE(g_lstFile,"[pas_StandardFunction]");

  /* Is the token a function? */

  if (g_token == tSTDFUNC)
    {
      /* Yes, process it procedure according to the extended token type */

      switch (g_tknSubType)
        {
           /* Memory alloctor */

        case txNEW :
          funcType = pas_NewFunc();
          break;

          /* Functions which return the same type as their argument */

        case txABS :
          funcType = pas_AbsFunc();
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

        case txGETENV : /* Non-standard C library interfaces */
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
          error(eINVALIDPROC);
          break;
        }
    }

  return funcType;
}

void pas_CheckLParen(void)
{
   getToken();                          /* Skip over function name */
   if (g_token != '(') error(eLPAREN);  /* Check for '(' */
   else getToken();
}

void pas_CheckRParen(void)
{
   if (g_token != ')') error(eRPAREN);  /* Check for ')') */
   else getToken();
}
