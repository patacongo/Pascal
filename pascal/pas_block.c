/****************************************************************************
 * pas_block.c
 * Process a Pascal Block
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

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_errcodes.h"
#include "pas_pcode.h"

#include "pas_main.h"
#include "pas_block.h"
#include "pas_initializer.h"
#include "pas_expression.h"
#include "pas_statement.h"
#include "pas_codegen.h"
#include "pas_token.h"
#include "pas_symtable.h"
#include "pas_insn.h"
#include "pas_machine.h"
#include "pas_error.h"

/****************************************************************************
 * Private Definitions
 ****************************************************************************/

/* This macro implements a test for:
 * FORM:  unsigned-constant = integer-number | real-number |
 *       character-literal | string-literal | constant-identifier |
 *       'nil'
 */

#define isConstant(x) \
        (  ((x) == tINT_CONST) \
        || ((x) == tBOOLEAN_CONST) \
        || ((x) == tCHAR_CONST) \
        || ((x) == tREAL_CONST) \
        || ((x) == sSCALAR_OBJECT))

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void      pas_DeclareLabel          (void);
static void      pas_DeclareConst          (void);
static symbol_t *pas_DeclareType           (char *typeName);
static symbol_t *pas_DeclareSimpleType     (char *typeName);
static symbol_t *pas_DeclareVar            (void);
static void      pas_ProcedureDeclaration  (void);
static void      pas_FunctionDeclaration   (void);

static symbol_t *pas_TypeIdentifier        (void);
static symbol_t *pas_CheckShortString      (symbol_t *typePtr, char *typeName);
static symbol_t *pas_TypeDenoter           (char *typeName);
static symbol_t *pas_FileTypeDenoter       (void);
static symbol_t *pas_NewComplexType        (char *typeName);
static symbol_t *pas_NewSimpleType         (char *typeName);
static symbol_t *pas_OrdinalTypeIdentifier (void);
static symbol_t *pas_GetArrayIndexType     (void);
static void      pas_GetArrayBaseType      (symbol_t *arrayTypePtr);
static symbol_t *pas_DeclareRecordType     (char *recordName);
static symbol_t *pas_DeclareField          (symbol_t *recordPtr,
                                            symbol_t *lastField);
static symbol_t *pas_DeclareParameter      (bool pointerType);
static void      pas_AddRecordInitializers (symbol_t *varPtr,
                                            symbol_t *typePtr);
static void      pas_AddArrayInitializers  (symbol_t *varPtr,
                                            symbol_t *typePtr);
static bool      pas_IntAlignRequired      (symbol_t *typePtr);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int32_t g_nParms;
static int32_t g_dwVarSize;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Process LABEL block */

static void pas_DeclareLabel(void)
{
   char   *labelname;                   /* Label symbol table name */

   TRACE(g_lstFile,"[pas_DeclareLabel]");

   /* FORM:  LABEL <integer>[,<integer>[,<integer>][...]]]; */

   do
     {
       getToken();
       if ((g_token == tINT_CONST) && (g_tknInt >= 0))
         {
           labelname = g_stringSP;
           (void)sprintf(labelname, "%" PRId32, g_tknInt);
           while (*g_stringSP++);
           (void)pas_AddLabel(labelname, ++g_label);
           getToken();
         }
       else error(eINTCONST);
     }
   while (g_token == ',');

   if (g_token != ';') error(eSEMICOLON);
   else getToken();
}

/****************************************************************************/
/* Process constant definition:
 * FORM: constant-definition = identifier '=' constant
 * FORM: constant = [ sign ] integer-number |
 *                  [ sign ] real-number |
 *                  [ sign ] constant-identifier |
 *                           character-literal |
 *                           string-literal
 */

static void pas_DeclareConst(void)
{
  char *const_name;

  TRACE(g_lstFile,"[pas_DeclareConst]");

  /* FORM:  <identifier> = <numeric constant|string>
   * NOTE:  Only integer constants are supported
   */

  /* Save the name of the constant */

  const_name = g_tokenString;

  /* Verify that the name is followed by '=' and get the
   * following constant value.
   */

  getToken();
  if (g_token != '=') error(eEQ);
  else getToken();

  /* Handle constant expressions */

  pas_ConstantExpression();

  /* Add the constant to the symbol table based on the type of
   * the constant found following the '= [ sign ]'
   */

  switch (g_constantToken)
    {
    case tINT_CONST :
    case tCHAR_CONST :
    case tBOOLEAN_CONST :
    case sSCALAR_OBJECT :
      (void)pas_AddConstant(const_name, g_constantToken, &g_constantInt, NULL);
      break;

    case tREAL_CONST :
      (void)pas_AddConstant(const_name, g_constantToken, (int32_t*)&g_constantReal, NULL);
      break;

    case tSTRING_CONST :
      {
        uint32_t offset = poffAddRoDataString(g_poffHandle, g_constantStart);
        (void)pas_AddStringConstant(const_name, offset, strlen(g_constantStart));
      }
      break;

    default :
      error(eINVCONST);
    }
}

/****************************************************************************/
/* Process TYPE declaration */

static symbol_t *pas_DeclareType(char *typeName)
{
  symbol_t *typePtr;

  TRACE(g_lstFile,"[pas_DeclareType]");

  /* This function processes the type-denoter in
   * FORM: type-definition = identifier '=' type-denoter
   * FORM: array-type = 'array' '[' index-type-list ']' 'of' type-denoter
   */

  /* FORM: type-denoter = type-identifier | new-type
   * FORM: new-type = new-ordinal-type | new-complex-type
   */

  typePtr = pas_NewComplexType(typeName);
  if (typePtr == NULL)
    {
      /* Check for Simple Types */

      typePtr = pas_DeclareSimpleType(typeName);
      if (typePtr == NULL)
        {
          error(eINVTYPE);
        }
    }

  return typePtr;
}

/****************************************************************************/
/* Process a simple TYPE declaration */

static symbol_t *pas_DeclareSimpleType(char *typeName)
{
  symbol_t *typePtr;
  symbol_t *typeIdPtr;

  /* Declare a new ordinal type */

  typePtr = pas_NewSimpleType(typeName);

  /* Otherwise, declare a type equivalent to a previously defined type
   * NOTE: the following logic is incomplete.  Its is only good for
   * sKind == sType
   */

  if (typePtr == NULL)
    {
      typeIdPtr = pas_TypeIdentifier();
      if (typeIdPtr)
        {
          /* Before we do that, handle the special case of short strings.
           * These look like a standard string but have size information
           * post-pended to them, making them a different type.
           */

          if (typeIdPtr->sParm.t.tType == sSTRING)
            {
              /* Check if this is really a new short string type */

              typePtr = pas_CheckShortString(typePtr, typeName);
            }

          /* No? Then define the new type as an alias for the existing type. */

          if (typePtr == NULL)
            {
              typePtr = pas_AddTypeDefine(typeName, typeIdPtr->sParm.t.tType,
                                          g_dwVarSize, typeIdPtr);
            }
        }
    }

  return typePtr;
}

/****************************************************************************/
/* Process VAR declaration */

static symbol_t *pas_DeclareVar(void)
{
  symbol_t *varPtr;
  symbol_t *typePtr;
  char     *varName;

  TRACE(g_lstFile,"[pas_DeclareVar]");

  /* FORM: variable-declaration = identifier-list ':' type-denoter
   * FORM: identifier-list = identifier { ',' identifier }
   */

  typePtr  = NULL;

  /* Save the current identifier */

  varName = g_tokenString;
  getToken();

  /* A comma indicates that there is another indentifier in the
   * identifier-list
   */

  if (g_token == ',')
    {
      /* Yes ..Process the next identifer in the indentifier list
       * via recursion (limiting the search to the current level).
       */

      getLevelToken();
      if (g_token != tIDENT) error(eIDENT);
      else typePtr = pas_DeclareVar();
    }
  else
    {
      /* No.. verify that the identifer-list is followed by ':' */

      if (g_token != ':') error(eCOLON);
      else getToken();

      /* Let's handle files differently.  There are a few quirks to
       * address.
       */

      typePtr = pas_FileTypeDenoter();
      if (typePtr == NULL)
        {
          /* Process the normal, un-named type-denoter */

          typePtr = pas_TypeDenoter("");
          if (typePtr == NULL)
            {
              error(eINVTYPE);
            }
        }
    }

  if (typePtr != NULL)
    {
      uint16_t varType = typePtr->sParm.t.tType;

      if (varType == sFILE || varType == sTEXTFILE)
        {
          symbol_t *filePtr;

          /* For the FILE case, typePtr is not a type at all but the pointer
           * to the file variable that was declared from a more nested
           * recusion.  We can use that symbol to clone the one at this
           * nesting level.
           */

          filePtr = pas_AddFile(varName, typePtr->sParm.t.tType, g_dStack,
                                typePtr->sParm.t.tAllocSize,
                                typePtr->sParm.t.tParent);

          pas_AddFileInitializer(filePtr, false, 0);
          g_dStack += sINT_SIZE;
        }
      else
        {
          /* Determine if alignment to INTEGER boundaries is necessary */

          if (!INT_ISALIGNED(g_dStack) && pas_IntAlignRequired(typePtr))
            {
              g_dStack = INT_ALIGNUP(g_dStack);
            }

          /* Add the new variable to the symbol table */

          varPtr = pas_AddVariable(varName, varType, g_dStack, g_dwVarSize,
                                   typePtr);

          /* If we just created a string variable, then set up and
           * initializer for the string; memory for the string buffer
           * must be set up at run time.
           */

          if (varType == sSTRING || varType == sSHORTSTRING)
            {
              pas_AddStringInitializer(varPtr);
            }

          /* A more complex case:  We just created a RECORD variable that
           * may contain string fields that need to be initialized.
           */

          else if (varType == sRECORD)
            {
              pas_AddRecordInitializers(varPtr, typePtr);
            }

          /* Or an array that may contain variables that need initialization.
           * (OR an array or records with fields that are arrays that ... and
           * all need to be initialized).
           */

          else if (varType == sARRAY)
            {
              pas_AddArrayInitializers(varPtr, typePtr);
            }

          /* If the variable is declared in an interface section at level
           * zero, then it is a candidate to be imported or exported.
           */

          if (!g_level && FP->section == eIsInterfaceSection)
            {
              /* Are we importing or exporting the interface?
               *
               * PROGRAM EXPORTS:
               * If we are generating a program binary (i.e., FP0->kind ==
               * eIsProgram) then the variable memory allocation must appear
               * on the initial stack allocation; therefore the variable
               * stack offset myst be exported by the program binary.
               *
               * UNIT IMPORTS:
               * If we are generating a unit binary (i.e., FP0->kind ==
               * eIsUnit), then we are importing the level 0 stack offset
               * from the main program.
               */

              if (FP0->kind == eIsUnit)
                {
                  /* Mark the symbol as external and replace the absolute
                   * offset with this relative offset.
                   */

                  varPtr->sParm.v.vFlags  |= SVAR_EXTERNAL;
                  varPtr->sParm.v.vOffset  = g_dStack - FP->dstack;

                  /* IMPORT the symbol; assign an offset relative to
                   * the dstack at the beginning of this file
                   */

                  pas_GenerateStackImport(varPtr);
                }
              else /* if (FP0->kind == eIsProgram) */
                {
                  /* EXPORT the symbol */

                  pas_GenerateStackExport(varPtr);
                }
            }

          /* In any event, bump the stack offset to include space for
           * this new symbol.  The 'bumped' stack offset will be the
           * offset for the next variable that is declared.
           */

          g_dStack += g_dwVarSize;
        }
    }

  return typePtr;
}

