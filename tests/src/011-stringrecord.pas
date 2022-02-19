{ Verifies that strings in records are initialized properly }

program stringRecord;
type
  StringRecord =
    record
      verbage : string;
      number  : integer;
    end;

var
   StringStuff : StringRecord;

begin
   StringStuff.verbage := 'Some string';
   StringStuff.number  := 42;
   writeln(StringStuff.number, ': ', StringStuff.verbage);
end.
