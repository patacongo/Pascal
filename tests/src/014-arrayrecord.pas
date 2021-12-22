program arrayRecord;
type
NameString =
  record
     title: packed array [1..50] of char;
     info:  packed array [1..50] of char;
  end;

var
   Names : array[1..3] of NameString;

procedure printBook( var name : NameString );
begin
   writeln(name.tile, ': ', name.info);
end;

begin
   Names[1].tile := 'Name 1';
   Names[1].info := 'This is name 1';

   Names[2].tile := 'Name 2';
   Names[2].info := 'This is name 2';

   Names[3].tile := 'Name 3';
   Names[3].info := 'This is name 3';

   printName(Names[1]);
   printName(Names[2]);
   printName(Names[3]);
end