/****************************************************************************/
/* Process Procedure Declaration Block */

static void pas_ProcedureDeclaration(void)
{
  uint16_t     procLabel = ++g_label;
  char        *saveStringSP;
  symbol_t    *procPtr;
  unsigned int saveSymOffset;    /* Saved previous level symbol offset */
  unsigned int saveConstOffset;  /* Saved previous level constant offset */
  int          i;

  TRACE(g_lstFile,"[pas_ProcedureDeclaration]");

  /* FORM: procedure-declaration =
   *       procedure-heading ';' directive |
   *       procedure-heading ';' procedure-block
   * FORM: procedure-heading =
   *       'procedure' identifier [ formal-parameter-list ]
   * FORM: procedure-identifier = identifier
   *
   * On entry, g_token refers to token AFTER the 'procedure' reserved
   * word.
   */

  /* Process the procedure-heading */

  if (g_token != tIDENT)
    {
      error(eIDENT);
      return;
    }

  /* Add the procedure to the symbol table */

  procPtr = pas_AddProcedure(g_tokenString, sPROC, procLabel, 0, NULL);

  /* Save the string stack pointer so that we can release all
   * formal parameter strings later.  Then get the next token.
   */

  saveStringSP       = g_stringSP;

  /* Set the current symbol/constant table offsets for this level */

  saveSymOffset      = g_levelSymOffset;
  saveConstOffset    = g_levelConstOffset;

  g_levelSymOffset   = g_nSym;
  g_levelConstOffset = g_nConst;

  /* NOTE:  The level associated with the PROCEDURE symbol is the level
   * At which the procedure was declared.  Everything declare within the
   * PROCEDURE is at the next level
   */

  g_level++;

  /* Process parameter list */

  getToken();
  (void)pas_FormalParameterList(procPtr);

  if (g_token !=  ';') error(eSEMICOLON);
  else getToken();

  /* If we are here then we know that we are either in a program file
   * or the 'implementation' part of a unit file (see pas_unit.c -- At
   * present, the procedure declarations of the 'interface' section of
   * a unit file follow a different path).  In the latter case (only), we
   * should export every procedure declared at level zero.
   */

  if (g_level == 1 && FP->kind == eIsUnit)
    {
      /* EXPORT the procedure symbol. */

      pas_GenerateProcExport(procPtr);
    }

  /* Save debug information about the procedure */

  pas_GenerateDebugInfo(procPtr, 0);

  /* Process block */

  pas_GenerateDataOperation(opLABEL, (int32_t)procLabel);
  pas_Block(0);

  /* Destroy formal parameter names */

  for (i = 1; i <= procPtr->sParm.p.pNParms; i++)
    {
      procPtr[i].sName = NULL;
    }

  g_stringSP = saveStringSP;

  /* Generate exit from procedure */

  pas_GenerateSimple(opRET);
  g_level--;

  /* Restore the symbol/constant table offsets for the previous level */

  g_levelSymOffset   = saveSymOffset;
  g_levelConstOffset = saveConstOffset;

  /* Verify that END terminates with a semicolon */

  if (g_token !=  ';') error(eSEMICOLON);
  else getToken();
}

/****************************************************************************/
/* Process Function Declaration Block */

static void pas_FunctionDeclaration(void)
{
  uint16_t    funcLabel = ++g_label;
  int16_t     parameterOffset;
  char        *saveStringSP;
  symbol_t    *funcPtr;
  symbol_t    *valPtr;
  symbol_t    *typePtr;
  char        *funcName;
  unsigned int saveSymOffset;   /* Saved previous level symbol offset */
  unsigned int saveConstOffset; /* Saved previous level constant offset */
  int          i;

  TRACE(g_lstFile,"[pas_FunctionDeclaration]");

  /* FORM: function-declaration =
   *       function-heading ';' directive |
   *       function-heading ';' function-block
   * FORM: function-heading =
   *       'function' function-identifier [ formal-parameter-list ]
   *       ':' result-type
   *
   * On entry g_token should lrefer to the function-identifier.
   */

  /* Verify function-identifier */

  if (g_token != tIDENT)
    {
      error(eIDENT);
      return;
    }

  funcPtr = pas_AddProcedure(g_tokenString, sFUNC, funcLabel, 0, NULL);

  /* NOTE:  The level associated with the FUNCTION symbol is the level
   * At which the procedure was declared.  Everything declare within the
   * PROCEDURE is at the next level
   */

  g_level++;

  /* Save the string stack pointer so that we can release all
   * formal parameter strings later.
   */

  funcName           = g_tokenString;
  saveStringSP       = g_stringSP;

  /* Set the current symbol/constant table offsets for this level */

  saveSymOffset      = g_levelSymOffset;
  saveConstOffset    = g_levelConstOffset;

  g_levelSymOffset   = g_nSym;
  g_levelConstOffset = g_nConst;

  /* Process parameter list */

  getToken();
  parameterOffset    = pas_FormalParameterList(funcPtr);

  /* Verify that the parameter list is followed by a colon */

  if (g_token !=  ':') error(eCOLON);
  else getToken();

  /* Declare the function return value variable.  This variable has
   * the same name as the function itself.  We fill the variable
   * symbol descriptor with bogus information now (but we fix it
   * below).
   */

  valPtr  = pas_AddVariable(funcName, sINT, 0, sINT_SIZE, NULL);

  /* Get function type, return value type/size and offset to return value */

  typePtr = pas_TypeIdentifier();
  if (typePtr)
   {
      /* The offset to the return value is the offset to the last
       * parameter minus the size of the return value (aligned to
       * multiples of size of INTEGER).
       */

      parameterOffset        -= g_dwVarSize;
      parameterOffset         = INT_ALIGNUP(parameterOffset);

      /* Save the TYPE for the function return value local variable */

      valPtr->sKind           = typePtr->sParm.t.tType;
      valPtr->sParm.v.vOffset = parameterOffset;
      valPtr->sParm.v.vSize   = g_dwVarSize;
      valPtr->sParm.v.vParent = typePtr;

      /* Save the TYPE for the function */

      funcPtr->sParm.p.pParent = typePtr;

      /* If we are here then we know that we are either in a program file
       * or the 'implementation' part of a unit file (see pas_unit.c -- At
       * present, the function declarations of the 'interface' section of a
       * unit file follow a different path).  In the latter case (only), we
       * should export every function declared at level zero.
       */

      if ((g_level == 1) && (FP->kind == eIsUnit))
        {
          /* EXPORT the function symbol. */

          pas_GenerateProcExport(funcPtr);
        }
    }
  else
   {
      error(eINVTYPE);
   }

  /* Save debug information about the function */

  pas_GenerateDebugInfo(funcPtr, g_dwVarSize);

  /* Process block */

  if (g_token !=  ';') error(eSEMICOLON);
  else getToken();

  pas_GenerateDataOperation(opLABEL, (int32_t)funcLabel);
  pas_Block(0);

  /* Destroy formal parameter names and the function return value name */

  for (i = 1; i <= funcPtr->sParm.p.pNParms; i++)
    {
      funcPtr[i].sName = ((char *) NULL);
    }

  valPtr->sName = ((char *) NULL);
  g_stringSP    = saveStringSP;

  /* Generate exit from procedure/function */

  pas_GenerateSimple(opRET);
  g_level--;

  /* Restore the symbol/constant table offsets for the previous level */

  g_levelSymOffset   = saveSymOffset;
  g_levelConstOffset = saveConstOffset;

  /* Verify that END terminates with a semicolon */

  if (g_token !=  ';') error(eSEMICOLON);
  else getToken();
}

/****************************************************************************/
/* Verify that the next token is a type identifer
 * NOTE:  This function modifies the global variable g_dwVarSize
 * as a side-effect
 */

static symbol_t *pas_TypeIdentifier(void)
{
  symbol_t *typePtr = NULL;

  TRACE(g_lstFile,"[pas_TypeIdentifier]");

  /* Check for type-identifier */

  if (g_token == sTYPE)
    {
      /* Return a reference to the type token. */

      typePtr = g_tknPtr;
      getToken();

      /* Return the size of an allocated instance of this type. */

      g_dwVarSize = typePtr->sParm.t.tAllocSize;

      /* Before we fully commit to doing that, handle the special case of
       * short strings:  These look like a standard string but have size
       * information post-appended to them, making them a different type.
       */

      if (typePtr->sParm.t.tType == sSTRING)
        {
          /* Check if this is really a new, un-named short string type */

          typePtr = pas_CheckShortString(typePtr, "");
        }
    }

  return typePtr;
}

