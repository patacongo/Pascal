{ Verifies compilation of a simple record using WITH selector }

PROGRAM simpleRecord;
TYPE
  letters =
    record
      a : char;
      b : char;
      c : char;
    END;

VAR
   abc : letters;
   pabc : ^letters;

PROCEDURE ghi(VAR vabc : letters);
BEGIN
   WITH vabc DO
   BEGIN
      a := 'G';
      b := 'H';
      c := 'I';

      WRITELN('a = ', a);
      WRITELN('b = ', b);
      WRITELN('c = ', c)
   END;
END;

BEGIN
   WITH abc DO
   BEGIN
      a := 'A';
      b := 'B';
      c := 'C';

      WRITELN('a = ', a);
      WRITELN('b = ', b);
      WRITELN('c = ', c)
   END;

   WITH pabc^ DO
   BEGIN
      a := 'D';
      b := 'E';
      c := 'F';

      WRITELN('a = ', a);
      WRITELN('b = ', b);
      WRITELN('c = ', c)
   END;

   ghi(abc);
END.
