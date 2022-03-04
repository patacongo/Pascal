UNIT StringUtils;

INTERFACE
  USES
    Machine in 'Machine.pas'

  CONST

  TYPE

  FUNCTION TokenizeString(inputLine, tokenDelimiter : string;
                          VAR Token : string; VAR strPos : integer) : boolean;

IMPLEMENTATION

  { TokenizeString gets the next token from the string 'inputLine' on each
    call.  A token is defined to be a substring of 'inputLine' delimited by
    one of the characters from the string 'tokenDelimiter'.  The integer VAR
    parameter 'strPos' must be provided so that TokenizeString can continue
    parsing the string 'inputLine' from call-to-call.  'strPos' simply holds
    the position in the string 'inputLine' where TokenizeString will resume
    parsing on the next call.  This function returns true if the next token
    pas found or false if all tokens have been parsed.  }

  FUNCTION TokenizeString(inputLine, tokenDelimiter : string;
                          VAR Token : string; VAR strPos : integer) : boolean;
  VAR
    ChStr : string;
    DelimPos : integer;
    StartPos : integer;
    TokenSize : integer

  BEGIN
    (* Skip over leading delimiters *)

    REPEAT
      ChStr := Copy(inputLine, strPos, 1);
      DelimPos := Pos(ChStr, tokenDelimiter);
      strPos := strPos + 1;
    UNTIL DelimPos = 0;

    (* Then scan to the dimiter position after the token *)

    StartPos := strPos;
    REPEAT
      ChStr := Copy(inputLine, strPos, 1);
      DelimPos := Pos(ChStr, tokenDelimiter);
      strPos := strPos + 1;
    UNTIL DelimPos > 0;

    (* Return the delimited substring *)

    TokenSize := strPos - StartPos;
    IF (TokenSize > 0) THEN
    BEGIN
      Token := Copy(inputLine, StartPos, TokenSize);
      TokenizeString := true
    END
    ELSE
    BEGIN
      Token := '';
      TokenizeString := false
    END;
  END;
END.
