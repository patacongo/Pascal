/****************************************************************************
 * pas_symtable.c
 * Table Management Package
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
 * (1) Standard Pascal reserved word
 * (2) Standard Pascal Function
 * (3) Standard Pascal Procedure
 * (4) Extended (or non-standard) Pascal reserved word
 * (5) Extended (or non-standard) Pascal function
 * (6) Extended (or non-standard) Pascal procedure
 * (7) Built-In Function
 */

static const reservedWord_t g_rsw[] =                /* Reserved word list */
{
  {"ABS",            tSTDFUNC,        txABS},        /* (2) */
  {"AND",            tAND,            txNONE},       /* (1) */
  {"APPEND",         tSTDPROC,        txAPPEND},     /* (3) */
  {"ARCTAN",         tSTDFUNC,        txARCTAN},     /* (2) */
  {"ARRAY",          tARRAY,          txNONE},       /* (1) */
  {"ASSIGNFILE",     tSTDPROC,        txASSIGNFILE}, /* (3) */
  {"BEGIN",          tBEGIN,          txNONE},       /* (1) */
  {"CASE",           tCASE,           txNONE},       /* (1) */
  {"CHR",            tSTDFUNC,        txCHR},        /* (2) */
  {"CLOSEFILE",      tSTDPROC,        txCLOSEFILE},  /* (3) */
  {"CONST",          tCONST,          txNONE},       /* (1) */
  {"COS",            tSTDFUNC,        txCOS},        /* (2) */
  {"DIV",            tDIV,            txNONE},       /* (1) */
  {"DO",             tDO,             txNONE},       /* (1) */
  {"DOWNTO",         tDOWNTO,         txNONE},       /* (1) */
  {"ELSE",           tELSE,           txNONE},       /* (1) */
  {"END",            tEND,            txNONE},       /* (1) */
  {"EOF",            tSTDFUNC,        txEOF},        /* (2) */
  {"EOLN",           tSTDFUNC,        txEOLN},       /* (2) */
  {"EXP",            tSTDFUNC,        txEXP},        /* (2) */
  {"FILE",           tFILE,           txNONE},       /* (1) */
  {"FINALIZATION",   tFINALIZATION,   txNONE},       /* (4) */
  {"FOR",            tFOR,            txNONE},       /* (1) */
  {"FUNCTION",       tFUNCTION,       txNONE},       /* (1) */
  {"GET",            tSTDPROC,        txGET},        /* (3) */
  {"GETENV",         tSTDFUNC,        txGETENV},     /* (5) */
  {"GOTO",           tGOTO,           txNONE},       /* (1) */
  {"HALT",           tSTDPROC,        txHALT},       /* (3) */
  {"IF",             tIF,             txNONE},       /* (1) */
  {"IMPLEMENTATION", tIMPLEMENTATION, txNONE},       /* (4) */
  {"IN",             tIN,             txNONE},       /* (1) */
  {"INITIALIZATION", tINITIALIZATION, txNONE},       /* (4) */
  {"INTERFACE",      tINTERFACE,      txNONE},       /* (4) */
  {"LABEL",          tLABEL,          txNONE},       /* (1) */
  {"LN",             tSTDFUNC,        txLN},         /* (2) */
  {"MOD",            tMOD,            txNONE},       /* (1) */
  {"NEW",            tSTDPROC,        txNEW},        /* (3) */
  {"NOT",            tNOT,            txNONE},       /* (1) */
  {"ODD",            tSTDFUNC,        txODD},        /* (2) */
  {"OF",             tOF,             txNONE},       /* (1) */
  {"OR",             tOR,             txNONE},       /* (1) */
  {"ORD",            tSTDFUNC,        txORD},        /* (2) */
  {"PACK",           tSTDPROC,        txPACK},       /* (3) */
  {"PACKED",         tPACKED,         txNONE},       /* (1) */
  {"PAGE",           tSTDPROC,        txPAGE},       /* (3) */
  {"PRED",           tSTDFUNC,        txPRED},       /* (2) */
  {"PROCEDURE",      tPROCEDURE,      txNONE},       /* (1) */
  {"PROGRAM",        tPROGRAM,        txNONE},       /* (1) */
  {"PUT",            tSTDPROC,        txPUT},        /* (3) */
  {"READ",           tSTDPROC,        txREAD},       /* (3) */
  {"READLN",         tSTDPROC,        txREADLN},     /* (3) */
  {"RECORD",         tRECORD,         txNONE},       /* (1) */
  {"REPEAT",         tREPEAT,         txNONE},       /* (1) */
  {"RESET",          tSTDPROC,        txRESET},      /* (3) */
  {"REWRITE",        tSTDPROC,        txREWRITE},    /* (3) */
  {"ROUND",          tSTDFUNC,        txROUND},      /* (2) */
  {"SET",            tSET,            txNONE},       /* (1) */
  {"SHL",            tSHL,            txNONE},       /* (4) */
  {"SHR",            tSHR,            txNONE},       /* (4) */
  {"SIN",            tSTDFUNC,        txSIN},        /* (2) */
  {"SIZEOF",         tBUILTIN,        txSIZEOF},     /* (7) */
  {"SQR",            tSTDFUNC,        txSQR},        /* (2) */
  {"SQRT",           tSTDFUNC,        txSQRT},       /* (2) */
  {"SUCC",           tSTDFUNC,        txSUCC},       /* (2) */
  {"THEN",           tTHEN,           txNONE},       /* (1) */
  {"TO",             tTO,             txNONE},       /* (1) */
  {"TRUNC",          tSTDFUNC,        txTRUNC},      /* (2) */
  {"TYPE",           tTYPE,           txNONE},       /* (1) */
  {"UNIT",           tUNIT,           txNONE},       /* (4) */
  {"UNPACK",         tSTDPROC,        txUNPACK},     /* (3) */
  {"UNTIL",          tUNTIL,          txNONE},       /* (1) */
  {"USES",           tUSES,           txNONE},       /* (4) */
  {"VAL",            tSTDPROC,        txVAL},        /* (6) */
  {"VAR",            tVAR,            txNONE},       /* (1) */
  {"WHILE",          tWHILE,          txNONE},       /* (1) */
  {"WITH",           tWITH,           txNONE},       /* (1) */
  {"WRITE",          tSTDPROC,        txWRITE},      /* (3) */
  {"WRITELN",        tSTDPROC,        txWRITELN},    /* (3) */
  {NULL,             0,               txNONE}        /* List terminator */
};

