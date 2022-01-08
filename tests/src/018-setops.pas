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
  WRITE('Card result: ', CARD(result), ' Result: [');
  FOR letter := 'A' TO 'E' DO
  BEGIN
    IF letter IN result THEN
    BEGIN
      WRITE(letter);
    END
  END;
  WRITELN(']');
END;
  
BEGIN
  s1 := ['A', 'B', 'C'];
  s2 := ['C', 'D', 'E'];

  WRITELN('Card S1: ', CARD(s1));
  WRITELN('Card S2: ', CARD(s2));

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
