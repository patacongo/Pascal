program exPointers;
var
   number: integer;
   iptr: ^integer;
begin
   number := 100;
   writeln('Number is: ', number);
   iptr := @number;
   writeln('iptr points to a value: ', iptr^);
   iptr^ := 200;
   writeln('Number is: ', number);
   writeln('iptr points to a value: ', iptr^);
end.
