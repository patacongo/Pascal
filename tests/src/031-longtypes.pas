{ Used out-of-range constants to explore the range of long integer and word types.
  Also uses lots of type casting to verify that feature and some pointer casts. }

PROGRAM wordType;

TYPE
  lintptr = ^LONGINTEGER;
  lwordptr = ^LONGWORD;

VAR
  lint1, lint2 : LONGINTEGER;
  lword1, lword2 : LONGWORD;
  lint1ptr : lintptr;
  lword1ptr : lwordptr;

BEGIN
  { Set both to the range of signed integer }

  lint1  := LONGINTEGER(-2147483648);
  lint2  := LONGINTEGER(2147483647);
  lword1 := LONGWORD(-2147483648);
  lword2 := LONGWORD(2147483647);

  WRITELN(' lint1  = ',  lint1:12,  ' lint2  = ',  lint2:12,
          ' lword1 = ', lword1:12, ' lword2 = ', lword2:12);

  { Set both to the range of unsigned word }

  lint1  := LONGINTEGER(0);
  lint2  := LONGINTEGER(4294967295);
  lword1 := LONGWORD(0);
  lword2 := LONGWORD(4294967295);

  WRITELN(' lint1  = ',  lint1:12,  ' lint2  = ',  lint2:12,
          ' lword1 = ', lword1:12, ' lword2 = ', lword2:12);

  { Set both to the their limits, the increment/decrement beyond }

  lint1  := LONGINTEGER(-2147483648);
  lint2  := LONGINTEGER(2147483647);
  lword1 := LONGWORD(0);
  lword2 := LONGWORD(4294967295);

  WRITELN(' lint1  = ',  lint1:12,  ' lint2  = ',  lint2:12,
          ' lword1 = ', lword1:12, ' lword2 = ', lword2:12);

  lint1  := lint1  - 1;
  lint2  := lint2  + 1;
  lword1 := lword1 - 1;
  lword2 := lword2 + 1;

  WRITELN(' lint1  = ',  lint1:12,  ' lint2  = ',  lint2:12,
          ' lword1 = ', lword1:12, ' lword2 = ', lword2:12);

  { Unrelated check of word and integer pointers }

  lint1ptr   := lintptr(@lint1);
  lint1ptr^  := LONGINTEGER(100);
  lword1ptr  := lwordptr(@lword1);
  lword1ptr^ := LONGWORD(200);

  WRITELN(' lint1  = ', lint1:12,  ' lintptr^  = ',  lint1ptr^);
  WRITELN(' lword1 = ', lword1:12, ' lwordptr^ = ', lword1ptr^);
END.
