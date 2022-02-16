{ Verifies that strings in arrays are initialized properly }

PROGRAM StringArray;
VAR
   lines : PACKED ARRAY[0..1] OF STRING;

BEGIN
   lines[0] := 'Now is the time';
   lines[0] := lines[0] + ' for all good men';
   WRITELN(lines[0]);

   lines[1] := ' of their party.';
   lines[1] := 'To come to the aid' + lines[1];
   WRITELN(lines[1]);
END.
