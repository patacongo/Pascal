PROGRAM StringOperations;

VAR
  string1, string2 : STRING;
  value, index : INTEGER

BEGIN
  string1 := 'This is a string';
  WRITELN('[', string1, '] Length=', LENGTH(string1));
  WRITELN('[', string1, '] substring at 6, length 2=[', COPY(string1, 6, 2), ']');
  WRITELN('[', string1, '] First ''is'' is at offset=', POS('is', string1));

  string2 := '12345';
  VAL(string2, value, index);
  WRITELN('[', string2, '] Value is ', value, ' Terminator is ', index);
END.
