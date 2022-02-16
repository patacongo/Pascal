PROGRAM VarValStrings;

TYPE
  NameType = STRING[8];
  NameArrayType = ARRAY[1..3] OF NameType;

VAR
  Names : NameArrayType;
  Name : NameType;

PROCEDURE SaveName1(VAR SomeNames: NameArrayType; AName: NameType; NameIndex : INTEGER);
BEGIN
  SomeNames[NameIndex] := AName
END;

PROCEDURE SaveName2(VAR SomeNames: NameArrayType; VAR AName: NameType; NameIndex : INTEGER);
BEGIN
  SomeNames[NameIndex] := AName
END;

BEGIN
  Name := 'Marge';
  SaveName1(Names, Name, 1);
  Name := 'Wilbur';
  SaveName2(Names, Name, 2);
  SaveName1(Names, 'Barry', 3);
  WRITELN('Names: ', Names[1], ', ', Names[2], ', and ', Names[3])
END.
