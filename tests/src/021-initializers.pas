PROGRAM initializers;
TYPE
  stooges = (larry, curly, moe)

VAR
  intval : INTEGER = 42;
  boolval : BOOLEAN = true;
  stooge : stooges = moe;
  pi : REAL = 3.14159;
  hello : STRING[8] = 'hello';
  pointer : ^INTEGER = NIL

BEGIN
  WRITELN('intval = ', intval);
  IF boolval THEN
    WRITELN('boolval is TRUE')
  ELSE
    WRITELN('boolval is FALSE');

  WRITELN('pi = ', pi:7:3);
  WRITELN('hello = "', hello, '"');
END.
