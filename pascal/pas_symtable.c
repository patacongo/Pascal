/****************************************************************************
 * pas_symtable.c
 * Table Management Package
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

#include <sys/types.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <strings.h>

#include "config.h"
#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_errcodes.h"

#include "pas_main.h"
#include "pas_initializer.h" /* for pas_AddFileInitializer() */
#include "pas_symtable.h"
#include "pas_error.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct symbolAlias_s
{
  const char *alt;
  const char *rsw;
};

typedef struct symbolAlias_s symbolAlias_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static symbol_t *addSymbol(char *name, int16_t type);

/****************************************************************************
 * Public Data
 ****************************************************************************/

symbol_t    *g_parentInteger = NULL;
symbol_t    *g_parentString  = NULL;
symbol_t    *g_inputFile     = NULL;  /* Shortcut to the INPUT file symbol */
symbol_t    *g_outputFile    = NULL;  /* Shortcut to the OUTPUT file symbol */
unsigned int g_nSym          = 0;     /* Number symbol table entries */
unsigned int g_nConst        = 0;     /* Number constant table entries */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* NOTES in the following:
 * (1) Standard or extended Pascal reserved word
 * (2) Standard or extended Pascal function
 * (3) Standard or extended Pascal procedure
 * (4) Extended Pascal reserved word
 * (5) Non-standard Pascal function
 * (6) Non-standard Pascal procedure
 * (7) Built-In Function
 * (8) Borland-style and/or Free Pascal string/file operations
 */

