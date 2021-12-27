program exAppendfile;
var
   myfile: text;
   info: string;

begin
   assign(myfile, 'contact.txt');
   append(myfile);
   
   writeln(myfile, 'Contact Details');
   writeln(myfile, 'webmaster@tutorialspoint.com');
   close(myfile);
   
   (* let us read from this file *)

   assign(myfile, 'contact.txt');
   reset(myfile);

   while not eof(myfile) do
   begin
      readln(myfile, info);
      writeln(info);
   end;

   close(myfile);
end.