static symbol_t *g_symbolTable;                      /* Symbol Table */

/* The g_aliasTable[] allows support for different versions of
 * pascal source files that differ only in naming.
 */

static const symbolAlias_t g_aliasTable[] =
{
    {"ASSIGN", "ASSIGNFILE"},
    {"CLOSE",  "CLOSEFILE"},
    {"TEXT",   "TEXTFILE"},
    {NULL,     NULL}
};

/**************************************************************/

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

/**************************************************************/

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

/***************************************************************/

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

/***************************************************************/

static symbol_t *addSymbol(char *name, int16_t kind)
{
   TRACE(g_lstFile,"[addSymbol]");

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

/***************************************************************/

symbol_t *pas_AddTypeDefine(char *name, uint8_t type, uint16_t size,
                            symbol_t *parent, symbol_t *index)
{
   symbol_t *typePtr;

   TRACE(g_lstFile,"[pas_AddTypeDefine]");

   /* Get a slot in the symbol table */

   typePtr = addSymbol(name, sTYPE);
   if (typePtr)
     {
       /* Add the type definition to the symbol table
        * NOTES:
        * 1. The minValue and maxValue fields (for scalar and subrange)
        *    types must be set external to this function
        * 2. For most variables, allocated size/type (rsize/rtype) and
        *    the clone size/type are the same.  If this is not the case,
        *    external logic will need to clarify this as well.
        * 3. We assume that there are no special flags associated with
        *    the type.
        */

       typePtr->sParm.t.type     = type;
       typePtr->sParm.t.rtype    = type;
       typePtr->sParm.t.flags    = 0;
       typePtr->sParm.t.asize    = size;
       typePtr->sParm.t.rsize    = size;
       typePtr->sParm.t.parent   = parent;
       typePtr->sParm.t.index    = index;
     }

   /* Return a pointer to the new constant symbol */

   return typePtr;
}

/***************************************************************/

symbol_t *pas_AddConstant(char *name, uint8_t type, int32_t *value,
                          symbol_t *parent)
{
   symbol_t *constPtr;

   TRACE(g_lstFile,"[pas_AddConstant]");

   /* Get a slot in the symbol table */
   constPtr = addSymbol(name, type);
   if (constPtr) {

     /* Add the value of the constant to the symbol table */
     if (type == tREAL_CONST)
       constPtr->sParm.c.val.f = *((double*) value);
     else
       constPtr->sParm.c.val.i = *value;

     constPtr->sParm.c.parent = parent;
   }

   /* Return a pointer to the new constant symbol */

   return constPtr;
}

/***************************************************************/

symbol_t *pas_AddStringConstant(char *name, uint32_t offset, uint32_t size)
{
  symbol_t *stringPtr;

  TRACE(g_lstFile,"[pas_AddStringConstant]");

  /* Get a slot in the symbol table */

  stringPtr = addSymbol(name, sSTRING_CONST);
  if (stringPtr)
    {
      /* Add the value of the constant to the symbol table */

      stringPtr->sParm.s.offset = offset;
      stringPtr->sParm.s.size   = size;
    }

  /* Return a pointer to the new string symbol */

  return stringPtr;
}

/***************************************************************/

symbol_t *pas_AddFile(char *name, uint16_t kind, uint16_t offset,
                      uint16_t xfrUnit, symbol_t *typePtr)
{
  symbol_t *filePtr;

  TRACE(g_lstFile,"[pas_AddFile]");

  /* Get a slot in the symbol table */

  filePtr = addSymbol(name, kind);
  if (filePtr)
    {
      /* Add the file to the symbol table */

      filePtr->sParm.v.xfrUnit    = xfrUnit;    /* Size of each transfer (binary) */
      filePtr->sParm.v.offset     = offset;     /* Offset to variable */
      filePtr->sParm.v.size       = sINT_SIZE;  /* Run time storage size */
      filePtr->sParm.v.parent     = typePtr;    /* FILE OF Type (binary) */
    }

  /* Return a pointer to the new file variable symbol */

  return filePtr;
}

/***************************************************************/

symbol_t *pas_AddProcedure(char *name, uint8_t type, uint16_t label,
                           uint16_t nParms, symbol_t *parent)
{
   symbol_t *procPtr;

   TRACE(g_lstFile,"[pas_AddProcedure]");

   /* Get a slot in the symbol table */
   procPtr = addSymbol(name, type);
   if (procPtr)
     {
       /* Add the procedure/function definition to the symbol table */

       procPtr->sParm.p.label    = label;
       procPtr->sParm.p.nParms   = nParms;
       procPtr->sParm.p.flags    = 0;
       procPtr->sParm.p.symIndex = 0;
       procPtr->sParm.p.parent   = parent;
     }

   /* Return a pointer to the new procedure/function symbol */

   return procPtr;
}

/***************************************************************/

symbol_t *pas_AddVariable(char *name, uint8_t type, uint16_t offset,
                          uint16_t size, symbol_t *parent)
{
  symbol_t *varPtr;

  TRACE(g_lstFile,"[pas_AddVariable]");

  /* Get a slot in the symbol table */

  varPtr = addSymbol(name, type);
  if (varPtr)
    {
      /* Add the variable to the symbol table */

      varPtr->sParm.v.offset   = offset;
      varPtr->sParm.v.size     = size;
      varPtr->sParm.v.parent   = parent;
    }

  /* Return a pointer to the new variable symbol */

  return varPtr;
}

/***************************************************************/

symbol_t *pas_AddLabel(char *name, uint16_t label)
{
  symbol_t *labelPtr;

  TRACE(g_lstFile,"[pas_AddLabel]");

  /* Get a slot in the symbol table */

  labelPtr = addSymbol(name, sLABEL);
  if (labelPtr)
    {
      /* Add the label to the symbol table */

      labelPtr->sParm.l.label = label;
      labelPtr->sParm.l.unDefined = true;
    }

  /* Return a pointer to the new label symbol */

  return labelPtr;
}

/***************************************************************/

symbol_t *pas_AddField(char *name, symbol_t *record)
{
   symbol_t *fieldPtr;

   TRACE(g_lstFile,"[pas_AddField]");

   /* Get a slot in the symbol table */
   fieldPtr = addSymbol(name, sRECORD_OBJECT);
   if (fieldPtr) {

     /* Add the field to the symbol table */
     fieldPtr->sParm.r.record = record;

   }

   /* Return a pointer to the new variable symbol */

   return fieldPtr;
}

/***************************************************************/

void pas_PrimeSymbolTable(unsigned long symbolTableSize)
{
  int32_t trueValue   = BOOLEAN_TRUE;
  int32_t falseValue  = BOOLEAN_FALSE;
  int32_t maxintValue = MAXINT;
  symbol_t *typePtr;

  TRACE(g_lstFile,"[pas_PrimeSymbolTable]");

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

  typePtr = pas_AddTypeDefine("INTEGER", sINT, sINT_SIZE, NULL, NULL);
  if (typePtr)
    {
      g_parentInteger           = typePtr;
      typePtr->sParm.t.minValue = MININT;
      typePtr->sParm.t.maxValue = MAXINT;
    }

  typePtr = pas_AddTypeDefine("BOOLEAN", sBOOLEAN, sBOOLEAN_SIZE, NULL, NULL);
  if (typePtr)
    {
      typePtr->sParm.t.minValue = trueValue;
      typePtr->sParm.t.maxValue = falseValue;
    }

  typePtr = pas_AddTypeDefine("REAL", sREAL, sREAL_SIZE, NULL, NULL);

  typePtr = pas_AddTypeDefine("CHAR", sCHAR, sCHAR_SIZE, NULL, NULL);
  if (typePtr)
    {
      typePtr->sParm.t.minValue = MINCHAR;
      typePtr->sParm.t.maxValue = MAXCHAR;
    }

  typePtr = pas_AddTypeDefine("TEXTFILE", sTEXTFILE, sCHAR_SIZE, NULL, NULL);
  if (typePtr)
    {
      typePtr->sParm.t.subType  = sCHAR;
      typePtr->sParm.t.minValue = MINCHAR;
      typePtr->sParm.t.maxValue = MAXCHAR;
    }

  /* Add some enhanced Pascal standard" types to the symbol table
   *
   * string is represent by a 256 byte memory regions consisting of
   * one byte for the valid string length plus 255 bytes for string
   * storage
   */

  typePtr = pas_AddTypeDefine("STRING", sSTRING, sSTRING_SIZE, NULL, NULL);
  if (typePtr)
    {
      g_parentString            = typePtr;
      typePtr->sParm.t.rtype    = sRSTRING;
      typePtr->sParm.t.subType  = sCHAR;
      typePtr->sParm.t.rsize    = sRSTRING_SIZE;
      typePtr->sParm.t.flags    = STYPE_VARSIZE;
      typePtr->sParm.t.minValue = MINCHAR;
      typePtr->sParm.t.maxValue = MAXCHAR;
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
   int16_t i;                 /* loop index */

   for (i=symIndex; i < g_nSym; i++)
     if ((g_symbolTable[i].sKind == sLABEL)
     &&  (g_symbolTable[i].sParm.l.unDefined))
     {
       error (eUNDEFLABEL);
     }
}

/***************************************************************/

#if CONFIG_DEBUG
const char noName[] = "********";
void dumpTables(void)
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
                  g_symbolTable[i].sParm.c.val.i,
                  g_symbolTable[i].sParm.c.parent);
          break;
        case tREAL_CONST :
          fprintf(g_lstFile, "val=%f parent=[%p]\n",
                  g_symbolTable[i].sParm.c.val.f,
                  g_symbolTable[i].sParm.c.parent);
          break;

          /* Types */

        case sTYPE  :
          fprintf(g_lstFile,
                  "type=%02x rtype=%02x subType=%02x flags=%02x "
                  "asize=%" PRId32 " rsize=%" PRId32 " minValue=%" PRId32
                  " maxValue=%" PRId32 " parent=[%p]\n",
                  g_symbolTable[i].sParm.t.type,
                  g_symbolTable[i].sParm.t.rtype,
                  g_symbolTable[i].sParm.t.subType,
                  g_symbolTable[i].sParm.t.flags,
                  g_symbolTable[i].sParm.t.asize,
                  g_symbolTable[i].sParm.t.rsize,
                  g_symbolTable[i].sParm.t.minValue,
                  g_symbolTable[i].sParm.t.maxValue,
                  g_symbolTable[i].sParm.t.parent);
          break;

          /* Procedures/Functions */

          /* Procedures and Functions */

        case sPROC :
        case sFUNC :
          fprintf(g_lstFile,
                  "label=L%04x nParms=%d flags=%02x parent=[%p]\n",
                  g_symbolTable[i].sParm.p.label,
                  g_symbolTable[i].sParm.p.nParms,
                  g_symbolTable[i].sParm.p.flags,
                  g_symbolTable[i].sParm.p.parent);
          break;

          /* Labels */

        case sLABEL :
          fprintf(g_lstFile, "label=L%04x unDefined=%d\n",
                  g_symbolTable[i].sParm.l.label,
                  g_symbolTable[i].sParm.l.unDefined);
          break;

          /* Variables */

        case sINT :
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
                  g_symbolTable[i].sParm.v.flags,
                  g_symbolTable[i].sParm.v.xfrUnit,
                  g_symbolTable[i].sParm.v.offset,
                  g_symbolTable[i].sParm.v.size,
                  g_symbolTable[i].sParm.v.parent);
          break;

          /* Record objects */

        case sRECORD_OBJECT :
          fprintf(g_lstFile,
                  "offset=%" PRId32 " size=%" PRId32 " record=[%p] "
                  "parent=[%p]\n",
                  g_symbolTable[i].sParm.r.offset,
                  g_symbolTable[i].sParm.r.size,
                  g_symbolTable[i].sParm.r.record,
                  g_symbolTable[i].sParm.r.parent);
          break;

          /* Constant strings */

        case sSTRING_CONST :
          fprintf(g_lstFile, "offset=%04" PRIx32 " size=%" PRId32 "\n",
                  g_symbolTable[i].sParm.s.offset,
                  g_symbolTable[i].sParm.s.size);
          break;

        default :
          fprintf(g_lstFile, "Unknown sKind\n");
          break;
        }
    }
}
#endif
