(* Verifies simple string concatenation, character to string concatenation *)

PROGRAM helloworld;
VAR
  hello : string

BEGIN
  hello := 'Hello';
  hello := hello + ',';
  hello := hello + ' ';
  hello := hello + 'World!';
  WRITELN(hello);
END.
