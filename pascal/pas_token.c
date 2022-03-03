/****************************************************************************
 * pas_token.c
 * Tokenization Package
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
 * Included Functions
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>

#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_errcodes.h"

#include "pas_main.h"
#include "pas_token.h"
#include "pas_symtable.h"
#include "pas_error.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void pas_GetCharacter        (void);
static void pas_SkipLine            (void);
static bool pas_GetLine             (void);

static bool pas_SymbolToken         (const char *name, bool recObj);
static void pas_Identifier          (uint16_t lastToken);
static void pas_StringToken         (void);
static void pas_UnsignedNumber      (void);
static void pas_UnsignedRealNumber  (void);
static void pas_UnsignedExponent    (void);
static void pas_UnsignedHexadecimal (void);
static void pas_UnsignedBinary      (void);

/****************************************************************************
 * Private Variables
 ****************************************************************************/

static char    *g_strStack;        /* String Stack */
static uint16_t g_inChar;          /* last gotten character */
static int      g_symStart   = 0;  /* Symbol search start index */
static int      g_constStart = 0;  /* Constant search start index */

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/* String stack access variables */

char *g_tokenString;           /* Start of token in string stack */
char *g_stringSP;              /* Top of string stack */

/* Level-related data */

int   g_levelSymOffset   = 0;  /* Index to symbols for this level */
int   g_levelConstOffset = 0;  /* Index to constants for this level */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/

static bool pas_SymbolToken(const char *name, bool findRecObj)
{
  symbol_t *tknPtr;
  int foundIndex;
  bool found = false;

  /* Check if this identifier name matches a registered symbol name */

  tknPtr = pas_FindSymbol(name, g_symStart, &foundIndex);

  /* Loop, skipping over record objects unless a record object expected */

  while (tknPtr != NULL)
    {
      /* Check if this token is a record object name.  If we are not expecting
       * a record object in this context and one was foung, then keep looking.
       * Otherwise, the record pointer will obfuscate another symbol that may
       * have the same name.
       */

      bool isRecObj = (tknPtr->sKind == sRECORD_OBJECT);

      if ((findRecObj && isRecObj) || (!findRecObj && !isRecObj))
        {
          g_token    = tknPtr->sKind;  /* Get the type from symbol table */
          g_stringSP = g_tokenString;  /* Pop token from stack */

          /* The following assignments only apply to constants.  However it
           * is simpler just to make the assignments than it is to determine
           * if is appropriate to do so
           */

          if (g_token == tREAL_CONST)
            {
              g_tknReal = tknPtr->sParm.c.cValue.f;
            }
          else
            {
              g_tknUInt = tknPtr->sParm.c.cValue.u;
            }

          found = true;
          break;
        }

      /* It was a record object name.  Skip over it and keep looking */

      tknPtr = pas_FindNextSymbol(name, g_symStart, foundIndex, &foundIndex);
    }

  g_tknPtr = tknPtr;
  return found;
}

/****************************************************************************/

static void pas_Identifier(uint16_t lastToken)
{
  const reservedWord_t *rptr;        /* Pointer to reserved word */
  const char *aliasedName;           /* Pointer to alias */

  g_tknSubType = txNONE;             /* Initialize */

  /* Concatenate identifier */

  do
    {
      *g_stringSP++ = g_inChar;      /* Concatenate char */
      pas_GetCharacter();            /* Get next character */
    }
  while ((isalnum(g_inChar)) || (g_inChar == '_'));

  *g_stringSP++ = '\0';                       /* make ASCIIZ string */

  /* Check if the identifier that we found has an alias.  We do this in order
   * to support compatibility to slightly different naming used by different
   * pascall compilers.
   */

  aliasedName = pas_MapToAlias(g_tokenString);

  /* Check if the (possibly aliased) identifier is a reserved word */

  rptr = pas_FindReservedWord(aliasedName);
  if (rptr)
    {
      g_token      = rptr->rtype;             /* get type from rsw table */
      g_tknSubType = rptr->subtype;           /* get subtype from rsw table */
      g_stringSP   = g_tokenString;           /* pop token from stack */
    }

  /* Check if the (possibly aliased) inentifier is a previously defined
   * symbol.
   */

  else
    {
      /* Is a record object expected in this context? A record object would
       * be expected:
       *
       *   After record-name '.', or
       *   Any time while within a with block.
       */

      bool recObjExpected = (lastToken == '.' || g_withRecord.wParent != NULL);

      /* Check for a symbol with this name.  If recObjExpected is true, then
       * give precedence to record objects (at the cost of doing the symbol
       * table lookup twice).
       */

      g_token = tIDENT;
      if (pas_SymbolToken(aliasedName, recObjExpected))
        {
          /* Record object found (if recObjExpected == true) */
        }
      else if (recObjExpected)
        {
          /* Record object not found, then what is it? */

          pas_SymbolToken(aliasedName, false);
        }
    }
}