/****************************************************************************/
/* A STRING type has been found.  If there is a size associated with the
 * STRING, then create a new SHORTSTRING type.
 */

static symbol_t *pas_CheckShortString(symbol_t *typePtr, char *typeName)
{
  TRACE(g_lstFile,"[pas_CheckShortString]");

  /* Is the type name followed by a storage size value. */

  if (g_token == '[')
    {
      /* Parse the size specification:
       *
       *   FORM: string-type = pascal-string-type | c-string-type
       *   FORM: pascal-string-type = 'string' [ max-string-length ]
       *
       * The left bracket should be followed by a constant expression.
       */

      getToken();
      pas_ConstantExpression();

      if (g_constantToken != tINT_CONST) error(eINTCONST);
      else if (g_constantInt <= 0) error(eINVCONST);
      else
        {
          /* Create a new, unique, un-named SHORTSTRING type. */

          typePtr = pas_AddTypeDefine(typeName, sSHORTSTRING,
                                      sSHORTSTRING_SIZE, NULL);

          /* Save the size of the short string buffer allocation in the
           * tMaxValue field.
           */

          if (typePtr != NULL)
            {
              typePtr->sParm.t.tMaxValue = g_tknInt;
            }

          /* Return the size of an allocated instance of this type. */

          g_dwVarSize = typePtr->sParm.t.tAllocSize;

          /* Verify that the correct token terminated the size
           * specification.  This could be either ')' or ']'
           */

          if (g_token != ']') error(eRBRACKET);
          else getToken();
        }
    }

  return typePtr;
}

/****************************************************************************/

static symbol_t *pas_TypeDenoter(char *typeName)
{
  symbol_t *typePtr;

  TRACE(g_lstFile,"[pas_TypeDenoter]");

  /* FORM: type-denoter = type-identifier | new-type
   *
   * Check for type-identifier
   */

  typePtr = pas_TypeIdentifier();
  if (typePtr != NULL)
    {
      /* Check for a SHORTSTRING type */

      if (typePtr->sParm.t.tType == sSTRING)
        {
          typePtr = pas_CheckShortString(typePtr, "");
        }

      /* Return the type identifier */

      return typePtr;
    }

  /* Check for new-type
   * FORM: new-type = new-ordinal-type | new-complex-type
   */

  /* Check for new-complex-type */

  typePtr = pas_NewComplexType(typeName);
  if (typePtr == NULL)
    {
      /* Check for new-ordinal-type */

      typePtr = pas_NewSimpleType(typeName);
    }

  /* Return the size of an allocated instance of this type. */

  g_dwVarSize = typePtr->sParm.t.tAllocSize;
  return typePtr;
}

/****************************************************************************/
/* Declare is new ordinal type */

static symbol_t *pas_NewSimpleType(char *typeName)
{
  symbol_t *typePtr = NULL;

  /* Declare a new-ordinal-type
   * FORM: new-ordinal-type = enumerated-type | subrange-type
   */

  /* FORM: enumerated-type = '(' enumerated-constant-list ')' */

  if (g_token == '(')
    {
      int32_t nObjects;
      nObjects = 0;
      typePtr = pas_AddTypeDefine(typeName, sSCALAR, sINT_SIZE, NULL);

      /* Now declare each instance of the scalar */

      do
        {
          getToken();
          if (g_token != tIDENT) error(eIDENT);
          else
            {
              (void)pas_AddConstant(g_tokenString, sSCALAR_OBJECT, &nObjects, typePtr);
              nObjects++;
              getToken();
            }
        }
      while (g_token == ',');

      /* Save the number of objects associated with the scalar type (the
       * maximum ORD is nObjects - 1).
       */

      typePtr->sParm.t.tMaxValue = nObjects - 1;

      if (g_token != ')') error(eRPAREN);
      else getToken();
    }

  /* Declare a new subrange type
   * FORM: subrange-type = constant '..' constant
   * FORM: constant =
   *    [ sign ] integer-number |  [ sign ] real-number |
   *    [ sign ] constant-identifier | character-literal | string-literal
   *
   * Case 1: <constant> is INTEGER
   */

  else if (g_token == tINT_CONST ||
           g_token == '-' ||
           g_token == '+')
    {
      int16_t  value = g_tknInt;

      if (g_token == '-' || g_token == '+')
        {
          uint16_t unary = g_token;

          getToken();
          if (g_token != tINT_CONST) error(eINTCONST);
          else
            {
              value = (unary == '-') ? -g_tknInt : g_tknInt;
            }
        }

      /* Create the new INTEGER subrange type */

      typePtr = pas_AddTypeDefine(typeName, sSUBRANGE, sINT_SIZE, NULL);
      typePtr->sParm.t.tSubType  = sINT;
      typePtr->sParm.t.tMinValue = value;
      typePtr->sParm.t.tMaxValue = MAXINT;

      /* Verify that ".." separates the two constants */

      getToken();
      if (g_token != tSUBRANGE) error(eSUBRANGE);
      else getToken();

      /* Verify that the ".." is following by an INTEGER constant */

      if (g_token == tINT_CONST)
        {
          value = g_tknInt;
        }
      else if (g_token == '-' || g_token == '+')
        {
          uint16_t unary = g_token;

          getToken();
          if (g_token != tINT_CONST) error(eINTCONST);
          else
            {
              value = (unary == '-') ? -g_tknInt : g_tknInt;
            }
        }
      else
        {
          error(eINTCONST);
        }

      if (value < typePtr->sParm.t.tMinValue)
        {
          error(eSUBRANGETYPE);
        }
      else
        {
          typePtr->sParm.t.tMaxValue = value;
        }

      getToken();
    }

  /* Case 2: <constant> is CHAR */

  else if (g_token == tCHAR_CONST)
    {
      /* Create the new CHAR subrange type */

      typePtr = pas_AddTypeDefine(typeName, sSUBRANGE, sCHAR_SIZE, NULL);
      typePtr->sParm.t.tSubType  = sCHAR;
      typePtr->sParm.t.tMinValue = g_tknInt;
      typePtr->sParm.t.tMaxValue = MAXCHAR;

      /* Verify that ".." separates the two constants */

      getToken();
      if (g_token != tSUBRANGE) error(eSUBRANGE);
      else getToken();

      /* Verify that the ".." is following by a CHAR constant */

      if (g_token != tCHAR_CONST || g_tknInt < typePtr->sParm.t.tMinValue)
        {
          error(eSUBRANGETYPE);
        }
      else
        {
          typePtr->sParm.t.tMaxValue = g_tknInt;
          getToken();
        }
    }

  /* Case 3: <constant> is a SCALAR type */

  else if (g_token == sSCALAR_OBJECT)
     {
      /* Create the new SCALAR subrange type */

      typePtr = pas_AddTypeDefine(typeName, sSUBRANGE, sINT_SIZE, g_tknPtr);
      typePtr->sParm.t.tSubType  = g_token;
      typePtr->sParm.t.tMinValue = g_tknInt;
      typePtr->sParm.t.tMaxValue = MAXINT;

      /* Verify that ".." separates the two constants */

      getToken();
      if (g_token != tSUBRANGE) error(eSUBRANGE);
      else getToken();

      /* Verify that the ".." is following by a SCALAR constant of the same
       * type as the one which preceded it
       */

      if ((g_token != sSCALAR_OBJECT) ||
          (g_tknPtr != typePtr->sParm.t.tParent) ||
          (g_tknPtr->sParm.c.cValue.i < typePtr->sParm.t.tMinValue))
        {
          error(eSUBRANGETYPE);
        }
      else
        {
          typePtr->sParm.t.tMaxValue = g_tknPtr->sParm.c.cValue.i;
          getToken();
        }
    }

  return typePtr;
}

/****************************************************************************/

static symbol_t *pas_FileTypeDenoter(void)
{
  symbol_t *fileTypePtr = NULL;

  /* There are three possibilities here for files.
   *
   *   FORM: file-variable-name : FILE OF type-name;
   *   FORM: file-variable-name : TEXFILE;
   *   FORM: file-variable-name : file-type-name
   *
   * Let's handle the first two forms first.
   */

  if (g_token == tFILE || g_token == sTEXTFILE)
    {
      /* TEXTFILE is a pre-defined type */

      if (g_token == sTEXTFILE)
        {
          fileTypePtr = g_tknPtr;
          getToken();
        }

      /* There are many different binary files, differing in the size of
       * transfer unit.
       */

      else /* if (g_token == tFILE) */
        {
          getToken();

          /* Make sure that 'file' is followed by 'of' */

          if (g_token != tOF) error(eOF);
          else getToken();

          /* Make sure that the token following 'of' is a type */

          if (g_token != sTYPE) error(eINVTYPE);
          else
            {
              symbol_t *parentTypePtr = g_tknPtr;

              /* Create an un-named type for the file */

              fileTypePtr =
                pas_AddTypeDefine("", sFILE,
                                  parentTypePtr->sParm.t.tAllocSize,
                                  parentTypePtr);
              if (fileTypePtr != NULL)
                {
                  fileTypePtr->sParm.t.tSubType  =
                    parentTypePtr->sParm.t.tType;
                  fileTypePtr->sParm.t.tMinValue =
                    parentTypePtr->sParm.t.tMinValue;
                  fileTypePtr->sParm.t.tMaxValue =
                    parentTypePtr->sParm.t.tMaxValue;
                }

              getToken();
            }
        }
    }

  /* Now we expect the beginning of some kind of type */

  if (g_tknPtr != NULL)
    {
      symbol_t *baseTypePtr;

      /* Get a pointer to the base type symbol (in case it is a defined
       * type).  Loop until we hit the last type in the chain OR until we
       * hit a file type (in that case, the last type may be the type of the
       * binary file, not the type of the file type).
       */

      baseTypePtr = pas_GetBaseTypePointer(g_tknPtr);

      /* Did we find a typed file? */

      if (baseTypePtr->sParm.t.tType == sFILE ||
          baseTypePtr->sParm.t.tType == sTEXTFILE)
        {
          /* TEXTFILE is a pre-defined type and has no parent */

          if (baseTypePtr->sParm.t.tType == sTEXTFILE)
            {
              fileTypePtr = baseTypePtr;
              getToken();
            }
          else /* if (baseTypePtr->sParm.t.tType == sFILE) */
            {
              symbol_t *parentTypePtr = baseTypePtr->sParm.t.tParent;

              /* Create an un-named type for the file */

              fileTypePtr =
                pas_AddTypeDefine("", sFILE,
                                  parentTypePtr->sParm.t.tAllocSize,
                                  parentTypePtr);
              if (fileTypePtr != NULL)
                {
                  fileTypePtr->sParm.t.tSubType =
                    parentTypePtr->sParm.t.tType;
                  fileTypePtr->sParm.t.tMinValue =
                    parentTypePtr->sParm.t.tMinValue;
                  fileTypePtr->sParm.t.tMaxValue =
                    parentTypePtr->sParm.t.tMaxValue;
                }

              getToken();
            }
        }
    }

  return fileTypePtr;
}

