{ writeln with a variety of arguments }

PROGRAM DoWriteln(output);

CONST
  floater      = 1.8;
  low          = 37;
  high         = 492;
  range_string = ' range=';

VAR
  range : low..high;

BEGIN
  range := (low + high) DIV 2;
  WRITELN('A', range_string, range, ' B floater=', floater);
END.
