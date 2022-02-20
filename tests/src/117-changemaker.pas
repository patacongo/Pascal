(* Make change  for a dollar *)

PROGRAM  ChangeMaker;

  VAR
  Cost:      INTEGER;
  Remainder: INTEGER;
  Quarters:  INTEGER;
  Nickels:   INTEGER;
  Pennies:   INTEGER; 
  Dimes:     INTEGER;

BEGIN
  (* Input the  Cost *)

  Write('Enter the  cost in cents: ');
  Read(Cost);
  
  (* Make the  Change *)

  Remainder := 100 - Cost;
  Quarters  := Remainder DIV 25;
  Remainder := Remainder MOD 25;
  Dimes     := Remainder DIV 10;
  Remainder := Remainder MOD 10;
  Nickels   := Remainder DIV 5;
  Remainder := Remainder MOD 5;
  Pennies   := Remainder;

  (* Output the  coin count *)

  Writeln('The  change is ');
  WriteLn(Quarters:2, ' quarters');
  WriteLn(Dimes:2,    ' dimes');
  WriteLn(Nickels:2,  ' nickels');
  WriteLn(Pennies:2,  ' pennies');
END.