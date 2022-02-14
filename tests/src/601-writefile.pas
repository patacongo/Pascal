PROGRAM DataFiles;
TYPE
   StudentRecord = RECORD
      s_name: PACKED ARRAY[1..16] OF CHAR;
      s_addr: PACKED ARRAY[1..32] OF CHAR;
      s_batchcode: PACKED ARRAY[1..16] OF CHAR;
   END;

VAR
   Student: StudentRecord;
   f: FILE OF StudentRecord;

BEGIN
   ASSIGN(f,'students.dat');
   REWRITE(f);
   Student.s_name := 'John Smith';
   Student.s_addr := 'United States of America';
   Student.s_batchcode := 'Computer Science';
   WRITE(f,Student);
   CLOSE(f);
END.
