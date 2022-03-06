UNIT StringUtils;

INTERFACE
  USES
    Machine in 'Machine.pas'

  TYPE

  FUNCTION UpperCase(ch : char) : char;
  FUNCTION LowerCase(ch : char) : char;
  FUNCTION TokenizeString(inputLine, tokenDelimiter : string;
                          VAR Token : string; VAR strPos : integer) : boolean;

IMPLEMENTATION

  CONST
    LowerA = Ord('a');
    LowerZ = Ord('z');
    UpperA = Ord('A');
    UpperZ = Ord('Z')

  { Convert characters from upper to lower case, and vice versa }

  FUNCTION UpperCase(ch : char) : char;
  VAR
    Ascii : integer

  BEGIN
    Ascii := Ord(ch);
    IF (Ascii >= LowerA) AND (Ascii <= LowerZ) THEN
      ch := Chr(Ascii - LowerA + UpperA);

    UpperCase := ch;
  END;

  FUNCTION LowerCase(ch : char) : char;
  VAR
    Ascii : integer

  BEGIN
    Ascii := Ord(ch);
    IF (Ascii >= UpperA) AND (Ascii <= UpperZ) THEN
      ch := Chr(Ascii - UpperA + LowerA);

    LowerCase := ch;
  END;

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
    TokenizeString := false; (* Assume failure *)

    (* Verify that we have not parsed to the end of the string *)

    IF strPos < Length(inputLine) THEN
    BEGIN
      (* Skip over leading delimiters *)

      REPEAT
        ChStr := Copy(inputLine, strPos, 1);
        DelimPos := Pos(ChStr, tokenDelimiter);
        IF DelimPos <> 0 THEN
          strPos := strPos + 1;
      UNTIL DelimPos = 0;

      (* Verify that There is something other than delimiters at the end of *)
      (* the string                                                         *)

      IF strPos < Length(inputLine) THEN
      BEGIN
        (* Then scan to the dimiter position after the token *)

        StartPos := strPos;
        strPos   := strPos + 1;

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
      END
    END
  END;
END.
