PROGRAM globalScope;
VAR
   a, b, c: INTEGER;

PROCEDURE display;
VAR
   x, y, z: INTEGER;

BEGIN
   (* local variables *)
   x := 10;
   y := 20;
   z := x + y;

  (*global variables *)
   a := 30;
   b := 40;
   c := a + b;

   WRITELN('Within the procedure display');
   WRITELN(' Displaying the global variables a, b, and c');
   WRITELN('value of a = ', a, ' b = ',  b, ' and c = ', c);
   WRITELN('Displaying the local variables x, y, and z');
   WRITELN('value of x = ', x, ' y = ',  y, ' and z = ', z);
END;

BEGIN
   a := 100;
   b := 200;
   c := 300;

   WRITELN('Within the program exlocal');
   WRITELN('value of a = ', a , ' b = ',  b, ' and c = ', c);
   display();
END.