/****************************************************************************/

static symbol_t *pas_NewComplexType(char *typeName)
{
  symbol_t *typePtr = NULL;
  symbol_t *typeIdPtr;
  symbol_t *indexTypePtr;

  TRACE(g_lstFile,"[pas_NewComplexType]");

  /* FORM: new-complex-type = new-structured-type | new-pointer-type */

  switch (g_token)
    {
      /* FORM: new-pointer-type = '^' domain-type | '@' domain-type */

    case '^'      :
      getToken();
      typeIdPtr = pas_TypeIdentifier();
      if (typeIdPtr)
        {
          typePtr = pas_AddTypeDefine(typeName, sPOINTER, g_dwVarSize,
                                      typeIdPtr);
        }
      else
        {
          error(eINVTYPE);
        }
      break;

      /* FORM: new-structured-type =
       *     [ 'packed' ] array-type | [ 'packed' ] record-type |
       *     [ 'packed' ] set-type   | [ 'packed' ] file-type |
       *     [ 'packed' ] list-type  | object-type | string-type
       */

      /* PACKED Types */

    case tPACKED :
      /* REVISIT: Packed arrays not yet supported. Fail silently.
       * error(eNOTYET);
       */

      getToken();
      if (g_token != tARRAY) break;

      /* Fall through to process PACKED ARRAY type */

      /* Array Types
       * FORM: array-type = 'array' [ index-type-list ']' 'of' type-denoter
       */

    case tARRAY :
      {
        symbol_t *lastIndexTypePtr = NULL;

        /* On entry, 'g_token' refers to the 'array' reserved word.
         *
         * FORM: array-type = 'array' '[' index-type-list ']' 'of'
         *                    type-denoter
         *
         * Skip over the 'array' keyword and verify that the index-type-list
         * is preceded by '['.
         */

        getToken();
        if (g_token != '[') error(eLBRACKET);

        /* Create a dummy ARRAY type to which we will eventually add the
         * indexing and sizing information.
         *
         * Initially:  size = 0, dimension = 0, parent = NULL, and no
         * indices.
         */

        typePtr = pas_AddTypeDefine(typeName, sARRAY, 0, NULL);

        /* Loop for each index-type in the index-type-list
         *
         * FORM: index-type-list = index-type { ',' index-type }
         */

        do
          {
            /* 'g_token' should refer to '[' on entry but to ',' on subsequent
             * passes through this loop.  Skip over whichever we have here.
             */

            getToken();

            /* Get the next index-type from the index-type-list.  This should be
             * some kind of subrange type.
             */

            indexTypePtr = pas_GetArrayIndexType();
            if (indexTypePtr != NULL)
              {
                /* Check for a proper terminating character for the the index-type.
                 * There are two possibilities:
                 *
                 * FORM: array-type = 'array' '[' index-type-list ']' 'of'
                 *                    type-denoter
                 *
                 *   ']' which terminates the index-type-list.
                 *
                 * FORM: index-type-list = index-type { ',' index-type }
                 *
                 *   Or ',' which indicates that there is another dimenion (and
                 *   another index-type in the index-type-list).
                 */

                /* Increase the array dimension and note the dimension
                 * assocated with this index.
                 */

                typePtr->sParm.t.tDimension++;
                indexTypePtr->sParm.t.tDimension = typePtr->sParm.t.tDimension;

                /* Append the new index type to the end of the chain of index
                 * type.
                 */

                if (lastIndexTypePtr == NULL)
                  {
                    typePtr->sParm.t.tIndex = indexTypePtr;
                  }
                else
                  {
                    lastIndexTypePtr->sParm.t.tIndex = indexTypePtr;
                  }

                lastIndexTypePtr = indexTypePtr;
              }
          }
        while (g_token == ',');

        /* Verify that the index-type-list is terminated by the closing ']' */

        if (g_token != ']') error(eRBRACKET);
        else getToken();

        /* Get the base type of the ARRAY and finish off the array type
         * symbol.  At this point, 'g_token' should refer to the OF keyword
         * that precedes the type-denoter.
         */

        pas_GetArrayBaseType(typePtr);
      }
      break;

      /* RECORD Types
       * FORM: record-type = 'record' field-list 'end'
       */

    case tRECORD :
      getToken();
      typePtr = pas_DeclareRecordType(typeName);
      break;

      /* Set Types
       *
       * FORM: set-type = 'set' 'of' ordinal-type
       */

    case tSET :

      /* Verify that 'set' is followed by 'of' */

      getToken();
      if (g_token != tOF) error(eOF);
      else getToken();

      /* Verify that 'set of' is followed by an ordinal-type
       * If not, then declare a new one with no name
       */

      typeIdPtr = pas_OrdinalTypeIdentifier();
      if (typeIdPtr)
        {
          getToken();
        }
      else
        {
          typeIdPtr = pas_DeclareSimpleType(NULL);
        }

      /* Verify that the ordinal-type is either a scalar or a
       * subrange type.  These are the only valid types for 'SET'
       */

      if ((typeIdPtr) &&
          ((typeIdPtr->sParm.t.tType == sSCALAR) ||
           (typeIdPtr->sParm.t.tType == sSUBRANGE)))
        {
          /* Declare the SET type */

          typePtr = pas_AddTypeDefine(typeName, sSET, sSET_SIZE, typeIdPtr);
          if (typePtr)
            {
              int16_t nObjects;

              /* Copy the scalar/subrange characteristics for convenience */

              typePtr->sParm.t.tSubType  = typeIdPtr->sParm.t.tType;
              typePtr->sParm.t.tMinValue = typeIdPtr->sParm.t.tMinValue;
              typePtr->sParm.t.tMaxValue = typeIdPtr->sParm.t.tMaxValue;

              /* Verify that the number of objects associated with the
               * scalar or subrange type will fit into an integer
               * representation of a set as a bit-string.
               */

              nObjects = typeIdPtr->sParm.t.tMaxValue -
                         typeIdPtr->sParm.t.tMinValue + 1;
              if (nObjects > sSET_MAXELEM)
                {
                  error(eSETRANGE);
                  typePtr->sParm.t.tMaxValue = typePtr->sParm.t.tMinValue +
                                               sSET_MAXELEM - 1;
                }
            }
        }
      else
        {
          error(eSET);
        }
      break;

      /* File Types
       * FORM: file-type = 'file' 'of' type-denoter
       * FORM: file-type = 'text'
       */

      /* FORM: file-type = 'file' 'of' type-denoter */

    case tFILE :

      /* Make sure that 'file' is followed by 'of' */

      getToken();
      if (g_token != tOF) error(eOF);
      else getToken();

      /* Get the type-denoter */

      typeIdPtr = pas_TypeDenoter(NULL);
      if (typeIdPtr)
        {
          typePtr = pas_AddTypeDefine(typeName, sFILE, g_dwVarSize,
                                      typeIdPtr);
          if (typePtr)
            {
              typePtr->sParm.t.tSubType = typeIdPtr->sParm.t.tType;
            }
        }
      else
        {
          error(eINVTYPE);
        }
      break;

      /* FORM: file-type = 'text' */

    case sTEXTFILE :

      /* Get the type-denoter */

      typeIdPtr = pas_AddTypeDefine(typeName, sTEXTFILE, g_dwVarSize,
                                    g_tknPtr);
      if (typePtr)
        {
          typePtr->sParm.t.tSubType = typeIdPtr->sParm.t.tType;
        }
      break;

      /* FORM: list-type = 'list' 'of' type-denoter
       * FORM: object-type = 'object' | 'class'
       */

    default :
      break;
   }

  return typePtr;
}

/****************************************************************************/
/* Verify that the next token is a type identifer
 */

static symbol_t *pas_OrdinalTypeIdentifier(void)
{
  symbol_t *typePtr;

  TRACE(g_lstFile,"[pas_OrdinalTypeIdentifier]");

  /* Get the next type from the input stream */

  typePtr = pas_TypeIdentifier();

  /* Was a type encountered? */

  if (typePtr != NULL)
    {
      switch (typePtr->sParm.t.tType)
        {
          /* Check for an ordinal type (verify this list!) */

        case sINT :
        case sBOOLEAN :
        case sCHAR :
        case sSCALAR :
        case sSUBRANGE:
          /* If it is an ordinal type, then just return the
           * type pointer.
           */

          break;

        default :
          /* If not, return NULL */

          typePtr = NULL;
          break;
        }
    }
  return typePtr;
}

/****************************************************************************/
/* Get index and array type for TYPE block or variable declaration */

