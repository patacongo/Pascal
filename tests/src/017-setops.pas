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
  WRITE(' = [');
  FOR letter := 'A' TO 'E' DO
  BEGIN
    IF letter IN result THEN
    BEGIN
      WRITE(letter);
    END
  END;
  WRITELN('] Card: ', CARD(result));
END;
  
BEGIN
  s1 := ['A'..'C'];
  s2 := ['C'..'E'];

  WRITE('Initial S1'); showresult(S1);
  WRITE('Initial S2'); showresult(S2);
  WRITE('Card S2: ', CARD(s2));

  WRITE('S1 + S2'); showresult(S1 + S2);
  WRITE('S1 - S2'); showresult(S1 - S2);
  WRITE('S1 * S2'); showresult(S1 * S2);
  WRITE('S1 >< S2'); showresult(S1 >< S2);

  WRITE('S1 = S2 is ');
  if S1 = S2 THEN
    WRITELN('TRUE')
  ELSE
    WRITELN('FALSE');
  
  WRITE('S1 <> S2 is ');
  if S1 <> S2 THEN
    WRITELN('TRUE')
  ELSE
    WRITELN('FALSE');

  WRITE('S1 <= S2 is ');
  if S1 <= S2 THEN
    WRITELN('TRUE')
  ELSE
    WRITELN('FALSE');

  WRITE('INCLUDE(S1, D)'); showresult(INCLUDE(S1, 'D'));
  WRITE('EXCLUDE(S2, D)'); showresult(EXCLUDE(S2, 'D'));

  WRITE('E IN S2 is ');
  if 'E' IN S2 THEN
    WRITELN('TRUE')
  ELSE
    WRITELN('FALSE');
END.
