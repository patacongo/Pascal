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
  VAL(string1, value, index);
  WRITELN('[', string2, '] Value is ', value, ' Terminator is ', index);

  DELETE(string1, 11, 2);
  WRITELN('[', string1, '] Deleted index 11, Lengh 2');

  DELETE(string1, 8, 1);
  WRITELN('[', string1, '] Deleted index 8, Lengh 1');

  DELETE(string1, 1, 1);
  WRITELN('[', string1, '] Deleted index 1, Lengh 1');

  string1 := 'This is a string';
  WRITELN('[', string1, '] Length=', LENGTH(string1));

  INSERT('not ', string1, 9);
  WRITELN('[', string1, '] Inserted ''not''');

  DELETE(string1, 15, 6);
  INSERT('dog.', string1, 15);
  WRITELN('[', string1, '] Replace ''string'' with ''dog''');

  DELETE(string1, 1, 4);
  INSERT('He', string1, 1);
  WRITELN('[', string1, '] Replace ''This'' with ''He''');
END.
