program DataFiles;
type
   StudentRecord = Record
      s_name: String;
      s_addr: String;
      s_batchcode: String;
   end;

var
   Student: StudentRecord;
   f: file of StudentRecord;

begin
   assign(f, 'students.dat');
   reset(f); 
   while not eof(f) do
   
   begin
      read(f,Student);
      writeln('Name: ',Student.s_name);
      writeln('Address: ',Student.s_addr);
      writeln('Batch Code: ', Student.s_batchcode);
   end;
   
   close(f);
end.

When the above code is compiled and executed, it produces the following result −

Name: John Smith
Address: United States of America
Batch Code: Computer Science

Files as Subprogram Parameter

Pascal allows file variables to be used as parameters in standard and user-defined subprograms. The following example illustrates this concept. The program creates a file named rainfall.txt and stores some rainfall data. Next, it opens the file, reads the data and computes the average rainfall.

Please note that, if you use a file parameter with subprograms, it must be declared as a var parameter.

program addFiledata;
const
   MAX = 4;
type
   raindata = file of real;

var
   rainfile: raindata;
   filename: string;
procedure writedata(var f: raindata);

var
   data: real;
   i: integer;

begin
   rewrite(f, sizeof(data));
   for i:=1 to MAX do
   
   begin
      writeln('Enter rainfall data: ');
      readln(data);
      write(f, data);
   end;
   
   close(f);
end;

procedure computeAverage(var x: raindata);
var
   d, sum: real;
   average: real;

begin
   reset(x);
   sum:= 0.0;
   while not eof(x) do
   
   begin
      read(x, d);
      sum := sum + d;
   end;
   
   average := sum/MAX;
   close(x);
   writeln('Average Rainfall: ', average:7:2);
end;

begin
   writeln('Enter the File Name: ');
   readln(filename);
   assign(rainfile, filename);
   writedata(rainfile);
   computeAverage(rainfile);
end.