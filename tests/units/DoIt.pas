PROGRAM DoIt;

USES
   SayIt IN 'SayIt.pas';

VAR
   message : STRING;

BEGIN
  secret  := 42;
  message := 'Hello, World!';
  SayIt(message);
END.
