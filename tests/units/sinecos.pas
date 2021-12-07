program MyProgram;

uses
   MyCosineUnit in 'unit-cosine.pas';
   MySineUnit in 'unit-sine.pas';
   MyDataUnit in 'unit-data.pas';

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