static symbol_t *pas_GetArrayIndexType(void)
{
  symbol_t *indexTypePtr = NULL;
  int32_t minValue;
  int32_t maxValue;
  uint16_t indexSize;
  uint16_t indexType;
  uint16_t subType;
  bool    haveIndex;

  TRACE(g_lstFile,"[pas_GetArrayIndexType]");

  /* FORM: array-type = 'array' '[' index-type-list ']' 'of' type-denoter
   * FORM: [PACKED] ARRAY [<integer>] OF type-denoter
   *
   * 'g_token' should refer to the next index-type in the index-type-list on
   * entry.
   */

  haveIndex = false;

  /* FORM: index-type-list = index-type { ',' index-type }
   * FORM: index-type = ordinal-type
   */

  if (g_token == tINT_CONST ||
     (g_token == sTYPE && g_tknPtr->sParm.t.tType == tINT_CONST))
    {
      int32_t saveTknInt = g_tknInt;

      /* Check for a subrange-type of integer constants.
       *
       * FORM: ordinal-type = new-ordinal-type | ordinal-type-identifier
       * FORM: new-ordinal-type = enumerated-type | subrange-type
       * FORM: subrange-type = constant '..' constant
       *
       * REVISIT:  Probably should be any valid subrange type.
       */

      getToken();
      if (g_token == tSUBRANGE)
        {
          /* Get the upper value of the sub-range */

          getToken();
          if (g_token != tINT_CONST &&
              (g_token != sTYPE || g_tknPtr->sParm.t.tType != tINT_CONST))
            {
              error(eINTCONST);
            }
          else
            {
              if (g_tknInt <= saveTknInt) error(eSUBRANGETYPE);
              else
                {
                  minValue  = saveTknInt;
                  maxValue  = g_tknInt;
                  indexSize = sINT_SIZE;
                  indexType = sSUBRANGE;
                  subType   = sINT;
                  haveIndex = true;

                  getToken();
                }
            }
        }

#if 0
      /* Some versions of small pascal allow a NON-STANDARD single
       * integer constant as a 'dimension' of the array.
       */

      else
        {
          if (g_tknInt < 0) error(eINVCONST);
          else
            {
              minValue  = 0;
              maxValue  = saveTknInt - 1;
              indexType = sSUBRANGE;
              subType   = sINT;
              indexSize = sINT_SIZE;
              haveIndex = true;

              getToken();
            }
        }
#else
      else
        {
          error(eINDEXTYPE);
        }
#endif
    }

  /* Check for enumerated-type
   *
   * FORM: ordinal-type = new-ordinal-type | ordinal-type-identifier
   * FORM: new-ordinal-type = enumerated-type | subrange-type
   * FORM: enumerated-type = '(' enumerated-constant-list ')'
   * FORM: enumerated-constant-list = enumerated-constant { ',' enumerated-constant }
   * FORM: enumerated-constant = identifier
   */

  else if (g_token == '(')
    {
      error(eNOTYET);
      getToken();
    }

  /* Check for ordinal-type-identifier
   *
   * FORM: ordinal-type = new-ordinal-type | ordinal-type-identifier
   * FORM: ordinal-type-identifier = identifier
   */

  else
    {
      uint16_t ordinalType;

      /* g_token should refer to a type identifier */

      if (g_token != sTYPE) error(eINDEXTYPE);

      /* Get the base type of the type identifier */

      ordinalType = g_tknPtr->sParm.t.tType;

      /* Check for an ordinal type. */

      if (ordinalType == sBOOLEAN ||
          ordinalType == sSCALAR ||
          ordinalType == sSUBRANGE)
        {
          minValue  = g_tknPtr->sParm.t.tMinValue;
          maxValue  = g_tknPtr->sParm.t.tMaxValue;
          indexSize = (ordinalType == sBOOLEAN ?
                       sBOOLEAN_SIZE :
                       sINT_SIZE);
          indexType = ordinalType;
          subType   = g_tknPtr->sParm.t.tType;
          haveIndex = true;

          getToken();
        }

      /* REVISIT: What about other ordinal types like sINT and sCHAR? */

      else if (ordinalType == sINT || ordinalType == sCHAR)
        {
          error(eNOTYET);
          getToken();
        }

      /* Not a recognized index-type */

      else
        {
          error(eINDEXTYPE);
          getToken();
          if (g_token == ']') getToken();
          return NULL;
        }
    }

  /* Check for success */

  if (haveIndex)
    {
      /* We have the array size in elements and the base type, now
       * create the unnamed index-type and convert the size for the
       * type found
       *
       * REVISIT:  This needs to be extended when additional logic is
       * added to deal with ordinal type names as index-type.
       */

     indexTypePtr = pas_AddTypeDefine(NULL, indexType, indexSize, NULL);
      if (indexTypePtr)
        {
          indexTypePtr->sParm.t.tSubType  = subType;
          indexTypePtr->sParm.t.tMinValue = minValue;
          indexTypePtr->sParm.t.tMaxValue = maxValue;
        }
    }

  return indexTypePtr;
}

static void pas_GetArrayBaseType(symbol_t *arrayTypePtr)
{
  symbol_t *typeDenoter  = NULL;
  symbol_t *indexTypePtr = NULL;
  symbol_t *baseTypePtr;
  uint32_t indexUnit;

  TRACE(g_lstFile,"[pas_GetArrayBaseType]");

  /* FORM: array-type = 'array' '[' index-type-list ']' 'of' type-denoter
   * FORM: [PACKED] ARRAY [<integer>] OF type-denoter
   *
   * 'g_token' should refer to 'OF' on entry.
   */

  /* Verify that 'of' precedes the type-denoter */

  if (g_token != tOF) error(eOF);
  else getToken();

  /* OF should be followed by the type-denoter base type.
   * This may be an the name of a previously defined type or a
   * new, unnamed type definition.
   */

  typeDenoter = pas_TypeDenoter(NULL);
  if (typeDenoter == NULL) error(eINVTYPE);

  /* Get a pointer to the underlying base type of the array */

  baseTypePtr = pas_GetBaseTypePointer(typeDenoter);

  /* Get the size of each dimension of the array and size of the whole array
   * by traversing the chain of index types.
   */

  indexUnit = baseTypePtr->sParm.t.tAllocSize;

  for (indexTypePtr = arrayTypePtr->sParm.t.tIndex;
       indexTypePtr != NULL;
       indexTypePtr = indexTypePtr->sParm.t.tIndex)
    {
      uint16_t indexRange;

      /* Save the unit address increment for this index */

      indexTypePtr->sParm.t.tAllocSize = indexUnit;

      /* Next indexUnit[n+1] = indexUnit[n] * indexRange */

      indexRange = indexTypePtr->sParm.t.tMaxValue -
                   indexTypePtr->sParm.t.tMinValue + 1;
      indexUnit *= indexRange;
    }

  /* Final, update the array type info */

  arrayTypePtr->sParm.t.tAllocSize = indexUnit;
  arrayTypePtr->sParm.t.tParent    = typeDenoter;
}

/****************************************************************************/

