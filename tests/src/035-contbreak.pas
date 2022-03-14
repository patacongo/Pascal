PROGRAM ContinueBreak;

  PROCEDURE TestForLoop;
  VAR
    n : Integer

  BEGIN
    WRITELN;
    WRITELN('FOR-DO loop with IF_THEN-ELSE');

    FOR n := 1 TO 10 DO
    BEGIN
      IF (n = 2) OR (n = 4) THEN
      BEGIN
        WRITELN('- Continuing at ', n);
        Continue
      END
      ELSE IF n = 6 THEN
      BEGIN
        WRITELN('- Breaking at ', n);
        Break
      END;

      WRITELN('- End of loop at ', n)
    END;

    WRITELN('- Out of the loop at ', n);
    WRITELN;
    WRITELN('FOR-DO loop with CASE');

    FOR n := 1 TO 10 DO
    BEGIN
      CASE n OF
        2, 4:
          BEGIN
            WRITELN('- Continuing at ', n);
            Continue;
          END;
        6 :
          BEGIN
            WRITELN('- Breaking at ', n);
            Break;
          END;
      END;

      WRITELN('- End of loop at ', n)
    END;

    WRITELN('- Out of the loop at ', n)
  END;

  PROCEDURE TestWhileLoop;
  VAR
    n : Integer

  BEGIN
    WRITELN;
    WRITELN('WHILE-DO loop with IF_THEN-ELSE');

    n := 1;
    WHILE n < 10 DO
    BEGIN
      IF (n = 2) OR (n = 4) THEN
      BEGIN
        WRITELN('- Continuing at ', n);
        n := n + 1;
        Continue
      END
      ELSE IF n = 6 THEN
      BEGIN
        WRITELN('- Breaking at ', n);
        Break
      END;

      WRITELN('- End of loop at ', n);
      n := n + 1
    END;

    WRITELN('- Out of the loop at ', n);
    WRITELN;
    WRITELN('WHILE-DO loop with CASE');

    n := 1;
    WHILE n < 10 DO
    BEGIN
      CASE n OF
        2, 4:
          BEGIN
            WRITELN('- Continuing at ', n);
            n := n + 1;
            Continue;
          END;
        6 :
          BEGIN
            WRITELN('- Breaking at ', n);
            Break;
          END;
      END;

      WRITELN('- End of loop at ', n);
      n := n + 1
    END;

    WRITELN('- Out of the loop at ', n)
  END;

  PROCEDURE TestRepeatLoop;
  VAR
    n : Integer

  BEGIN
    WRITELN;
    WRITELN('REPEAT-UNTIL  loop with IF_THEN-ELSE');

    n := 1;
    REPEAT
      IF (n = 2) OR (n = 4) THEN
      BEGIN
        WRITELN('- Continuing at ', n);
        n := n + 1;
        Continue
      END
      ELSE IF n = 6 THEN
      BEGIN
        WRITELN('- Breaking at ', n);
        Break
      END;

      WRITELN('- End of loop at ', n);
      n := n + 1
    UNTIL n >= 10;

    WRITELN('- Out of the loop at ', n);
    WRITELN;
    WRITELN('REPEAT-UNTIL loop with CASE');

    n := 1;
    REPEAT
      CASE n OF
        2, 4:
          BEGIN
            WRITELN('- Continuing at ', n);
            n := n + 1;
            Continue;
          END;
        6 :
          BEGIN
            WRITELN('- Breaking at ', n);
            Break;
          END
      END;

    WRITELN('- End of loop at ', n);
    n := n + 1
    UNTIL n >= 10;

    WRITELN('- Out of the loop at ', n)
  END;

BEGIN
  TestForLoop;
  TestWhileLoop;
  TestRepeatLoop;
END.