static const reservedWord_t g_rsw[] =                /* Reserved word list */
{
  {"ABS",            tSTDFUNC,        txABS},        /* (2) */
  {"ADDR",           tSTDFUNC,        txADDR},       /* (2) */
  {"AND",            tAND,            txNONE},       /* (1) */
  {"APPEND",         tSTDPROC,        txAPPEND},     /* (3) */
  {"ARCTAN",         tSTDFUNC,        txARCTAN},     /* (2) */
  {"ARRAY",          tARRAY,          txNONE},       /* (1) */
  {"ASSIGNFILE",     tSTDPROC,        txASSIGNFILE}, /* (3) */
  {"BEGIN",          tBEGIN,          txNONE},       /* (1) */
  {"CARD",           tSTDFUNC,        txCARD},       /* (2) */
  {"CASE",           tCASE,           txNONE},       /* (1) */
  {"CHDIR",          tSTDPROC,        txCHDIR},      /* (8) */
  {"CHR",            tSTDFUNC,        txCHR},        /* (2) */
  {"CLOSEDIR",       tSTDFUNC,        txCLOSEDIR},   /* (8) */
  {"CLOSEFILE",      tSTDPROC,        txCLOSEFILE},  /* (3) */
  {"CONCAT",         tSTDFUNC,        txCONCAT},     /* (8) */
  {"CONST",          tCONST,          txNONE},       /* (1) */
  {"COPY",           tSTDFUNC,        txCOPY},       /* (8) */
  {"COS",            tSTDFUNC,        txCOS},        /* (2) */
  {"CREATEDIR",      tSTDFUNC,        txCREATEDIR},  /* (8) */
  {"DELETE",         tSTDPROC,        txDELETE},     /* (8) */
  {"DISPOSE",        tSTDPROC,        txDISPOSE},    /* (3) */
  {"DIV",            tDIV,            txNONE},       /* (1) */
  {"DO",             tDO,             txNONE},       /* (1) */
  {"DOWNTO",         tDOWNTO,         txNONE},       /* (1) */
  {"ELSE",           tELSE,           txNONE},       /* (1) */
  {"END",            tEND,            txNONE},       /* (1) */
  {"EOF",            tSTDFUNC,        txEOF},        /* (2) */
  {"EOLN",           tSTDFUNC,        txEOLN},       /* (2) */
  {"EXCLUDE",        tSTDFUNC,        txEXCLUDE},    /* (2) */
  {"EXIT",           tSTDPROC,        txEXIT},       /* (3) */
  {"EXP",            tSTDFUNC,        txEXP},        /* (2) */
  {"FILE",           tFILE,           txNONE},       /* (1) */
  {"FILEPOS",        tSTDFUNC,        txFILEPOS},    /* (2) */
  {"FILESIZE",       tSTDFUNC,        txFILESIZE},   /* (2) */
  {"FILLCHAR",       tSTDPROC,        txFILLCHAR},   /* (8) */
  {"FINALIZATION",   tFINALIZATION,   txNONE},       /* (4) */
  {"FOR",            tFOR,            txNONE},       /* (1) */
  {"FUNCTION",       tFUNCTION,       txNONE},       /* (1) */
  {"GET",            tSTDPROC,        txGET},        /* (3) */
  {"GETDIR",         tSTDFUNC,        txGETDIR},     /* (8) */
  {"GETENV",         tSTDFUNC,        txGETENV},     /* (5) */
  {"GOTO",           tGOTO,           txNONE},       /* (1) */
  {"HALT",           tSTDPROC,        txHALT},       /* (3) */
  {"IF",             tIF,             txNONE},       /* (1) */
  {"IMPLEMENTATION", tIMPLEMENTATION, txNONE},       /* (1) */
  {"IN",             tIN,             txNONE},       /* (1) */
  {"INCLUDE",        tSTDFUNC,        txINCLUDE},    /* (2) */
  {"INITIALIZATION", tINITIALIZATION, txNONE},       /* (1) */
  {"INSERT",         tSTDPROC,        txINSERT},     /* (8) */
  {"INTERFACE",      tINTERFACE,      txNONE},       /* (1) */
  {"LABEL",          tLABEL,          txNONE},       /* (1) */
  {"LENGTH",         tBUILTIN,        txLENGTH},     /* (8) */
  {"LN",             tSTDFUNC,        txLN},         /* (2) */
  {"MKDIR",          tSTDPROC,        txMKDIR},      /* (8) */
  {"MOD",            tMOD,            txNONE},       /* (1) */
  {"NEW",            tSTDPROC,        txNEW},        /* (3) */
  {"NOT",            tNOT,            txNONE},       /* (1) */
  {"ODD",            tSTDFUNC,        txODD},        /* (2) */
  {"OF",             tOF,             txNONE},       /* (1) */
  {"OPENDIR",        tSTDFUNC,        txOPENDIR},    /* (8) */
  {"OR",             tOR,             txNONE},       /* (1) */
  {"ORD",            tSTDFUNC,        txORD},        /* (2) */
  {"PACK",           tSTDPROC,        txPACK},       /* (3) */
  {"PACKED",         tPACKED,         txNONE},       /* (1) */
  {"PAGE",           tSTDPROC,        txPAGE},       /* (3) */
  {"POS",            tSTDFUNC,        txPOS},        /* (8) */
  {"PRED",           tSTDFUNC,        txPRED},       /* (2) */
  {"PROCEDURE",      tPROCEDURE,      txNONE},       /* (1) */
  {"PROGRAM",        tPROGRAM,        txNONE},       /* (1) */
  {"PUT",            tSTDPROC,        txPUT},        /* (3) */
  {"READ",           tSTDPROC,        txREAD},       /* (3) */
  {"READDIR",        tSTDFUNC,        txREADDIR},    /* (8) */
  {"READLN",         tSTDPROC,        txREADLN},     /* (3) */
  {"RECORD",         tRECORD,         txNONE},       /* (1) */
  {"REMOVEDIR",      tSTDFUNC,        txREMOVEDIR},  /* (8) */
  {"REPEAT",         tREPEAT,         txNONE},       /* (1) */
  {"RESET",          tSTDPROC,        txRESET},      /* (3) */
  {"REWINDDIR",      tSTDFUNC,        txREWINDDIR},  /* (8) */
  {"REWRITE",        tSTDPROC,        txREWRITE},    /* (3) */
  {"RMDIR",          tSTDPROC,        txRMDIR},      /* (8) */
  {"ROUND",          tSTDFUNC,        txROUND},      /* (2) */
  {"SEEK",           tSTDPROC,        txSEEK},       /* (3) */
  {"SEEKEOF",        tSTDFUNC,        txSEEKEOF},    /* (2) */
  {"SEEKEOLN",       tSTDFUNC,        txSEEKEOLN},   /* (2) */
  {"SET",            tSET,            txNONE},       /* (1) */
  {"SETCURRENTDIR",  tSTDFUNC,        txSETCURRDIR}, /* (8) */
  {"SHL",            tSHL,            txNONE},       /* (4) */
  {"SHR",            tSHR,            txNONE},       /* (4) */
  {"SIN",            tSTDFUNC,        txSIN},        /* (2) */
  {"SIZEOF",         tBUILTIN,        txSIZEOF},     /* (7) */
  {"SQR",            tSTDFUNC,        txSQR},        /* (2) */
  {"SQRT",           tSTDFUNC,        txSQRT},       /* (2) */
  {"STR",            tSTDPROC,        txSTR},        /* (8) */
  {"SUCC",           tSTDFUNC,        txSUCC},       /* (2) */
  {"THEN",           tTHEN,           txNONE},       /* (1) */
  {"TO",             tTO,             txNONE},       /* (1) */
  {"TRUNC",          tSTDFUNC,        txTRUNC},      /* (2) */
  {"TYPE",           tTYPE,           txNONE},       /* (1) */
  {"UNIT",           tUNIT,           txNONE},       /* (1) */
  {"UNPACK",         tSTDPROC,        txUNPACK},     /* (3) */
  {"UNTIL",          tUNTIL,          txNONE},       /* (1) */
  {"USES",           tUSES,           txNONE},       /* (1) */
  {"VAL",            tSTDPROC,        txVAL},        /* (8) */
  {"VAR",            tVAR,            txNONE},       /* (1) */
  {"WHILE",          tWHILE,          txNONE},       /* (1) */
  {"WITH",           tWITH,           txNONE},       /* (1) */
  {"WRITE",          tSTDPROC,        txWRITE},      /* (3) */
  {"WRITELN",        tSTDPROC,        txWRITELN},    /* (3) */
  {"XOR",            tXOR,            txNONE},       /* (1) */
  {NULL,             0,               txNONE}        /* List terminator */
};

