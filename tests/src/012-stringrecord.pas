{ Verifies that strings in records are initialized properly }

program stringRecord;
type
  StringRecord =
    record
      str    : string;
      number : integer;
    end;

var
   StringStuff : StringRecord;

begin
   StringStuff.str    := 'Some string';
   StringStuff.number := 42;
   writeln(StringStuff.number, ': ', StringStuff.str);
end.
