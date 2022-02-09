PROGRAM PointerRecordExample(INPUT, OUTPUT);

TYPE
  rptr = ^recdata;
  recdata = RECORD
    number : INTEGER;
    code : STRING;
    nextrecord : rptr
  END;

VAR
 startrecord, listrecord, insertptr : rptr;
 digitcode : INTEGER;
 textstring : STRING;
 exitflag, first : BOOLEAN;

BEGIN
 exitflag := FALSE;
 first := TRUE;

 WHILE exitflag = FALSE DO
 BEGIN
  WRITELN('Enter in a digit [-1 to end]');
  READLN(digitcode);
  if digitcode = -1 THEN
   exitflag := TRUE
  ELSE
  BEGIN
   WRITELN('Enter in a small text string');
   READLN(textstring);
   NEW(insertptr);
   IF insertptr = NIL THEN
   BEGIN
    WRITELN('1: unable to allocate storage space');
    HALT
   END;
   IF first = TRUE THEN BEGIN
    startrecord := insertptr;
    listrecord := insertptr;
    first := FALSE
   END
   ELSE BEGIN
    listrecord^.nextrecord := insertptr;
    listrecord := insertptr
   END;
   insertptr^.number := digitcode;
   insertptr^.code := textstring;
   insertptr^.nextrecord := NIL
  END
 END;

 WHILE startrecord <> NIL DO
 BEGIN
  listrecord := startrecord;
  WRITELN(startrecord^.number);
  WRITELN(startrecord^.code);
  startrecord := startrecord^.nextrecord;
  DISPOSE(listrecord)
 END
END.
