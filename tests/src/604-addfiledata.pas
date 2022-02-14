PROGRAM addFiledata;
CONST
  MAX = 4;

TYPE
  raindata = FILE OF REAL;

VAR
  rainfile: raindata;
  filename: STRING;

PROCEDURE writedata(VAR f: raindata);

VAR
  data: REAL;
  i: INTEGER;

BEGIN
  rewrite(f, sizeof(data));

  FOR i := 1 TO MAX DO
  begin
    WRITELN('Enter rainfall data: ');
    READLN(data);
    WRITE(f, data);
  END;

  CLOSE(f);
END;

PROCEDURE computeAverage(VAR x: raindata);
VAR
  d, sum: REAL;
  average: REAL;

begin
  RESET(x);
  sum :=  0.0;

  WHILE NOT EOF(x) DO
  BEGIN
    READ(x, d);
    sum := sum + d;
  END;

  average := sum/MAX;
  CLOSE(x);
  WRITELN('Average Rainfall: ', average:7:2);
END;

BEGIN
  WRITELN('Enter the File Name: ');
  READLN(filename);
  ASSIGN(rainfile, filename);
  WRITEDATA(rainfile);
  computeAverage(rainfile);
END.
