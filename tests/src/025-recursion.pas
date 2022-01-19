PROGRAM recursion;
  VAR
    level0 : INTEGER;

  PROCEDURE nest1(value : INTEGER);
    VAR
      level1 : INTEGER;

    PROCEDURE nest2(value : INTEGER);
      VAR
        level2 : INTEGER;
      BEGIN
        level2 := value;
        WRITELN('nest2: ', level0, ', ', level1, ', ', level2);
        if (value <= 3) THEN
          nest2(value + 1)
      END;

    BEGIN
      level1 := value;
      WRITELN('nest1: ', level0, ', ', level1);
      nest2(0);
      if (value <= 3) THEN
        nest1(value + 1)
    END;

  BEGIN
    FOR level0 := 0 TO 3 DO
    BEGIN
      WRITELN('nest0: ', level0);
      nest1(0);
    END
  END.
