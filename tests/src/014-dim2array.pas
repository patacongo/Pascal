PROGRAM dim2array;
VAR 
   a: ARRAY [0..3, 0..3] OF INTEGER;
   i, j : INTEGER;  

BEGIN  
   FOR i := 0 TO 3 DO
      FOR j:= 0 TO 3 DO
         a[i, j] := i * j;  
   
   FOR i := 0 TO 3 DO
   begin  
      FOR j := 0 TO 3 DO  
         WRITE(a[i, j], ' ');  
      WRITELN;  
   END;  
END.