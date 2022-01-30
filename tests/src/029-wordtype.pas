{ Used out-of-range constants to explore the range of integer and word types.
  Also uses lots of type casting to verify that feature and some pointer casts. }

PROGRAM wordType;

TYPE
  intptr = ^INTEGER;
  wordptr = ^WORD;
VAR
  int1, int2 : INTEGER;
  word1, word2 : WORD;
  int1ptr : intptr;
  word1ptr : wordptr;

BEGIN
  { Set both to the range of signed integer }

  int1  := INTEGER(-32768);
  int2  := INTEGER(32767);
  word1 := WORD(-32768);
  word2 := WORD(32767);

  WRITELN(' int1  = ',  int1:6,  ' int2  = ',  int2:6,
          ' word1 = ', word1:6, ' word2 = ', word2:6);

  { Set both to the range of unsigned word }

  int1  := INTEGER(0);
  int2  := INTEGER(65535);
  word1 := WORD(0);
  word2 := WORD(65535);

  WRITELN(' int1  = ',  int1:6,  ' int2  = ',  int2:6,
          ' word1 = ', word1:6, ' word2 = ', word2:6);

  { Set both to the their limits, the increment/decrement beyond }

  int1  := INTEGER(-32768);
  int2  := INTEGER(32767);
  word1 := WORD(0);
  word2 := WORD(65535);

  WRITELN(' int1  = ',  int1:6,  ' int2  = ',  int2:6,
          ' word1 = ', word1:6, ' word2 = ', word2:6);
  WRITELN(' int1  = ',  (int1  - 1):6, ' int2  = ',  (int2  + 1):6,
          ' word1 = ', (word1 - 1):6, ' word2 = ', (word2 + 1):6);

  { Unrelated check of word and integer pointers }

  int1ptr   := intptr(@int1);
  int1ptr^  := INTEGER(100);
  word1ptr  := wordptr(@word1);
  word1ptr^ := WORD(200);

  WRITELN(' int1  = ', int1:6,  ' intptr^  = ',  int1ptr^);
  WRITELN(' word1 = ', word1:6, ' wordptr^ = ', word1ptr^);
END.
