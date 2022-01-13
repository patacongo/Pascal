program exCallbyRef;
var
   a, b : integer;
(*procedure definition *)
procedure swap(var x, y: integer);
var
   temp: integer;
begin
   temp := x;
   x:= y;
   y := temp;
end;

begin
   a := 100;
   b := 200;
   writeln('Before swap, value of a : ', a );
   writeln('Before swap, value of b : ', b );
   (* calling the procedure swap  by value   *)
   swap(a, b);
   writeln('After swap, value of a : ', a );
   writeln('After swap, value of b : ', b );
end.
