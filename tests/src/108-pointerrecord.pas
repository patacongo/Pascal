program PointerRecordExample( input, output );

type
  rptr = ^recdata;
  recdata = record
    number : integer;
    code : string;
    nextrecord : rptr
  end;

var
 startrecord, listrecord, insertptr : rptr;
 digitcode : integer;
 textstring : string;
 exitflag, first : boolean;

begin
 exitflag := false;
 first := true;
 while exitflag = false do
 begin
  writeln('Enter in a digit [-1 to end]');
  readln( digitcode );
  if digitcode = -1 then
   exitflag := true
  else
  begin
   writeln('Enter in a small text string');
   readln( textstring );
   new( insertptr );
   if insertptr = nil then
   begin
    writeln('1: unable to allocate storage space');
    exit
   end;
   if first = true then begin
    startrecord := insertptr;
    listrecord := insertptr;
    first := false
   end
   else begin
    listrecord^.nextrecord := insertptr;
    listrecord := insertptr
   end;
   insertptr^.number := digitcode;
   insertptr^.code := textstring;
   insertptr^.nextrecord := nil
  end
        end;
        while startrecord <> nil do
        begin
      listrecord := startrecord;
             writeln( startrecord^.number );
      writeln( startrecord^.code );
      startrecord := startrecord^.nextrecord;
      dispose( listrecord )
 end
end.