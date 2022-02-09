{ This file tests only that data can be shared correctly between units }

UNIT SayIt;

INTERFACE

VAR
   secret : INTEGER;

PROCEDURE SayIt(message : STRING);

IMPLEMENTATION

PROCEDURE SayIt(message : STRING);
BEGIN
   WRITELN(message, secret);
END;
END.
