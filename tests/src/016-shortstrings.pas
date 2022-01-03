(* Verifies short string concatenation, character to short string concatenation *)

PROGRAM shortstrings;
VAR
  hello   : string[5];
  world   : string[8];
  message : string[16];

BEGIN
  hello   := 'Hello';
  world   := 'World';
  message := hello, ',', world, '!';
  WRITELN(message);
END.
