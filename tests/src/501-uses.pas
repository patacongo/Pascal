program MyProgram;

uses
   MyCosineUnit;
   MySineUnit;
   MyDataUnit;

var
   x : real;

begin
   write('Enter radians	: ');
   read(x);
   mycosx := mycosine(x);
   writeln('cos(', x, ')=', mycosx);
   mysinx := mysine(x);
   writeln('sin(', x, ')=', mysinx);
   checkvars;
   writeln('sin(', x, ')**2 + cos(', x, ')**2=', myone)
end.
