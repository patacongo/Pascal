{ Verifies that strings in arrays are initialized properly }

program stringArray;
var
   lines : packed array[0..1] of string;

begin
   lines[0] := 'Now is the time';
   lines[1] := lines[0] + ' for all good men';
   lines[2] := ' of their party.';
   lines[2] := 'To come to the aid' + lines[2];

   writeln (lines[0]);
   writeln (lines[1]);
end.