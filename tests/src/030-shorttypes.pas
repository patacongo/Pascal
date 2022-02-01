{ Used out-of-range constants to explore the range of short integer and word
  types.  Otherwise, this is a clone of 029-wordtype.pas }

PROGRAM ShortTypes;

TYPE
  sptr = ^SHORTINTEGER;
  uptr = ^SHORTWORD;
VAR
  int1, int2   : SHORTINTEGER;
  word1, word2 : SHORTWORD;
  int1ptr      : sptr;
  word1ptr     : uptr;

BEGIN
  { Set both to the range of short signed integer }

  int1  := SHORTINTEGER(-128);
  int2  := SHORTINTEGER(127);
  word1 := SHORTWORD(-128);
  word2 := SHORTWORD(127);

  WRITELN(' int1  = ',  int1:6, ' int2  = ',  int2:6,
          ' word1 = ', word1:6, ' word2 = ', word2:6);

  { Set both to the range of short unsigned word }

  int1  := SHORTINTEGER(0);
  int2  := SHORTINTEGER(255);
  word1 := SHORTWORD(0);
  word2 := SHORTWORD(255);

  WRITELN(' int1  = ',  int1:6, ' int2  = ',  int2:6,
          ' word1 = ', word1:6, ' word2 = ', word2:6);

  { Set both to the their limits, the increment/decrement beyond }

  int1  := SHORTINTEGER(-128);
  int2  := SHORTINTEGER(127);
  word1 := SHORTWORD(0);
  word2 := SHORTWORD(255);

  WRITELN(' int1  = ',  int1:6, ' int2  = ',  int2:6,
          ' word1 = ', word1:6, ' word2 = ', word2:6);
  WRITELN(' int1  = ',  (int1 - 1):6, ' int2  = ', (int2  + 1):6,
          ' word1 = ', (word1 - 1):6, ' word2 = ', (word2 + 1):6);

  { Unrelated check of word and integer pointers }

  int1ptr   := sptr(@int1);
  int1ptr^  := SHORTINTEGER(100);
  word1ptr  := uptr(@word1);
  word1ptr^ := SHORTWORD(200);

  WRITELN(' int1  = ',  int1:6, ' sptr^ = ', int1ptr^);
  WRITELN(' word1 = ', word1:6, ' uptr^ = ', word1ptr^);
END.