static symbol_t *pas_DeclareRecordType(char *recordName)
{
  symbol_t *recordPtr;
  symbol_t *fieldPtr;
  int16_t recordOffset;
  int recordCount;

  TRACE(g_lstFile,"[pas_DeclareRecordType]");

  /* FORM: record-type = 'record' field-list 'end' */

  /* Declare the new RECORD type */

  recordPtr = pas_AddTypeDefine(recordName, sRECORD, 0, NULL);

  /* Then declare the field-list associated with the RECORD
   * FORM: field-list =
   *       [
   *         fixed-part [ ';' ] variant-part [ ';' ] |
   *         fixed-part [ ';' ] |
   *         variant-part [ ';' ] |
   *       ]
   *
   * Process the fixed-part first.
   * FORM: fixed-part = record-section { ';' record-section }
   * FORM: record-section = identifier-list ':' type-denoter
   * FORM: identifier-list = identifier { ',' identifier }
   *
   * The first RECORD OBJECT is guaranteed to for the RECORD type
   */

  fieldPtr = NULL;
  for (; ; )
    {
      /* Terminate parsing of the fixed-part when we encounter
       * 'case' indicating the beginning of the variant part of
       * the record.  If there is no fixed-part, then 'case' will
       * appear immediately.
       */

      if (g_token == tCASE) break;

      /* We now expect to see and indentifier representing the beginning
       * of the next fixed field.
       */

      fieldPtr = pas_DeclareField(recordPtr, fieldPtr);

      /* If the field declaration terminates with a semicolon, then
       * we expect to see another <fixed part> declaration in the
       * record.
       */

      if (g_token == ';')
        {
          /* Skip over the semicolon and process the next fixed
           * field declaration.
           */

          getToken();

          /* We will treat this semi colon as optional.  If we
           * hit 'end' or 'case' after the semicolon, then we
           * will terminate the fixed part with no complaint.
           */

          if (g_token == tEND || g_token == tCASE)
            {
              break;
            }
        }

      /* If there is no semicolon after the field declaration,
       * then 'end' or 'case' is expected.  This will be verified
       * below.
       */

      else break;
    }

  /* Get the total size of the RECORD type and the offset of each
   * field within the RECORD.
   */

  fieldPtr = &recordPtr[1];
  for (recordOffset = 0, recordCount = 0;
       recordCount < recordPtr->sParm.t.tMaxValue && fieldPtr != NULL;
       recordCount++, fieldPtr = fieldPtr->sParm.r.rNext)
    {
      /* We know that 'maxValue' sRECORD_OBJECT symbols follow the sRECORD
       * type declaration.  However, these may not be sequential due to the
       * possible declaration of sTYPEs associated with each field.
       */

      if (fieldPtr->sKind == sRECORD_OBJECT)
        {
          /* Align the recordOffset (if necessary) */

          if (!INT_ISALIGNED(recordOffset) &&
              pas_IntAlignRequired(fieldPtr->sParm.r.rParent))
            {
              recordOffset = INT_ALIGNUP(recordOffset);
            }

          /* Save the offset associated with this field, and determine the
           * offset to the next field (if there is one)
           */

          fieldPtr->sParm.r.rOffset = recordOffset;
          recordOffset             += fieldPtr->sParm.r.rSize;
        }
    }

  /* Update the RECORD entry for the total size of all fields */

  recordPtr->sParm.t.tAllocSize = recordOffset;

  /* Now we are ready to process the variant-part.
   * FORM: variant-part = 'case' variant-selector 'of' variant-body
   */

  if (g_token == tCASE)
    {
      int16_t variantOffset;
      uint16_t maxRecordSize;

      /* Skip over the 'case' */

      getToken();

      /* Check for variant-selector
       * FORM: variant-selector = [ identifier ':' ] ordinal-type-identifer
       */

      if (g_token != tIDENT) error(eRECORDDECLARE);

      /* Add a variant-selector to the fixed-part of the record */

      else
        {
          symbol_t *typePtr;
          char  *fieldName;

          /* Save the field name */

          fieldName = g_tokenString;
          getToken();

          /* Verify that the identifier is followed by a colon */

          if (g_token != ':') error(eCOLON);
          else getToken();

          /* Get the ordinal-type-identifier */

          typePtr  = pas_OrdinalTypeIdentifier();
          if (!typePtr) error(eINVTYPE);
          else
            {
              /* Declare a <field> with this <identifier> as its name */

              fieldPtr = pas_AddField(fieldName, recordPtr, fieldPtr);

              /* Increment the number of fields in the record */

              recordPtr->sParm.t.tMaxValue++;

              /* Copy the size of field from the sTYPE entry into the
               * <field> type entry.  NOTE:  This element is not essential
               * since it  can be obtained from the parent type pointer
               */

              fieldPtr->sParm.r.rSize = typePtr->sParm.t.tAllocSize;

              /* Save a pointer back to the parent field type */

              fieldPtr->sParm.r.rParent = typePtr;

              /* Align the recordOffset (if necessary) */

              if (!INT_ISALIGNED(recordOffset) &&
                  pas_IntAlignRequired(typePtr))
                {
                  recordOffset = INT_ALIGNUP(recordOffset);
                }

              /* Save the offset associated with this field, and determine
               * the offset to the next field (if there is one)
               */

              fieldPtr->sParm.r.rOffset = recordOffset;
              recordOffset             += fieldPtr->sParm.r.rSize;
            }
        }

      /* Save the offset to the start of the variant portion of the RECORD */

      variantOffset = recordOffset;
      maxRecordSize = recordOffset;

      /* Skip over the 'of' following the variant selector */

      if (g_token != tOF) error(eOF);
      else getToken();

      /* Loop to process the variant-body
       * FORM: variant-body =
       *       variant-list [ [ ';' ] variant-part-completer ] |
       *       variant-part-completer
       * FORM: variant-list = variant { ';' variant }
       * FORM: variant-part-completer = ( 'otherwise' | 'else' ) ( field-list )
       */

      for (; ; )
        {
          /* Now process each variant where:
           * FORM: variant = case-constant-list ':' '(' field-list ')'
           * FORM: case-constant-list = case-specifier { ',' case-specifier }
           * FORM: case-specifier = case-constant [ '..' case-constant ]
           */

          /* Verify that the case selector begins with a case-constant.
           * Note that subrange case-specifiers are not yet supported.
           */

          if (!isConstant(g_token))
            {
              error(eINVCONST);
              break;
            }

          /* Just consume the <case selector> for now -- Really need to
           * verify that each constant is of the same type as the type
           * identifier (or the type associated with the tag) in the CASE
           */

          do
            {
              getToken();
              if (g_token == ',') getToken();
            }
          while (isConstant(g_token));

          /* Make sure a colon separates case-constant-list from the
           * field-list
           */

          if (g_token == ':') getToken();
          else error(eCOLON);

          /* The field-list must be enclosed in parentheses */

          if (g_token == '(') getToken();
          else error(eLPAREN);

          /* Special case the empty variant <field list> */

          if (g_token != ')')
            {
              symbol_t *firstFieldPtr = NULL;

              /* Now process the <field list> for the variant.  This works
               * just like the field list of the fixed part, except the
               * offset is reset for each variant.
               * FORM: field-list =
               *       [
               *         fixed-part [ ';' ] variant-part [ ';' ] |
               *         fixed-part [ ';' ] |
               *         variant-part [ ';' ] |
               *       ]
               */

              for (; ; )
                {
                  /* We now expect to see and indentifier representating the
                   * beginning of the next variable field.
                   */

                  fieldPtr = pas_DeclareField(recordPtr, fieldPtr);
                  if (firstFieldPtr == NULL)
                    {
                      firstFieldPtr = fieldPtr;
                    }

                  /* If the field declaration terminates with a semicolon,
                   * then we expect to see another <variable part>
                   * declaration in the record.
                   */

                  if (g_token == ';')
                    {
                      /* Skip over the semicolon and process the next
                       * variable field declaration.
                       */

                      getToken();

                      /* We will treat this semi colon as optional.  If we
                       * hit 'end' after the semicolon, then we will
                       * terminate the fixed part with no complaint.
                       */

                      if (g_token == tEND)
                        {
                          break;
                        }
                    }
                  else break;
                }

              /* Get the total size of the RECORD type and the offset of each
               * field within the RECORD.
               */

              fieldPtr = &recordPtr[1];
              for (fieldPtr = firstFieldPtr,
                     recordOffset = variantOffset;
                   fieldPtr != NULL &&
                     recordCount < recordPtr->sParm.t.tMaxValue;
                   fieldPtr = fieldPtr->sParm.r.rNext)
                {
                  /* We know that 'maxValue' sRECORD_OBJECT symbols follow
                   * the sRECORD type declaration.  However, these may not
                   * be sequential due to the possible declaration of sTYPEs
                   * associated with each field.
                   */

                  if (fieldPtr->sKind != sRECORD_OBJECT)
                    {
                      error(eHUH);  /* RECORD OBJECT is not where it should be */
                    }

                  /* Align the recordOffset (if necessary) */

                  if (!INT_ISALIGNED(recordOffset) &&
                      pas_IntAlignRequired(fieldPtr->sParm.r.rParent))
                    {
                      recordOffset = INT_ALIGNUP(recordOffset);
                    }

                  /* Save the offset associated with this field, and
                   * determine the offset to the next field (if there
                   * is one)
                   */

                  fieldPtr->sParm.r.rOffset = recordOffset;
                  recordOffset             += fieldPtr->sParm.r.rSize;
                  recordCount++;
                }

              /* Check if this is the largest variant that we have found
               * so far
               */

              if (recordOffset > maxRecordSize)
                {
                  maxRecordSize = recordOffset;
                }
            }

          /* Verify that the <field list> is enclosed in parentheses */

          if (g_token == ')') getToken();
          else error(eRPAREN);

          /* A semicolon at this position means that another <variant>
           * follows.  Keep looping until all of the variants have been
           * processed (i.e., no semi-colon)
           *
           * Hmm.. real world examples that I have seen do allow a semi-colon
           * on the final variant.  So will I.
           */

          if (g_token == ';')
            {
              getToken();
            }
          else
            {
              break;
            }

          if (g_token == tEND)
            {
              break;
            }
        }

      /* Update the RECORD entry for the maximum size of all variants */

      recordPtr->sParm.t.tAllocSize = maxRecordSize;
    }

  /* Verify that the RECORD declaration terminates with END */

  if (g_token != tEND) error(eRECORDDECLARE);
  else getToken();

  return recordPtr;
}

/****************************************************************************/

static symbol_t *pas_DeclareField(symbol_t *recordPtr, symbol_t *lastField)
{
  symbol_t *fieldPtr = NULL;
  symbol_t *typePtr  = NULL;

  TRACE(g_lstFile,"[pas_DeclareField]");

  /* Declare one record-section with a record.
   * FORM: record-section = identifier-list ':' type-denoter
   * FORM: identifier-list = identifier { ',' identifier }
   */

  if (g_token != tIDENT) error(eIDENT);
  else
    {
      /* Declare a <field> with this <identifier> as its name */

      fieldPtr = pas_AddField(g_tokenString, recordPtr, lastField);
      getToken();

      /* Check for multiple fields of this <type> */

      if (g_token == ',')
        {
          symbol_t *nextPtr;

          getToken();
          nextPtr = pas_DeclareField(recordPtr, fieldPtr);
          if (nextPtr != NULL)
            {
              typePtr = nextPtr->sParm.r.rParent;
            }
        }
      else
        {
          if (g_token != ':') error(eCOLON);
          else getToken();

          /* Use the existing type or declare a new type with no name */

          typePtr = pas_TypeDenoter(NULL);
        }

      recordPtr->sParm.t.tMaxValue++;
      if (typePtr != NULL)
        {
          /* Copy the size of field from the sTYPE entry into the <field>
           * type entry.  NOTE:  This element is not essential since it
           * can be obtained from the parent type pointer.
           */

          fieldPtr->sParm.r.rSize    = typePtr->sParm.t.tAllocSize;

          /* Save a pointer back to the parent field type */

          fieldPtr->sParm.r.rParent  = typePtr;
        }
    }

  return fieldPtr;
}

/****************************************************************************/
/* Process VAR/value Parameter Declaration
 *
 * NOTE:  This function increments the global variable g_nParms
 * as a side-effect
 */