static symbol_t *g_symbolTable;                      /* Symbol Table */

/* The g_aliasTable[] allows support for different versions of pascal source
 * files that differ only in naming.  This mapping also supports substituions
 * in the case where features or types are not supported by the compiler.
 */

static const symbolAlias_t g_aliasTable[] =
{
  {"ASSIGN",        "ASSIGNFILE"},
  {"CLOSE",         "CLOSEFILE"},
  {"GETCURRENTDIR", "GETDIR"},
  {"INT64",         "LONGINTEGER"},
  {"LONGINT",       "LONGINTEGER"},
  {"SHORTINT",      "SHORTINTEGER"},
  {"TEXT",          "TEXTFILE"},
  {NULL,       NULL}
};

/****************************************************************************/

const char *pas_MapToAlias(const char *name)
{
  const symbolAlias_t *ptr;               /* Point into symbol alias list */
  int16_t cmp;                            /* 0=equal; >0=past it */

  /* Try each alias */

  for (ptr = g_aliasTable; ptr->alt != NULL; ptr++)
    {
      /* Check if the identifier matches a reserved alias */

      cmp = strcasecmp(ptr->alt, name);
      if (!cmp)
        {
          /* Return the mapped reserved word */

          return ptr->rsw;
        }

      /* Exit early if we are past the possible matches */

      else if (cmp > 0)
        {
          break;
        }
    }

  /* Return the original name if no match */

  return name;
}

/****************************************************************************/

const reservedWord_t *pas_FindReservedWord(const char *name)
{
  const reservedWord_t *ptr;              /* Point into reserved word list */
  int16_t cmp;                            /* 0=equal; >0=past it */

  /* Try each each reserved word */

  for (ptr = g_rsw; ptr->rname != NULL; ptr++)
    {
      /* Check if the identifier matches a reserved word */

      cmp = strcasecmp(ptr->rname, name);
      if (!cmp)
        {
          /* Return pointer to entry if match */

          return ptr;
        }

      /* Exit early if we are past the possible matches */

      else if (cmp > 0)
        {
          break;
        }
    }

  /* return NULL pointer if no match */

  return (reservedWord_t *)NULL;
}

/****************************************************************************/

