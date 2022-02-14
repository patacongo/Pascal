PROGRAM DataFiles;
TYPE
   StudentRecord =
   RECORD
      s_name: PACKED ARRAY[1..16] OF CHAR;
      s_addr: PACKED ARRAY[1..32] OF CHAR;
      s_batchcode: PACKED ARRAY[1..16] OF CHAR;
   END;

VAR
   Student: StudentRecord;
   f: FILE OF StudentRecord;

BEGIN
   ASSIGN(f, 'students.dat');
   RESET(f);

   WHILE NOT EOF(f) DO
   BEGIN
      READ(f, Student);
      WRITELN('Name: ', Student.s_name);
      WRITELN('Address: ', Student.s_addr);
      WRITELN('Batch Code: ', Student.s_batchcode);
   END;

   CLOSE(f);
END.