/****************************************************************************/
/* Process string */

static void pas_StringToken(void)
{
  int16_t count = 0;                  /* # chars in string */

  g_token = tSTRING_CONST;           /* Indicate string constant type */
  pas_GetCharacter();                /* Skip over 1st single quote */

  /* Outer loop handles quoted single quotes in the comment */

  for (; ; )
    {
      uint16_t prevChar;

      /* The inner loop concatenates normal text characters until
       * a single quote is encountered.
       */

      while (g_inChar != SQUOTE)      /* Loop until next single quote */
        {
          if (g_inChar == '\n')       /* Check for EOL in string */
            {
              error(eNOSQUOTE);       /* ERROR, terminate string */
              break;
            }
          else
            {
              *g_stringSP++ = g_inChar; /* Concatenate character */
              count++;                /* Bump count of chars */
            }

           pas_GetCharacter();        /* Get the next character */
        }

      prevChar = g_inChar;            /* Remember the terminating character */
      pas_GetCharacter();             /* Skip over the quote (or newline) */

      /* Check for quoted singlel quote */

      if (prevChar == SQUOTE && g_inChar == SQUOTE)
        {
          *g_stringSP++ = g_inChar;   /* Concatenate the single quote */
          count++;                    /* Bump count of chars */
          pas_GetCharacter();         /* Skip over the second single quote */
          continue;                   /* And continue building the string */
        }

      break;
    }

  *g_stringSP++ = '\0';               /* terminate ASCIIZ string */

  if (count == 1)                     /* Check for char constant */
    {
      g_token    = tCHAR_CONST;       /* indicate char constant type */
      g_tknUInt  = *g_tokenString;    /* (integer) value = single char */
      g_stringSP = g_tokenString;     /* "pop" from string stack */
    }
}

/****************************************************************************/

static void pas_GetCharacter(void)
{
  /* Get the next character from the line buffer.  If EOL, get next line */

  g_inChar = *(FP->cp)++;
  if (!g_inChar)
    {
      /* We have used all of the characters on this line.  Read the next
       * line of data
       */

      pas_SkipLine();
    }
}

/****************************************************************************/

static void pas_SkipLine(void)
{
  if (pas_GetLine())
    {
      /* Uh-oh, we are out of data!  Just return some bogus value. */

      g_inChar = '?';
    }
  else
    {
      /* Otherwise, get the first character from the line */

      pas_GetCharacter();
    }
}

/****************************************************************************/

static bool pas_GetLine(void)
{
  bool endOfFile = false;

  /* Reset the character pointer to the start of the new line */

  FP->cp = FP->buffer;

  /* Read the next line from the currently active file */

  if (!fgets((char *)FP->cp, LINE_SIZE, FP->stream))
    {
      /* We are at an EOF for this file.  Check if we are processing an
       * included file
       */

      if (g_includeIndex > 0)
        {
          /* Yes.  Close the file */

          pas_CloseNestedFile();

          /* Indicate that there is no data on the input line. NOTE:
           * that FP now refers to the previous file at the next lower
           * level of nesting.
           */

          FP->buffer[0] = '\0';
        }
       else
         {
           /* No.  We are completely out of data.  Return true in this case. */

           endOfFile = true;
         }
     }
   else
     {
       /* We have a new line of data.  Increment the line number, then echo
        * the new line to the list file.
        */

       (FP->line)++;
       fprintf(g_lstFile, "%d:%04" PRId32 " %s",
               FP->include, FP->line, FP->buffer);
     }

   return endOfFile;
}