symbol_t *pas_FindSymbol(const char *inName, int tableOffset)
{
  int i;

  for (i = g_nSym - 1; i >= tableOffset; i--)
    {
      if (g_symbolTable[i].sName)
        {
          if (!strcasecmp(g_symbolTable[i].sName, inName))
            {
              return &g_symbolTable[i];
            }
        }
    }

  return (symbol_t *)NULL;
}

/****************************************************************************/

static symbol_t *addSymbol(char *name, int16_t kind)
{
  /* Check for Symbol Table overflow */

  if (g_nSym >= MAX_SYM)
    {
      fatal(eOVF);
      return (symbol_t *)NULL;
    }
  else
    {
      /* Clear all elements of the symbol table entry */

      memset(&g_symbolTable[g_nSym], 0, sizeof(symbol_t));

      /* Set the elements which are independent of sKind */

      g_symbolTable[g_nSym].sName  = name;
      g_symbolTable[g_nSym].sKind  = kind;
      g_symbolTable[g_nSym].sLevel = g_level;

      return &g_symbolTable[g_nSym++];
    }
}

/****************************************************************************/

symbol_t *pas_AddTypeDefine(char *name, uint8_t type, uint16_t size,
                            symbol_t *parent)
{
  symbol_t *typePtr;

  /* Get a slot in the symbol table */

  typePtr = addSymbol(name, sTYPE);
  if (typePtr)
    {
      /* Add the type definition to the symbol table
       * NOTES:
       * 1. The minValue and maxValue fields (for scalar and subrange)
       *    types must be set external to this function
       * 2. We assume that there are no special flags associated with
       *    the type.
       * 3. Additional external settings are necessary for ARRAY types
       *    as well (tDimension, tIndex).
       */

      typePtr->sParm.t.tType      = type;
      typePtr->sParm.t.tAllocSize = size;
      typePtr->sParm.t.tParent    = parent;
    }

  /* Return a pointer to the new constant symbol */

  return typePtr;
}

/****************************************************************************/

symbol_t *pas_AddConstant(char *name, uint8_t type, int32_t *value,
                          symbol_t *parent)
{
  symbol_t *constPtr;

  /* Get a slot in the symbol table */

  constPtr = addSymbol(name, type);
  if (constPtr)
    {
      /* Add the value of the constant to the symbol table */

      if (type == tREAL_CONST)
        {
          constPtr->sParm.c.cValue.f = *((double *)value);
        }
      else
        {
          constPtr->sParm.c.cValue.i = *value;
        }

      constPtr->sParm.c.cParent = parent;
    }

  /* Return a pointer to the new constant symbol */

  return constPtr;
}

/****************************************************************************/

symbol_t *pas_AddStringConstant(char *name, uint32_t offset, uint32_t size)
{
  symbol_t *stringPtr;

  /* Get a slot in the symbol table */

  stringPtr = addSymbol(name, sSTRING_CONST);
  if (stringPtr)
    {
      /* Add the value of the constant to the symbol table */

      stringPtr->sParm.s.roOffset = offset;
      stringPtr->sParm.s.roSize   = size;
    }

  /* Return a pointer to the new string symbol */

  return stringPtr;
}

/****************************************************************************/

symbol_t *pas_AddFile(char *name, uint16_t kind, uint16_t offset,
                      uint16_t xfrUnit, symbol_t *typePtr)
{
  symbol_t *filePtr;

  /* Get a slot in the symbol table */

  filePtr = addSymbol(name, kind);
  if (filePtr)
    {
      /* Add the file to the symbol table */

      filePtr->sParm.v.vXfrUnit   = xfrUnit;    /* Size of each transfer (binary) */
      filePtr->sParm.v.vOffset    = offset;     /* Offset to variable */
      filePtr->sParm.v.vSize      = sINT_SIZE;  /* Run time storage size */
      filePtr->sParm.v.vParent    = typePtr;    /* FILE OF Type (binary) */
    }

  /* Return a pointer to the new file variable symbol */

  return filePtr;
}

/****************************************************************************/

