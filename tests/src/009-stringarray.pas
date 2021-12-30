{ Verifies that strings in arrays are initialized properly }

program stringArray;
var
   lines : packed array[0..1] of string;

begin
   lines[0] := 'Now is the time';
   lines[0] := lines[0] + ' for all good men';
   lines[1] := ' of their party.';
   lines[1] := 'To come to the aid' + lines[1];

   writeln(lines[0]);
   writeln(lines[1]);
end.