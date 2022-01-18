(* Verifies short string concatenation, standard to short string conversion,
   character to short string concatenation *)

PROGRAM shortstrings;
CONST
  short1size = 8

TYPE
  short2type = STRING[16];

VAR
  standard : STRING;
  short1   : ARRAY[1..3] OF STRING[short1size];
  short2   : short2type;
  i        : 1..3

BEGIN
  standard  := 'Hello';
  short1[1] := 'America';
  short1[2] := 'World';
  short1[3] := 'Universe';

  FOR i := 1 TO 3 DO
  BEGIN
    short2  := standard + ', ' + short1[i] + '!';
    WRITELN(short2);
  END
END.