symbol_t *pas_AddProcedure(char *name, uint8_t type, uint16_t label,
                           uint16_t nParms, symbol_t *parent)
{
  symbol_t *procPtr;

  /* Get a slot in the symbol table */

  procPtr = addSymbol(name, type);
  if (procPtr)
    {
      /* Add the procedure/function definition to the symbol table */

      procPtr->sParm.p.pLabel    = label;
      procPtr->sParm.p.pNParms   = nParms;
      procPtr->sParm.p.pFlags    = 0;
      procPtr->sParm.p.pSymIndex = 0;
      procPtr->sParm.p.pParent   = parent;
    }

  /* Return a pointer to the new procedure/function symbol */

  return procPtr;
}

/****************************************************************************/

symbol_t *pas_AddVariable(char *name, uint8_t type, uint16_t offset,
                          uint16_t size, symbol_t *parent)
{
  symbol_t *varPtr;

  /* Get a slot in the symbol table */

  varPtr = addSymbol(name, type);
  if (varPtr)
    {
      /* Add the variable to the symbol table */

      varPtr->sParm.v.vOffset   = offset;
      varPtr->sParm.v.vSize     = size;
      varPtr->sParm.v.vParent   = parent;
    }

  /* Return a pointer to the new variable symbol */

  return varPtr;
}

/****************************************************************************/

symbol_t *pas_AddLabel(char *name, uint16_t label)
{
  symbol_t *labelPtr;

  /* Get a slot in the symbol table */

  labelPtr = addSymbol(name, sLABEL);
  if (labelPtr)
    {
      /* Add the label to the symbol table */

      labelPtr->sParm.l.lLabel     = label;
      labelPtr->sParm.l.lUnDefined = true;
    }

  /* Return a pointer to the new label symbol */

  return labelPtr;
}

/****************************************************************************/

symbol_t *pas_AddField(char *name, symbol_t *record, symbol_t *lastField)
{
  symbol_t *fieldPtr;

  /* Get a slot in the symbol table */

  fieldPtr = addSymbol(name, sRECORD_OBJECT);
  if (fieldPtr)
    {
      /* Add the field to the symbol table */

      fieldPtr->sParm.r.rRecord = record;

      /* Link the previous field to this one */

      if (lastField != NULL)
        {
          lastField->sParm.r.rNext = fieldPtr;
        }
    }

  /* Return a pointer to the new variable symbol */

  return fieldPtr;
}

/****************************************************************************/

