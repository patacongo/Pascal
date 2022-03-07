UNIT StringUtils;

INTERFACE
  USES
    Machine in 'Machine.pas'

  TYPE

  FUNCTION UpperCase(ch : char) : char;
  FUNCTION LowerCase(ch : char) : char;
  FUNCTION TokenizeString(InputLine, tokenDelimiters : string;
                          VAR Token : string; VAR StrPos : integer) : boolean;

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

  { Return the position of the next occurence of 'CheckChar' in 'StrInput'
    after position 'CheckPos' }

  FUNCTION CharPos(StrInput : STRING; CheckChar : char;
                   StartPos : integer) : integer;
  VAR
    StrLength : integer;
    CheckPos  : integer

  BEGIN
    StrLength := Length(StrInput);
    CharPos   := 0;

    IF (StartPos > 0) AND (StartPos <= StrLength) THEN
    BEGIN
      CheckPos     := StartPos;
      WHILE (CheckPos <= StrLength) AND (CharPos = 0) DO
        IF CheckChar = CharAt(StrInput, CheckPos) THEN
          CharPos  := CheckPos
        ELSE
          CheckPos := CheckPos + 1
    END
  END;

  { TokenizeString gets the next token from the string 'InputLine' on each
    call.  A token is defined to be a substring of 'InputLine' delimited by
    one of the characters from the string 'tokenDelimiters'.  The integer VAR
    parameter 'StrPos' must be provided so that TokenizeString can continue
    parsing the string 'InputLine' from call-to-call.  'StrPos' simply holds
    the position in the string 'InputLine' where TokenizeString will resume
    parsing on the next call.  This function returns true if the next token
    pas found or false if all tokens have been parsed.  }

  FUNCTION TokenizeString(InputLine, tokenDelimiters : STRING;
                          VAR Token : STRING; VAR StrPos : integer) : boolean;
  VAR
    Ch        : char;
    StrLength : integer;
    StartPos  : integer;
    EndPos    : integer;
    TokenSize : integer

  BEGIN
    TokenizeString := false; (* Assume failure *)
    StrLength      := Length(InputLine);

    (* Verify that we have not parsed to the end of the string *)

    IF StrPos <= StrLength THEN
    BEGIN
      (* Skip over leading delimiters *)

      StartPos := 0;

      REPEAT
        Ch := CharAt(InputLine, StrPos);
        IF CharPos(tokenDelimiters, Ch, 1) = 0 THEN
          StartPos := StrPos;
        StrPos := StrPos + 1;
      UNTIL (StartPos <> 0) OR (StrPos > StrLength);

      (* Verify that There is something other than delimiters at the end of *)
      (* the string                                                         *)

      IF StartPos <> 0 THEN
      BEGIN
        (* Then scan to the dimiter position after the token *)

        EndPos := 0;

        REPEAT
          Ch := CharAt(InputLine, StrPos);
          IF CharPos(tokenDelimiters, Ch, 1) > 0 THEN
            EndPos := StrPos;
          StrPos := StrPos + 1;
        UNTIL (EndPos <> 0) OR (StrPos > StrLength);

        (* Handle the case of no delimiter at the end of the line. *)

        IF EndPos = 0 THEN
          EndPos := StrLength + 1;

        (* Return the delimited substring *)

        TokenSize := EndPos - StartPos;
        IF (TokenSize > 0) THEN
        BEGIN
          Token := Copy(InputLine, StartPos, TokenSize);
          TokenizeString := true
        END
      END
    END
  END;
END.