static symbol_t *pas_DeclareParameter(bool pointerType)
{
   int16_t   varType = 0;
   symbol_t *varPtr;
   symbol_t *typePtr;

   TRACE(g_lstFile,"[pas_DeclareParameter]");

   /* FORM:
    * <identifier>[,<identifier>[,<identifier>[...]]] : <type identifier>
    */

   if (g_token != tIDENT) error(eIDENT);
   else
     {
       /* Set up for this formal parameter */

       varPtr = pas_AddVariable(g_tokenString, sINT, 0, sINT_SIZE, NULL);

       /* The parameter name may be followed by either ',' or ':' */

       getToken();

       if (g_token == ',')
         {
           /* Get the next formal parameter name.  The search is
            * limited to the current level.
            */

           getLevelToken();
           typePtr = pas_DeclareParameter(pointerType);
         }
       else
         {
           /* Check for a type identifier */

           if (g_token != ':') error(eCOLON);
           else getToken();

           /* Get the type-identifier following the colon.  After calling
            * pas_TypeIdentifier(), g_token should refer to the ',' or ')' in
            * the formal parameter list.
            */

           typePtr = pas_TypeIdentifier();
           if (typePtr == NULL) error(eINVTYPE);
         }

       if (pointerType)
         {
           varType     = sVAR_PARM;
           g_dwVarSize = sPTR_SIZE;
         }
       else
         {
           varType = typePtr->sParm.t.tType;
         }

       g_nParms++;
       varPtr->sKind           = varType;
       varPtr->sParm.v.vSize   = g_dwVarSize;
       varPtr->sParm.v.vParent = typePtr;
     }

   return typePtr;
}

/****************************************************************************/

static void pas_AddRecordInitializers(symbol_t *varPtr, symbol_t *typePtr)
{
  symbol_t *recordTypePtr = varPtr->sParm.v.vParent;

  if (recordTypePtr == NULL ||
      recordTypePtr->sKind != sTYPE ||
      recordTypePtr->sParm.t.tType != sRECORD)
    {
      error(eRECORDTYPE);
    }

  /* Looks like a good RECORD type */

  else
    {
      symbol_t *recordObjectPtr;
      int nObjects = recordTypePtr->sParm.t.tMaxValue;
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

          /* If this field is a string, then set up to initialize
           * it.
           */

          parentTypePtr = recordObjectPtr->sParm.r.rParent;

          if (parentTypePtr == NULL || parentTypePtr->sKind != sTYPE)
            {
              error(eHUH);
            }
          else if (parentTypePtr->sParm.t.tType == sSTRING ||
                   parentTypePtr->sParm.t.tType == sSHORTSTRING)
            {
               pas_AddRecordObjectInitializer(varPtr, recordObjectPtr);
            }
          else if (parentTypePtr->sParm.t.tType == sFILE ||
                   parentTypePtr->sParm.t.tType == sTEXTFILE)
            {
               pas_AddRecordObjectInitializer(varPtr, recordObjectPtr);
            }
          else if (parentTypePtr->sParm.t.tType == sARRAY)
            {
              symbol_t varInfo;

              /* "Fake" a variable symbol at the offset of the array */

              varInfo.sName             = varPtr->sName;
              varInfo.sKind             = parentTypePtr->sParm.t.tType;
              varInfo.sLevel            = varPtr->sLevel;

              varInfo.sParm.v.vFlags    = varPtr->sParm.v.vFlags;
              varInfo.sParm.v.vXfrUnit  = varPtr->sParm.v.vXfrUnit;
              varInfo.sParm.v.vOffset   = varPtr->sParm.v.vOffset;
              varInfo.sParm.v.vSize     = parentTypePtr->sParm.t.tAllocSize;
              varInfo.sParm.v.vSymIndex = 0;
              varInfo.sParm.v.vParent   = parentTypePtr;

              pas_AddArrayInitializers(&varInfo, parentTypePtr);
            }
        }
    }
}

/****************************************************************************/

static void pas_AddArrayInitializers(symbol_t *varPtr, symbol_t *typePtr)
{
  symbol_t *nextPtr;
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

  nextPtr         = typePtr->sParm.t.tParent;
  baseTypePtr     = nextPtr;

  while (nextPtr != NULL && nextPtr->sKind == sTYPE)
    {
      baseTypePtr = nextPtr;
      nextPtr     = baseTypePtr->sParm.t.tParent;
    }

  if (baseTypePtr->sParm.t.tType == sFILE        ||
      baseTypePtr->sParm.t.tType == sTEXTFILE    ||
      baseTypePtr->sParm.t.tType == sSTRING      ||
      baseTypePtr->sParm.t.tType == sSHORTSTRING ||
      baseTypePtr->sParm.t.tType == sRECORD)
    {
      symbol_t  varInfo;
      symbol_t *indexPtr;
      int       nElements;
      int       index;

      /* "Fake" a variable symbol at the offset of the array */

      varInfo.sName             = varPtr->sName;
      varInfo.sKind             = baseTypePtr->sParm.t.tType;
      varInfo.sLevel            = varPtr->sLevel;

      varInfo.sParm.v.vFlags    = varPtr->sParm.v.vFlags;
      varInfo.sParm.v.vXfrUnit  = varPtr->sParm.v.vXfrUnit;
      varInfo.sParm.v.vOffset   = varPtr->sParm.v.vOffset;
      varInfo.sParm.v.vSize     = baseTypePtr->sParm.t.tAllocSize;
      varInfo.sParm.v.vSymIndex = 0;
      varInfo.sParm.v.vParent   = baseTypePtr;

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
          switch (baseTypePtr->sParm.t.tType)
            {
              case sFILE :
              case sTEXTFILE :
                pas_AddFileInitializer(&varInfo, false, 0);
                break;

              case sSTRING :
              case sSHORTSTRING :
                pas_AddStringInitializer(&varInfo);
                break;

              case sRECORD :
                pas_AddRecordInitializers(&varInfo, baseTypePtr);
                break;

              default:
                error(eHUH);
                break;
            }

          /* Set the offset to the next array element.
           * REVISIT:  If the array is not packed, then need to align the
           * size.
           */

          varInfo.sParm.v.vOffset += baseTypePtr->sParm.t.tAllocSize;
        }
    }
}

/****************************************************************************/

