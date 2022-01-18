PROGRAM pointers;
VAR
   number: INTEGER;
   iptr: ^INTEGER;

BEGIN
   number := 100;
   WRITELN('Number is: ', number);
   
   iptr := @number;
   WRITELN('iptr points to a value: ', iptr^);
   
   iptr^ := 200;
   WRITELN('Number is: ', number);
   WRITELN('iptr points to a value: ', iptr^);
END.