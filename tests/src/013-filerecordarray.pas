program fileRecord;
type
FileInfo =
  record
     name: string;
     f:    text;
  end;

var
   Files : array[1..3] of NameString;

begin
   Files[1].name := 'File1.dat';
   Files[2].name := 'File2".dat';
   Files[3].name := 'File3".dat';

   AssignFile(Files[1].f, Files[1].name);
   AssignFile(Files[2].f, Files[2].name);
   AssignFile(Files[3].f, Files[3].name);
end