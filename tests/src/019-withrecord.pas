{ Verifies compilation of a simple record using WITH selector }

program simpleRecord;
type
letters =
  record
    a : char;
    b : char;
    c : char;
  end;

var
   abc : letters;

begin
   with abc do
   begin
      a := 'A';
      b := 'B'; 
      c := 'C';

      writeln ('a = ', a);
      writeln ('b = ', b);
      writeln ('c = ', C)
   end;
end.
