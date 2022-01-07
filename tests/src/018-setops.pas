PROGRAM setoperations;
TYPE
  letters   = 'A' .. 'E';
  letterset = SET OF letters;

VAR
  s1, s2 : letterset;

PROCEDURE showresult(result : letterset);
VAR
  letter : letters;

BEGIN
  WRITE('Card result: ', card(result), 'Result: ');
  FOR letter := 'A' TO 'E' DO
  BEGIN
    IF letter IN result THEN
    BEGIN
      WRITE(letter);
    END
  END;
  WRITELN;
END;
  
BEGIN
  s1 := ['A', 'B', 'C'];
  s2 := ['C', 'D', 'E'];

  WRITE('Card S1: ', card(s1));
  WRITE('Card S2: ', card(s2));

  showresult(S1 + S2);
  showresult(S1 - S2);
  showresult(S1 * S2);
  showresult(S1 >< S2);

  if S1 = S2 THEN
    WRITELN('TRUE')
  ELSE
    WRITELN('FALSE');
  
  if S1 <> S2 THEN
    WRITELN('TRUE')
  ELSE
    WRITELN('FALSE');

  if S1 <= S2 THEN
    WRITELN('TRUE')
  ELSE
    WRITELN('FALSE');

  showresult(INCLUDE(S1, 'D'));
  showresult(EXCLUDE(S2, 'D'));

  if 'E' IN S2 THEN
    WRITELN('TRUE')
  ELSE
    WRITELN('FALSE');
END.
