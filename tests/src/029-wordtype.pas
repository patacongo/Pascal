{ Used out-of-range constants to explore the range of integer and word types. }

PROGRAM wordType;
VAR
  int1, int2 : INTEGER;
  word1, word2 : WORD;
BEGIN
  int1  := INTEGER(-32768);
  int2  := INTEGER(32767);
  word1 := WORD(-32768);
  word2 := WORD(32767);

  WRITELN(' int1 = ',  int1:6,  ' int2 = ',  int2:6,
          ' word1 = ', word1:6, ' word2 = ', word2:6);

  int1  := INTEGER(0);
  int2  := INTEGER(65535);
  word1 := WORD(0);
  word2 := WORD(65535);

  WRITELN(' int1 = ',  int1:6,  ' int2 = ',  int2:6,
          ' word1 = ', word1:6, ' word2 = ', word2:6);

  int1  := INTEGER(-32768);
  int2  := INTEGER(32767);
  word1 := WORD(0);
  word2 := WORD(65535);

  WRITELN(' int1 = ',  int1:6,  ' int2 = ',  int2:6,
          ' word1 = ', word1:6, ' word2 = ', word2:6);
  WRITELN(' int1 = ',  (int1  - 1):6, ' int2 = ',  (int2  + 1):6,
          ' word1 = ', (word1 - 1):6, ' word2 = ', (word2 + 1):6);
END.