static bool pas_IntAlignRequired(symbol_t *typePtr)
{
  bool returnValue = false;

  /* Type CHAR and ARRAYS of CHAR do not require alignment (unless
   * they are passed as value parameters).  Otherwise, alignment
   * to type INTEGER boundaries is required.
   */

  if (typePtr)
    {
      if (typePtr->sKind == sCHAR)
        {
          returnValue = true;
        }
      else if (typePtr->sKind == sARRAY)
        {
          typePtr = typePtr->sParm.t.tParent;
          if (typePtr != NULL && typePtr->sKind == sCHAR)
            {
              returnValue = true;
            }
        }
    }

  return returnValue;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/* Process BLOCK.  This function implements:
 *
 * block = declaration-group compound-statement
 *
 * Where block can appear in the followinging:
 *
 * function-block = block
 * function-declaration =
 *     function-heading ';' directive |
 *     function-heading ';' function-block
 *
 * procedure-block = block
 * procedure-declaration =
 *     procedure-heading ';' directive |
 *     procedure-heading ';' procedure-block
 *
 * program = program-heading ';' [ uses-section ] block '.'
 */

void pas_Block(int32_t preAllocatedDStack)
{
  uint16_t beginLabel;                /* BEGIN label */
  int32_t saveDStack;                 /* Saved DSEG size */
  char   *saveStringSP;               /* Saved top of string stack */
  unsigned int saveNSym;              /* Saved top of symbol table */
  unsigned int saveNConst;            /* Saved top of constant table */
  unsigned int saveNInitializer;      /* Saved top of initializer stack */
  unsigned int saveSymOffset;         /* Saved previous level symbol offset */
  unsigned int saveConstOffset;       /* Saved previous level constant offset */
  unsigned int saveInitializerOffset; /* Saved previous level initializer offset */

  TRACE(g_lstFile,"[pas_Block]");

  /* Set the begin label number */

  beginLabel              = ++g_label;

  /* Save the DSEG and string stack sizes.  These will grow within the block,
   * but we use these saved values to restore the sizes at the END of the
   * block.
   */

  saveDStack              = g_dStack;
  saveStringSP            = g_stringSP;

  /* Save the current top-of-stack data for symbols, constants, but not
   * initializers (we'll get those later).
   */

  saveNSym                 = g_nSym;
  saveNConst               = g_nConst;
  saveNInitializer         = g_nInitializer;

  saveSymOffset            = g_levelSymOffset;
  saveConstOffset          = g_levelConstOffset;
  saveInitializerOffset    = g_levelInitializerOffset;

  /* Set the current symbol and constant table offsets for this level */

  g_levelSymOffset         = saveNSym;
  g_levelConstOffset       = saveNConst;
  g_levelInitializerOffset = saveNInitializer;

  /* When we enter block at level zero, then we must be at the
   * entry point to the program.  Save the entry point label
   * in the POFF file.
   */

  if (g_level == 0 && FP0->kind == eIsProgram)
    {
      poffSetEntryPoint(g_poffHandle, g_label);
    }

  /* Init size of the new DSEG.  Normally nothing is preallocated on the
   * stack.  The exception is at the program level where the INPUT and OUTPUT
   * (and maybe other) file variables are created.
   */

  g_dStack = preAllocatedDStack;

  /* FORM: block = declaration-group compound-statement
   * Process the declaration-group
   *
   * declaration-group =
   *     label-declaration-group |
   *     constant-definition-group |
   *     type-definition-group |
   *     variable-declaration-group |
   *     function-declaration  |
   *     procedure-declaration
   */

  pas_DeclarationGroup(beginLabel);

  /* Process the compound-statement
   *
   * FORM: compound-statement = 'begin' statement-sequence 'end'
   */

  /* Verify that the compound-statement begins with BEGIN */

  if (g_token != tBEGIN)
    {
      error(eBEGIN);
    }

  /* It may be necessary to jump around some local functions to
   * get to the main body of the block.  If any jumps are generated,
   * they will come to the beginLabel emitted here.
   */

  pas_GenerateDataOperation(opLABEL, (int32_t)beginLabel);

  /* Since we don't know for certain how we got here, invalidate
   * the level stack pointer (LSP).  This is, of course, only
   * meaningful on architectures that implement an LSP.
   */

  pas_InvalidateCurrentStackLevel();

  /* Allocate data stack */

  if (g_dStack)
    {
      /* Make sure that the data stack is aligned */

      g_dStack = INT_ALIGNUP(g_dStack);
      pas_GenerateDataOperation(opINDS, (int32_t)g_dStack);
    }

  /* Generate the initializers */

  g_levelInitializerOffset = saveInitializerOffset;
  pas_Initialization();

  /* Then emit the compound statement itself */

  g_levelInitializerOffset = g_nInitializer;
  pas_CompoundStatement();

  /* Release the allocated data stack */

  if (g_dStack)
    {
      pas_GenerateDataOperation(opINDS, -(int32_t)g_dStack);
    }

  /* Generate finalizers */

  g_levelInitializerOffset = saveInitializerOffset;
  pas_Finalization();

  /* Make sure all declared labels were defined in the block */

  pas_VerifyLabels(saveNSym);

  /* "Pop" declarations local to this block */

  g_dStack                 = saveDStack;       /* Restore old DSEG size */
  g_stringSP               = saveStringSP;     /* Restore top of string stack */

  /* Restore the symbol and constant table offsets for the previous level. */

  g_levelSymOffset         = saveSymOffset;
  g_levelConstOffset       = saveConstOffset;
  g_levelInitializerOffset = saveInitializerOffset;

  /* Release the symbols, constants, and initializers used by this level */

  g_nSym                   = saveNSym;         /* Restore top of symbol table */
  g_nConst                 = saveNConst;       /* Restore top of constant table */
  g_nInitializer           = saveNInitializer; /* Restore top of initializers */
}

/****************************************************************************/
/* Process declarative-part */

void pas_DeclarationGroup(int32_t beginLabel)
{
  unsigned int notFirst = 0;          /* Init count of nested procs */
  unsigned int saveSymOffset;         /* Saved previous level symbol offset */
  unsigned int saveConstOffset;       /* Saved previous level constant offset */
  unsigned int saveInitializerOffset; /* Saved previous level initializer offset */

  TRACE(g_lstFile,"[pas_DeclarationGroup]");

  /* Save the current top-of-stack data for symbols and constants. */

  saveSymOffset      = g_levelSymOffset;
  saveConstOffset    = g_levelConstOffset;

  /* Set the current symbol, constant, and initializer table offsets for this
   * level
   */

  g_levelSymOffset   = g_nSym;
  g_levelConstOffset = g_nConst;

  /* FORM: declarative-part = { declaration-group }
   * FORM: declaration-group =
   *       label-declaration-group | constant-definition-group |
   *       type-definition-group   | variable-declaration-group |
   *       function-declaration    | procedure-declaration
   */

  /* Process label-declaration-group.
   * FORM: label-declaration-group = 'label' label { ',' label } ';'
   */

  if (g_token == tLABEL) pas_DeclareLabel();

  /* Process constant-definition-group.
   * FORM: constant-definition-group =
   *       'const' constant-definition ';' { constant-definition ';' }
   */

  if (g_token == tCONST)
    {
      /* Limit search to present level */

      getLevelToken();

      /* Process constant-definition.
       * FORM: constant-definition = identifier '=' constant
       */

      pas_ConstantDefinitionGroup();
    }

  /* Process type-definition-group
   * FORM: type-definition-group =
   *       'type' type-definition ';' { type-definition ';' }
   */

  if (g_token == tTYPE)
    {
      /* Limit search to present level */

      getLevelToken();

      /* Process the type-definitions in the type-definition-group
       * FORM: type-definition = identifier '=' type-denoter
       */

      pas_TypeDefinitionGroup();
    }

  /* Process variable-declaration-group
   * FORM: variable-declaration-group =
   *       'var' variable-declaration { ';' variable-declaration }
   */

  if (g_token == tVAR)
    {
      /* Limit search to present level */

      getLevelToken();

      /* Process the variable declarations
       * FORM: variable-declaration = identifier-list ':' type-denoter
       * FORM: identifier-list = identifier { ',' identifier }
       */

      pas_VariableDeclarationGroup();
    }

  /* Process procedure/function-declaration(s) if present
   * FORM: function-declaration =
   *       function-heading ';' directive |
   *       function-heading ';' function-block
   * FORM: procedure-declaration =
   *       procedure-heading ';' directive |
   *       procedure-heading ';' procedure-block
   *
   * NOTE:  a JMP to the executable body of this block is generated
   * if there are nested procedures and this is not level=0
   */

  saveInitializerOffset    = g_levelInitializerOffset;
  g_levelInitializerOffset = g_nInitializer;

  for (; ; )
    {
      /* FORM: function-heading =
       *       'function' identifier [ formal-parameter-list ] ':' result-type
       */

      if (g_token == tFUNCTION)
        {
          /* Check if we need to put a jump around the function */

          if ((beginLabel > 0) && !(notFirst) && (g_level > 0))
            {
              pas_GenerateDataOperation(opJMP, (int32_t)beginLabel);
            }

          /* Get the procedure-identifier */
          /* Limit search to present level */

          getLevelToken();

          /* Define the function */

          pas_FunctionDeclaration();
          notFirst++;                 /* No JMP next time */
        }

      /* FORM: procedure-heading =
       *       'procedure' identifier [ formal-parameter-list ]
       */

      else if (g_token == tPROCEDURE)
        {
          /* Check if we need to put a jump around the function */

          if ((beginLabel > 0) && !(notFirst) && (g_level > 0))
            {
              pas_GenerateDataOperation(opJMP, (int32_t)beginLabel);
            }

          /* Get the procedure-identifier */
          /* Limit search to present level */

          getLevelToken();

          /* Define the procedure */

          pas_ProcedureDeclaration();
          notFirst++;                 /* No JMP next time */
        }
      else
       {
         break;
       }
    }

  /* Restore the symbol/constant table offsets for the previous level */

  g_levelSymOffset         = saveSymOffset;
  g_levelConstOffset       = saveConstOffset;
  g_levelInitializerOffset = saveInitializerOffset;
}

/****************************************************************************/

void pas_ConstantDefinitionGroup(void)
{
  /* Process constant-definition-group.
   * FORM: constant-definition-group =
   *       'const' constant-definition ';' { constant-definition ';' }
   * FORM: constant-definition = identifier '=' constant
   *
   * On entry, g_token should point to the identifier of the first
   * constant-definition.
   */

  for (; ; )
    {
      if (g_token == tIDENT)
        {
          pas_DeclareConst();
          if (g_token != ';') break;
          else getToken();
        }
      else break;
    }
}

/****************************************************************************/

void pas_TypeDefinitionGroup(void)
{
  char   *typeName;

  /* Process type-definition-group
   * FORM: type-definition-group =
   *       'type' type-definition ';' { type-definition ';' }
   * FORM: type-definition = identifier '=' type-denoter
   *
   * On entry, g_token refers to the first identifier (if any) of
   * the type-definition list.
   */

  for (; ; )
    {
      if (g_token == tIDENT)
        {
          /* Save the type identifier */

          typeName = g_tokenString;
          getToken();

          /* Verify that '=' follows the type identifier */

          if (g_token != '=') error(eEQ);
          else getToken();

          (void)pas_DeclareType(typeName);
          if (g_token != ';') break;
          else getToken();

        }
      else break;
    }
}

/****************************************************************************/

void pas_VariableDeclarationGroup(void)
{
   /* Process variable-declaration-group
    * FORM: variable-declaration-group =
    *       'var' variable-declaration { ';' variable-declaration }
    * FORM: variable-declaration = identifier-list ':' type-denoter
    * FORM: identifier-list = identifier { ',' identifier }
    *
    * Only entry, g_token holds the first identfier (if any) of the
    * variable-declaration list.
    */

  while (g_token == tIDENT)
    {
      (void)pas_DeclareVar();
      if (g_token != ';') break;
      else getToken();
    }
}

/****************************************************************************/
/* Process formal-parameter-list */

int16_t pas_FormalParameterList(symbol_t *procPtr)
{
  int16_t parameterOffset;
  int16_t i;
  bool    pointerType;

  TRACE(g_lstFile,"[pas_FormalParameterList]");

  /* FORM: formal-parameter-list =
   *       '(' formal-parameter-section { ';' formal-parameter-section } ')'
   * FORM: formal-parameter-section =
   *       value-parameter-specification |
   *       variable-parameter-specification |
   *       procedure-parameter-specification |
   *       function-parameter-specification
   * FORM: value-parameter-specification =
   *       identifier-list ':' type-identifier
   * FORM: variable-parameter-specification =
   *       'var' identifier-list ':' type-identifier
   *
   * On entry g_token should refer to the '(' at the beginning of the
   * (optional) formal parameter list.
   */

  g_nParms = 0;

  /* Check if the formal-parameter-list is present.  It is optional in
   * all contexts in which this function is called.
   */

  if (g_token == '(')
    {
      /* Process each formal-parameter-section */

      do
        {
          /* Get the formal parameter name.  Symbol search is restricted
           * to the current level.
           */

          getLevelToken();

          /* Check for variable-parameter-specification */

          if (g_token == tVAR)
            {
              pointerType = 1;
              getLevelToken();
            }
          else
            {
              pointerType = 0;
            }

          /* Process the common part of the variable-parameter-specification
           * and the value-parameter specification.
           * NOTE that procedure-parameter-specification and
           * function-parameter-specification are not yet supported.
           */

          (void)pas_DeclareParameter(pointerType);
        }
      while (g_token == ';');

      /* Verify that the formal parameter list terminates with a
       * right parenthesis.
       */

      if (g_token != ')') error(eRPAREN);
      else getToken();
    }

  /* Save the number of parameters found in sPROC/sFUNC symbol table entry */

  procPtr->sParm.p.pNParms = g_nParms;

  /* Now, calculate the parameter offsets from the size of each parameter */

  parameterOffset = -sRETURN_SIZE;
  for (i = g_nParms; i > 0; i--)
    {
      /* The offset to the next parameter is the offset to the previous
       * parameter minus the size of the new parameter (aligned to
       * multiples of size of INTEGER).
       */

      parameterOffset -= procPtr[i].sParm.v.vSize;
      parameterOffset  = INT_ALIGNUP(parameterOffset);
      procPtr[i].sParm.v.vOffset = parameterOffset;
    }

  return parameterOffset;
}
