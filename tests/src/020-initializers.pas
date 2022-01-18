PROGRAM initializers;
TYPE
  stooges = (larry, curly, moe);
  stoogeset = SET OF stooges;

VAR
  intval : INTEGER = 5 * 8 + 2;
  boolval : BOOLEAN = true;
  stooge : stooges = moe;
  curlyset : stoogeset = [curly];
  otherstooges : stoogeset = [larry..moe] >< [curly];
  pi : REAL = 3.14159;
  hello : STRING[8] = 'hello';
  pointer : ^INTEGER = NIL

BEGIN
  WRITELN('intval = ', intval);
  IF boolval THEN
    WRITELN('boolval is TRUE')
  ELSE
    WRITELN('boolval is FALSE');

  WRITELN('CARD(curlyset) = ', CARD(curlyset));
  WRITELN('CARD(otherstooges) = ', CARD(otherstooges));
  WRITELN('pi = ', pi:7:3);
  WRITELN('hello = "', hello, '"');
END.