/****************************************************************************/

static void pas_UnsignedNumber(void)
{
  /* This logic (along with with pas_UnsignedRealNumber, and
   * unsignedRealExponent) handles:
   *
   * FORM: integer-number = decimal-integer | hexadecimal-integer |
   *       binary-integer
   * FORM: decimal-integer = digit-sequence
   * FORM: real-number =
   *       digit-sequence '.' [digit-sequence] [ exponent scale-factor ] |
   *       '.' digit-sequence [ exponent scale-factor ] |
   *       digit-sequence exponent scale-factor
   * FORM: exponent = 'e' | 'E'
   *
   * When called, g_inChar is equal to the leading digit of a
   * digit-sequence. NOTE that the real-number form beginning with
   * '.' does not use this logic.
   */

  /* Assume an integer type (might be real) */

  g_token = tINT_CONST;

  /* Concatenate all digits until an non-digit is found */

  do
    {
      *g_stringSP++ = g_inChar;
      pas_GetCharacter();
    }
  while (isdigit(g_inChar));

  /* If it is a digit-sequence followed by 'e' (or 'E'), then
   * continue processing this token as a real number.
   */

  if (g_inChar == 'e' || g_inChar == 'E')
    {
      pas_UnsignedExponent();
    }

  /* If the digit-sequence is followed by '.' but not by ".." (i.e.,
   * this is not a subrange), then switch we are parsing a real time.
   * Otherwise, convert the integer string to binary.
   */

  else if (g_inChar != '.' || pas_GetNextCharacter(false) == '.')
    {
      /* Terminate the integer string and convert it using sscanf */

      *g_stringSP++ = '\0';
      (void)sscanf(g_tokenString, "%" PRIu32, &g_tknUInt);

      /* Remove the integer string from the character identifer stack */

      g_stringSP = g_tokenString;
    }
  else
    {
      /* Its a real value!  Now really get the next character and
       * after the decimal point (this will work whether or not
       * pas_GetNextCharacter() was called). Then process the real number.
       */

      pas_GetCharacter();
      pas_UnsignedRealNumber();
    }
}

/****************************************************************************/

static void pas_UnsignedRealNumber(void)
{
  /* This logic (along with with pas_UnsignedNumber and pas_UnsignedExponent)
   * handles:
   *
   * FORM: real-number =
   *       digit-sequence '.' [digit-sequence] [ exponent scale-factor ] |
   *       '.' digit-sequence [ exponent scale-factor ] |
   *       digit-sequence exponent scale-factor
   * FORM: exponent = 'e' | 'E'
   *
   * When called:
   * - g_inChar is the character AFTER the '.'.
   * - Any leading digit-sequence is already in the character stack
   * - the '.' is not in the character stack.
   */

  /* Its a real constant */

  g_token = tREAL_CONST;

  /* Save the decimal point (g_inChar points to the character after
   * the decimal point).
   */

  *g_stringSP++ = '.';

  /* Now, loop to process the optional digit-sequence after the
   * decimal point.
   */

  while (isdigit(g_inChar))
    {
      *g_stringSP++ = g_inChar;
      pas_GetCharacter();
    }

  /* If it is a digit-sequence followed by 'e' (or 'E'), then
   * continue processing this token as a real number.
   */

  if (g_inChar == 'e' || g_inChar == 'E')
    {
      pas_UnsignedExponent();
    }
  else
    {
      /* There is no exponent...
       * Terminate the real number string  and convert it to binay
       * using sscanf.
       */

      *g_stringSP++ = '\0';
      (void) sscanf(g_tokenString, "%lf", &g_tknReal);
    }

  /* Remove the number string from the character identifer stack */

  g_stringSP = g_tokenString;
}

/****************************************************************************/

