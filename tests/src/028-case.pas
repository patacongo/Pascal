PROGRAM checkcase;
VAR
  grade: CHAR;

BEGIN
  grade := 'A';

  CASE (grade) OF
    'A' : WRITELN('Excellent!' );
    'B', 'C': WRITELN('Well done' );
    'D' : WRITELN('You passed' );
    'F' : WRITELN('Better try again' );
  END;     

  WRITELN('Your grade is  ', grade );
END.