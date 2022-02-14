PROGRAM AppendFile;
VAR
  myfile: TEXT;
  info: STRING;

BEGIN
  ASSIGN(myfile, 'contact.txt');
  APPEND(myfile);

  WRITELN(myfile, 'Contact Details');
  WRITELN(myfile, 'webmaster@tutorialspoint.com');
  CLOSE(myfile);

  (* let us read from this file *)

  ASSIGN(myfile, 'contact.txt');
  RESET(myfile);

  WHILE NOT EOF(myfile) DO
  BEGIN
    READLN(myfile, info);
    WRITELN(info);
  END;

  CLOSE(myfile);
END.