static void pas_UnsignedExponent(void)
{
  /* This logic (along with with pas_UnsignedNumber and pas_UnsignedRealNumber)
   * handles:
   *
   * FORM: real-number =
   *       digit-sequence '.' [digit-sequence] [ exponent scale-factor ] |
   *       '.' digit-sequence [ exponent scale-factor ] |
   *       digit-sequence exponent scale-factor
   * FORM: exponent = 'e'
   * FORM: scale-factor = [ sign ] digit-sequence
   *
   * When called:
   * - g_inChar holds the 'E' (or 'e') exponent
   * - Any leading digit-sequences or decimal points are already in the
   *   character stack
   * - the 'E' (or 'e') is not in the character stack.
   */

  /* Its a real constant */

  g_token = tREAL_CONST;

  /* Save the decimal point (g_inChar points to the character after
   * the decimal point).
   */

  *g_stringSP++ = g_inChar;
  pas_GetCharacter();

  /* Check for an optional sign before the exponent value */

  if (g_inChar == '-' || g_inChar == '+')
    {
      /* Add the sign to the stack */

      *g_stringSP++ = g_inChar;
      pas_GetCharacter();
    }
  else
    {
      /* Add a '+' sign to the stack */

      *g_stringSP++ = '+';
    }

  /* A digit sequence must appear after the exponent and optional
   * sign.
   */

  if (!isdigit(g_inChar))
    {
      error(eEXPONENT);
      g_tknReal = 0.0;
    }
  else
    {
      /* Now, loop to process the required digit-sequence */

      do
        {
          *g_stringSP++ = g_inChar;
          pas_GetCharacter();
        }
      while (isdigit(g_inChar));

      /* Terminate the real number string  and convert it to binay
       * using sscanf.
       */

      *g_stringSP++ = '\0';
      (void) sscanf(g_tokenString, "%lf", &g_tknReal);
    }

  /* Remove the number string from the character identifer stack */

  g_stringSP = g_tokenString;
}

/****************************************************************************/

static void pas_UnsignedHexadecimal(void)
{
  /* FORM: integer-number = decimal-integer | hexadecimal-integer |
   *       binary-integer
   * FORM: hexadecimal-integer = '$' hex-digit-sequence
   * FORM: hex-digit-sequence = hex-digit { hex-digit }
   * FORM: hex-digit = digit | 'a' | 'b' | 'c' | 'd' | 'e' | 'f'
   *
   * On entry, g_inChar is '$'
   */

  /* This is another representation for an integer */

  g_token = tINT_CONST;

  /* Loop to process each hex 'digit' */

  for (; ; )
    {
      /* Get the next character */

      pas_GetCharacter();

      /* Is it a decimal digit? */

      if (isdigit(g_inChar))
        {
          *g_stringSP++ = g_inChar;
        }

      /* Is it a hex 'digit'? */

      else if (g_inChar >= 'A' && g_inChar <= 'F')
        {
          *g_stringSP++ = g_inChar;
        }
      else if (g_inChar >= 'a' && g_inChar <= 'f')
        {
          *g_stringSP++ = toupper(g_inChar);
        }

      /* Otherwise, that must be the end of the hex value */

      else break;
    }

  /* Terminate the hex string and convert to binary using sscanf */

  *g_stringSP++ = '\0';
  (void)sscanf(g_tokenString, "%" PRIx32, &g_tknUInt);

  /* Remove the hex string from the character identifer stack */

  g_stringSP = g_tokenString;
}

/****************************************************************************/

