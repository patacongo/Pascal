UNIT StringUtils;

INTERFACE
  USES
    Machine in 'Machine.pas'

  CONST

  TYPE

  FUNCTION TokenizeString(inputLine, tokenDelimiter : string;
                          VAR strPos : integer) : string;

IMPLEMENTATION

  { TokenizeString returns the next token from the string 'inputLine' on each
    call.  A token is defined to be a substring of 'inputLine' delimited by
    one of the characters from the string 'tokenDelimiter'.  The integer VAR
    parameter 'strPos' must be provided so that TokenizeString can continue
    parsing the string 'inputLine' from call-to-call.  'strPos' simply holds
    the position in the string 'inputLine' where TokenizeString will resume
    parsing on the next call. }

  FUNCTION TokenizeString(inputLine, tokenDelimiter : string;
                          VAR strPos : integer) : string;
  VAR
    chStr : string;
    delimPos : integer;
    startPos : integer;
    tokenSize : integer

  BEGIN
    (* Skip over leading delimiters *)

    REPEAT
      chStr := Copy(inputLine, strPos, 1);
      delimPos := Pos(chStr, tokenDelimiter);
      strPos := strPos + 1;
    UNTIL delimPos = 0;

    (* Then scan to the dimiter position after the token *)

    startPos := strPos;
    REPEAT
      chStr := Copy(inputLine, strPos, 1);
      delimPos := Pos(chStr, tokenDelimiter);
      strPos := strPos + 1;
    UNTIL delimPos > 0;

    (* Return the delimited substring *)

    tokenSize := strPos - startPos;
    TokenizeString := Copy(inputLine, startPos, strPos - StartPos);
  END;
END.
