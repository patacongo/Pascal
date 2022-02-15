PROGRAM VarValStrings;

TYPE
  NameType = STRING[8];
  NameArrayType = ARRAY[1..2] OF NameType;

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
  WRITELN('Names: ', Names[1], ' and ', Names[2])
END.