static void pas_UnsignedBinary(void)
{
  uint32_t value;

  /* FORM: integer-number = decimal-integer | hexadecimal-integer |
   *       binary-integer
   * FORM: binary-integer = '%' binary-digit-sequence
   * FORM: binary-digit-sequence = binary-digit { binary-digit }
   * FORM: binary-digit = '0' | '1'
   *
   * On entry, g_inChar is '%'
   */

  /* This is another representation for an integer */

  g_token = tINT_CONST;

  /* Loop to process each hex 'digit' */

  value = 0;

  for (;;)
    {
      /* Get the next character */

      pas_GetCharacter();

      /* Is it a binary 'digit'? */

      if (g_inChar == '0')
        {
          value <<= 1;
        }

      else if (g_inChar == '1')
        {
          value <<= 1;
          value  |= 1;
        }

      /* Otherwise, that must be the end of the binary value */

      else break;
    }

  /* I don't think there is an sscanf conversion for binary, that's
   * why we did it above.
   */

  g_tknUInt = value;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int16_t pas_PrimeTokenizer(unsigned long stringStackSize)
{
  /* Allocate and initialize the string stack and stack pointers */

  g_strStack = malloc(stringStackSize);
  if (!g_strStack)
    {
      fatal(eNOMEMORY);
    }

  /* Initially, everything points to the bottom of the
   * string stack.
   */

  g_tokenString = g_strStack;
  g_stringSP    = g_strStack;

  /* Set up for input at the initial level of file parsing */

  pas_RePrimeTokenizer();
  return 0;
}

/****************************************************************************/

int16_t pas_RePrimeTokenizer(void)
{
  /* (Re-)set the char pointer to the beginning of the line */

  FP->cp = FP->buffer;

  /* Read the next line from the input stream */

  if (!fgets((char *)FP->cp, LINE_SIZE, FP->stream))
    {
      /* EOF.. close file */

      return 1;
    }

  /* Initialize the line nubmer */

  FP->line = 1;

  /* Get the first character from the new file */

  pas_GetCharacter();
  return 0;
}

/****************************************************************************/
/* Tell 'em what what the next character will be (if they should
 * choose to get it).  This is similar to pas_GetCharacter(), except that
 * the character pointer is not incremented past the character.  The
 * next time that pas_GetCharacter() is called, it will get the character
 * again.
 */

char pas_GetNextCharacter(bool skipWhiteSpace)
{
  /* Get the next character from the line buffer. */

  g_inChar = *(FP->cp);

  /* If it is the EOL then read the next line from the input file */

  if (!g_inChar)
    {
      /* We have used all of the characters on this line.  Read the next
       * line of data
       */

      if (pas_GetLine())
        {
          /* Uh-oh, we are out of data!  Just return some bogus value. */

          g_inChar = '?';
        }
      else
        {
          /* Otherwise, recurse to try again. */

          return pas_GetNextCharacter(skipWhiteSpace);

        }
    }

  /* If it is a space and we have been told to skip spaces then consume
   * the input line until a non-space or the EOL is encountered.
   */

  else if (skipWhiteSpace)
    {
      while (isspace(g_inChar) && g_inChar != '\0')
        {
          /* Skip over the space */

          (FP->cp)++;

          /* A get the character after the space */

          g_inChar = *(FP->cp);

        }

      /* If we hit the EOL while searching for the next non-space, then
       * recurse to try again on the next line
       */

      if (!g_inChar)
        {
          return pas_GetNextCharacter(skipWhiteSpace);
        }
    }

  return g_inChar;
}

/****************************************************************************/

void getToken(void)
{
  uint16_t lastToken;

  /* Remember the current token */

  lastToken = g_token;

  /* Reset a few globals that may be left in a bad state */

  g_tknPtr = NULL;

  /* Skip over leading spaces and comments */

  while (isspace(g_inChar)) pas_GetCharacter();

  /* Point to the beginning of the next token */

  g_tokenString = g_stringSP;

  /* Process Identifier, Symbol, or Reserved Word */

  if (isalpha(g_inChar) || g_inChar == '_')
    {
      pas_Identifier(lastToken);
    }

  /* Process Numeric */

  else if (isdigit(g_inChar))
    {
      pas_UnsignedNumber();
    }

  /* Process string */

  else if (g_inChar == SQUOTE)
    {
      pas_StringToken();  /* process string type */
    }

  /* Process ':' or assignment */

  else if (g_inChar == ':')
    {
      pas_GetCharacter();
      if (g_inChar == '=')
        {
          g_token = tASSIGN;
          pas_GetCharacter();
        }
      else
        {
          g_token = ':';
        }
    }

  /* Process '.' or subrange or real-number */

  else if (g_inChar == '.')
    {
      /* Get the character after the '.' */

      pas_GetCharacter();

      /* ".." indicates a subrange */

      if (g_inChar == '.')
        {
          g_token = tSUBRANGE;
          pas_GetCharacter();
        }

      /* '.' digit is a real number */

      else if (isdigit(g_inChar))
        {
          pas_UnsignedRealNumber();
        }

      /* Otherwise, it is just a '.' */

      else
        {
          g_token = '.';
        }
    }

  /* Process '<' or '<=' or '<>' or '<<' */

  else if (g_inChar == '<')
    {
      pas_GetCharacter();
      if (g_inChar == '>')
        {
          g_token = tNE;
          pas_GetCharacter();
        }
      else if (g_inChar == '=')
        {
          g_token = tLE;
          pas_GetCharacter();
        }
      else if (g_inChar == '<')
        {
          g_token = tSHL;
          pas_GetCharacter();
        }
      else
        {
          g_token = tLT;
        }
    }

  /* Process '>' or '>=' or '><' or '>>' */

  else if (g_inChar == '>')
    {
      pas_GetCharacter();
      if (g_inChar == '<')
        {
          g_token = tSYMDIFF;
          pas_GetCharacter();
        }
      else if (g_inChar == '=')
        {
          g_token = tGE;
          pas_GetCharacter();
        }
      else if (g_inChar == '>')
        {
          g_token = tSHR;
          pas_GetCharacter();
        }
      else
        {
          g_token = tGT;
        }
    }

  /* Get Comment -- form { .. } */

  else if (g_inChar == '{')
    {
      do pas_GetCharacter();             /* Get the next character */
      while (g_inChar != '}');           /* Loop until end of comment */
      pas_GetCharacter();                /* Skip over end of comment */
      getToken();                        /* Get the next real token */
    }

  /* Get comment -- form (* .. *) */

  else if (g_inChar == '(')
    {
      pas_GetCharacter();                /* Skip over comment character */
      if (g_inChar != '*')               /* Is this a comment? */
        {
          g_token = '(';                 /* No return '(' leaving the
                                          * unprocessed char in g_inChar */
        }
      else
        {
          uint16_t lastChar = ' ';       /* YES... prime the look behind */
          for (; ; )                     /* look for end of comment */
            {
              pas_GetCharacter();        /* get the next character */
              if (lastChar == '*' &&     /* Is it '*)' ?  */
                  g_inChar == ')')
                {
                  break;                 /* Yes... break out */
                }

              lastChar = g_inChar;       /* save the last character */
            }

          pas_GetCharacter();            /* skip over the comment end char */
          getToken();                    /* and get the next real token */
      }
    }

  /* NONSTANDARD:  All C/C++-style comments */

  else if (g_inChar == '/')
    {
      pas_GetCharacter();                /* skip over comment character */
      if (g_inChar == '/')               /* C++ style comment? */
        {
          pas_SkipLine();                /* Yes, skip rest of line */
          getToken();                    /* and get the next real token */
        }
      else if (g_inChar != '*')          /* is this a C-style comment? */
        {
          g_token = '/';                 /* No return '/' leaving the
                                          * unprocessed char in g_inChar */
        }
      else
        {
          uint16_t lastChar = ' ';       /* YES... prime the look behind */
          for (;;)                       /* look for end of comment */
            {
              pas_GetCharacter();        /* get the next character */
              if (lastChar == '*' &&     /* Is it '*)' ?  */
                  g_inChar == '/')
                {
                  break;                 /* Yes... break out */
                }

              lastChar = g_inChar;       /* save the last character */
            }

          pas_GetCharacter();            /* skip over the comment end char */
          getToken();                    /* and get the next real token */
      }
    }

  /* Check for $XXXX (hex) */

  else if (g_inChar == '%')
    {
      pas_UnsignedHexadecimal();
    }

  /* Check for $BBBB (binary) */

  else if (g_inChar == '%')
    {
      pas_UnsignedBinary();
    }

  /* if g_inChar is an ASCII character then return token = character */

  else if (isascii(g_inChar))
    {
      g_token = g_inChar;
      pas_GetCharacter();
    }

  /* Otherwise, discard the character and try again */

  else
    {
      pas_GetCharacter();
      getToken();
    }

  DEBUG(g_lstFile,"[%02x]", g_token);
}

/****************************************************************************/

void getLevelToken(void)
{
  g_constStart = g_levelConstOffset;  /* Limit search to current level */
  g_symStart   = g_levelSymOffset;
  getToken();                         /* Get the token in this scope */
  g_constStart = 0;
  g_symStart   = 0;
}
