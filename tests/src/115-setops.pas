PROGRAM setoperations;
TYPE
  letters   = (A, B, C, D, E);
  letterset = SET OF letters;

VAR
  s1, s2 : letterset;

PROCEDURE showresult(set : letterset)
VAR
  letter : letters;

BEGIN
  FOR letter := A TO B DO
  BEGIN
    IF letter IN set THEN
    BEGIN
      WRITE(letter);
    END
  END;
  WRITELN;
END;
  
BEGIN
  s1 = [A, B, C];
  s2 = [C, D, E];

  showresult(S1 + S2);
  showresult(S1 - S2);
  showresult(S1 * S2);
  showresult(S1 >< S2);

  if S1 = S2 THEN
    WRITELN('TRUE');
  ELSE
    WRITELN('FALSE');
  
  if S1 <> S2 THEN
    WRITELN('TRUE');
  ELSE
    WRITELN('FALSE');

  if S1 <= S2 THEN
    WRITELN('TRUE');
  ELSE
    WRITELN('FALSE');

  showresult(INCLUDE(S1, [D]);
  showresult(EXCLUDE(S2, [D]);

  if [E] IN S2 THEN
    WRITELN('TRUE');
  ELSE
    WRITELN('FALSE');
END.