void pas_PrimeSymbolTable(unsigned long symbolTableSize)
{
  int32_t trueValue   = BOOLEAN_TRUE;
  int32_t falseValue  = BOOLEAN_FALSE;
  int32_t maxintValue = MAXINT;
  symbol_t *typePtr;

  /* Allocate and initialize symbol table */

  g_symbolTable = malloc(symbolTableSize * sizeof(symbol_t));
  if (!g_symbolTable)
    {
      fatal(eNOMEMORY);
    }

  g_nSym = 0;

  /* Add the standard constants to the symbol table */

  (void)pas_AddConstant("TRUE",   tBOOLEAN_CONST, &trueValue,   NULL);
  (void)pas_AddConstant("FALSE",  tBOOLEAN_CONST, &falseValue,  NULL);
  (void)pas_AddConstant("MAXINT", tINT_CONST,     &maxintValue, NULL);
  (void)pas_AddConstant("NIL",    tNIL,           &falseValue,  NULL);

  /* Add the standard types to the symbol table */

  typePtr = pas_AddTypeDefine("INTEGER", sINT, sINT_SIZE, NULL);
  if (typePtr)
    {
      g_parentInteger            = typePtr;
      typePtr->sParm.t.tMinValue = MININT;
      typePtr->sParm.t.tMaxValue = MAXINT;
    }

  typePtr = pas_AddTypeDefine("WORD", sWORD, sWORD_SIZE, NULL);
  if (typePtr)
    {
      g_parentInteger            = typePtr;
      typePtr->sParm.t.tMinValue = MINWORD;
      typePtr->sParm.t.tMaxValue = MAXWORD;
    }

  typePtr = pas_AddTypeDefine("SHORTINTEGER", sSHORTINT, sSHORTINT_SIZE,
                               NULL);
  if (typePtr)
    {
      g_parentInteger            = typePtr;
      typePtr->sParm.t.tMinValue = MINSHORTINT;
      typePtr->sParm.t.tMaxValue = MAXSHORTINT;
    }

  typePtr = pas_AddTypeDefine("SHORTWORD", sSHORTWORD, sSHORTWORD_SIZE,
                              NULL);
  if (typePtr)
    {
      g_parentInteger            = typePtr;
      typePtr->sParm.t.tMinValue = MINSHORTWORD;
      typePtr->sParm.t.tMaxValue = MAXSHORTWORD;
    }

  typePtr = pas_AddTypeDefine("LONGINTEGER", sLONGINT, sLONGINT_SIZE,
                               NULL);
  if (typePtr)
    {
      g_parentInteger            = typePtr;
      typePtr->sParm.t.tMinValue = MINLONGINT;
      typePtr->sParm.t.tMaxValue = MAXLONGINT;
    }

  typePtr = pas_AddTypeDefine("LONGWORD", sLONGWORD, sLONGWORD_SIZE,
                              NULL);
  if (typePtr)
    {
      g_parentInteger            = typePtr;
      typePtr->sParm.t.tMinValue = MINLONGWORD;
      typePtr->sParm.t.tMaxValue = MAXLONGWORD;
    }

  typePtr = pas_AddTypeDefine("BOOLEAN", sBOOLEAN, sBOOLEAN_SIZE, NULL);
  if (typePtr)
    {
      typePtr->sParm.t.tMinValue = trueValue;
      typePtr->sParm.t.tMaxValue = falseValue;
    }

  typePtr = pas_AddTypeDefine("REAL", sREAL, sREAL_SIZE, NULL);

  typePtr = pas_AddTypeDefine("CHAR", sCHAR, sCHAR_SIZE, NULL);
  if (typePtr)
    {
      typePtr->sParm.t.tMinValue = MINCHAR;
      typePtr->sParm.t.tMaxValue = MAXCHAR;
    }

  typePtr = pas_AddTypeDefine("TEXTFILE", sTEXTFILE, sCHAR_SIZE, NULL);
  if (typePtr)
    {
      typePtr->sParm.t.tSubType  = sCHAR;
      typePtr->sParm.t.tMinValue = MINCHAR;
      typePtr->sParm.t.tMaxValue = MAXCHAR;
    }

  /* Add some enhanced Pascal standard" types to the symbol table
   *
   * string is represent by a large buffer in separate string memory.
   */

  typePtr = pas_AddTypeDefine("STRING", sSTRING, sSTRING_SIZE, NULL);
  if (typePtr)
    {
      g_parentString            = typePtr;
      typePtr->sParm.t.tSubType = sCHAR;
    }

  /* Add the standard files to the symbol table */

  g_inputFile = pas_AddFile("INPUT", sTEXTFILE, g_dStack, sCHAR_SIZE, NULL);
  pas_AddFileInitializer(g_inputFile, true, INPUT_FILE_NUMBER);
  g_dStack += sINT_SIZE;

  g_outputFile = pas_AddFile("OUTPUT", sTEXTFILE, g_dStack, sCHAR_SIZE, NULL);
  pas_AddFileInitializer(g_outputFile, true, OUTPUT_FILE_NUMBER);
  g_dStack += sINT_SIZE;
}

/****************************************************************************/

void pas_VerifyLabels(int32_t symIndex)
{
  int16_t i;
  
  for (i=symIndex; i < g_nSym; i++)
    {
      if (g_symbolTable[i].sKind == sLABEL &&
          g_symbolTable[i].sParm.l.lUnDefined)
        {
           error (eUNDEFLABEL);
        }
    }
}

/****************************************************************************/

