program bitwise;
var
   a, b, c: integer;

begin
   a := 60;         (* 60 = 0011 1100 *)  
   b := 13;         (* 13 = 0000 1101 *)
   c := 0;           

   c := a and b;    (* 12 = 0000 1100 *)
   writeln('Line 1 - Value of c is  ', c );

   c := a or b;     (* 61 = 0011 1101 *)
   writeln('Line 2 - Value of c is  ', c );

   c := not a;      (* -61 = 1100 0011 *)
   writeln('Line 3 - Value of c is  ', c );

   c := a << 2;     (* 240 = 1111 0000 *)
   writeln('Line 4 - Value of c is  ', c );

   c := a >> 2;     (* 15 = 0000 1111 *)
   writeln('Line 5 - Value of c is  ', c );
end.
