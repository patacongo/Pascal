PROGRAM StringOperations;

VAR
  string1, string2, string3 : STRING;
  sstring1, sstring2 : STRING[8];
  value, position, code : INTEGER;
  fvalue : REAL

BEGIN
  { LENGTH, COPY, POS }

  string1 := 'This is a string';
  WRITELN('[', string1, '] Length=', LENGTH(string1));
  WRITELN('[', string1, '] substring at 6, length 2=[', COPY(string1, 6, 2), ']');
  WRITELN('[', string1, '] First ''is'' is at position=', POS('is', string1));
  WRITELN;

  { VAL, STR}

  string2 := '12345';
  VAL(string1, value, code);
  WRITELN('[', string2, '] Value is ', value, ' Terminator is ', code);

  value   := 123;
  fvalue  := 987.654;

  string1 := '';
  STR(value, string1);
  WRITELN('[', string1, '] Converted from ', value);

  string1 := '';
  STR(fvalue, string1);
  WRITELN('[', string1, '] Converted from ', fvalue);

  string1 := '';
  STR(value:10, string1);
  WRITELN('[', string1, '] Converted from ', value, ':10');

  string1 := '';
  STR(fvalue:10:4, string1);
  WRITELN('[', string1, '] Converted from ', fvalue, ':10:4');
  WRITELN;

  { DELETE }

  string1 := 'This is a string';
  WRITELN('[', string1, '] Length=', LENGTH(string1));

  DELETE(string1, 11, 2);
  WRITELN('[', string1, '] Deleted position 11, Length 2');

  DELETE(string1, 8, 1);
  WRITELN('[', string1, '] Deleted position 8, Length 1');

  DELETE(string1, 1, 1);
  WRITELN('[', string1, '] Deleted position 1, Length 1');
  WRITELN;

  { INSERT, DELETE }

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
  WRITELN;

  { CONCAT }

  string2  := 'Hi, ';
  string3  := 'my name ';
  sstring1 := 'is ';
  sstring2 := 'Andy';
  string1  := CONCAT(string2, string3, sstring1, sstring2);
  WRITELN('[', string1, '] Four strings concatenated');
  WRITELN;

  { FILLCHAR }

  string1  := 'Lotsa:';
  FILLCHAR(string1, 20, ORD('x'));
  WRITELN('[', string1, '] Padded to length 20 with ''x''');
  WRITELN;
END.
