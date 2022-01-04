(* Verifies short string concatenation, standard to short string conversion,
   character to short string concatenation *)

PROGRAM shortstrings;
VAR
  standard : string;
  short1   : string[8];
  short2   : string[16];

BEGIN
  standard := 'Hello';
  short1   := 'World';
  short2   := standard + ',' + short1 + '!';
  WRITELN(short2);
END.
