{ Verifies compilation of a simple record }

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
   abc.a := 'A';
   abc.b := 'B'; 
   abc.c := 'C';

   writeln ('a = ', abc.a);
   writeln ('b = ', abc.b);
   writeln ('c = ', abc.C);
end.