#if CONFIG_DEBUG
const char noName[] = "********";
void pas_DumpTables(void)
{
  int16_t i;

  fprintf(g_lstFile,"\nSYMBOL TABLE:\n");
  fprintf(g_lstFile,"[  Addr  ]     NAME KIND LEVL\n");

  for (i = 0; i < g_nSym; i++)
    {
      fprintf(g_lstFile,"[%p] ", &g_symbolTable[i]);

      if (g_symbolTable[i].sName)
        {
          fprintf(g_lstFile, "%8s", g_symbolTable[i].sName);
        }
      else
        {
          fprintf(g_lstFile, "%8s", noName);
        }

      fprintf(g_lstFile," %04x %04x ",
              g_symbolTable[i].sKind,
              g_symbolTable[i].sLevel);

      switch (g_symbolTable[i].sKind)
        {
          /* Constants */

        case tINT_CONST :
        case tCHAR_CONST :
        case tBOOLEAN_CONST :
        case tNIL :
        case sSCALAR :
          fprintf(g_lstFile, "val=%" PRId32 " parent=[%p]\n",
                  g_symbolTable[i].sParm.c.cValue.i,
                  g_symbolTable[i].sParm.c.cParent);
          break;
        case tREAL_CONST :
          fprintf(g_lstFile, "val=%f parent=[%p]\n",
                  g_symbolTable[i].sParm.c.cValue.f,
                  g_symbolTable[i].sParm.c.cParent);
          break;

          /* Types */

        case sTYPE  :
          fprintf(g_lstFile,
                  "type=%02x subType=%02x flags=%02x dimension=%u "
                  "allocSize=%" PRId32 " minValue=%" PRId32 " maxValue=%"
                  PRId32 " parent=[%p]\n",
                  g_symbolTable[i].sParm.t.tType,
                  g_symbolTable[i].sParm.t.tSubType,
                  g_symbolTable[i].sParm.t.tFlags,
                  g_symbolTable[i].sParm.t.tDimension,
                  g_symbolTable[i].sParm.t.tAllocSize,
                  g_symbolTable[i].sParm.t.tMinValue,
                  g_symbolTable[i].sParm.t.tMaxValue,
                  g_symbolTable[i].sParm.t.tParent);
          break;

          /* Procedures/Functions */

          /* Procedures and Functions */

        case sPROC :
        case sFUNC :
          fprintf(g_lstFile,
                  "label=L%04x nParms=%d flags=%02x parent=[%p]\n",
                  g_symbolTable[i].sParm.p.pLabel,
                  g_symbolTable[i].sParm.p.pNParms,
                  g_symbolTable[i].sParm.p.pFlags,
                  g_symbolTable[i].sParm.p.pParent);
          break;

          /* Labels */

        case sLABEL :
          fprintf(g_lstFile, "label=L%04x unDefined=%d\n",
                  g_symbolTable[i].sParm.l.lLabel,
                  g_symbolTable[i].sParm.l.lUnDefined);
          break;

          /* Variables */

        case sINT :
        case sWORD :
        case sSHORTINT :
        case sSHORTWORD :
        case sLONGINT :
        case sLONGWORD :
        case sBOOLEAN :
        case sCHAR  :
        case sREAL :
        case sARRAY :
        case sPOINTER :
        case sVAR_PARM :
        case sRECORD :
        case sFILE :
        case sTEXTFILE :
          fprintf(g_lstFile, "flags=%02x xfrUnit=%u offset=%" PRId32
                  " size=%" PRId32 " parent=[%p]\n",
                  g_symbolTable[i].sParm.v.vFlags,
                  g_symbolTable[i].sParm.v.vXfrUnit,
                  g_symbolTable[i].sParm.v.vOffset,
                  g_symbolTable[i].sParm.v.vSize,
                  g_symbolTable[i].sParm.v.vParent);
          break;

          /* Record objects */

        case sRECORD_OBJECT :
          fprintf(g_lstFile,
                  "offset=%" PRId32 " size=%" PRId32 " record=[%p] "
                  "parent=[%p] next=[%p]\n",
                  g_symbolTable[i].sParm.r.rOffset,
                  g_symbolTable[i].sParm.r.rSize,
                  g_symbolTable[i].sParm.r.rRecord,
                  g_symbolTable[i].sParm.r.rParent,
                  g_symbolTable[i].sParm.r.rNext);
          break;

          /* Constant strings */

        case sSTRING_CONST :
          fprintf(g_lstFile, "offset=%04" PRIx32 " size=%" PRId32 "\n",
                  g_symbolTable[i].sParm.s.roOffset,
                  g_symbolTable[i].sParm.s.roSize);
          break;

        default :
          fprintf(g_lstFile, "Unknown sKind\n");
          break;
        }
    }
}
#endif
